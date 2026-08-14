// WiFi + BLE + Display + Buttons isolation test — Gardener and WaterPumps still disabled.

#include <Arduino.h>
#include "Config.h"
#include "Secrets.h"
#include "Store.h"
#include "BlueC.h"
#include "Display.h"
#include "WaterPumps.h"
#include "Gardener.h"
#include <WifiManager.h>
#include <PrometheusServer.h>
#include <AceButton.h>

using namespace ace_button;

Store store;
BlueC bluec(store);
Display display;
WaterPumps pumps(27, 26, 25, 33);
Gardener gardener(pumps, display, 25, COOLDOWN_MS);
WifiManager wifiManager(SECRET_SSID, SECRET_PASS);

static String buildMetrics(PrometheusServer& prom) {
  return store.buildMetrics(prom);
}
PrometheusServer promSrv(buildMetrics);

ButtonConfig buttonConfig;
AceButton btn1(&buttonConfig, 14); // DOWN
AceButton btn2(&buttonConfig, 0);  // UP (BOOT)

static UIMode _uiMode  = RUN;

static void setUIMode(UIMode mode) {
  if (_uiMode != mode) {
    _uiMode = mode;
    const char* modeStr = "";
    switch (_uiMode) {
      case RUN:                 modeStr = "Mode: RUN"; break;
      case SETUP_NAV:           modeStr = "Mode: SETUP NAV"; break;
      case SENSOR_SETUP:        modeStr = "Mode: SENSOR SETUP"; break;
      case SENSOR_SETTINGS_NAV: modeStr = "Mode: SETTINGS NAV"; break;
      case SENSOR_SETTING_EDIT: modeStr = "Mode: SETTING EDIT"; break;
      case WATERING_SETUP:      modeStr = "Mode: WATER SETUP"; break;
    }
    display.log(modeStr);
    Serial.println(modeStr);
  }
}
static int    _setupBox  = 1;
static int    _setupStep = 0;
static int    _activeSettingLine = 1; // 1=Pump, 2=Threshold, 3=Duration, 4=Delete
static bool   _deleteConfirm = false;
static SensorReading _tempConfig;

static int           _sensorIdx          = 0;
static unsigned long _lastSensorRefresh  = 0;
static const unsigned long SENSOR_REFRESH_MS = 15000;

static const char* getHumidityStateStr(HumidityState state) {
  switch (state) {
    case HUMIDITY_DRY:      return "DRY";
    case HUMIDITY_THIRSTY:  return "THIRSTY";
    case HUMIDITY_HEALTHY:  return "HEALTHY";
    case HUMIDITY_UNKNOWN:  
    default:                return "UNKNOWN";
  }
}

static const char* getWateringStateStr(WateringState state) {
  switch (state) {
    case WATERING_ACTIVE:          return "ACTIVE";
    case WATERING_COOLDOWN:        return "COOLDOWN";
    case WATERING_WAIT_MEASUREMENT: return "WAIT_MEASUREMENT";
    case WATERING_IDLE:
    default:                       return "IDLE";
  }
}

static void _logSensorState(const SensorReading& r) {
  if (ENABLE_DEBUG_GARDENER) {
    char msg[64];
    snprintf(msg, sizeof(msg), "%s: H:%s W:%s", r.name, getHumidityStateStr(r.humidityState), getWateringStateStr(r.wateringState));
    display.log(msg);
  }
}

static void _showCurrentSensor(bool loggedByButton = false) {
  store.lock();
  const auto& all = store.getAll();
  if (all.empty()) {
    display.showNoSensors();
    store.unlock();
    return;
  }
  if (_sensorIdx >= (int)all.size()) _sensorIdx = 0;
  if (_sensorIdx < 0) _sensorIdx = (int)all.size() - 1;
  auto it = all.begin();
  std::advance(it, _sensorIdx);

  if (_uiMode == SENSOR_SETTINGS_NAV || _uiMode == SENSOR_SETTING_EDIT) {
    display.showSensor(_tempConfig, _sensorIdx + 1, (int)all.size());
    display.setDeleteConfirm(_deleteConfirm);
    display.setSettingsFocus(_activeSettingLine, _uiMode == SENSOR_SETTING_EDIT);
  } else {
    display.showSensor(it->second, _sensorIdx + 1, (int)all.size());
    display.setDeleteConfirm(false);
    display.setSettingsFocus(0, false);
  }

  display.setSelectedPlant(_sensorIdx);
  _lastSensorRefresh = millis();

  if (loggedByButton) {
    _logSensorState(it->second);
  }

  store.unlock();
}

