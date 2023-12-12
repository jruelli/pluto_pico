/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file user_led.c
 * @brief LED control module
 *
 * This module provides functions to initialize and control an LED. It includes
 * both initialization and a continuous control loop for LED state toggling.
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include "inc/usb_cli.h"

/**
 * @brief Initializes the USB CLI interface.
 *
 * This function sets up the USB device controller and waits for the Data Terminal
 * Ready (DTR) signal, indicating that the USB serial connection is established.
 */
void usb_cli_init(void) {
    const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
}

/**
 * @brief USB CLI thread function.
 *
 * This thread manages the USB CLI interface, handling input and output.
 */
_Noreturn void usb_cli_thread(void) {
    while (1) {
        // Handle USB CLI communication here
        printk("USB CLI active...\n");
        k_sleep(K_SECONDS(1));
    }
}
