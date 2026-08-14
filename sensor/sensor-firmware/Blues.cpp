#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include "Blues.h"
#include "Secrets.h"

// Company ID 0xFFFF is reserved for internal / testing use (Bluetooth SIG).
#define BLE_COMPANY_ID_LO 0xFF
#define BLE_COMPANY_ID_HI 0xFF

// Custom 128-bit service UUID identifying this soil-moisture sensor service.
// BLE clients can filter advertisements by this UUID.
#define BLE_SERVICE_UUID "a1b2c3d4-0001-4c9a-8f2e-5a7b9e3d6f80"

BlueS::BlueS() : _pAdvertising(nullptr), _hasData(false), _data({-1, 0.0f, 0, 0, 0, 0, 0}) {
}

// Encodes the internal _data struct into a binary Manufacturer Data payload (12 bytes)
// and updates the BLE hardware controller's Scan Response data buffer.
// 
// How & Why it is broadcasted:
// 1. Primary advertisement (setAdvertisementData) continuously broadcasts our static Service UUID.
// 2. setScanResponseData(scanResponse) configures the BLE controller's secondary response buffer
//    with the sensor device name (SENSOR_ID) and the encoded 12-byte telemetry payload.
// 3. When an active BLE scanner (e.g. Plant Server / BlueC) hears the primary advertisement, it sends
//    a "SCAN_REQ" packet. The ESP32 BLE hardware automatically transmits a "SCAN_RSP" packet back
//    containing this payload — allowing live telemetry to be broadcast without establishing a connection.
void BlueS::_updateScanResponse() {
  if (!_pAdvertising) return;

  // Scan response manufacturer data layout:
  //   Byte 0-1   : company ID (little-endian, 0xFFFF = testing)
  //   Byte 2     : moisture percentage (0-100), 0xFF = not yet calibrated
  //   Byte 3     : battery voltage * 10 (e.g. 38 = 3.8 V)
  //   Byte 4     : power source (0 = battery, 1 = USB)
  //   Byte 5-6   : rawValue (uint16_t little-endian)
  //   Byte 7-8   : rawMin   (uint16_t little-endian)
  //   Byte 9-10  : rawMax   (uint16_t little-endian)
  //   Byte 11    : bootCount (wraps at 255)
  uint16_t rawValue = (uint16_t)_data.rawValue;
  uint16_t rawMin   = (uint16_t)_data.rawMin;
  uint16_t rawMax   = (uint16_t)_data.rawMax;

  // Use a binary-safe byte array instead of a raw signed char array or standard C-string functions.
  // Converting via c_str() truncates at the first 0x00 (null) byte.
  // On battery power, byte 4 (powerSource) is 0x00, truncating the payload to 4 bytes and causing the server to ignore it.
  uint8_t buf[12];
  buf[0]  = BLE_COMPANY_ID_LO;
  buf[1]  = BLE_COMPANY_ID_HI;
  buf[2]  = (_data.moisture < 0 ? 0xFF : (uint8_t)constrain(_data.moisture, 0, 100));
  buf[3]  = (uint8_t)constrain((int)(_data.battery * 10.0f), 0, 255);
  buf[4]  = (uint8_t)_data.powerSource;
  buf[5]  = (uint8_t)(rawValue & 0xFF);
  buf[6]  = (uint8_t)(rawValue >> 8);
  buf[7]  = (uint8_t)(rawMin   & 0xFF);
  buf[8]  = (uint8_t)(rawMin   >> 8);
  buf[9]  = (uint8_t)(rawMax   & 0xFF);
  buf[10] = (uint8_t)(rawMax   >> 8);
  buf[11] = (uint8_t)_data.bootCount;

  String payload = String((char*)buf, 12);

  BLEAdvertisementData scanResponse;
  scanResponse.setName(SENSOR_ID);
  scanResponse.setManufacturerData(payload);
  _pAdvertising->setScanResponseData(scanResponse);
}

void BlueS::setData(const SensorData& data) {
  // Store a local member copy: SensorData is passed from a temporary stack variable in Sensor::loop().
  // Copying by value prevents dangling reference issues and retains telemetry state for BLE updates.
  _data = data;
  _hasData = true;
  _updateScanResponse(); // Update advertising hardware immediately with fresh telemetry
  Serial.printf("[DEBUG] BLE data updated [%s]: moisture=%d%%, battery=%.1fV, power=%d\n",
                SENSOR_ID, data.moisture, data.battery, data.powerSource);
}

void BlueS::init() {
  BLEDevice::init(SENSOR_ID); // this SENSOR_ID will not be sent in broadcast.
  _pAdvertising = BLEDevice::getAdvertising();
  _pAdvertising->setScanResponse(true);
  _pAdvertising->setMinPreferred(0x00); // disable connection hints

  // Advertisement: UUID only. Static — set once and never changed.
  // Clients use this to filter and identify the sensor without connecting.
  BLEAdvertisementData advData;
  advData.setCompleteServices(BLEUUID(BLE_SERVICE_UUID));
  _pAdvertising->setAdvertisementData(advData);

  // Set initial scan response telemetry data before starting broadcast
  _updateScanResponse();

  _pAdvertising->start();
  Serial.println("[INFO ] BLE advertising started");
}

void BlueS::loop() {
  // BLE advertising updates are driven by setData() events to eliminate redundant register writes
}
