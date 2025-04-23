#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "uart_comm.h"

volatile char rx_buffer[BUFFER_SIZE];
volatile int rx_index = 0;
volatile bool message_received = false;

// Calculate XOR checksum
uint8_t calculate_checksum(const char *data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

// UART Interrupt Handler
void uart_rx_handler() {
    while (uart_is_readable(UART_ID)) {
        char c = uart_getc(UART_ID);
        gpio_put(PICO_DEFAULT_LED_PIN, 1); // Blink LED on reception
        
        if (c == '\n' || rx_index >= BUFFER_SIZE - 1) { 
            rx_buffer[rx_index] = '\0';
            rx_index = 0;
            message_received = true;
        } else {
            rx_buffer[rx_index++] = c;
        }
        
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
    }
}

// Initialize UART Communication
void uart_comm_init(void) {
    uart_init(UART_ID, BAUD_RATE);  // Updated baud rate to 57600
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);

    gpio_init(DE_PIN);
    gpio_set_dir(DE_PIN, GPIO_OUT);
    gpio_put(DE_PIN, 0); // Start in receive mode

    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    // Set UART interrupt to lower priority (higher number = lower priority)
    uart_set_irq_enables(UART_ID, true, false);  // Enable RX interrupt
    irq_set_exclusive_handler(UART1_IRQ, uart_rx_handler);  // Set handler for UART1 interrupt
    irq_set_priority(UART1_IRQ, 32);  // Set priority to 32 (lower than default priority)
    irq_set_enabled(UART1_IRQ, true);  // Enable UART interrupt

    // GPIO and other interrupts can be set to higher priorities if needed
    // Example: irq_set_priority(GPIO_IRQ, 0); for GPIO interrupt at highest priority
}

// Send temperature response
void send_temperature(float temperature, float drive_output) {
    char response[BUFFER_SIZE];
    int len = snprintf(response, sizeof(response), "%.2f:%.2f:", temperature, drive_output);
    uint8_t checksum = calculate_checksum(response, len);
    snprintf(response + len, sizeof(response) - len, "%d\n", checksum);

    gpio_put(DE_PIN, 1); // Enable TX mode
    uart_puts(UART_ID, response);
    uart_tx_wait_blocking(UART_ID); // Ensure transmission completes before switching to RX
    gpio_put(DE_PIN, 0); // Back to RX mode
}

// Check if new data is available
bool uart_data_available(void) {
    return message_received;
}

// Retrieve received data. Returns True if checksum is valid, False otherwise
bool get_received_data(float *setpoint, float *a, float *b, float *c) {
    if (!message_received) return(0);

    message_received = false;
    float setpoint_t;
    float a_t;
    float b_t;
    float c_t;

    // Parse message manually (faster than sscanf)
    int received_checksum;
    char *token = strtok((char *)rx_buffer, ":");
    if (token) setpoint_t = atof(token);
    token = strtok(NULL, ":");
    if (token) a_t = atof(token);
    token = strtok(NULL, ":");
    if (token) b_t = atof(token);
    token = strtok(NULL, ":");
    if (token) c_t = atof(token);
    token = strtok(NULL, ":");
    if (token) received_checksum = atoi(token);

    // Validate checksum
    char temp_buffer[BUFFER_SIZE];
    snprintf(temp_buffer, BUFFER_SIZE, "%.2f:%.2f:%.2f:%.2f", setpoint_t, a_t, b_t, c_t);
    uint8_t calculated_checksum = calculate_checksum(temp_buffer, strlen(temp_buffer));

    if (calculated_checksum == received_checksum) {
        *setpoint = setpoint_t;
        *a = a_t;
        *b = b_t;
        *c = c_t;
        return(1);
    }
    else {
        printf("Checksum mismatch: %d != %d\n", calculated_checksum, received_checksum);
        return(0);
    }
}
