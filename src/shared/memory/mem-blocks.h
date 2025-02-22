#ifndef __MEM_BLOCKS_H__
#define __MEM_BLOCKS_H__

#include "libopencm3/cm3/vector.h"
#include "libopencm3/stm32/flash.h"
#include "shared/fwinfo.h"
#include "shared/parameters/parameters.h"

/* 
 * ### MEMORY MAP ###
 *
 * ******************************************** 0x0800000 Flash base  / Bootloader start
 *  Bootloader: 16KB
 * ******************************************** 0x0804000 Bootloader end / Parameters start
 *  Parameters: 8KB
 *  (Config and application settings)
 *  ----------------------- 0x8002000
 *  Doggy litter box: 8KB 
 *  (Watchdog register dump area)
 * ******************************************** 0x0808000 Parameters end / App start
 * Vector table
 * ------------------------
 *  Firmware info 
 *  (Version, size, etc)
 *  -----------------------
 *  Application code
 *
 * */

#define MEM_BL_SIZE                            (0x4000U)
#define MEM_PARAMS_OFFSET                      (FLASH_BASE + MEM_BL_SIZE)
#define MEM_PARAMS_SECTOR                      (0x1U)
#define MEM_DOGGY_LITTER_BOX_OFFSET            (MEM_PARAMS_OFFSET + 0x2000)
#define MEM_DOGGY_LITTER_BOX_SIZE              (0x2000)
#define MEM_PARAMS_SIZE                        (0x4000U)
#define MEM_APP_OFFSET                         (FLASH_BASE + MEM_BL_SIZE + MEM_PARAMS_SIZE)
#define MEM_APP_SECTOR                         (0x2U)
#define MEM_FW_CAPACITY                        (1024U * 512U - MEM_BL_SIZE - MEM_PARAMS_SIZE)
#define MEM_FW_INFO_OFFSET                     (MEM_APP_OFFSET + sizeof(vector_table_t))
#define MEM_FW_INFO_VALIDATE_FROM              (MEM_PARAMS_OFFSET)
#define MEM_FW_INFO_VALIDATE_LENGTH(fw_length) (fw_length + MEM_PARAMS_SIZE)

typedef enum {
    FW_INFO_SENTINEL,
    FW_INFO_DEVID,
    FW_INFO_VERSION,
    FW_INFO_LENGTH,
    FW_INFO_CRC32,
} FWInfoElement_t;

#endif // __MEM_BLOCKS_H__
