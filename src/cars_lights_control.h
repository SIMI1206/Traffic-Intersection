#ifndef CARS_LIGHTS_CONTROL_H
#define CARS_LIGHTS_CONTROL_H

#include <zephyr/kernel.h>

void init_traffic_auto(void); // traffic pin configuration + cars thread start
void force_auto_red(void); // all car lights red, used for reset and hazard mode
void restart_car_traffic_thread(void); // start/restart the thread that controls the traffic lights
void stop_car_traffic_thread(void); // thread stop

void force_cars_ns_green_ew_red(void);
void force_cars_ew_green_ns_red(void);

#endif