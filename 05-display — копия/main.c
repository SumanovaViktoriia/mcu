#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "protocol-task.h"
#include "led-task/led-task.h"
#include "stdio-task.h"
#include "bme280-driver.h"
#include "ili9341-driver.h"
#include "ili9341-display.h"
#include "ili9341-font.h"

#define DEVICE_NAME  "MyPicoDevice"
#define DEVICE_VRSN  "1.0.0"
#define BME280_I2C_ADDR 0x76

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

static ili9341_display_t ili9341_display = {0};

void rp2040_i2c_read(uint8_t* buffer, uint16_t length)
{
    i2c_read_timeout_us(i2c1, BME280_I2C_ADDR, buffer, length, false, 100000);
}

void rp2040_i2c_write(uint8_t* data, uint16_t size)
{
    i2c_write_timeout_us(i2c1, BME280_I2C_ADDR, data, size, false, 100000);
}

void rp2040_spi_write(const uint8_t* data, uint32_t size)
{
    spi_write_blocking(spi0, data, size);
}

void rp2040_spi_read(uint8_t* buffer, uint32_t length)
{
    spi_read_blocking(spi0, 0, buffer, length);
}

void rp2040_gpio_cs_write(bool level)
{
    gpio_put(ILI9341_PIN_CS, level);
}

void rp2040_gpio_dc_write(bool level)
{
    gpio_put(ILI9341_PIN_DC, level);
}

void rp2040_gpio_reset_write(bool level)
{
    gpio_put(ILI9341_PIN_RESET, level);
}

void rp2040_delay_ms(uint32_t ms)
{
    sleep_ms(ms);
}

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
void disp_px_callback(const char* args);
void disp_line_callback(const char* args);
void disp_rect_callback(const char* args);
void disp_frect_callback(const char* args);
void disp_text_callback(const char* args);
void disp_screen_callback(const char* args);  // НОВАЯ КОМАНДА

uint16_t color_from_string(const char* color_str)
{
    if (strcmp(color_str, "BLACK") == 0) return COLOR_BLACK;
    if (strcmp(color_str, "WHITE") == 0) return COLOR_WHITE;
    if (strcmp(color_str, "RED") == 0) return COLOR_RED;
    if (strcmp(color_str, "GREEN") == 0) return COLOR_GREEN;
    if (strcmp(color_str, "BLUE") == 0) return COLOR_BLUE;
    if (strcmp(color_str, "YELLOW") == 0) return COLOR_YELLOW;
    if (strcmp(color_str, "CYAN") == 0) return COLOR_CYAN;
    if (strcmp(color_str, "MAGENTA") == 0) return COLOR_MAGENTA;
    return COLOR_WHITE;
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
    {"disp_px", disp_px_callback, "Draw pixel: disp_px <x> <y> <COLOR>"},
    {"disp_line", disp_line_callback, "Draw line: disp_line <x0> <y0> <x1> <y1> <COLOR>"},
    {"disp_rect", disp_rect_callback, "Draw rectangle: disp_rect <x> <y> <w> <h> <COLOR>"},
    {"disp_frect", disp_frect_callback, "Draw filled rectangle: disp_frect <x> <y> <w> <h> <COLOR>"},
    {"disp_text", disp_text_callback, "Draw text: disp_text <x> <y> <COLOR> <text>"},
    {"disp_screen", disp_screen_callback, "Fill entire screen with color: disp_screen <COLOR>"},
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
    uint32_t temp = bme280_read_temp_raw();
    printf("%lu\n", temp);
}

