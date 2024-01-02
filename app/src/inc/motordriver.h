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

#define ADJUST_SPEED_DELAY_MS 5

typedef struct {
    const char* name;
    struct gpio_dt_spec dir_pin;
    struct pwm_dt_spec pwm_spec;
    bool emergency_stop;
    bool direction;
    bool target_direction;
    uint32_t speed;
    uint32_t target_speed;
    uint32_t acceleration_rate;
    int32_t acceleration_rate_delay;
    uint32_t braking_rate;
    int32_t braking_rate_delay;
    struct k_mutex *mutex;            // Mutex for thread-safe access
    struct k_timer *timer;            // Timer for non-blocking speed control

} motor_t;

// Function declarations
void motordriver_init();
motor_t motordriver_get_motor1();
motor_t motordriver_get_motor2();
void motordriver_set_dir(motor_t* motor, bool dir);
void motordriver_adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed);
void motordriver_adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed);


#endif //APP_MOTORDRIVER_H
