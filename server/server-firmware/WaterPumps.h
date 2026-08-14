#ifndef WATER_PUMPS_H
#define WATER_PUMPS_H

#include <Arduino.h>

class WaterPumps {
public:
  WaterPumps(int p1, int p2, int p3, int p4);
  void init();
  void setPump(int index, bool on);
  void allOff();

private:
  int _pins[4];
  // Most HW-316 boards are Active Low (LOW = Relay ON)
  const bool ON_LEVEL  = LOW;
  const bool OFF_LEVEL = HIGH;
};

#endif
