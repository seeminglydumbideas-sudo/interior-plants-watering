#include "BootButton.h"
#include <Arduino.h>

BootButton::BootButton(Moisture& moisture)
  : _moisture(moisture), _state(RELEASED), _pressedAt(0) {
}

void BootButton::init() {
  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
}

void BootButton::loop() {
  bool pressed = (digitalRead(BOOT_BUTTON_PIN) == LOW);

  switch (_state) {

    case RELEASED:
      if (pressed) {
        _pressedAt = millis();
        _state     = PRESSING;
      }
      break;

    case PRESSING:
      if (!pressed) {
        _state = RELEASED; // released too early
      } else if (millis() - _pressedAt >= LONG_PRESS_MS) {
        _moisture.resetCalibration();
        _state = TRIGGERED; // wait for release before re-arming
      }
      break;

    case TRIGGERED:
      if (!pressed) {
        _state = RELEASED;
      }
      break;
  }
}