static void onButtonEvent(AceButton* btn, uint8_t eventType, uint8_t /*buttonState*/) {
  display.wake();
  Serial.println("click");
  if (_uiMode == RUN) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 0) {
        // UP button released: cycle forward through sensors
        _sensorIdx++;
        _showCurrentSensor(true);
      }
    } else if (eventType == AceButton::kEventLongPressed && btn->getPin() == 14) {
      // DOWN button long-pressed: enter box selection mode
      setUIMode(SETUP_NAV);
      _setupBox = 1;
      display.highlightBox(_setupBox);
    }
  } else if (_uiMode == SETUP_NAV) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 0) {
        // UP button released: cycle highlighted boxes indefinitely
        _setupBox++;
        if (_setupBox > 3) {
          _setupBox = 1;
        }
        display.highlightBox(_setupBox);
      } else if (btn->getPin() == 14) {
        // DOWN button released: select box
        if (_setupBox == 1) {
          setUIMode(SENSOR_SETUP);
          display.highlightBox(1);
          display.setSettingsMode(true);
          _showCurrentSensor();
        } else {
          setUIMode(RUN);
          display.highlightBox(0);
          display.setSettingsMode(false);
          _showCurrentSensor();
        }
      }
    } else if (eventType == AceButton::kEventLongPressed) {
      // Long-press either button: exit box selection mode
      setUIMode(RUN);
      display.highlightBox(0);
      display.setSettingsMode(false);
      _showCurrentSensor();
    }
  } else if (_uiMode == SENSOR_SETUP) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 14) {
        // DOWN button released: enter settings navigation mode for the current sensor
        store.lock();
        const auto& all = store.getAll();
        if (!all.empty()) {
          auto it = all.begin();
          std::advance(it, _sensorIdx);
          _tempConfig = it->second;
          _deleteConfirm = false;
          setUIMode(SENSOR_SETTINGS_NAV);
          _activeSettingLine = 1; // start on Pump
          _showCurrentSensor();
        }
        store.unlock();
      } else if (btn->getPin() == 0) {
        // UP button released: cycle through sensors
        _sensorIdx++;
        _showCurrentSensor(true);
      }
    } else if (eventType == AceButton::kEventLongPressed) {
      // Long-press either button: exit to run mode
      setUIMode(RUN);
      display.highlightBox(0);
      display.setSettingsMode(false);
      _showCurrentSensor();
    }
  } else if (_uiMode == SENSOR_SETTINGS_NAV) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 0) {
        // UP button released: cycle active setting line
        _activeSettingLine++;
        if (_activeSettingLine > 4) {
          _activeSettingLine = 1;
        }
        _showCurrentSensor();
      } else if (btn->getPin() == 14) {
        // DOWN button released: select setting to edit
        if (_activeSettingLine == 4) {
          _deleteConfirm = false;
        }
        setUIMode(SENSOR_SETTING_EDIT);
        _showCurrentSensor();
      }
    } else if (eventType == AceButton::kEventLongPressed) {
      // Long-press either button: exit back to SENSOR_SETUP
      setUIMode(SENSOR_SETUP);
      _showCurrentSensor();
    }
  } else if (_uiMode == SENSOR_SETTING_EDIT) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 0) {
        // UP button released: increment value / toggle delete confirm (looping)
        if (_activeSettingLine == 1) {
          _tempConfig.pumpIndex++;
          if (_tempConfig.pumpIndex > 3) _tempConfig.pumpIndex = -1;
        } else if (_activeSettingLine == 2) {
          _tempConfig.moistureThreshold += 5;
          if (_tempConfig.moistureThreshold > 100) _tempConfig.moistureThreshold = 0;
        } else if (_activeSettingLine == 3) {
          _tempConfig.durationSeconds += 5;
          if (_tempConfig.durationSeconds > 60) _tempConfig.durationSeconds = 5;
        } else if (_activeSettingLine == 4) {
          _deleteConfirm = !_deleteConfirm;
        }
        _showCurrentSensor();
      } else if (btn->getPin() == 14) {
        // DOWN button released: save value / delete sensor
        if (_activeSettingLine == 4) {
          if (_deleteConfirm) {
            // Delete sensor!
            store.remove(_tempConfig.name);
            _sensorIdx = 0;
            
            store.lock();
            bool empty = store.getAll().empty();
            store.unlock();

            if (empty) {
              setUIMode(RUN);
              display.setSettingsMode(false);
            } else {
              setUIMode(SENSOR_SETUP);
            }
            _showCurrentSensor();
            return;
          } else {
            setUIMode(SENSOR_SETTINGS_NAV);
            _showCurrentSensor();
          }
        } else {
          _tempConfig.lastSeen = 0; // sentinel: config-only update
          store.update(_tempConfig);
          setUIMode(SENSOR_SETTINGS_NAV);
          _showCurrentSensor();
        }
      }
    } else if (eventType == AceButton::kEventLongPressed) {
      // Long-press either button: discard edits by reloading from store
      if (_activeSettingLine == 4) {
        _deleteConfirm = false;
      } else {
        store.lock();
        const auto& all = store.getAll();
        if (!all.empty()) {
          auto it = all.begin();
          std::advance(it, _sensorIdx);
          _tempConfig = it->second;
        }
        store.unlock();
      }
      setUIMode(SENSOR_SETTINGS_NAV);
      _showCurrentSensor();
    }
  } else if (_uiMode == WATERING_SETUP) {
    if (eventType == AceButton::kEventReleased) {
      if (btn->getPin() == 0) {
        if (_setupStep == 0) {
          _tempConfig.pumpIndex++;
          if (_tempConfig.pumpIndex > 3) _tempConfig.pumpIndex = -1;
        } else if (_setupStep == 1) {
          _tempConfig.moistureThreshold += 5;
          if (_tempConfig.moistureThreshold > 100) _tempConfig.moistureThreshold = 0;
        } else if (_setupStep == 2) {
          _tempConfig.durationSeconds += 5;
          if (_tempConfig.durationSeconds > 60) _tempConfig.durationSeconds = 5;
        }
      } else {
        _setupStep++;
        if (_setupStep > 2) {
          _tempConfig.lastSeen = 0; // sentinel: config-only update, preserve BLE data
          store.update(_tempConfig);
          setUIMode(RUN);
          display.highlightBox(0);
          _showCurrentSensor();
        }
      }
    }
  }
}

