#ifndef ADC_TASK_H
#define ADC_TASK_H

void adc_task_init(void);
float adc_task_read_voltage(void);
float adc_task_read_temperature(void);

#endif