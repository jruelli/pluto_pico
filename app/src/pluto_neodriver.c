/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/logging/log.h>
#include <zephyr/devicetree_generated.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/shell/shell.h>
#include <stdlib.h>

#include "inc/pluto_neodriver.h"
#include "inc/pluto_config.h"


LOG_MODULE_REGISTER(pluto_neodriver, LOG_LEVEL_WRN);

#define NEODRIVER_NODE DT_NODELABEL(neodriver)
#define NEODRIVER_I2C_ADDR DT_REG_ADDR(NEODRIVER_NODE)
#define NEOPIXEL_PIN 15

static struct pluto_neodriver driver;
static uint16_t max_led_index = 120;
static uint8_t animation_mode = 0;

int neodriver_init(void) {
    driver.i2c_dev = DEVICE_DT_GET(DT_BUS(NEODRIVER_NODE));
    if (!driver.i2c_dev) {
        LOG_ERR("I2C: Device driver not found");
        return -ENODEV;
    }
    driver.i2c_addr = NEODRIVER_I2C_ADDR;
    if (!device_is_ready(driver.i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }
    uint8_t buf[2] = { SEESAW_NEOPIXEL_PIN, NEOPIXEL_PIN };
    int ret = i2c_write(driver.i2c_dev, buf, sizeof(buf), driver.i2c_addr);
    if (ret) {
        LOG_ERR("Failed to set Neopixel pin");
        return ret;
    }
    neodriver_set_all_colors(0,0,0, 0);
    // Update the strip
    neodriver_show();
    return 0;
}

int neodriver_set_color(uint16_t led_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    uint8_t buf[7];
    buf[0] = SEESAW_NEOPIXEL_BUF;
    buf[1] = (led_index >> 8) & 0xFF;
    buf[2] = led_index & 0xFF;
    buf[3] = red;
    buf[4] = green;
    buf[5] = blue;
    buf[6] = white;
    int ret = i2c_write(driver.i2c_dev, buf, sizeof(buf), driver.i2c_addr);
    if (ret) {
        LOG_ERR("Failed to set Neopixel color");
    }
    return ret;
}

int neodriver_set_all_colors(uint8_t red, uint8_t green, uint8_t blue, uint8_t white) {
    animation_mode = 0; // Reset animation mode to 0
    for (uint16_t i = 0; i < max_led_index; i++) {
        int ret = neodriver_set_color(i, red, green, blue, white);
        if (ret) {
            return ret;
        }
    }
    return 0;
}

int neodriver_show(void) {
    uint8_t cmd = SEESAW_NEOPIXEL_SHOW;
    return i2c_write(driver.i2c_dev, &cmd, 1, driver.i2c_addr);
}

void running_light_animation(void) {
    for (uint16_t i = 0; i < max_led_index; i++) {
        neodriver_set_all_colors(0, 0, 0, 0); // Clear all LEDs
        neodriver_set_color(i, 255, 0, 0, 0); // Set current LED to red
        neodriver_show();
        k_msleep(PLUTO_NEOPIXEL_THREAD_SLEEP_TIME_MS); // Delay for smooth animation
    }
}

_Noreturn void neodriver_thread(void) {
    while (1) {
        if (animation_mode == 1) {
            running_light_animation();
        }
        k_msleep(PLUTO_NEOPIXEL_THREAD_SLEEP_TIME_MS);
    }
}

/* Define the Neodriver thread */
K_THREAD_DEFINE(neodriver_thread_id, PLUTO_NEOPIXEL_THREAD_STACK_SIZE, neodriver_thread, NULL, NULL, NULL,
                PLUTO_NEOPIXEL_THREAD_PRIORITY, 0, 0);

int cmd_neodriver_set_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Usage: set-animation-mode <0|1>");
        return -EINVAL;
    }
    uint8_t mode = atoi(argv[1]);
    if (mode > 1) {
        shell_error(shell, "Invalid mode. Must be 0 or 1.");
        return -EINVAL;
    }
    animation_mode = mode;
    shell_print(shell, "%d", animation_mode);
    return 0;
}

