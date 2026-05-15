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

#define DEVICE_NAME  "AtmosphereMonitor"
#define DEVICE_VRSN  "2.0.0"
#define BME280_I2C_ADDR 0x76

#define ILI9341_PIN_MISO 4
#define ILI9341_PIN_CS 10
#define ILI9341_PIN_SCK 6
#define ILI9341_PIN_MOSI 7
#define ILI9341_PIN_DC 8
#define ILI9341_PIN_RESET 9

#define HISTORY_SIZE 60
#define MEASUREMENT_INTERVAL_MS 1000

static ili9341_display_t ili9341_display = {0};

static float temp_history[HISTORY_SIZE] = {0};
static float pres_history[HISTORY_SIZE] = {0};
static float hum_history[HISTORY_SIZE] = {0};
static int history_index = 0;
static int history_count = 0;

static float current_temp = 0;
static float current_pres = 0;
static float current_hum = 0;

static uint32_t last_measurement_time = 0;
static uint32_t measurement_interval_ms = MEASUREMENT_INTERVAL_MS;

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

void draw_battery_indicator(void)
{
    ili9341_draw_rect(&ili9341_display, 290, 5, 25, 12, COLOR_WHITE);
    ili9341_draw_filled_rect(&ili9341_display, 293, 8, 10, 6, COLOR_GREEN);
}

void draw_gauge(int x, int y, int width, int height, float value, float min_val, float max_val, uint16_t color)
{
    int fill_width = (int)((value - min_val) / (max_val - min_val) * width);
    if (fill_width < 0) fill_width = 0;
    if (fill_width > width) fill_width = width;
    
    ili9341_draw_rect(&ili9341_display, x, y, width, height, COLOR_WHITE);
    if (fill_width > 0) {
        ili9341_draw_filled_rect(&ili9341_display, x + 1, y + 1, fill_width - 2, height - 2, color);
    }
}

void draw_graph(int x, int y, int width, int height, float* data, int data_count, float min_val, float max_val, uint16_t color)
{
    if (data_count < 2) return;
    
    int prev_x = x;
    int prev_y = y + height - (int)((data[0] - min_val) / (max_val - min_val) * height);
    if (prev_y < y) prev_y = y;
    if (prev_y > y + height) prev_y = y + height;
    
    for (int i = 1; i < data_count; i++) {
        int curr_x = x + (i * width) / HISTORY_SIZE;
        int curr_y = y + height - (int)((data[i] - min_val) / (max_val - min_val) * height);
        if (curr_y < y) curr_y = y;
        if (curr_y > y + height) curr_y = y + height;
        
        ili9341_draw_line(&ili9341_display, prev_x, prev_y, curr_x, curr_y, color);
        prev_x = curr_x;
        prev_y = curr_y;
    }
}

void update_display(void)
{
    ili9341_fill_screen(&ili9341_display, COLOR_BLACK);
    
    draw_battery_indicator();
    
    ili9341_set_text_color(&ili9341_display, COLOR_CYAN, COLOR_BLACK);
    ili9341_set_font(&ili9341_display, &jetbrains_font);
    ili9341_draw_text(&ili9341_display, 10, 5, "Atmosphere Monitor", COLOR_CYAN, COLOR_BLACK);
    
    ili9341_draw_line(&ili9341_display, 0, 25, 320, 25, COLOR_WHITE);
    
    ili9341_set_text_color(&ili9341_display, COLOR_WHITE, COLOR_BLACK);
    
    char buffer[64];
    
    ili9341_draw_text(&ili9341_display, 10, 35, "Temperature:", COLOR_YELLOW, COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "%.1f C", current_temp);
    ili9341_draw_text(&ili9341_display, 150, 35, buffer, COLOR_WHITE, COLOR_BLACK);
    
    uint16_t temp_color = COLOR_RED;
    if (current_temp < 20) temp_color = COLOR_BLUE;
    if (current_temp >= 20 && current_temp < 25) temp_color = COLOR_GREEN;
    draw_gauge(10, 55, 300, 15, current_temp, 0, 40, temp_color);
    
    ili9341_draw_text(&ili9341_display, 10, 80, "Pressure:", COLOR_YELLOW, COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "%.1f hPa", current_pres);
    ili9341_draw_text(&ili9341_display, 150, 80, buffer, COLOR_WHITE, COLOR_BLACK);
    draw_gauge(10, 100, 300, 15, current_pres, 980, 1040, COLOR_GREEN);
    
    ili9341_draw_text(&ili9341_display, 10, 125, "Humidity:", COLOR_YELLOW, COLOR_BLACK);
    snprintf(buffer, sizeof(buffer), "%.1f %%", current_hum);
    ili9341_draw_text(&ili9341_display, 150, 125, buffer, COLOR_WHITE, COLOR_BLACK);
    
    uint16_t hum_color = COLOR_BLUE;
    if (current_hum < 30) hum_color = COLOR_YELLOW;
    if (current_hum > 70) hum_color = COLOR_RED;
    draw_gauge(10, 145, 300, 15, current_hum, 0, 100, hum_color);
    
    ili9341_draw_rect(&ili9341_display, 5, 170, 310, 65, COLOR_WHITE);
    ili9341_draw_text(&ili9341_display, 10, 175, "Last 60s History", COLOR_CYAN, COLOR_BLACK);
    
    ili9341_draw_text(&ili9341_display, 10, 195, "Temp", COLOR_RED, COLOR_BLACK);
    draw_graph(50, 190, 250, 35, temp_history, history_count, 15, 35, COLOR_RED);
    
    ili9341_draw_text(&ili9341_display, 10, 235, "Hum", COLOR_BLUE, COLOR_BLACK);
    draw_graph(50, 230, 250, 35, hum_history, history_count, 20, 80, COLOR_BLUE);
}

