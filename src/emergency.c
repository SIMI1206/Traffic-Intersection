#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "emergency.h"
#include "cars_lights_control.h"
#include "pedestrians_lights.h"

LOG_MODULE_REGISTER(emergency, LOG_LEVEL_INF);

K_SEM_DEFINE(emergency_sem, 0, 1);

static volatile emergency_type_t current_emerg_type = EMERG_ALL_RED;
static volatile bool emergency_active = false;

static volatile bool has_pending_emergency = false;
static volatile emergency_type_t pending_emerg_type;

void trigger_emergency(emergency_type_t type) {
    if (emergency_active) {
        LOG_WRN("O urgență este deja activă! Programăm direcția nouă pentru mai târziu.");
        has_pending_emergency = true;
        pending_emerg_type = type;
    } else {
        emergency_active = true;
        current_emerg_type = type;
        k_sem_give(&emergency_sem);
    }
}

void emergency_thread_fn(void *arg1, void *arg2, void *arg3) {
    while (1) {
        k_sem_take(&emergency_sem, K_FOREVER);

        stop_car_traffic_thread();
        
        while (1) {
            force_ped_red();

            if (current_emerg_type == EMERG_NS) {
                LOG_INF(">>> VERDE FORȚAT PE NORD-SUD (10 SECUNDE) <<<");
                force_cars_ns_green_ew_red();
            } 
            else if (current_emerg_type == EMERG_EW) {
                LOG_INF(">>> VERDE FORȚAT PE EST-VEST (10 SECUNDE) <<<");
                force_cars_ew_green_ns_red();
            } 
            else {
                force_auto_red();
            }

            k_msleep(10000);

            if (has_pending_emergency) {
                LOG_INF("Trecem direct la urgenta din așteptare!");
                current_emerg_type = pending_emerg_type; 
                has_pending_emergency = false;           
            } else {
                break;
            }
        }

        k_sem_reset(&emergency_sem);
        

        emergency_active = false;

        LOG_INF("Toate urgențele s-au încheiat. Reluăm ciclul normal.");
        restart_car_traffic_thread();
    }
}

K_THREAD_DEFINE(emergency_id, 1024, emergency_thread_fn, NULL, NULL, NULL, 1, 0, 0);

void init_emergency(void) {

}