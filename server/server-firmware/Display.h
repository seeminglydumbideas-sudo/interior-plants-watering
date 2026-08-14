#ifndef DISPLAY_H
#define DISPLAY_H

#include <TFT_eSPI.h>
#include "Store.h"
#include "LightDisplay.h"

class Display {
public:
  void init();
  void update(const Store& store);
  void log(const char* message);
  void showSensor(const SensorReading& r, int idx, int total);
  void showNoSensors();
  void setPumpActive(int pumpIdx, bool active, uint32_t durationMs = 0);
  void setCooldownActive(int pumpIdx, bool active);
  void setSelectedPlant(int plantIdx);
  void highlightBox(int boxId); // 0=none, 1, 2, 3
  void showWateringSetup(const SensorReading& r, int step);
  void wake(); // Reset inactivity timer and restore brightness
  bool isDisplayOff(); // Check if screen is completely off
  void setSettingsMode(bool enabled);
  void setSettingsFocus(int lineIdx, bool isEditing);
  void setDeleteConfirm(bool confirm);
  void setButtonLabels(const char* upShort, const char* upLong, const char* downShort, const char* downLong);

private:
  TFT_eSPI _tft;
  LightDisplay* _light;
  int _highlightedBox; // 0=none, 1, 2, 3
  bool _settingsMode;
  int _focusedLine;    // 0=none, 1, 2, 3, 4
  bool _isEditingSetting;
  bool _deleteConfirm;

  // Backlight management
  unsigned long _lastActivity;
  uint8_t       _currentBrightness;
  static const int      BACKLIGHT_PIN = 38;

  static const int      SCREEN_W      = 320;
  static const int      SCREEN_H      = 170;

  static const int      BOX1_W        = 150;
  static const int      BOX1_H        = 140;
  static const int      BOX1_X        = 20;
  static const int      BOX1_Y        = 0;
  static const int      BOX2_W        = 149;
  static const int      BOX2_H        = 140;
  static const int      BOX2_X        = BOX1_X + BOX1_W + 1; // 171
  static const int      BOX2_Y        = 0;
  static const int      BOX3_W        = 300;
  static const int      BOX3_H        = SCREEN_H-BOX1_H-1;
  static const int      BOX3_X        = 20;
  static const int      BOX3_Y        = BOX1_H+1;

  static const int      LOG_COLS      = 48;
  static const uint16_t COLOR_DRKGREEN = 0x5465;

  // log state
  char logBuf[2][LOG_COLS];
  bool _redrawLog;

  // sensor display state
  struct SensorDisplay {
    SensorReading reading;
    int           idx;
    int           total;
    bool          valid;  // false = "no sensors" state
  } _sensorDisp;
  bool _redrawSensor;

  void _updateLog();
  void _updateSensor();
  void _clear_box1();
  void _clear_box2();
  void _clear_box3();
  void _drawDrop(TFT_eSPI& canvas, int x, int y, uint16_t color);
  void _drawBattery(TFT_eSPI& canvas, int x, int y, uint16_t color);
  void _drawClock(TFT_eSPI& canvas, int x, int y, uint16_t color);
  void _drawPump(TFT_eSPI& canvas, int x, int y, uint16_t color);
  void _drawTrash(TFT_eSPI& canvas, int x, int y, uint16_t color);
  void _drawSidebar();
  void _drawRotatedText(const char* prefix, const char* text, int xCenter, int yCenter, uint16_t color);

  TFT_eSprite* _sidebarSprite;
  TFT_eSprite* _box1Sprite;
  TFT_eSprite* _box3Sprite;
  char _upShort[12];
  char _upLong[12];
  char _downShort[12];
  char _downLong[12];
  bool _redrawSidebarFlag;
};

#endif
