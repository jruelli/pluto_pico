#include <sys/cdefs.h>
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

#include "inc/user_led.h"
#include <zephyr/kernel.h>

/* LED toggle interval in milliseconds */
#define SLEEP_TIME_MS   2000

/* LED device tree specification */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/**
 * @brief Initialize the user LED
 *
 * This function initializes the user LED specified by the LED device tree
 * specification. It configures the LED as an output and sets its initial
 * state to active.
 */
void user_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
}

/**
 * @brief LED control thread
 *
 * This thread function toggles the state of the user LED every 2000 milliseconds.
 * It's designed to run as a continuous thread in a Zephyr application, demonstrating
 * simple LED control.
 */
void user_led_thread(void) {
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_sleep(K_MSEC(SLEEP_TIME_MS));
    }
}
