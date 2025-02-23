#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>

uint8_t *bl_packet_received(void);

void packet_init(void);
void packet_send_nack(void);
void packet_send_ack(void);

#endif // __PACKET_H__
