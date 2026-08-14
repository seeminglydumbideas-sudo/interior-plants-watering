#include <Arduino.h>
#include "Sensor.h"
#include "Blues.h"
#include "Moisture.h"
#include "PowerManager.h"

// Interval between the end of one read cycle and the start of the next.
#define SENSOR_READ_INTERVAL_MS (30 * 1000UL)

Sensor::Sensor(BlueS& blues, Moisture& moisture, PowerManager& power)
  : _blues(blues), _moisture(moisture), _power(power), _nextReadAt(0) {
}

void Sensor::init() {
  _nextReadAt = 0; // trigger an immediate first read in loop()
}

void Sensor::loop() {
  if (_moisture.isReady()) {
    Moisture::Reading m = _moisture.getReading(); // clears ready, returns to IDLE

    SensorData data;
    data.moisture    = m.percentage;
    data.battery     = _power.getLevel();
    data.powerSource = _power.checkSource();
    data.bootCount   = _power.getBootCount();
    data.rawValue    = m.rawValue;
    data.rawMin      = m.rawMin;
    data.rawMax      = m.rawMax;
    _blues.setData(data);
    _power.notifyDataReady(data.moisture >= 0);

    Serial.printf("[INFO ] Sensor: moisture=%d%%, raw=[%d<%d<%d], battery=%.2fV, power=%d, boot=%d\n",
                  data.moisture, data.rawMin, data.rawValue, data.rawMax,
                  data.battery, data.powerSource, data.bootCount);

    _nextReadAt = millis() + SENSOR_READ_INTERVAL_MS;
  }

  if (_moisture.isIdle() && millis() >= _nextReadAt) {
    _moisture.startRead();
  }
}
