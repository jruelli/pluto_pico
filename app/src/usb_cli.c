/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file usb_cli.c
 * @brief USB Command Line Interface (CLI) Module
 *
 * This module provides functions to set up and manage a USB CLI interface.
 * It handles USB communication with a host computer, including input/output
 * processing and USB interrupts. The module allows users to interact with
 * the device via a USB serial connection.
 *
 * @author Jannis Ruellmann
 * @license Apache-2.0
 */

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "inc/usb_cli.h"

/* Define global variables */
static const struct device *uart_dev;
static struct ring_buf ringbuf;
static bool rx_throttled;
static char rx_buf[MSG_SIZE];
static int rx_buf_pos;
K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);
static struct k_thread usb_cli_thread_data;
K_THREAD_STACK_DEFINE(usb_cli_thread_stack, USB_CLI_THREAD_STACK_SIZE);
uint8_t ring_buffer[RING_BUF_SIZE];

LOG_MODULE_REGISTER(usb_cli, LOG_LEVEL_INF);

/* Function prototypes */
static void usb_cli_thread(void);
static void usb_irq_cb(const struct device *dev, void *user_data);
void print_usb(const char *buf);

/**
 * @brief Initialize the USB CLI interface.
 *
 * This function initializes the USB CLI interface by enabling USB, waiting for
 * the Data Terminal Ready (DTR) signal to indicate that the USB serial connection
 * is established, and starting the USB CLI thread for input and output handling.
 * It also sets up the UART interrupt callback, configures the ring buffer, and
 * sends initial messages to the USB serial interface.
 */
void usb_cli_init(void) {
    uart_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

    /* Enable USB */
    if (usb_enable(NULL)) {
        return;
    }

    /* Wait for DTR signal */
    uint32_t dtr = 0;
    while (!dtr) {
        uart_line_ctrl_get(uart_dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }
    /* Initialize the ring buffer for UART data */
    ring_buf_init(&ringbuf, sizeof(ring_buffer), ring_buffer);
    /* Set up UART interrupt callback for USB CLI handling */
    uart_irq_callback_set(uart_dev, usb_irq_cb);
    uart_irq_rx_enable(uart_dev);

    /* Send initial welcome messages to USB serial interface */
    print_usb("Hello! I'm your echo bot.\r\n");
    print_usb("Tell me something and press enter:\r\n");

    /* Create and start the USB CLI thread */
    k_tid_t usb_cli_tid = k_thread_create(&usb_cli_thread_data, usb_cli_thread_stack,
                                          K_THREAD_STACK_SIZEOF(usb_cli_thread_stack), (k_thread_entry_t)usb_cli_thread, NULL, NULL, NULL,
                                          USB_CLI_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_start(usb_cli_tid);
}

/**
 * @brief USB CLI thread function.
 *
 */
 static void usb_cli_thread(void) {
    char tx_buf[MSG_SIZE];
    while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
        print_usb(tx_buf);
        print_usb("\r\n");
        // Split the input message into command and arguments
        char* command = strtok(tx_buf, " "); // Assuming space as a separator
        char* args = strtok(NULL, ""); // Get the rest of the message as arguments

        if (command != NULL) {
            // Check the command and execute corresponding functionality
            if (strcmp(command, "echo") == 0) {
                // Check if the "--help" argument is present
                if (args != NULL && strcmp(args, "--help") == 0) {
                    // Provide help message for the "echo" command
                    print_usb("Usage: echo <message>\r\n");
                    print_usb("  - Print the provided message.\r\n");
                } else {
                    // Execute the "echo" command
                    print_usb(args); // Print the message
                    print_usb("\r\n");
                }
            } else {
                // Command not recognized, provide an error message
                print_usb("Error: Command not recognized\r\n");
            }
        }
    }
}

/**
 * @brief USB IRQ callback function.
 *
 * This function is the interrupt service routine (ISR) callback for handling
 * USB-related UART interrupts. It manages the reception and transmission of
 * data between the device and a host computer over a USB serial connection.
 *
 * When called, this function checks for incoming data, processes it, and sends
 * outgoing data to maintain communication with the host. It also handles flow
 * control by throttling reception when the ring buffer is full and enabling
 * reception when it has space.
 *
 * @param dev       Pointer to the UART device associated with the USB serial
 *                  interface.
 * @param user_data Pointer to user-specific data (unused in this function).
 */
static void usb_irq_cb(const struct device *dev, void *user_data) {
    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (!rx_throttled && uart_irq_rx_ready(dev)) {
            int recv_len, rb_len;
            uint8_t c;
            size_t len = MIN(ring_buf_space_get(&ringbuf),
                             sizeof(c));
            if (len == 0) {
                /* Throttle because ring buffer is full */
                uart_irq_rx_disable(dev);
                rx_throttled = true;
                continue;
            }

            recv_len = uart_fifo_read(dev, &c, len);
            if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
                /* Terminate string */
                rx_buf[rx_buf_pos] = '\0';

                /* If queue is full, message is silently dropped */
                k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

                /* Reset the buffer (it was copied to the msgq) */
                rx_buf_pos = 0;
            } else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
                rx_buf[rx_buf_pos++] = c;
            }
            if (recv_len < 0) {
                LOG_ERR("Failed to read UART FIFO");
                recv_len = 0;
            };

            rb_len = ring_buf_put(&ringbuf, &c, recv_len);
            if (rb_len < recv_len) {
                LOG_ERR("Drop %u bytes", recv_len - rb_len);
            }
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
                LOG_DBG("Ring buffer empty, disable TX IRQ");
                uart_irq_tx_disable(dev);
                continue;
            }

            if (rx_throttled) {
                uart_irq_rx_enable(dev);
                rx_throttled = false;
            }
            // Write user data back as TX-data
            send_len = uart_fifo_fill(dev, buffer, rb_len);
            if (send_len < rb_len) {
                LOG_ERR("Drop %d bytes", rb_len - send_len);
            }
            LOG_DBG("ringbuf -> tty fifo %d bytes", send_len);
        }
    }
}

/**
 * @brief Print a null-terminated string to UART interface
 *
 */
void print_usb(const char *buf) {
    int msg_len = strlen(buf);
    for (int i = 0; i < msg_len; i++) {
        uart_poll_out(uart_dev, buf[i]);
    }
}
