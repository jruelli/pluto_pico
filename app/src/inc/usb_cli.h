/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file usb_cli.h
 * @brief USB Command Line Interface (CLI) module.
 *
 * Header for usb_cli module
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#ifndef APP_USB_CLI_H
#define APP_USB_CLI_H

#include <zephyr/kernel.h>

/** @brief Size of the ring buffer used for USB CLI data storage. */
#define RING_BUF_SIZE 1024
/** @brief Maximum size of a USB CLI message. */
#define MSG_SIZE 32
/** @brief Priority of the USB CLI thread. */
#define USB_CLI_THREAD_PRIORITY 5
/** @brief Stack size for the USB CLI thread. */
#define USB_CLI_THREAD_STACK_SIZE 1024

/** @brief Macro to convert a symbol to a string (stringify). */
#define USB_CLI_STR(x) # x
/** @brief Macro to expand and stringify a symbol. */
#define USB_CLI_X_STR(x) USB_CLI_STR(x)

/**
 * @brief Structure to define a USB CLI command.
 *
 * This structure defines a USB CLI command, including its name, usage,
 * description, and a handler function to execute the command.
 */
typedef struct {
    /** @brief Name of the command. */
    const char *name;
    /** @brief Usage information for the command. */
    const char *usage;
    /** @brief Description of the command. */
    const char *description;
    /** @brief Handler function to execute the command. */
    void (*handler)(char *args);
} Command;

/* Function declarations */
void usb_cli_init(void);

#endif // APP_USB_CLI_H
