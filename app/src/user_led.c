#include "inc/user_led.h"
#include <zephyr/kernel.h>

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   2000

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void user_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
}

void user_led_thread(void) {
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }
}
