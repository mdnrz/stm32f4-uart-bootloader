#include "ring-buffer.h"

void ring_buffer_setup(RingBuffer_t *rb, uint8_t *buffer, uint16_t len) {
    rb->buffer = buffer;
    rb->len = len;
    rb->head = 0;
    rb->tail = 0;
}

bool ring_buffer_empty(RingBuffer_t *rb) { return rb->head == rb->tail; }

bool ring_buffer_read(RingBuffer_t *rb, uint8_t *byte) {
    uint16_t localHead = rb->head;
    uint16_t localTail = rb->tail;

    if (localHead == localTail)
        return false;
    *byte = rb->buffer[localHead];
    localHead = (localHead + 1) & (rb->len - 1);
    rb->head = localHead;
    return true;
}

bool ring_buffer_write(RingBuffer_t *rb, const uint8_t byte) {
    // uint16_t localHead = rb->head;
    uint16_t localTail = rb->tail;
    rb->buffer[localTail] = byte;

    uint16_t nextTail = (localTail + 1) & (rb->len - 1);
    // if (nextTail == localHead)
    //     return false; // TODO: what should be done here: overwrite or return?
    rb->tail = nextTail;
    return true;
}

uint16_t ring_buffer_capacity(RingBuffer_t *rb) {
    uint16_t localHead = rb->head;
    uint16_t localTail = rb->tail;
    if (localHead == localTail)
        return rb->len;
    if (localTail > localHead)
        return rb->len - (localTail - localHead);
    return (localHead - localTail);
}

uint16_t ring_buffer_read_chunk(RingBuffer_t *rb, uint8_t *dest, uint16_t size) {
    uint16_t localHead = rb->head;
    uint16_t localTail = rb->tail;
    uint16_t bytes_read = 0;
    if (localTail == localHead)
        return 0;
    while (bytes_read < size) {
        if (ring_buffer_read(rb, dest + bytes_read))
            bytes_read++;
        else
            return bytes_read;
    }
    return size;
}

uint16_t ring_buffer_write_chunk(RingBuffer_t *rb, const uint8_t *src, const uint16_t size) {
    // uint16_t localHead = rb->head;
    // uint16_t localTail = rb->tail;
    uint16_t bytes_written = 0;
    while (bytes_written < size) {
        if (ring_buffer_write(rb, *(src + bytes_written)))
            bytes_written++;
        else
            return bytes_written;
    }
    return size;
}
