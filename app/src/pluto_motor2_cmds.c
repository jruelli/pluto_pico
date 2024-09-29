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
LOG_MODULE_REGISTER(motor2_cmds, LOG_LEVEL_WRN);

static int cmd_motor2(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

/* Subcommand implementations for motor2 */

static int cmd_motor2_set_dir(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        bool target_direction = simple_strtou8(argv[1]) != 0;
        shell_print(shell, "%d", target_direction);
        motordriver_set_dir(&motor2, target_direction);
    } else {
        shell_error(shell, "Usage: motor2 set-dir <0/1>");
    }
    return 0;
}

static int cmd_motor2_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t target_speed = simple_strtou8(argv[1]);
        shell_print(shell, "%d", target_speed);
        motordriver_adjust_motor_speed_non_blocking(&motor2, target_speed);
    } else {
        shell_error(shell, "Usage: motor2 set-speed <0-100>");
    }
    return 0;
}

static int cmd_motor2_unsafe_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t speed = simple_strtou8(argv[1]);
        set_speed(&motor2, speed);
    } else {
        shell_error(shell, "Usage: motor2 Zset-speed <0-100> (unsafe)");
    }
    return 0;
}

static int cmd_motor2_get_speed(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor2.speed);
    return 0;
}

static int cmd_motor2_get_dir(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor2.direction);
    return 0;
}

static int cmd_motor2_get_motor(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                       "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                motor2.name, motor2.direction, motor2.speed, motor2.acceleration_rate,
                motor2.acceleration_rate_delay, motor2.braking_rate, motor2.braking_rate_delay);
    return 0;
}

static int cmd_motor2_config_acc_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t acceleration_rate = simple_strtou8(argv[1]);
        if (acceleration_rate != 0 && acceleration_rate < 100) {
            shell_print(shell, "%d", acceleration_rate);
            motor2.acceleration_rate = acceleration_rate;
        } else {
            shell_error(shell, "Invalid acceleration rate.");
        }
    } else {
        shell_error(shell, "Usage: motor2 config-acc-rate <0-100>");
    }
    return 0;
}

static int cmd_motor2_config_brak_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t braking_rate = simple_strtou8(argv[1]);
        if (braking_rate != 0 && braking_rate < 100) {
            shell_print(shell, "%d", braking_rate);
            motor2.braking_rate = braking_rate;
        } else {
            shell_error(shell, "Invalid braking rate.");
        }
    } else {
        shell_error(shell, "Usage: motor2 config-brak-rate <0-100>");
    }
    return 0;
}

static int cmd_motor2_config_acc_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t acceleration_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (acceleration_rate_delay != 0) {
            shell_print(shell, "%d", acceleration_rate_delay);
            motor2.acceleration_rate_delay = acceleration_rate_delay;
        } else {
            shell_error(shell, "Invalid acceleration rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor2 config-acc-rate-delay <ms>");
    }
    return 0;
}

static int cmd_motor2_config_brak_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t braking_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (braking_rate_delay != 0) {
            shell_print(shell, "%d", braking_rate_delay);
            motor2.braking_rate_delay = braking_rate_delay;
        } else {
            shell_error(shell, "Invalid braking rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor2 config-brak-rate-delay <ms>");
    }
    return 0;
}

void cmd_motor2_init() {
    LOG_INF("Adding motor2 commands.");
}

/* Creating subcommands (level 1 command) array for command "motor2". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_motor2,
                               SHELL_CMD(set-dir, NULL, "Set motor direction <dir[1||0]>", cmd_motor2_set_dir),
                               SHELL_CMD(set-speed, NULL, "Set motor speed <speed[0..100]>", cmd_motor2_set_speed),
                               SHELL_CMD(Zset-speed, NULL, "Unsafe set motor speed (direct PWM) <speed[0..100]>", cmd_motor2_unsafe_set_speed),
                               SHELL_CMD(get-speed, NULL, "Get motor speed", cmd_motor2_get_speed),
                               SHELL_CMD(get-dir, NULL, "Get motor direction", cmd_motor2_get_dir),
                               SHELL_CMD(get-motor, NULL, "Get motor configuration", cmd_motor2_get_motor),
                               SHELL_CMD(config-acc-rate, NULL, "Configure acceleration rate <rate[0..99]>", cmd_motor2_config_acc_rate),
                               SHELL_CMD(config-brak-rate, NULL, "Configure braking rate <rate[0..99]>", cmd_motor2_config_brak_rate),
                               SHELL_CMD(config-acc-rate-delay, NULL, "Configure acceleration rate delay <delay[0..0xFFFF]>", cmd_motor2_config_acc_rate_delay),
                               SHELL_CMD(config-brak-rate-delay, NULL, "Configure braking rate delay <delay[0..0xFFFF]>", cmd_motor2_config_brak_rate_delay),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(motor2, &sub_motor2,
                   "control motor2 of pico-pluto.",
                   cmd_motor2);
