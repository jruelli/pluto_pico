#ifndef TB6612FNG_H
#define TB6612FNG_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>

// GPIO specifications from device tree
#define RELAY_0	DT_ALIAS(relay1)
#define RELAY_1	DT_ALIAS(relay2)
#define RELAY_2	DT_ALIAS(relay3)
#define RELAY_3	DT_ALIAS(relay4)
#define RELAY_4	DT_ALIAS(relay5)
#define RELAY_5	DT_ALIAS(relay6)
#define RELAY_6	DT_ALIAS(relay7)
#define RELAY_7	DT_ALIAS(relay8)

// Function declarations
void relay_init();
void control_relays(uint8_t value);
void control_relay_by_name(const char *name, bool state);
const char* get_relay_name(int relay_number);

#endif // TB6612FNG_H
