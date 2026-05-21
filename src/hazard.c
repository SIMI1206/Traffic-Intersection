#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "hazard.h"
#include "cars_lights_control.h"

bool hazard_mode_active = false;

static const struct gpio_dt_spec btn_ns = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ns), gpios);
static const struct gpio_dt_spec btn_ew = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ew), gpios);


void hazard_monitor_thread_fn(void *arg1, void *arg2, void *arg3) {
    int hold_ticks = 0;
    while(1) {
        if (gpio_pin_get_dt(&btn_ns) == 1 && gpio_pin_get_dt(&btn_ew) == 1) {
            hold_ticks++;
            if (hold_ticks >= 30) { 
                hazard_mode_active = !hazard_mode_active; 
                restart_car_traffic_thread(); 
                k_msleep(3000); 
                hold_ticks = 0;
            }
        } else {
            hold_ticks = 0; 
        }
        k_msleep(100);
    }
}

K_THREAD_DEFINE(hazard_monitor_id, 1024, hazard_monitor_thread_fn, NULL, NULL, NULL, 2, 0, 0);