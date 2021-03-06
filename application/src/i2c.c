#include "i2c.h"

#include <libopencm3/stm32/f0/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/dma.h>

#include <FreeRTOS.h>
#include <semphr.h>

#include "clock.h"

static SemaphoreHandle_t semaphore_handle;

static inline bool i2c1_irq_active(void)
{
    static const uint32_t mask = I2C_ISR_ARLO | I2C_ISR_BERR | I2C_ISR_OVR |
                                 I2C_ISR_PECERR | I2C_ISR_TIMEOUT |
                                 I2C_ISR_ALERT | I2C_ISR_NACKF | I2C_ISR_TC;
    return (I2C_ISR(I2C1) & mask) != 0;
}

void i2c1_isr(void)
{
    // Work around spurious bus errors
    if ((I2C_ISR(I2C1) & I2C_ISR_BERR) != 0) {
        I2C_ICR(I2C1) |= I2C_ICR_BERRCF;
    }

    if (!i2c1_irq_active()) {
        return;
    }

    nvic_disable_irq(NVIC_I2C1_IRQ);

    if (i2c_transfer_complete(I2C1)) {
        if ((I2C_CR2(I2C1) & I2C_CR2_RD_WRN) != 0) {
            return;
        }
    }

    BaseType_t need_yield;
    xSemaphoreGiveFromISR(semaphore_handle, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}

static inline bool dma1_channel2_3_irq_active(void)
{
    return dma_get_interrupt_flag(DMA1, DMA_CHANNEL2, DMA_ISR_GIF_BIT) ||
           dma_get_interrupt_flag(DMA1, DMA_CHANNEL3, DMA_ISR_GIF_BIT);
}

void dma1_channel2_3_isr(void)
{
    if (!dma1_channel2_3_irq_active()) {
        return;
    }

    nvic_disable_irq(NVIC_DMA1_CHANNEL2_3_IRQ);

    if (dma_get_interrupt_flag(DMA1, DMA_CHANNEL2, DMA_ISR_TCIF_BIT)) {
        return;
    }

    BaseType_t need_yield;
    xSemaphoreGiveFromISR(semaphore_handle, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}

void i2c_init(void)
{
    static StaticSemaphore_t semaphore_data;

    rcc_periph_clock_enable(RCC_GPIOF);

    const uint16_t gpios = GPIO0 | GPIO1;
    gpio_mode_setup(GPIOF, GPIO_MODE_AF, GPIO_PUPD_NONE, gpios);
    gpio_set_output_options(GPIOF, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, gpios);
    gpio_set_af(GPIOF, GPIO_AF1, gpios);

    rcc_set_i2c_clock_sysclk(I2C1);

    rcc_periph_clock_enable(RCC_I2C1);

    i2c_set_speed(I2C1, i2c_speed_fm_400k, system_core_clock / 1000000);

    i2c_peripheral_enable(I2C1);

    rcc_periph_clock_enable(RCC_DMA1);

    i2c_enable_rxdma(I2C1);
    i2c_enable_txdma(I2C1);

    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);

    dma_set_read_from_memory(DMA1, DMA_CHANNEL2);
    dma_set_read_from_peripheral(DMA1, DMA_CHANNEL3);

    dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
    dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);

    dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);

    dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t)&I2C1_TXDR);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t)&I2C1_RXDR);

    semaphore_handle = xSemaphoreCreateBinaryStatic(&semaphore_data);

    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL3);

    dma_enable_transfer_error_interrupt(DMA1, DMA_CHANNEL2);
    dma_enable_transfer_error_interrupt(DMA1, DMA_CHANNEL3);

    nvic_enable_irq(NVIC_DMA1_CHANNEL2_3_IRQ);

    i2c_enable_interrupt(I2C1, I2C_CR1_ERRIE | I2C_CR1_NACKIE);

    nvic_enable_irq(NVIC_I2C1_IRQ);
}

static inline void i2c1_soft_reset(void)
{
    I2C_CR1(I2C1) &= ~I2C_CR1_PE;

    while ((I2C_CR1(I2C1) & I2C_CR1_PE) != 0) {
    }

    I2C_CR1(I2C1) |= I2C_CR1_PE;
}

static size_t i2c_dma_transfer(uint8_t address,
                               bool write,
                               volatile const uint8_t *data,
                               size_t size,
                               bool stop)
{
    uint8_t channel;

    if (size == 0) {
        return 0;
    }

    if (write) {
        channel = DMA_CHANNEL2;
        i2c_set_write_transfer_dir(I2C1);
    } else {
        channel = DMA_CHANNEL3;
        i2c_set_read_transfer_dir(I2C1);
    }

    i2c_set_7bit_addr_mode(I2C1);
    i2c_set_7bit_address(I2C1, address);

    uint8_t bytes = (size < 255) ? (uint8_t)size : 255;

    i2c_set_bytes_to_transfer(I2C1, bytes);
    dma_set_number_of_data(DMA1, channel, bytes);

    dma_set_memory_address(DMA1, channel, (uint32_t)data);

    xSemaphoreTake(semaphore_handle, 0);

    dma_enable_channel(DMA1, channel);

    i2c_send_start(I2C1);

    xSemaphoreTake(semaphore_handle, pdMS_TO_TICKS(1000));

    dma_disable_channel(DMA1, channel);

    if (i2c_transfer_complete(I2C1)) {
        if (stop) {
            i2c_send_stop(I2C1);
        }
    } else {
        if (i2c_nack(I2C1)) {
            I2C_ICR(I2C1) |= I2C_ICR_NACKCF | I2C_ICR_STOPCF;
        } else {
            i2c1_soft_reset();
        }

        bytes = 0;
    }

    if (!dma_get_interrupt_flag(DMA1, channel, DMA_ISR_TCIF_BIT) ||
        dma_get_interrupt_flag(DMA1, channel, DMA_ISR_TEIF_BIT)) {
        bytes = 0;
    }

    dma_clear_interrupt_flags(DMA1, channel, DMA_IFCR_CGIF_BIT);

    nvic_clear_pending_irq(NVIC_DMA1_CHANNEL2_3_IRQ);
    nvic_enable_irq(NVIC_DMA1_CHANNEL2_3_IRQ);

    nvic_clear_pending_irq(NVIC_I2C1_IRQ);
    nvic_enable_irq(NVIC_I2C1_IRQ);

    return bytes;
}

bool i2c_read(uint8_t address, uint8_t *data, size_t size)
{
    if (i2c_dma_transfer(address, false, data, size, true) != size) {
        return false;
    }

    return true;
}

bool i2c_write(uint8_t address, const uint8_t *data, size_t size)
{
    if (i2c_dma_transfer(address, true, data, size, true) != size) {
        return false;
    }

    return true;
}

bool i2c_write_read(uint8_t address,
                    const uint8_t *data_1, size_t size_1,
                    uint8_t *data_2, size_t size_2)
{
    if (i2c_dma_transfer(address, true, data_1, size_1, false) != size_1) {
        return false;
    }

    if (i2c_dma_transfer(address, false, data_2, size_2, true) != size_2) {
        return false;
    }

    return true;
}

bool i2c_write_write(uint8_t address,
                     const uint8_t *data_1, size_t size_1,
                     const uint8_t *data_2, size_t size_2)
{
    if (i2c_dma_transfer(address, true, data_1, size_1, false) != size_1) {
        return false;
    }

    if (i2c_dma_transfer(address, true, data_2, size_2, true) != size_2) {
        return false;
    }

    return true;
}
