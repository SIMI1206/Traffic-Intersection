#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

#include "cars_lights_control.h"
#include "pedestrians_lights.h"
#include "emergency.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

int main(void) {
    /* 1. Pornim cablul USB pentru Log-uri */
    usb_enable(NULL);
    
    /* 2. Asteptam 3 secunde ca Windows-ul sa recunoasca portul (COM10) */
    k_msleep(3000); 


    init_wifi();
    init_traffic_auto();
    init_traffic_pedestrians();
    init_emergency();

    return 0;
}