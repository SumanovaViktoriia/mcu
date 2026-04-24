#include "bme280.h"
#include "bme280-regs.h"
#include <stdio.h>

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

static int32_t t_fine;

void bme280_read_calib_data(void)
{
    uint8_t buffer[26] = {0};
    bme280_read_regs(0x88, buffer, 26);
    
    dig_T1 = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
    dig_T2 = (int16_t)buffer[2] | ((int16_t)buffer[3] << 8);
    dig_T3 = (int16_t)buffer[4] | ((int16_t)buffer[5] << 8);
    
    dig_P1 = (uint16_t)buffer[6] | ((uint16_t)buffer[7] << 8);
    dig_P2 = (int16_t)buffer[8] | ((int16_t)buffer[9] << 8);
    dig_P3 = (int16_t)buffer[10] | ((int16_t)buffer[11] << 8);
    dig_P4 = (int16_t)buffer[12] | ((int16_t)buffer[13] << 8);
    dig_P5 = (int16_t)buffer[14] | ((int16_t)buffer[15] << 8);
    dig_P6 = (int16_t)buffer[16] | ((int16_t)buffer[17] << 8);
    dig_P7 = (int16_t)buffer[18] | ((int16_t)buffer[19] << 8);
    dig_P8 = (int16_t)buffer[20] | ((int16_t)buffer[21] << 8);
    dig_P9 = (int16_t)buffer[22] | ((int16_t)buffer[23] << 8);
    
    uint8_t hbuffer[7] = {0};
    bme280_read_regs(0xE1, hbuffer, 7);
    
    dig_H1 = buffer[25];
    dig_H2 = (int16_t)hbuffer[0] | ((int16_t)hbuffer[1] << 8);
    dig_H3 = hbuffer[2];
    dig_H4 = (int16_t)hbuffer[3] << 4 | (hbuffer[4] & 0x0F);
    dig_H5 = (int16_t)hbuffer[5] << 4 | (hbuffer[4] >> 4);
    dig_H6 = (int8_t)hbuffer[6];
}

void bme280_init(bme280_i2c_read i2c_read, bme280_i2c_write i2c_write)
{
    bme280_ctx.i2c_read = i2c_read;
    bme280_ctx.i2c_write = i2c_write;
    
    uint8_t id_reg_buf[1] = {0};
    bme280_read_regs(BME280_REG_id, id_reg_buf, sizeof(id_reg_buf));
    
    if (id_reg_buf[0] != 0x60)
    {
        printf("BME280 error: invalid ID (0x%X). Expected 0x60\n", id_reg_buf[0]);
    }
    else
    {
        printf("BME280 found: ID=0x%X\n", id_reg_buf[0]);
    }
    
    bme280_read_calib_data();
    
    uint8_t ctrl_hum_reg_value = 0;
    ctrl_hum_reg_value |= (0b001 << 0);
    bme280_write_reg(BME280_REG_ctrl_hum, ctrl_hum_reg_value);
    
    uint8_t config_reg_value = 0;
    config_reg_value |= (0b00 << 0);
    config_reg_value |= (0b000 << 2);
    config_reg_value |= (0b001 << 5);
    bme280_write_reg(BME280_REG_config, config_reg_value);
    
    uint8_t ctrl_meas_reg_value = 0;
    ctrl_meas_reg_value |= (0b001 << 5);
    ctrl_meas_reg_value |= (0b001 << 2);
    ctrl_meas_reg_value |= (0b11 << 0);
    bme280_write_reg(BME280_REG_ctrl_meas, ctrl_meas_reg_value);
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

uint16_t bme280_read_temp_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_temp_msb, read, sizeof(read));
    uint16_t value = ((uint16_t)read[0] << 8) | ((uint16_t)read[1]);
    return value;
}

uint16_t bme280_read_pres_raw(void)
{
    uint8_t read[3] = {0};
    bme280_read_regs(BME280_REG_press_msb, read, sizeof(read));
    uint32_t value = ((uint32_t)read[0] << 12) | ((uint32_t)read[1] << 4) | ((uint32_t)read[2] >> 4);
    return (uint16_t)value;
}

uint16_t bme280_read_hum_raw(void)
{
    uint8_t read[2] = {0};
    bme280_read_regs(BME280_REG_hum_msb, read, sizeof(read));
    uint16_t value = ((uint16_t)read[0] << 8) | ((uint16_t)read[1]);
    return value;
}

float bme280_read_temp(void)
{
    int32_t adc_T = bme280_read_temp_raw();
    
    int32_t var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
    int32_t var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    int32_t T = (t_fine * 5 + 128) >> 8;
    
    return (float)T / 100.0f;
}

float bme280_read_pres(void)
{
    int32_t adc_P = bme280_read_pres_raw();
    
    int64_t var1 = (int64_t)t_fine - 128000;
    int64_t var2 = var1 * var1 * (int64_t)dig_P6;
    var2 = var2 + ((var1 * (int64_t)dig_P5) << 17);
    var2 = var2 + (((int64_t)dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)dig_P3) >> 8) + ((var1 * (int64_t)dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)dig_P1) >> 33;
    
    if (var1 == 0)
    {
        return 0.0f;
    }
    
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)dig_P7) << 4);
    
    return (float)p / 256.0f;
}

float bme280_read_hum(void)
{
    int32_t adc_H = bme280_read_hum_raw();
    
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
    v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
    
    return (float)(v_x1_u32r >> 12) / 1024.0f;
}