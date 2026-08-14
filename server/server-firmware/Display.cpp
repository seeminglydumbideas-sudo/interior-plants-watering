#include <SimpleLogging.h>
#include "Display.h"
#include "Config.h"

void Display::init() {
  _redrawLog        = true;
  _redrawSensor     = true;
  _sensorDisp.valid = false;
  _highlightedBox   = 0;
  _settingsMode     = false;
  _focusedLine      = 0;
  _isEditingSetting = false;
  _deleteConfirm    = false;
  logBuf[0][0]      = '\0';
  logBuf[1][0]      = '\0';
  _upShort[0]       = '\0';
  _upLong[0]        = '\0';
  _downShort[0]     = '\0';
  _downLong[0]      = '\0';
  _redrawSidebarFlag = true;
  _sidebarSprite    = new TFT_eSprite(&_tft);
  _sidebarSprite->createSprite(85, 10);
  _sidebarSprite->setColorDepth(8);
  _sidebarSprite->setFreeFont(NULL); // Zero-initialize GFXfont heap garbage pointer

  _box1Sprite       = new TFT_eSprite(&_tft);
  _box1Sprite->createSprite(BOX1_W, BOX1_H);
  _box1Sprite->setColorDepth(16);
  _box1Sprite->setFreeFont(NULL); // Zero-initialize GFXfont heap garbage pointer

  _box3Sprite       = new TFT_eSprite(&_tft);
  _box3Sprite->createSprite(BOX3_W, BOX3_H);
  _box3Sprite->setColorDepth(16);
  _box3Sprite->setFreeFont(NULL); // Zero-initialize GFXfont heap garbage pointer

  _light = new LightDisplay(_tft, BOX2_X, BOX2_Y, BOX2_W, BOX2_H);

  LOG_INFO("Initializing display controller...");
  Serial.flush();
  delay(100);

  // 1. Initialize TFT
  // NOTE: If this crashes/disconnects, check deploy.sh build flags.
  _tft.init();
  _tft.setRotation(3);
  _tft.fillScreen(TFT_BLACK);
  
  // 2. Setup Backlight AFTER TFT init
  _lastActivity = millis();
  _currentBrightness = BRIGHT_FULL;
  pinMode(BACKLIGHT_PIN, OUTPUT);
  // Use analogWrite instead of digitalWrite to ensure consistency with dimming
  analogWrite(BACKLIGHT_PIN, BRIGHT_FULL); 
  
  _clear_box1();
  if (_light) _light->init();
  _clear_box3();
  _drawSidebar();
  
  LOG_INFO("Display init done");
  Serial.flush();
}

void Display::wake() {
  _lastActivity = millis();
  if (_currentBrightness != BRIGHT_FULL) {
    _currentBrightness = BRIGHT_FULL;
    // analogWrite instead of digitalWrite: once analogWrite has been called to dim,
    // the pin stays in PWM mode and digitalWrite no longer takes effect.
    analogWrite(BACKLIGHT_PIN, BRIGHT_FULL);
    LOG_INFO("Display: Waking up");
  }
}

bool Display::isDisplayOff() {
  return (_currentBrightness == BRIGHT_OFF);
}

void Display::log(const char* message) {
  wake(); // Wake on new log
  strncpy(logBuf[0], logBuf[1], LOG_COLS);
  strncpy(logBuf[1], message, LOG_COLS - 1);
  logBuf[1][LOG_COLS - 1] = '\0';
  _redrawLog = true;
}

void Display::showSensor(const SensorReading& r, int idx, int total) {
  _sensorDisp.reading = r;
  _sensorDisp.idx     = idx;
  _sensorDisp.total   = total;
  _sensorDisp.valid   = true;
  _redrawSensor       = true;
}

void Display::showNoSensors() {
  _sensorDisp.valid   = false;
  _redrawSensor       = true;
}

void Display::setPumpActive(int pumpIdx, bool active, uint32_t durationMs) {
  if (active) wake(); // Wake on watering start
  if (_light) _light->setPumpActive(pumpIdx, active, durationMs);
}

