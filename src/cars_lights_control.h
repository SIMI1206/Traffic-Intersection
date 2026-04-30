#ifndef CARS_LIGHTS_CONTROL_H
#define CARS_LIGHTS_CONTROL_H

#include <zephyr/kernel.h>

void init_traffic_auto(void);
void force_auto_red(void);
void restart_car_traffic_thread(void);
void stop_car_traffic_thread(void); /* Functia noua, sigura */

#endif