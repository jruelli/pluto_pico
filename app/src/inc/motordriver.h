/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file motordriver.h
 * @brief Motor driver module
 *
 * Header for motordriver module
 *
 * @author Jannis Ruellmann
 */

#ifndef APP_MOTORDRIVER_H
#define APP_MOTORDRIVER_H

// GPIO specifications from device tree
#define DIR_1	DT_ALIAS(dir1)
#define DIR_2	DT_ALIAS(dir2)
#define PWM_1   DT_ALIAS(pwm1)
#define PWM_2   DT_ALIAS(pwm2)

typedef struct {
    const char* name;
    struct gpio_dt_spec dir_pin;
    struct pwm_dt_spec pwm_spec;
} motor_t;

// Function declarations
void motordriver_init();

#endif //APP_MOTORDRIVER_H
