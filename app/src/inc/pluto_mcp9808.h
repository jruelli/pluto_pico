/*
 * Copyright (c) Jannis Ruellmann 2024
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file relays.h
 * @brief Relays module.
 *
 * Header for relays module
 *
 * @author Jannis Ruellmann
 */

#ifndef APP_PLUTO_MCP9808_H
#define APP_PLUTO_MCP9808_H

#include <stdio.h>

struct mcp9808_sensor {
    const struct device *dev;
    bool enabled;
    double temperature;
};

// Function declarations
void pluto_mcp9808_init();

#endif //APP_PLUTO_MCP9808_H
