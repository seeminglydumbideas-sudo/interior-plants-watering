#include "WaterPumps.h"

WaterPumps::WaterPumps(int p1, int p2, int p3, int p4) {
  _pins[0] = p1;
  _pins[1] = p2;
  _pins[2] = p3;
  _pins[3] = p4;
}

void WaterPumps::init() {
  // Passive mode: do not write to physical GPIO pins
  /*
  for (int i = 0; i < 4; i++) {
    pinMode(_pins[i], OUTPUT);
    digitalWrite(_pins[i], OFF_LEVEL);
  }
  */
}

void WaterPumps::setPump(int index, bool on) {
  if (index < 0 || index >= 4) return;
  // Passive mode: do not write to physical GPIO pins
  // digitalWrite(_pins[index], on ? ON_LEVEL : OFF_LEVEL);
}

void WaterPumps::allOff() {
  // Passive mode: do not write to physical GPIO pins
  /*
  for (int i = 0; i < 4; i++) {
    digitalWrite(_pins[i], OFF_LEVEL);
  }
  */
}
