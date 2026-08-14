#ifndef GARDENER_H
#define GARDENER_H

#include <string>

// Forward declarations to decouple files and prevent circular inclusions
class Store;
class Display;
class WaterPumps;

enum HumidityState {
  HUMIDITY_DRY,
  HUMIDITY_THIRSTY,
  HUMIDITY_HEALTHY,
  HUMIDITY_UNKNOWN
};

enum WateringState {
  WATERING_ACTIVE,
  WATERING_COOLDOWN,
  WATERING_WAIT_MEASUREMENT,
  WATERING_IDLE
};

class Gardener {
// moistureThreshold is a default value for new sensors. it will be overwritten through settings.
public:
  Gardener(WaterPumps& pumps, Display& display, int moistureThreshold = 25, unsigned long cooldownMs = 30000);
  void init();
  void loop(const Store& store);

private:
  WaterPumps&   _pumps;
  Display&      _display;
  int           _moistureThreshold;
  unsigned long _cooldownMs;

  struct PumpStatus {
    bool          active;
    unsigned long startTime;
    unsigned long durationMs;
    std::string   sensorName; // Track which sensor triggered this pump
  } _pumpStatus[4];
};

#endif
