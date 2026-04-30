#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "emergency.h"
#include "cars_lights_control.h"
#include "pedestrians_lights.h"


/* Semafor pentru detectarea semnalului IR */
K_SEM_DEFINE(emergency_sem, 0, 1);

static const struct gpio_dt_spec ir_sensor = GPIO_DT_SPEC_GET(DT_ALIAS(ir_sensor), gpios);
static struct gpio_callback ir_cb_data;

/* Aceasta functie se executa INSTANT cand senzorul IR vede un semnal */
void ir_triggered(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    if (gpio_pin_get_dt(&ir_sensor) > 0) {
        k_sem_give(&emergency_sem);
    }
}

/* Thread-ul de Ambulanta (Prioritate Maxima) */
void emergency_thread_fn(void *arg1, void *arg2, void *arg3) {
    while (1) {
        /* Asteptam in liniste pana cand cineva apasa pe telecomanda */
        k_sem_take(&emergency_sem, K_FOREVER);

        /* 1. INGHETAM traficul normal (suspendam thread-ul) */
        k_thread_suspend(car_traffic_id);

        /* 2. Fortam absolut toate semafoarele pe ROSU */
        force_auto_red();
        force_ped_red();

        /* 3. Asteptam 10 secunde ca sa treaca ambulanta (timp blocat) */
        k_msleep(10000);

        /* 
         * Curatam semaforul, deoarece telecomanda trimite zeci de semnale pe secunda 
         * si nu vrem sa declanseze iar urgenta imediat ce se termina.
         */
        k_sem_reset(&emergency_sem);

        /* 4. DEZGHEȚĂM traficul normal (isi reia executia exact de unde a ramas) */
        k_thread_resume(car_traffic_id);
    }
}

/* Definim thread-ul de urgenta. Prioritate 1 (mai puternic decat 5 de la masini) */
K_THREAD_DEFINE(emergency_id, 1024, emergency_thread_fn, NULL, NULL, NULL, 1, 0, 0);

void init_emergency(void) {
    if (!gpio_is_ready_dt(&ir_sensor)) {
        return;
    }
    gpio_pin_configure_dt(&ir_sensor, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&ir_sensor, GPIO_INT_EDGE_TO_ACTIVE);
    gpio_init_callback(&ir_cb_data, ir_triggered, BIT(ir_sensor.pin));
    gpio_add_callback(ir_sensor.port, &ir_cb_data);
}