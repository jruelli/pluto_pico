/*
 * Copyright (c) Jannis Ruellmann 2023
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file motordriver.c
 * @brief Motor Control Module
 *
 * This module provides a set of functions for controlling and querying the state
 * of motor drivers in an embedded system. It includes capabilities to set the speed
 * and direction of individual motors, retrieve the current state of motors, and
 * interact with the motor control system through a command-line interface.
 * The functions make use of PWM (Pulse Width Modulation) and GPIO (General-Purpose Input/Output)
 * pins to manage motor control.
 *
 * This module facilitates the integration of motor control into larger systems,
 * allowing for effective management of mechanical movements and automation tasks.
 * It is designed to be easy to use and integrate into various embedded systems
 * requiring motor control functionality.
 *
 * Key functionalities include:
 * - Setting the speed and direction of motors.
 * - Querying the state of motors.
 * - Command-line interface for motor control.
 * - Initialization and configuration of motor control hardware.
 *
 * The module is a part of a larger system and can be utilized in applications such
 * as robotics, automation, and other scenarios where precise motor control is essential.
 *
 * @author Jannis Ruellmann
 */

#include <sys/cdefs.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "inc/motordriver.h"
#include "inc/usb_cli.h"

// Function declarations
void init_motor(motor_t* motor);
void set_speed(motor_t* motor, uint32_t speed_percent);
void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id);
void set_motors(motor_t *motor1, motor_t *motor2, uint32_t speed1, uint32_t speed2, bool dir1, bool dir2);
void motordriver_set_dir(motor_t* motor, bool dir);
void motordriver_adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed);
void motordriver_adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed);

static const struct pwm_dt_spec pwm_1 = PWM_DT_SPEC_GET_OR(PWM_1, {0});
static const struct pwm_dt_spec pwm_2 = PWM_DT_SPEC_GET_OR(PWM_2, {0});

static const struct gpio_dt_spec dir_1 = GPIO_DT_SPEC_GET_OR(DIR_1, gpios, {0});
static const struct gpio_dt_spec dir_2 = GPIO_DT_SPEC_GET_OR(DIR_2, gpios,{0});

// Define global mutexes and timers
struct k_mutex motor1_mutex;
struct k_timer motor1_timer;

struct k_mutex motor2_mutex;
struct k_timer motor2_timer;

motor_t motor1 = {
        .name = "motor1",
        .dir_pin = dir_1,
        .pwm_spec = pwm_1,
        .emergency_stop = 0,
        .direction = 0,
        .target_direction = 0,
        .speed = 0,
        .target_speed = 0,
        .acceleration_rate = 10,
        .acceleration_rate_delay = 100,
        .braking_rate = 10,
        .braking_rate_delay = 100,
        .mutex = &motor1_mutex,
        .timer = &motor1_timer,
};

motor_t motor2 = {
        .name = "motor2",
        .dir_pin = dir_2,
        .pwm_spec = pwm_2,
        .emergency_stop = 0,
        .direction = 0,
        .target_direction = 0,
        .speed = 0,
        .target_speed = 0,
        .acceleration_rate = 10,
        .acceleration_rate_delay = 100,
        .braking_rate = 10,
        .braking_rate_delay = 100,
        .mutex = &motor2_mutex,
        .timer = &motor2_timer,
};