void Display::setCooldownActive(int pumpIdx, bool active) {
  if (_light) _light->setCooldownActive(pumpIdx, active);
}

void Display::setSelectedPlant(int plantIdx) {
  if (_light) _light->setSelectedPlant(plantIdx);
}

void Display::highlightBox(int boxId) {
  wake();
  _highlightedBox = boxId;
  _redrawSensor = true; // Trigger redraw
  _redrawLog = true;
  _tft.drawRoundRect(BOX2_X, BOX2_Y, BOX2_W, BOX2_H, 4, _highlightedBox == 2 ? TFT_RED : TFT_DARKGREY);
}

void Display::setSettingsMode(bool enabled) {
  if (_settingsMode != enabled) {
    _settingsMode = enabled;
    _redrawSensor = true;
  }
}

void Display::setSettingsFocus(int lineIdx, bool isEditing) {
  if (_focusedLine != lineIdx || _isEditingSetting != isEditing) {
    _focusedLine = lineIdx;
    _isEditingSetting = isEditing;
    _redrawSensor = true;
  }
}

void Display::setDeleteConfirm(bool confirm) {
  if (_deleteConfirm != confirm) {
    _deleteConfirm = confirm;
    _redrawSensor = true;
  }
}

void Display::_clear_box1() {
  _tft.fillRect(BOX1_X, BOX1_Y, BOX1_W, BOX1_H, TFT_BLACK);
  _tft.drawRoundRect(BOX1_X, BOX1_Y, BOX1_W, BOX1_H, 4, _highlightedBox == 1 ? TFT_RED : TFT_DARKGREY);
}
void Display::_clear_box2() {
  _tft.fillRect(BOX2_X, BOX2_Y, BOX2_W, BOX2_H, TFT_BLACK);
  _tft.drawRoundRect(BOX2_X, BOX2_Y, BOX2_W, BOX2_H, 4, _highlightedBox == 2 ? TFT_RED : TFT_DARKGREY);
}
void Display::_clear_box3() {
  _tft.fillRect(BOX3_X, BOX3_Y, BOX3_W, BOX3_H, TFT_BLACK);
  _tft.drawRoundRect(BOX3_X, BOX3_Y, BOX3_W, BOX3_H, 4, _highlightedBox == 3 ? TFT_RED : TFT_DARKGREY);
}

void Display::_updateLog() {
  if (!_redrawLog) return;
  _redrawLog = false;
  
  _box3Sprite->fillSprite(TFT_BLACK);
  _box3Sprite->drawRoundRect(0, 0, BOX3_W, BOX3_H, 4, _highlightedBox == 3 ? TFT_RED : TFT_DARKGREY);
  
  _box3Sprite->setTextColor(COLOR_DRKGREEN, TFT_BLACK);
  _box3Sprite->setTextSize(1);
  _box3Sprite->drawString(logBuf[0], 6, 4);
  _box3Sprite->drawString(logBuf[1], 6, 16);
  
  _box3Sprite->pushSprite(BOX3_X, BOX3_Y);
}

