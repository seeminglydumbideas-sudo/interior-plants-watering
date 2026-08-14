#include "BlueC.h"
#include <Arduino.h>
#include <NimBLEDevice.h>

#define PAYLOAD_LEN 12 // see sensor code
#define COMPANY_ID_LO    0xFF
#define COMPANY_ID_HI    0xFF

// Use a static buffer to keep stack usage minimal
static SensorReading _sharedReading;

BlueC::BlueC(Store& store)
  : _store(store), _pScan(nullptr), _scanningEnabled(false) {
}

// Initialises the NimBLE stack and starts a continuous passive scan.
void BlueC::init() {
  NimBLEDevice::init("");

  _pScan = NimBLEDevice::getScan();
  _pScan->setScanCallbacks(this, true); // BlueC itself is the callback, wantDuplicates = true
  _pScan->setActiveScan(true); // Active scan is required to receive the Scan Response (Manufacturer Data)
  // 0.625 ms is the base time unit mandated by the Bluetooth spec for scan parameters
  //   (derived from the 1.6 kHz classic-BT clock; the value is passed raw to the HCI command).
  // Interval (160 × 0.625 ms = 100 ms): how often a new scan window starts.
  // Window   ( 80 × 0.625 ms =  50 ms): how long the radio actually listens each interval.
  // → 50 % duty cycle: the radio listens for 50 ms then idles for 50 ms.
  // Previously window = interval (100 % duty cycle), which saturated the NimBLE HCI event
  //   queue in busy RF environments, exhausting the mbuf pool and causing LoadProhibited
  //   crashes in ble_hs_hci_frag_alloc(). The 50 ms idle gives NimBLE time to drain its
  //   queue between windows.
  // Sensor coverage: sensors advertise for ~100 ms per event, once every ~6 hours.
  //   A 50 % duty cycle still gives ~50 % probability per advertisement burst of catching
  //   the packet, which is acceptable given the long inter-advertisement interval.
  // WiFi coexistence: Prometheus scrapes every 5 minutes and tolerates retries/misses.
  _pScan->setInterval(160);
  _pScan->setWindow(80);
  // Do NOT call setMaxResults(0). With maxResults=0, NimBLE deletes the device object
  // immediately after onResult() returns (heap churn). Under NVS flash write latency
  // inside _store.update(), the allocator can recycle that memory and corrupt the pointer
  // we still hold, causing a use-after-free crash. The default (0xFF = unlimited) keeps
  // device objects alive in the internal vector for the full scan session, making the
  // pointer stable. The vector is bounded in practice by the number of unique BLE devices
  // in range — not a concern for a fixed home sensor setup.

  // We don't start the scan automatically during init() now.
  // The main sketch will start the scan via startScan() based on WiFi status.
  Serial.println("[INFO ] BlueC NimBLE initialized");
}

void BlueC::startScan() {
  _scanningEnabled = true;
  if (_pScan && !_pScan->isScanning()) {
    Serial.println("[INFO ] BlueC BLE scan starting...");
    _pScan->start(0, false);
  }
}

void BlueC::stopScan() {
  _scanningEnabled = false;
  if (_pScan && _pScan->isScanning()) {
    Serial.println("[INFO ] BlueC BLE scan stopping...");
    _pScan->stop();
  }
}

// Called by NimBLE for every advertisement received.
// Filters by service UUID + company ID, then pushes to Store.
void BlueC::onResult(const NimBLEAdvertisedDevice* device) {
  // Pack the 6-byte MAC into a uint64 for cheap set lookup
  uint64_t addrInt = 0;
  memcpy(&addrInt, device->getAddress().getVal(), 6);

  // A single 31-byte AD payload cannot carry both a service UUID and manufacturer data,
  // so sensors may split them across separate advertisement packets (or alternate).
  // _seenSensorMacs whitelists any MAC that has ever advertised our service UUID so that
  // a later packet from the same MAC carrying only manufacturer data is still accepted.
  if (device->isAdvertisingService(NimBLEUUID(BLUEC_SERVICE_UUID))) {
    _seenSensorMacs.insert(addrInt);
  }

  if (_seenSensorMacs.find(addrInt) == _seenSensorMacs.end()) return; // not a known sensor MAC — skip
  if (!device->haveManufacturerData()) return;

  // getManufacturerData() returns a std::string by value — safe copy, no raw payload access needed.
  // Format: [company ID lo][company ID hi][payload bytes…]
  std::string mfg = device->getManufacturerData();
  if (mfg.size() < PAYLOAD_LEN) return;
  const uint8_t* mData = reinterpret_cast<const uint8_t*>(mfg.data());

  if (mData[0] != COMPANY_ID_LO || mData[1] != COMPANY_ID_HI) return; // not our vendor ID

  // Fill the reading
  std::string name = device->haveName() ? device->getName() : device->getAddress().toString();
  memset(&_sharedReading, 0, sizeof(SensorReading));
  strncpy(_sharedReading.name, name.c_str(), sizeof(_sharedReading.name) - 1);

  _sharedReading.moisture    = (mData[2] == 0xFF) ? -1 : (int)mData[2]; // 0xFF sentinel means sensor not ready
  _sharedReading.battery     = mData[3] / 10.0f;
  _sharedReading.powerSource = mData[4];
  _sharedReading.rawValue    = (uint16_t)(mData[5] | (mData[6] << 8));
  _sharedReading.rawMin      = (uint16_t)(mData[7] | (mData[8] << 8));
  _sharedReading.rawMax      = (uint16_t)(mData[9] | (mData[10] << 8));
  _sharedReading.bootCount   = mData[11];
  _sharedReading.lastSeen    = millis();

  _store.update(_sharedReading, true);

  Serial.printf("[INFO ] BlueC NimBLE [%s] moisture=%d%%, raw=[%d<%d<%d], battery=%.1fV, boot=%d\n",
                _sharedReading.name, _sharedReading.moisture,
                _sharedReading.rawMin, _sharedReading.rawValue, _sharedReading.rawMax,
                _sharedReading.battery, _sharedReading.bootCount);
}

// Watchdog callback: triggers when the BLE controller halts or scan duration ends
void BlueC::onScanEnd(const NimBLEScanResults& results, int reason) {
  Serial.printf("[WARNING] BlueC BLE scan ended (reason: %d).\n", reason);
  if (_scanningEnabled) {
    Serial.println("[INFO ] Restarting BLE scan...");
    _pScan->start(0, false);
  }
}