/**
 * @brief Root command function for motor1.
 *
 * This function is called if a wrong subcommand has been selected.
 * This is a root command (level 0 command).
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_motor1(const struct shell *shell, size_t argc, char **argv) {
    shell_error(shell, "Invalid subcommand or number of arguments.");
    return 0;
}

/**
 * @brief Shell command function to control and query motor2.
 *
 * This function is designed to be used as a shell command for controlling
 * and querying the state of motor1. It supports multiple sub-commands for
 * setting motor direction, speed, and querying motor status.
 *
 * **Usage**\n
 *     motor2 set-dir <0/1>                 // Sets the direction of motor\n
 *     motor2 set-speed <0-100>             // Sets the speed of motor (0..100)\n
 *     motor2 Zset-speed <0-100> (unsafe)   // UNSAFE Directly sets the PWM speed of motor (0..100)\n
 *     motor2 get-speed                     // Gets the current speed of motor\n
 *     motor2 get-dir                       // Gets the current direction of motor\n
 *     motor2 get-motor                     // Gets the current configuration of motor\n
 *     motor2 config-acc-rate <0-100>       // Configures the acceleration rate (1..99)\n
 *     motor2 config-brak-rate <0-100>      // Configures the braking rate (1..99)\n
 *     motor2 config-acc-rate-delay <ms>    // Configures the acceleration rate delay (1..0xFF)\n
 *     motor2 config-brak-rate-delay <ms>   // Configures the braking rate delay (1..0xFF)\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_motor2(const struct shell *shell, size_t argc, char **argv) {
    if (argc > 1) {
        if (strcmp(argv[1], "set-dir") == 0) {
            bool target_direction = simple_strtou8(argv[2]) != 0;
            motordriver_set_dir(&motor2, target_direction);
        } else if (strcmp(argv[1], "set-speed") == 0) {
            uint32_t target_speed = simple_strtou8(argv[2]);
            motordriver_adjust_motor_speed_non_blocking(&motor2, target_speed);
        } else if (strcmp(argv[1], "Zset-speed") == 0) {
            uint32_t speed = simple_strtou8(argv[2]);
            set_speed(&motor2, speed);
        } else if (strcmp(argv[1], "get-speed") == 0) {
            shell_print(shell, "%d", motor2.speed);
        } else if (strcmp(argv[1], "get-dir") == 0) {
            shell_print(shell, "%d", motor2.direction);
        } else if (strcmp(argv[1], "get-motor") == 0) {
            shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                               "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                        motor2.name, motor2.direction, motor2.speed, motor2.acceleration_rate,
                        motor2.acceleration_rate_delay, motor2.braking_rate, motor2.braking_rate_delay);
        } else if (strcmp(argv[1], "config-acc-rate") == 0) {
            uint32_t acceleration_rate = simple_strtou8(argv[2]);
            if (acceleration_rate != 0 && (acceleration_rate < 100)) {
                motor2.acceleration_rate = acceleration_rate;
            } else {
                shell_print(shell, "Invalid acceleration_rate.");
            }
        } else if (strcmp(argv[1], "config-brak-rate") == 0) {
            uint32_t braking_rate = simple_strtou8(argv[2]);
            if (braking_rate != 0 && (braking_rate < 100)) {
                motor2.braking_rate = braking_rate;
            } else {
                shell_print(shell, "Invalid braking_rate.");
            }
        } else if (strcmp(argv[1], "config-acc-rate-delay") == 0) {
            int32_t acceleration_rate_delay = (int32_t)simple_strtou32(argv[2]);
            if (acceleration_rate_delay != 0) {
                motor2.acceleration_rate_delay = acceleration_rate_delay;
            }
        } else if (strcmp(argv[1], "config-brak-rate-delay") == 0) {
            int32_t braking_rate_delay = (int32_t)simple_strtou32(argv[2]);
            if (braking_rate_delay != 0) {
                motor2.braking_rate_delay = braking_rate_delay;
            }

/* Subcommand implementations motor1 */

static int cmd_motor1_set_dir(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        bool target_direction = simple_strtou8(argv[1]) != 0;
        motordriver_set_dir(&motor1, target_direction);
    } else {
        shell_error(shell, "Usage: motor1 set-dir <0/1>");
    }
    return 0;
}

static int cmd_motor1_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t target_speed = simple_strtou8(argv[1]);
        motordriver_adjust_motor_speed_non_blocking(&motor1, target_speed);
    } else {
        shell_error(shell, "Usage: motor1 set-speed <0-100>");
    }
    return 0;
}

static int cmd_motor1_unsafe_set_speed(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t speed = simple_strtou8(argv[1]);
        set_speed(&motor1, speed);
    } else {
        shell_error(shell, "Usage: motor1 Zset-speed <0-100> (unsafe)");
    }
    return 0;
}

static int cmd_motor1_get_speed(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor1.speed);
    return 0;
}

