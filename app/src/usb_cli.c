/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file user_led.c
 * @brief LED control module
 *
 * This module provides functions to initialize and control an LED. It includes
 * both initialization and a continuous control loop for LED state toggling.
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "inc/user_led.h"
#include "inc/usb_cli.h"
LOG_MODULE_REGISTER(usb_cli, LOG_LEVEL_INF);

#define RING_BUF_SIZE 1024
uint8_t ring_buffer[RING_BUF_SIZE];
struct ring_buf ringbuf;

static bool rx_throttled;

/* Stack size and thread data */
#define STACK_SIZE 1024
K_THREAD_STACK_DEFINE(led_stack_area, STACK_SIZE);
struct k_thread led_thread_data;

static void interrupt_handler(const struct device *dev, void *user_data) {
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (!rx_throttled && uart_irq_rx_ready(dev)) {
            int recv_len, rb_len;
            uint8_t buffer[64];
            size_t len = MIN(ring_buf_space_get(&ringbuf),
                             sizeof(buffer));

            if (len == 0) {
                /* Throttle because ring buffer is full */
                uart_irq_rx_disable(dev);
                rx_throttled = true;
                continue;
            }

            recv_len = uart_fifo_read(dev, buffer, len);
            if (recv_len < 0) {
                printk("Failed to read UART FIFO\n");
                LOG_ERR("Failed to read UART FIFO");
                recv_len = 0;
            };

            rb_len = ring_buf_put(&ringbuf, buffer, recv_len);
            if (rb_len < recv_len) {
                printk("Drop %u bytes\n", recv_len - rb_len);
                LOG_ERR("Drop %u bytes", recv_len - rb_len);
            }
            printk("tty fifo -> ringbuf %d bytes\n", rb_len);
            LOG_DBG("tty fifo -> ringbuf %d bytes", rb_len);
            if (rb_len) {
                uart_irq_tx_enable(dev);
            }
        }

        if (uart_irq_tx_ready(dev)) {
            uint8_t buffer[64];
            int rb_len, send_len;

            rb_len = ring_buf_get(&ringbuf, buffer, sizeof(buffer));
            if (!rb_len) {
                printk("Ring buffer empty, disable TX IRQ\n");
                LOG_DBG("Ring buffer empty, disable TX IRQ");
                uart_irq_tx_disable(dev);
                continue;
            }

            if (rx_throttled) {
                uart_irq_rx_enable(dev);
                rx_throttled = false;
            }

            send_len = uart_fifo_fill(dev, buffer, rb_len);
            if (send_len < rb_len) {
                printk("Drop %u bytes line 92\n", rb_len - send_len);
                LOG_ERR("Drop %d bytes", rb_len - send_len);
            }
            printk("ringbuf -> tty fifo %d bytes\n", send_len);
            LOG_DBG("ringbuf -> tty fifo %d bytes", send_len);
        }
    }
}

/**
 * @brief Initializes the USB CLI interface.
 *
 * This function sets up the USB device controller and waits for the Data Terminal
 * Ready (DTR) signal, indicating that the USB serial connection is established.
 */
void usb_cli_init(void) {
    const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint32_t dtr = 0;

    if (usb_enable(NULL)) {
        return;
    }

    while (!dtr) {
        uart_line_ctrl_get(dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
    /* Initialize and start the LED thread */
    user_led_init();
    k_tid_t led_tid = k_thread_create(&led_thread_data, led_stack_area,
                                      K_THREAD_STACK_SIZEOF(led_stack_area),
                                      (k_thread_entry_t) user_led_thread,
                                      NULL, NULL, NULL,
                                      7, 0, K_NO_WAIT);
    ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);
    uart_irq_callback_set(dev, interrupt_handler);
    uart_irq_rx_enable(dev);
}



/**
 * @brief USB CLI thread function.
 *
 * This thread manages the USB CLI interface, handling input and output.
 */
_Noreturn void usb_cli_thread(void) {
    const struct device *dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    uint8_t buffer[64];
    int rb_len;

    while (1) {
        rb_len = ring_buf_get(&ringbuf, buffer, sizeof(buffer));
        if (rb_len) {
            printk("yo i got something.\n");
            for (int i = 0; i < rb_len; ++i) {
                if (buffer[i] == '\n' || buffer[i] == '\r') {
                    buffer[i] = '\0'; // Replace newline/carriage return with null terminator

                    // Check for the "version" command
                    if (strcmp(buffer, "version") == 0) {
                        const char *response = "huhu\n";
                        uart_fifo_fill(dev, response, strlen(response));
                        printk("wuhu");
                    }
                    memset(buffer, 0, sizeof(buffer)); // Clear the buffer
                    break;
                }
            }
        }
        k_sleep(K_MSEC(100));
    }
}

