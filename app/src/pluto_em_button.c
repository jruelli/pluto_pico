/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "inc/pluto_em_button.h"
#include "inc/pluto_motordriver.h"
#include "inc/usb_cli.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(emergency_button, LOG_LEVEL_WRN);

static const struct gpio_dt_spec em_button_0 = GPIO_DT_SPEC_GET_OR(EM_BUTTON_0, gpios, {0});

static struct gpio_callback emergency_button_cb_data;
static bool motor_stop_enabled = false;

/* Function prototypes */
bool get_em_button_by_name(const char *name);

void emergency_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
    LOG_DBG("State of emergency button changed!");
    if (motor_stop_enabled) {
        motordriver_stop_motors();
    } else {
        LOG_DBG("Motor stop not enabled.");
    }
}

bool get_em_button_by_name(const char *name) {
    bool state = false;
    if (strcmp(name, "em_0") == 0) {
        state = gpio_pin_get(em_button_0.port, em_button_0.pin);
        LOG_DBG("pin: %d has state: %d", em_button_0.pin, state);
    } else {
        LOG_ERR("em_button not known.");
    }
    return state;
}

static int cmd_em_button(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

static int cmd_em_button_get_state(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 1) {
        bool state = get_em_button_by_name("em_0");
        shell_print(shell, "em_0 state: %d", state);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_em_button_activate(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Invalid number of arguments. Usage: em_button activate <0|1>");
        return -EINVAL;
    }
    uint8_t enable = simple_strtou8(argv[1]);
    if (enable == 0u) {
        motor_stop_enabled = false;
        shell_print(shell, "%d", enable);
    } else if (enable == 1u) {
        motor_stop_enabled = true;
        shell_print(shell, "%d", enable);
    } else {
        shell_error(shell, "Invalid argument. Use 0 to disable, 1 to enable.");
        return -EINVAL;
    }

    return 0;
}

void emergency_button_init() {
    int ret;
    gpio_pin_configure_dt(&em_button_0, EMERGENCY_BUTTON_FLAGS);
    ret = gpio_pin_interrupt_configure(em_button_0.port, em_button_0.pin, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        LOG_ERR("Error %d: failed to configure interrupt on %d pin", ret,  em_button_0.pin);
        return;
    }
    gpio_init_callback(&emergency_button_cb_data, emergency_button_pressed,
                       BIT(em_button_0.pin));
    gpio_add_callback(em_button_0.port, &emergency_button_cb_data);
}

/* Creating subcommands (level 1 command) array for command "relays". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_em_button,
                               SHELL_CMD(get, NULL, "Get state of emergency_button",
                                         cmd_em_button_get_state),
                               SHELL_CMD(config-mode,
                                         NULL,
                                         "Enable(1)/disable(0) motor stop on emergency button press.",
                                         cmd_em_button_activate),
                                         SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "relays" */
SHELL_CMD_REGISTER(em_btn, &sub_em_button, "configure emergency_button", cmd_em_button);
