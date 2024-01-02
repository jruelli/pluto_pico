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
void set_speed(motor_t* motor, uint32_t speed_percent);

// Function declarations
void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id);

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
        .acceleration_rate_delay = 50,
        .braking_rate = 10,
        .braking_rate_delay = 100,
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
        .acceleration_rate_delay = 50,
        .braking_rate = 10,
        .braking_rate_delay = 100,
        .mutex = &motor2_mutex,
        .timer = &motor2_timer,
};

static int cmd_motor1(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "set-dir") == 0) {
            bool target_direction = simple_strtou8(argv[2]) != 0;
            motordriver_set_dir(&motor1, target_direction);
        } else if (strcmp(argv[1], "set-speed") == 0) {
            uint32_t target_speed = simple_strtou8(argv[2]);
            motordriver_adjust_motor_speed_non_blocking(&motor1, target_speed);
        } else if (strcmp(argv[1], "Zset-speed") == 0) {
            uint32_t speed = simple_strtou8(argv[2]);
            set_speed(&motor1, speed);
        } else if (strcmp(argv[1], "get-speed") == 0) {
            shell_print(shell, "%d", motor1.speed);
        } else if (strcmp(argv[1], "get-dir") == 0) {
            shell_print(shell, "%d", motor1.direction);
        } else if (strcmp(argv[1], "get-motor") == 0) {
            shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                               "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                               motor1.name, motor1.direction, motor1.speed, motor1.acceleration_rate,
                               motor1.acceleration_rate_delay, motor1.braking_rate, motor1.braking_rate_delay);
        } else if (strcmp(argv[1], "config-acc-rate") == 0) {
                uint32_t acceleration_rate = simple_strtou8(argv[2]);
                if (acceleration_rate != 0 && (acceleration_rate < 100)) {
                    motor1.acceleration_rate = acceleration_rate;
                } else {
                    shell_print(shell, "Invalid acceleration_rate.");
                }
        } else if (strcmp(argv[1], "config-brak-rate") == 0) {
            uint32_t braking_rate = simple_strtou8(argv[2]);
            if (braking_rate != 0 && (braking_rate < 100)) {
                motor1.braking_rate = braking_rate;
            } else {
                shell_print(shell, "Invalid braking_rate.");
            }
        } else if (strcmp(argv[1], "config-acc-rate-delay") == 0) {
            int32_t acceleration_rate_delay = (int32_t)simple_strtou8(argv[2]);
            if (acceleration_rate_delay != 0) {
                motor1.acceleration_rate_delay = acceleration_rate_delay;
            }
        } else if (strcmp(argv[1], "config-brak-rate-delay") == 0) {
            int32_t braking_rate_delay = (int32_t)simple_strtou8(argv[2]);
            if (braking_rate_delay != 0) {
                motor1.braking_rate_delay = braking_rate_delay;
            }
        } else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor1 set-dir <0/1>"
                           "| motor1 set-speed <0-100>"
                           "| motor1 Zset-speed <0-100> (unsafe)"
                           "| motor1 get-dir"
                           "| motor1 get-speed"
                           "| motor1 get-motor"
                           "| motor1 config-acc-rate <0-100>"
                           "| motor1 config-acc-rate-delay <0-0xFF>"
                           "| motor1 config-brak-rate <0-100>"
                           "| motor1 config-brak-rate-delay <0-0xFF>");
    }
    return 0;
}

