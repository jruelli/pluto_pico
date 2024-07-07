/*
 * Copyright (c) Jannis Ruellmann 2024
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

#ifndef APP_PLUTO_MOTORDRIVER_H
#define APP_PLUTO_MOTORDRIVER_H

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

// GPIO specifications from device tree
#define DIR_1	DT_ALIAS(dir1)
#define DIR_2	DT_ALIAS(dir2)
#define PWM_1   DT_ALIAS(pwm1)
#define PWM_2   DT_ALIAS(pwm2)

/** @brief Delay before performing initial speed adjustment. */
#define ADJUST_SPEED_DELAY_MS 5
/** @brief CHECK_INTERVAL for verifying speed is 0 before accelerating in opposite direction. */
#define CHECK_INTERVAL_MS 10
/** @brief Wait time before moving to opposite direction. */
#define WAIT_DIR_CHANGE_INTERVAL_MS 100

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

// Function declarations
void init_motor(motor_t* motor);
void set_speed(motor_t* motor, uint32_t speed_percent);
void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id);
void set_motors(motor_t *motor1, motor_t *motor2, uint32_t speed1, uint32_t speed2, bool dir1, bool dir2);
void motordriver_set_dir(motor_t* motor, bool dir);
void motordriver_adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed);
void motordriver_adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed);
void motordriver_stop_motors();

void cmd_motor1_init();
void cmd_motor2_init();
void cmd_motors_init();

// Declare global motor structs
extern motor_t motor1;
extern motor_t motor2;

#endif //APP_PLUTO_MOTORDRIVER_H
