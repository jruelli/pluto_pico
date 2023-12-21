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
 * It handles USB communication with a USB host, including input/output
 * processing and USB interrupts. The module allows users to interact with
 * the device via a USB serial connection.\n
 * The module is started via usb_cli_init() which will set up usb_cli_thread()
 * that will be used to process any incoming commands.
 *
 * @author Jannis Ruellmann
 */

#include <zephyr/sys/printk.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <app_version.h>

#include "inc/usb_cli.h"
#include "inc/relays.h"

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
static void handle_echo(char *args);
static void handle_version(char *args);
static void handle_relays(char *args);
uint8_t simple_strtou8(const char *str);

/* Global variables*/
static Command commands[] = {
        { "echo",
          "usage: echo <message> or echo --help\r\n",
          "description: print back the provided message\r\n",
          handle_echo
          },
        {
            "version",
            "usage: version or version --build-ver\r\n",
            "description: print back the version\r\n",
            handle_version
        },
        {
                "relays",
                "usage: relays --bin-value <X> | --relay-name <X> <state> | --get-relay-names | --help\r\n",
                "description: Control or query relays\r\n",
                handle_relays
        }
};

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
    print_usb("Hello! I'm pluto-pico!\r\n");
    print_usb("I support the following commands:\r\n");
    print_usb("version, echo\r\n");
    print_usb("Use arg --help to get description for commands\r\n");

    /* Create and start the USB CLI thread */
    k_tid_t usb_cli_tid = k_thread_create(&usb_cli_thread_data, usb_cli_thread_stack,
                                          K_THREAD_STACK_SIZEOF(usb_cli_thread_stack), (k_thread_entry_t)usb_cli_thread, NULL, NULL, NULL,
                                          USB_CLI_THREAD_PRIORITY, 0, K_NO_WAIT);
    k_thread_start(usb_cli_tid);
}

/**
 * @brief Find a command by name.
 *
 * @param name The name of the command to find.
 * @return A pointer to the command if found, NULL otherwise.
 */
static Command *find_command(const char *name) {
    size_t numberOfCommands = sizeof(commands) / sizeof(Command);
    for (size_t i = 0; i < numberOfCommands; i++) {
        if (strcmp(name, commands[i].name) == 0) {
            return &commands[i];
        }
    }
    return NULL;
}

/**
 * @brief USB CLI thread function.
 *
 * This function handles the USB CLI interface by processing incoming messages,
 * splitting them into commands and arguments, and executing the corresponding
 * command handlers. If a command is recognized, it invokes the associated handler
 * function. If the command is not recognized, it prints an error message.
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
            Command *cmd = find_command(command);
            if (cmd != NULL) {
                cmd->handler(args);
            } else {
                // Command not recognized, provide an error message
                print_usb("Error: Command not recognized\r\n");
            }
        }
    }
}

/**
 * @brief Command handler for the "echo" command.
 *
 * This function handles the "echo" command by printing the provided message
 * to the USB interface. If the "--help" argument is provided, it displays
 * usage and description information for the "echo" command.
 *
 * @param args The arguments provided with the "echo" command.
 */
static void handle_echo(char *args) {
    Command *cmd = find_command("echo");
    if ((args == NULL) || (strcmp(args, "--help") == 0)) {
        // Provide help message for the "echo" command
        print_usb(cmd->description);
        print_usb(cmd->usage);
    } else {
        // Execute the "echo" command
        print_usb(args); // Print the message
        print_usb("\r\n");
    }
}

/**
 * @brief Command handler for the "version" command.
 *
 * This function handles the "version" command by printing the application's
 * version and build information to the USB interface. If the "--help" argument
 * is provided, it displays usage and description information for the "version"
 * command.
 *
 * @param args The arguments provided with the "version" command.
 */
static void handle_version(char *args) {
    Command *cmd = find_command("version");
    if (args == NULL) {
        print_usb("App Version: ");
        print_usb(APP_VERSION_STRING);
        print_usb("\r\n");
    } else if (strcmp(args, "--help") == 0) {
        print_usb(cmd->description);
        print_usb(cmd->usage);
    } else if (strcmp(args, "--build-ver") == 0) {
        print_usb("App Build Version: ");
        print_usb(USB_CLI_X_STR(APP_BUILD_VERSION));
        print_usb("\r\n");
    } else {
        print_usb("Unknown parameter: ");
        print_usb("\r\n");

    }
}

static void handle_relays(char *args) {
    if (args == NULL || strcmp(args, "--help") == 0) {
        Command *cmd = find_command("relays");
        print_usb(cmd->description);
        print_usb(cmd->usage);
        return;
    }
    char *arg = strtok(args, " ");
    while (arg != NULL) {
        if (strcmp(arg, "--bin-value") == 0) {
            arg = strtok(NULL, " ");
            if (arg != NULL) {
                uint8_t value = simple_strtou8(arg);
                control_relays(value);
            }
        } else if (strcmp(arg, "--relay-name") == 0) {
            char *name = strtok(NULL, " ");
            char *state = strtok(NULL, " ");
            if (name != NULL && state != NULL) {
                bool state_val = (bool)simple_strtou8(state);
                control_relay_by_name(name, state_val);
            }
        } else if (strcmp(arg, "--get-relay-names") == 0) {
            for (int i = 0; i <= 7; i++) {
                print_usb(get_relay_name(i));
                print_usb("\r\n");
            }
        } else {
            print_usb("unknown argument\r\n");
        }
        arg = strtok(NULL, " ");
    }
}

uint8_t simple_strtou8(const char *str) {
    uint8_t result = 0;
    while (*str) {
        if (*str < '0' || *str > '9') {
            print_usb("not a digit.\r\n");
            break;
        }
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
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
