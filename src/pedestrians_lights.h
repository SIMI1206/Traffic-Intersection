#ifndef PEDESTRIANS_LIGHTS_H
#define PEDESTRIANS_LIGHTS_H

// pedestrian request semaphores
extern struct k_sem ped_ns_sem;
extern struct k_sem ped_ew_sem;

void init_traffic_pedestrians(void);

// pedestrian light control functions
void set_ped_ns_green(bool is_green);
void set_ped_ew_green(bool is_green);

//blink after pedestrian green
void blink_ped_ns_green(void);
void blink_ped_ew_green(void);

//force red for all pedestrians
void force_ped_red(void);

//check cooldown timers
bool is_ns_cooldown_passed(void);
bool is_ew_cooldown_passed(void);

#endif 