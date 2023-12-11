//
// Created by Jannis on 11.12.2023.
//

#ifndef APP_USER_LED_H
#define APP_USER_LED_H

#include <zephyr/drivers/gpio.h>

/* The devicetree node identifier for the "led0" alias. */
#define LED0_NODE DT_ALIAS(led0)

/* Function declarations */
void user_led_init(void);
void user_led_thread(void);

#endif //APP_USER_LED_H


