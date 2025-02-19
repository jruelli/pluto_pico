/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/cdefs.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include "inc/pluto_motordriver.h"
#include "inc/usb_cli.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(motor1_cmds, LOG_LEVEL_WRN);

static int cmd_motor1(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

/* Subcommand implementations motor1 */

static int cmd_motor1_set_dir(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        bool target_direction = simple_strtou8(argv[1]) != 0;
        shell_print(shell, "%d", target_direction);
        motordriver_set_dir(&motor1, target_direction);
    } else {
        shell_error(shell, "Usage: motor1 set-dir <0/1>");
    }
    return 0;
}

static int cmd_motor1_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t target_speed = simple_strtou8(argv[1]);
        shell_print(shell, "%d", target_speed);
        motordriver_adjust_motor_speed_non_blocking(&motor1, target_speed);
    } else {
        shell_error(shell, "Usage: motor1 set-speed <0-100>");
    }
    return 0;
}

static int cmd_motor1_unsafe_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t speed = simple_strtou8(argv[1]);
        set_speed(&motor1, speed);
    } else {
        shell_error(shell, "Usage: motor1 Zset-speed <0-100> (unsafe)");
    }
    return 0;
}

static int cmd_motor1_get_speed(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor1.speed);
    return 0;
}

static int cmd_motor1_get_dir(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor1.direction);
    return 0;
}

static int cmd_motor1_get_motor(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                       "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                motor1.name, motor1.direction, motor1.speed, motor1.acceleration_rate,
                motor1.acceleration_rate_delay, motor1.braking_rate, motor1.braking_rate_delay);
    return 0;
}

static int cmd_motor1_config_acc_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t acceleration_rate = simple_strtou8(argv[1]);
        if (acceleration_rate != 0 && acceleration_rate < 100) {
            shell_print(shell, "%d", acceleration_rate);
            motor1.acceleration_rate = acceleration_rate;
        } else {
            shell_error(shell, "Invalid acceleration rate.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-acc-rate <0-100>");
    }
    return 0;
}

static int cmd_motor1_config_brak_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t braking_rate = simple_strtou8(argv[1]);
        if (braking_rate != 0 && braking_rate < 100) {
            shell_print(shell, "%d", braking_rate);
            motor1.braking_rate = braking_rate;
        } else {
            shell_error(shell, "Invalid braking rate.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-brak-rate <0-100>");
    }
    return 0;
}

static int cmd_motor1_config_acc_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t acceleration_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (acceleration_rate_delay != 0) {
            shell_print(shell, "%d", acceleration_rate_delay);
            motor1.acceleration_rate_delay = acceleration_rate_delay;
        } else {
            shell_error(shell, "Invalid acceleration rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-acc-rate-delay <ms>");
    }
    return 0;
}

static int cmd_motor1_config_brak_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t braking_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (braking_rate_delay != 0) {
            shell_print(shell, "%d", braking_rate_delay);
            motor1.braking_rate_delay = braking_rate_delay;
        } else {
            shell_error(shell, "Invalid braking rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-brak-rate-delay <ms>");
    }
    return 0;
}

void cmd_motor1_init() {
    LOG_INF("Adding motor1 commands.");
}

/* Creating subcommands (level 1 command) array for command "motor1". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_motor1,
                               SHELL_CMD(set-dir, NULL, "Set motor direction <dir[1||0]>", cmd_motor1_set_dir),
                               SHELL_CMD(set-speed, NULL, "Set motor speed <speed[0..100]>", cmd_motor1_set_speed),
                               SHELL_CMD(Zset-speed, NULL, "Unsafe set motor speed (direct PWM) <speed[0..100]>", cmd_motor1_unsafe_set_speed),
                               SHELL_CMD(get-speed, NULL, "Get motor speed", cmd_motor1_get_speed),
                               SHELL_CMD(get-dir, NULL, "Get motor direction", cmd_motor1_get_dir),
                               SHELL_CMD(get-motor, NULL, "Get motor configuration", cmd_motor1_get_motor),
                               SHELL_CMD(config-acc-rate, NULL, "Configure acceleration rate <rate[0..99]>", cmd_motor1_config_acc_rate),
                               SHELL_CMD(config-brak-rate, NULL, "Configure braking rate <rate[0..99]>", cmd_motor1_config_brak_rate),
                               SHELL_CMD(config-acc-rate-delay, NULL, "Configure acceleration rate delay <delay[0..0xFFFF]>", cmd_motor1_config_acc_rate_delay),
                               SHELL_CMD(config-brak-rate-delay, NULL, "Configure braking rate delay <delay[0..0xFFFF]>", cmd_motor1_config_brak_rate_delay),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(motor1, &sub_motor1,
                   "control motor1 of pico-pluto.",
                   cmd_motor1);
