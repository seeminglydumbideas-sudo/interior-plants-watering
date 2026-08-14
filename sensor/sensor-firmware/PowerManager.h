#ifndef POWERMANAGER_H
#define POWERMANAGER_H

#include <Arduino.h>

/**
 * @file PowerManager.h
 * @brief Power & Sleep Management for XIAO ESP32C3 Sensor Node.
 * 
 * @details
 * The PowerManager handles battery voltage monitoring and manages **three distinct sleep modes**:
 * 
 * 1. **`REFLASH_SAFETY_DELAY` ("Fake" Sleep):**
 *    - Active on `bootCount == 1` within the first 120 seconds of uptime (`REFLASH_SAFETY_WINDOW_MS`).
 *    - Stops BLE advertising and performs a CPU `delay()` without entering hardware deep sleep.
 *    - Keeps the USB CDC port open so firmware can be easily reflashed without needing BOOT/RST buttons.
 * 
 * 2. **`SHORT_DEEP_SLEEP` (2 minutes):**
 *    - Active while total accumulated uptime is under 40 minutes (`SHORT_SLEEP_WINDOW_SEC`).
 *    - Enters hardware deep sleep (`esp_deep_sleep_start()`) for `SHORT_SLEEP_DURATION_SEC` (2 min per cycle).
 *    - Provides frequent telemetry immediately after power-on or firmware update for testing.
 * 
 * 3. **`LONG_DEEP_SLEEP` (12 hours):**
 *    - Active after the initial 40-minute window (`totalElapsedSec >= SHORT_SLEEP_WINDOW_SEC`).
 *    - Enters hardware deep sleep (`esp_deep_sleep_start()`) for `LONG_SLEEP_DURATION_SEC` (12 hours per cycle).
 *    - Maximizes battery longevity during standard production operation.
 */

#define POWER_BAT 0
#define POWER_USB 1

/**
 * @brief Explicit sleep modes supported by the PowerManager.
 */
enum class SleepMode {
  REFLASH_SAFETY_DELAY, /**< CPU delay on 1st boot to allow USB reflash safety window */
  SHORT_DEEP_SLEEP,     /**< 2-min hardware deep sleep during initial 40-min window */
  LONG_DEEP_SLEEP       /**< 12-hr hardware deep sleep for long-term battery saving */
};

class PowerManager {

public:
  /**
   * @brief Initializes ADC pin D2, checks wakeup cause, updates boot count and accumulated time.
   */
  void init();

  /**
   * @brief Reads battery voltage via 2x220k resistor divider on ADC pin D2.
   * @return Voltage in Volts (e.g. 3.85V).
   */
  float getLevel();

  /**
   * @brief Determines power source based on voltage (>4.1V indicates USB).
   * @return POWER_USB (1) or POWER_BAT (0).
   */
  int checkSource();

  /**
   * @brief Gets the current RTC-backed boot count (survives deep sleep).
   * @return Current boot count integer.
   */
  int getBootCount() const;

  /**
   * @brief Gets total accumulated elapsed time in seconds across deep sleep wakeups.
   * @return Accumulated seconds integer.
   */
  uint32_t getTotalElapsedSec() const;

  /**
   * @brief Evaluates current system state and returns the active SleepMode.
   * @return Active SleepMode enum value.
   */
  SleepMode getSleepMode() const;

  /**
   * @brief Called by Sensor when fresh readings are ready to broadcast.
   * @param calibrated True if soil moisture calibration is complete, false otherwise.
   */
  void notifyDataReady(bool calibrated);

  /**
   * @brief Sleep management evaluator — call continuously from main loop().
   *        Stays awake if uncalibrated, waiting for data, or during 10s BLE broadcast window.
   */
  void loop();

private:
  bool          _dataReady;
  bool          _calibrated;
  unsigned long _dataReadyAt;

  void _enterHardwareDeepSleep(unsigned long seconds);
  void _handleReflashSafetyDelay(unsigned long seconds);
  void _incBootCount();
  void _printWakeupReason();
};

#endif // POWERMANAGER_H
