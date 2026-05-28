#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include "cars_lights_control.h"
#include "pedestrians_lights.h"
#include "hazard.h"

LOG_MODULE_REGISTER(cars_lights_control, LOG_LEVEL_INF);

#define MIN_GREEN_TIME   5000 // cars minim green time
#define MAX_GREEN_TIME   10000 // cars max green time
#define CLEARANCE_TIME   2000 //cars clearance time
#define YELLOW_TIME      2000 // time for yellow light
#define FULL_RED_TIME     1000 // time for both directions red 

// Cars traffic light GPIO configuration
static const struct gpio_dt_spec ns_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_red), gpios);
static const struct gpio_dt_spec ns_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_yel), gpios);
static const struct gpio_dt_spec ns_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_grn), gpios);
static const struct gpio_dt_spec ew_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_red), gpios);
static const struct gpio_dt_spec ew_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_yel), gpios);
static const struct gpio_dt_spec ew_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_grn), gpios);

// Thread data for car traffic control
static K_THREAD_STACK_DEFINE(car_traffic_stack, 1024);
static struct k_thread car_traffic_thread_data;
static k_tid_t car_traffic_tid = NULL;


void init_traffic_auto(void) {
    gpio_pin_configure_dt(&ns_red, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ns_yel, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ns_grn, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    
    gpio_pin_configure_dt(&ew_red, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ew_yel, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    gpio_pin_configure_dt(&ew_grn, GPIO_OUTPUT_INACTIVE | GPIO_INPUT);
    
    restart_car_traffic_thread();
}


// cars thread function - Priority 5
void car_traffic_thread_fn(void *arg1, void *arg2, void *arg3) {
    // safety red light on at start and after hazard mode
    force_auto_red();
    if (!hazard_mode_active) {
        force_ped_red();
        k_msleep(FULL_RED_TIME);
    }

    while (1) {
        if (hazard_mode_active) {
            force_ped_off(); 
            gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ew_red, 0);
            gpio_pin_set_dt(&ns_yel, 1); gpio_pin_set_dt(&ew_yel, 1);
            k_msleep(500);
            gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ew_yel, 0);
            k_msleep(500);
            continue; 
        }

        gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ns_grn, 1);

        bool ns_crossed = false;
        if (get_ns_ped_request() && is_ns_cooldown_passed()) {
            set_ped_ns_green(true); 
            ns_crossed = true;
        }

        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ew_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));

        if (ns_crossed) {
            extern uint32_t timp_apasare_buton;
            uint32_t timp_procesare = k_cycle_get_32();
            uint32_t latenta_cicluri = timp_procesare - timp_apasare_buton;
            uint32_t latenta_us = k_cyc_to_us_ceil32(latenta_cicluri);
            LOG_INF("Latență RTOS: %u microsecunde de la apăsare la procesare!", latenta_us);

            blink_ped_ns_green();
            blink_ped_ns_green();
            clear_ns_ped_request();
            k_sem_reset(&ped_ns_sem); 
            update_ns_cooldown();
            k_msleep(CLEARANCE_TIME); 
        }

        gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 1);
        k_msleep(YELLOW_TIME);
        gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
        k_msleep(FULL_RED_TIME
    );

        gpio_pin_set_dt(&ew_red, 0); gpio_pin_set_dt(&ew_grn, 1);

        bool ew_crossed = false;
        if (get_ew_ped_request() && is_ew_cooldown_passed()) {
            set_ped_ew_green(true);
            ew_crossed = true;
        }

        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ns_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));

        if (ew_crossed) {
            blink_ped_ew_green();
            clear_ew_ped_request();
            k_sem_reset(&ped_ew_sem); 
            update_ew_cooldown();
            k_msleep(CLEARANCE_TIME);
        }

        gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 1);
        k_msleep(YELLOW_TIME);
        gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
        k_msleep(FULL_RED_TIME
    );
    }
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


void stop_car_traffic_thread(void) {
    if (car_traffic_tid != NULL) {
        k_thread_abort(car_traffic_tid);
    }
}

void force_auto_red(void) {

    gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 0);
    gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 0);
    

    k_msleep(50); 
    gpio_pin_set_dt(&ns_red, 1);
    gpio_pin_set_dt(&ew_red, 1);
}

void force_cars_ns_green_ew_red(void) {

    gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_grn, 1);
    gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
}

void force_cars_ew_green_ns_red(void) {
    /* Est-Vest are Verde, Nord-Sud are Rosu */
    gpio_pin_set_dt(&ew_red, 0); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_grn, 1);
    gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
}

char get_auto_ns_color(void) {
    if (gpio_pin_get_dt(&ns_grn) == 1) return 'G';
    if (gpio_pin_get_dt(&ns_yel) == 1) return 'Y';
    if (gpio_pin_get_dt(&ns_red) == 1) return 'R';
    return 'O'; 
}

char get_auto_ew_color(void) {
    if (gpio_pin_get_dt(&ew_grn) == 1) return 'G';
    if (gpio_pin_get_dt(&ew_yel) == 1) return 'Y';
    if (gpio_pin_get_dt(&ew_red) == 1) return 'R';
    return 'O';
}