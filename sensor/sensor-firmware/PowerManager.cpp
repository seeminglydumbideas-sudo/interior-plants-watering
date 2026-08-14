#include <Arduino.h>
#include <esp_sleep.h>
#include <BLEDevice.h>
#include "PowerManager.h"
#include "Config.h"

/*
Use 2x220k resistor on an analog port to read voltage value (battery or USB)
ref: https://wiki.seeedstudio.com/check_battery_voltage/
*/

// RTC RAM variables: survive deep sleep, reset on cold power cycle.
RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR uint32_t totalElapsedSec = 0;

float PowerManager::getLevel() {
  uint32_t Vbatt = 0;
  for (int i = 0; i < 16; i++) {
    Vbatt += analogReadMilliVolts(D2); // ADC with correction
  }
  return 2 * Vbatt / 16 / 1000.0; // attenuation ratio 1/2, mV --> V
}

int PowerManager::checkSource() {
  return getLevel() > 4.1 ? POWER_USB : POWER_BAT;
}

int PowerManager::getBootCount() const {
  return bootCount;
}

uint32_t PowerManager::getTotalElapsedSec() const {
  return totalElapsedSec;
}

void PowerManager::_incBootCount() {
  bootCount++;
}

void PowerManager::init() {
  Serial.printf("[INFO ] Boot #%d (total elapsed: %lu min / %lu sec)\n",
                getBootCount(), (unsigned long)(getTotalElapsedSec() / 60), (unsigned long)getTotalElapsedSec());
  pinMode(A2, INPUT); // ADC on port D2
  _dataReady  = false;
  _calibrated = false;
  _printWakeupReason();
  _incBootCount();
}

SleepMode PowerManager::getSleepMode() const {
  if (getBootCount() == 1 && millis() < REFLASH_SAFETY_WINDOW_MS) {
    return SleepMode::REFLASH_SAFETY_DELAY;
  }
  if (getTotalElapsedSec() < SHORT_SLEEP_WINDOW_SEC) {
    return SleepMode::SHORT_DEEP_SLEEP;
  }
  return SleepMode::LONG_DEEP_SLEEP;
}

void PowerManager::notifyDataReady(bool calibrated) {
  _dataReady   = true;
  _dataReadyAt = millis();
  _calibrated  = calibrated;
}

#define BROADCAST_WINDOW_MS (10 * 1000UL) // stay awake for clients to scan

void PowerManager::loop() {
  if (!_calibrated) return; // sensor not yet calibrated — stay awake
  if (!_dataReady)  return; // no reading ready yet
  if (millis() - _dataReadyAt < BROADCAST_WINDOW_MS) return; // broadcast window
  _dataReady = false;

  switch (getSleepMode()) {
    case SleepMode::REFLASH_SAFETY_DELAY:
      _handleReflashSafetyDelay(SHORT_SLEEP_DURATION_SEC);
      break;
    case SleepMode::SHORT_DEEP_SLEEP:
      _enterHardwareDeepSleep(SHORT_SLEEP_DURATION_SEC);
      break;
    case SleepMode::LONG_DEEP_SLEEP:
      _enterHardwareDeepSleep(LONG_SLEEP_DURATION_SEC);
      break;
  }
}

void PowerManager::_handleReflashSafetyDelay(unsigned long seconds) {
  unsigned long remaining = (millis() < REFLASH_SAFETY_WINDOW_MS)
                          ? (REFLASH_SAFETY_WINDOW_MS - millis()) / 1000
                          : 0;
  Serial.printf("[INFO ] [REFLASH_SAFETY_DELAY] Delaying %lus (reflash window: %lus left)\n", seconds, remaining);

  // Stop BLE advertising to simulate deep sleep during this delay
  BLEDevice::getAdvertising()->stop();

  // Accumulate delay into total elapsed time
  totalElapsedSec += seconds;

  delay(seconds * 1000);
}

void PowerManager::_enterHardwareDeepSleep(unsigned long seconds) {
  if (seconds >= 60) {
    Serial.printf("[INFO ] [%s] Entering hardware deep sleep for %ldmin (elapsed: %lu min / %lu min window)\n",
                  getSleepMode() == SleepMode::SHORT_DEEP_SLEEP ? "SHORT_DEEP_SLEEP" : "LONG_DEEP_SLEEP",
                  seconds / 60, (unsigned long)(getTotalElapsedSec() / 60), (unsigned long)(SHORT_SLEEP_WINDOW_SEC / 60));
  } else {
    Serial.printf("[INFO ] [%s] Entering hardware deep sleep for %lusec (elapsed: %lusec)\n",
                  getSleepMode() == SleepMode::SHORT_DEEP_SLEEP ? "SHORT_DEEP_SLEEP" : "LONG_DEEP_SLEEP",
                  seconds, (unsigned long)getTotalElapsedSec());
  }

  // Accumulate active runtime + sleep duration into RTC time tracking
  totalElapsedSec += (millis() / 1000) + seconds;

  esp_sleep_enable_timer_wakeup(seconds * 1000000ULL);
  delay(100); // flush serial output before sleeping
  esp_deep_sleep_start();
}

void PowerManager::_printWakeupReason() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  switch (reason) {
    case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("[DEBUG] Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("[DEBUG] Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("[DEBUG] Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("[DEBUG] Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP:      Serial.println("[DEBUG] Wakeup caused by ULP program"); break;
    default: Serial.printf("[DEBUG] Wakeup was not caused by deep sleep: %d\n", reason); break;
  }
}
