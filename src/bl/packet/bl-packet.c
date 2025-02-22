#include "bl-packet.h"
#include "shared/comms/protocol/protocol.h"
#include "shared/comms/ring-buffer/ring-buffer.h"
#include "shared/drivers/drv-usart-dma.h"
#include "bl/stimer/stimer.h"
#include "libopencm3/stm32/f4/nvic.h"

RingBuffer_t dbgRxRingBuf;
RingBuffer_t dbgTxRingBuf;
RingBuffer_t gsmRxRingBuf;
RingBuffer_t gsmTxRingBuf;

bool packetReceived = false;
uint8_t buffer[255] = {0};

void packet_init(void) {
    ring_buffer_setup(&dbgRxRingBuf, dbg.dma.rxBuf, DMA_RX_BUFFER_SIZE);
    ring_buffer_setup(&dbgTxRingBuf, dbg.dma.txBuf, DMA_TX_BUFFER_SIZE);
    ring_buffer_setup(&gsmRxRingBuf, gsm.dma.rxBuf, DMA_RX_BUFFER_SIZE);
    ring_buffer_setup(&gsmTxRingBuf, gsm.dma.txBuf, DMA_TX_BUFFER_SIZE);
    usart_enable(gsm.port);
    usart_enable(dbg.port);
}

static void packet_tx_enqueue_data(const USART_t *usart, const uint8_t *data, const uint16_t len) {
    res_usart_write(usart, data, len);
}

void packet_send_nack(void) {
    uint8_t packet[4] = {PROTOCOL_TAG_NACK, 0, 30, 0};
    packet_tx_enqueue_data(comm, packet, sizeof packet);
}

void packet_send_ack(void) {
    uint8_t packet[4] = {PROTOCOL_TAG_ACK, 0, 152, 3};
    packet_tx_enqueue_data(comm, packet, sizeof packet);
}

uint8_t *bl_packet_received(void) {
    if (!packetReceived) {
        return 0;
    } else {
        packetReceived = false;
        return buffer;
    }
}

void usart2_isr(void) {
    (void)USART_SR(USART2);
    (void)USART_DR(USART2); // This read sequence clears the idle interrupt flag
    gsmRxRingBuf.tail = res_usart_dma_get_buffer_tail(&gsm);
    uint16_t len = (DMA_RX_BUFFER_SIZE - 1) & (gsmRxRingBuf.tail - gsmRxRingBuf.head);
    ring_buffer_read_chunk(&gsmRxRingBuf, buffer, len);
    comm = &gsm;
    packetReceived = true;
    // protocol_handle_packet(buffer);
}

void usart1_isr(void) {
    (void)USART_SR(USART1);
    (void)USART_DR(USART1); // This read sequence clears the idle interrupt flag
    dbgRxRingBuf.tail = res_usart_dma_get_buffer_tail(&dbg);
    uint16_t len = (DMA_RX_BUFFER_SIZE - 1) & (dbgRxRingBuf.tail - dbgRxRingBuf.head);
    ring_buffer_read_chunk(&dbgRxRingBuf, buffer, len);
    comm = &dbg;
    packetReceived = true;
}

void dma1_stream5_isr(void) {
    // TODO: deactivate dma interrupt if nothing is done here
    dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
}

void dma2_stream2_isr(void) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM2, DMA_TCIF);
}


