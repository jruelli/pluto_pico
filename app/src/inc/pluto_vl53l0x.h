//
// Created by Jannis on 02.02.2024.
//

#ifndef APP_PLUTO_VL53L0X_H
#define APP_PLUTO_VL53L0X_H

enum sensor_mode {
    VL53L0X_MODE_DISTANCE,
    VL53L0X_MODE_PROXIMITY,
    VL53L0X_MODE_OFF,
    VL53L0X_MODE_ERROR
};
// Function declarations
void vl53l0x_init(void);

#endif //APP_PLUTO_VL53L0X_H
