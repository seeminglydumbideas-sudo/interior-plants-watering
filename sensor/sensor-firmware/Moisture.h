#ifndef MOISTURE_H
#define MOISTURE_H

/**
 * @file Moisture.h
 * @brief Non-blocking Soil Moisture Sensor Driver with Auto-Calibration and Power Gating.
 * 
 * @details
 * The Moisture class manages the soil moisture probe lifecycle on the ESP32:
 * 
 * 1. **Power Gating:** Powers the analog sensor via GPIO D0 (`SENSOR_POWER`) only when sampling
 *    to prevent galvanic probe corrosion and save power during sleep intervals.
 * 
 * 2. **Async State Machine:** Executes non-blockingly via loop():
 *    - `IDLE`: Sensor powered down.
 *    - `STABILIZING`: Sensor powered up, waiting 400ms for analog voltage to settle.
 *    - `SAMPLING`: Samples analog input on GPIO D1 (`SENSOR_PIN`) 5 times at 500ms intervals.
 *    - `COMPLETE`: Sensor powered off, reading computed, ready to be fetched via getReading().
 * 
 * 3. **Auto-Calibration & Persistence:** Continuously updates observed min/max ADC values (`rawMin`, `rawMax`)
 *    in Non-Volatile Storage (NVS preferences namespace `"moisture"`). Ignores noise readings (ADC <= 500).
 *    Inverts raw values so higher moisture yields a higher percentage.
 */
class Moisture {

public:
  /**
   * @brief Struct containing computed soil moisture percentage and raw ADC data.
   */
  struct Reading {
    int percentage; /**< Calculated soil moisture % (0-100), or -1 if bounds are uncalibrated */
    int rawValue;   /**< Raw ADC voltage reading from the sensor pin (0-4095) */
    int rawMin;     /**< Minimum observed raw ADC bound */
    int rawMax;     /**< Maximum observed raw ADC bound */
  };

  Moisture();

  /**
   * @brief Initializes sensor pins and loads persisted raw min/max calibration bounds from NVS.
   */
  void init();

  /**
   * @brief Powers on the sensor and initiates an asynchronous, non-blocking sampling cycle.
   */
  void startRead();

  /**
   * @brief Non-blocking state machine stepper. Call from main loop() while sampling.
   */
  void loop();

  /**
   * @brief Checks if a fresh measurement cycle has finished.
   * @return true if fresh Reading is ready, false otherwise.
   */
  bool isReady() const;

  /**
   * @brief Checks if the sensor is in the IDLE state (powered down and ready for startRead()).
   * @return true if idle, false if a measurement is in progress or awaiting retrieval.
   */
  bool isIdle() const;

  /**
   * @brief Fetches the latest computed Reading and resets the sensor state to IDLE.
   * @return Reading struct with percentage, rawValue, rawMin, and rawMax.
   */
  Reading getReading();

  /**
   * @brief Clears stored raw ADC calibration bounds from NVS flash and resets to defaults.
   */
  void resetCalibration();

private:
  enum State { IDLE, STABILIZING, SAMPLING, COMPLETE };

  State         _state;
  unsigned long _stateTimer;
  int           _sampleCount;
  int           _rawValue;
  int           _rawMin;
  int           _rawMax;
  bool          _ready;
  Reading       _lastReading;

  void _finalize();
  void _saveCalibration();
};

#endif // MOISTURE_H
