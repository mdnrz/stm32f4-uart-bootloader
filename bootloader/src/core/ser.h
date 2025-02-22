#ifndef __SER_H__
#define __SER_H__

#include <stdint.h>


int configure_serial_port(int fd);
int ser_send_packet(int port_fd, uint8_t *data, size_t len);
int ser_listen_and_receive_packet(int fd, uint8_t *rx_buffer, uint8_t len, int timeout_ms);

#endif // __SER_H__
