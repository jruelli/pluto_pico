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
#include "inc/vl53l0x.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(vl53l0x, LOG_LEVEL_INF);

int vl53l0x_test(void)
{
    const struct device *vl53l0x_0 = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_0));
    int ret;
    if (!device_is_ready(vl53l0x_0)) {
        LOG_ERR("sensor: device not ready.");
        return 0;
    }
    LOG_INF("vl53l0x is configured");
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
          LOG_INF("raw dis value: %d", distance_mm);
        }
        k_sleep(K_MSEC(1000));
    }
    return 0;
}
