#ifndef __PACKET_H__
#define __PACKET_H__

#include "app/os.h"
#include "shared/drv-usart-dma.h"
#include <stdbool.h>

extern const OS_TaskAttr_t packet_rx_taskAttr;
extern TaskHandle_t packet_rx_taskHandle;

extern const OS_TaskAttr_t packet_tx_taskAttr;
extern TaskHandle_t packet_tx_taskHandle;
extern OS_QueueAttr_t packet_tx_queueAttr;
extern QueueHandle_t packet_tx_queueHandle;

void packet_init(void);
void packet_breath(void);
void packet_tx_enqueue_data(USART_t *usart, const uint8_t *data, const uint16_t len);

void packet_rx_task(void *parameters);
void packet_tx_task(void *parameters);
bool packet_usart_line_free(void);

#endif // __PACKET_H__
