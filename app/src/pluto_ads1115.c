/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <devicetree_generated.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "inc/pluto_ads1115.h"

LOG_MODULE_REGISTER(pluto_ads1115, LOG_LEVEL_DBG);

static struct ads1115_input inputs[] = {
        { "a_0", false, -1.0 },
        { "a_1", false, -1.0 },
        { "a_2", false, -1.0 },
        { "a_3", false, -1.0 },
};

#define PLUTO_MCP9808_NUM_SENSORS (sizeof(inputs) / sizeof(inputs[0]))


static int cmd_ads1115_list_inputs(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i < PLUTO_MCP9808_NUM_SENSORS; i++) {
        shell_print(shell, "Input %d: %s, Enabled: %s", i, inputs[i].name, inputs[i].enabled ? "Yes" : "No");
    }
    return 0;
}

static int cmd_ads1115_get_input(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Usage: ads1115 get-input <input_index>");
        return -EINVAL;
    }
    return 0;
}

static int cmd_ads1115_config_input(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) {
        shell_error(shell, "Usage: ads1115 config-input <input_index> <e|d>");
        return -EINVAL;
    }
    int sensor_index = atoi(argv[1]);
    if (sensor_index < 0 || sensor_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid sensor index.");
        return -EINVAL;
    }
    bool enable = strcmp(argv[2], "e") == 0;
    inputs[sensor_index].enabled = enable;
    shell_print(shell, "ads1115_%d %s", sensor_index, enable ? "enabled" : "disabled");
    return 0;
}

void pluto_ads1115_init() {
    LOG_INF("Initializing ads1115 module");
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ads1115,
                               SHELL_CMD(get-input, NULL, "Get voltage of ads1115 input <input_index>.",
                                         cmd_ads1115_get_input),
                               SHELL_CMD(config-input, NULL, "Enable/disable ads1115 input <input_index>.",
                                         cmd_ads1115_config_input),
                               SHELL_CMD(list-inputs, NULL, "List all ads1115 inputs.",
                                         cmd_ads1115_list_inputs),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ads1115, &sub_ads1115, "Control ADS1115 Analog-digital-converter.", NULL);
