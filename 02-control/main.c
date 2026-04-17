#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include <stdio.h>
#include "pico/stdlib.h"

#define DEVICE_NAME  "MyPicoDevice"
#define DEVICE_VRSN  "1.0.0"

void version_callback(const char* args);
void led_on_callback(const char* args);
void led_off_callback(const char* args);
void led_blink_callback(const char* args);
void led_blink_set_period_ms_callback(const char* args);
void help_callback(const char* args);
void mem_callback(const char* args);
void wmem_callback(const char* args);

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

int main(void)
{
    stdio_init_all();
    
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