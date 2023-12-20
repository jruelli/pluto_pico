/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file main.c
 * @brief Main entry point for the Pluto_pico project
 *
 * This file contains the main function of the Pluto_pico project. It initializes the
 * system, sets up the USB communication, and starts the LED control thread. This serves
 * as the primary management function for the application's operation.
 *
 * The main function initializes the USB device controller and waits for the Data Terminal
 * Ready (DTR) signal. Once the DTR signal is received, it initializes the user LED module
 * and enters a loop that periodically prints "Hello World!" to the console. The LED thread
 * runs concurrently, toggling the state of an LED at regular intervals.
 *
 * @note This application is designed to run on Zephyr RTOS and demonstrates basic usage of
 *       USB communication and threading with the Zephyr kernel.
 *
 * @author Jannis Ruellmann
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include "inc/usb_cli.h"
#include "inc/user_led.h"
#include "inc/tb6612fng.h"


BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
             "Console device is not ACM CDC UART device");

/**
 * @brief Entry point for the Pluto_pico application.
 *
 * This is the main function for the Pluto_pico project. It performs the following key tasks:
 * - Initializes the USB device controller.
 * - Waits for the Data Terminal Ready (DTR) signal.
 * - Initializes and starts the user LED control thread.
 * - Enters a loop that periodically prints "Hello World!" to the console.
 *
 * The function sets up a continuous communication channel via USB and initializes the
 * LED control functionality. The system will continually operate within the main loop,
 * providing periodic updates to the console and maintaining LED control operations.
 *
 * @return int The return value is zero for successful execution and non-zero in case of error.
 */
int main(void) {
    /* Initialize and start the USB CLI thread */
    usb_cli_init();
    /* Initialize and start the user_led thread */
    user_led_init();
    /* Test motor*/
    motor_t *motor_a = NULL;
    motor_a_init(motor_a);
    return 0;
}
