#ifndef __OS_H__
#define __OS_H__

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/*
 * Number of cirtical tasks, queues, and interrupts (doggies ;P ) to be checked by watchdog
 * ** Periodic events:
 * 1. Capture timer interrupt
 * 2. Channel switching task
 * 3. Detector data queue
 * 4. Detector task
 * ** Semi-rare events:
 * 5. Classifier data queue
 * 6. Classifier task
 * ** Rare events:
 * 7. Usart receive interrupt
 * 8. Packet task
 */
#define OS_DOGGY_NUM          0x08
#define OS_DOGGY_RESET_VALUE  (0x01 << 0x00)
#define OS_DOGGY_TIM_CAPTURE  (0x01 << 0x01)
#define OS_DOGGY_CHANNEL_T    (0x01 << 0x02)
#define OS_DOGGY_DETECTOR_Q   (0x01 << 0x03)
#define OS_DOGGY_DETECTOR_T   (0x01 << 0x04)
#define OS_DOGGY_CLASSIFIER_Q (0x01 << 0x05)
#define OS_DOGGY_CLASSIFIER_T (0x01 << 0x06)
#define OS_DOGGY_UART_DATA    (0x01 << 0x07)
#define OS_DOGGY_PACKET_T     (0x01 << 0x08)

extern volatile uint16_t osDoggyRegister;

enum OS_DOGGY_REG_SAFE_VALUES {
    PERIODIC_CHAIN =
        (OS_DOGGY_DETECTOR_T | OS_DOGGY_DETECTOR_Q | OS_DOGGY_CHANNEL_T | OS_DOGGY_TIM_CAPTURE),
    CLASSIFIER_CHAIN = (OS_DOGGY_CLASSIFIER_T | OS_DOGGY_CLASSIFIER_Q),
    UART_CHAIN = (OS_DOGGY_PACKET_T | OS_DOGGY_UART_DATA),
};

typedef struct {
    TaskFunction_t function;
    const char *const name;
    const configSTACK_DEPTH_TYPE stackSize;
    void *const parameters;
    UBaseType_t priority;
} OS_TaskAttr_t;

typedef struct {
    UBaseType_t length;
    UBaseType_t itemSize;
    uint8_t *pxStorageBuf;
    StaticQueue_t *pxQueueBuf;
} OS_QueueAttr_t;

void os_init_rtos(void);
TaskHandle_t os_create_task(const OS_TaskAttr_t attr, TaskHandle_t handle);
QueueHandle_t os_create_queue(const OS_QueueAttr_t attr, QueueHandle_t handle);

#endif // __OS_H__
