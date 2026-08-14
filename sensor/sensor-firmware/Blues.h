#ifndef BLUES_H
#define BLUES_H

/**
 * @file Blues.h
 * @brief BLE Telemetry Broadcaster (Bluetooth Server / Producer)
 * 
 * @details
 * The BlueS class manages the Bluetooth Low Energy (BLE) stack for the plant sensor node.
 * Instead of establishing active BLE connections (which consume high battery power),
 * BlueS operates in non-connectable broadcast mode:
 * 
 * 1. Primary Advertisement: Broadcasts a static 128-bit Service UUID
 *    ("a1b2c3d4-0001-4c9a-8f2e-5a7b9e3d6f80") so receiver nodes (e.g. Plant Server)
 *    can filter and identify sensor broadcasts without establishing a connection.
 * 
 * 2. Scan Response: Packages live telemetry (moisture %, battery voltage, ADC raw bounds,
 *    power source, boot count) into a 12-byte binary Manufacturer Data payload
 *    which central receivers fetch via active scanning.
 * 
 * @note Lifecycle & Thread Safety:
 * - Driven by Sensor class via setData() whenever fresh readings are sampled.
 * - Non-blocking; loop() is empty as BLE hardware advertising runs asynchronously on ESP32 BLE stack.
 */

#include <BLEAdvertising.h>
#include "Sensor.h" // for SensorData

class BlueS {

public:
  BlueS();
  
  /**
   * @brief Initializes the ESP32 BLE stack, configures advertising parameters, and starts broadcasting.
   */
  void init();

  /**
   * @brief Loop handler (no-op as BLE advertising runs asynchronously in hardware/background task).
   */
  void loop();

  /**
   * @brief Update the scan response payload with fresh telemetry data.
   * @param data Struct containing moisture %, battery voltage, raw bounds, and power status.
   */
  void setData(const SensorData& data);

private:
  BLEAdvertising* _pAdvertising;
  bool       _hasData;
  SensorData _data;

  /**
   * @brief Internal helper to encode telemetry into binary manufacturer data and update the BLE advertisement stack.
   */
  void _updateScanResponse();
};

#endif // BLUES_H
