#ifndef BME280_H
#define BME280_H

#include <stdint.h>

typedef void (*bme280_i2c_read)(uint8_t* buffer, uint16_t length);
typedef void (*bme280_i2c_write)(uint8_t* data, uint16_t size);

typedef struct
{
    bme280_i2c_read i2c_read;
    bme280_i2c_write i2c_write;
} bme280_ctx_t;

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write);
void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length);
void bme280_write_reg(uint8_t reg_address, uint8_t value);
uint16_t bme280_read_temp_raw(void);
uint16_t bme280_read_pres_raw(void);
uint16_t bme280_read_hum_raw(void);
float bme280_read_temp(void);
float bme280_read_pres(void);
float bme280_read_hum(void);

#endif