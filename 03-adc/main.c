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

static void on_tm_start(const char* args) {
    adc_task_set_state(ADC_TASK_STATE_RUN);
    printf("Telemetry started\n");
}

static void on_tm_stop(const char* args) {
    adc_task_set_state(ADC_TASK_STATE_IDLE);
    printf("Telemetry stopped\n");
}

static api_t device_api[] = {
    {"get_adc", on_get_adc, "Get ADC voltage"},
    {"get_temp", on_get_temp, "Get RP2040 temperature"},
    {"tm_start", on_tm_start, "Start telemetry"},
    {"tm_stop", on_tm_stop, "Stop telemetry"},
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
        adc_task_handle();
        sleep_ms(10);
    }
    
    return 0;
}