void Display::_updateSensor() {
  if (!_redrawSensor) return;
  _redrawSensor = false;

  // Clear off-screen buffer and draw border in local coordinates (0, 0)
  _box1Sprite->fillSprite(TFT_BLACK);
  _box1Sprite->drawRoundRect(0, 0, BOX1_W, BOX1_H, 4, _highlightedBox == 1 ? TFT_RED : TFT_DARKGREY);

  _box1Sprite->setTextSize(1);
  if (!_sensorDisp.valid) {
    _box1Sprite->setTextColor(TFT_DARKGREY, TFT_BLACK);
    _box1Sprite->drawString("No sensors", 8, 46);
    _box1Sprite->pushSprite(BOX1_X, BOX1_Y);
    return;
  }
  const SensorReading& r = _sensorDisp.reading;
  
  // Header is grey in SENSOR_SETTING_EDIT mode, otherwise white
  uint16_t headerColor = TFT_WHITE;
  if (_settingsMode && _focusedLine > 0 && _isEditingSetting) {
    headerColor = TFT_DARKGREY;
  }
  _box1Sprite->setTextColor(headerColor, TFT_BLACK);
  _box1Sprite->drawString(r.name, 6, 8);

  // Index indicator — right-aligned
  char idxBuf[8];
  _box1Sprite->setTextColor(headerColor, TFT_BLACK);
  snprintf(idxBuf, sizeof(idxBuf), "%d/%d", _sensorDisp.idx, _sensorDisp.total);
  _box1Sprite->drawString(idxBuf, BOX1_W - _box1Sprite->textWidth(idxBuf) - 6, 8);

  // Value lines - Spaced out more for 140px height
  char line[32];
  _box1Sprite->setFreeFont(&FreeSansBold9pt7b);
  
  if (_settingsMode) {
    // Switch to smaller built-in system font for settings mode
    _box1Sprite->setFreeFont(NULL);
    _box1Sprite->setTextSize(1);

    uint16_t iconColor1, textColor1;
    uint16_t iconColor2, textColor2;
    uint16_t iconColor3, textColor3;
    uint16_t iconColor4, textColor4;

    if (_focusedLine > 0) {
      if (_isEditingSetting) {
        // SENSOR_SETTING_EDIT mode: active symbol grey, value white; inactive darker grey (0x4208)
        iconColor1 = (_focusedLine == 1) ? TFT_DARKGREY : 0x4208;
        textColor1 = (_focusedLine == 1) ? TFT_WHITE    : 0x4208;

        iconColor2 = (_focusedLine == 2) ? TFT_DARKGREY : 0x4208;
        textColor2 = (_focusedLine == 2) ? TFT_WHITE    : 0x4208;

        iconColor3 = (_focusedLine == 3) ? TFT_DARKGREY : 0x4208;
        textColor3 = (_focusedLine == 3) ? TFT_WHITE    : 0x4208;

        iconColor4 = (_focusedLine == 4) ? TFT_DARKGREY : 0x4208;
        textColor4 = (_focusedLine == 4) ? TFT_WHITE    : 0x4208;
      } else {
        // SENSOR_SETTINGS_NAV mode: active symbol & value white; inactive grey (TFT_DARKGREY)
        iconColor1 = (_focusedLine == 1) ? TFT_WHITE : TFT_DARKGREY;
        textColor1 = (_focusedLine == 1) ? TFT_WHITE : TFT_DARKGREY;

        iconColor2 = (_focusedLine == 2) ? TFT_WHITE : TFT_DARKGREY;
        textColor2 = (_focusedLine == 2) ? TFT_WHITE : TFT_DARKGREY;

        iconColor3 = (_focusedLine == 3) ? TFT_WHITE : TFT_DARKGREY;
        textColor3 = (_focusedLine == 3) ? TFT_WHITE : TFT_DARKGREY;

        iconColor4 = (_focusedLine == 4) ? TFT_WHITE : TFT_DARKGREY;
        textColor4 = (_focusedLine == 4) ? TFT_WHITE : TFT_DARKGREY;
      }
    } else {
      // SENSOR_SETUP mode: all in grey
      iconColor1 = textColor1 = TFT_DARKGREY;
      iconColor2 = textColor2 = TFT_DARKGREY;
      iconColor3 = textColor3 = TFT_DARKGREY;
      iconColor4 = textColor4 = TFT_DARKGREY;
    }

    // Line 1: Associated Pump (Y shifted to 32, icon to 42)
    _drawPump(*_box1Sprite, 12, 42, iconColor1);
    _box1Sprite->setTextColor(textColor1, TFT_BLACK);
    if (r.pumpIndex == -1) snprintf(line, sizeof(line), "Pump: None");
    else snprintf(line, sizeof(line), "Pump: #%d", r.pumpIndex + 1);
    _box1Sprite->drawString(line, 40, 32);

    // Line 2: Moisture Threshold (Y shifted to 59, icon to 69)
    _drawDrop(*_box1Sprite, 12, 69, iconColor2);
    _box1Sprite->setTextColor(textColor2, TFT_BLACK);
    snprintf(line, sizeof(line), "Thresh: %d%%", r.moistureThreshold);
    _box1Sprite->drawString(line, 40, 59);

    // Line 3: Pumping Duration (Y shifted to 86, icon to 96)
    _drawClock(*_box1Sprite, 12, 96, iconColor3);
    _box1Sprite->setTextColor(textColor3, TFT_BLACK);
    snprintf(line, sizeof(line), "Time: %ds", r.durationSeconds);
    _box1Sprite->drawString(line, 40, 86);

    // Line 4: Delete Option (Y shifted to 113, icon to 123)
    _drawTrash(*_box1Sprite, 12, 123, iconColor4);
    _box1Sprite->setTextColor(textColor4, TFT_BLACK);
    if (_focusedLine == 4 && _isEditingSetting) {
      snprintf(line, sizeof(line), "Delete: %s", _deleteConfirm ? "YES" : "NO");
    } else {
      snprintf(line, sizeof(line), "Delete");
    }
    _box1Sprite->drawString(line, 40, 113);
  } else {
    // Line 1: Moisture
    uint16_t moistureColor = TFT_CYAN;
    if (r.humidityState == HUMIDITY_DRY) {
      moistureColor = TFT_RED;
    } else if (r.humidityState == HUMIDITY_THIRSTY) {
      moistureColor = TFT_ORANGE;
    } else if (r.humidityState == HUMIDITY_UNKNOWN) {
      moistureColor = TFT_DARKGREY;
    }
    _drawDrop(*_box1Sprite, 12, 50, moistureColor);
    _box1Sprite->setTextColor(moistureColor, TFT_BLACK);
    if (r.moisture == -1) snprintf(line, sizeof(line), "--%%");
    else snprintf(line, sizeof(line), "%d%%", r.moisture);
    _box1Sprite->drawString(line, 40, 38);
 
    // Line 2: Battery
    uint16_t batteryColor = (r.battery < 3.0f) ? TFT_RED : TFT_YELLOW;
    _drawBattery(*_box1Sprite, 12, 85, batteryColor);
    _box1Sprite->setTextColor(batteryColor, TFT_BLACK);
    snprintf(line, sizeof(line), "%.1fV", r.battery);
    _box1Sprite->drawString(line, 40, 73);
 
    // Line 3: Last Seen
    _drawClock(*_box1Sprite, 12, 120, TFT_GREEN);
    _box1Sprite->setTextColor(TFT_GREEN, TFT_BLACK);
    long secondsAgo = (millis() - r.lastSeen) / 1000;
    if (secondsAgo < 60) snprintf(line, sizeof(line), "%lds", secondsAgo);
    else snprintf(line, sizeof(line), "%ldm", secondsAgo / 60);
    _box1Sprite->drawString(line, 40, 108);
  }

  _box1Sprite->setFreeFont(NULL);
  
  _box1Sprite->pushSprite(BOX1_X, BOX1_Y);
}

