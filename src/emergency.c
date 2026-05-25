#include <zephyr/kernel.h>
#include "emergency.h"
#include "cars_lights_control.h"
#include "pedestrians_lights.h"

K_SEM_DEFINE(emergency_sem, 0, 1);

/* Memoram directia ceruta de la distanta */
static volatile emergency_type_t current_emerg_type = EMERG_ALL_RED;

/* Functia publica apelata de mqtt_manager cand primeste textul */
void trigger_emergency(emergency_type_t type) {
    current_emerg_type = type;
    k_sem_give(&emergency_sem);
}

void emergency_thread_fn(void *arg1, void *arg2, void *arg3) {
    while (1) {
        /* Asteptam comanda din cloud */
        k_sem_take(&emergency_sem, K_FOREVER);

        /* Oprim ciclul normal al semafoarelor */
        stop_car_traffic_thread();
        
        /* Regula de aur: Indiferent de ambulanta, pietonii stau pe rosu */
        force_ped_red();

        /* Decidem cine are verde in functie de comanda primita */
        if (current_emerg_type == EMERG_NS) {
            force_cars_ns_green_ew_red();
        } 
        else if (current_emerg_type == EMERG_EW) {
            force_cars_ew_green_ns_red();
        } 
        else {
            force_auto_red();
        }

        /* Tine ambulanta pe verde 10 secunde */
        k_msleep(10000);

        k_sem_reset(&emergency_sem);

        /* Revino la normal */
        restart_car_traffic_thread();
    }
}

K_THREAD_DEFINE(emergency_id, 1024, emergency_thread_fn, NULL, NULL, NULL, 1, 0, 0);

void init_emergency(void) {
    /* Functia este pastrata goala pentru a nu da eroare la apelul din main.c */
    /* Nu mai avem pini fizici de configurat aici */
}