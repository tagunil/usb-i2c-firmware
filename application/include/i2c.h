#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

void i2c_init(void);

bool i2c_read(uint8_t address, uint8_t *data, size_t size);

bool i2c_write(uint8_t address, const uint8_t *data, size_t size);

bool i2c_write_read(uint8_t address,
                    const uint8_t *data_1, size_t size_1,
                    uint8_t *data_2, size_t size_2);

bool i2c_write_write(uint8_t address,
                     const uint8_t *data_1, size_t size_1,
                     const uint8_t *data_2, size_t size_2);
