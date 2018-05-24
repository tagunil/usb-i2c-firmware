#include "usb.h"

#include <stdnoreturn.h>
#include <stdatomic.h>

#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>

#include <libopencm3/stm32/f0/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/syscfg.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#define COMMUNICATIONS_ENDPOINT 0x83
#define COMMUNICATIONS_PACKET_SIZE 16

#define DATA_IN_ENDPOINT 0x82
#define DATA_IN_PACKET_SIZE 64

#define DATA_OUT_ENDPOINT 0x01
#define DATA_OUT_PACKET_SIZE 64

struct usb_cdc_acm_functional_descriptors
{
    struct usb_cdc_header_descriptor header_descriptor;
    struct usb_cdc_call_management_descriptor call_management_descriptor;
    struct usb_cdc_acm_descriptor acm_descriptor;
    struct usb_cdc_union_descriptor union_descriptor;
} __attribute__((packed));

static const struct usb_cdc_acm_functional_descriptors
cdc_acm_functional_descriptors = {
    .header_descriptor = {
        .bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_HEADER,
        .bcdCDC = 0x0110
    },
    .call_management_descriptor = {
        .bFunctionLength = sizeof(struct usb_cdc_call_management_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
        .bmCapabilities = 0,
        .bDataInterface = 1
    },
    .acm_descriptor = {
        .bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_ACM,
        .bmCapabilities = 0
    },
    .union_descriptor = {
        .bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
        .bDescriptorType = CS_INTERFACE,
        .bDescriptorSubtype = USB_CDC_TYPE_UNION,
        .bControlInterface = 0,
        .bSubordinateInterface0 = 1
    }
};

static const struct usb_endpoint_descriptor
communications_endpoint_descriptors[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = COMMUNICATIONS_ENDPOINT,
        .bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
        .wMaxPacketSize = COMMUNICATIONS_PACKET_SIZE,
        .bInterval = 255
    }
};

static const struct usb_interface_descriptor
communications_interface_descriptor = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0,
    .bAlternateSetting = 0,
    .bNumEndpoints = 1,
    .bInterfaceClass = USB_CLASS_CDC,
    .bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
    .bInterfaceProtocol = USB_CDC_PROTOCOL_NONE,
    .iInterface = 0,

    .endpoint = communications_endpoint_descriptors,

    .extra = &cdc_acm_functional_descriptors,
    .extralen = sizeof(cdc_acm_functional_descriptors)
};

static const struct usb_endpoint_descriptor
data_endpoint_descriptors[] = {
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = DATA_OUT_ENDPOINT,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = DATA_OUT_PACKET_SIZE,
        .bInterval = 1
    },
    {
        .bLength = USB_DT_ENDPOINT_SIZE,
        .bDescriptorType = USB_DT_ENDPOINT,
        .bEndpointAddress = DATA_IN_ENDPOINT,
        .bmAttributes = USB_ENDPOINT_ATTR_BULK,
        .wMaxPacketSize = DATA_IN_PACKET_SIZE,
        .bInterval = 1
    }
};

static const struct usb_interface_descriptor
data_interface_descriptor = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 1,
    .bAlternateSetting = 0,
    .bNumEndpoints = 2,
    .bInterfaceClass = USB_CLASS_DATA,
    .bInterfaceSubClass = 0,
    .bInterfaceProtocol = 0,
    .iInterface = 0,

    .endpoint = data_endpoint_descriptors
};

static const struct usb_interface
interface_descriptors[] = {
    {
        .num_altsetting = 1,
        .altsetting = &communications_interface_descriptor
    },
    {
        .num_altsetting = 1,
        .altsetting = &data_interface_descriptor
    }
};

static const struct usb_config_descriptor
configuration_descriptor = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,
    .bNumInterfaces = 2,
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80,
    .bMaxPower = 50,

    .interface = interface_descriptors
};

static const struct usb_device_descriptor
device_descriptor = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x0483,
    .idProduct = 0x5740,
    .bcdDevice = 0x0200,
    .iManufacturer = 1,
    .iProduct = 2,
    .iSerialNumber = 3,
    .bNumConfigurations = 1
};

