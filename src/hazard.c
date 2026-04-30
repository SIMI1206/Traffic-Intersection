#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "hazard.h"
#include "cars_lights_control.h"

/* Aici locuieste fizic variabila, celelalte o doar citesc */
bool hazard_mode_active = false;

/* Ne conectam la butoanele fizice pentru a le putea citi starea */
static const struct gpio_dt_spec btn_ns = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ns), gpios);
static const struct gpio_dt_spec btn_ew = GPIO_DT_SPEC_GET(DT_ALIAS(sw_ped_ew), gpios);

/* --- LOGICA PENTRU MODUL DE AVARIE (Tine apasat 3 sec) --- */
void hazard_monitor_thread_fn(void *arg1, void *arg2, void *arg3) {
    int hold_ticks = 0;
    while(1) {
        /* Citim starea pinilor (1 inseamna apasat, datorita ACTIVE_LOW din overlay) */
        if (gpio_pin_get_dt(&btn_ns) == 1 && gpio_pin_get_dt(&btn_ew) == 1) {
            hold_ticks++;
            if (hold_ticks >= 30) { /* 30 x 100ms = 3 secunde */
                hazard_mode_active = !hazard_mode_active; /* Schimbam starea */
                restart_car_traffic_thread(); /* Dam reset la intersectie */
                k_msleep(3000); /* Pauza sa iei degetele de pe butoane */
                hold_ticks = 0;
            }
        } else {
            hold_ticks = 0; /* S-a luat degetul, resetam numaratoarea */
        }
        k_msleep(100);
    }
}

/* Thread-ul suprem de monitorizare avarie - Prioritatea 2 */
K_THREAD_DEFINE(hazard_monitor_id, 1024, hazard_monitor_thread_fn, NULL, NULL, NULL, 2, 0, 0);