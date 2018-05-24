#include <FreeRTOS.h>
#include <task.h>

#include "clock.h"
#include "usb.h"
#include "i2c.h"
#include "shell.h"

int main(void)
{
    clock_setup();

    usb_init();

    i2c_init();

    shell_init();

    vTaskStartScheduler();

    return 1;
}
