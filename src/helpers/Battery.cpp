#include <Arduino.h>
#include "Battery.h"

uint16_t batt_percent(uint8_t batt, uint16_t mv) {
    uint16_t last = mv; // set last to mv so it can never start greater than itself
    uint16_t p = 0;
    for (int i = 0; i < 102; i++) { // always 101 elements, 100 percent and 0
        // if we match the current value or are between the current and last value
        // we return the lower of the the two, giving accuracy +/- 1% soc
        if (mv <= last && mv >= batt_curve[i][batt]) {
            p = batt_curve[i][batt_p_col];
            break;
        }
        last = batt_curve[i][batt];
    } 
    return p;
};
