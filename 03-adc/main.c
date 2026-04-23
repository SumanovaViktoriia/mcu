#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "protocol-task/protocol-task.h"
#include "led-task/led-task.h"
#include "adc-task/adc-task.h"

static void on_get_adc(const char* args) {
    float voltage = adc_task_read_voltage();
    printf("%f\n", voltage);
}

static void on_get_temp(const char* args) {
    float temperature = adc_task_read_temperature();
    printf("%f\n", temperature);
}

static api_t device_api[] = {
    {"get_adc", on_get_adc, "Get ADC voltage"},
    {"get_temp", on_get_temp, "Get RP2040 temperature"},
    {NULL, NULL, NULL}
};

int main() {
    stdio_init_all();
    
    led_task_init();
    adc_task_init();
    protocol_task_init(device_api);
    
    char buffer[256];
    int pos = 0;
    
    while(1) {
        int c = getchar_timeout_us(0);
        if (c != PICO_ERROR_TIMEOUT) {
            if (c == '\n' || c == '\r') {
                if (pos > 0) {
                    buffer[pos] = '\0';
                    protocol_task_handle(buffer);
                    pos = 0;
                }
            } else if (pos < (int)sizeof(buffer) - 1) {
                buffer[pos++] = (char)c;
            }
        }
        
        led_task_handle();
        sleep_ms(10);
    }
    
    return 0;
}