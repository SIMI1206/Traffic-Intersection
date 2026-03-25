#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define GREEN_TIME   5000  
#define YELLOW_TIME  2000 
#define RED_RED_TIME 1000  


static const struct gpio_dt_spec ns_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_red), gpios);
static const struct gpio_dt_spec ns_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_yel), gpios);
static const struct gpio_dt_spec ns_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ns_grn), gpios);

static const struct gpio_dt_spec ew_red = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_red), gpios);
static const struct gpio_dt_spec ew_yel = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_yel), gpios);
static const struct gpio_dt_spec ew_grn = GPIO_DT_SPEC_GET(DT_ALIAS(led_ew_grn), gpios);

#define STACK_SIZE 1024
#define THREAD_PRIORITY 5

void car_traffic_thread_fn(void *arg1, void *arg2, void *arg3) {
   
    gpio_pin_set_dt(&ns_red, 1); gpio_pin_set_dt(&ns_yel, 0); gpio_pin_set_dt(&ns_grn, 0);
    gpio_pin_set_dt(&ew_red, 1); gpio_pin_set_dt(&ew_yel, 0); gpio_pin_set_dt(&ew_grn, 0);
    k_msleep(RED_RED_TIME);

    while (1) {

        gpio_pin_set_dt(&ns_red, 0); 
        gpio_pin_set_dt(&ns_grn, 1);
        k_msleep(GREEN_TIME);

        gpio_pin_set_dt(&ns_grn, 0); 
        gpio_pin_set_dt(&ns_yel, 1);
        k_msleep(YELLOW_TIME);

        gpio_pin_set_dt(&ns_yel, 0); 
        gpio_pin_set_dt(&ns_red, 1);
        k_msleep(RED_RED_TIME);

        gpio_pin_set_dt(&ew_red, 0); 
        gpio_pin_set_dt(&ew_grn, 1);
        k_msleep(GREEN_TIME);

        gpio_pin_set_dt(&ew_grn, 0); 
        gpio_pin_set_dt(&ew_yel, 1);
        k_msleep(YELLOW_TIME);

        gpio_pin_set_dt(&ew_yel, 0); 
        gpio_pin_set_dt(&ew_red, 1);
        k_msleep(RED_RED_TIME);
    }
}


K_THREAD_DEFINE(car_traffic_id, STACK_SIZE, car_traffic_thread_fn, NULL, NULL, NULL, THREAD_PRIORITY, 0, 0);

int main(void) {

    gpio_pin_configure_dt(&ns_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ns_grn, GPIO_OUTPUT_INACTIVE);
    
    gpio_pin_configure_dt(&ew_red, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_yel, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&ew_grn, GPIO_OUTPUT_INACTIVE);
    
    return 0;
}