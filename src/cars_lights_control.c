#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "cars_lights_control.h"
#include "pedestrians_lights.h"

/* --- SETARI DE TIMP DEMO (in milisecunde) --- */
#define MIN_GREEN_TIME   5000  /* Timp verde garantat */
#define MAX_GREEN_TIME   10000 /* Timp verde maxim inainte sa se schimbe singur */
#define CLEARANCE_TIME   2000  /* NOU: Timpul de degajare in care masinile raman pe verde */
#define YELLOW_TIME      2000  /* Timpul pentru galben auto */
#define RED_RED_TIME     1000  /* Secunda de siguranta intre schimbari */

static const struct gpio_dt_spec ns_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_red), gpios);
static const struct gpio_dt_spec ns_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_yel), gpios);
static const struct gpio_dt_spec ns_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_grn), gpios);

static const struct gpio_dt_spec ew_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_red), gpios);
static const struct gpio_dt_spec ew_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_yel), gpios);
static const struct gpio_dt_spec ew_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_grn), gpios);

void init_traffic_auto(void) {
    gpio_pin_configure_dt(&ns_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_grn, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_grn, GPIO_OUTPUT_INACTIVE);
}

void force_auto_red(void) {
    gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
    gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
}

void car_traffic_thread_fn(void *arg1, void *arg2, void *arg3) {
    gpio_pin_set_dt(&ns_red, 1); gpio_pin_set_dt(&ew_red, 1);
    set_ped_ns_green(false); set_ped_ew_green(false);
    k_msleep(RED_RED_TIME);

    while (1) {
        /* ================= FAZA 1: NORD - SUD ================= */
        k_sem_reset(&ped_ew_sem); 

        gpio_pin_set_dt(&ns_red, 0); gpio_pin_set_dt(&ns_grn, 1);
        set_ped_ns_green(true); 
        
        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ew_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));
        
        /* Pietonii N-S clipesc, apoi se fac ROSII */
        blink_ped_ns_green();

        /* TIMP DE DEGAJARE: Pietonii au rosu, dar masinile isi pastreaza verdele! */
        k_msleep(CLEARANCE_TIME);

        /* Trecerea auto: Galben N-S */
        gpio_pin_set_dt(&ns_grn, 0); gpio_pin_set_dt(&ns_yel, 1);
        k_msleep(YELLOW_TIME);

        /* Siguranta (Toate rosii) */
        gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_red, 1);
        k_msleep(RED_RED_TIME);


        /* ================= FAZA 2: EST - VEST ================= */
        k_sem_reset(&ped_ns_sem);

        gpio_pin_set_dt(&ew_red, 0); gpio_pin_set_dt(&ew_grn, 1);
        set_ped_ew_green(true);
        
        k_msleep(MIN_GREEN_TIME);
        k_sem_take(&ped_ns_sem, K_MSEC(MAX_GREEN_TIME - MIN_GREEN_TIME));

        /* Pietonii E-W clipesc, apoi se fac ROSII */
        blink_ped_ew_green();

        /* TIMP DE DEGAJARE: Masinile E-W degajeaza intersectia */
        k_msleep(CLEARANCE_TIME);

        /* Trecerea auto: Galben E-W */
        gpio_pin_set_dt(&ew_grn, 0); gpio_pin_set_dt(&ew_yel, 1);
        k_msleep(YELLOW_TIME);

        /* Siguranta */
        gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_red, 1);
        k_msleep(RED_RED_TIME);
    }
}

K_THREAD_DEFINE(car_traffic_id, 1024, car_traffic_thread_fn, NULL, NULL, NULL, 5, 0, 0);