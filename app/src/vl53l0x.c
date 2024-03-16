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
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include "inc/vl53l0x.h"
#include "vl53l0x_types.h"
#include "vl53l0x_api.h"
#include "inc/usb_cli.h"
#include "inc/motordriver.h"

/* Enable logging for module. Change Log Level for debugging. */
LOG_MODULE_REGISTER(vl53l0x, LOG_LEVEL_INF);

#define VL53L0X_REG_WHO_AM_I                    0xC0
#define VL53L0X_CHIP_ID                         0xEEAA

#define SENSOR_POLL_INTERVAL K_MSEC(1000)
#define NUM_SENSORS 4

static struct k_thread vl53l0x_thread_data;
K_THREAD_STACK_DEFINE(vl53l0x_stack_area, 1024u);

K_SEM_DEFINE(data_sem, 1, 1); // Semaphore to protect shared data

struct vl53l0x {
    const char* name;
    uint16_t threshold;
    enum sensor_mode mode;
    bool is_ready_checked;
    uint32_t distance_mm;
    VL53L0X_Dev_t vl53l0x;
    bool is_proxy;
};

uint8_t set_threshold_by_name(const char* name, uint16_t threshold);
uint8_t get_threshold_by_name(const char* name);
uint32_t get_distance_by_name(const char* name);
enum sensor_mode get_mode_by_name(const char* name);
uint8_t set_mode_by_name(const char* name, enum sensor_mode mode);
const char* get_proxy_name(int proxy_number);
uint32_t get_is_proxy_state_by_name(const char* name);

