//
// Created by Jannis on 21.12.2023.
//
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <devicetree_generated.h>
#include <zephyr/sys/printk.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>

#include "inc/adafruit_ms_v2.h"

#define NODE_I2C DT_NODELABEL(i2c0)

uint32_t i2c_cfg = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;


void adafruit_ms_v2_init() {
    const struct device *dev = DEVICE_DT_GET(NODE_I2C);
    if (!device_is_ready(dev)) {
        printk("Device not ready\n");
        //LOG_ERR("Device not ready");
    }
    printk("Found i2c device!\n");
}