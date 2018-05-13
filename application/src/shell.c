#include "shell.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>

#include "usb.h"

static void send_ok(void)
{
    const char *answer = "OK\r\n";
    usb_send((const uint8_t *)answer, strlen(answer));
}

static void send_error(void)
{
    const char *answer = "ERROR\r\n";
    usb_send((const uint8_t *)answer, strlen(answer));
}

static void shell_process_command(char *command)
{
    const char *action = strtok(command, " ");
    if (!action || (action[0] == '\0')) {
        send_error();
        return;
    }

    if (strcmp(action, "PING") == 0) {
        send_ok();
    } else if (strcmp(action, "READ") == 0) {
        send_error();
    } else if (strcmp(action, "WRITE") == 0) {
        send_error();
    } else {
        send_error();
    }

    return;
}

bool is_character(uint8_t byte)
{
    return (byte >= ' ') && (byte <= '~');
}

noreturn static void shell_task(void *parameter)
{
    (void)parameter;

    static uint8_t command_buffer[256];
    size_t command_length = 0;

    for (;;) {
        uint8_t byte;

        if (usb_recv(&byte, 1) == 0) {
            continue;
        }

        if ((byte == '\n') || (byte == '\r')) {
            if (command_length != 0) {
                command_buffer[command_length] = '\0';
                shell_process_command((char *)command_buffer);
                command_length = 0;
            }
        } else if (is_character(byte)) {
            if (command_length < sizeof(command_buffer) - 1) {
                command_buffer[command_length] = byte;
                command_length++;
            }
        }
    }
}

void shell_init(void)
{
    static StaticTask_t task_block;
    static StackType_t task_stack[configMINIMAL_STACK_SIZE * 2];

    xTaskCreateStatic(&shell_task,
                      "Shell",
                      sizeof(task_stack) / sizeof(StackType_t),
                      NULL,
                      1,
                      task_stack,
                      &task_block);
}
