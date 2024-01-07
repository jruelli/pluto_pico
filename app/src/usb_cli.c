/*
 * Copyright (c) Jannis Ruellmann 2024
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

/* Enable logging for module. Change Log Level for debugging. */
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
static int cmd_echo(const struct shell *sh, size_t argc, char **argv) {
    // No additional arguments empty message
    if (argc > 1) {
        shell_print(sh, "%s", argv[1]);
    } else {
        shell_print(sh, "Usage: echo <message>");
    }
    return 0;
}

/**
 * @brief Display version.
 *
 * This command outputs the application's version. This function is useful for
 * identifying the software version running on the device.
 * This is a root (level 0) command "version".
 * **Usage**\n
 *     version\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments; argv[1] can be "--build-ver" to display build version.
 * @return Returns 0 on successful execution, or an error code on failure.
 */
static int cmd_version(const struct shell *sh, size_t argc, char **argv) {
    if (argc == 1) {
        shell_print(sh, "App Version: " APP_VERSION_STRING);
    } else {
        shell_print(sh, "Unknown parameter: '%s'", argv[1]);
    }
    return 0;
}

/**
 * @brief Display app build version.
 *
 * This command outputs the application's build version. This function is useful for
 * identifying the software version running on the device.
 * This is a subcommand (level 1 command) array for command "version".
 *
 * **Usage**\n
 *     version --build-ver  // Displays the app build version\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments;
 * @return Returns 0 on successful execution, or an error code on failure.
 */
static int cmd_version_build_ver(const struct shell *sh, size_t argc,
                                 char **argv) {
    shell_print(sh, "App Build Version: " USB_CLI_X_STR(APP_BUILD_VERSION));
    return 0;
}

/**
 * @brief Convert a string to an unsigned 8-bit integer.
 *
 * This function parses a string and converts it into an 8-bit unsigned integer.
 * It processes characters until a non-digit is encountered or the end of the
 * string is reached. This function is used for simple string-to-number conversion
 * without external dependencies.
 *
 * **Usage**\n
 *     uint8_t num = simple_strtou8("123"); // Converts "123" to 123\n
 *
 * @param str Pointer to the null-terminated string to be converted.
 * @return The converted 8-bit unsigned integer value.
 */
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

/**
 * @brief Convert a string to an unsigned 16-bit integer.
 *
 * This function parses a string and converts it into a 16-bit unsigned integer.
 * It processes characters until a non-digit is encountered or the end of the
 * string is reached. This function is used for simple string-to-number conversion
 * without external dependencies.
 *
 * **Usage**\n
 *     uint16_t num = simple_strtou16("12345"); // Converts "12345" to 12345\n
 *
 * @param str Pointer to the null-terminated string to be converted.
 * @return The converted 16-bit unsigned integer value.
 */
uint16_t simple_strtou16(const char *str) {
    uint16_t result = 0;
    while (*str) {
        if (*str < '0' || *str > '9') {
            break;
        }
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

/**
 * @brief Convert a string to an unsigned 32-bit integer.
 *
 * This function parses a string and converts it into a 32-bit unsigned integer.
 * It processes characters until a non-digit is encountered or the end of the
 * string is reached. This function is used for simple string-to-number conversion
 * without external dependencies.
 *
 * **Usage**\n
 *     uint32_t num = simple_strtou32("1234567890"); // Converts "1234567890" to 1234567890\n
 *
 * @param str Pointer to the null-terminated string to be converted.
 * @return The converted 32-bit unsigned integer value.
 */
uint32_t simple_strtou32(const char *str) {
    uint32_t result = 0;
    while (*str) {
        if (*str < '0' || *str > '9') {
            break;
        }
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
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

/* Creating subcommands (level 1 command) array for command "version". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_version,
                               SHELL_CMD(build-ver, NULL, "App Build Version.",
                                         cmd_version_build_ver),
                               SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "echo" */
SHELL_CMD_REGISTER(echo, NULL, "echo <message> back", cmd_echo);

/* Creating root (level 0) command "version" */
SHELL_CMD_REGISTER(version, &sub_version, "App version.", cmd_version);
