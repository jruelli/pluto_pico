/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file relays.c
 * @brief Relay Control Module
 *
 * This module provides a set of functions for controlling and querying the state
 * of relay modules in an embedded system. It includes capabilities to set the state
 * of individual relays or all relays simultaneously, retrieve the current state of
 * any relay, and interact with the relay module through a command-line interface.
 * The functions make use of GPIO (General-Purpose Input/Output) pins to manage
 * the relay states.
 *
 * This module facilitates the integration of relay control into larger systems,
 * allowing for effective management of power and signal control through relays.
 * It is designed to be easy to use and integrate into various embedded systems
 * requiring relay control functionality.
 *
 * Key functionalities include:
 * - Setting the state of individual or multiple relays.
 * - Querying the state of any relay.
 * - Command-line interface for relay control.
 * - Initialization and configuration of relay GPIO pins.
 *
 * The module is a part of a larger system and can be utilized in applications such
 * as home automation, industrial control, and other scenarios where relay control
 * is essential.
 *
 * @author Jannis Ruellmann
 */

#include <devicetree_generated.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <stdio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/shell/shell.h>
#include "inc/vl53l0x.h"
#include "vl53l0x_types.h"
#include "vl53l0x_api.h"
#include "inc/usb_cli.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(vl53l0x, LOG_LEVEL_INF);

#define VL53L0X_REG_WHO_AM_I                    0xC0
#define VL53L0X_CHIP_ID                         0xEEAA


struct vl53l0x_config {
    const char* name;
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec xshut;
    uint16_t threshold;
    enum sensor_mode mode;
};

struct vl53l0x_data {
    bool started;
    VL53L0X_Dev_t vl53l0x;
    VL53L0X_RangingMeasurementData_t RangingMeasurementData;
};
uint8_t set_threshold_by_name(const char* name, uint16_t threshold);
uint8_t get_threshold_by_name(const char* name);
uint32_t get_distance_by_name(const char* name);
enum sensor_mode get_mode_by_name(const char* name);
uint8_t set_mode_by_name(const char* name, enum sensor_mode mode);
const char* get_proxy_name(int proxy_number);

struct vl53l0x_config vl53l0x_config_0;
struct vl53l0x_data vl53l0x_data_0;

