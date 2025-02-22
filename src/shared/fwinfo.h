#ifndef __FWINFO_H__
#define __FWINFO_H__

#include <stdint.h>

typedef struct __attribute__((__packed__)) {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t hash[5];
} FWVersion_t;

#define FWINFO_SENTINEL (0xDEADC0DE)

typedef struct __attribute__((__packed__)) {
    uint32_t sentinel;
    uint32_t deviceID;
    FWVersion_t version;
    uint32_t length;
    uint32_t crc32;
} FirmwareInfo_t;

// void firmware_write_info(void);

#endif // __FWINFO_H__
