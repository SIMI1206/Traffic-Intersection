#include <zephyr/kernel.h>

#include <zephyr/usb/usb_device.h>
#include <zephyr/logging/log.h>

#include "cars_lights_control.h"
#include "pedestrians_lights.h"
#include "emergency.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"

int main(void) {
    usb_enable(NULL);
    
    k_msleep(3000); 

    init_wifi();
    init_traffic_auto();
    init_traffic_pedestrians();
    init_emergency();

    return 0;
}