void Display::showWateringSetup(const SensorReading& r, int step) {
  wake();
  
  _box1Sprite->fillSprite(TFT_BLACK);
  _box1Sprite->drawRoundRect(0, 0, BOX1_W, BOX1_H, 4, _highlightedBox == 1 ? TFT_RED : TFT_DARKGREY);
  
  _box1Sprite->setTextColor(TFT_WHITE, TFT_BLACK);
  _box1Sprite->setTextSize(1);
  char header[48];
  snprintf(header, sizeof(header), "Setup: %s", r.name);
  _box1Sprite->drawString(header, 4, 6);

  char buf[16];
  _box1Sprite->setFreeFont(&FreeSansBold9pt7b);

  // Step 0: Pump Index
  uint16_t c0 = (step == 0 ? TFT_WHITE : TFT_DARKGREY);
  _drawPump(*_box1Sprite, 8, 48, c0);
  _box1Sprite->setTextColor(c0, TFT_BLACK);
  if (r.pumpIndex == -1) snprintf(buf, sizeof(buf), "None");
  else snprintf(buf, sizeof(buf), "#%d", r.pumpIndex + 1);
  _box1Sprite->drawString(buf, 30, 36);

  // Step 1: Threshold
  uint16_t c1 = (step == 1 ? TFT_WHITE : TFT_DARKGREY);
  _drawDrop(*_box1Sprite, 8, 68, c1);
  _box1Sprite->setTextColor(c1, TFT_BLACK);
  snprintf(buf, sizeof(buf), "%d%%", r.moistureThreshold);
  _box1Sprite->drawString(buf, 30, 56);

  // Step 2: Duration
  uint16_t c2 = (step == 2 ? TFT_WHITE : TFT_DARKGREY);
  _drawClock(*_box1Sprite, 8, 88, c2);
  _box1Sprite->setTextColor(c2, TFT_BLACK);
  snprintf(buf, sizeof(buf), "%ds", r.durationSeconds);
  _box1Sprite->drawString(buf, 30, 76);

  _box1Sprite->setFreeFont(NULL);
  
  _box1Sprite->pushSprite(BOX1_X, BOX1_Y);
}

