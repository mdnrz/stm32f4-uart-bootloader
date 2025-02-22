#ifndef __DRV_USART_DMA_H__
#define __DRV_USART_DMA_H__

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>

#define DMA_RX_BUFFER_SIZE (0x100U)  // 128B
#define DMA_TX_BUFFER_SIZE (0x400U) // 1KiB

typedef struct {
    uint32_t addr;
    uint32_t clock;
    uint32_t tx_stream;
    uint32_t rx_stream;
    uint8_t *rxBuf;
    uint8_t *txBuf;
    uint32_t tx_stream_irq;
    uint32_t rx_stream_irq;
} DMA_t;

typedef struct {
    uint32_t port;
    uint32_t clock;
    uint32_t nvic_irq;
    uint32_t gpio_port;
    uint32_t gpio_rx;
    uint32_t gpio_tx;
    DMA_t dma;
} USART_t;

extern USART_t dbg;
extern USART_t gsm;
extern USART_t *comm;

void res_usart_init(void);
void res_usart_dma_gsm_setup(void);
void res_usart_dma_dbg_setup(void);
void res_usart_write(const USART_t *usart, const uint8_t *data, const uint32_t length);
uint32_t res_usart_read(uint8_t *data, const uint32_t length);
uint8_t res_usart_read_byte(void);
bool res_usart_data_available(void);
void res_usart_dma_write(const USART_t *usart, const uint16_t offset, const uint32_t length);
uint16_t res_usart_dma_get_buffer_tail(USART_t *usart);

#endif // __DRV_USART_DMA_H__
