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

#include "inc/motordriver.h"
#include "inc/usb_cli.h"

void motor_init(motor_t* motor);
void motordriver_set_dir(motor_t* motor, bool dir);
static const struct gpio_dt_spec dir_1 = GPIO_DT_SPEC_GET_OR(DIR_1, gpios, {0});
static const struct gpio_dt_spec dir_2 = GPIO_DT_SPEC_GET_OR(DIR_2, gpios,{0});
motor_t motor1 = {
        .name = "motor1",
        .dir_pin = dir_1,
        .pwm_pin = pwm_1,
        .pwm_period_ns = pwm_period_ns,
};

motor_t motor2 = {
        .name = "motor2",
        .dir_pin = dir_2,
        .pwm_pin = pwm_2,
        .pwm_period_ns = pwm_period_ns,
};
static int cmd_motor1(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 2) {
        if (strcmp(argv[1], "--set-dir") == 0) {
            bool dir = simple_strtou8(argv[2]) != 0;
            motordriver_set_dir(&motor1, dir);
            shell_print(shell, "Set direction of %s to %s", motor1.name, dir ? "FORWARD" : "REVERSE");
        } else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor1 --set-dir <0/1> or motor1 --set-speed <0-100>");
    }
    return 0;
}
static int cmd_motor2(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 2) {
        if (strcmp(argv[1], "--set-dir") == 0) {
            bool dir = simple_strtou8(argv[2]) != 0;
            motordriver_set_dir(&motor2, dir);
            shell_print(shell, "Set direction of %s to %s", motor2.name, dir ? "FORWARD" : "REVERSE");
        } else {
            shell_print(shell, "Invalid command or number of arguments.");
        }
    } else {
        shell_print(shell, "Usage: motor2 --set-dir <0/1> or motor2 --set-speed <0-100>");
    }
    return 0;
}
void motordriver_set_dir(motor_t* motor, bool dir) {
    // Set the direction of the motor
    gpio_pin_set(motor->dir_pin.port, motor->dir_pin.pin, dir);
    printk("Direction of %s set to %s\n", motor->name, dir ? "FORWARD" : "REVERSE");
}
void motor_init(motor_t* motor) {
    // Initialize GPIO pins as outputs for direction and PWM
    gpio_pin_configure_dt(&motor->dir_pin, GPIO_OUTPUT);
    // Set initial direction and speed (PWM) to OFF
    gpio_pin_set(motor->dir_pin.port, motor->dir_pin.pin, 0);
    // Here you need to set the PWM to a safe value, e.g., 0
    printk("%s configured and set to OFF!\n", motor->name);
}
/**
 * @brief Initialize the motordriver module.
 *
 * This function sets up the GPIO pins connected to the motordriver.
 *
 */
void motordriver_init() {
    motor_init(&motor1);
    motor_init(&motor2);
}

SHELL_CMD_REGISTER(motor1, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor1);
SHELL_CMD_REGISTER(motor2, NULL,
                   "control motordriver of pico-pluto. Execute without arguments to get more info",
                   cmd_motor2);
