#include <stdio.h>
#include "pico/stdlib.h"
#include "uart_comm.h"

int main() {
    stdio_init_all();
    uart_comm_init(); // Initialize UART communication

    float setpoint, a, b, c;

    while (1) {
        if (uart_data_available()) {
            get_received_data(&setpoint, &a, &b, &c);
            if (setpoint >= 0) { // Check if checksum was valid
                printf("Received: Setpoint=%.2f, a=%.2f, b=%.2f, c=%.2f\n", setpoint, a, b, c);
                send_temperature(72.50); // Send a response
            } else {
                printf("Checksum error!\n");
            }
        }
    }
}
