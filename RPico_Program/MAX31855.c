// max31855.c - Library implementation for MAX31855 Thermocouple Driver
#include "max31855.h"
#include <math.h>
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define SPI_PORT spi0
#define SCLK_PIN 18
#define MISO_PIN 16
#define CS_PIN 17

static bool initialized = false;
static uint8_t faultMask = 0x07; // Default to check all faults

// Initialize the MAX31855
bool max31855_init() {
    if (initialized) {
        return true;
    }

    // Configure SPI
    spi_init(SPI_PORT, 1000000); // 1 MHz clock
    gpio_set_function(SCLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    // Configure CS pin
    gpio_init(CS_PIN);
    gpio_set_dir(CS_PIN, GPIO_OUT);
    gpio_put(CS_PIN, 1); // Set CS high initially

    initialized = true;
    return true;
}

// Read 32 bits of data from the MAX31855
uint32_t max31855_read_raw() {
    uint32_t data = 0;
    uint8_t buf[4] = {0};

    if (!initialized) {
        max31855_init();
    }

    gpio_put(CS_PIN, 0); // Pull CS low to start transaction
    spi_read_blocking(SPI_PORT, 0x00, buf, 4); // Read 4 bytes
    gpio_put(CS_PIN, 1); // Pull CS high to end transaction

    data = buf[0];
    data = (data << 8) | buf[1];
    data = (data << 8) | buf[2];
    data = (data << 8) | buf[3];

    return data;
}

// Read internal temperature (in Celsius)
double max31855_read_internal_temp() {
    uint32_t raw = max31855_read_raw();

    // Ignore the bottom 4 bits and extract internal temperature
    raw >>= 4;
    int16_t temp = raw & 0x7FF;

    if (raw & 0x800) { // Check sign bit for negative temperature
        temp = 0xF800 | temp; // Sign extend
    }

    return temp * 0.0625; // Each LSB is 0.0625 degrees
}

// Read thermocouple temperature (in Celsius)
double max31855_read_thermocouple_temp() {
    int32_t raw = max31855_read_raw();

    if (raw & faultMask) {
        return NAN; // Return NaN if fault detected
    }

    if (raw & 0x80000000) { // Negative temperature
        raw = 0xFFFFC000 | ((raw >> 18) & 0x00003FFF); // Sign extend
    } else {
        raw >>= 18; // Positive temperature
    }

    return raw * 0.25; // Each LSB is 0.25 degrees
}

// Read thermocouple temperature (in Fahrenheit)
double max31855_read_thermocouple_temp_f() {
    double temp_c = max31855_read_thermocouple_temp();
    return temp_c * 9.0 / 5.0 + 32.0;
}

// Read fault state
uint8_t max31855_read_error() {
    return max31855_read_raw() & 0x7;
}

// Set faults to check during temperature reads
void max31855_set_fault_checks(uint8_t faults) {
    faultMask = faults & 0x07;
}
