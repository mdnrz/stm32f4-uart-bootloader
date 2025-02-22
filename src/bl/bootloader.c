#include "bl/packet/bl-packet.h"
#include "shared/memory/mem-blocks.h"
#include "bl/update/update.h"
#include <shared/drivers/drv-gpio.h>
#include <shared/drivers/drv-rcc.h>
#include <shared/drivers/drv-usart-dma.h>
#include "bl/stimer/stimer.h"
#include <string.h>

static void jump_to_app(void) {
    typedef void (*void_fn)(void);

    uint32_t *reset_vector_entry = (uint32_t *)(MEM_APP_OFFSET + 4U);
    uint32_t *reset_vector = (uint32_t *)(*reset_vector_entry);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
    void_fn jump_fn = (void_fn)reset_vector;
#pragma GCC diagnostic pop

    jump_fn();
}

int main(void) {
    res_rcc_setup();
    stimer_systick_init();
    res_gpio_setup();
    res_usart_init();
    res_usart_dma_gsm_setup();
    res_usart_dma_dbg_setup();
    usart_enable_idle_interrupt(gsm.port);
    usart_disable_rx_interrupt(gsm.port);
    usart_enable_idle_interrupt(dbg.port);
    usart_disable_rx_interrupt(dbg.port);
    packet_init();
    update_state_machine();

    jump_to_app();
}
