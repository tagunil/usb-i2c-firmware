#pragma once

#include <stddef.h>
#include <stdint.h>

void i2c_init(void);

size_t i2c_read(uint8_t address, uint8_t *data, size_t size);

size_t i2c_write(uint8_t address, const uint8_t *data, size_t size);
