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
 */

#ifndef APP_USB_CLI_H
#define APP_USB_CLI_H

#include <zephyr/kernel.h>

/** @brief Macro to convert a symbol to a string (stringify). */
#define USB_CLI_STR(x) # x
/** @brief Macro to expand and stringify a symbol. */
#define USB_CLI_X_STR(x) USB_CLI_STR(x)

/* Function declarations */
void usb_cli_init(void);
uint8_t simple_strtou8(const char *str);

#endif // APP_USB_CLI_H
