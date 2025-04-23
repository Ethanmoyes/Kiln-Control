// triac_control.h
#ifndef TRIAC_CONTROL_H
#define TRIAC_CONTROL_H

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "pico/multicore.h"
#include "pid_control.h"


#define zero_cross_pin 21
#define triac_gate_pin 20
#define phase_adc 26

extern int phase_delay;
extern drive_char drive;
extern pid_param pid_terms;



// Function prototypes
void zero_crossing_callback(uint gpio, uint32_t events);
bool triac_pulse();
int find_adc_input(uint gpio);
void initialize_adc(int gpio);
int get_adc(int gpio, int min, int max);
uint32_t adc_map(uint32_t IN, uint32_t INmin, uint32_t INmax, uint32_t OUTmin, uint32_t OUTmax); // Scale input range to an output range
void setup_zero_crossing();

#endif // TRIAC_CONTROL_H
