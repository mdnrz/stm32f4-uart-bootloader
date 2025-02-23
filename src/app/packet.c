#include "packet.h"
#include "app/os.h"
#include "libopencm3/stm32/f4/nvic.h"
#include "shared/protocol.h"
#include "shared/ring-buffer.h"
#include "shared/drv-usart-dma.h"
#include <string.h>
#include "libopencm3/cm3/scb.h"
#include "shared/flash-io.h"

RingBuffer_t dbgRxRingBuf;
RingBuffer_t dbgTxRingBuf;
RingBuffer_t gsmRxRingBuf;
RingBuffer_t gsmTxRingBuf;

uint8_t packetBuf[DMA_RX_BUFFER_SIZE] = {0};

typedef struct {
    uint16_t offset;
    uint16_t len;
    USART_t *usart;
} PacketTxChunk_t;

const OS_TaskAttr_t packet_rx_taskAttr = {
    .function = packet_rx_task,
    .name = "packet tx task",
    .stackSize = configMINIMAL_STACK_SIZE * 10,
    .parameters = NULL,
    .priority = 1,
};
TaskHandle_t packet_rx_taskHandle = NULL;

const OS_TaskAttr_t packet_tx_taskAttr = {
    .function = packet_tx_task,
    .name = "packet rx task",
    .stackSize = configMINIMAL_STACK_SIZE * 10,
    .parameters = NULL,
    .priority = 1,
};
TaskHandle_t packet_tx_taskHandle = NULL;

PacketTxChunk_t packet_tx_queueStorageBuf[8];
StaticQueue_t packet_tx_staticQueue;
OS_QueueAttr_t packet_tx_queueAttr = {.length = 8,
                                      .itemSize = sizeof(PacketTxChunk_t),
                                      .pxStorageBuf = (uint8_t *)packet_tx_queueStorageBuf,
                                      .pxQueueBuf = &packet_tx_staticQueue};
QueueHandle_t packet_tx_queueHandle = NULL;

static bool packetLineFree = true;

static void packet_send_nack(void) {
    uint8_t packet[4] = {PROTOCOL_TAG_NACK, 0, 30, 0};
    packet_tx_enqueue_data(comm, packet, sizeof packet);
}

static void packet_send_ack(void) {
    uint8_t packet[4] = {PROTOCOL_TAG_ACK, 0, 152, 3};
    packet_tx_enqueue_data(comm, packet, sizeof packet);
}


bool packet_usart_line_free(void) { return packetLineFree; }

static void packet_state_machine(ProtocolPacketAttr_t attr) {
    uint8_t response[DMA_RX_BUFFER_SIZE] = {0};
    switch (attr.tag) {
        case PROTOCOL_TAG_PR: {
              if (flash_io_read_params(response + PACK_DATA_INDEX, attr.offset, attr.length) == attr.length) {
                  protocol_create_packet(response, PROTOCOL_TAG_ACK, attr.length);
                  packet_tx_enqueue_data(comm, response, attr.length + 4);
              }
              else packet_send_nack();
        } break;
        case PROTOCOL_TAG_PW: {
              vTaskEnterCritical();
              if (flash_io_write_params(packetBuf + PACK_DATA_INDEX, attr.offset, attr.length) == attr.length) packet_send_ack();
              else packet_send_nack();
              vTaskExitCritical();
        } break;
        case PROTOCOL_TAG_FIR: {
              if (flash_io_read_fw_info(response + PACK_DATA_INDEX, attr.offset, attr.length) == attr.length) {
                  protocol_create_packet(response, PROTOCOL_TAG_ACK, attr.length);
                  packet_tx_enqueue_data(comm, response, attr.length + 4);
              }
              else packet_send_nack();
        } break;
        case PROTOCOL_TAG_RST: {
            packet_send_ack();
            vTaskDelay(100);
            scb_reset_core();
        } break;
        case PROTOCOL_TAG_LOCK: {
            packetLineFree = false;
            packet_send_ack();
        } break;
        case PROTOCOL_TAG_UNLOCK: {
            packetLineFree = true;
            packet_send_ack();
        } break;
        case PROTOCOL_TAG_DIAG: {
            uint8_t diagLength = flash_io_read_diagnostic(response + PACK_DATA_INDEX);
            protocol_create_packet(response, PROTOCOL_TAG_ACK, diagLength);
            packet_tx_enqueue_data(comm, response, diagLength + 4);
        } break;
        case PROTOCOL_TAG_ACK: {
        } break;
        case PROTOCOL_TAG_NACK: {
        } break;
        default:
            packet_send_nack();
            break;
    }
}

/* TODO: This can be removed probably
 * Find a way to bond ring buffers to dma buffers other than this so 
 * that in the packet_tx_enqueue_data function there's no need to
 * check the usart port
 */
void packet_init(void) {
    ring_buffer_setup(&gsmRxRingBuf, gsm.dma.rxBuf, DMA_RX_BUFFER_SIZE);
    ring_buffer_setup(&gsmTxRingBuf, gsm.dma.txBuf, DMA_TX_BUFFER_SIZE);
    ring_buffer_setup(&dbgRxRingBuf, dbg.dma.rxBuf, DMA_RX_BUFFER_SIZE);
    ring_buffer_setup(&dbgTxRingBuf, dbg.dma.txBuf, DMA_TX_BUFFER_SIZE);
    usart_enable(gsm.port);
    usart_enable(dbg.port);
    comm = &dbg; // set serial communication port to dbg (usart1)
}

