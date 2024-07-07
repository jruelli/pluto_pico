#include <devicetree_generated.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "inc/pluto_mcp9808.h"
#include "inc/pluto_motordriver.h"
#include "inc/pluto_config.h"

LOG_MODULE_REGISTER(pluto_mcp9808, LOG_LEVEL_WRN);

static struct mcp9808_sensor sensors[] = {
        { DEVICE_DT_GET(DT_NODELABEL(mcp9808_0)), false, -1.0, false, -69.0 },
        { DEVICE_DT_GET(DT_NODELABEL(mcp9808_1)), false, -1.0, false, -69.0 },
        { DEVICE_DT_GET(DT_NODELABEL(mcp9808_2)), false, -1.0, false, -69.0 },
};

#define PLUTO_MCP9808_NUM_SENSORS (sizeof(sensors) / sizeof(sensors[0]))

static void double_to_string(double value, char *str, size_t str_size) {
    int integer_part = (int)value;
    int fractional_part = (int)((value - integer_part) * 100);
    snprintf(str, str_size, "%d.%02d", integer_part, fractional_part);
}

void mcp9808_thread(void) {
    while (1) {
        for (int i = 0; i < PLUTO_MCP9808_NUM_SENSORS; i++) {
            if (sensors[i].enabled) {
                struct sensor_value temp;
                int rc = sensor_sample_fetch(sensors[i].dev);
                if (rc != 0) {
                    LOG_ERR("Failed to fetch sample from sensor %d", i);
                    continue;
                }
                rc = sensor_channel_get(sensors[i].dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
                if (rc != 0) {
                    LOG_ERR("Failed to get temperature from sensor %d", i);
                    continue;
                }
                sensors[i].temperature = sensor_value_to_double(&temp);
            } else {
                // make sure an unenabled sensor will run always into threshold
                sensors[i].threshold = -69.0;
            }
            // Check threshold and perform special action if needed
            if (sensors[i].threshold_enabled && sensors[i].temperature > sensors[i].threshold) {
                char temp_str[16];
                double_to_string(sensors[i].temperature, temp_str, sizeof(temp_str));
                LOG_WRN("Threshold exceeded for sensor %d: %s C", i, temp_str);
                motordriver_stop_motors();
                k_sleep(K_SECONDS(PLUTO_MCP9808_THRESH_SLEEP_TIME_S));
            }
        }
        k_sleep(K_SECONDS(PLUTO_MCP9808_THREAD_SLEEP_TIME_S));
    }
}

K_THREAD_DEFINE(mcp9808_thread_id, PLUTO_MCP9808_THREAD_STACK_SIZE, mcp9808_thread, NULL, NULL, NULL,
                PLUTO_MCP9808_THREAD_PRIORITY, 0, 0);

static int cmd_mcp9808_list_sensors(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i < PLUTO_MCP9808_NUM_SENSORS; i++) {
        shell_print(shell, "Sensor %d: %s, Enabled: %s", i, sensors[i].dev->name, sensors[i].enabled ? "Yes" : "No");
    }
    return 0;
}

static int cmd_mcp9808_config_sensor(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 3) {
        shell_error(shell, "Usage: mcp9808 config-sensor <sensor_index> <e|d>");
        return -EINVAL;
    }
    int sensor_index = atoi(argv[1]);
    if (sensor_index < 0 || sensor_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid sensor index.");
        return -EINVAL;
    }
    bool enable = strcmp(argv[2], "e") == 0;
    sensors[sensor_index].enabled = enable;
    shell_print(shell, "mcp9808_%d %s", sensor_index, enable ? "enabled" : "disabled");
    return 0;
}

static int cmd_mcp9808_get_sensor(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Usage: mcp9808 get-temp <sensor_index>");
        return -EINVAL;
    }
    int sensor_index = atoi(argv[1]);
    if (sensor_index < 0 || sensor_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid sensor index.");
        return -EINVAL;
    }
    if (!sensors[sensor_index].enabled) {
        shell_print(shell, "-1");
        return 0;
    }
    struct sensor_value temp;
    int rc = sensor_sample_fetch(sensors[sensor_index].dev);
    if (rc != 0) {
        LOG_ERR("Failed to fetch sample from sensor %d", sensor_index);
        return rc;
    }
    rc = sensor_channel_get(sensors[sensor_index].dev, SENSOR_CHAN_AMBIENT_TEMP, &temp);
    if (rc != 0) {
        LOG_ERR("Failed to get temperature from sensor %d", sensor_index);
        return rc;
    }
    sensors[sensor_index].temperature = sensor_value_to_double(&temp);
    char temp_str[16];
    double_to_string(sensors[sensor_index].temperature, temp_str, sizeof(temp_str));
    shell_print(shell, "%s C", temp_str);
    return 0;
}

static int cmd_mcp9808_config_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 4) {
        shell_error(shell, "Usage: mcp9808 config-threshold <sensor_index> <e|d> <threshold_value>");
        return -EINVAL;
    }
    int sensor_index = atoi(argv[1]);
    if (sensor_index < 0 || sensor_index >= PLUTO_MCP9808_NUM_SENSORS) {
        shell_error(shell, "Invalid sensor index.");
        return -EINVAL;
    }
    bool enable = strcmp(argv[2], "e") == 0;
    double threshold = atof(argv[3]);
    sensors[sensor_index].threshold_enabled = enable;
    sensors[sensor_index].threshold = threshold;
    char temp_str[16];
    double_to_string(sensors[sensor_index].threshold, temp_str, sizeof(temp_str));
    shell_print(shell, "Threshold for mcp9808_%d %s with value %s", sensor_index, enable ? "enabled" : "disabled", temp_str);
    return 0;
}

void pluto_mcp9808_init() {
    LOG_INF("Initializing mcp9808 module");
    for (int i = 0; i < PLUTO_MCP9808_NUM_SENSORS; i++) {
        if (!device_is_ready(sensors[i].dev)) {
            LOG_WRN("Device %s is not ready.", sensors[i].dev->name);
        }
    }
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mcp9808,
                               SHELL_CMD(get-sensor, NULL, "Get temp value [°C] of temp sensor <sensor_index>.", cmd_mcp9808_get_sensor),
                               SHELL_CMD(config-sensor, NULL, "Enable/disable temp sensor <sensor_index>.", cmd_mcp9808_config_sensor),
                               SHELL_CMD(config-threshold, NULL, "Set threshold for temp sensor <sensor_index>.", cmd_mcp9808_config_threshold),
                               SHELL_CMD(list-sensors, NULL, "List all mcp9808 sensors.", cmd_mcp9808_list_sensors),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(mcp9808, &sub_mcp9808, "Control temperature sensors.", NULL);
