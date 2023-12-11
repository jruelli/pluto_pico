/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>

#include "inc/user_led.h"


BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_console), zephyr_cdc_acm_uart),
             "Console device is not ACM CDC UART device");

/* Stack size and thread data */
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(led_stack_area, STACK_SIZE);
struct k_thread led_thread_data;

int main(void) {
    const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return 0;
    }

    /* Poll if the DTR flag was set */
    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    printk("Version: 1.0\n");
    printk("Pluto_pico CLI tool\n");
    k_sleep(K_SECONDS(1));

    /* Initialize and start the LED thread */
    user_led_init();
    k_tid_t led_tid = k_thread_create(&led_thread_data, led_stack_area,
                                      K_THREAD_STACK_SIZEOF(led_stack_area),
                                      (k_thread_entry_t) user_led_thread,
                                      NULL, NULL, NULL,
                                      7, 0, K_NO_WAIT);

    while (1) {
        printk("Hello World! %s\n", CONFIG_ARCH);
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
