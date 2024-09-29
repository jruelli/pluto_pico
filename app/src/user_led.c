/*
 * Copyright (c) Jannis Ruellmann 2024
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
 */

#include <sys/cdefs.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "inc/user_led.h"
#include "inc/pluto_config.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(user_led, LOG_LEVEL_WRN);

/* LED device tree specification */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

K_THREAD_DEFINE(user_led_thread_id, PLUTO_LED_THREAD_STACK_SIZE,
                user_led_thread, NULL, NULL, NULL, PLUTO_LED_THREAD_PRIORITY, 0, 0);

/**
 * @brief Initialize the user LED
 *
 * This function initializes the user LED specified by the LED device tree
 * specification. It configures the LED as an output and sets its initial
 * state to active. After it starts the led_tid thread.
 */
void user_led_init(void) {
    if (!gpio_is_ready_dt(&led)) {
        return;
    }
    gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    LOG_INF("Starting LED thread");
}

/**
 * @brief LED control thread
 *
 * This thread function toggles the state of the user LED every 2000 milliseconds.
 * It's designed to run as a continuous thread in a Zephyr application, demonstrating
 * simple LED control.
 */
_Noreturn void user_led_thread(void) {
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_sleep(K_SECONDS(PLUTO_LED_THREAD_SLEEP_TIME_S));
        LOG_DBG("Toggling user LED");
    }
}
