#ifndef TRAFFIC_AUTO_H
#define TRAFFIC_AUTO_H

void init_traffic_auto(void);

extern const k_tid_t car_traffic_id; /* Expunem ID-ul thread-ului pentru a-l putea ingheta */
void force_auto_red(void);           /* Functie de urgenta */


#endif 