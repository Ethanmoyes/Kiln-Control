#ifndef UART_COMM_H
#define UART_COMM_H

#include <stdint.h>
#include <stdbool.h>

#define UART_ID uart1
#define DE_PIN 7  // RS485 Direction Control
#define TX_PIN 8
#define RX_PIN 9
#define BAUD_RATE 57600
#define BUFFER_SIZE 64

void uart_comm_init(void);
void send_temperature(float temperature, float drive_output);
void uart_rx_handler(void);
bool uart_data_available(void);
bool get_received_data(float *setpoint, float *a, float *b, float *c);

#endif // UART_COMM_H