static int cmd_motor1_get_dir(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "%d", motor1.direction);
    return 0;
}

static int cmd_motor1_get_motor(const struct shell *shell, size_t argc, char **argv) {
    shell_print(shell, "name: %s\ndirection: %d\nspeed: %d\nacceleration_rate: %d\n"
                       "acceleration_rate_delay: %dms\nbraking_rate: %d\nbraking_rate_delay: %dms",
                motor1.name, motor1.direction, motor1.speed, motor1.acceleration_rate,
                motor1.acceleration_rate_delay, motor1.braking_rate, motor1.braking_rate_delay);
    return 0;
}

static int cmd_motor1_config_acc_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t acceleration_rate = simple_strtou8(argv[1]);
        if (acceleration_rate != 0 && acceleration_rate < 100) {
            motor1.acceleration_rate = acceleration_rate;
        } else {
            shell_error(shell, "Invalid acceleration rate.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-acc-rate <0-100>");
    }
    return 0;
}

static int cmd_motor1_config_brak_rate(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        uint32_t braking_rate = simple_strtou8(argv[1]);
        if (braking_rate != 0 && braking_rate < 100) {
            motor1.braking_rate = braking_rate;
        } else {
            shell_error(shell, "Invalid braking rate.");
        }
    } else {
        shell_print(shell, "Usage: motor2 set-dir <0/1>"
                           "| motor2 set-speed <0-100>"
                           "| motor2 Zset-speed <0-100> (unsafe)"
                           "| motor2 get-dir"
                           "| motor2 get-speed"
                           "| motor2 get-motor"
                           "| motor2 config-acc-rate <0-100>"
                           "| motor2 config-acc-rate-delay <0-0xFF>"
                           "| motor2 config-brak-rate <0-100>"
                           "| motor2 config-brak-rate-delay <0-0xFF>");
        shell_error(shell, "Usage: motor1 config-brak-rate <0-100>");
    }
    return 0;
}

static int cmd_motor1_config_acc_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t acceleration_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (acceleration_rate_delay != 0) {
            motor1.acceleration_rate_delay = acceleration_rate_delay;
        } else {
            shell_error(shell, "Invalid acceleration rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-acc-rate-delay <ms>");
    }
    return 0;
}

static int cmd_motor1_config_brak_rate_delay(const struct shell *shell, size_t argc, char **argv) {
    if (argc == 2) {
        int32_t braking_rate_delay = (int32_t)simple_strtou32(argv[1]);
        if (braking_rate_delay != 0) {
            motor1.braking_rate_delay = braking_rate_delay;
        } else {
            shell_error(shell, "Invalid braking rate delay.");
        }
    } else {
        shell_error(shell, "Usage: motor1 config-brak-rate-delay <ms>");
    }
    return 0;
}

    }
    return 0;
}

/**
 * @brief Shell command function to control both motors simultaneously.
 *
 * This function allows for simultaneous control of two motors, including setting
 * their speeds and directions. It's useful for coordinated movements of a dual-motor
 * system like a robot with chain tracks.
 *
 * **Usage**\n
 *     motors set <speed_motor1> <dir_motor1> <speed_motor2> <dir_motor2>\n
 *        - Sets the speed (0-100) and direction (0/1) for both motors.\n
 *
 * @param shell Pointer to the shell structure.
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return Returns 0 on success, or an error code on failure.
 */
static int cmd_motors(const struct shell *shell, size_t argc, char **argv) {
    if (argc < 5) {
        shell_print(shell, "Usage: motors set <speed_motor1> <dir_motor1> <speed_motor2> <dir_motor2>");
        return 0;
    }

    if (strcmp(argv[1], "set") == 0) {
        uint32_t speed_motor1 = simple_strtou32(argv[2]);
        bool dir_motor1 = simple_strtou32(argv[3]) != 0;
        uint32_t speed_motor2 = simple_strtou32(argv[4]);
        bool dir_motor2 = simple_strtou32(argv[5]) != 0;

        // Ensuring the speeds are within the valid range
        speed_motor1 = (speed_motor1 > 100) ? 100 : speed_motor1;
        speed_motor2 = (speed_motor2 > 100) ? 100 : speed_motor2;

        set_motors(&motor1, &motor2, speed_motor1, speed_motor2, dir_motor1, dir_motor2);
        shell_print(shell, "Motors set: Motor1 - Speed %d, Direction %d; Motor2 - Speed %d, Direction %d",
                    speed_motor1, dir_motor1, speed_motor2, dir_motor2);
    } else {
        shell_print(shell, "Invalid command or number of arguments.");
    }
    return 0;
}


