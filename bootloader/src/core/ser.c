#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <termios.h>
#include <sys/epoll.h>
#include <string.h>
#include <errno.h>

#define SERIAL_PORT "/dev/ttyUSB0"
#define BAUD_RATE B921600

struct termios tty;

int configure_serial_port(int fd) {

    if (tcgetattr(fd, &tty) != 0) {
        return -1;
    }

    // Set baud rate
    cfsetospeed(&tty, BAUD_RATE);
    cfsetispeed(&tty, BAUD_RATE);

    // Configure 8N1 mode (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB; // No parity bit
    tty.c_cflag &= ~CSTOPB; // Only one stop bit
    tty.c_cflag &= ~CSIZE;  // Clear current data bit setting
    tty.c_cflag |= CS8;     // 8 data bits

    // Disable hardware flow control
    tty.c_cflag &= ~CRTSCTS;

    // Enable the receiver and set local mode
    tty.c_cflag |= (CLOCAL | CREAD);

    // Disable canonical mode, echo, and signal chars
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Disable XON/XOFF flow control on input and output
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Set raw output mode
    tty.c_oflag &= ~OPOST;

    // Set timeout to return immediately if no data is available
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 0x05;

    tcflush(fd, TCIOFLUSH);
    tcdrain(fd);
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        return -1;
    }

    return 0;
}

int ser_send_packet(int port_fd, uint8_t *data, size_t len)
{
    return write(port_fd, data, len);
}

int ser_listen_and_receive_packet(int port_fd, uint8_t *rx_buffer, uint8_t len, int timeout_ds)
{
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = timeout_ds;
    if (tcsetattr(port_fd, TCSANOW, &tty) != 0) {
        return -1;
    }
    int read_cnt = read(port_fd, rx_buffer, len);
    if (read_cnt == 0 || read_cnt == len) return read_cnt;
    tty.c_cc[VMIN] = len - read_cnt;
    if (tcsetattr(port_fd, TCSANOW, &tty) != 0) {
        return -1;
    }
    return read(port_fd, rx_buffer + read_cnt, len - read_cnt);
}
