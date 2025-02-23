#ifndef __FLASH_IO_H__
#define __FLASH_IO_H__

#include <stdint.h>
#include <stdbool.h>

uint8_t flash_io_read_params(uint8_t *buffer, uint8_t offset, uint8_t length);
uint8_t flash_io_write_params(uint8_t *buffer, uint8_t offset, uint8_t length);
uint8_t flash_io_read_fw_info(uint8_t *buffer, uint8_t offset, uint8_t length);
uint8_t flash_io_read_diagnostic(uint8_t *buffer);
void flash_io_dump_diagnostic(uint16_t diagReg);
bool flash_io_check_dev_id(uint32_t *id);
uint32_t flash_io_check_update_size(uint32_t *size);
bool flash_io_erase_fw_partition(void);
uint32_t flash_io_write_new_firmware(uint8_t *chunk, uint16_t len);
bool flash_io_check_fw_integrity(void);
bool flash_io_write_main_entry(void);

#endif // __FLASH_IO_H__