/**
 * @brief Sets the direction of a motor.
 *
 * Sets the direction of the specified motor. It stops the motor (if moving) before
 * setting the new direction. The function locks the motor's mutex to ensure thread safety.
 *
 * **Usage**
 * ```
 * motor_t motor; // Assume this is initialized
 * motordriver_set_dir(&motor, true); // Set direction to true/1
 * ```
 *
 * @param motor Pointer to the motor structure whose direction is to be set.
 * @param dir The new direction for the motor (true for one direction, false for the opposite).
 */
void motordriver_set_dir(motor_t* motor, bool dir) {
    // Set the direction of the motor
    k_mutex_lock(motor->mutex, K_FOREVER);
    if (motor->direction != dir)
    {
        uint32_t target_speed = 0;
        motordriver_adjust_motor_speed_blocking(motor, target_speed);
        motor->direction = dir;
        gpio_pin_set(motor->dir_pin.port, motor->dir_pin.pin,motor->direction);
        printk("Direction of %s set to %d\n", motor->name, motor->direction);
    }
    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Sets the speed of the motor.
 *
 * Sets the speed of the motor by adjusting the PWM duty cycle. Speed is represented
 * as a percentage of the maximum speed.
 *
 * **Usage**
 * ```
 * motor_t motor; // Assume this is initialized
 * set_speed(&motor, 50); // Set speed to 50%
 * ```
 *
 * @param motor Pointer to the motor structure.
 * @param speed_percent The speed of the motor as a percentage (0-100).
 */
void set_speed(motor_t* motor, uint32_t speed_percent) {
    k_mutex_lock(motor->mutex, K_FOREVER);
    // Ensure the speed_percent is within bounds
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    // Calculate the duty cycle based on the speed percentage
    uint32_t duty_cycle_ns = motor->pwm_spec.period * speed_percent / 100;
    // Set the PWM duty cycle
    int ret = pwm_set(motor->pwm_spec.dev,
                      motor->pwm_spec.channel,
                      motor->pwm_spec.period,
                      duty_cycle_ns, motor->pwm_spec.flags);
    if (ret < 0) {
        printk("Error setting PWM speed for %s: %d\n", motor->name, ret);
    } else {
        // Update the motors speed in the struct
        motor->speed = speed_percent;
    }
    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Initializes a motor.
 *
 * Initializes a motor by setting up GPIO and PWM, and initializing the mutex and timer.
 * It also sets the initial direction and speed to OFF.
 *
 * **Usage**
 * ```
 * motor_t motor; // Declare a motor
 * init_motor(&motor); // Initialize the motor
 * ```
 *
 * @param motor Pointer to the motor structure to be initialized.
 */
void init_motor(motor_t* motor) {
    if (!device_is_ready(motor->pwm_spec.dev)) {
        printk("%s Error: PWM not ready.\n", motor->name);
        return;
    }
    // Initialize GPIO pins as outputs for direction and PWM
    gpio_pin_configure_dt(&motor->dir_pin, GPIO_OUTPUT);
    k_mutex_init(motor->mutex);
    k_timer_init(motor->timer, motor_speed_adjust_timer_expiry_function, NULL);
    k_timer_user_data_set(motor->timer, motor);
    // Set initial direction and speed (PWM) to OFF
    bool initial_direction = 0;
    uint32_t initial_speed = 0;
    motordriver_set_dir(motor, initial_direction);
    printk("%s configured!\n", motor->name);
    motordriver_adjust_motor_speed_blocking(motor, initial_speed);
}

/**
 * @brief Gradually adjusts the motor speed in a blocking manner.
 *
 * This function adjusts the speed of the motor to the target speed in a blocking
 * manner, meaning it will not return until the target speed is reached. It
 * incrementally increases or decreases the speed based on the current and target
 * values, handling both acceleration and braking.
 *
 * **Usage**
 * ```
 * motor_t motor; // Assume this is initialized
 * motordriver_adjust_motor_speed_blocking(&motor, 50); // Set target speed to 50%
 * ```
 *
 * @param motor Pointer to the motor structure.
 * @param target_speed The target speed as a percentage (0-100).
 */
void motordriver_adjust_motor_speed_blocking(motor_t* motor, uint32_t target_speed) {
    if (motor->acceleration_rate == 0) {
        printk("Rate of speed change cannot be zero.\n");
        return;
    }
    // Ensure target speed is within bounds
    if (target_speed > 100) {
        target_speed = 100;
    }
    while (motor->speed != target_speed) {
        if (motor->speed < target_speed) {
            // Accelerate
            motor->speed += (motor->speed + motor->acceleration_rate > target_speed) ?
                            (target_speed - motor->speed) : motor->acceleration_rate;
            set_speed(motor, motor->speed);
            k_msleep(motor->acceleration_rate_delay);
        } else if (motor->speed > target_speed){
            // Brake
            motor->speed -= (motor->speed - motor->braking_rate < target_speed) ?
                            (motor->speed - target_speed) : motor->braking_rate;
            set_speed(motor, motor->speed);
            k_msleep(motor->braking_rate_delay);
        }
    }
    printk("%s target speed: %d reached.\n", motor->name, motor->speed);
}

/**
 * @brief Timer expiry function for motor speed adjustment.
 *
 * Invoked when the motor's timer expires. It adjusts the motor speed towards the
 * target speed, either by accelerating or braking, based on the current and target
 * speeds. This function is usually used in conjunction with non-blocking speed adjustment.
 *
 * **Usage**
 * Typically called internally by a timer and not directly used in application code.
 *
 * @param timer_id Pointer to the expired timer.
 */
void motor_speed_adjust_timer_expiry_function(struct k_timer *timer_id) {
    motor_t *motor = k_timer_user_data_get(timer_id);
    k_mutex_lock(motor->mutex, K_FOREVER);
    if (motor->speed < motor->target_speed) {
        // Accelerate
        uint32_t speed_increment = MIN(motor->acceleration_rate, motor->target_speed - motor->speed);
        motor->speed += speed_increment;
        set_speed(motor, motor->speed);
    } else if (motor->speed > motor->target_speed) {
        // Brake
        uint32_t speed_decrement = MIN(motor->braking_rate, motor->speed - motor->target_speed);
        motor->speed -= speed_decrement;
        set_speed(motor, motor->speed);
    }

    if (motor->speed != motor->target_speed) {
        if (motor->speed < motor->target_speed) {
            //acceleration timer
            k_timer_start(timer_id, K_MSEC(motor->acceleration_rate_delay), K_NO_WAIT);
        } else {
            //braking timer
            k_timer_start(timer_id, K_MSEC(motor->braking_rate_delay), K_NO_WAIT);
        }
    } else {
        printk("%s target speed: %d reached.\n", motor->name, motor->speed);
    }

    k_mutex_unlock(motor->mutex);
}

/**
 * @brief Gradually adjusts the motor speed in a non-blocking manner.
 *
 * Starts or restarts a timer to adjust the motor speed towards the target speed.
 * The actual speed adjustment is performed in the timer callback function. This
 * function allows other operations to continue while the motor speed is being adjusted.
 *
 * **Usage**
 * ```
 * motor_t motor; // Assume this is initialized
 * motordriver_adjust_motor_speed_non_blocking(&motor, 75); // Adjust speed to 75%
 * ```
 *
 * @param motor Pointer to the motor structure.
 * @param target_speed The target speed as a percentage (0-100).
 */
void motordriver_adjust_motor_speed_non_blocking(motor_t *motor, uint32_t target_speed) {
    if (motor->acceleration_rate == 0 || motor->braking_rate == 0) {
        printk("Rate of speed change cannot be zero.\n");
        return;
    }
    k_mutex_lock(motor->mutex, K_FOREVER);
    uint32_t new_target_speed = MIN(target_speed, 100);
    // Only start or restart the timer if the target speed has changed
    if (motor->target_speed != new_target_speed) {
        motor->target_speed = new_target_speed;
        // wait a bit before starting adjusting speed
        k_timer_start(motor->timer, K_MSEC(ADJUST_SPEED_DELAY_MS), K_NO_WAIT);
    }
    k_mutex_unlock(motor->mutex);
}


void set_motors(motor_t *m1, motor_t *m2, uint32_t speed1, uint32_t speed2, bool dir1, bool dir2) {
    // Brake both motors to zero speed non-blocking if direction change is needed
    bool needToStopM1 = (m1->direction != dir1);
    bool needToStopM2 = (m2->direction != dir2);
    if (needToStopM1) {
        motordriver_adjust_motor_speed_non_blocking(m1, 0);
    }
    if (needToStopM2) {
        motordriver_adjust_motor_speed_non_blocking(m2, 0);
    }
    // Monitor the speed of both motors if needed
    while ((needToStopM1 && m1->speed != 0) || (needToStopM2 && m2->speed != 0)) {
        // Small delay to avoid busy waiting
        k_msleep(CHECK_INTERVAL_MS);
    }
    // Change directions once motors are stopped
    if (needToStopM1 || needToStopM2) {
        k_msleep(WAIT_DIR_CHANGE_INTERVAL_MS);
    }
    if (needToStopM1) {
        motordriver_set_dir(m1, dir1);
    }
    if (needToStopM2) {
        motordriver_set_dir(m2, dir2);
    }
    // Set the new speeds
    motordriver_adjust_motor_speed_non_blocking(m1, speed1);
    motordriver_adjust_motor_speed_non_blocking(m2, speed2);
}


/**
 * @brief Initializes the motor driver module.
 *
 * Sets up the motors and their associated hardware, preparing them for operation.
 * This includes initializing the motors, setting up GPIO and PWM, and ensuring
 * the motors are in a known state.
 *
 * **Usage**
 * ```
 * motordriver_init(); // Initialize both motors
 * ```
 */
void motordriver_init() {
    init_motor(&motor1);
    init_motor(&motor2);
}

/* Creating subcommands (level 1 command) array for command "motor1". */
SHELL_STATIC_SUBCMD_SET_CREATE(sub_motor1,
                               SHELL_CMD(set-dir, NULL, "Set motor direction", cmd_motor1_set_dir),
                               SHELL_CMD(set-speed, NULL, "Set motor speed", cmd_motor1_set_speed),
                               SHELL_CMD(Zset-speed, NULL, "Unsafe set motor speed (direct PWM)", cmd_motor1_unsafe_set_speed),
                               SHELL_CMD(get-speed, NULL, "Get motor speed", cmd_motor1_get_speed),
                               SHELL_CMD(get-dir, NULL, "Get motor direction", cmd_motor1_get_dir),
                               SHELL_CMD(get-motor, NULL, "Get motor configuration", cmd_motor1_get_motor),
                               SHELL_CMD(config-acc-rate, NULL, "Configure acceleration rate", cmd_motor1_config_acc_rate),
                               SHELL_CMD(config-brak-rate, NULL, "Configure braking rate", cmd_motor1_config_brak_rate),
                               SHELL_CMD(config-acc-rate-delay, NULL, "Configure acceleration rate delay", cmd_motor1_config_acc_rate_delay),
                               SHELL_CMD(config-brak-rate-delay, NULL, "Configure braking rate delay", cmd_motor1_config_brak_rate_delay),
                               SHELL_SUBCMD_SET_END
);


SHELL_CMD_REGISTER(motor1, &sub_motor1,
                   "control motor1 of pico-pluto.",
                   cmd_motor1);
SHELL_CMD_REGISTER(motor2, NULL,
                   "control motor2 of pico-pluto. Execute without arguments to get more info",
                   cmd_motor2);
SHELL_CMD_REGISTER(motors, NULL,
                   "control both motors of pico-pluto. Execute without arguments to get more info",
                   cmd_motors);
