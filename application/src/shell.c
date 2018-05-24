#include "shell.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdnoreturn.h>
#include <string.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>

#include "usb.h"

static bool write_hex(const uint8_t *data, char *string, size_t length)
{
    for (size_t position = 0; position < length; position++) {
        uint8_t value = data[position];

        for (size_t place = 0; place < 2; place++) {
            char digit = value & 0x0f;

            value >>= 4;

            if (digit <= 9) {
                digit += '0';
            } else {
                digit += 'a' - 10;
            }

            string[position * 2 + 1 - place] = digit;
        }
    }

    string[length * 2] = '\0';

    return true;
}

static bool read_hex(const char *string, uint8_t *data, size_t length)
{
    for (size_t position = 0; position < length; position++) {
        uint8_t value = 0;

        for (size_t place = 0; place < 2; place++) {
            char digit = string[position * 2 + place];

            value <<= 4;

            if ((digit >= '0') && (digit <= '9')) {
                value += digit - '0';
            } else if ((digit >= 'a') && (digit <= 'f')) {
                value += digit - 'a' + 10;
            } else {
                return false;
            }
        }

        data[position] = value;
    }

    return true;
}

static int read_hex_u8(const char *string)
{
    uint8_t byte;

    if (!read_hex(string, &byte, 1)) {
        return -1;
    }

    return byte;
}

static int read_u16(const char *string)
{
    int value = 0;

    const char *pointer = string;
    while (*pointer != '\0') {
        char digit = *pointer;

        if ((digit >= '0') && (digit <= '9')) {
            value *= 10;
            value += digit - '0';

            if (value > UINT16_MAX) {
                return -1;
            }
        } else {
            return -1;
        }

        pointer++;
    }

    return value;
}

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
    static StaticTask_t task_data;
    static StackType_t task_stack[configMINIMAL_STACK_SIZE * 2];

    xTaskCreateStatic(&shell_task,
                      "Shell",
                      sizeof(task_stack) / sizeof(StackType_t),
                      NULL,
                      1,
                      task_stack,
                      &task_data);
}
