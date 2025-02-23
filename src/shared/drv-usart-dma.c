/*#include "drv-usart.h"*/
/*#include "shared/drivers/drv-dma.h"*/
#include "drv-usart-dma.h"
#include "shared/drv-gpio.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/f4/rcc.h>

static uint8_t dbgRxBuf[DMA_RX_BUFFER_SIZE];
static uint8_t dbgTxBuf[DMA_TX_BUFFER_SIZE];
static uint8_t gsmRxBuf[DMA_RX_BUFFER_SIZE];
static uint8_t gsmTxBuf[DMA_TX_BUFFER_SIZE];

USART_t dbg;
USART_t gsm;
USART_t *comm;

void res_usart_init(void)
{
    dbg.port = USART1;
    dbg.clock = RCC_USART1;
    dbg.nvic_irq = NVIC_USART1_IRQ;

    dbg.gpio_port = SERIAL_PORT;
    dbg.gpio_tx = SERIAL_TX_PIN;
    dbg.gpio_rx = SERIAL_RX_PIN;

    dbg.dma.addr = DMA2;
    dbg.dma.clock = RCC_DMA2;
    dbg.dma.tx_stream = DMA_STREAM7;
    dbg.dma.rx_stream = DMA_STREAM2;
    dbg.dma.txBuf = dbgTxBuf;
    dbg.dma.rxBuf = dbgRxBuf;
    dbg.dma.tx_stream_irq = NVIC_DMA2_STREAM7_IRQ; 
    dbg.dma.rx_stream_irq = NVIC_DMA2_STREAM2_IRQ; 

    gsm.port = USART2;
    gsm.clock = RCC_USART2;
    gsm.nvic_irq = NVIC_USART2_IRQ;

    gsm.gpio_port = GSM_PORT;
    gsm.gpio_tx = GSM_TX_PIN;
    gsm.gpio_rx = GSM_RX_PIN;

    gsm.dma.addr = DMA1;
    gsm.dma.clock = RCC_DMA1;
    gsm.dma.tx_stream = DMA_STREAM6;
    gsm.dma.rx_stream = DMA_STREAM5;
    gsm.dma.txBuf = gsmTxBuf;
    gsm.dma.rxBuf = gsmRxBuf;
    gsm.dma.tx_stream_irq = NVIC_DMA1_STREAM6_IRQ; 
    gsm.dma.rx_stream_irq = NVIC_DMA1_STREAM5_IRQ; 
}

