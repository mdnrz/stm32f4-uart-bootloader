#include "flash-io.h"
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "shared/parameters/parameters.h"
#include "shared/fwinfo.h"
#include "shared/memory/mem-blocks.h"

static uint32_t mainAppEntry = 0XFFFF;

uint8_t flash_io_read_params(uint8_t *buffer, uint8_t offset, uint8_t length){
    if ((offset + length) > (sizeof(Parameters_t))) {
        return 0;
    }
    memcpy(buffer, (uint8_t *)(MEM_PARAMS_OFFSET + offset), length);
    // TODO: handle errors here
    return length;
}

uint8_t flash_io_write_params(uint8_t *buffer, uint8_t offset, uint8_t length){
    if (length == 0 || offset != 0) {
        return 0;
    }         
    if ((offset + length) > (sizeof(Parameters_t))) {
        // TODO: send address out of range message
        return 0;
    } 
    flash_unlock();
    flash_erase_sector(MEM_PARAMS_SECTOR, 3);
    for (uint16_t i = 0; i < length; i++) {
        flash_program_byte(MEM_PARAMS_OFFSET + i, buffer[i]);
    }
    flash_lock();
    // TODO: handle errors here
    return length;
}

uint8_t flash_io_read_fw_info(uint8_t *buffer, uint8_t offset, uint8_t length){
    if ((offset + length) > (sizeof(FirmwareInfo_t))) {
        // TODO: send address out of range message
        return 0;
    } 
    memcpy(buffer, (uint8_t *)(MEM_FW_INFO_OFFSET + offset), length);
    return length;
}

uint8_t flash_io_read_diagnostic(uint8_t *buffer) {
    uint32_t *diagAddr = (uint32_t *)MEM_DOGGY_LITTER_BOX_OFFSET;
    uint16_t diagCount = 0;
    while ((uint16_t) * (diagAddr + diagCount) != 0xFFFF) {
        diagCount++;
    }
    memcpy(buffer, (uint8_t *)(MEM_DOGGY_LITTER_BOX_OFFSET), diagCount);
    return diagCount;
}

void flash_io_dump_diagnostic(uint16_t diagReg) {
    uint16_t *dumpAddr = (uint16_t *)MEM_DOGGY_LITTER_BOX_OFFSET;
    static bool registerDumped = false;
    // Search for empty untouched memory
    while (!registerDumped) {
        if (*dumpAddr == 0xFFFF) {
            flash_unlock();
            flash_program_half_word((uint32_t)dumpAddr, diagReg);
            flash_lock();
            registerDumped = true;
            while(true);
        }
        dumpAddr++;
        if (((uint32_t)dumpAddr - MEM_DOGGY_LITTER_BOX_OFFSET) >=
                MEM_DOGGY_LITTER_BOX_SIZE) {
            // Memory area is full. Reboot without dumping
            break;
        }
    }
}

bool flash_io_check_dev_id(uint32_t *id) {
    FirmwareInfo_t *fwInfo = (FirmwareInfo_t *)MEM_FW_INFO_OFFSET;
    uint32_t devID = fwInfo->deviceID;
    return *id == devID;
}

uint32_t flash_io_check_update_size(uint32_t *size) {
    if (*size > MEM_FW_CAPACITY) {
        return 0;
    }
    return *size;
}

bool flash_io_erase_fw_partition(void) {
    flash_unlock();
    for (uint8_t i = 0; i + MEM_PARAMS_SECTOR < 12; i++) {
        flash_erase_sector(MEM_PARAMS_SECTOR + i, 3);
    }
    // TODO: Check if the flash area is erased completely
    flash_lock();
    return true;
}

uint32_t flash_io_write_new_firmware(uint8_t *chunk, uint16_t len) {
    static uint32_t writtenBytesNo = 0;
    if (writtenBytesNo == MEM_PARAMS_SIZE) {
        // Hold on to first 4-bytes of vector table to flash it after everything else
        mainAppEntry = *(uint32_t *)chunk;
        *(uint32_t *)chunk = 0xFFFFFFFF; // Replace entry with all 1s
    }
    flash_unlock();
    for (uint8_t writeOffset = 0; writeOffset < len; writeOffset++) {
        flash_program_byte(MEM_PARAMS_OFFSET + (writeOffset + writtenBytesNo), chunk[writeOffset]);
    }
    flash_lock();
    writtenBytesNo += len;
    return len;
}

bool flash_io_check_fw_integrity(void) {
    uint32_t vecTableEntry = *(uint32_t *)MEM_APP_OFFSET;
    return !(vecTableEntry == 0xFFFFFFFF || vecTableEntry == 0x0); // Vector table is not written
}

// Write the vector table entry to flash memory
bool flash_io_write_main_entry(void) {
    flash_unlock();
    flash_program_word(MEM_APP_OFFSET, mainAppEntry);
    flash_lock();
    return true;
}
