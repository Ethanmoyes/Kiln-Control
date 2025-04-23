#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/adc.h"
#include "hardware/irq.h"

#define zero_cross 2
#define phase_adc 26
#define triac 3
#define PICO_DEFAULT_TIMER 0

uint outputs[] = {triac}; // Initialize outputs
uint inputs[] = {zero_cross}; // Initialize inputs
uint hysterisis[] = {0}; // Enable or disable hysteresis

int phase_time = 8190; // us, length of each half cycle
int phase_delay;
int pulse_length = 300; //Duration the triac is active
int zero_cross_delay = 400; // Adjustment factor in us for delay between trigger and actual 0
volatile bool timer_active = false; // Flag to indicate if the timer is active

bool triac_pulse(struct repeating_timer *t); //Fire triac for pulse length when timer alarm triggers
void zero_crossing_callback(uint gpio, uint32_t events); //Start the timer to alarm after phase_delay
void initializeIO(uint *outputs,uint *inputs, uint *hysteresis); // Setup inputs and Outputs
uint32_t adc_map(uint32_t IN, uint32_t INmin, uint32_t INmax, uint32_t OUTmin, uint32_t OUTmax); // Scale input range to an output range

int main() {
    initializeIO(outputs,inputs,hysterisis); // Initialize the IO, hysterisis

    uint16_t analog; // = adc_read(); 0-4095
    adc_init();
    adc_gpio_init(phase_adc);
    adc_select_input(0); // ADC 0 = GPIO 26

    gpio_set_irq_enabled_with_callback(zero_cross, GPIO_IRQ_EDGE_RISE, true, &zero_crossing_callback);
    irq_set_priority(IO_IRQ_BANK0, PICO_HIGHEST_IRQ_PRIORITY); // Set highest priority for zero crossing
    gpio_put(triac, false);
    gpio_set_pulls(zero_cross, 0, 0);

    while (true) {
        analog = adc_read();
        phase_delay = adc_map(analog, 0, 4095, 0, phase_time); // Map 12-bit ADC value to 0-833us delay
        printf("Wait %dus\n", phase_delay);
        sleep_ms(1000);
    }
}

bool triac_pulse(struct repeating_timer *t) {
    gpio_put(triac, true);
    busy_wait_us(pulse_length);
    gpio_put(triac, false);
    timer_active = false; // Reset the flag when the timer completes
    return false;
}

void zero_crossing_callback(uint gpio, uint32_t events) {
    if (!timer_active) { // Only set the timer if it's not already active
        struct repeating_timer timer;
        irq_set_enabled(IO_IRQ_BANK0, false); // Disable interrupts
        add_repeating_timer_us((phase_delay + zero_cross_delay), triac_pulse, NULL, &timer);
        irq_set_enabled(IO_IRQ_BANK0, true); // Re-enable interrupts
        timer_active = true; // Set the flag to indicate the timer is active
    }
}

void initializeIO(uint *out, uint *in, uint *hyst) {
    #define ELEMENTS(x) (sizeof(x) / sizeof(x)[0])
    stdio_init_all(); // Initialize all pins
    for (int i = 0; i < ELEMENTS(out); i++) { // Setup each output in order
        gpio_init(out[i]);
        gpio_set_dir(out[i], true);
        gpio_put(out[i], 1);
    }
    for (int i = 0; i < ELEMENTS(in); i++) {
        gpio_init(in[i]);
        gpio_set_dir(in[i], GPIO_IN);
        if (hyst[i] == 1) {
            gpio_set_input_hysteresis_enabled(in[i], true);
        }
        gpio_pull_up(in[i]);
    }
}

uint32_t adc_map(uint32_t IN, uint32_t INmin, uint32_t INmax, uint32_t OUTmin, uint32_t OUTmax) {
    return ((((IN - INmin) * (OUTmax - OUTmin)) / (INmax - INmin)) + OUTmin);
}

