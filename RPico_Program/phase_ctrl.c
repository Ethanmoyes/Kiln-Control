/* Program Todos
Linearize Thermocouple Data
Phase delay -> 0-100.00% power = x 
phase_delay = drive.max-(-.5cos(pi*x)+.5)*drive.max
*/

#include <stdio.h>
#include "pico/stdlib.h"
#include "triac_control.h"
#include "MAX31855.h"
#include "pid_control.h"
#include "uart_comm.h"
#include "math.h"

int debug_level = 0; // Debug level 0-3
uint outputs[] = {triac_gate_pin}; // Initialize outputs
uint inputs[] = {zero_cross_pin}; // Initialize inputs
uint hysterisis[] = {0}; // Enable or disable hysteresis

int phase_delay = 0;

extern pid_gains gains;
extern drive_char drive;
extern pid_param pid_terms;
extern int phase_ctrl_range;

void initializeIO(uint *outputs,uint *inputs, uint *hysteresis); // Setup inputs and Outputs

int main() {    
    initializeIO(outputs,inputs,hysterisis);
    init_pid(&pid_terms);
    max31855_init();
    uart_comm_init();
    gpio_put(triac_gate_pin, false);
    gpio_set_pulls(zero_cross_pin, 0, 0); // Disable Pull ups/downs, uses external pull up
    multicore_launch_core1(&setup_zero_crossing); // Start zero crossing and triac trigger on core 1
    int cycles = 0;

    while (true) {
        
        if(cycles > 100){ //Restrict thermocouple read to every 100 cycles (1 second)
            float temp = max31855_read_thermocouple_temp_f();
            if(isinf(temp) || isnan(temp)); // Filter Bad Data
            else {
                pid_terms.pv = temp;
            }
            if (debug_level > 2) printf("%.2f %.2f %d \n", pid_terms.pv, pid_terms.sp, drive.output);
            cycles = 0;
            sleep_ms(5);
        }
        
        if(pid_terms.sp > 1999) { //Manual control
            drive.output = phase_delay=drive.max-(((pid_terms.sp-2000)/100)*drive.max); //Setpoint 2000+%power(0-100)
        }
        else{ //Auto/PID control
            phase_delay = drive.max - pid_drive(&pid_terms, &gains, &drive); // Since the delay is the inverse of the %power, drive max - drive power
        }

        cycles++;
        if (uart_data_available()) { // Monitor for data from RS485
            if(get_received_data(&pid_terms.sp, &gains.kP , &gains.kI, &gains.kD)) {  // Check if checksum was valid
                // Print to serial for graphing
                if (debug_level > 1) printf("%.2f %.2f %d \n", pid_terms.pv, pid_terms.sp, drive.output);
                send_temperature(pid_terms.pv, drive.output); // Send a response

            }
        }
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
