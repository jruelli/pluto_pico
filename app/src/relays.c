/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file relays.c
 * @brief Relay Control Module
 *
 * This module provides a set of functions for controlling and querying the state
 * of relay modules in an embedded system. It includes capabilities to set the state
 * of individual relays or all relays simultaneously, retrieve the current state of
 * any relay, and interact with the relay module through a command-line interface.
 * The functions make use of GPIO (General-Purpose Input/Output) pins to manage
 * the relay states.
 *
 * This module facilitates the integration of relay control into larger systems,
 * allowing for effective management of power and signal control through relays.
 * It is designed to be easy to use and integrate into various embedded systems
 * requiring relay control functionality.
 *
 * Key functionalities include:
 * - Setting the state of individual or multiple relays.
 * - Querying the state of any relay.
 * - Command-line interface for relay control.
 * - Initialization and configuration of relay GPIO pins.
 *
 * The module is a part of a larger system and can be utilized in applications such
 * as home automation, industrial control, and other scenarios where relay control
 * is essential.
 *
 * @author Jannis Ruellmann
 */

#include <zephyr/drivers/gpio.h>
#include <devicetree_generated.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <string.h>

#include "inc/relays.h"
#include "inc/usb_cli.h"

static const struct gpio_dt_spec relay_0 = GPIO_DT_SPEC_GET_OR(RELAY_0, gpios, {0});
static const struct gpio_dt_spec relay_1 = GPIO_DT_SPEC_GET_OR(RELAY_1, gpios,{0});
static const struct gpio_dt_spec relay_2 = GPIO_DT_SPEC_GET_OR(RELAY_2, gpios, {0});
static const struct gpio_dt_spec relay_3 = GPIO_DT_SPEC_GET_OR(RELAY_3, gpios,{0});
static const struct gpio_dt_spec relay_4 = GPIO_DT_SPEC_GET_OR(RELAY_4, gpios, {0});
static const struct gpio_dt_spec relay_5 = GPIO_DT_SPEC_GET_OR(RELAY_5, gpios,{0});
static const struct gpio_dt_spec relay_6 = GPIO_DT_SPEC_GET_OR(RELAY_6, gpios, {0});
static const struct gpio_dt_spec relay_7 = GPIO_DT_SPEC_GET_OR(RELAY_7, gpios,{0});

/* Function prototypes */
void set_relays(uint8_t value);
void set_relay_by_name(const char *name, bool state);
bool get_relay_by_name(const char *name);
const char* get_relay_name(int relay_number);

/**
 * @brief Set the state of all relays.
 *
 * This function controls the state of all eight relays simultaneously.
 * The state of each relay is determined by the corresponding bit in the
 * 8-bit `value` parameter (0 for OFF, 1 for ON).
 *
 * **Usage**\n
 *     set_relays(0b00001111); // Sets first four relays ON, others OFF
 *
 * @param value An 8-bit value where each bit represents the state of a relay.
 */
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

/**
 * @brief Set the state of a specific relay by its name.
 *
 * This function allows control of an individual relay by specifying its name
 * and desired state. The relay name should match one of the predefined relay names
 * (e.g., "relay_0", "relay_1", etc.). The state is a boolean where true means ON
 * and false means OFF.
 *
 * **Usage**\n
 *     set_relay_by_name("relay_3", true); // Turns ON relay_3
 *
 * @param name The name of the relay to control.
 * @param state The desired state of the relay (true for ON, false for OFF).
 */
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

/**
 * @brief Get the current state of a specific relay by its name.
 *
 * Retrieves the current state of the relay specified by its name.
 * The function returns a boolean indicating the state of the relay
 * (true if the relay is ON, false if OFF).
 *
 * **Usage**\n
 *     bool state = get_relay_by_name("relay_2"); // Gets the state of relay_2
 *
 * @param name The name of the relay whose state is to be retrieved.
 * @return The current state of the specified relay (true for ON, false for OFF).
 */
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

/**
 * @brief Get the name of the relay corresponding to a given relay number.
 *
 * This function returns the name of the relay as a string based on the
 * relay's number (0 to 7). If the relay number is out of range, it returns
 * "Unknown".
 *
 * **Usage**\n
 *     const char* name = get_relay_name(4); // Returns "relay_4"\n
 *
 * @param relay_number The number of the relay (0-7).
 * @return The name of the relay, or "Unknown" if the number is invalid.
 */
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

/**
 * @brief Shell command function to control and query relays.
 *
 * This function is designed to be used as a shell command for controlling
 * and querying the state of relays. It supports multiple sub-commands for
 * setting relay states, retrieving relay states, and listing all relays.
 *
 * **Usage**\n
 *     relays --set-bytes &lt;value&gt;    // Sets all relays according to &lt;value&gt;\n
 *     relays --get-relay &lt;name&gt;     // Gets the state of the relay &lt;name&gt;\n
 *     relays --set-relay &lt;name&gt; &lt;state&gt; // Sets the state of relay &lt;name&gt; and &lt;state&gt;\n
 *     relays --list-relays                // Lists all relays\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
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

/**
 * @brief Initialize the relay module.
 *
 * This function sets up the GPIO pins connected to the relays (relay0 through relay7).
 * Each relay is configured as an output. After initialization, all relays are set to their
 * OFF state to ensure a known startup state. This function should be called at the start
 * of the program to prepare the relay hardware for operation.
 *
 */
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