void Display::update(const Store& store) {
  unsigned long now = millis();
  unsigned long inactiveTime = now - _lastActivity;

  // Backlight Power Management Logic
  if (inactiveTime > OFF_TIMEOUT) {
    if (_currentBrightness != BRIGHT_OFF) {
      _currentBrightness = BRIGHT_OFF;
      analogWrite(BACKLIGHT_PIN, BRIGHT_OFF);
      LOG_INFO("Display: Powered OFF (Timeout)");
    }
  } else if (inactiveTime > DIM_TIMEOUT) {
    if (_currentBrightness != BRIGHT_DIM) {
      _currentBrightness = BRIGHT_DIM;
      analogWrite(BACKLIGHT_PIN, _currentBrightness);
      LOG_INFO("Display: Dimming screen");
    }
  }

  // Only render if display is ON or DIMMED (not OFF)
  if (_currentBrightness != BRIGHT_OFF) {
    if (_currentBrightness == BRIGHT_FULL) {
        analogWrite(BACKLIGHT_PIN, BRIGHT_FULL); // see wake() — must use analogWrite after first dim
    }
    _drawSidebar();
    _updateLog();
    _updateSensor();
    if (_light) _light->update(store);
  }
}

void Display::_drawDrop(TFT_eSPI& canvas, int x, int y, uint16_t color) {
  int cx = x + 10;
  int cy = y - 6;
  canvas.fillTriangle(cx, cy - 8, cx - 7, cy + 3, cx + 7, cy + 3, color);
  canvas.fillCircle(cx, cy + 3, 7, color);
}
void Display::_drawBattery(TFT_eSPI& canvas, int x, int y, uint16_t color) {
  int cx = x + 10;
  int cy = y - 6;
  canvas.drawRect(cx - 8, cy - 4, 16, 8, color);
  canvas.fillRect(cx + 8, cy - 2, 3, 4, color);
  canvas.fillRect(cx - 6, cy - 2, 12, 4, color);
}
void Display::_drawClock(TFT_eSPI& canvas, int x, int y, uint16_t color) {
  int cx = x + 10;
  int cy = y - 6;
  canvas.drawCircle(cx, cy, 9, color);
  canvas.drawLine(cx, cy, cx, cy - 5, color); // Hour
  canvas.drawLine(cx, cy, cx + 4, cy, color); // Minute
}
void Display::_drawPump(TFT_eSPI& canvas, int x, int y, uint16_t color) {
  int cx = x + 10;
  int cy = y - 6;
  canvas.drawRect(cx - 7, cy - 3, 14, 10, color);
  canvas.fillRect(cx - 3, cy + 2, 6, 4, color);
  canvas.drawLine(cx + 7, cy + 3, cx + 10, cy + 3, color);
  canvas.drawLine(cx + 10, cy + 3, cx + 10, cy + 7, color);
}

