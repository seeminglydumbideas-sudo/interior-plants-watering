#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>


// Gardener watering parameters
// Colldown : how long to let water flows in earth before re-watering (only if we have a new measurement)
const unsigned long COOLDOWN_MS = 10 * 60 * 1000UL; // 1 minutes


// Display Power Management
const uint8_t  BRIGHT_FULL   = 255;
const uint8_t  BRIGHT_DIM    = 30;
const uint8_t  BRIGHT_OFF    = 0;
const uint32_t DIM_TIMEOUT   = 120000;  // 2min to dim
const uint32_t OFF_TIMEOUT   = 360000;  // 3min to power off (3x dim)

enum UIMode { RUN, SETUP_NAV, WATERING_SETUP, SENSOR_SETUP, SENSOR_SETTINGS_NAV, SENSOR_SETTING_EDIT };

// Debug Coordinate Grid Overlay
const bool ENABLE_DEBUG_GRID = false;

// Gardener debug logging in bottom box
const bool ENABLE_DEBUG_GARDENER = true;

#endif
