#include "Moisture.h"
#include <Arduino.h>
#include <Preferences.h>

#define SENSOR_POWER D0  // Pin to power the sensor
#define SENSOR_PIN   D1  // Pin to read the analog signal

#define STABILIZE_MS  400  // time to wait after powering sensor before sampling
#define SAMPLE_MS     500  // time between samples
#define SAMPLE_COUNT    5  // number of samples (last one is used)

Moisture::Moisture()
  : _state(IDLE), _stateTimer(0), _sampleCount(0),
    _rawValue(0), _rawMin(4095), _rawMax(0), _ready(false), _lastReading({-1, 0, 0, 0}) {
}

void Moisture::init() {
  pinMode(SENSOR_POWER, OUTPUT);
  pinMode(SENSOR_PIN, INPUT);
  // Load calibration bounds from flash here, after Serial and the framework
  // are fully initialised (constructor runs too early, before setup()).
  Preferences prefs;
  prefs.begin("moisture", true); // read-only
  _rawMin = prefs.getInt("rawMin", 4095);
  _rawMax = prefs.getInt("rawMax", 0);
  prefs.end();

  // Sanity check: if stored rawMin is corrupted (e.g. <= 1000 from a bad reading or noise), recover it
  if (_rawMin <= 1000) {
    _rawMin = 4095;
  }

  Serial.printf("[INFO ] Moisture cal loaded: rawMin=%d, rawMax=%d\n", _rawMin, _rawMax);
}

void Moisture::startRead() {
  if (_state != IDLE) return; // ignore if a read is already in progress
  _sampleCount = 0;
  _rawValue    = 0;
  _ready       = false;
  digitalWrite(SENSOR_POWER, HIGH);
  _stateTimer = millis();
  _state      = STABILIZING;
}

void Moisture::loop() {
  switch (_state) {

    case IDLE:
    case COMPLETE:
      break;

    case STABILIZING:
      if (millis() - _stateTimer >= STABILIZE_MS) {
        _stateTimer = millis();
        _state      = SAMPLING;
      }
      break;

    case SAMPLING:
      if (millis() - _stateTimer >= SAMPLE_MS) {
        _rawValue = analogRead(SENSOR_PIN);
        Serial.printf("[TRACE] Raw Value [%d]\n", _rawValue);
        _sampleCount++;
        _stateTimer = millis();
        if (_sampleCount >= SAMPLE_COUNT) {
          _finalize();
        }
      }
      break;
  }
}

void Moisture::resetCalibration() {
  _rawMin = 4095;
  _rawMax = 0;
  _saveCalibration();
  Serial.println("[INFO ] Moisture calibration reset");
}

void Moisture::_saveCalibration() {
  Preferences prefs;
  prefs.begin("moisture", false); // read-write
  prefs.putInt("rawMin", _rawMin);
  prefs.putInt("rawMax", _rawMax);
  prefs.end();
  Serial.printf("[INFO ] Moisture cal saved: rawMin=%d, rawMax=%d\n", _rawMin, _rawMax);
}

void Moisture::_finalize() {
  digitalWrite(SENSOR_POWER, LOW);

  // auto-calibrate bounds; save to flash only when a bound changes
  // Ignore invalid / noise readings (<= 1000) so rawMin never gets corrupted to 0
  bool changed = false;
  if (_rawValue > 1000) {
    if (_rawMax < _rawValue) { _rawMax = _rawValue; changed = true; }
    if (_rawMin > _rawValue) { _rawMin = _rawValue; changed = true; }
    if (changed) _saveCalibration();
  } else {
    Serial.printf("[TRACE] Rejected low value [%d] (<= 1000)\n", _rawValue);
  }

  int percentage = -1;
  if (_rawMax > _rawMin + 1000) {
    percentage = 100 - ((100 * (_rawValue - _rawMin)) / (_rawMax - _rawMin));
    Serial.printf("[DEBUG] Moisture Raw Value [%d< %d <%d] => %d%%\n", _rawMin, _rawValue, _rawMax, percentage);
  } else {
    Serial.printf("[DEBUG] Moisture Raw Value [%d< %d <%d] => n/a\n", _rawMin, _rawValue, _rawMax);
  }

  _lastReading = { percentage, _rawValue, _rawMin, _rawMax };
  _ready = true;
  _state = COMPLETE;
}

bool Moisture::isReady() const {
  return _ready;
}

bool Moisture::isIdle() const {
  return _state == IDLE;
}

Moisture::Reading Moisture::getReading() {
  _ready = false;
  _state = IDLE;
  return _lastReading;
}