static const char *strings[] = {
    "Ilya Tagunov <tagunil@gmail.com>",
    "USB-I2C Adapter",
    "000000000000000000000000"
};

static enum usbd_request_return_codes
cdc_acm_control_request(usbd_device *device,
                        struct usb_setup_data *request,
                        uint8_t **buffer,
                        uint16_t *length,
                        void (**complete)(usbd_device *device,
                                          struct usb_setup_data *request))
{
    (void)device;
    (void)request;
    (void)buffer;
    (void)length;
    (void)complete;

    return USBD_REQ_NOTSUPP;
}

#define RECV_BUFFER_STORAGE_SIZE 512

static StaticStreamBuffer_t recv_buffer_data;
static uint8_t recv_buffer_storage[RECV_BUFFER_STORAGE_SIZE];
static StreamBufferHandle_t recv_buffer;
static volatile atomic_bool receiving;

static void cdc_acm_recv_callback(usbd_device *device, uint8_t endpoint)
{
    uint8_t buffer[DATA_OUT_PACKET_SIZE];

    usbd_ep_nak_set(device, endpoint, 1);

    size_t length = usbd_ep_read_packet(device,
                                        endpoint,
                                        buffer,
                                        sizeof(buffer));

    if (length > 0) {
        xStreamBufferSend(recv_buffer,
                          buffer,
                          length,
                          portMAX_DELAY);
    }

    size_t available = xStreamBufferSpacesAvailable(recv_buffer);
    if (available >= DATA_OUT_PACKET_SIZE) {
        usbd_ep_nak_set(device, endpoint, 0);
    } else {
        receiving = false;
    }
}

#define SEND_BUFFER_STORAGE_SIZE 512

static StaticStreamBuffer_t send_buffer_data;
static uint8_t send_buffer_storage[SEND_BUFFER_STORAGE_SIZE];
static StreamBufferHandle_t send_buffer;
static volatile atomic_bool sending;

static void cdc_acm_send_callback(usbd_device *device, uint8_t endpoint)
{
    uint8_t buffer[DATA_IN_PACKET_SIZE];

    size_t length = xStreamBufferReceive(send_buffer,
                                         buffer,
                                         sizeof(buffer),
                                         0);

    if (length > 0) {
        sending = true;
    }

    if (sending) {
        usbd_ep_write_packet(device,
                             endpoint,
                             buffer,
                             (uint16_t)length);
    }

    if (length < DATA_IN_PACKET_SIZE) {
        sending = false;
    }
}

static void cdc_acm_set_config(usbd_device *device, uint16_t wValue)
{
    (void)wValue;

    usbd_ep_setup(device,
                  DATA_OUT_ENDPOINT,
                  USB_ENDPOINT_ATTR_BULK,
                  DATA_OUT_PACKET_SIZE,
                  cdc_acm_recv_callback);
    usbd_ep_setup(device,
                  DATA_IN_ENDPOINT,
                  USB_ENDPOINT_ATTR_BULK,
                  DATA_IN_PACKET_SIZE,
                  cdc_acm_send_callback);
    usbd_ep_setup(device,
                  COMMUNICATIONS_ENDPOINT,
                  USB_ENDPOINT_ATTR_INTERRUPT,
                  COMMUNICATIONS_PACKET_SIZE,
                  NULL);

    usbd_register_control_callback(device,
                                   USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
                                   USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
                                   cdc_acm_control_request);
}

static inline void usb_enable_irq(bool enable)
{
    static const uint32_t mask = USB_CNTR_RESETM | USB_CNTR_CTRM |
                                 USB_CNTR_SUSPM | USB_CNTR_WKUPM |
                                 USB_CNTR_SOFM;

    if (enable) {
        *USB_CNTR_REG |= mask;
        nvic_enable_irq(NVIC_USB_IRQ);
    } else {
        nvic_disable_irq(NVIC_USB_IRQ);
        *USB_CNTR_REG &= ~mask;
    }
}

