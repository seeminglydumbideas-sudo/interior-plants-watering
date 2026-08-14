#ifndef SENSOR_H
#define SENSOR_H

struct SensorData {
  int   moisture;    // percentage 0-100, or -1 when not yet calibrated
  float battery;     // voltage in volts
  int   powerSource; // POWER_BAT or POWER_USB (from PowerManager.h)
  int   rawValue;    // raw ADC reading (0-4095)
  int   rawMin;      // calibration lower bound
  int   rawMax;      // calibration upper bound
  int   bootCount;   // number of boots since power cycle
};

// Forward declarations — full definitions included by Sensor.cpp
class BlueS;
class Moisture;
class PowerManager;

class Sensor {

public:
  Sensor(BlueS& blues, Moisture& moisture, PowerManager& power);
  void init();
  void loop();

private:
  BlueS&        _blues;
  Moisture&     _moisture;
  PowerManager& _power;
  unsigned long _nextReadAt;
};

#endif // SENSOR_H
