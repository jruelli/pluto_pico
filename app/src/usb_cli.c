/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file usb_cli.c
 * @brief USB Command Line Interface (CLI) Module
 *
 * This module provides functions to set up and manage a USB CLI interface.
 * It handles USB communication with a USB host, including input/output
 * processing and USB interrupts. The module allows users to interact with
 * the device via a USB serial connection.\n
 * The module is started via usb_cli_init() which will set up usb_cli_thread()
 * that will be used to process any incoming commands.
 *
 * @author Jannis Ruellmann
 */

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <app_version.h>
#include <zephyr/shell/shell.h>
#include "inc/usb_cli.h"
#include "inc/relays.h"

LOG_MODULE_REGISTER(usb_cli, LOG_LEVEL_INF);

/* Function prototypes */
uint8_t simple_strtou8(const char *str);

static int cmd_echo(const struct shell *shell, size_t argc, char **argv) {
    // No additional arguments empty message
    if (argc > 1) {
        shell_print(shell, "%s", argv[1]);
    } else {
        shell_print(shell, "Usage: echo <message>");
    }
    return 0;
}

static int cmd_version(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 1) {
        // No additional arguments, just print the app version
        shell_print(shell, "App Version: " APP_VERSION_STRING);
    } else if (argc == 2 && strcmp(argv[1], "--build-ver") == 0) {
        shell_print(shell, "App Build Version: " USB_CLI_X_STR(APP_BUILD_VERSION));
    } else {
        shell_print(shell, "Unknown parameter: '%s'", argv[1]);
    }
    return 0;
}

static int cmd_relays(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 1) {
        shell_print(shell, "Usage: relays --set-bytes <value[0..255]> "
                           "| --set-relay <name> <state[1/0]> "
                           "| --list-relays");
        return 0;
    }
    if (strcmp(argv[1], "--set-bytes") == 0) {
        if (argc == 3) {
            uint8_t value = simple_strtou8(argv[2]);
            control_relays(value);
        } else {
            shell_print(shell, "Invalid number of arguments for --bin-value");
        }
    } else if (strcmp(argv[1], "--set-relay") == 0) {
        if (argc == 4) {
            const char *name = argv[2];
            bool state_val = simple_strtou8(argv[3]) != 0; // Convert to boolean
            control_relay_by_name(name, state_val);
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

SHELL_CMD_REGISTER(echo, NULL, "echo <message>", cmd_echo);
SHELL_CMD_REGISTER(version, NULL, "print version", cmd_version);
SHELL_CMD_REGISTER(relays, NULL, "control relays of pico. Execute without arguments to get more info", cmd_relays);

/**
 * @brief Initialize the USB CLI interface.
 *
 * This function initializes the USB CLI interface by enabling USB, waiting for
 * the Data Terminal Ready (DTR) signal to indicate that the USB serial connection
 * is established, and starting the USB CLI thread for input and output handling.
 * It also sets up the UART interrupt callback, configures the ring buffer, and
 * sends initial messages to the USB serial interface.
 */
void usb_cli_init(void) {
    printk("Starting USB shell...\n");
    if (usb_enable(NULL)) {
        printk("Failed to enable USB\n");
        return;
    }
    printk("USB shell started. Type your commands.\n");
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