static int cmd_motor2(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "set-dir") == 0) {
            bool target_direction = simple_strtou8(argv[2]) != 0;
            motordriver_set_dir(&motor2, target_direction);
        } else if (strcmp(argv[1], "set-speed") == 0) {
            uint32_t target_speed = simple_strtou8(argv[2]);
            motordriver_adjust_motor_speed_non_blocking(&motor2, target_speed);
        } else if (strcmp(argv[1], "Zset-speed") == 0) {
            uint32_t speed = simple_strtou8(argv[2]);
            set_speed(&motor2, speed);
        } else if (strcmp(argv[1], "get-speed") == 0) {
            shell_print(shell, "%d", motor2.speed);
        } else if (strcmp(argv[1], "get-dir") == 0) {
            shell_print(shell, "%d", motor2.direction);
        } else if (strcmp(argv[1], "get-motor") == 0) {
            shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                               "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                        motor2.name, motor2.direction, motor2.speed, motor2.acceleration_rate,
                        motor2.acceleration_rate_delay, motor2.braking_rate, motor2.braking_rate_delay);
        } else if (strcmp(argv[1], "config-acc-rate") == 0) {
            uint32_t acceleration_rate = simple_strtou8(argv[2]);
            if (acceleration_rate != 0 && (acceleration_rate < 100)) {
                motor2.acceleration_rate = acceleration_rate;
            } else {
                shell_print(shell, "Invalid acceleration_rate.");
            }
        } else if (strcmp(argv[1], "config-brak-rate") == 0) {
            uint32_t braking_rate = simple_strtou8(argv[2]);
            if (braking_rate != 0 && (braking_rate < 100)) {
                motor2.braking_rate = braking_rate;
            } else {
                shell_print(shell, "Invalid braking_rate.");
            }
        } else if (strcmp(argv[1], "config-acc-rate-delay") == 0) {
            int32_t acceleration_rate_delay = (int32_t)simple_strtou8(argv[2]);
            if (acceleration_rate_delay != 0) {
                motor2.acceleration_rate_delay = acceleration_rate_delay;
            }
        } else if (strcmp(argv[1], "config-brak-rate-delay") == 0) {
            int32_t braking_rate_delay = (int32_t)simple_strtou8(argv[2]);
            if (braking_rate_delay != 0) {
                motor2.braking_rate_delay = braking_rate_delay;
            }
        } else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor2 set-dir <0/1>"
                           "| motor2 set-speed <0-100>"
                           "| motor2 Zset-speed <0-100> (unsafe)"
                           "| motor2 get-dir"
                           "| motor2 get-speed"
                           "| motor2 get-motor"
                           "| motor2 config-acc-rate <0-100>"
                           "| motor2 config-acc-rate-delay <0-0xFF>"
                           "| motor2 config-brak-rate <0-100>"
                           "| motor2 config-brak-rate-delay <0-0xFF>");
    }
    return 0;
}

void motordriver_set_dir(motor_t* motor, bool dir) {
    // Set the direction of the motor
    k_mutex_lock(motor->mutex, K_FOREVER);
    if (motor->direction != dir)
    {
        uint32_t target_speed = 0;
        motordriver_adjust_motor_speed_blocking(motor, target_speed);
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
    k_mutex_lock(motor->mutex, K_FOREVER);
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
    k_mutex_unlock(motor->mutex);
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
    motordriver_set_dir(motor, initial_direction);
    printk("%s configured!\n", motor->name);
    motordriver_adjust_motor_speed_blocking(motor, initial_speed);
}

/**
 * @brief Gradually adjusts the speed of the motor.
 *
 * @param motor Pointer to the motor structure.
 * @param target_speed The target speed as a percentage (0-100).
 * @param rate The rate of speed change (acceleration or braking).
 */
void motordriver_adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed) {
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
            k_msleep(motor->acceleration_rate_delay);
        } else if (motor->speed > target_speed){
            // Brake
            motor->speed -= (motor->speed - motor->braking_rate < target_speed) ?
                            (motor->speed - target_speed) : motor->braking_rate;
            set_speed(motor, motor->speed);
            k_msleep(motor->braking_rate_delay);
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
            k_timer_start(timer_id, K_MSEC(motor->acceleration_rate_delay), K_NO_WAIT);
        } else {
            //braking timer
            k_timer_start(timer_id, K_MSEC(motor->braking_rate_delay), K_NO_WAIT);
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
void motordriver_adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed) {
    if (motor->acceleration_rate == 0 || motor->braking_rate == 0) {
        printk("Rate of speed change cannot be zero.\n");
        return;
    }
    k_mutex_lock(motor->mutex, K_FOREVER);
    uint32_t new_target_speed = MIN(target_speed, 100);
    // Only start or restart the timer if the target speed has changed
    if (motor->target_speed != new_target_speed) {
        motor->target_speed = new_target_speed;
        // wait a bit before starting adjusting speed
        k_timer_start(motor->timer, K_MSEC(ADJUST_SPEED_DELAY_MS), K_NO_WAIT);
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

motor_t motordriver_get_motor1() {
    return motor1;
}

motor_t motordriver_get_motor2() {
    return motor2;
}
SHELL_CMD_REGISTER(motor1, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor1);
SHELL_CMD_REGISTER(motor2, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor2);
