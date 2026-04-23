#include "adc-task.h"
#include "hardware/adc.h"

#define ADC_GPIO_PIN 26
#define ADC_CHANNEL_VOLTAGE 0
#define ADC_CHANNEL_TEMP 4
#define ADC_VREF 3.3f
#define ADC_MAX_COUNTS 4095

void adc_task_init(void) {
    adc_init();
    adc_gpio_init(ADC_GPIO_PIN);
    adc_set_temp_sensor_enabled(true);
}

float adc_task_read_voltage(void) {
    adc_select_input(ADC_CHANNEL_VOLTAGE);
    uint16_t voltage_counts = adc_read();
    float voltage_V = (voltage_counts * ADC_VREF) / ADC_MAX_COUNTS;
    return voltage_V;
}

float adc_task_read_temperature(void) {
    adc_select_input(ADC_CHANNEL_TEMP);
    uint16_t temp_counts = adc_read();
    float temp_V = (temp_counts * ADC_VREF) / ADC_MAX_COUNTS;
    float temp_C = 27.0f - (temp_V - 0.706f) / 0.001721f;
    return temp_C;
}