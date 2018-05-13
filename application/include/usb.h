#pragma once

#include <stddef.h>
#include <stdint.h>

void usb_init(void);

size_t usb_recv(uint8_t *data, size_t size);

size_t usb_send(const uint8_t *data, size_t size);
