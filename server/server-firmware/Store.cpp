#include "Store.h"
#include <PrometheusServer.h>

void Store::begin() {
  _mutex = xSemaphoreCreateRecursiveMutex();
  _prefs.begin("sensors", false);
  _load();
}

void Store::lock() const {
  if (_mutex) xSemaphoreTakeRecursive(_mutex, portMAX_DELAY);
}

void Store::unlock() const {
  if (_mutex) xSemaphoreGiveRecursive(_mutex);
}

void Store::update(const SensorReading& r, bool isBLEUpdate) {
  lock();
  std::string key = r.name;
  bool changed = false;

  if (_data.count(key)) {
    SensorReading& existing = _data[key];
    
    if (isBLEUpdate) {
      // BLE update: check if telemetry values changed to throttle flash writes
      if (existing.moisture    != r.moisture    ||
          existing.battery     != r.battery     ||
          existing.powerSource != r.powerSource ||
          existing.rawValue    != r.rawValue    ||
          existing.rawMin      != r.rawMin      ||
          existing.rawMax      != r.rawMax      ||
          existing.bootCount   != r.bootCount) {
        changed = true;
      }
      
      // Update only telemetry fields from BLE
      existing.moisture    = r.moisture;
      existing.battery     = r.battery;
      existing.powerSource = r.powerSource;
      existing.rawValue    = r.rawValue;
      existing.rawMin      = r.rawMin;
      existing.rawMax      = r.rawMax;
      existing.bootCount   = r.bootCount;
      existing.lastSeen    = r.lastSeen;
    } else if (r.lastSeen == 0) {
      // Configuration-only update
      if (existing.pumpIndex != r.pumpIndex ||
          existing.moistureThreshold != r.moistureThreshold ||
          existing.durationSeconds != r.durationSeconds) {
        existing.pumpIndex = r.pumpIndex;
        existing.moistureThreshold = r.moistureThreshold;
        existing.durationSeconds = r.durationSeconds;
        changed = true;
      }
    } else {
      // Internal / Gardener state update
      // Check if lastWateredTime changed to know if we need to write to flash
      if (existing.lastWateredTime != r.lastWateredTime) {
        changed = true;
      }
      existing = r;
    }
  } else {
    // New sensor
    _data[key] = r;
    // Default config for new sensors
    _data[key].pumpIndex = -1;
    _data[key].moistureThreshold = 25;
    _data[key].durationSeconds = 10;
    changed = true;
  }
  
  if (changed) {
    _save(_data[key]);
  }
  unlock();
}

void Store::remove(const std::string& key) {
  lock();
  if (_data.erase(key) > 0) {
    if (_data.empty()) {
      _prefs.remove("data");
    } else {
      SensorReading dummy;
      _save(dummy);
    }
  }
  unlock();
}

const std::map<std::string, SensorReading>& Store::getAll() const {
  return _data;
}

String Store::buildMetrics(PrometheusServer& prom) const {
  lock();
  String body;
  for (const auto& kv : _data) {
    const SensorReading& r  = kv.second;
    const char*          sn = r.name;
    float ageSec = (millis() - r.lastSeen) / 1000.0f;
    prom.appendMetric(body, "sensor_moisture",      "Soil moisture % (0-100, -1=uncalibrated)", "gauge",   sn, r.moisture);
    prom.appendMetric(body, "sensor_battery_volts", "Battery voltage",                          "gauge",   sn, r.battery);
    prom.appendMetric(body, "sensor_power_source",  "Power source (0=battery 1=USB)",           "gauge",   sn, r.powerSource);
    prom.appendMetric(body, "sensor_raw_value",     "Raw ADC value (0-4095)",                   "gauge",   sn, r.rawValue);
    prom.appendMetric(body, "sensor_raw_min",       "Calibration lower bound",                  "gauge",   sn, r.rawMin);
    prom.appendMetric(body, "sensor_raw_max",       "Calibration upper bound",                  "gauge",   sn, r.rawMax);
    prom.appendMetric(body, "sensor_boot_count",         "Device boot count",                        "counter", sn, r.bootCount);
    prom.appendMetric(body, "sensor_age_seconds",        "Seconds since last BLE advertisement",     "gauge",   sn, ageSec);
    prom.appendMetric(body, "sensor_moisture_threshold", "Configured moisture threshold %",          "gauge",   sn, r.moistureThreshold);
    float moistureAlert = (r.moisture >= 0 && r.moisture < r.moistureThreshold) ? 1.0f : 0.0f;
    prom.appendMetric(body, "sensor_moisture_alert",     "1 if moisture is below threshold",         "gauge",   sn, moistureAlert);
    body += "\n";
  }
  unlock();
  return body;
}

void Store::_save(const SensorReading& /*r*/) {
  // Assuming lock is already held by update() or other caller
  size_t count = _data.size();
  if (count == 0) return;
  if (count > 60) count = 60; // NVS blob limit safety

  StoredReading* buffer = new StoredReading[count];
  size_t i = 0;
  for (const auto& kv : _data) {
    if (i >= count) break;
    const SensorReading& src = kv.second;
    StoredReading& dst = buffer[i++];
    memset(&dst, 0, sizeof(StoredReading));
    strncpy(dst.name, src.name, sizeof(dst.name) - 1);
    dst.moisture = src.moisture;
    dst.battery = src.battery;
    dst.powerSource = src.powerSource;
    dst.rawValue = src.rawValue;
    dst.rawMin = src.rawMin;
    dst.rawMax = src.rawMax;
    dst.bootCount = src.bootCount;
    dst.pumpIndex = src.pumpIndex;
    dst.moistureThreshold = src.moistureThreshold;
    dst.durationSeconds = src.durationSeconds;
    dst.lastWateredTime = src.lastWateredTime;
  }

  _prefs.putBytes("data", buffer, count * sizeof(StoredReading));
  delete[] buffer;
}

void Store::_load() {
  lock();
  size_t len = _prefs.getBytesLength("data");
  if (len == 0 || len % sizeof(StoredReading) != 0) {
    unlock();
    return;
  }

  size_t count = len / sizeof(StoredReading);
  StoredReading* buffer = new StoredReading[count];
  _prefs.getBytes("data", buffer, len);

  for (size_t i = 0; i < count; i++) {
    StoredReading& src = buffer[i];
    // Force null termination for safety
    src.name[sizeof(src.name)-1] = '\0';
    
    SensorReading dst;
    memset(&dst, 0, sizeof(SensorReading));
    dst.humidityState = HUMIDITY_UNKNOWN;
    dst.wateringState = WATERING_IDLE;
    strncpy(dst.name, src.name, sizeof(dst.name) - 1);
    dst.moisture = src.moisture;
    dst.battery = src.battery;
    dst.powerSource = src.powerSource;
    dst.rawValue = src.rawValue;
    dst.rawMin = src.rawMin;
    dst.rawMax = src.rawMax;
    dst.bootCount = src.bootCount;
    dst.pumpIndex = src.pumpIndex;
    dst.moistureThreshold = src.moistureThreshold;
    dst.durationSeconds = src.durationSeconds;
    dst.lastWateredTime = src.lastWateredTime;
    dst.lastSeen = millis(); // Default to "just seen" on load to avoid huge age
    _data[std::string(dst.name)] = dst;
  }

  delete[] buffer;
  unlock();
  Serial.printf("[INFO ] Store loaded %d sensors from NVS\n", (int)count);
}