// Define configurations and data for each sensor
struct vl53l0x vl53l0x_sensors[NUM_SENSORS] = {
        {"prox_0", 100, VL53L0X_MODE_OFF, false},
        {"prox_1", 100, VL53L0X_MODE_OFF, false},
        {"prox_2", 100, VL53L0X_MODE_OFF, false},
        {"prox_3", 100, VL53L0X_MODE_OFF, false}
};

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
static int cmd_proxy(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

static int cmd_proxy_set_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 3) {
        const char *name = argv[1];
        uint16_t threshold = simple_strtou16(argv[2]) != 0; // Convert to boolean
        set_threshold_by_name(name, threshold);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxy_get_threshold(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        uint16_t threshold = get_threshold_by_name(name);
        shell_print(shell, "%s state: %d", name, threshold);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxy_get_distance(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        uint32_t distance = get_distance_by_name(name);
        shell_print(shell, "%s distance: %d", name, distance);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}


static int cmd_proxy_get_proxy_state(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        bool is_proxy = get_is_proxy_state_by_name(name);
        shell_print(shell, "%s is_proxy: %d", name, is_proxy);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxy_set_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 3) {
        const char *name = argv[1];
        const char *mode = argv[2];
        enum sensor_mode sensor_mode = VL53L0X_MODE_PROXIMITY;
        if (strcmp(mode, "p") == 0) {
            sensor_mode = VL53L0X_MODE_PROXIMITY;
        } else if (strcmp(mode, "d") == 0) {
            sensor_mode = VL53L0X_MODE_DISTANCE;
        } else if (strcmp(mode, "o") == 0) {
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

static int cmd_proxy_get_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        const char *name = argv[1];
        enum sensor_mode mode = get_mode_by_name(name);
        const char *mode_str;
        switch (mode) {
            case VL53L0X_MODE_DISTANCE:
                mode_str = "Distance(d)";
                break;
            case VL53L0X_MODE_PROXIMITY:
                mode_str = "Proximity(p)";
                break;
            case VL53L0X_MODE_OFF:
                mode_str = "Off(o)";
                break;
            default:
                mode_str = "Unknown";
        }
        shell_print(shell, "%s mode: %s", name, mode_str);
    } else {
        shell_error(shell, "Invalid number of arguments for subcommand");
    }
    return 0;
}

static int cmd_proxy_list_prox(const struct shell *shell, size_t argc, char **argv) {
    for (int i = 0; i <= 3; i++) {
        shell_print(shell, "%s", get_proxy_name(i));
    }
    return 0;
}

const char* get_proxy_name(int proxy_number) {
    switch (proxy_number) {
        case 0: return "prox_0";
        case 1: return "prox_1";
        case 2: return "prox_2";
        case 3: return "prox_3";
        default: return "Unknown";
    }
}

uint8_t set_threshold_by_name(const char* name, uint16_t threshold) {
    LOG_DBG("Setting prox: %s to threshold: %u\n", name, threshold);
    if (strcmp(name, "prox_0") == 0) {
        vl53l0x_sensors[0].threshold = threshold;
    } else if (strcmp(name, "prox_1") == 0) {
        vl53l0x_sensors[1].threshold = threshold;
    } else if (strcmp(name, "prox_2") == 0) {
        vl53l0x_sensors[2].threshold = threshold;
    } else if (strcmp(name, "prox_3") == 0) {
        vl53l0x_sensors[3].threshold = threshold;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return 0;
}

uint8_t get_threshold_by_name(const char* name) {
    LOG_DBG("Getting threshold: of prox sensor %s\n", name);
    uint16_t threshold = 0;
    if (strcmp(name, "prox_0") == 0) {
        threshold = vl53l0x_sensors[0].threshold;
    } else if (strcmp(name, "prox_1") == 0) {
        threshold = vl53l0x_sensors[1].threshold;
    } else if (strcmp(name, "prox_2") == 0) {
        threshold = vl53l0x_sensors[2].threshold;
    } else if (strcmp(name, "prox_3") == 0) {
        threshold = vl53l0x_sensors[3].threshold;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return threshold;
}

uint8_t set_mode_by_name(const char* name, enum sensor_mode mode) {
    LOG_DBG("Setting prox: %s to mode: %u\n", name, mode);
    if (strcmp(name, "prox_0") == 0) {
        vl53l0x_sensors[0].mode = mode;
    } else if (strcmp(name, "prox_1") == 0) {
        vl53l0x_sensors[1].mode = mode;
    } else if (strcmp(name, "prox_2") == 0) {
        vl53l0x_sensors[2].mode = mode;
    } else if (strcmp(name, "prox_3") == 0) {
        vl53l0x_sensors[3].mode = mode;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return 0;
}

enum sensor_mode get_mode_by_name(const char* name) {
    LOG_DBG("Getting threshold: of prox sensor %s\n", name);
    enum sensor_mode state = 0;
    if (strcmp(name, "prox_0") == 0) {
        state = vl53l0x_sensors[0].mode;
    } else if (strcmp(name, "prox_1") == 0) {
        state = vl53l0x_sensors[1].mode;
    } else if (strcmp(name, "prox_2") == 0) {
        state = vl53l0x_sensors[2].mode;
    } else if (strcmp(name, "prox_3") == 0) {
        state = vl53l0x_sensors[3].mode;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return state;
}

uint32_t get_distance_by_name(const char* name) {
    uint32_t distance_mm = 0;
    if (strcmp(name, "prox_0") == 0) {
        distance_mm = vl53l0x_sensors[0].distance_mm;
    } else if (strcmp(name, "prox_1") == 0) {
        distance_mm = vl53l0x_sensors[1].distance_mm;
    } else if (strcmp(name, "prox_2") == 0) {
        distance_mm = vl53l0x_sensors[2].distance_mm;
    } else if (strcmp(name, "prox_3") == 0) {
        distance_mm = vl53l0x_sensors[3].distance_mm;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return distance_mm;
}

uint32_t get_is_proxy_state_by_name(const char* name) {
    bool is_proxy = 0;
    if (strcmp(name, "prox_0") == 0) {
        is_proxy = vl53l0x_sensors[0].is_proxy;
    } else if (strcmp(name, "prox_1") == 0) {
        is_proxy = vl53l0x_sensors[1].is_proxy;
    } else if (strcmp(name, "prox_2") == 0) {
        is_proxy = vl53l0x_sensors[2].is_proxy;
    } else if (strcmp(name, "prox_3") == 0) {
        is_proxy = vl53l0x_sensors[3].is_proxy;
    } else {
        LOG_ERR("prox sensor not known.");
    }
    return is_proxy;
}

void sensor_thread(void *unused1, void *unused2, void *unused3) {
    int ret;

    while (1) {
        k_sleep(SENSOR_POLL_INTERVAL);

        // Iterate through each sensor
        for (int i = 0; i < ARRAY_SIZE(vl53l0x_sensors); i++) {
            const struct device *vl53l0x;
            // find correct DT_Sensor
            if (strcmp(vl53l0x_sensors[i].name, "prox_0") == 0) {
                vl53l0x = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_0));
            } else if (strcmp(vl53l0x_sensors[i].name, "prox_1") == 0) {
                vl53l0x = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_1));
            } else if (strcmp(vl53l0x_sensors[i].name, "prox_2") == 0) {
                vl53l0x = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_2));
            } else if (strcmp(vl53l0x_sensors[i].name, "prox_3") == 0) {
                vl53l0x = DEVICE_DT_GET(DT_NODELABEL(vl53l0x_3));
            } else {
                LOG_ERR("Could not get device binding for %s", vl53l0x_sensors[i].name);
                continue; // Skip to the next sensor
            }
            // Skip to the next sensor if inactive
            if (vl53l0x_sensors[i].mode == VL53L0X_MODE_OFF) {
                continue;
            }
            if (vl53l0x_sensors[i].is_ready_checked == false) {
                if (!device_is_ready(vl53l0x)) {
                    LOG_ERR("sensor: device %s not ready.", vl53l0x_sensors[i].name);
                    continue;
                } else {
                    vl53l0x_sensors[i].is_ready_checked = true;
                }
            }
            ret = sensor_sample_fetch(vl53l0x);
            if (ret) {
                LOG_ERR("sensor_sample_fetch failed for %s, ret %d", vl53l0x_sensors[i].name, ret);
                continue; // Skip to the next sensor
            }
            struct sensor_value dist_value;
            ret = sensor_channel_get(vl53l0x, SENSOR_CHAN_DISTANCE, &dist_value);
            if (ret) {
                LOG_ERR("sensor_channel_get failed for %s, ret %d", vl53l0x_sensors[i].name, ret);
                continue; // Skip to the next sensor
            }
            k_sem_take(&data_sem, K_FOREVER); // Take semaphore before accessing shared data
            vl53l0x_sensors[i].distance_mm = (dist_value.val1 * 1000) + (dist_value.val2 / 1000);
            k_sem_give(&data_sem); // Give semaphore after accessing shared data
            // Skip to the next sensor if just measures distance
            if (vl53l0x_sensors[i].mode == VL53L0X_MODE_DISTANCE) {
                continue;
            }
            if (vl53l0x_sensors[i].distance_mm > vl53l0x_sensors[i].threshold) {
                vl53l0x_sensors[i].is_proxy = true;
                motordriver_stop_motors();
            } else {
                vl53l0x_sensors[i].is_proxy = false;
            }
        }
    }
}

void vl53l0x_init() {
    // Create sensor thread
    k_tid_t vl53l0x_tid = k_thread_create(&vl53l0x_thread_data, vl53l0x_stack_area,
                                          K_THREAD_STACK_SIZEOF(vl53l0x_stack_area),
                                          sensor_thread, NULL, NULL, NULL,
                                          K_PRIO_PREEMPT(7), 0, K_NO_WAIT);
    k_thread_start(vl53l0x_tid);
}

/* Creating subcommands (level 1 command) array for command "proxy". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_proxy,
                               SHELL_CMD(set-threshold, NULL, "Configure threshold for sensor <name> to <value[0..2000(mm)]>.",
                                         cmd_proxy_set_threshold),
                               SHELL_CMD(get-threshold, NULL, "Get current threshold of sensor <name>.",
                                         cmd_proxy_get_threshold),
                               SHELL_CMD(get-prox-state, NULL, "Get current proximity state of sensor <name>.",
                                         cmd_proxy_get_proxy_state),
                               SHELL_CMD(get-dis, NULL, "Get current distance of sensor <name>.",
                                         cmd_proxy_get_distance),
                               SHELL_CMD(get-mode, NULL,
                                         "Get conf for sensor <name>.",
                                         cmd_proxy_get_mode),
                               SHELL_CMD(set-mode, NULL,
                                         "Configure sensor <name> to distance (d) ,proximity measurement (p) "
                                         "or off (off) <[d||p||off]>.",
                                         cmd_proxy_set_mode),
                               SHELL_CMD(list-sensors, NULL, "List all sensors.",
                                         cmd_proxy_list_prox),
                               SHELL_SUBCMD_SET_END
);

/* Creating root (level 0) command "proxies" */
SHELL_CMD_REGISTER(proxy, &sub_proxy, "control/configure proximity sensors.", cmd_proxy);
