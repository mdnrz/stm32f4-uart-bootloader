#include "drv-timer.h"
#include "libopencm3/cm3/nvic.h"
#include "libopencm3/stm32/f4/nvic.h"
#include "libopencm3/stm32/f4/rcc.h"
#include "libopencm3/stm32/rcc.h"
#include "shared/parameters/parameters.h"
#include "shared/drivers/drv-gpio.h"

void res_captureTimer_setup(void) {
    rcc_periph_clock_enable(RCC_TIM1);
    rcc_periph_clock_enable(RCC_TIM2);

    timer_set_master_mode(capTimLo, TIM_CR2_MMS_UPDATE);
    timer_slave_set_trigger(capTimHi, TIM_SMCR_TS_ITR0);
    timer_slave_set_mode(capTimHi, TIM_SMCR_SMS_ECM1);

    timer_set_prescaler(capTimLo, 0);
    timer_set_period(capTimLo, 52500 - 1); // update rate = 3200 Hz
    timer_ic_set_input(capTimLo, TIM_IC1, TIM_IC_IN_TI1); // input capture mode
    timer_ic_set_filter(capTimLo, TIM_IC1, TIM_IC_OFF);
    timer_ic_set_prescaler(capTimLo, TIM_IC1, TIM_IC_PSC_8);

    timer_set_prescaler(capTimHi, 0);
    timer_set_period(capTimHi, 4 - 1); // total update rate = 800 Hz

    gpio_mode_setup(MUX_OUT0_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, MUX_OUT0_PIN);
    gpio_set_af(MUX_OUT0_PORT, GPIO_AF1, MUX_OUT0_PIN);

    // activate interrupt
    nvic_set_priority(NVIC_TIM1_CC_IRQ, 0x06 << 4);
    nvic_set_priority(NVIC_TIM2_IRQ, 0x07 << 4);
    nvic_enable_irq(NVIC_TIM1_CC_IRQ); // TIM1 Capture
    nvic_enable_irq(NVIC_TIM2_IRQ); // TIM2 Update
}

void res_timers_reconfig(void) {
    timer_set_period( capTimLo, (2 * rcc_apb1_frequency / ((TIM_PSC(capTimLo) + 1) * param.captureConfig.TB_SW_frequency)) - 1);
    (void)TIM_CCR1(capTimLo); // This clears the interrupt flag
    timer_set_counter(capTimLo, 0);
}