void packet_rx_task(void *parameters) {
    (void)parameters;
    uint16_t len = 0;
    for (;;) {
        xTaskNotifyWait(0, 0, (uint32_t *)&len, portMAX_DELAY);
        osDoggyRegister |= OS_DOGGY_PACKET_T;
        if (comm == &dbg) ring_buffer_read_chunk(&dbgRxRingBuf, packetBuf, len);
        else ring_buffer_read_chunk(&gsmRxRingBuf, packetBuf, len);
        ProtocolPacketAttr_t attr = protocol_extract_packet_attr(packetBuf, false);
        if (attr.tag != PROTOCOL_TAG_INVALID) packet_state_machine(attr);
        else packet_send_nack();
    }
}

void packet_tx_task(void *parameters) {
    PacketTxChunk_t chunk;
    uint32_t dummy = 0;
    (void)parameters;
    for (;;) {
        if (xQueuePeek(packet_tx_queueHandle, &chunk, portMAX_DELAY)) {
            xTaskNotifyWait(0, 0, &dummy, portMAX_DELAY);
            xQueueReceive(packet_tx_queueHandle, &chunk, portMAX_DELAY);
            res_usart_dma_write(chunk.usart, chunk.offset, chunk.len);
        }
    }
}

void packet_tx_enqueue_data(USART_t *usart, const uint8_t *data, const uint16_t len) {
    // write data to tx ring buffer
    PacketTxChunk_t chunk;
    RingBuffer_t *pBuf = NULL;
    if (usart == &gsm) pBuf = &gsmTxRingBuf;
    else if (usart == &dbg) pBuf = &dbgTxRingBuf;
    chunk.usart = usart;
    ring_buffer_write_chunk(pBuf, data, len);
    if ((DMA_TX_BUFFER_SIZE - pBuf->head) < len) {
        chunk.offset = pBuf->head;
        chunk.len = DMA_TX_BUFFER_SIZE - pBuf->head;
        xQueueSend(packet_tx_queueHandle, &chunk, 2);
        chunk.offset = 0;
        chunk.len = len - (DMA_TX_BUFFER_SIZE - pBuf->head);
        xQueueSend(packet_tx_queueHandle, &chunk, 2);
        pBuf->head = pBuf->tail;
        return;
    }
    chunk.offset = pBuf->head;
    chunk.len = len;
    pBuf->head = pBuf->tail;
    // Enqueue whole chunk for transmit
    xQueueSend(packet_tx_queueHandle, &chunk, 2);
}

void usart2_isr(void) {
    uint16_t packetLen = 0;
    (void)USART_SR(USART2);
    (void)USART_DR(USART2); // This read sequence clears the idle interrupt flag
    gsmRxRingBuf.tail = res_usart_dma_get_buffer_tail(&gsm);
    packetLen = (DMA_RX_BUFFER_SIZE - 1) & (gsmRxRingBuf.tail - gsmRxRingBuf.head);
    comm = &gsm;
    BaseType_t xHigherTaskWoken = pdFALSE;
    xTaskNotifyFromISR(packet_rx_taskHandle, packetLen, eSetValueWithOverwrite, &xHigherTaskWoken);
    osDoggyRegister |= OS_DOGGY_UART_DATA;
    portYIELD_FROM_ISR(xHigherTaskWoken);
}

void usart1_isr(void) {
    uint16_t packetLen = 0;
    (void)USART_SR(USART1);
    (void)USART_DR(USART1); // This read sequence clears the idle interrupt flag
    dbgRxRingBuf.tail = res_usart_dma_get_buffer_tail(&dbg);
    packetLen = (DMA_RX_BUFFER_SIZE - 1) & (dbgRxRingBuf.tail - dbgRxRingBuf.head);
    comm = &dbg;
    BaseType_t xHigherTaskWoken = pdFALSE;
    xTaskNotifyFromISR(packet_rx_taskHandle, packetLen, eSetValueWithOverwrite, &xHigherTaskWoken);
    osDoggyRegister |= OS_DOGGY_UART_DATA;
    portYIELD_FROM_ISR(xHigherTaskWoken);
}

void dma1_stream5_isr(void) {
    // TODO: deactivate dma interrupt if nothing is done here
    dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
}

void dma2_stream2_isr(void) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM2, DMA_TCIF);
}

void dma2_stream7_isr(void) {
    uint8_t dummy = 0;
    dma_clear_interrupt_flags(DMA2, DMA_STREAM7, DMA_TCIF);
    dma_disable_stream(DMA2, DMA_STREAM7);
    BaseType_t xHigherTaskWoken = pdFALSE;
    xTaskNotifyFromISR(packet_tx_taskHandle, dummy, eSetValueWithOverwrite, &xHigherTaskWoken);
    portYIELD_FROM_ISR(xHigherTaskWoken);
}

void dma1_stream6_isr(void) {
    uint8_t dummy = 0;
    dma_clear_interrupt_flags(DMA1, DMA_STREAM6, DMA_TCIF);
    dma_disable_stream(DMA1, DMA_STREAM6);
    BaseType_t xHigherTaskWoken = pdFALSE;
    xTaskNotifyFromISR(packet_tx_taskHandle, dummy, eSetValueWithOverwrite, &xHigherTaskWoken);
    portYIELD_FROM_ISR(xHigherTaskWoken);
}
