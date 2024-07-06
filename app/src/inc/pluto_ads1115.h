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

#ifndef APP_PLUTO_ADS1115_H
#define APP_PLUTO_ADS1115_H

#include <stdio.h>

struct ads1115_input {
    const char *name;
    bool enabled;
    double voltage;
    bool threshold_enabled;
    double threshold;
};

// Function declarations
void pluto_ads1115_init();

#endif //APP_PLUTO_ADS1115_H