static inline bool usb_irq_active(void)
{
    static const uint32_t mask = USB_ISTR_RESET | USB_ISTR_CTR |
                                 USB_ISTR_SUSP | USB_ISTR_WKUP |
                                 USB_ISTR_SOF;

    return (*USB_ISTR_REG & mask) != 0;
}

enum
{
    POLL_NOTIFICATION = 0x01,
    RECV_NOTIFICATION = 0x02,
    SEND_NOTIFICATION = 0x04
};

static TaskHandle_t task_handle;

void usb_isr(void)
{
    if (!usb_irq_active()) {
        return;
    }

    usb_enable_irq(false);

    BaseType_t need_yield;
    xTaskNotifyFromISR(task_handle, POLL_NOTIFICATION, eSetBits, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}

noreturn static void usb_task(void *parameter)
{
    usbd_device *device = parameter;

    for (;;) {
        uint32_t notification = 0;

        xTaskNotifyWait(0,
                        UINT32_MAX,
                        &notification,
                        portMAX_DELAY);

        if ((notification & POLL_NOTIFICATION) != 0) {
            while (usb_irq_active()) {
                usbd_poll(device);
            }

            usb_enable_irq(true);
        }

        if ((notification & RECV_NOTIFICATION) != 0) {
            if (!receiving) {
                size_t available = xStreamBufferSpacesAvailable(recv_buffer);
                if (available >= DATA_OUT_PACKET_SIZE) {
                    receiving = true;
                    usbd_ep_nak_set(device, DATA_OUT_ENDPOINT, 0);
                }
            }
        }

        if ((notification & SEND_NOTIFICATION) != 0) {
            if (!sending) {
                size_t available = xStreamBufferBytesAvailable(send_buffer);
                if (available > 0) {
                    sending = true;
                    cdc_acm_send_callback(device, DATA_IN_ENDPOINT);
                }
            }
        }
    }
}

void usb_init(void)
{
    static StaticTask_t task_data;
    static StackType_t task_stack[configMINIMAL_STACK_SIZE * 2];

    static usbd_device *device;

    static uint8_t control_buffer[128];

    rcc_periph_clock_enable(RCC_SYSCFG_COMP);
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;

    rcc_periph_clock_enable(RCC_GPIOA);
    gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO11 | GPIO12);

    rcc_periph_clock_enable(RCC_CRS);
    crs_autotrim_usb_enable();

    rcc_set_usbclk_source(RCC_HSI48);
    rcc_periph_clock_enable(RCC_USB);

    device = usbd_init(&st_usbfs_v2_usb_driver,
                       &device_descriptor,
                       &configuration_descriptor,
                       strings,
                       3,
                       control_buffer,
                       sizeof(control_buffer));
    usbd_register_set_config_callback(device, cdc_acm_set_config);

    recv_buffer = xStreamBufferCreateStatic(sizeof(recv_buffer_storage) - 1,
                                            1,
                                            recv_buffer_storage,
                                            &recv_buffer_data);
    receiving = true;

    send_buffer = xStreamBufferCreateStatic(sizeof(send_buffer_storage) - 1,
                                            1,
                                            send_buffer_storage,
                                            &send_buffer_data);
    sending = false;

    task_handle = xTaskCreateStatic(&usb_task,
                                    "USB",
                                    sizeof(task_stack) / sizeof(StackType_t),
                                    device,
                                    2,
                                    task_stack,
                                    &task_data);

    usb_enable_irq(true);
}

size_t usb_recv(uint8_t *data, size_t size)
{
    size_t length = xStreamBufferReceive(recv_buffer,
                                         data,
                                         size,
                                         portMAX_DELAY);

    if (!receiving) {
        xTaskNotify(task_handle, RECV_NOTIFICATION, eSetBits);
    }

    return length;
}

size_t usb_send(const uint8_t *data, size_t size)
{
    size_t length = xStreamBufferSend(send_buffer, data, size, portMAX_DELAY);

    if (!sending) {
        xTaskNotify(task_handle, SEND_NOTIFICATION, eSetBits);
    }

    return length;
}