static void onWifiStateChange(WifiManager::State state) {
  if (state == WifiManager::CONNECTED) {
    char msg[48];
    snprintf(msg, sizeof(msg), "IP: %s", WiFi.localIP().toString().c_str());
    display.log(msg);
    bluec.startScan(); // WiFi is connected, resume BLE scanning
  } else if (state == WifiManager::CONNECTING) {
    bluec.stopScan();  // WiFi is actively connecting, pause BLE scanning to reduce RF contention
  } else if (state == WifiManager::FAILED || state == WifiManager::DISCONNECTED) {
    if (state == WifiManager::FAILED) {
      display.log("WiFi lost");
    }
    bluec.startScan(); // WiFi is in waiting/back-off phase, keep BLE active for local monitoring
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  // LilyGo T-Display-S3: LCD power rail
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  Serial.println("[BOOT] WiFi+BLE+Display+Buttons isolation test");
  store.begin();
  display.init();
  gardener.init();
  bluec.init(); // Initialize BLE first so state changes in wifiManager can safely trigger BLE scan actions
  wifiManager.onStateChange(onWifiStateChange);
  wifiManager.init();
  promSrv.init();

  pinMode(14, INPUT_PULLUP);
  pinMode(0,  INPUT_PULLUP);
  buttonConfig.setEventHandler(onButtonEvent);
  buttonConfig.setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfig.setFeature(ButtonConfig::kFeatureSuppressAfterLongPress);

  _showCurrentSensor();
  Serial.println("[BOOT] Running");
}

void loop() {
  if (_uiMode == RUN) {
    display.setButtonLabels("CYCLE", "", "", "SETUP");
  } else if (_uiMode == SETUP_NAV) {
    display.setButtonLabels("CYCLE", "EXIT", "SELECT", "EXIT");
  } else if (_uiMode == SENSOR_SETUP) {
    display.setButtonLabels("CYCLE", "EXIT", "SELECT", "EXIT");
  } else if (_uiMode == SENSOR_SETTINGS_NAV) {
    display.setButtonLabels("CYCLE", "EXIT", "EDIT", "EXIT");
  } else if (_uiMode == SENSOR_SETTING_EDIT) {
    display.setButtonLabels("CHANGE", "CANCEL", "SAVE", "CANCEL");
  } else if (_uiMode == WATERING_SETUP) {
    display.setButtonLabels("CHANGE", "", "NEXT", "");
  }

  btn1.check();
  btn2.check();
  wifiManager.loop();
  promSrv.loop();
  gardener.loop(store);

  if (!display.isDisplayOff()) {
    if (_uiMode == RUN) {
      if (millis() - _lastSensorRefresh >= SENSOR_REFRESH_MS) {
        _sensorIdx++;
        _showCurrentSensor();
      }
      display.update(store);
    } else if (_uiMode == WATERING_SETUP) {
      display.showWateringSetup(_tempConfig, _setupStep);
    } else {
      display.update(store);
    }
  } else {
    display.update(store);
  }
}
