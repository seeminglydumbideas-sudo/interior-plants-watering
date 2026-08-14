#ifndef BLUE_C_H
#define BLUE_C_H

#include <set>
#include <string>
#include <NimBLEDevice.h>
#include "Store.h"

// Must match the UUID advertised by the sensor nodes.
#define BLUEC_SERVICE_UUID "a1b2c3d4-0001-4c9a-8f2e-5a7b9e3d6f80"

// Extends NimBLEScanCallbacks so NimBLE calls onResult() directly on this object —
// no inner class or owner pointer needed.
class BlueC : public NimBLEScanCallbacks {
public:
  BlueC(Store& store);
  void init();
  void startScan();
  void stopScan();

  void onResult(const NimBLEAdvertisedDevice* device) override;
  void onScanEnd(const NimBLEScanResults& results, int reason) override;

private:
  Store&             _store;
  NimBLEScan*        _pScan;
  bool               _scanningEnabled;
  // MACs that have ever advertised our service UUID.
  // A BLE payload can't hold both a service UUID and manufacturer data simultaneously,
  // so a sensor may send them in separate packets. This set whitelists a MAC on the
  // UUID packet so the subsequent manufacturer-data-only packet is accepted.
  std::set<uint64_t> _seenSensorMacs;
};

#endif // BLUE_C_H
