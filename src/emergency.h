#ifndef EMERGENCY_H
#define EMERGENCY_H

typedef enum {
    EMERG_ALL_RED, 
    EMERG_NS,      
    EMERG_EW       
} emergency_type_t;

void init_emergency(void);
void trigger_emergency(emergency_type_t type);

#endif