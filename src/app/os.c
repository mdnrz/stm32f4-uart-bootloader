#include "os.h"
#include "app/packet.h"
#include "libopencm3/stm32/iwdg.h"
#include "shared/flash-io.h"
#include "shared/drv-usart-dma.h"

volatile uint16_t osDoggyRegister = OS_DOGGY_RESET_VALUE;

TaskHandle_t os_create_task(const OS_TaskAttr_t attr, TaskHandle_t handle) {
    xTaskCreate(attr.function, attr.name, attr.stackSize, attr.parameters, attr.priority, &handle);
    return handle;
}

QueueHandle_t os_create_queue(const OS_QueueAttr_t attr, QueueHandle_t handle) {
    handle = xQueueCreateStatic(attr.length, attr.itemSize, attr.pxStorageBuf, attr.pxQueueBuf);
    return handle;
}

void os_init_rtos(void) {

    packet_tx_queueHandle = os_create_queue(packet_tx_queueAttr, packet_tx_queueHandle);
    packet_rx_taskHandle = os_create_task(packet_rx_taskAttr, packet_rx_taskHandle);
    packet_tx_taskHandle = os_create_task(packet_tx_taskAttr, packet_tx_taskHandle);

    usart_enable_idle_interrupt(gsm.port);
    usart_disable_rx_interrupt(gsm.port);

    usart_enable_idle_interrupt(dbg.port);
    usart_disable_rx_interrupt(dbg.port);

    iwdg_start();
    iwdg_set_period_ms(1000);
    while (iwdg_reload_busy() || iwdg_prescaler_busy());
    uint8_t dummy = 0;
    xTaskNotify(packet_tx_taskHandle, dummy, eSetValueWithOverwrite);
}

static uint16_t os_check_for_failure(void) {
    // Watchdog scheme:
    static uint16_t periodicFailCnt = 0;
    static uint16_t classifierFailCnt = 0;
    static uint16_t uartFailCnt = 0;
    static uint16_t periodicFailTrigger = 0x2500; // temporary
    static uint16_t classifierFailTrigger = 0x05; // temporary
    static uint16_t uartFailTrigger = 0x02;       // temporary
    if ((osDoggyRegister & PERIODIC_CHAIN) == PERIODIC_CHAIN) {
        periodicFailCnt = 0;
        osDoggyRegister &= ~(uint16_t)PERIODIC_CHAIN;
    } else {
        periodicFailCnt++;
        if (periodicFailCnt > periodicFailTrigger) {
            return osDoggyRegister & PERIODIC_CHAIN;
        } else osDoggyRegister &= ~(uint16_t)PERIODIC_CHAIN;
    }
    if ((osDoggyRegister & CLASSIFIER_CHAIN) == (CLASSIFIER_CHAIN)) {
        classifierFailCnt = 0;
        osDoggyRegister &= ~(uint16_t)CLASSIFIER_CHAIN;
    } else if ((osDoggyRegister & CLASSIFIER_CHAIN) == OS_DOGGY_CLASSIFIER_Q) {
        classifierFailCnt++;
        if (classifierFailCnt > classifierFailTrigger) {
            return osDoggyRegister & CLASSIFIER_CHAIN;
        } else osDoggyRegister &= ~(uint16_t)CLASSIFIER_CHAIN;
    }
    if ((osDoggyRegister & UART_CHAIN) == (UART_CHAIN)) {
        uartFailCnt = 0;
        osDoggyRegister &= ~(uint16_t)UART_CHAIN;
    } else if ((osDoggyRegister & UART_CHAIN) == OS_DOGGY_UART_DATA) {
        uartFailCnt++;
        if (uartFailCnt > uartFailTrigger) {
            return osDoggyRegister & UART_CHAIN;
        } else osDoggyRegister &= ~(uint16_t)UART_CHAIN;
    }
    return 0;
}

// Automatically created idle_task by FreeRTOS
void vApplicationIdleHook(void) {
    for (;;) {
        vTaskEnterCritical();
        uint16_t result = os_check_for_failure();
        if (result == 0) {
            iwdg_reset();
        } else {
            // Dump the osDoggyRegister to flash area
            flash_io_dump_diagnostic(osDoggyRegister);
            // Go for reboot
        }
        vTaskExitCritical();
    }
}
