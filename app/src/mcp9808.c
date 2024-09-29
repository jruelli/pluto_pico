/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file mcp9808.c
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
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "inc/mcp9808.h"
#include "inc/usb_cli.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(mcp9808, LOG_LEVEL_DBG);

/* Function prototypes */
const char* get_mcp9808_name(int mcp9808_number);
double get_mcp9808_by_name(const char *name);

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
const char* get_mcp9808_name(int mcp9808_number) {
    switch (mcp9808_number) {
        case 0: return "mcp9808_0";
        case 1: return "mcp9808_1";
        case 2: return "mcp9808_2";
        default: return "Unknown";
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
double get_mcp9808_by_name(const char *name) {
    double temp = 69.0;
    if (strcmp(name, "mcp9808_0") == 0) {
        temp = 0.0;
    } else if (strcmp(name, "mcp9808_1") == 0) {
        temp = 1.0;
    } else if (strcmp(name, "mcp9808_2") == 0) {
        temp = 2.0;
    } else {
        LOG_ERR("mcp9808 not known.");
    }
    return temp;
}

/**
 * @brief Root command function for relays.
 *
 * This function is called if a wrong subcommand has been selected.
 * This is a root command (level 0 command).
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_mcp9808(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

/**
 * @brief Gets the state of relay &lt;name&gt;
 *
 * This command is useful to get state of a specific relay.
 * This is a subcommand (level 1 command) array for command "relays".
 *
 * **Usage**\n
 *     relays --get-relay &lt;name&gt; \n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments;
 * @return Returns 0 on successful execution, or an error code on failure.
 */
static int cmd_mcp9808_get_temp(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        double temperature = get_mcp9808_by_name(name);
        shell_print(shell, "%f", temperature);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

/**
 * @brief Lists names of all mcp9808 devices
 *
 * This command is useful to get names of all relays.
 * This is a subcommand (level 1 command) array for command "relays".
 *
 * **Usage**\n
 *     mcp9808 --list-sensors \n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments;
 * @return Returns 0 on successful execution, or an error code on failure.
 */
static int cmd_mcp9808_list_sensors(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 1) {
        for (int i = 0; i <= 2; i++) {
            shell_print(shell, "%s", get_mcp9808_name(i));
        }
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
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
void mcp9808_pluto_init() {
    LOG_INF("Initializing mcp9808 module");
}

/* Creating subcommands (level 1 command) array for command "mcp9808". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_mcp9808,
                               SHELL_CMD(get-temp, NULL, "Get temp value [°C] of temp sensor <name>.",
                                         cmd_mcp9808_get_temp),
                               SHELL_CMD(list-sensors, NULL, "List all mcp9808 sensor names.",
                                         cmd_mcp9808_list_sensors),
                               SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "mcp9808" */
SHELL_CMD_REGISTER(mcp9808, &sub_mcp9808, "control temperature sensors.", cmd_mcp9808);