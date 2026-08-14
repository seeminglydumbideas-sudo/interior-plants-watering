#include "Gardener.h"
#include "Store.h"
#include "Display.h"
#include "WaterPumps.h"
#include "Config.h"
#include <Arduino.h>
#include <SimpleLogging.h>

Gardener::Gardener(WaterPumps& pumps, Display& display, int moistureThreshold, unsigned long cooldownMs) 
  : _pumps(pumps), _display(display), _moistureThreshold(moistureThreshold), _cooldownMs(cooldownMs) {
  for (int i = 0; i < 4; i++) {
    _pumpStatus[i].active = false;
    _pumpStatus[i].startTime = 0;
    _pumpStatus[i].durationMs = 0;
    _pumpStatus[i].sensorName = "";
  }
}

void Gardener::init() {
  _pumps.init();
  LOG_INFO("Gardener initialized");
}

void Gardener::loop(const Store& store) {
  unsigned long now = millis();

  store.lock();
  // 1. Process active pumps (simulated)
  for (int i = 0; i < 4; i++) {
    if (_pumpStatus[i].active) {
      if (now - _pumpStatus[i].startTime >= _pumpStatus[i].durationMs) {
        // Duration expired, turn off simulated pump
        _pumps.setPump(i, false);
        _display.setPumpActive(i, false);
        _display.setCooldownActive(i, true);
        _pumpStatus[i].active = false;
        
        // Update persistent state in Store
        const auto& all = store.getAll();
        if (all.count(_pumpStatus[i].sensorName)) {
           SensorReading r = all.at(_pumpStatus[i].sensorName);
           r.lastWateredTime = now;
           ((Store&)store).update(r); 
        }
        
        LOG_INFO("Gardener: Simulated Pump %d OFF (Completed)", i + 1);
      }
    }
  }

  // 2. Check all sensors for watering needs and update their states
  const auto& all = store.getAll();
  for (const auto& kv : all) {
    SensorReading r = kv.second;
    bool stateChanged = false;
    
    // Determine dynamic target moisture threshold (use custom config or Gardener fallback)
    int threshold = (r.moistureThreshold > 0) ? r.moistureThreshold : _moistureThreshold;

    // Safety: Handle Reboot/Rollover
    if (r.lastWateredTime > now) {
      r.lastWateredTime = 0;
      r.lastSeen = now;
      stateChanged = true;
    }

    // A. Re-evaluate WateringState for this sensor
    WateringState calculatedWateringState = WATERING_IDLE;
    
    if (r.pumpIndex >= 0 && r.pumpIndex <= 3) {
      int p = r.pumpIndex;
      bool isPumping = _pumpStatus[p].active;
      bool inCooldown = (r.lastWateredTime > 0 && (now - r.lastWateredTime < _cooldownMs));
      bool gotNewMeasurement = (r.lastSeen >= r.lastWateredTime);
      
      _display.setCooldownActive(p, inCooldown);
      
      if (isPumping) {
        calculatedWateringState = WATERING_ACTIVE;
      } else if (inCooldown) {
        calculatedWateringState = WATERING_COOLDOWN;
      } else if (!gotNewMeasurement) {
        calculatedWateringState = WATERING_WAIT_MEASUREMENT;
      } else {
        calculatedWateringState = WATERING_IDLE;
      }
    } else {
      calculatedWateringState = WATERING_IDLE;
    }

    // B. Re-evaluate HumidityState for this sensor
    HumidityState calculatedHumidityState = HUMIDITY_UNKNOWN;
    
    if (r.moisture == -1) {
      calculatedHumidityState = HUMIDITY_UNKNOWN;
    } else if (r.lastWateredTime > 0 && r.lastSeen < r.lastWateredTime) {
      // after last watering, we did not get any more measurement yet
      calculatedHumidityState = HUMIDITY_UNKNOWN;
    } else {
      if (r.moisture <= threshold) {
        calculatedHumidityState = HUMIDITY_DRY;
      } else if (r.moisture <= 2 * threshold) {
        calculatedHumidityState = HUMIDITY_THIRSTY;
      } else {
        calculatedHumidityState = HUMIDITY_HEALTHY;
      }
    }

    // C. Trigger watering decision
    if (r.pumpIndex >= 0 && r.pumpIndex <= 3) {
      int p = r.pumpIndex;
      if (calculatedWateringState == WATERING_IDLE && calculatedHumidityState == HUMIDITY_DRY) {
        // START WATERING (Simulated)
        _pumpStatus[p].active = true;
        _pumpStatus[p].startTime = now;
        _pumpStatus[p].durationMs = (unsigned long)r.durationSeconds * 1000UL;
        _pumpStatus[p].sensorName = std::string(r.name);
        
        _pumps.setPump(p, true);
        _display.setPumpActive(p, true, _pumpStatus[p].durationMs);
        LOG_INFO("Gardener: Simulated Pump %d ON (Sensor %s: %d%% <= %d%%)", 
                 p + 1, r.name, r.moisture, threshold);
                 
        // Immediately reflect the updated states
        calculatedWateringState = WATERING_ACTIVE;
        calculatedHumidityState = HUMIDITY_UNKNOWN;
      }
    }

    // D. Update states in the Store if changed
    if (r.wateringState != calculatedWateringState || r.humidityState != calculatedHumidityState) {
      r.wateringState = calculatedWateringState;
      r.humidityState = calculatedHumidityState;
      stateChanged = true;
    }
    
    if (stateChanged) {
      ((Store&)store).update(r);
    }
  }
  store.unlock();
}
