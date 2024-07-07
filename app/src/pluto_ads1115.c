#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "inc/pluto_ads1115.h"
#include "inc/ads1115.h"
#include "inc/pluto_motordriver.h"

LOG_MODULE_REGISTER(pluto_ads1115, LOG_LEVEL_WRN);

ADS1115 ads1115;

static struct ads1115_input inputs[] = {
        { "a_0", false, -1.0, false, 0.0 },
        { "a_1", false, -1.0, false, 0.0 },
        { "a_2", false, -1.0, false, 0.0 },
        { "a_3", false, -1.0, false, 0.0 },
};

#define PLUTO_MCP9808_NUM_SENSORS (sizeof(inputs) / sizeof(inputs[0]))

static void double_to_string(double value, char *str, size_t str_size) {
    int integer_part = (int)value;
    int fractional_part = (int)((value - integer_part) * 1000000);
    snprintf(str, str_size, "%d.%06d", integer_part, fractional_part);
}

void ads1115_thread(void) {
    while (1) {
        for (int i = 0; i < PLUTO_MCP9808_NUM_SENSORS; i++) {
            double input = -1.0;
            if (inputs[i].enabled) {
                switch (i) {
                    case 0:
                        input = (double) ADS1115_readADC(&ads1115, CH_0);
                        break;
                    case 1:
                        input = (double) ADS1115_readADC(&ads1115, CH_1);
                        break;
                    case 2:
                        input = (double) ADS1115_readADC(&ads1115, CH_2);
                        break;
                    case 3:
                        input = (double) ADS1115_readADC(&ads1115, CH_3);
                        break;
                    default:
                        input = -1.0;
                        break;
                }
                inputs[i].voltage = input;
            }
            // Check threshold and perform special action if needed
            if (inputs[i].threshold_enabled && input < inputs[i].threshold) {
                // Perform special action
                char vol_str[16];
                double_to_string(input, vol_str, sizeof(vol_str));
                LOG_WRN("Threshold exceeded for input %d: %s V", i, vol_str);
                motordriver_stop_motors();
                k_sleep(K_SECONDS(10));
            }
        }
        k_sleep(K_SECONDS(5));
    }
}

K_THREAD_DEFINE(ads1115_thread_id, 1024, ads1115_thread, NULL, NULL, NULL, 7, 0, 0);

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
    int input_index = atoi(argv[1]);
    if (input_index < 0 || input_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid input index.");
        return -EINVAL;
    }
    if (!inputs[input_index].enabled) {
        shell_print(shell, "-1");
        return 0;
    }
    char vol_str[16];
    double_to_string(inputs[input_index].voltage, vol_str, sizeof(vol_str));
    shell_print(shell, "%d: %s V", input_index, vol_str);
    return 0;
}

static int cmd_ads1115_config_input(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) {
        shell_error(shell, "Usage: ads1115 config-input <input_index> <e|d>");
        return -EINVAL;
    }
    int input_index = atoi(argv[1]);
    if (input_index < 0 || input_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid input index.");
        return -EINVAL;
    }
    bool enable = strcmp(argv[2], "e") == 0;
    inputs[input_index].enabled = enable;
    shell_print(shell, "ads1115_%d %s", input_index, enable ? "enabled" : "disabled");
    return 0;
}

static int cmd_ads1115_config_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 4) {
        shell_error(shell, "Usage: ads1115 config-threshold <input_index> <e|d> <threshold_value>");
        return -EINVAL;
    }

    int input_index = atoi(argv[1]);
    if (input_index < 0 || input_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid input index.");
        return -EINVAL;
    }

    bool enable = strcmp(argv[2], "e") == 0;
    double threshold = atof(argv[3]);

    inputs[input_index].threshold_enabled = enable;
    inputs[input_index].threshold = threshold;
    char thr_str[16];
    double_to_string(inputs[input_index].threshold, thr_str, sizeof(thr_str));
    shell_print(shell, "Threshold for ads1115_%d %s with value %s", input_index, enable ? "enabled" : "disabled", thr_str);
    return 0;
}

void pluto_ads1115_init() {
    LOG_INF("Initializing ads1115 module");
    ADS1115_init(&ads1115);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_ads1115,
                               SHELL_CMD(get-input, NULL, "Get voltage of ads1115 input <input_index>.", cmd_ads1115_get_input),
                               SHELL_CMD(config-input, NULL, "Enable/disable ads1115 input <input_index>.", cmd_ads1115_config_input),
                               SHELL_CMD(config-threshold, NULL, "Set threshold for ads1115 input <input_index>.", cmd_ads1115_config_threshold),
                               SHELL_CMD(list-inputs, NULL, "List all ads1115 inputs.", cmd_ads1115_list_inputs),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ads1115, &sub_ads1115, "Control ADS1115 Analog-digital-converter.", NULL);
