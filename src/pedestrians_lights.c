#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "pedestrians_lights.h"

/* Declararea semafoarelor RTOS pentru butoane */
K_SEM_DEFINE(ped_ns_sem, 0, 1);
K_SEM_DEFINE(ped_ew_sem, 0, 1);

static const struct gpio_dt_spec ped_ns_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ped_ns_red), gpios);
static const struct gpio_dt_spec ped_ns_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ped_ns_grn), gpios);
static const struct gpio_dt_spec ped_ew_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ped_ew_red), gpios);
static const struct gpio_dt_spec ped_ew_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ped_ew_grn), gpios);

static const struct gpio_dt_spec btn_ns = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ns), gpios);
static const struct gpio_dt_spec btn_ew = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ew), gpios);
static struct gpio_callback btn_ns_cb_data;
static struct gpio_callback btn_ew_cb_data;

/* Functiile care detecteaza apasarea butoanelor */
void button_ns_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_sem_give(&ped_ns_sem);
}

void button_ew_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_sem_give(&ped_ew_sem);
}

void init_traffic_pedestrians(void) {
    gpio_pin_configure_dt(&ped_ns_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ped_ns_grn, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ped_ew_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ped_ew_grn, GPIO_OUTPUT_INACTIVE);

    gpio_pin_configure_dt(&btn_ns, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&btn_ns, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_ns_cb_data, button_ns_pressed, BIT(btn_ns.pin));
    gpio_add_callback(btn_ns.port, &btn_ns_cb_data);

    gpio_pin_configure_dt(&btn_ew, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&btn_ew, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_ew_cb_data, button_ew_pressed, BIT(btn_ew.pin));
    gpio_add_callback(btn_ew.port, &btn_ew_cb_data);
}

void set_ped_ns_green(bool green) {
    gpio_pin_set_dt(&ped_ns_red, !green);
    gpio_pin_set_dt(&ped_ns_grn, green);
}

void set_ped_ew_green(bool green) {
    gpio_pin_set_dt(&ped_ew_red, !green);
    gpio_pin_set_dt(&ped_ew_grn, green);
}

void force_ped_red(void) {
    set_ped_ns_green(false);
    set_ped_ew_green(false);
}

/* NOU: Functiile de clipire pentru pietoni */
void blink_ped_ns_green(void) {
    gpio_pin_set_dt(&ped_ns_red, 0); /* Tine rosul stins */
    for (int i = 0; i < 6; i++) {
        gpio_pin_set_dt(&ped_ns_grn, 0);
        k_msleep(250);
        gpio_pin_set_dt(&ped_ns_grn, 1);
        k_msleep(250);
    }
    gpio_pin_set_dt(&ped_ns_grn, 0);
    gpio_pin_set_dt(&ped_ns_red, 1);
}

void blink_ped_ew_green(void) {
    gpio_pin_set_dt(&ped_ew_red, 0); 
    for (int i = 0; i < 6; i++) {
        gpio_pin_set_dt(&ped_ew_grn, 0);
        k_msleep(250);
        gpio_pin_set_dt(&ped_ew_grn, 1);
        k_msleep(250);
    }
    gpio_pin_set_dt(&ped_ew_grn, 0);
    gpio_pin_set_dt(&ped_ew_red, 1);
}