//
// Created by Jannis on 02.02.2024.
//

#ifndef APP_VL53L0X_H
#define APP_VL53L0X_H

enum proximity_state {
    PROXIMITY_STATE_DISTANCE,
    PROXIMITY_STATE_PROXIMITY,
    PROXIMITY_STATE_OFF
};
// Function declarations
int vl53l0x_test(void);

#endif //APP_VL53L0X_H
