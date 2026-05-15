#include "bme280-driver.h"
#include "bme280-regs.h"
#include <stdio.h>

typedef struct
{
    bme280_i2c_read i2c_read;
    bme280_i2c_write i2c_write;
} bme280_ctx_t;

static bme280_ctx_t bme280_ctx = {};

static uint16_t dig_T1;
static int16_t dig_T2;
static int16_t dig_T3;

static uint16_t dig_P1;
static int16_t dig_P2;
static int16_t dig_P3;
static int16_t dig_P4;
static int16_t dig_P5;
static int16_t dig_P6;
static int16_t dig_P7;
static int16_t dig_P8;
static int16_t dig_P9;

static uint8_t dig_H1;
static int16_t dig_H2;
static uint8_t dig_H3;
static int16_t dig_H4;
static int16_t dig_H5;
static int8_t dig_H6;

static int32_t t_fine = 0;

static void bme280_read_calib_data(void)
{
    uint8_t buffer[24] = {0};
    bme280_read_regs(0x88, buffer, 24);
    
    dig_T1 = (uint16_t)(buffer[0] | (buffer[1] << 8));
    dig_T2 = (int16_t)(buffer[2] | (buffer[3] << 8));
    dig_T3 = (int16_t)(buffer[4] | (buffer[5] << 8));
    
    dig_P1 = (uint16_t)(buffer[6] | (buffer[7] << 8));
    dig_P2 = (int16_t)(buffer[8] | (buffer[9] << 8));
    dig_P3 = (int16_t)(buffer[10] | (buffer[11] << 8));
    dig_P4 = (int16_t)(buffer[12] | (buffer[13] << 8));
    dig_P5 = (int16_t)(buffer[14] | (buffer[15] << 8));
    dig_P6 = (int16_t)(buffer[16] | (buffer[17] << 8));
    dig_P7 = (int16_t)(buffer[18] | (buffer[19] << 8));
    dig_P8 = (int16_t)(buffer[20] | (buffer[21] << 8));
    dig_P9 = (int16_t)(buffer[22] | (buffer[23] << 8));
    
    uint8_t h_buffer[7] = {0};
    bme280_read_regs(0xE1, h_buffer, 7);
    
    dig_H1 = buffer[25];
    dig_H2 = (int16_t)(h_buffer[0] | (h_buffer[1] << 8));
    dig_H3 = h_buffer[2];
    dig_H4 = (int16_t)((h_buffer[3] << 4) | (h_buffer[4] & 0x0F));
    dig_H5 = (int16_t)((h_buffer[4] >> 4) | (h_buffer[5] << 4));
    dig_H6 = (int8_t)h_buffer[6];
    
    printf("BME280 calibration loaded\n");
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write)
{
    bme280_ctx.i2c_read = i2c_read;
    bme280_ctx.i2c_write = i2c_write;
    
    uint8_t id = 0;
    bme280_read_regs(BME280_REG_id, &id, 1);
    
    if (id != 0x60)
    {
        printf("BME280 error: ID=0x%02X (expected 0x60)\n", id);
        return;
    }
    printf("BME280 found: ID=0x%02X\n", id);
    
    bme280_read_calib_data();
    
    bme280_write_reg(BME280_REG_ctrl_hum, 0x01);
    
    bme280_write_reg(BME280_REG_config, 0x00);
    
    bme280_write_reg(BME280_REG_ctrl_meas, 0x27);
    
    printf("BME280 initialized\n");
}

void bme280_read_regs(uint8_t start_reg_address, uint8_t* buffer, uint8_t length)
{
    uint8_t data[1] = {start_reg_address};
    bme280_ctx.i2c_write(data, sizeof(data));
    bme280_ctx.i2c_read(buffer, length);
}

void bme280_write_reg(uint8_t reg_address, uint8_t value)
{
    uint8_t data[2] = {reg_address, value};
    bme280_ctx.i2c_write(data, sizeof(data));
}

uint32_t bme280_read_temp_raw(void)
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_temp_msb, read, 3);
    return ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
}

uint32_t bme280_read_pres_raw(void)
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_press_msb, read, 3);
    return ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
}

uint16_t bme280_read_hum_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_hum_msb, read, 2);
    return ((uint16_t)read[0] << 8) | read[1];
}

float bme280_read_temp(void)
{
    uint32_t adc_T = bme280_read_temp_raw();
    
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    int32_t T = (t_fine * 5 + 128) >> 8;
    
    return (float)T / 100.0f;
}

float bme280_read_pres(void)
{
    bme280_read_temp();
    
    uint32_t adc_P = bme280_read_pres_raw();
    
    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    
    if (var1 == 0) return 0.0f;
    
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    
    return (float)p / 256.0f;
}

float bme280_read_hum(void)
{
    bme280_read_temp();
    
    int32_t adc_H = bme280_read_hum_raw();
    
    int32_t v1 = t_fine - 76800;
    int32_t v2 = adc_H * 16384;
    int32_t v3 = (int32_t)dig_H4 * 1048576;
    int32_t v4 = (int32_t)dig_H5 * v1;
    int32_t v5 = (v2 - v3 - v4 + 16384) >> 15;
    
    int32_t v6 = (v1 * (int32_t)dig_H6) >> 10;
    int32_t v7 = (v1 * (int32_t)dig_H3) >> 11;
    int32_t v8 = (v6 * (v7 + 32768)) >> 10;
    int32_t v9 = (v8 + 2097152) * (int32_t)dig_H2 + 8192;
    int32_t v10 = v9 >> 14;
    int32_t v11 = (v5 * v10) >> 15;
    
    int32_t v12 = (v11 >> 7) * (v11 >> 7);
    int32_t v13 = (v12 * (int32_t)dig_H1) >> 4;
    int32_t humidity = v11 - v13;
    
    if (humidity < 0) humidity = 0;
    if (humidity > 419430400) humidity = 419430400;
    
    float h = (float)(humidity >> 12) / 1024.0f;
    if (h > 100.0f) h = 100.0f;
    if (h < 0.0f) h = 0.0f;
    
    return h;
}