void pres_raw_callback(const char* args)
{
    uint32_t pres = bme280_read_pres_raw();
    printf("%lu\n", pres);
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

void disp_px_callback(const char* args)
{
    int x, y;
    char color_str[32];
    
    if (sscanf(args, "%d %d %31s", &x, &y, color_str) != 3)
    {
        printf("Error: invalid arguments. Usage: disp_px <x> <y> <COLOR>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    ili9341_draw_pixel(&ili9341_display, x, y, color);
    printf("Pixel drawn at (%d, %d) with color %s\n", x, y, color_str);
}

void disp_line_callback(const char* args)
{
    int x0, y0, x1, y1;
    char color_str[32];
    
    if (sscanf(args, "%d %d %d %d %31s", &x0, &y0, &x1, &y1, color_str) != 5)
    {
        printf("Error: invalid arguments. Usage: disp_line <x0> <y0> <x1> <y1> <COLOR>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    ili9341_draw_line(&ili9341_display, x0, y0, x1, y1, color);
    printf("Line drawn from (%d,%d) to (%d,%d) with color %s\n", x0, y0, x1, y1, color_str);
}

void disp_rect_callback(const char* args)
{
    int x, y, w, h;
    char color_str[32];
    
    if (sscanf(args, "%d %d %d %d %31s", &x, &y, &w, &h, color_str) != 5)
    {
        printf("Error: invalid arguments. Usage: disp_rect <x> <y> <width> <height> <COLOR>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    ili9341_draw_rect(&ili9341_display, x, y, w, h, color);
    printf("Rectangle drawn at (%d,%d) size %dx%d with color %s\n", x, y, w, h, color_str);
}

void disp_frect_callback(const char* args)
{
    int x, y, w, h;
    char color_str[32];
    
    if (sscanf(args, "%d %d %d %d %31s", &x, &y, &w, &h, color_str) != 5)
    {
        printf("Error: invalid arguments. Usage: disp_frect <x> <y> <width> <height> <COLOR>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    ili9341_draw_filled_rect(&ili9341_display, x, y, w, h, color);
    printf("Filled rectangle drawn at (%d,%d) size %dx%d with color %s\n", x, y, w, h, color_str);
}

void disp_text_callback(const char* args)
{
    int x, y;
    char color_str[32];
    char text[128];
    
    int matched = sscanf(args, "%d %d %31s %127[^\n]", &x, &y, color_str, text);
    if (matched < 4)
    {
        printf("Error: invalid arguments. Usage: disp_text <x> <y> <COLOR> <text>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    
    while (text[0] == ' ') {
        for (int i = 0; text[i]; i++) {
            text[i] = text[i + 1];
        }
    }
    
    ili9341_draw_text(&ili9341_display, x, y, text, &jetbrains_font, color, COLOR_BLACK);
    printf("Text drawn at (%d,%d) with color %s: '%s'\n", x, y, color_str, text);
}

void disp_screen_callback(const char* args)
{
    char color_str[32];
    
    if (sscanf(args, "%31s", color_str) != 1)
    {
        printf("Error: invalid arguments. Usage: disp_screen <COLOR>\n");
        printf("Colors: BLACK, WHITE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA\n");
        return;
    }
    
    uint16_t color = color_from_string(color_str);
    ili9341_fill_screen(&ili9341_display, color);
    printf("Screen filled with color %s\n", color_str);
}

int main(void)
{
    stdio_init_all();
    
    stdio_task_init();
    
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    
    spi_init(spi0, 62500000);
    
    gpio_set_function(ILI9341_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(ILI9341_PIN_SCK, GPIO_FUNC_SPI);
    
    gpio_init(ILI9341_PIN_CS);
    gpio_set_dir(ILI9341_PIN_CS, GPIO_OUT);
    gpio_put(ILI9341_PIN_CS, 1);
    
    gpio_init(ILI9341_PIN_DC);
    gpio_set_dir(ILI9341_PIN_DC, GPIO_OUT);
    gpio_put(ILI9341_PIN_DC, 0);
    
    gpio_init(ILI9341_PIN_RESET);
    gpio_set_dir(ILI9341_PIN_RESET, GPIO_OUT);
    gpio_put(ILI9341_PIN_RESET, 0);
    
    ili9341_hal_t ili9341_hal = {0};
    ili9341_hal.spi_write = rp2040_spi_write;
    ili9341_hal.spi_read = rp2040_spi_read;
    ili9341_hal.gpio_cs_write = rp2040_gpio_cs_write;
    ili9341_hal.gpio_dc_write = rp2040_gpio_dc_write;
    ili9341_hal.gpio_reset_write = rp2040_gpio_reset_write;
    ili9341_hal.delay_ms = rp2040_delay_ms;
    
    ili9341_init(&ili9341_display, &ili9341_hal);
    
    ili9341_set_rotation(&ili9341_display, ILI9341_ROTATION_90);
    
    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
    sleep_ms(300);
    
    ili9341_draw_filled_rect(&ili9341_display, 10, 10, 100, 60, COLOR_RED);
    ili9341_draw_filled_rect(&ili9341_display, 120, 10, 100, 60, COLOR_GREEN);
    ili9341_draw_filled_rect(&ili9341_display, 230, 10, 80, 60, COLOR_BLUE);
    
    ili9341_draw_rect(&ili9341_display, 10, 90, 300, 80, COLOR_WHITE);
    
    ili9341_draw_line(&ili9341_display, 0, 0, 319, 239, COLOR_YELLOW);
    ili9341_draw_line(&ili9341_display, 319, 0, 0, 239, COLOR_CYAN);
    
    ili9341_draw_text(&ili9341_display, 20, 100, "Hello, ILI9341!", &jetbrains_font, COLOR_WHITE, COLOR_BLACK);
    
    ili9341_draw_text(&ili9341_display, 20, 116, "RP2040 / Pico SDK", &jetbrains_font, COLOR_YELLOW, COLOR_BLACK);
    
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