/**
 * @brief Root command function for relays.
 *
 * This function is called if a wrong subcommand has been selected.
 * This is a root command (level 0 command).
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_proxies(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

static int cmd_proxies_set_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 3) {
        const char *name = argv[1];
        uint16_t threshold = simple_strtou16(argv[2]) != 0; // Convert to boolean
        set_threshold_by_name(name, threshold);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxies_get_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        uint16_t threshold = get_threshold_by_name(name);
        shell_print(shell, "%s state: %d", name, threshold);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxies_get_distance(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        uint32_t distance = get_distance_by_name(name);
        shell_print(shell, "%s distance: %d", name, distance);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxies_set_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 3) {
        const char *name = argv[1];
        const char *mode = argv[2];
        enum sensor_mode sensor_mode = VL53L0X_MODE_PROXIMITY;
        if (strcmp(mode, "p") == 0) {
            sensor_mode = VL53L0X_MODE_PROXIMITY;
        } else if (strcmp(mode, "d") == 0) {
            sensor_mode = VL53L0X_MODE_DISTANCE;
        } else if (strcmp(mode, "d") == 0) {
            sensor_mode = VL53L0X_MODE_OFF;
        } else {
            shell_error(shell, "mode not known.");
        }
        set_mode_by_name(name, sensor_mode);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxies_get_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        enum sensor_mode mode = get_mode_by_name(name);
        shell_print(shell, "%s state: %d", name, mode);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxies_list_prox(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i <= 7; i++) {
        shell_print(shell, "%s", get_proxy_name(i));
    }
    return 0;
}

const char* get_proxy_name(int proxy_number) {
    switch (proxy_number) {
        case 0: return "proxy_0";
        case 1: return "proxy_1";
        case 2: return "proxy_2";
        case 3: return "proxy_3";
        default: return "Unknown";
    }
}

uint8_t set_threshold_by_name(const char* name, uint16_t threshold) {
    LOG_DBG("Setting prox: %s to threshold: %u\n", name, threshold);
    if (strcmp(name, "prox_0") == 0) {
        vl53l0x_config_0.threshold = threshold;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    // TODO Set threshold for device
    return 0;
}

uint8_t get_threshold_by_name(const char* name) {
    LOG_DBG("Getting threshold: of prox sensor %u\n", name);
    uint16_t threshold = 0;
    if (strcmp(name, "prox_0") == 0) {
        threshold = vl53l0x_config_0.threshold;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return threshold;
}

uint8_t set_mode_by_name(const char* name, enum sensor_mode mode) {
    LOG_DBG("Setting prox: %s to threshold: %u\n", name, threshold);
    if (strcmp(name, "prox_0") == 0) {
        vl53l0x_config_0.mode = mode;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    // TODO Set mode for device
    return 0;
}

enum sensor_mode get_mode_by_name(const char* name) {
    LOG_DBG("Getting threshold: of prox sensor %u\n", name);
    enum sensor_mode state = 0;
    if (strcmp(name, "prox_0") == 0) {
        state = vl53l0x_config_0.mode;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return state;
}

uint32_t get_distance_by_name(const char* name) {
    const struct device *vl53l0x;
    if (strcmp(name, "prox_0") == 0) {
        vl53l0x = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_0));
    } else {
        LOG_ERR("prox sensor not known.");
    }
    int ret;
    struct sensor_value dist_value;
    uint32_t distance_mm = 0;
    ret = sensor_channel_get(vl53l0x,
                             SENSOR_CHAN_DISTANCE,
                             &dist_value);
    if (ret) {
        LOG_ERR("sensor_sample_fetch failed for SENSOR_CHAN_DISTANCE ret %d", ret);
    } else {
        distance_mm = (dist_value.val1 * 1000) + (dist_value.val2 / 1000);
        LOG_INF("raw dis value: %d", distance_mm);
    }
    return distance_mm;
}

void vl53l0x_config_init() {
    vl53l0x_config_0.threshold = 100;
    vl53l0x_config_0.mode = VL53L0X_MODE_PROXIMITY;
    vl53l0x_config_0.name = "prox_0";
    // TODO Set threshold for device
}

int vl53l0x_test(void)
{
    const struct device *vl53l0x_0 = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_0));
    int ret;
    if (!device_is_ready(vl53l0x_0)) {
        LOG_ERR("sensor: device not ready.");
        return 0;
    }

    struct vl53l0x_data *drv_data = vl53l0x_0->data;
    VL53L0X_DeviceInfo_t vl53l0x_dev_info = { 0 };

    struct sensor_value prox_value;
    struct sensor_value dist_value;

    while (1) {
        ret = sensor_sample_fetch(vl53l0x_0);
        if (ret) {
            LOG_ERR("sensor_sample_fetch failed ret %d", ret);
        }
        ret = sensor_channel_get(vl53l0x_0, SENSOR_CHAN_PROX, &prox_value);
        if (ret) {
            LOG_ERR("sensor_sample_fetch failed for SENSOR_CHAN_PROX ret %d", ret);
        } else {
            //LOG_INF("prox is %d", prox_value.val1);
        }
        ret = sensor_channel_get(vl53l0x_0,
                                 SENSOR_CHAN_DISTANCE,
                                 &dist_value);
        if (ret) {
          LOG_ERR("sensor_sample_fetch failed for SENSOR_CHAN_DISTANCE ret %d", ret);
        } else {
          uint32_t distance_mm = (dist_value.val1 * 1000) + (dist_value.val2 / 1000);
          //LOG_INF("raw dis value: %d", distance_mm);
        }
        LOG_INF("vl53l0x is configured");
        ret = VL53L0X_GetDeviceInfo(&drv_data->vl53l0x, &vl53l0x_dev_info);
        if (ret < 0) {
            LOG_ERR("[%s] Could not get info from device.", vl53l0x_0->name);
            return -ENODEV;
        }

        LOG_INF("[%s] VL53L0X_GetDeviceInfo = %d", vl53l0x_0->name, ret);
        LOG_INF("   Device Name : %s", vl53l0x_dev_info.Name);
        LOG_INF("   Device Type : %s", vl53l0x_dev_info.Type);
        LOG_INF("   Device ID : %s", vl53l0x_dev_info.ProductId);
        LOG_INF("   ProductRevisionMajor : %d",
                vl53l0x_dev_info.ProductRevisionMajor);
        LOG_INF("   ProductRevisionMinor : %d",
                vl53l0x_dev_info.ProductRevisionMinor);
        uint16_t vl53l0x_id = 0U;
        ret = VL53L0X_RdWord(&drv_data->vl53l0x,
                             VL53L0X_REG_WHO_AM_I,
                             &vl53l0x_id);
        if ((ret < 0) || (vl53l0x_id != VL53L0X_CHIP_ID)) {
            LOG_ERR("[%s] Issue on device identification", vl53l0x_0->name);
            return -ENOTSUP;
        }
        k_sleep(K_MSEC(1000));
    }
    return 0;
}

/* Creating subcommands (level 1 command) array for command "proxies". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_proxies,
                               SHELL_CMD(set-threshold, NULL, "Configure threshold for sensor <name> to <value[0..2000(mm)]>.",
                                         cmd_proxies_set_threshold),
                               SHELL_CMD(get-threshold, NULL, "Get current threshold of sensor <name>.",
                                         cmd_proxies_get_threshold),
                               //SHELL_CMD(get-prox-state, NULL, "Get current proximity state of sensor <name>.",
                               //          cmd_proxies_get_prox_state),
                               SHELL_CMD(get-dis, NULL, "Get current distance of sensor <name>.",
                                         cmd_proxies_get_distance),
                               SHELL_CMD(get-mode, NULL,
                                         "Get conf for sensor <name>.",
                                         cmd_proxies_get_mode),
                               SHELL_CMD(set-mode, NULL,
                                         "Configure sensor <name> to distance (d) ,proximity measurement (p) "
                                         "or off (off) <[d||p||off]>.",
                                         cmd_proxies_set_mode),
                               SHELL_CMD(list-sensors, NULL, "List all sensors.",
                                         cmd_proxies_list_prox),
                               SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "proxies" */
SHELL_CMD_REGISTER(proxies, &sub_proxies, "control/configure proximity sensors.", cmd_proxies);
