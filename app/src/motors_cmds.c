/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file motors_cmd.c
 * @brief Functions for motor2 command
 *
 * This module provides a set of functions for controlling and querying the state
 * of motors via the pico-shell.
 *
 * @author Jannis Ruellmann
 */
#include <sys/cdefs.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>


#include "inc/motordriver.h"
#include "inc/usb_cli.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(motors_cmds, LOG_LEVEL_WRN);

/**
 * @brief Root command function for motor2.
 *
 * This function is called if a wrong subcommand has been selected.
 * This is a root command (level 0 command).
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_motors(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

/* Subcommand implementations for motors */

static int cmd_motors_set(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 5) {
        uint32_t speed_motor1 = simple_strtou32(argv[1]);
        bool dir_motor1 = simple_strtou32(argv[2]) != 0;
        uint32_t speed_motor2 = simple_strtou32(argv[3]);
        bool dir_motor2 = simple_strtou32(argv[4]) != 0;

        // Ensuring the speeds are within the valid range
        speed_motor1 = (speed_motor1 > 100) ? 100 : speed_motor1;
        speed_motor2 = (speed_motor2 > 100) ? 100 : speed_motor2;

        set_motors(&motor1, &motor2, speed_motor1, speed_motor2, dir_motor1, dir_motor2);
        LOG_DBG("Motors set: Motor1 - Speed %d, Direction %d; Motor2 - Speed %d, Direction %d",
                    speed_motor1, dir_motor1, speed_motor2, dir_motor2);
    } else {
        LOG_ERR("Usage: motors set <speed_motor1> <dir_motor1> <speed_motor2> <dir_motor2>");
    }
    return 0;
}

void cmd_motors_init() {
    LOG_DBG("Adding motors commands.");
}

/* Creating subcommands (level 1 command) array for command "motors". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_motors,
                               SHELL_CMD(set,
                                         NULL,
                                         "Set both motors <speed_motor1> <dir_motor1> <speed_motor2> <dir_motor2>",
                                         cmd_motors_set),
                               SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(motors, &sub_motors,
                   "control both motors of pico-pluto.",
                   cmd_motors);
