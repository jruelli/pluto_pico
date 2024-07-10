#ifndef NEODRIVER_H
#define NEODRIVER_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>

/*=========================================================================
    REGISTERS
    -----------------------------------------------------------------------*/
#define SEESAW_NEOPIXEL_BASE 0x0E
#define SEESAW_NEOPIXEL_PIN 0x01
#define SEESAW_NEOPIXEL_STATUS 0x00
#define SEESAW_NEOPIXEL_SPEED 0x02
#define SEESAW_NEOPIXEL_BUF_LENGTH 0x03
#define SEESAW_NEOPIXEL_BUF 0x04
#define SEESAW_NEOPIXEL_SHOW 0x05


/*=========================================================================*/

/*=========================================================================
    STRUCTURES
    -----------------------------------------------------------------------*/
struct pluto_neodriver {
    const struct device *i2c_dev;
    uint8_t i2c_addr;
};

/*=========================================================================
    FUNCTION PROTOTYPES
    -----------------------------------------------------------------------*/
int neodriver_init(void);
int neodriver_set_color(uint16_t led_index, uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
int neodriver_set_all_colors(uint8_t red, uint8_t green, uint8_t blue, uint8_t white);
int neodriver_show(void);

#endif // NEODRIVER_H
