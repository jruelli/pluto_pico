/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file user_led.h
 * @brief User LED control interface.
 *
 * Header for user_led module
 *
 * @author Jannis Ruellmann
 */

#ifndef APP_USER_LED_H
#define APP_USER_LED_H

#include <zephyr/drivers/gpio.h>

/** @brief The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* Function declarations */
void user_led_init(void);
_Noreturn void user_led_thread(void);

#endif // APP_USER_LED_H