void Display::_drawTrash(TFT_eSPI& canvas, int x, int y, uint16_t color) {
  int cx = x + 10;
  int cy = y - 6;
  // Lid cap
  canvas.drawRect(cx - 2, cy - 7, 5, 2, color);
  // Lid line
  canvas.drawLine(cx - 6, cy - 5, cx + 6, cy - 5, color);
  // Body
  canvas.drawRect(cx - 5, cy - 4, 11, 11, color);
  // Vertical slots in body
  canvas.drawLine(cx - 2, cy - 2, cx - 2, cy + 4, color);
  canvas.drawLine(cx + 2, cy - 2, cx + 2, cy + 4, color);
}

void Display::setButtonLabels(const char* upShort, const char* upLong, const char* downShort, const char* downLong) {
  if (strcmp(_upShort, upShort) != 0 ||
      strcmp(_upLong, upLong) != 0 ||
      strcmp(_downShort, downShort) != 0 ||
      strcmp(_downLong, downLong) != 0) {
    
    strncpy(_upShort, upShort, sizeof(_upShort) - 1);
    _upShort[sizeof(_upShort) - 1] = '\0';
    
    strncpy(_upLong, upLong, sizeof(_upLong) - 1);
    _upLong[sizeof(_upLong) - 1] = '\0';
    
    strncpy(_downShort, downShort, sizeof(_downShort) - 1);
    _downShort[sizeof(_downShort) - 1] = '\0';
    
    strncpy(_downLong, downLong, sizeof(_downLong) - 1);
    _downLong[sizeof(_downLong) - 1] = '\0';
    
    _redrawSidebarFlag = true;
    LOG_INFO("Display: setButtonLabels UP(S='%s', L='%s') DOWN(S='%s', L='%s')", _upShort, _upLong, _downShort, _downLong);
   }
}

void Display::_drawSidebar() {
  if (!_redrawSidebarFlag) return;
  _redrawSidebarFlag = false;
  
  // Clear the 20px sidebar area (X: 0 to 19, Y: 0 to 170)
  _tft.fillRect(0, 0, 20, SCREEN_H, TFT_BLACK);
  
  // Draw horizontal divider line at Y = 85 (middle of screen height 170)
  _tft.drawLine(0, 85, 19, 85, TFT_DARKGREY);
  
  // Draw UP button labels (Top Half, Y: 0 to 84)
  // Short press: Column 1 centered at X = 5, Y = 42
  _drawRotatedText("S:", _upShort, 5, 42, TFT_LIGHTGREY);
  // Long press: Column 2 centered at X = 14, Y = 42
  _drawRotatedText("L:", _upLong, 14, 42, TFT_YELLOW);
  
  // Draw DOWN button labels (Bottom Half, Y: 85 to 169)
  // Short press: Column 1 centered at X = 5, Y = 127
  _drawRotatedText("S:", _downShort, 5, 127, TFT_LIGHTGREY);
  // Long press: Column 2 centered at X = 14, Y = 127
  _drawRotatedText("L:", _downLong, 14, 127, TFT_YELLOW);
}

void Display::_drawRotatedText(const char* prefix, const char* text, int xCenter, int yCenter, uint16_t color) {
  if (!text || text[0] == '\0') {
    return;
  }
  
  _sidebarSprite->fillSprite(TFT_BLACK);
  
  char buf[32];
  if (prefix && prefix[0] != '\0') {
    snprintf(buf, sizeof(buf), "%s%s", prefix, text);
  } else {
    snprintf(buf, sizeof(buf), "%s", text);
  }
  
  _sidebarSprite->setTextColor(color, TFT_BLACK);
  _sidebarSprite->setTextSize(1);
  _sidebarSprite->setTextFont(1);
  _sidebarSprite->setFreeFont(NULL); // Clear any active FreeFont
  
  int textWidth = _sidebarSprite->textWidth(buf);
  // Center horizontally inside our 85px sprite
  _sidebarSprite->drawString(buf, (85 - textWidth) / 2, 1);
  
  LOG_INFO("Display: _drawRotatedText buf='%s', textWidth=%d, x=%d, y=%d", buf, textWidth, xCenter, yCenter);
  
  // Set the TFT pivot to the target coordinate and push rotated by 270 degrees
  _tft.setPivot(xCenter, yCenter);
  _sidebarSprite->pushRotated(270);
}
