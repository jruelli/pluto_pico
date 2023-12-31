/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file usb_cli.c
 * @brief USB Command Line Interface (CLI) Module
 *
 * This module offers a command-line interface over USB, allowing for interactive
 * control and monitoring of the device's functionalities. It initializes and manages
 * a USB serial connection, enabling communication between the device and a USB host.
 *
 * Key functionalities include:
 * - Initializing the USB CLI interface and handling USB communication setup.
 * - Registering and handling custom shell commands, such as 'echo' for echoing messages
 *   and 'version' for displaying application version information.
 * - Processing user input from the USB serial interface and responding accordingly.
 *
 * The module is initialized using usb_cli_init(), which sets up the necessary USB
 * configurations, registers shell commands, and prepares the system for receiving
 * and processing commands over the USB interface. This module is essential for
 * applications requiring direct interaction with the user through a simple and
 * accessible interface.
 *
 * Usage scenarios include debugging, configuration, or control of the embedded device
 * in development and production environments.
 *
 * @author Jannis Ruellmann
 */


#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <app_version.h>
#include <zephyr/shell/shell.h>
#include "inc/usb_cli.h"

LOG_MODULE_REGISTER(usb_cli, LOG_LEVEL_INF);

/**
 * @brief Echo a message.
 *
 * This command echoes the provided message back to the shell. It is used to
 * demonstrate basic input and output functionality of the shell over USB.
 *
 * **Usage**\n
 *     echo &lt;message&gt; // Echoes the provided message\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments; argv[1] is the message to echo.
 * @return Returns 0 on successful execution, or an error code on failure.
 */
static int cmd_echo(const struct shell *shell, size_t argc, char **argv) {
    // No additional arguments empty message
    if (argc > 1) {
        shell_print(shell, "%s", argv[1]);
    } else {
        shell_print(shell, "Usage: echo <message>");
    }
    return 0;
}

/**
 * @brief Display version information.
 *
 * This command outputs the application's version information. It supports an
 * optional argument to display the build version. This function is useful for
 * identifying the software version running on the device.
 *
 * **Usage**\n
 *     version              // Displays the app version\n
 *     version --build-ver  // Displays the app build version\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments; argv[1] can be "--build-ver" to display build version.
 * @return Returns 0 on successful execution, or an error code on failure.
 */
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

/**
 * @brief Initialize the USB CLI interface.
 *
 * This function sets up the USB CLI interface. It enables the USB subsystem,
 * waits for a connection to be established, and prepares the shell interface
 * for command processing. It is essential for enabling communication over USB
 * and should be called during system initialization.
 *
 * **Usage**\n
 *     usb_cli_init(); // Initializes the USB CLI interface\n
 *
 * The function handles USB initialization and provides initial output to
 * indicate successful startup. It is a critical step in setting up the device's
 * interactive capabilities.
 */
void usb_cli_init(void) {
    printk("Starting USB shell...\n");
    if (usb_enable(NULL)) {
        printk("Failed to enable USB\n");
        return;
    }
    printk("USB shell started. Type your commands.\n");
}

SHELL_CMD_REGISTER(echo, NULL, "echo <message>", cmd_echo);
SHELL_CMD_REGISTER(version, NULL, "print version", cmd_version);
