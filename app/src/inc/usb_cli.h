/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file usb_cli.h
 * @brief User LED control interface.
 *
 * Header for usb_cli module
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#ifndef APP_USB_CLI_H
#define APP_USB_CLI_H

#include <zephyr/kernel.h>

/* Function declarations */
void usb_cli_init(void);
_Noreturn void usb_cli_thread(void);

#endif // APP_USB_CLI_H
