#include "app/os.h"
#include "libopencm3/cm3/scb.h"
#include "shared/protocol.h"
#include "shared/mem-blocks.h"
#include "shared/drv-gpio.h"
#include "shared/drv-rcc.h"
#include "shared/drv-usart-dma.h"
#include "app/packet.h"

#include <string.h>

#define DBGMCU_APB1_FZ MMIO32(DBGMCU_BASE + 0x08)
#define DBGMCU_APB2_FZ MMIO32(DBGMCU_BASE + 0x0C)

// symbols related to ccm memory area in linker script
extern unsigned _ccm_loadaddr, _sccmram, _eccmram;

// Copy data from rom/ram to ccm
static void initialize_ccm(void) {
	volatile unsigned *src, *dest;
	for (src = &_ccm_loadaddr, dest = &_sccmram; dest < &_eccmram; src++, dest++) {
		*dest = *src;
	}
}

static void vector_table_setup(void) { SCB_VTOR = MEM_BL_SIZE + MEM_PARAMS_SIZE; }

int main(void) {
    initialize_ccm();
    vector_table_setup();
    res_rcc_setup();
    res_gpio_setup();
    res_usart_init();
    res_usart_dma_gsm_setup();
    res_usart_dma_dbg_setup();
    packet_init();

    os_init_rtos();

    DBGMCU_APB1_FZ |= 0x0002; // stop TIM3 on debug breaks
    DBGMCU_APB2_FZ |= 0x0001; // stop TIM1 on debug breaks

    vTaskStartScheduler();
    while (1) {
    }

    return 0;
}

// Dummy implementation for syscalls to avoid nosys warnings
void _close(void);
void _fstat(void);
void _getpid(void);
void _isatty(void);
void _kill(void);
void _lseek(void);
void _read(void);
void _write(void);

void _close(void) {}
void _fstat(void) {}
void _getpid(void) {}
void _isatty(void) {}
void _kill(void) {}
void _lseek(void) {}
void _read(void) {}
void _write(void) {}
