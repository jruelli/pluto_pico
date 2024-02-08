//
// Created by Jannis on 02.02.2024.
//

#ifndef APP_VL53L0X_H
#define APP_VL53L0X_H

enum sensor_mode {
    VL53L0X_MODE_DISTANCE,
    VL53L0X_MODE_PROXIMITY,
    VL53L0X_MODE_OFF
};
// Function declarations
int vl53l0x_test(void);

#endif //APP_VL53L0X_H
