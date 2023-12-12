/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file user_led.h
 * @brief User LED control interface.
 *
 * This header file contains the declarations for the user LED control functions
 * used in the Pluto_pico project. It includes functions for initializing and
 * controlling an LED based on the devicetree node identifier `LED0_NODE`.
 *
 * The `user_led_init` function is used to initialize the LED, setting it up for
 * control. The `user_led_thread` function contains the logic for toggling the
 * LED state in a loop.
 *
 * These functions work in conjunction with the Zephyr RTOS GPIO driver to control
 * the physical state of an LED connected to the hardware.
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#ifndef APP_USER_LED_H
#define APP_USER_LED_H

#include <zephyr/drivers/gpio.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* Function declarations */
void user_led_init(void);

_Noreturn void user_led_thread(void);

#endif // APP_USER_LED_H

