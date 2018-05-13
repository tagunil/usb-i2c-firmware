#include "clock.h"

#include <libopencm3/stm32/rcc.h>

uint32_t system_core_clock;

void clock_setup(void)
{
    rcc_clock_setup_in_hsi48_out_48mhz();
    system_core_clock = 48000000;
}
