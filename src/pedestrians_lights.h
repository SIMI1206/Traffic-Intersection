#ifndef PEDESTRIANS_LIGHTS_H
#define PEDESTRIANS_LIGHTS_H

/* Semafoare RTOS (mecanismul de comunicare) limitate la max 1 cerere */
extern struct k_sem ped_ns_sem;
extern struct k_sem ped_ew_sem;

/* Functii expuse */
void init_traffic_pedestrians(void);
void set_ped_ns_green(bool is_green);
void set_ped_ew_green(bool is_green);

void blink_ped_ns_green(void);
void blink_ped_ew_green(void);

void force_ped_red(void);

#endif 