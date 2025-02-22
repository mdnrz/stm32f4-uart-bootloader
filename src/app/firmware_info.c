#include "globals/version.h"
#include "shared/fwinfo.h"

__attribute__((section(".firmware_info"))) FirmwareInfo_t fw_info = {
    .sentinel = FWINFO_SENTINEL,
    .deviceID = 0x454,
    .version =
        {
            .major = gGIT_VERSION_MAJOR,
            .minor = gGIT_VERSION_MINOR,
            .patch = gGIT_VERSION_PATCH,
            .hash = gGIT_VERSION_HASH,
        },
    .length = 0xffffffff,
    .crc32 = 0xaaaaaaaa,
};
