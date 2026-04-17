#include <stdio.h>
#include "pico/stdlib.h"

#define LED_PIN 25
#define DEVICE_NAME "my-pico-device"
#define DEVICE_VRSN "v0.0.1"

int main() {
    stdio_init_all();
    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    while (1) {
        char symbol = getchar();
        
        switch(symbol) {
            case 'e':
                gpio_put(LED_PIN, true);
                printf("led enable done\n");
                break;
                
            case 'd':
                gpio_put(LED_PIN, false);
                printf("led disable done\n");
                break;
                
            case 'v':
                printf("Device: %s\n", DEVICE_NAME);
                printf("Version: %s\n", DEVICE_VRSN);
                break;
                
            default:
                break;
        }
    }
}