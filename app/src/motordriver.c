#include <sys/cdefs.h>
/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "inc/motordriver.h"
#include "inc/usb_cli.h"

void init_motor(motor_t* motor);
void set_dir(motor_t* motor, bool dir);
void set_speed(motor_t* motor, uint32_t speed_percent);
void motordriver_stop(motor_t* motor, uint32_t braking_rate);
void adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed);
void adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed);
void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id);

_Noreturn void motor_control_thread(void *p1, void *p2, void *p3);

static const struct pwm_dt_spec pwm_1 = PWM_DT_SPEC_GET_OR(PWM_1, {0});
static const struct pwm_dt_spec pwm_2 = PWM_DT_SPEC_GET_OR(PWM_2, {0});

static const struct gpio_dt_spec dir_1 = GPIO_DT_SPEC_GET_OR(DIR_1, gpios, {0});
static const struct gpio_dt_spec dir_2 = GPIO_DT_SPEC_GET_OR(DIR_2, gpios,{0});

// Define global mutexes and timers
struct k_mutex motor1_mutex;
struct k_timer motor1_timer;

struct k_mutex motor2_mutex;
struct k_timer motor2_timer;

motor_t motor1 = {
        .name = "motor1",
        .dir_pin = dir_1,
        .pwm_spec = pwm_1,
        .emergency_stop = 0,
        .direction = 0,
        .target_direction = 0,
        .speed = 0,
        .target_speed = 0,
        .acceleration_rate = 10,
        .braking_rate = 10,
        .mutex = &motor1_mutex,
        .timer = &motor1_timer,
};

motor_t motor2 = {
        .name = "motor2",
        .dir_pin = dir_2,
        .pwm_spec = pwm_2,
        .emergency_stop = 0,
        .direction = 0,
        .target_direction = 0,
        .speed = 0,
        .target_speed = 0,
        .acceleration_rate = 10,
        .braking_rate = 10,
        .mutex = &motor2_mutex,
        .timer = &motor2_timer,
};

#define ACCELERATION_STEP_DELAY_MS 50
#define BRAKING_STEP_DELAY_MS 100

static int cmd_motor1(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 2) {
        if (strcmp(argv[1], "set-dir") == 0) {
            bool target_direction = simple_strtou8(argv[2]) != 0;
            set_dir(&motor1, target_direction);
        } else if (strcmp(argv[1], "set-speed") == 0) {
            uint32_t target_speed = simple_strtou8(argv[2]);
            adjust_motor_speed_non_blocking(&motor1, target_speed);
        } else if (strcmp(argv[1], "Xset-speed") == 0) {
            uint32_t speed = simple_strtou8(argv[2]);
            set_speed(&motor1, speed);
        }
        else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor1 set-dir <0/1> or motor1 set-speed <0-100>");
    }
    return 0;
}

static int cmd_motor2(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 2) {
        if (strcmp(argv[1], "set-dir") == 0) {
            bool target_direction = simple_strtou8(argv[2]) != 0;
            set_dir(&motor2, target_direction);
        } else if (strcmp(argv[1], "set-speed") == 0) {
            uint32_t target_speed = simple_strtou8(argv[2]);
            adjust_motor_speed_non_blocking(&motor2, target_speed);
        } else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor2 set-dir <0/1> or motor2 set-speed <0-100>");
    }
    return 0;
}

