#ifndef __COMM_H__
#define __COMM_H__

typedef enum {
    COMMAND_PR,
    COMMAND_PU,
    COMMAND_RFI,
    COMMAND_RST,
    COMMAND_LOCK,
    COMMAND_UNLOCK,
    COMMAND_RDIAG,
    COMMAND_QUIT,
} Command_t;

uint16_t execute_command(uint8_t command, char *response);
int comm_open_serial_port(char *addr, char *response);
void comm_close_serial_port(void);
#endif // __COMM_H__
