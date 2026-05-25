#ifndef EMERGENCY_H
#define EMERGENCY_H

/* Tipurile de comenzi pe care le putem primi de pe internet */
typedef enum {
    EMERG_ALL_RED, /* Optiune generala (Toate pe Rosu) */
    EMERG_NS,      /* Prioritate ambulanta Nord-Sud */
    EMERG_EW       /* Prioritate ambulanta Est-Vest */
} emergency_type_t;

void init_emergency(void);
void trigger_emergency(emergency_type_t type);

#endif