static void setup_dma_for_usart(USART_t *usart)
{
    rcc_periph_clock_enable(usart->dma.clock);
    // Stream5: usart_rx
    dma_disable_stream(usart->dma.addr, usart->dma.rx_stream);
    dma_set_peripheral_address(usart->dma.addr, usart->dma.rx_stream, usart->port + 0x04); // DR register of usart->port
    dma_set_memory_address(usart->dma.addr, usart->dma.rx_stream, (uint32_t)usart->dma.rxBuf);
    dma_set_number_of_data(usart->dma.addr, usart->dma.rx_stream, DMA_RX_BUFFER_SIZE);
    dma_channel_select(usart->dma.addr, usart->dma.rx_stream, DMA_SxCR_CHSEL_4);
    dma_enable_memory_increment_mode(usart->dma.addr, usart->dma.rx_stream);
    dma_set_transfer_mode(usart->dma.addr, usart->dma.rx_stream, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
    dma_enable_circular_mode(usart->dma.addr, usart->dma.rx_stream);
    
    // Stream6: usart_tx
    dma_disable_stream(usart->dma.addr, usart->dma.tx_stream);
    dma_set_peripheral_address(usart->dma.addr, usart->dma.tx_stream, usart->port + 0x04); // DR register of usart->port
    dma_channel_select(usart->dma.addr, usart->dma.tx_stream, DMA_SxCR_CHSEL_4);
    dma_enable_memory_increment_mode(usart->dma.addr, usart->dma.tx_stream);
    dma_set_transfer_mode(usart->dma.addr, usart->dma.tx_stream, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

    nvic_set_priority(usart->dma.rx_stream_irq, 0x06 << 4);
    nvic_enable_irq(usart->dma.rx_stream_irq);
    nvic_set_priority(usart->dma.tx_stream_irq, 0x06 << 4);
    nvic_enable_irq(usart->dma.tx_stream_irq);
    dma_enable_transfer_complete_interrupt(usart->dma.addr, usart->dma.rx_stream);
    dma_enable_transfer_complete_interrupt(usart->dma.addr, usart->dma.tx_stream);

    dma_enable_stream(usart->dma.addr, usart->dma.rx_stream);
}

void res_usart_dma_gsm_setup(void)
{
    // Config peripheral
    rcc_periph_clock_enable(gsm.clock);
    usart_set_flow_control(gsm.port, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(gsm.port, 921600);
    usart_set_databits(gsm.port, 8);
    usart_set_stopbits(gsm.port, 1);
    usart_set_parity(gsm.port, 0);
    usart_set_mode(gsm.port, USART_MODE_TX_RX);
    usart_enable_rx_dma(gsm.port);
    usart_enable_tx_dma(gsm.port);

    gpio_mode_setup(gsm.gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, gsm.gpio_rx | gsm.gpio_tx);
    gpio_set_af(gsm.gpio_port, GPIO_AF7, gsm.gpio_rx | gsm.gpio_tx);

    // Enable interrupt
    nvic_set_priority(gsm.nvic_irq, 0x05 << 4);
    nvic_enable_irq(gsm.nvic_irq);

    gsm.dma.addr = DMA1;
    gsm.dma.clock = RCC_DMA1;
    gsm.dma.tx_stream = DMA_STREAM6;
    gsm.dma.rx_stream = DMA_STREAM5;
    gsm.dma.txBuf = gsmTxBuf;
    gsm.dma.rxBuf = gsmRxBuf;
    gsm.dma.tx_stream_irq = NVIC_DMA1_STREAM6_IRQ; 
    gsm.dma.rx_stream_irq = NVIC_DMA1_STREAM5_IRQ; 

    setup_dma_for_usart(&gsm);
}

void res_usart_dma_dbg_setup(void)
{
    // Config peripheral
    rcc_periph_clock_enable(dbg.clock);
    usart_set_flow_control(dbg.port, USART_FLOWCONTROL_NONE);
    usart_set_baudrate(dbg.port, 921600);
    usart_set_databits(dbg.port, 8);
    usart_set_stopbits(dbg.port, 1);
    usart_set_parity(dbg.port, 0);
    usart_set_mode(dbg.port, USART_MODE_TX_RX);
    usart_enable_rx_dma(dbg.port);
    usart_enable_tx_dma(dbg.port);

    gpio_mode_setup(dbg.gpio_port, GPIO_MODE_AF, GPIO_PUPD_NONE, dbg.gpio_rx | dbg.gpio_tx);
    gpio_set_af(dbg.gpio_port, GPIO_AF7, dbg.gpio_rx | dbg.gpio_tx);

    // Enable interrupt
    nvic_set_priority(dbg.nvic_irq, 0x05 << 4);
    nvic_enable_irq(dbg.nvic_irq);

    dbg.dma.addr = DMA2;
    dbg.dma.clock = RCC_DMA2;
    dbg.dma.tx_stream = DMA_STREAM7;
    dbg.dma.rx_stream = DMA_STREAM2;
    dbg.dma.txBuf = dbgTxBuf;
    dbg.dma.rxBuf = dbgRxBuf;
    dbg.dma.tx_stream_irq = NVIC_DMA2_STREAM7_IRQ; 
    dbg.dma.rx_stream_irq = NVIC_DMA2_STREAM2_IRQ; 

    setup_dma_for_usart(&dbg);
}

static void res_usart_write_byte(const uint32_t port, const uint8_t data) { usart_send_blocking(port, (uint16_t)data); }

void res_usart_write(const USART_t *usart, const uint8_t *data, const uint32_t length) {
    for (uint32_t i = 0; i < length; i++) {
        res_usart_write_byte(usart->port, data[i]);
    }
}

void res_usart_dma_write(const USART_t *usart, const uint16_t offset, const uint32_t length) {
    dma_set_memory_address(usart->dma.addr, usart->dma.tx_stream, (uint32_t)(usart->dma.txBuf + offset));
    dma_set_number_of_data(usart->dma.addr, usart->dma.tx_stream, length);
    dma_enable_stream(usart->dma.addr, usart->dma.tx_stream);
}

/*uint32_t res_usart_read(uint8_t *data, const uint32_t length) {*/
/*    if (length > 0 && dataAvailable) {*/
/*        *data = dataBuffer;*/
/*        dataAvailable = false;*/
/*        return 1;*/
/*    }*/
/*    return 0;*/
/*}*/

/*uint8_t res_usart_read_byte(void) {*/
/*    dataAvailable = false;*/
/*    return dataBuffer;*/
/*}*/

/*bool res_usart_data_available(void) { return dataAvailable; }*/

uint16_t res_usart_dma_get_buffer_tail(USART_t *usart) {
    return DMA_RX_BUFFER_SIZE - dma_get_number_of_data(usart->dma.addr, usart->dma.rx_stream);
}
