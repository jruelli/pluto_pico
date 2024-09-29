/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_PLUTO_CONFIG_H
#define APP_PLUTO_CONFIG_H

/* led thread config */
#define PLUTO_LED_THREAD_STACK_SIZE         512
#define PLUTO_LED_THREAD_PRIORITY           12u
#define PLUTO_LED_THREAD_SLEEP_TIME_S       1

/* neopixel thread config */
#define PLUTO_NEOPIXEL_THREAD_STACK_SIZE         512
#define PLUTO_NEOPIXEL_THREAD_PRIORITY           14u
#define PLUTO_NEOPIXEL_THREAD_SLEEP_TIME_MS      50u

/* mcp9808 thread config */
#define PLUTO_MCP9808_THREAD_STACK_SIZE         512
#define PLUTO_MCP9808_THREAD_PRIORITY           10u
#define PLUTO_MCP9808_THREAD_SLEEP_TIME_S      (1)
#define PLUTO_MCP9808_THRESH_SLEEP_TIME_S      (10)

/* ads1115 thread config */
#define PLUTO_ADS1115_THREAD_STACK_SIZE         512
#define PLUTO_ADS1115_THREAD_PRIORITY           9u
#define PLUTO_ADS1115_THREAD_SLEEP_TIME_S      (1)
#define PLUTO_ADS1115_THRESH_SLEEP_TIME_S      (10)

/* vl53l0x thread config */
#define PLUTO_VL53L0X_THREAD_STACK_SIZE         1024
#define PLUTO_VL53L0X_THREAD_PRIORITY           8u
#define PLUTO_VL53L0X_THREAD_SLEEP_TIME_MS      (500)

#endif //APP_PLUTO_CONFIG_H
