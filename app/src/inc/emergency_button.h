/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file emergency_button.h
 * @brief Emergency Button module.
 *
 * Header for emergency_button module
 *
 * @author Jannis Ruellmann
 */

#ifndef APP_EMERGENCY_BUTTON_H
#define APP_EMERGENCY_BUTTON_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define EM_BUTTON_0	DT_ALIAS(embutton)
#define EMERGENCY_BUTTON_FLAGS (GPIO_INPUT | GPIO_PULL_DOWN | GPIO_INT_EDGE_TO_ACTIVE)


// Function declarations
void emergency_button_init();

#endif //APP_EMERGENCY_BUTTON_H
