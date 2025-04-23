// max31855.h - Header file for MAX31855 Thermocouple Driver
#ifndef MAX31855_H
#define MAX31855_H

#include <stdint.h>
#include <stdbool.h>

#define MAX31855_FAULT_OPEN 0x01
#define MAX31855_FAULT_SHORT_GND 0x02
#define MAX31855_FAULT_SHORT_VCC 0x04
#define MAX31855_FAULT_ALL 0x07
#define MAX31855_FAULT_NONE 0x00

// Initialize the MAX31855
bool max31855_init(void);

// Read raw 32-bit data from the MAX31855
uint32_t max31855_read_raw(void);

// Read internal temperature (in Celsius)
double max31855_read_internal_temp(void);

// Read thermocouple temperature (in Celsius)
double max31855_read_thermocouple_temp(void);

// Read thermocouple temperature (in Fahrenheit)
double max31855_read_thermocouple_temp_f(void);

// Read fault state
uint8_t max31855_read_error(void);

// Set faults to check during temperature reads
void max31855_set_fault_checks(uint8_t faults);

#endif // MAX31855_H
