#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "cars_lights_control.h"
#include "pedestrians_lights.h"
#include "hazard.h"

#define MIN_GREEN_TIME   5000
#define MAX_GREEN_TIME   10000
#define CLEARANCE_TIME   2000
#define YELLOW_TIME      2000
#define RED_RED_TIME     1000


static const struct gpio_dt_spec ns_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_red), gpios);
static const struct gpio_dt_spec ns_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_yel), gpios);
static const struct gpio_dt_spec ns_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_grn), gpios);
static const struct gpio_dt_spec ew_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_red), gpios);
static const struct gpio_dt_spec ew_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_yel), gpios);
static const struct gpio_dt_spec ew_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_grn), gpios);


static K_THREAD_STACK_DEFINE(car_traffic_stack, 1024);
static struct k_thread car_traffic_thread_data;
static k_tid_t car_traffic_tid = NULL;


void init_traffic_auto(void) {
    gpio_pin_configure_dt(&ns_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_grn, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_grn, GPIO_OUTPUT_INACTIVE);
    
    restart_car_traffic_thread();
}

void restart_car_traffic_thread(void) {
    if (car_traffic_tid != NULL) {
        k_thread_abort(car_traffic_tid);
    }
    car_traffic_tid = k_thread_create(&car_traffic_thread_data, car_traffic_stack,
                                      K_THREAD_STACK_SIZEOF(car_traffic_stack),
                                      car_traffic_thread_fn, NULL, NULL, NULL,
                                      5, 0, K_NO_WAIT);
}

void car_traffic_thread_fn(void *arg1, void *arg2, void *arg3) {
    force_auto_red();
    if (!hazard_mode_active) {
        force_ped_red();
        k_msleep(RED_RED_TIME);
    }

    while (1) {
        /* ==== MOD AVARIE (Galben Intermitent) ==== */
        if (hazard_mode_active) {
            force_ped_off(); 
            gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ew_red, 0);
            gpio_pin_set_dt(&ns_yel, 1); gpio_pin_set_dt(&ew_yel, 1);
            k_msleep(500);
            gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ew_yel, 0);
            k_msleep(500);
            continue; 
        }

        /* ==== FAZA 1: NORD - SUD ==== */
        /* Am sters reset-ul de aici ca sa nu pierdem apasarile legitime */
        gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ns_grn, 1);

        bool ns_crossed = false;
        if (get_ns_ped_request()) {
            set_ped_ns_green(true); 
            ns_crossed = true;
        }

        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ew_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));

        if (ns_crossed) {
            blink_ped_ns_green();
            clear_ns_ped_request();
            /* MUTAT AICI: Dupa ce trec N-S, stergem orice buton au mai apasat ei din greseala in timp ce mergeau */
            k_sem_reset(&ped_ns_sem); 
            update_ns_cooldown();
            k_msleep(CLEARANCE_TIME); 
        }

        gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 1);
        k_msleep(YELLOW_TIME);
        gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
        k_msleep(RED_RED_TIME);

        /* ==== FAZA 2: EST - VEST ==== */
        gpio_pin_set_dt(&ew_red, 0); gpio_pin_set_dt(&ew_grn, 1);

        bool ew_crossed = false;
        if (get_ew_ped_request()) {
            set_ped_ew_green(true);
            ew_crossed = true;
        }

        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ns_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));

        if (ew_crossed) {
            blink_ped_ew_green();
            clear_ew_ped_request();
            /* MUTAT AICI: Curatam spam-ul doar dupa ce au trecut */
            k_sem_reset(&ped_ew_sem); 
            update_ew_cooldown();
            k_msleep(CLEARANCE_TIME);
        }

        gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 1);
        k_msleep(YELLOW_TIME);
        gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
        k_msleep(RED_RED_TIME);
    }
}


void stop_car_traffic_thread(void) {
    if (car_traffic_tid != NULL) {
        k_thread_abort(car_traffic_tid);
    }
}

void force_auto_red(void) {
    gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
    gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
}