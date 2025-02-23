#include "crc16.h"
#include <stdlib.h>

uint16_t crc16_calculate(const uint8_t *data, uint16_t size) {
#define POLY16 (uint16_t)0x8005
    uint16_t out = 0;
    int bits_read = 0, bit_flag;
    /* Sanity check: */
    if (data == NULL)
        return 0;
    while (size > 0) {
        bit_flag = out >> 15;
        /* Get next bit: */
        out <<= 1;
        out |= (*data >> (7 - bits_read)) & 1;
        /* Increment bit counter: */
        bits_read++;
        if (bits_read > 7) {
            bits_read = 0;
            data++;
            size--;
        }
        /* Cycle check: */
        if (bit_flag) {
            out ^= POLY16;
        }
    }
    return out;
}

uint32_t crc32_calculate(const uint8_t *data, uint16_t size) {
#define POLY32 (uint32_t)0xEDB88320
  uint32_t out = 0;
  int bits_read = 0, bit_flag;
  /* Sanity check: */
  if (data == NULL)
    return 0;
  while (size > 0) {
    bit_flag = out >> 31;
    /* Get next bit: */
    out <<= 1;
    out |= (*data >> (7 - bits_read)) & 1;
    /* Increment bit counter: */
    bits_read++;
    if (bits_read > 7) {
      bits_read = 0;
      data++;
      size--;
    }
    /* Cycle check: */
    if (bit_flag) {
      out ^= POLY32;
    }
  }
  return out;
}

