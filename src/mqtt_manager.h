#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <stdbool.h> 

void init_mqtt(void);

int mqtt_publish_to_topic(const char *topic, const char *payload);
void mqtt_reconnect(void);

extern bool is_mqtt_connected;
#endif