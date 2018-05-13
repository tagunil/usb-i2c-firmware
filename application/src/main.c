#include <FreeRTOS.h>
#include <task.h>

#include "clock.h"
#include "usb.h"
#include "shell.h"

int main(void)
{
    clock_setup();

    usb_init();

    shell_init();

    vTaskStartScheduler();

    return 1;
}
