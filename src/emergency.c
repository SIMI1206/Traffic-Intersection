#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "emergency.h"
#include "cars_lights_control.h"
#include "pedestrians_lights.h"

K_SEM_DEFINE(emergency_sem, 0, 1);

static const struct gpio_dt_spec ir_sensor = GPIO_DT_SPEC_GET(DT_ALIAS(ir_sensor), gpios);
static struct gpio_callback ir_cb_data;

void ir_triggered(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (gpio_pin_get_dt(&ir_sensor) > 0) {
        k_sem_give(&emergency_sem);
    }
}

void emergency_thread_fn(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_sem_take(&emergency_sem, K_FOREVER);

        /* 1. OPRIM EXECUTIA (Masinile se opresc pe loc, indiferent ce faceau) */
        
        stop_car_traffic_thread();
        

        // FULL RED
        force_auto_red();
        force_ped_red();

        // Emergency Mode Time
        k_msleep(10000);

        /* 4. RESET DE SIGURANTA (Mai raman toate pe rosu 3 

        /* Curatam spam-ul senzorului inainte de repornire */
        k_sem_reset(&emergency_sem);

        /* 5. REPORNIM SISTEMUL (Va intra curat in Faza 1 N-S) */
        restart_car_traffic_thread();
    }
}

K_THREAD_DEFINE(emergency_id, 1024, emergency_thread_fn, NULL, NULL, NULL, 1, 0, 0);

void init_emergency(void) {
    if (!gpio_is_ready_dt(&ir_sensor)) return;
    gpio_pin_configure_dt(&ir_sensor, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&ir_sensor, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&ir_cb_data, ir_triggered, BIT(ir_sensor.pin));
    gpio_add_callback(ir_sensor.port, &ir_cb_data);
}