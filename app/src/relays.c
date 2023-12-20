#include <sys/cdefs.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/gpio.h>
#include <devicetree_generated.h>
#include <zephyr/sys/printk.h>
#include <string.h>

#include "inc/relays.h"



static const struct gpio_dt_spec relay_1 = GPIO_DT_SPEC_GET_OR(RELAY_1, gpios, {0});
static const struct gpio_dt_spec relay_2 = GPIO_DT_SPEC_GET_OR(RELAY_2, gpios,{0});
static const struct gpio_dt_spec relay_3 = GPIO_DT_SPEC_GET_OR(RELAY_3, gpios, {0});
static const struct gpio_dt_spec relay_4 = GPIO_DT_SPEC_GET_OR(RELAY_4, gpios,{0});
static const struct gpio_dt_spec relay_5 = GPIO_DT_SPEC_GET_OR(RELAY_5, gpios, {0});
static const struct gpio_dt_spec relay_6 = GPIO_DT_SPEC_GET_OR(RELAY_6, gpios,{0});
static const struct gpio_dt_spec relay_7 = GPIO_DT_SPEC_GET_OR(RELAY_7, gpios, {0});
static const struct gpio_dt_spec relay_8 = GPIO_DT_SPEC_GET_OR(RELAY_8, gpios,{0});

void relay_init() {
    // Initialize GPIO pins as outputs
    gpio_pin_configure_dt(&relay_1, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_2, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_3, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_4, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_5, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_6, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_7, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_8, GPIO_OUTPUT);
    printk("All relays configured!\n");
}

void control_relays(uint8_t value) {
    gpio_pin_set(relay_1.port, relay_1.pin, value & 0x01);
    gpio_pin_set(relay_2.port, relay_2.pin, (value >> 1) & 0x01);
    gpio_pin_set(relay_3.port, relay_3.pin, (value >> 2) & 0x01);
    gpio_pin_set(relay_4.port, relay_4.pin, (value >> 3) & 0x01);
    gpio_pin_set(relay_5.port, relay_5.pin, (value >> 4) & 0x01);
    gpio_pin_set(relay_6.port, relay_6.pin, (value >> 5) & 0x01);
    gpio_pin_set(relay_7.port, relay_7.pin, (value >> 6) & 0x01);
    gpio_pin_set(relay_8.port, relay_8.pin, (value >> 7) & 0x01);
}

void control_relay_by_name(const char *name, bool state) {
    if (strcmp(name, "relay_1") == 0) {
        gpio_pin_set(relay_1.port, relay_1.pin, state);
    } else if (strcmp(name, "relay_2") == 0) {
        gpio_pin_set(relay_2.port, relay_2.pin, state);
    } else if (strcmp(name, "relay_3") == 0) {
        gpio_pin_set(relay_3.port, relay_3.pin, state);
    } else if (strcmp(name, "relay_4") == 0) {
        gpio_pin_set(relay_4.port, relay_4.pin, state);
    } else if (strcmp(name, "relay_5") == 0) {
        gpio_pin_set(relay_5.port, relay_5.pin, state);
    } else if (strcmp(name, "relay_6") == 0) {
        gpio_pin_set(relay_6.port, relay_6.pin, state);
    } else if (strcmp(name, "relay_7") == 0) {
        gpio_pin_set(relay_7.port, relay_7.pin, state);
    } else if (strcmp(name, "relay_8") == 0) {
        gpio_pin_set(relay_8.port, relay_8.pin, state);
    }
}

const char* get_relay_name(int relay_number) {
    switch (relay_number) {
        case 1: return "relay_1";
        case 2: return "relay_2";
        case 3: return "relay_3";
        case 4: return "relay_4";
        case 5: return "relay_5";
        case 6: return "relay_6";
        case 7: return "relay_7";
        case 8: return "relay_8";
        default: return "Unknown";
    }
}