void set_dir(motor_t* motor, bool dir) {
    // Set the direction of the motor
    k_mutex_lock(motor->mutex, K_FOREVER);
    if (motor->direction != dir)
    {
        uint32_t target_speed = 0;
        adjust_motor_speed_blocking(motor, target_speed);
        motor->direction = dir;
        gpio_pin_set(motor->dir_pin.port, motor->dir_pin.pin,motor->direction);
        printk("Direction of %s set to %d\n", motor->name, motor->direction);
    }
    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Set the speed of the motor.
 *
 * This function sets the speed of the motor by adjusting the PWM duty cycle.
 * The speed is set as a percentage of the maximum speed, where 0% is stopped
 * and 100% is full speed.
 *
 * @param motor Pointer to the motor structure.
 * @param speed_percent The speed of the motor as a percentage (0-100).
 */
void set_speed(motor_t* motor, uint32_t speed_percent) {
    struct k_mutex *mutex = (motor == &motor1) ? &motor1_mutex : &motor2_mutex;
    k_mutex_lock(mutex, K_FOREVER);
    // Ensure the speed_percent is within bounds
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    // Calculate the duty cycle based on the speed percentage
    uint32_t duty_cycle_ns = motor->pwm_spec.period * speed_percent / 100;
    // Set the PWM duty cycle
    int ret = pwm_set(motor->pwm_spec.dev,
                      motor->pwm_spec.channel,
                      motor->pwm_spec.period,
                      duty_cycle_ns, motor->pwm_spec.flags);
    if (ret < 0) {
        printk("Error setting PWM speed for %s: %d\n", motor->name, ret);
    } else {
        // Update the motors speed in the struct
        motor->speed = speed_percent;
    }
    k_mutex_unlock(mutex);
}

void init_motor(motor_t* motor) {
    if (!device_is_ready(motor->pwm_spec.dev)) {
        printk("%s Error: PWM not ready.\n", motor->name);
        return;
    }
    // Initialize GPIO pins as outputs for direction and PWM
    gpio_pin_configure_dt(&motor->dir_pin, GPIO_OUTPUT);
    k_mutex_init(motor->mutex);
    k_timer_init(motor->timer, motor_speed_adjust_timer_expiry_function, NULL);
    k_timer_user_data_set(motor->timer, motor);
    // Set initial direction and speed (PWM) to OFF
    bool initial_direction = 0;
    uint32_t initial_speed = 0;
    set_dir(motor, initial_direction);
    printk("%s configured!\n", motor->name);
    adjust_motor_speed_blocking(motor, initial_speed);
}

/**
 * @brief Gradually adjusts the speed of the motor.
 *
 * @param motor Pointer to the motor structure.
 * @param target_speed The target speed as a percentage (0-100).
 * @param rate The rate of speed change (acceleration or braking).
 */
void adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed) {
    if (motor->acceleration_rate == 0) {
        printk("Rate of speed change cannot be zero.\n");
        return;
    }
    // Ensure target speed is within bounds
    if (target_speed > 100) {
        target_speed = 100;
    }
    while (motor->speed != target_speed) {
        if (motor->speed < target_speed) {
            // Accelerate
            motor->speed += (motor->speed + motor->acceleration_rate > target_speed) ?
                            (target_speed - motor->speed) : motor->acceleration_rate;
            set_speed(motor, motor->speed);
            k_msleep(ACCELERATION_STEP_DELAY_MS);
        } else if (motor->speed > target_speed){
            // Brake
            motor->speed -= (motor->speed - motor->braking_rate < target_speed) ?
                            (motor->speed - target_speed) : motor->braking_rate;
            set_speed(motor, motor->speed);
            k_msleep(BRAKING_STEP_DELAY_MS);
        }
    }
    printk("%s target speed: %d reached.\n", motor->name, motor->speed);
}

void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id) {
    motor_t *motor = k_timer_user_data_get(timer_id);
    k_mutex_lock(motor->mutex, K_FOREVER);
    if (motor->speed < motor->target_speed) {
        // Accelerate
        uint32_t speed_increment = MIN(motor->acceleration_rate, motor->target_speed - motor->speed);
        motor->speed += speed_increment;
        set_speed(motor, motor->speed);
    } else if (motor->speed > motor->target_speed) {
        // Brake
        uint32_t speed_decrement = MIN(motor->braking_rate, motor->speed - motor->target_speed);
        motor->speed -= speed_decrement;
        set_speed(motor, motor->speed);
    }

    if (motor->speed != motor->target_speed) {
        if (motor->speed < motor->target_speed) {
            //acceleration timer
            k_timer_start(timer_id, K_MSEC(ACCELERATION_STEP_DELAY_MS), K_NO_WAIT);
        } else {
            //braking timer
            k_timer_start(timer_id, K_MSEC(BRAKING_STEP_DELAY_MS), K_NO_WAIT);
        }
    } else {
        printk("%s target speed: %d reached.\n", motor->name, motor->speed);
    }

    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Gradually adjusts the speed of the motor in a nonblocking mode.
 *
 * @param motor Pointer to the motor structure.
 * @param target_speed The target speed as a percentage (0-100).
 * @param rate The rate of speed change (acceleration or braking).
 */
void adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed) {
    if (motor->acceleration_rate == 0 || motor->braking_rate == 0) {
        printk("Rate of speed change cannot be zero.\n");
        return;
    }
    k_mutex_lock(motor->mutex, K_FOREVER);
    uint32_t new_target_speed = MIN(target_speed, 100);
    // Only start or restart the timer if the target speed has changed
    if (motor->target_speed != new_target_speed) {
        motor->target_speed = new_target_speed;
        // wait a bit before starting adjusting speed. Define used as placeholder
        k_timer_start(motor->timer, K_MSEC(ACCELERATION_STEP_DELAY_MS), K_NO_WAIT);
    }
    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Initialize the motordriver module.
 *
 * This function sets up the GPIO pins connected to the motordriver.
 *
 */
void motordriver_init() {
    init_motor(&motor1);
    init_motor(&motor2);
}

SHELL_CMD_REGISTER(motor1, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor1);
SHELL_CMD_REGISTER(motor2, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor2);
