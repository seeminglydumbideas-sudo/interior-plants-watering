#ifndef BOOTBUTTON_H
#define BOOTBUTTON_H

/**
 * @file BootButton.h
 * @brief Onboard BOOT Button Handler & Calibration Reset
 * 
 * @details
 * The BootButton class monitors the onboard BOOT button (GPIO 9 on the Seeed Studio XIAO ESP32-C3)
 * to handle physical user interaction, specifically triggering a soil moisture calibration reset.
 * 
 * Hardware & Logic:
 * - Pin: GPIO 9 configured as INPUT_PULLUP (active-LOW; pressing pulls the pin LOW).
 * - State Machine: RELEASED -> PRESSING -> TRIGGERED.
 * - Long-Press Action: Holding the button for 3 seconds (LONG_PRESS_MS = 3000 ms) calls
 *   Moisture::resetCalibration(), which wipes stored dry/wet bounds (rawMin/rawMax) from NVS flash.
 * @warning Deep Sleep Behavior:
 * Button polling in loop() only runs while the ESP32 is awake. During deep sleep (CPU powered off),
 * pressing the BOOT button will NOT trigger a calibration reset. To calibrate a sleeping sensor,
 * press RESET (or power cycle) first to wake the MCU into an active window, then hold BOOT for 3 seconds.
 */

#include "Moisture.h"

// GPIO9 is the BOOT button on the XIAO ESP32C3.
// The pin is pulled HIGH internally; pressing the button pulls it LOW.
#define BOOT_BUTTON_PIN     9
#define LONG_PRESS_MS    3000  // hold duration to trigger calibration reset

class BootButton {

public:
  /**
   * @brief Constructs a BootButton instance tied to the Moisture controller.
   * @param moisture Reference to the Moisture manager to trigger calibration resets.
   */
  BootButton(Moisture& moisture);

  /**
   * @brief Configures GPIO 9 pin mode with internal pull-up resistor.
   */
  void init();

  /**
   * @brief Non-blocking state machine loop to track button press durations.
   */
  void loop();

private:
  enum State { RELEASED, PRESSING, TRIGGERED };

  Moisture&     _moisture;
  State         _state;
  unsigned long _pressedAt;
};

#endif // BOOTBUTTON_H
