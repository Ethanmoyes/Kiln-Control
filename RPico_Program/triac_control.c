// triac_control.c
#include "triac_control.h"

#define PICO_DEFAULT_TIMER 0

int pulse_length = 200; //Duration the triac is active in us
int zero_cross_delay = 400; // Adjustment factor in us for delay between trigger and actual 0
volatile bool timer_active = false; // Flag to indicate if the timer is active
uint16_t analog; // = adc_read(); 0-4095
int phase_ctrl_range = 20; // Range to operate phase control
extern int debug_level;
void setup_zero_crossing(){
    gpio_set_irq_enabled_with_callback(zero_cross_pin, GPIO_IRQ_EDGE_RISE, true, &zero_crossing_callback);
}

//Function to start a timer
void zero_crossing_callback(uint gpio, uint32_t events) {
        struct repeating_timer timer;
        if (debug_level > 2) printf("Zero Cross Detected\n");
        if (drive.output != 0){
        add_repeating_timer_us((phase_delay + zero_cross_delay), (triac_pulse), NULL, &timer);
        }
}

// Function to trigger a pulse on the triac
bool triac_pulse() {
    gpio_put(triac_gate_pin, true);
    busy_wait_us(pulse_length);
    gpio_put(triac_gate_pin, false);
    return false;
}

int find_adc_input(uint gpio) {
        switch (gpio)
        {
        case 26: return 0; break;
        case 27: return 1; break;
        case 28: return 2; break;
        default: return -1; break;
            break;
        }
    }

void initialize_adc(int gpio){
    adc_init();
    adc_gpio_init(gpio);
    adc_select_input(find_adc_input(gpio)); // ADC 0 = GPIO 26
}

int get_adc(int gpio, int min, int max){
        adc_select_input(find_adc_input(gpio));
        return adc_map(adc_read(), 0, 4095, min, max); // Map 12-bit ADC value to 0-833us delay
}

uint32_t adc_map(uint32_t IN, uint32_t INmin, uint32_t INmax, uint32_t OUTmin, uint32_t OUTmax) {
    return ((((IN - INmin) * (OUTmax - OUTmin)) / (INmax - INmin)) + OUTmin);
}
