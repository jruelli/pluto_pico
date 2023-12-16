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

/* Function declarations */
void usb_cli_init(void);

#endif // APP_USB_CLI_H
