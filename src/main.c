#include <zephyr/kernel.h>
#include "cars_lights_control.h" 
#include "pedestrians_lights.h"

int main(void) {
    init_traffic_auto();
    init_traffic_pedestrians();
    init_emergency();

    return 0;
}