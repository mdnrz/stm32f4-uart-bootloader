#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *buffer;
    uint16_t len;
    uint16_t head;
    uint16_t tail;
} RingBuffer_t;

void ring_buffer_setup(RingBuffer_t *rb, uint8_t *buffer, uint16_t len);

bool ring_buffer_empty(RingBuffer_t *rb);

bool ring_buffer_read(RingBuffer_t *rb, uint8_t *byte);

bool ring_buffer_write(RingBuffer_t *rb, const uint8_t byte);

uint16_t ring_buffer_capacity(RingBuffer_t *rb);

uint16_t ring_buffer_read_chunk(RingBuffer_t *rb, uint8_t *dest, uint16_t size);

uint16_t ring_buffer_write_chunk(RingBuffer_t *rb, const uint8_t *src, const uint16_t size);
#endif // __RING_BUFFER_H__
