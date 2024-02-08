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
    enum proximity_state state;
};

struct vl53l0x_data {
    bool started;
    VL53L0X_Dev_t vl53l0x;
    VL53L0X_RangingMeasurementData_t RangingMeasurementData;
};
uint8_t set_threshold_by_name(const char* name, uint16_t threshold);
uint8_t get_threshold_by_name(const char* name);
bool get_proxy_by_name(const char *name);

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
    if (argc == 3) {
        const char *name = argv[1];
        uint16_t threshold = get_threshold_by_name(name);
        shell_print(shell, "%s state: %d", name, threshold);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
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

void vl53l0x_config_init() {
    vl53l0x_config_0.threshold = 100;
    vl53l0x_config_0.state = PROXIMITY_STATE_PROXIMITY;
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

    LOG_INF("vl53l0x is configured");

    struct vl53l0x_data *drv_data = vl53l0x_0->data;
    VL53L0X_DeviceInfo_t vl53l0x_dev_info = { 0 };

    struct sensor_value prox_value;
    struct sensor_value dist_value;
    struct sensor_value value;

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
                               //          cmd_proxies_set_prox_state),
                               //SHELL_CMD(get-conf, NULL,
                               //          "Get conf for sensor <name>.",
                               //          cmd_proxies_get_prox_conf),
                               //SHELL_CMD(set-conf, NULL,
                               //          "Configure sensor <name> to distance (d) ,proximity measurement (p) "
                               //          "or off (off) <[d||p||off]>.",
                               //          cmd_proxies_set_prox_conf),
                               //SHELL_CMD(get-details, NULL, "Get details of sensor <name>.",
                               //          cmd_proxies_get_details),
                               //SHELL_CMD(list-sensors, NULL, "List all sensors.",
                               //          cmd_proxies_list_prox),
                               SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "proxies" */
SHELL_CMD_REGISTER(proxies, &sub_proxies, "control/configure proximity sensors.", cmd_proxies);
