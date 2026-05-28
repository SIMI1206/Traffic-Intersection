#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "pedestrians_lights.h"
#include "cars_lights_control.h"
#include "hazard.h"

#define COOLDOWN_TIME 15000 

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

bool ns_ped_request = false;
bool ew_ped_request = false;
int64_t last_ns_time = -15000; 
int64_t last_ew_time = -15000;

uint32_t timp_apasare_buton = 0;

void button_ns_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    timp_apasare_buton = k_cycle_get_32();
    if (!hazard_mode_active) {
        ns_ped_request = true; 
        if (is_ns_cooldown_passed()) {
            k_sem_give(&ped_ns_sem); 
        }
    }
}

void button_ew_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (!hazard_mode_active) {
        ew_ped_request = true;
        if (is_ew_cooldown_passed()) {
            k_sem_give(&ped_ew_sem);
        }
    }
}

bool is_ns_cooldown_passed(void) {
    return (k_uptime_get() - last_ns_time >= COOLDOWN_TIME);
}

bool is_ew_cooldown_passed(void) {
    return (k_uptime_get() - last_ew_time >= COOLDOWN_TIME);
}


void init_traffic_pedestrians(void) {
    gpio_pin_configure_dt(&ped_ns_red, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ped_ns_grn, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ped_ew_red, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ped_ew_grn, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);

    gpio_pin_configure_dt(&btn_ns, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&btn_ns, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_ns_cb_data, button_ns_pressed, BIT(btn_ns.pin));
    gpio_add_callback(btn_ns.port, &btn_ns_cb_data);

    gpio_pin_configure_dt(&btn_ew, GPIO_INPUT | GPIO_PULL_UP);
    gpio_pin_interrupt_configure_dt(&btn_ew, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&btn_ew_cb_data, button_ew_pressed, BIT(btn_ew.pin));
    gpio_add_callback(btn_ew.port, &btn_ew_cb_data);
}

bool get_ns_ped_request(void) { return ns_ped_request; }
bool get_ew_ped_request(void) { return ew_ped_request; }
void clear_ns_ped_request(void) { ns_ped_request = false; }
void clear_ew_ped_request(void) { ew_ped_request = false; }
void update_ns_cooldown(void) { last_ns_time = k_uptime_get(); }
void update_ew_cooldown(void) { last_ew_time = k_uptime_get(); }

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
void force_ped_off(void) {
    gpio_pin_set_dt(&ped_ns_red, 0); gpio_pin_set_dt(&ped_ns_grn, 0);
    gpio_pin_set_dt(&ped_ew_red, 0); gpio_pin_set_dt(&ped_ew_grn, 0);
}
void blink_ped_ns_green(void) {
    gpio_pin_set_dt(&ped_ns_red, 0); 
    for (int i = 0; i < 6; i++) {
        gpio_pin_set_dt(&ped_ns_grn, 0); k_msleep(250);
        gpio_pin_set_dt(&ped_ns_grn, 1); k_msleep(250);
    }
    gpio_pin_set_dt(&ped_ns_grn, 0); gpio_pin_set_dt(&ped_ns_red, 1);
}
void blink_ped_ew_green(void) {
    gpio_pin_set_dt(&ped_ew_red, 0); 
    for (int i = 0; i < 6; i++) {
        gpio_pin_set_dt(&ped_ew_grn, 0); k_msleep(250);
        gpio_pin_set_dt(&ped_ew_grn, 1); k_msleep(250);
    }
    gpio_pin_set_dt(&ped_ew_grn, 0); gpio_pin_set_dt(&ped_ew_red, 1);
}



char get_ped_ns_color(void) {
    if (gpio_pin_get_dt(&ped_ns_grn) == 1) return 'G'; 
    if (gpio_pin_get_dt(&ped_ns_red) == 1) return 'R';
    return 'O';
}

char get_ped_ew_color(void) {
    if (gpio_pin_get_dt(&ped_ew_grn) == 1) return 'G';
    if (gpio_pin_get_dt(&ped_ew_red) == 1) return 'R';
    return 'O';
}

void clear_pedestrian_requests(void) {

    ns_ped_request = false;  
    ew_ped_request = false;  
    
}