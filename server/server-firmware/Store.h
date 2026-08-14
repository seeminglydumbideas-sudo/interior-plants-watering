#ifndef STORE_H
#define STORE_H

#include <Arduino.h>
#include <map>
#include <string>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "Gardener.h"

class PrometheusServer;

struct SensorReading {
  char          name[32];
  int           moisture;    // 0-100, or -1 if not yet calibrated
  float         battery;     // volts
  int           powerSource; // 0 = battery, 1 = USB
  int           rawValue;    // raw ADC (0-4095)
  int           rawMin;      // calibration lower bound
  int           rawMax;      // calibration upper bound
  int           bootCount;
  unsigned long lastSeen;    // millis() when last received

  // Watering Config
  int           pumpIndex;         // -1 = none, 0-3 = pumps
  int           moistureThreshold; // 0-100%
  int           durationSeconds;   // 1-60s
  unsigned long lastWateredTime;   // millis() when last watering finished

  // Runtime State
  HumidityState humidityState = HUMIDITY_UNKNOWN;
  WateringState wateringState = WATERING_IDLE;
};

// Compact struct for binary storage in NVS
struct StoredReading {
  char  name[32];
  int   moisture;
  float battery;
  int   powerSource;
  int   rawValue;
  int   rawMin;
  int   rawMax;
  int   bootCount;
  int   pumpIndex;
  int   moistureThreshold;
  int   durationSeconds;
  unsigned long lastWateredTime;
};

class Store {
public:
  void                                   begin();
  void                                   update(const SensorReading& r, bool isBLEUpdate = false);
  void                                   remove(const std::string& key);
  const std::map<std::string, SensorReading>& getAll() const;
  String                                 buildMetrics(PrometheusServer& prom) const;
  
  // Thread safety
  void lock() const;
  void unlock() const;

private:
  std::map<std::string, SensorReading> _data;
  Preferences                     _prefs;
  mutable SemaphoreHandle_t       _mutex;
  void                            _save(const SensorReading& r);
  void                            _load();
};

#endif