void add_to_history(float temp, float pres, float hum)
{
    temp_history[history_index] = temp;
    pres_history[history_index] = pres;
    hum_history[history_index] = hum;
    
    history_index = (history_index + 1) % HISTORY_SIZE;
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
}

void measure_sensors(void)
{
    current_temp = bme280_read_temp();
    current_pres = bme280_read_pres();
    current_hum = bme280_read_hum();
    
    add_to_history(current_temp, current_pres, current_hum);
    
    printf("Measurement: T=%.2f C, P=%.2f hPa, H=%.2f %%\n", 
           current_temp, current_pres, current_hum);
}

void version_callback(const char* args);
void help_callback(const char* args);
void set_period_callback(const char* args);

api_t device_api[] =
{
    {"version", version_callback, "get device name and firmware version"},
    {"help", help_callback, "print all available commands with descriptions"},
    {"set_period", set_period_callback, "set measurement period in ms"},
    {NULL, NULL, NULL},
};

static const int commands_count = sizeof(device_api) / sizeof(api_t) - 1;

void version_callback(const char* args)
{
    printf("device name: '%s', firmware version: %s\n", DEVICE_NAME, DEVICE_VRSN);
}

void help_callback(const char* args)
{
    printf("Available commands:\n");
    for (int i = 0; i < commands_count; i++)
    {
        printf("  %s - %s\n", device_api[i].command_name, device_api[i].command_help);
    }
}

void set_period_callback(const char* args)
{
    uint32_t period_ms = 0;
    sscanf(args, "%lu", &period_ms);
    
    if (period_ms < 100 || period_ms > 10000)
    {
        printf("Error: period must be between 100 and 10000 ms\n");
        return;
    }
    
    measurement_interval_ms = period_ms;
    printf("Measurement period set to %lu ms\n", measurement_interval_ms);
}

int main(void)
{
    stdio_init_all();
    sleep_ms(2000);
    
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
    ili9341_set_text_color(&ili9341_display, COLOR_WHITE, COLOR_BLACK);
    ili9341_set_font(&ili9341_display, &jetbrains_font);
    ili9341_draw_text(&ili9341_display, 50, 100, "Atmosphere Monitor", COLOR_CYAN, COLOR_BLACK);
    ili9341_draw_text(&ili9341_display, 80, 130, "Starting...", COLOR_WHITE, COLOR_BLACK);
    
    bme280_init(rp2040_i2c_read, rp2040_i2c_write);
    
    sleep_ms(500);
    
    for (int i = 0; i < 5; i++) {
        measure_sensors();
        sleep_ms(100);
    }
    
    protocol_task_init(device_api);
    led_task_init();
    
    last_measurement_time = time_us_64() / 1000;
    
    while (1)
    {
        char* command_string = stdio_task_handle();
        
        if (command_string != NULL)
        {
            protocol_task_handle(command_string);
        }
        
        uint32_t now = time_us_64() / 1000;
        if (now - last_measurement_time >= measurement_interval_ms) {
            last_measurement_time = now;
            measure_sensors();
            update_display();
        }
        
        led_task_handle();
        
        sleep_ms(10);
    }
    
    return 0;
}