#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "bme280.h"

#define DEVICE_NAME  "MyPicoDevice"
#define DEVICE_VRSN  "1.0.0"
#define BME280_I2C_ADDR 0x76

void version_callback(const char* args);
void led_on_callback(const char* args);
void led_off_callback(const char* args);
void led_blink_callback(const char* args);
void led_blink_set_period_ms_callback(const char* args);
void help_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);
void read_regs_callback(const char* args);
void write_reg_callback(const char* args);
void temp_raw_callback(const char* args);
void pres_raw_callback(const char* args);
void hum_raw_callback(const char* args);
void temp_callback(const char* args);
void pres_callback(const char* args);
void hum_callback(const char* args);

void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
    i2c_read_timeout_us(i2c1, BME280_I2C_ADDR, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
    i2c_write_timeout_us(i2c1, BME280_I2C_ADDR, data, size, false, 100000);
}

api_t device_api[] =
{
    {"version", version_callback, "get device name and firmware version"},
    {"on", led_on_callback, "turn LED on"},
    {"off", led_off_callback, "turn LED off"},
    {"blink", led_blink_callback, "make LED blink"},
    {"set_period", led_blink_set_period_ms_callback, "set LED blink period in milliseconds"},
    {"help", help_callback, "print all available commands with descriptions"},
    {"mem", mem_callback, "Read 32-bit value from memory address (hex)"},
    {"wmem", wmem_callback, "Write 32-bit value to memory address (hex)"},
    {"read_regs", read_regs_callback, "Read BME280 registers: read_regs <addr> <N>"},
    {"write_reg", write_reg_callback, "Write BME280 register: write_reg <addr> <value>"},
    {"temp_raw", temp_raw_callback, "Read raw temperature value"},
    {"pres_raw", pres_raw_callback, "Read raw pressure value"},
    {"hum_raw", hum_raw_callback, "Read raw humidity value"},
    {"temp", temp_callback, "Read temperature in Celsius"},
    {"pres", pres_callback, "Read pressure in hPa"},
    {"hum", hum_callback, "Read humidity in percent"},
    {NULL, NULL, NULL},
};

static const int commands_count = sizeof(device_api) / sizeof(api_t) - 1;

void version_callback(const char* args)
{
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void led_on_callback(const char* args)
{
    led_task_state_set(LED_STATE_ON);
    printf("LED turned ON\n");
}

void led_off_callback(const char* args)
{
    led_task_state_set(LED_STATE_OFF);
    printf("LED turned OFF\n");
}

void led_blink_callback(const char* args)
{
    led_task_state_set(LED_STATE_BLINK);
    printf("LED blinking mode activated\n");
}

void led_blink_set_period_ms_callback(const char* args)
{
    uint period_ms = 0;
    sscanf(args, "%u", &period_ms);
    
    if (period_ms == 0)
    {
        printf("Error: invalid period. Usage: set_period <ms>\n");
        return;
    }
    
    led_task_set_blink_period_ms(period_ms);
    printf("LED blink period set to %u ms\n", period_ms);
}

void help_callback(const char* args)
{
    printf("Available commands:\n");
    
    for (int i = 0; i < commands_count; i++)
    {
        printf("  %s - %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

extern char* stdio_task_handle(void);

void mem_callback(const char* args) {
    uint32_t address = 0;
    if (sscanf(args, "%x", &address) != 1) {
        printf("Usage: mem <hex_address>\n");
        return;
    }

    volatile uint32_t* ptr = (volatile uint32_t*)address;
    
    uint32_t value = *ptr;
    
    printf("Value at address 0x%08x: 0x%08x (%u)\n", address, value, value);
}

void wmem_callback(const char* args) {
    uint32_t address = 0;
    uint32_t value = 0;
    if (sscanf(args, "%x %x", &address, &value) != 2) {
        printf("Usage: wmem <hex_address> <hex_value>\n");
        return;
    }
    volatile uint32_t* ptr = (volatile uint32_t*)address;
    
    *ptr = value;
    
    uint32_t readback = *ptr;
    printf("Written 0x%08x to address 0x%08x. Readback: 0x%08x\n", value, address, readback);
}

void read_regs_callback(const char* args)
{
    uint32_t addr = 0;
    uint32_t N = 0;
    
    if (sscanf(args, "%x %u", &addr, &N) != 2)
    {
        printf("Error: invalid arguments. Usage: read_regs <addr> <N>\n");
        return;
    }
    
    if (addr > 0xFF)
    {
        printf("Error: addr must be <= 0xFF\n");
        return;
    }
    
    if (N > 0xFF)
    {
        printf("Error: N must be <= 0xFF\n");
        return;
    }
    
    if (addr + N > 0x100)
    {
        printf("Error: addr + N must be <= 0x100\n");
        return;
    }
    
    uint8_t buffer[256] = {0};
    bme280_read_regs((uint8_t)addr, buffer, (uint8_t)N);
    
    for (int i = 0; i < N; i++)
    {
        printf("bme280 register [0x%X] = 0x%X\n", addr + i, buffer[i]);
    }
}

void write_reg_callback(const char* args)
{
    uint32_t addr = 0;
    uint32_t value = 0;
    
    if (sscanf(args, "%x %x", &addr, &value) != 2)
    {
        printf("Error: invalid arguments. Usage: write_reg <addr> <value>\n");
        return;
    }
    
    if (addr > 0xFF)
    {
        printf("Error: addr must be <= 0xFF\n");
        return;
    }
    
    if (value > 0xFF)
    {
        printf("Error: value must be <= 0xFF\n");
        return;
    }
    
    bme280_write_reg((uint8_t)addr, (uint8_t)value);
    printf("Written 0x%02X to register 0x%02X\n", (uint8_t)value, (uint8_t)addr);
}

void temp_raw_callback(const char* args)
{
    uint16_t temp = bme280_read_temp_raw();
    printf("%u\n", temp);
}

void pres_raw_callback(const char* args)
{
    uint16_t pres = bme280_read_pres_raw();
    printf("%u\n", pres);
}

void hum_raw_callback(const char* args)
{
    uint16_t hum = bme280_read_hum_raw();
    printf("%u\n", hum);
}

void temp_callback(const char* args)
{
    float temp = bme280_read_temp();
    printf("%.2f\n", temp);
}

void pres_callback(const char* args)
{
    float pres = bme280_read_pres();
    printf("%.2f\n", pres);
}

void hum_callback(const char* args)
{
    float hum = bme280_read_hum();
    printf("%.2f\n", hum);
}

int main(void)
{
    stdio_init_all();
    
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    
    bme280_init(rp2040_i2c_read, rp2040_i2c_write);
    
    protocol_task_init(device_api);
    led_task_init();
    
    while (1)
    {
        char* command_string = stdio_task_handle();
        
        if (command_string != NULL)
        {
            protocol_task_handle(command_string);
        }
        
        led_task_handle();
        sleep_ms(10);
    }
    
    return 0;
}