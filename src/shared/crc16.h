#ifndef __CRC16_H__
#define __CRC16_H__
#include <stdint.h>

uint16_t crc16_calculate(const uint8_t *data, uint16_t size);
uint32_t crc32_calculate(const uint8_t *data, uint16_t size);

#endif // __CRC16_H__
