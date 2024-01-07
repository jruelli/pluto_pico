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

LOG_MODULE_REGISTER(user_led, LOG_LEVEL_INF);

static struct k_thread user_led_thread_data;
K_THREAD_STACK_DEFINE(user_led_stack_area, USER_LED_STACK_SIZE);

/* LED device tree specification */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

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
    k_tid_t led_tid = k_thread_create(&user_led_thread_data, user_led_stack_area,
                                      K_THREAD_STACK_SIZEOF(user_led_stack_area),
                                      (k_thread_entry_t)user_led_thread,
                                      NULL, NULL, NULL,
                                      USER_LED_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_start(led_tid);
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
        k_sleep(K_MSEC(USER_LED_SLEEP_TIME_MS));
        LOG_DBG("Toggling user LED");
    }
}
