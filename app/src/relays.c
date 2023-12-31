#include <zephyr/drivers/gpio.h>
#include <devicetree_generated.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <string.h>

#include "inc/relays.h"

static const struct gpio_dt_spec relay_0 = GPIO_DT_SPEC_GET_OR(RELAY_0, gpios, {0});
static const struct gpio_dt_spec relay_1 = GPIO_DT_SPEC_GET_OR(RELAY_1, gpios,{0});
static const struct gpio_dt_spec relay_2 = GPIO_DT_SPEC_GET_OR(RELAY_2, gpios, {0});
static const struct gpio_dt_spec relay_3 = GPIO_DT_SPEC_GET_OR(RELAY_3, gpios,{0});
static const struct gpio_dt_spec relay_4 = GPIO_DT_SPEC_GET_OR(RELAY_4, gpios, {0});
static const struct gpio_dt_spec relay_5 = GPIO_DT_SPEC_GET_OR(RELAY_5, gpios,{0});
static const struct gpio_dt_spec relay_6 = GPIO_DT_SPEC_GET_OR(RELAY_6, gpios, {0});
static const struct gpio_dt_spec relay_7 = GPIO_DT_SPEC_GET_OR(RELAY_7, gpios,{0});

/* Function prototypes */
uint8_t simple_strtou8(const char *str);
void set_relays(uint8_t value);
void set_relay_by_name(const char *name, bool state);
bool get_relay_by_name(const char *name);
const char* get_relay_name(int relay_number);

void set_relays(uint8_t value) {
    gpio_pin_set(relay_0.port, relay_0.pin, value & 0x01);
    gpio_pin_set(relay_1.port, relay_1.pin, (value >> 1) & 0x01);
    gpio_pin_set(relay_2.port, relay_2.pin, (value >> 2) & 0x01);
    gpio_pin_set(relay_3.port, relay_3.pin, (value >> 3) & 0x01);
    gpio_pin_set(relay_4.port, relay_4.pin, (value >> 4) & 0x01);
    gpio_pin_set(relay_5.port, relay_5.pin, (value >> 5) & 0x01);
    gpio_pin_set(relay_6.port, relay_6.pin, (value >> 6) & 0x01);
    gpio_pin_set(relay_7.port, relay_7.pin, (value >> 7) & 0x01);
}

void set_relay_by_name(const char *name, bool state) {
    if (strcmp(name, "relay_0") == 0) {
        gpio_pin_set(relay_0.port, relay_0.pin, state);
    } else if (strcmp(name, "relay_1") == 0) {
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
    }
}

bool get_relay_by_name(const char *name) {
    bool state = false;
    if (strcmp(name, "relay_0") == 0) {
        state = gpio_pin_get(relay_0.port, relay_0.pin);
    } else if (strcmp(name, "relay_1") == 0) {
        state = gpio_pin_get(relay_1.port, relay_1.pin);
    } else if (strcmp(name, "relay_2") == 0) {
        state = gpio_pin_get(relay_2.port, relay_2.pin);
    } else if (strcmp(name, "relay_3") == 0) {
        state = gpio_pin_get(relay_3.port, relay_3.pin);
    } else if (strcmp(name, "relay_4") == 0) {
        state = gpio_pin_get(relay_4.port, relay_4.pin);
    } else if (strcmp(name, "relay_5") == 0) {
        state = gpio_pin_get(relay_5.port, relay_5.pin);
    } else if (strcmp(name, "relay_6") == 0) {
        state = gpio_pin_get(relay_6.port, relay_6.pin);
    } else if (strcmp(name, "relay_7") == 0) {
        state = gpio_pin_get(relay_7.port, relay_7.pin);
    }
    return state;
}


const char* get_relay_name(int relay_number) {
    switch (relay_number) {
        case 0: return "relay_0";
        case 1: return "relay_1";
        case 2: return "relay_2";
        case 3: return "relay_3";
        case 4: return "relay_4";
        case 5: return "relay_5";
        case 6: return "relay_6";
        case 7: return "relay_7";
        default: return "Unknown";
    }
}
static int cmd_relays(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 1) {
        shell_print(shell, "Usage: relays --set-bytes <value[0..255]> "
                           "| --get-relay <name> "
                           "| --set-relay <name> <state[1/0]> "
                           "| --list-relays");
        return 0;
    }
    if (strcmp(argv[1], "--set-bytes") == 0) {
        if (argc == 3) {
            uint8_t value = simple_strtou8(argv[2]);
            set_relays(value);
        } else {
            shell_print(shell, "Invalid number of arguments for --bin-value");
        }
    } else if (strcmp(argv[1], "--set-relay") == 0) {
        if (argc == 4) {
            const char *name = argv[2];
            bool state_val = simple_strtou8(argv[3]) != 0; // Convert to boolean
            set_relay_by_name(name, state_val);
        } else {
            shell_print(shell, "Invalid number of arguments for -w");
        }
    } else if (strcmp(argv[1], "--get-relay") == 0) {
        if (argc == 3) {
            const char *name = argv[2];
            bool state = get_relay_by_name(name);
            shell_print(shell, "%s state: %d", name, state);
        } else {
            shell_print(shell, "Invalid number of arguments for -w");
        }
    } else if (strcmp(argv[1], "--list-relays") == 0) {
        if (argc == 2) {
            for (int i = 0; i <= 7; i++) {
                shell_print(shell, "%s", get_relay_name(i));
            }
        } else {
            shell_print(shell, "Invalid number of arguments for --get-relay-names");
        }
    } else {
        shell_print(shell, "Invalid command or number of arguments.");
    }
    return 0;
}
uint8_t simple_strtou8(const char *str) {
    uint8_t result = 0;
    while (*str) {
        if (*str < '0' || *str > '9') {
            break;
        }
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

void relay_init() {
    // Initialize GPIO pins as outputs
    gpio_pin_configure_dt(&relay_0, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_1, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_2, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_3, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_4, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_5, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_6, GPIO_OUTPUT);
    gpio_pin_configure_dt(&relay_7, GPIO_OUTPUT);

    // Set all relays to OFF
    gpio_pin_set(relay_0.port, relay_0.pin, 0);
    gpio_pin_set(relay_1.port, relay_1.pin, 0);
    gpio_pin_set(relay_2.port, relay_2.pin, 0);
    gpio_pin_set(relay_3.port, relay_3.pin, 0);
    gpio_pin_set(relay_4.port, relay_4.pin, 0);
    gpio_pin_set(relay_5.port, relay_5.pin, 0);
    gpio_pin_set(relay_6.port, relay_6.pin, 0);
    gpio_pin_set(relay_7.port, relay_7.pin, 0);
    printk("All relays configured and set to OFF!\n");
}

SHELL_CMD_REGISTER(relays, NULL, "control relays of pico. Execute without arguments to get more info", cmd_relays);
