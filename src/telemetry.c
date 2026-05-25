#include <zephyr/kernel.h>
#include <stdio.h>
#include "telemetry.h"
#include "cars_lights_control.h"
#include "pedestrians_lights.h"
#include "mqtt_manager.h" // Ca sa avem acces la mqtt_publish_to_topic

/* Thread-ul care trimite date la fiecare 500ms */
void telemetry_thread_fn(void *arg1, void *arg2, void *arg3) {
    char payload[128];
    
    while (1) {
        k_msleep(500);

        /* PROTECTIA: Daca nu suntem conectati la cloud, ne oprim aici si incercam din nou peste 500ms */
        if (!is_mqtt_connected) {
            continue; 
        }

        /* Daca avem internet, formatam si trimitem datele */
        snprintf(payload, sizeof(payload),
            "A_NS:%c,A_EW:%c,P_NS:%c,P_EW:%c,REQ_NS:%d,REQ_EW:%d",
            get_auto_ns_color(), get_auto_ew_color(),
            get_ped_ns_color(), get_ped_ew_color(),
            get_ns_ped_request() ? 1 : 0,
            get_ew_ped_request() ? 1 : 0
        );

        mqtt_publish_to_topic("simi/intersectie/live", payload);
    }
}

/* K_THREAD_DEFINE inregistreaza si porneste thread-ul automat la boot */
K_THREAD_DEFINE(telemetry_tid, 4096, telemetry_thread_fn, NULL, NULL, NULL, 7, 0, 0);