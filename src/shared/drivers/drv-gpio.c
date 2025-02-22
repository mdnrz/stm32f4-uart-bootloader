#include "drv-gpio.h"

void res_gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_GPIOD);
    rcc_periph_clock_enable(RCC_GPIOE);
    rcc_periph_clock_enable(RCC_GPIOF);
    rcc_periph_clock_enable(RCC_GPIOG);

    gpio_mode_setup(ST_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ST_LED0_PIN);
    gpio_mode_setup(ST_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ST_LED1_PIN);
    gpio_mode_setup(ST_LED2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ST_LED2_PIN);
    gpio_mode_setup(ST_LED3_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ST_LED3_PIN);

    gpio_mode_setup(CH0_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH0_LED0_PIN);
    gpio_mode_setup(CH0_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH0_LED1_PIN);
    gpio_mode_setup(CH1_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH1_LED0_PIN);
    gpio_mode_setup(CH1_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH1_LED1_PIN);
    gpio_mode_setup(CH2_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH2_LED0_PIN);
    gpio_mode_setup(CH2_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH2_LED1_PIN);
    gpio_mode_setup(CH3_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH3_LED0_PIN);
    gpio_mode_setup(CH3_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH3_LED1_PIN);
    gpio_mode_setup(CH4_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH4_LED0_PIN);
    gpio_mode_setup(CH4_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH4_LED1_PIN);
    gpio_mode_setup(CH5_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH5_LED0_PIN);
    gpio_mode_setup(CH5_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH5_LED1_PIN);
    gpio_mode_setup(CH6_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH6_LED0_PIN);
    gpio_mode_setup(CH6_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH6_LED1_PIN);
    gpio_mode_setup(CH7_LED0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH7_LED0_PIN);
    gpio_mode_setup(CH7_LED1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, CH7_LED1_PIN);

    gpio_mode_setup(DBG0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DBG0_PIN);
    gpio_mode_setup(ACTIVE_CH0_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH0_PIN);
    gpio_mode_setup(ACTIVE_CH1_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH1_PIN);
    gpio_mode_setup(ACTIVE_CH2_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH2_PIN);
    gpio_mode_setup(ACTIVE_CH3_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH3_PIN);
    gpio_mode_setup(ACTIVE_CH4_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH4_PIN);
    gpio_mode_setup(ACTIVE_CH5_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH5_PIN);
    gpio_mode_setup(ACTIVE_CH6_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH6_PIN);
    gpio_mode_setup(ACTIVE_CH7_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, ACTIVE_CH7_PIN);

    gpio_mode_setup(MUX_IN_A_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
                    MUX_IN_A_PIN | MUX_IN_B_PIN | MUX_IN_C_PIN);
}

void res_gpio_write_pin(uint32_t port, uint16_t pin, bool value) {
    (value == true) ? gpio_set(port, pin) : gpio_clear(port, pin);
}
