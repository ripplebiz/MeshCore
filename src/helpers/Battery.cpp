#include <Arduino.h>
#include "Battery.h"

uint16_t Battery::percent(uint8_t batt, uint16_t mv) {
    uint16_t last = mv; // set last to mv so it can never start greater than itself
    uint16_t p = 0;
    for (int i = 0; i < 101; i++) { // always 101 elements, 100 percentile soc points
        // if we match the current value or are between the current and last value
        // we return the lower of the the two, giving accuracy +/- 1% soc
        if (mv <= last && mv >= Battery::curve[i][batt]) {
            p = Battery::curve[i][Battery::percent_col];
            break;
        }
        last = Battery::curve[i][batt];
    } 
    return p;
};