int cmd_neodriver_get_mode(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 1) {
        shell_error(shell, "Usage: get-animation-mode");
        return -EINVAL;
    }
    shell_print(shell, "%d", animation_mode);
    return 0;
}

int cmd_neodriver_config_led_index(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        shell_error(shell, "Usage: set-max-led-index <value>");
        return -EINVAL;
    }
    uint16_t value = atoi(argv[1]);
    if (value > 0 && value < 171 ) {
        max_led_index = value;
        shell_print(shell, "Max LED index set to %d", max_led_index);
    } else {
        shell_error(shell, "Invalid value. Must be between 1 .. 170.");
        return -EINVAL;
    }
    return 0;
}

int cmd_neodriver_update_one_color(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 6) {
        shell_error(shell, "Usage: set-one-color <index> <red> <green> <blue> <white>");
        return -EINVAL;
    }
    uint8_t index = atoi(argv[1]);
    uint8_t red = atoi(argv[2]);
    uint8_t green = atoi(argv[3]);
    uint8_t blue = atoi(argv[4]);
    uint8_t white = atoi(argv[5]);
    int ret = -1;
    if (index > max_led_index) {
        shell_error(shell, "given index greater than max_led_index");
        return ret;
    }
    LOG_DBG("Setting one led");
    // Reset animation mode to 0
    animation_mode = 0;
    ret = neodriver_set_color(index, red, green, blue, white);
    if (ret) {
        shell_error(shell, "Failed to set colors for LED %d", index);
        return ret;
    }
    ret = neodriver_show();
    if (ret) {
        shell_error(shell, "Failed to update LED %d", index);
        return ret;
    }
    LOG_DBG("Done setting one led");
    shell_print(shell, "LED %d updated to color (%d, %d, %d, %d)", index, red, green, blue, white);
    return 0;
}

int cmd_neodriver_update_all_colors(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 5) {
        shell_error(shell, "Usage: set-all-colors <red> <green> <blue> <white>");
        return -EINVAL;
    }
    uint8_t red = atoi(argv[1]);
    uint8_t green = atoi(argv[2]);
    uint8_t blue = atoi(argv[3]);
    uint8_t white = atoi(argv[4]);
    LOG_DBG("Setting all led");
    int ret = neodriver_set_all_colors(red, green, blue, white);
    if (ret) {
        shell_error(shell, "Failed to set colors for all LEDs");
        return ret;
    }
    ret = neodriver_show();
    if (ret) {
        shell_error(shell, "Failed to update LEDs");
        return ret;
    }
    LOG_DBG("Done setting all led");
    shell_print(shell, "All LEDs updated to color (%d, %d, %d, %d)", red, green, blue, white);
    return 0;
}

/* Shell commands */
/* Shell commands */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_neodriver,
                               SHELL_CMD_ARG(config-led-index, NULL, "Set the maximum LED index to <index>.",
                                             cmd_neodriver_config_led_index, 2, 0),
                               SHELL_CMD_ARG(set-one-color, NULL, "Update the colors of one LED <index> <r> <g> <b> <w>.",
                                             cmd_neodriver_update_one_color, 6, 0),
                               SHELL_CMD_ARG(set-all-colors, NULL, "Update the colors of all LEDs <r> <g> <b> <w>.",
                                             cmd_neodriver_update_all_colors, 5, 0),
                               SHELL_CMD_ARG(set-animation-mode, NULL, "Set the animation mode <0|1>.",
                                             cmd_neodriver_set_mode, 2, 0),
                               SHELL_CMD_ARG(get-animation-mode, NULL, "Get the animation mode.",
                                             cmd_neodriver_get_mode, 1, 0),
                               SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(neodriver, &sub_neodriver, "Neodriver commands", NULL);
