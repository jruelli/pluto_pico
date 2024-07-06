/*
 * Copyright (c) Jannis Ruellmann 2024
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

#include "inc/usb_cli.h"
#include "inc/user_led.h"
#include "inc/relays.h"
#include "inc/motordriver.h"
#include "inc/vl53l0x.h"
#include "inc/emergency_button.h"
#include "inc/pluto_mcp9808.h"
#include "inc/pluto_ads1115.h"

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
    /* Init relays */
    relay_init();
    /* Init motordriver */
    motordriver_init();
    /* Init vl53l0x*/
    vl53l0x_init();
    /* Init emrgency_button */
    emergency_button_init();
    /* Init mcp9808 temperature sensors */
    pluto_mcp9808_init();
    /* Init ads1115 adc */
    pluto_ads1115_init();
    return 0;
}
