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
#include "i2c.h"

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

static void send_data(const uint8_t *data, size_t length)
{
    const char *header = "DATA ";
    usb_send((const uint8_t *)header, strlen(header));

    for (size_t position = 0; position < length; position++) {
        char string[3];
        write_hex(&data[position], string, 1);
        usb_send((const uint8_t *)string, 2);
    }

    const char *trailer = "\r\n";
    usb_send((const uint8_t *)trailer, strlen(trailer));
}

static void send_error(void)
{
    const char *answer = "ERROR\r\n";
    usb_send((const uint8_t *)answer, strlen(answer));
}

#define MAX_DATA_LENGTH 64

static void shell_process_command(char *command)
{
    const char *action = strtok(command, " ");
    if (!action) {
        send_error();
        return;
    }

    if (strcmp(action, "PING") == 0) {
        send_ok();
    } else if (strcmp(action, "READ") == 0) {
        const char *address_token = strtok(NULL, " ");
        if (!address_token) {
            send_error();
            return;
        }

        if (strlen(address_token) != 2) {
            send_error();
            return;
        }

        int address_value = read_hex_u8(address_token);
        if (address_value <= 0) {
            send_error();
            return;
        }

        uint8_t address = (uint8_t)address_value;

        const char *length_token = strtok(NULL, " ");
        if (!length_token) {
            send_error();
            return;
        }

        int length_value = read_u16(length_token);
        if ((length_value <= 0) || (length_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length = (size_t)length_value;

        uint8_t data[MAX_DATA_LENGTH];
        if (!i2c_read(address, data, length)) {
            send_error();
            return;
        }

        send_data(data, length);
    } else if (strcmp(action, "WRITE") == 0) {
        const char *address_token = strtok(NULL, " ");
        if (!address_token) {
            send_error();
            return;
        }

        if (strlen(address_token) != 2) {
            send_error();
            return;
        }

        int address_value = read_hex_u8(address_token);
        if (address_value <= 0) {
            send_error();
            return;
        }

        uint8_t address = (uint8_t)address_value;

        const char *length_token = strtok(NULL, " ");
        if (!length_token) {
            send_error();
            return;
        }

        int length_value = read_u16(length_token);
        if ((length_value <= 0) || (length_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length = (size_t)length_value;

        const char *data_token = strtok(NULL, " ");
        if (!data_token) {
            send_error();
            return;
        }

        if (strlen(data_token) != length * 2) {
            send_error();
            return;
        }

        uint8_t data[MAX_DATA_LENGTH];
        if (!read_hex(data_token, data, length))
        {
            send_error();
            return;
        }

        if (!i2c_write(address, data, length)) {
            send_error();
            return;
        }

        send_ok();
    } else if (strcmp(action, "WRITE_READ") == 0) {
        const char *address_token = strtok(NULL, " ");
        if (!address_token) {
            send_error();
            return;
        }

        if (strlen(address_token) != 2) {
            send_error();
            return;
        }

        int address_value = read_hex_u8(address_token);
        if (address_value <= 0) {
            send_error();
            return;
        }

        uint8_t address = (uint8_t)address_value;

        const char *length_1_token = strtok(NULL, " ");
        if (!length_1_token) {
            send_error();
            return;
        }

        int length_1_value = read_u16(length_1_token);
        if ((length_1_value <= 0) || (length_1_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length_1 = (size_t)length_1_value;

        const char *data_1_token = strtok(NULL, " ");
        if (!data_1_token) {
            send_error();
            return;
        }

        if (strlen(data_1_token) != length_1 * 2) {
            send_error();
            return;
        }

        uint8_t data_1[MAX_DATA_LENGTH];
        if (!read_hex(data_1_token, data_1, length_1))
        {
            send_error();
            return;
        }

        const char *length_2_token = strtok(NULL, " ");
        if (!length_2_token) {
            send_error();
            return;
        }

        int length_2_value = read_u16(length_2_token);
        if ((length_2_value <= 0) || (length_2_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length_2 = (size_t)length_2_value;

        uint8_t data_2[MAX_DATA_LENGTH];
        if (!i2c_write_read(address, data_1, length_1, data_2, length_2)) {
            send_error();
            return;
        }

        send_data(data_2, length_2);
    } else if (strcmp(action, "WRITE_WRITE") == 0) {
        const char *address_token = strtok(NULL, " ");
        if (!address_token) {
            send_error();
            return;
        }

        if (strlen(address_token) != 2) {
            send_error();
            return;
        }

        int address_value = read_hex_u8(address_token);
        if (address_value <= 0) {
            send_error();
            return;
        }

        uint8_t address = (uint8_t)address_value;

        const char *length_1_token = strtok(NULL, " ");
        if (!length_1_token) {
            send_error();
            return;
        }

        int length_1_value = read_u16(length_1_token);
        if ((length_1_value <= 0) || (length_1_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length_1 = (size_t)length_1_value;

        const char *data_1_token = strtok(NULL, " ");
        if (!data_1_token) {
            send_error();
            return;
        }

        if (strlen(data_1_token) != length_1 * 2) {
            send_error();
            return;
        }

        uint8_t data_1[MAX_DATA_LENGTH];
        if (!read_hex(data_1_token, data_1, length_1))
        {
            send_error();
            return;
        }

        const char *length_2_token = strtok(NULL, " ");
        if (!length_2_token) {
            send_error();
            return;
        }

        int length_2_value = read_u16(length_2_token);
        if ((length_2_value <= 0) || (length_2_value > MAX_DATA_LENGTH)) {
            send_error();
            return;
        }

        size_t length_2 = (size_t)length_2_value;

        const char *data_2_token = strtok(NULL, " ");
        if (!data_2_token) {
            send_error();
            return;
        }

        if (strlen(data_2_token) != length_2 * 2) {
            send_error();
            return;
        }

        uint8_t data_2[MAX_DATA_LENGTH];
        if (!read_hex(data_2_token, data_2, length_2))
        {
            send_error();
            return;
        }

        if (!i2c_write_write(address, data_1, length_1, data_2, length_2)) {
            send_error();
            return;
        }

        send_ok();
    } else {
        send_error();
    }

    return;
}

bool is_character(uint8_t byte)
{
    return (byte >= ' ') && (byte <= '~');
}

#define MAX_COMMAND_LENGTH 511

noreturn static void shell_task(void *parameter)
{
    (void)parameter;

    static uint8_t command_buffer[MAX_COMMAND_LENGTH + 1];
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
            if (command_length < MAX_COMMAND_LENGTH) {
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
