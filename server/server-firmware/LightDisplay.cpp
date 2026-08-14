#include "LightDisplay.h"
#include "Sprites.h"
#include "Config.h"
#include <Arduino.h>
#include <vector>
#include <algorithm>

static uint32_t hashString(const std::string& str) {
  uint32_t hash = 5381;
  for (unsigned int i = 0; i < str.length(); i++) {
    hash = ((hash << 5) + hash) + str[i];
  }
  return hash;
}

static void getSlotCoords(int idx, int& gx, int& gy) {
  // Pre-designed, beautifully spaced slot coordinates across the remaining grid.
  // The first 6 coordinates are completely disjoint (zero overlap) and cover all quadrants.
  //
  // Shed is visually at gx=6, gy=4 (blocking 6,4, 7,4, 6,5, 7,5) and physically blocks 6,5, 7,5 on the ground.
  // To leave beautiful spacing, all slots stay strictly outside the 12 lower-right tiles where gx >= 5 and gy >= 3.
  static const int idealSlots[26][2] = {
    {0, 0}, {5, 0}, {3, 2}, {0, 4}, {2, 5}, {2, 0}, // 6 completely disjoint, horizontally balanced slots
    {1, 1}, {4, 1}, {1, 4}, {3, 0}, {0, 3}, {1, 5},
    {6, 1}, {2, 4}, {0, 5}, {5, 1}, {3, 1}, {1, 3},
    {2, 3}, {0, 2}, {1, 2}, {1, 0}, {6, 0}, {2, 1},
    {0, 1}, {4, 0}
  };
  
  int safeIdx = idx % 26;
  gx = idealSlots[safeIdx][0];
  gy = idealSlots[safeIdx][1];
}

static const uint16_t* getTreeSprite(const SensorReading& r) {
  int treeType = r.pumpIndex;
  if (treeType < 0 || treeType > 3) {
    treeType = abs(treeType) % 4;
  }
  
  int treeState = 0; // 0=healthy, 1=thirsty, 2=dry
  if (r.humidityState == HUMIDITY_DRY) {
    treeState = 2;
  } else if (r.humidityState == HUMIDITY_THIRSTY) {
    treeState = 1;
  } else if (r.humidityState == HUMIDITY_HEALTHY) {
    treeState = 0;
  } else {
    // HUMIDITY_UNKNOWN:
    // If currently being watered or recently watered and waiting for feedback,
    // show as healthy to provide instant visual feedback of watering success.
    if (r.wateringState == WATERING_ACTIVE || 
        r.wateringState == WATERING_COOLDOWN || 
        r.wateringState == WATERING_WAIT_MEASUREMENT) {
      treeState = 0; // healthy
    } else {
      // Uncalibrated / truly unknown: default to thirsty
      treeState = 1;
    }
  }
  
  if (treeType == 0) {
    return (treeState == 0) ? tree_0_h : (treeState == 1 ? tree_0_u : tree_0_d);
  } else if (treeType == 1) {
    return (treeState == 0) ? tree_1_h : (treeState == 1 ? tree_1_u : tree_1_d);
  } else if (treeType == 2) {
    return (treeState == 0) ? tree_2_h : (treeState == 1 ? tree_2_u : tree_2_d);
  } else {
    return (treeState == 0) ? tree_3_h : (treeState == 1 ? tree_3_u : tree_3_d);
  }
}

LightDisplay::LightDisplay(TFT_eSPI& tft, int x, int y, int w, int h) 
  : _tft(tft), _canvas(nullptr), _x(x), _y(y), _w(w), _h(h) {
  for(int i=0; i<4; i++) {
    _pumpStates[i] = false;
    _cooldownStates[i] = false;
  }
  _selectedPlant = -1;
  _lastStateHash = 0;
  
  // Initialize gardener walking and animation state
  _gardenerX = 6.5f;
  _gardenerY = 6.5f;
  _nextWaypointIdx = 0; // Start at waypoint 0 (6.5f, 6.5f)
  _lastUpdateMs = millis();
  _dir = DIR_DOWN;
  _isMoving = true;
  _animStep = 0;
  _lastAnimFrameMs = millis();
  _isPaused = false;
  _pauseEndMs = 0;
  
  // Initialize targeted path routing state
  _targetWaypointCount = 0;
  _targetWaypointIdx = 0;
  _trackedPlantIdx = -2; // sentinel to trigger initial path determination on first update
}

void LightDisplay::init() {
  _canvas = new TFT_eSprite(&_tft);
  _canvas->createSprite(_w, _h);
  _canvas->setColorDepth(16);
  _canvas->setFreeFont(NULL); // Zero-initialize GFXfont heap garbage pointer
  _canvas->setSwapBytes(true); // Swap bytes for pushImage to render colors correctly!
  
  _drawBackground();
  _drawContent(Store());
  _lastStateHash = 0;
  
  _canvas->pushSprite(_x, _y);
}

void LightDisplay::setPumpActive(int pumpIdx, bool active, uint32_t durationMs) {
  if (pumpIdx >= 0 && pumpIdx < 4) {
    _pumpStates[pumpIdx] = active;
  }
}

void LightDisplay::setCooldownActive(int pumpIdx, bool active) {
  if (pumpIdx >= 0 && pumpIdx < 4) {
    _cooldownStates[pumpIdx] = active;
  }
}

void LightDisplay::setSelectedPlant(int plantIdx) {
  _selectedPlant = plantIdx;
}

uint32_t LightDisplay::_getCurrentStateHash(const Store& store) {
  uint32_t hash = 17;
  hash = hash * 31 + _selectedPlant;
  for (int i = 0; i < 4; i++) {
    hash = hash * 31 + (_pumpStates[i] ? 1 : 0);
    hash = hash * 31 + (_cooldownStates[i] ? 1 : 0);
  }
  store.lock();
  const auto& all = store.getAll();
  hash = hash * 31 + all.size();
  for (auto const& kv : all) {
    hash = hash * 31 + hashString(kv.first);
    hash = hash * 31 + kv.second.moisture;
    hash = hash * 31 + kv.second.pumpIndex;
    hash = hash * 31 + kv.second.humidityState;
    hash = hash * 31 + kv.second.wateringState;
  }
  store.unlock();
  return hash;
}

void LightDisplay::update(const Store& store, bool forceRedraw) {
  unsigned long now = millis();
  
  // Update gardener position and animation frame
  bool gardenerMoved = _updateGardener(store);
  
  uint32_t currentHash = _getCurrentStateHash(store);
  
  // Rate-limit the screen redraws to approx 30 FPS (every 33ms) to avoid CPU starvation
  static unsigned long lastRedrawMs = 0;
  bool timeToRedraw = (now - lastRedrawMs >= 33);
  
  if (timeToRedraw && (forceRedraw || gardenerMoved || currentHash != _lastStateHash)) {
    lastRedrawMs = now;
    _lastStateHash = currentHash;
    _drawContent(store);
    _canvas->pushSprite(_x, _y);
  }
}

void LightDisplay::_drawBackground() {
  uint16_t bgColor = pgm_read_word(&scene_sprite[0]);
  _canvas->fillRect(0, 0, _w, _h, TFT_BLACK);
  _canvas->fillRoundRect(0, 0, _w, _h, 4, bgColor);
  _canvas->drawRoundRect(0, 0, _w, _h, 4, TFT_DARKGREY);
}

void LightDisplay::_drawContent(const Store& store) {
  uint16_t bgColor = pgm_read_word(&scene_sprite[0]);
  // clear with a black rectangle (is it necessary?)
  _canvas->fillRect(0, 0, _w, _h, TFT_BLACK);
  // fill with bg color, smaller to have a black line left around
  _canvas->fillRoundRect(2, 2, _w-4, _h-4, 4, bgColor);  
  // silver border like other boxes
  _canvas->drawRoundRect(0, 0, _w, _h, 4, TFT_DARKGREY);
  // Draw centered 140x132 scene sprite from PROGMEM on the sprite
  _canvas->pushImage(4, 5, 140, 132, scene_sprite);

  // Collect all elements to draw
  struct RenderElement {
    enum Type { TREE, SHED, GARDENER, WET_FLOOR, RAIN };
    Type type;
    float gx, gy;
    const uint16_t* sprite;
    bool mirror; // Only used by GARDENER
  };

  std::vector<RenderElement> elements;
  
  // Add shed at cell (6, 4) (which visually blocks Y=4 and Y=5, base collision gy = 5)
  elements.push_back({RenderElement::SHED, 6.0f, 5.0f, nullptr, false});

  // Determine gardener sprite and horizontal mirroring dynamically based on direction/motion
  const uint16_t* gardenerSprite = gardener_f;
  bool mirror = false;

  if (_dir == DIR_DOWN) {
    if (!_isMoving || _isPaused) {
      gardenerSprite = gardener_f;
    } else {
      gardenerSprite = (_animStep == 0) ? gardener_f_l : gardener_f_r;
    }
  } else if (_dir == DIR_UP) {
    if (!_isMoving || _isPaused) {
      gardenerSprite = gardener_b;
    } else {
      gardenerSprite = (_animStep == 0) ? gardener_b_l : gardener_b_r;
    }
  } else if (_dir == DIR_RIGHT) {
    if (!_isMoving || _isPaused) {
      gardenerSprite = gardener_r;
    } else {
      gardenerSprite = (_animStep == 0) ? gardener_r_l : gardener_r_r;
    }
  } else if (_dir == DIR_LEFT) {
    mirror = true;
    if (!_isMoving || _isPaused) {
      gardenerSprite = gardener_r;
    } else {
      gardenerSprite = (_animStep == 0) ? gardener_r_l : gardener_r_r;
    }
  }

  // Add dynamic gardener hero sprite at smooth floating coordinates
  elements.push_back({RenderElement::GARDENER, _gardenerX, _gardenerY, gardenerSprite, mirror});

  // Query sensors and place one tree for each sensor
  store.lock();
  const auto& all = store.getAll();
  
  int sensorIdx = 0;
  for (auto const& kv : all) {
    const SensorReading& r = kv.second;
    int gx = 0, gy = 0;
    getSlotCoords(sensorIdx, gx, gy);
    sensorIdx++;
    
    const uint16_t* treeSprite = getTreeSprite(r);
    // Tree ground/collision base is at gy + 1
    elements.push_back({RenderElement::TREE, (float)gx, (float)gy + 1.0f, treeSprite, false});
    
    // If in cooldown period, place wet floor sprite at bottom right of the tree (gx + 1, gy + 1)
    if (r.wateringState == WATERING_COOLDOWN) {
      elements.push_back({RenderElement::WET_FLOOR, (float)gx + 1.2f, (float)gy + 1.4f, wet_floor_sprite, false});
    }

    // If actively watering, place rain sprite overlay directly on top of the tree (gx, gy)
    if (r.wateringState == WATERING_ACTIVE) {
      int rainFrame = (millis() / 150) % 4;
      const uint16_t* rainSprite = nullptr;
      if (rainFrame == 0) rainSprite = rain_sprite_0;
      else if (rainFrame == 1) rainSprite = rain_sprite_1;
      else if (rainFrame == 2) rainSprite = rain_sprite_2;
      else rainSprite = rain_sprite_3;
      
      elements.push_back({RenderElement::RAIN, (float)gx, (float)gy + 1.001f, rainSprite, false});
    }
  }
  store.unlock();

  // Y-sorting: Sort elements by gy coordinate so higher Y are displayed in foreground (drawn later)
  std::sort(elements.begin(), elements.end(), [](const RenderElement& a, const RenderElement& b) {
    if (a.gy != b.gy) {
      return a.gy < b.gy;
    }
    return a.gx < b.gx;
  });

  // Render elements in Y-sorted order
  for (auto const& el : elements) {
    if (el.type == RenderElement::TREE) {
      _drawTree(el.sprite, (int)el.gx, (int)(el.gy - 1.0f));
    } else if (el.type == RenderElement::SHED) {
      _drawShed((int)el.gx, (int)(el.gy - 1.0f));
    } else if (el.type == RenderElement::GARDENER) {
      _drawGardener(el.sprite, el.gx, el.gy, el.mirror);
    } else if (el.type == RenderElement::WET_FLOOR) {
      _drawWetFloorSign(el.gx, el.gy);
    } else if (el.type == RenderElement::RAIN) {
      _drawRain(el.sprite, (int)el.gx, (int)(el.gy - 1.001f));
    }
  }

  // Draw 16x16 debug coordinate grid overlay if enabled
  if (ENABLE_DEBUG_GRID) {
    _canvas->setTextColor(TFT_RED);
    for (int gy = 0; gy < 7; gy++) {
      for (int gx = 0; gx < 8; gx++) {
        int px = 9 + gx * 16;
        int py = 13 + gy * 16;
        _canvas->drawRect(px, py, 16, 16, TFT_BLACK);
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", gx);
        _canvas->drawString(buf, px + 2, py + 1);
        snprintf(buf, sizeof(buf), "%d", gy);
        _canvas->drawString(buf, px + 9, py + 8);
      }
    }
  }
}

void LightDisplay::_drawTree(const uint16_t* tree, int gx, int gy) {
  int px_start = 9 + gx * 16;
  int py_start = 13 + gy * 16;
  for (int py = 0; py < 32; py++) {
    for (int px = 0; px < 32; px++) {
      uint16_t c = pgm_read_word(tree + (py * 32) + px);
      if (c != 0xF81F) _canvas->drawPixel(px_start + px, py_start + py, c);
    }
  }
}

void LightDisplay::_drawShed(int gx, int gy) {
  int px_start = 9 + gx * 16;
  int py_start = 13 + gy * 16;
  for (int py = 0; py < 32; py++) {
    for (int px = 0; px < 32; px++) {
      uint16_t c = pgm_read_word(shed_sprite + (py * 32) + px);
      if (c != 0xF81F) _canvas->drawPixel(px_start + px, py_start + py, c);
    }
  }
}

void LightDisplay::_drawGardener(const uint16_t* gardener, float gx, float gy, bool mirror) {
  int px_start = 9 + (int)(gx * 16.0f);
  int py_start = 13 + (int)(gy * 16.0f);
  for (int py = 0; py < 16; py++) {
    for (int px = 0; px < 16; px++) {
      uint16_t c = pgm_read_word(gardener + (py * 16) + px);
      if (c != 0xF81F) {
        int target_x = px_start + (mirror ? (15 - px) : px);
        _canvas->drawPixel(target_x, py_start + py, c);
      }
    }
  }
}

void LightDisplay::_drawWetFloorSign(float gx, float gy) {
  int px_start = 9 + (int)(gx * 16.0f);
  int py_start = 13 + (int)(gy * 16.0f);
  for (int py = 0; py < 16; py++) {
    for (int px = 0; px < 16; px++) {
      uint16_t c = pgm_read_word(wet_floor_sprite + (py * 16) + px);
      if (c != 0xF81F) {
        _canvas->drawPixel(px_start + px, py_start + py, c);
      }
    }
  }
}

void LightDisplay::_drawRain(const uint16_t* rain, int gx, int gy) {
  int px_start = 9 + gx * 16;
  int py_start = 13 + gy * 16;
  for (int py = 0; py < 32; py++) {
    for (int px = 0; px < 32; px++) {
      uint16_t c = pgm_read_word(rain + (py * 32) + px);
      if (c != 0xF81F) {
        _canvas->drawPixel(px_start + px, py_start + py, c);
      }
    }
  }
}

bool LightDisplay::_updateGardener(const Store& store) {
  unsigned long now = millis();
  unsigned long dt = now - _lastUpdateMs;
  _lastUpdateMs = now;

  if (dt > 100) {
    // Cap dt if there is major loop lag (e.g. during initialization or network activity)
    dt = 100;
  }

  // 1. Check if selected plant changed
  if (_selectedPlant != _trackedPlantIdx) {
    _trackedPlantIdx = _selectedPlant;
    if (_trackedPlantIdx >= 0) {
      int gx = 0, gy = 0;
      getSlotCoords(_trackedPlantIdx, gx, gy);
      
      float tx, ty;
      if (gy == 5) {
        // Stand to the left or right of the tree's bottom collision tiles, visually touching the side of the tree
        int left_gx = gx - 1;
        bool left_ok = (left_gx >= 0 && !_isCollisionTile(left_gx, 6, store));
        int right_gx = gx + 2;
        bool right_ok = (right_gx < 8 && !_isCollisionTile(right_gx, 6, store));
        
        if (left_ok) {
          tx = (float)gx - 1.0f; // visually touching the left of the tree
          ty = 6.0f;             // row 6
        } else if (right_ok) {
          tx = (float)gx + 2.0f; // visually touching the right of the tree
          ty = 6.0f;             // row 6
        } else {
          tx = (float)gx - 1.0f;
          ty = 6.0f;
        }
      } else {
        tx = (float)gx + 0.5f; // horizontally centered under the tree
        ty = (float)gy + 2.0f; // vertically touching the bottom of the tree (row gy + 2)
      }
      
      _setNewTarget(tx, ty, store);
    } else {
      // Not tracking any plant: walk back to front of shed, then continue walking around shed
      _setNewTarget(6.5f, 6.5f, store);
    }
  }

  // 2. Handle pause state
  if (_isPaused) {
    if (now >= _pauseEndMs) {
      _isPaused = false;
      if (_trackedPlantIdx >= 0) {
        // Reached final tree destination, stand still
        _isMoving = false;
      } else {
        // Idle mode: advance to next waypoint and trigger it
        static const float WAYPOINTS[9][2] = {
          {6.5f, 6.5f}, {7.5f, 6.5f}, {6.5f, 6.5f}, {5.5f, 6.5f},
          {5.5f, 3.5f}, {7.5f, 3.5f}, {5.5f, 3.5f}, {5.5f, 6.5f},
          {6.5f, 6.5f}
        };
        _nextWaypointIdx = (_nextWaypointIdx + 1) % 9;
        float tx = WAYPOINTS[_nextWaypointIdx][0];
        float ty = WAYPOINTS[_nextWaypointIdx][1];
        _setNewTarget(tx, ty, store);
      }
      return true;
    }
    return false; // Still standing paused
  }

  if (!_isMoving) {
    return false;
  }

  // 3. Resolve target coordinates
  float targetX = 6.5f;
  float targetY = 6.5f;
  float speed = 0.0012f;

  if (_targetWaypointIdx < _targetWaypointCount) {
    targetX = _targetWaypoints[_targetWaypointIdx][0];
    targetY = _targetWaypoints[_targetWaypointIdx][1];
    speed = 0.0015f; // slightly faster targeted walking
  }

  float dx = targetX - _gardenerX;
  float dy = targetY - _gardenerY;
  float dist = speed * dt;
  bool reachedWaypoint = false;

  // Orthogonal movement: walk horizontally first, then vertically. Never diagonally!
  if (abs(dx) > 0.01f) {
    // Walk horizontally
    _dir = (dx > 0) ? DIR_RIGHT : DIR_LEFT;
    
    if (abs(dx) <= dist) {
      _gardenerX = targetX;
      // If Y is also exhausted, we reached the waypoint
      if (abs(dy) <= 0.01f) {
        reachedWaypoint = true;
      }
    } else {
      _gardenerX += (dx > 0 ? 1.0f : -1.0f) * dist;
    }
  } else if (abs(dy) > 0.01f) {
    // Walk vertically
    _dir = (dy > 0) ? DIR_DOWN : DIR_UP;
    
    if (abs(dy) <= dist) {
      _gardenerY = targetY;
      reachedWaypoint = true;
    } else {
      _gardenerY += (dy > 0 ? 1.0f : -1.0f) * dist;
    }
  } else {
    // Already at target
    reachedWaypoint = true;
  }

  if (reachedWaypoint) {
    _targetWaypointIdx++;
    if (_targetWaypointIdx >= _targetWaypointCount) {
      // Reached final targeted destination!
      _isMoving = false;
      _dir = DIR_DOWN; // Face front
      
      // Pause naturally at destination
      _isPaused = true;
      if (_trackedPlantIdx >= 0) {
        _pauseEndMs = now + 99999999; // effectively forever
      } else {
        _pauseEndMs = now + 1000; // Pause for 1 second at idle waypoint
        
        // Face a natural direction during pause
        if (_nextWaypointIdx == 1 || _nextWaypointIdx == 5) {
          _dir = DIR_UP; // Face the shed
        } else {
          _dir = DIR_DOWN; // Face front
        }
      }
    }
  }

  // Update walking animation frame
  if (_isMoving) {
    if (now - _lastAnimFrameMs >= 200) {
      _animStep = (_animStep + 1) % 2;
      _lastAnimFrameMs = now;
      reachedWaypoint = true;
    }
  }

  return reachedWaypoint || _isMoving;
}

void LightDisplay::_setNewTarget(float tx, float ty, const Store& store) {
  int start_gx = (int)_gardenerX;
  int start_gy = (int)_gardenerY;
  int target_gx = (int)tx;
  int target_gy = (int)ty;

  // Clamp start and target to grid bounds
  if (start_gx < 0) start_gx = 0; if (start_gx > 7) start_gx = 7;
  if (start_gy < 0) start_gy = 0; if (start_gy > 6) start_gy = 6;
  if (target_gx < 0) target_gx = 0; if (target_gx > 7) target_gx = 7;
  if (target_gy < 0) target_gy = 0; if (target_gy > 6) target_gy = 6;

  _targetWaypointCount = 0;
  _targetWaypointIdx = 0;

  // BFS structures to find the shortest collision-free grid path
  int parentX[8][7];
  int parentY[8][7];
  bool visited[8][7];
  for (int x = 0; x < 8; x++) {
    for (int y = 0; y < 7; y++) {
      visited[x][y] = false;
      parentX[x][y] = -1;
      parentY[x][y] = -1;
    }
  }

  int queueX[64];
  int queueY[64];
  int head = 0, tail = 0;

  queueX[tail] = start_gx;
  queueY[tail] = start_gy;
  tail++;
  visited[start_gx][start_gy] = true;

  bool found = false;
  while (head < tail) {
    int cx = queueX[head];
    int cy = queueY[head];
    head++;

    if (cx == target_gx && cy == target_gy) {
      found = true;
      break;
    }

    static const int dx[] = {0, 0, -1, 1};
    static const int dy[] = {-1, 1, 0, 0};
    for (int i = 0; i < 4; i++) {
      int nx = cx + dx[i];
      int ny = cy + dy[i];

      if (nx >= 0 && nx < 8 && ny >= 0 && ny < 7) {
        if (!visited[nx][ny]) {
          // Allow the target cell even if it happens to be considered a collision tile by rounding
          if ((nx == target_gx && ny == target_gy) || !_isCollisionTile(nx, ny, store)) {
            visited[nx][ny] = true;
            parentX[nx][ny] = cx;
            parentY[nx][ny] = cy;
            queueX[tail] = nx;
            queueY[tail] = ny;
            tail++;
          }
        }
      }
    }
  }

  if (found) {
    // Reconstruct path backward
    int pathX[64];
    int pathY[64];
    int pathLen = 0;

    int cx = target_gx;
    int cy = target_gy;
    while (cx != -1 && cy != -1) {
      pathX[pathLen] = cx;
      pathY[pathLen] = cy;
      pathLen++;
      int px = parentX[cx][cy];
      int py = parentY[cx][cy];
      cx = px;
      cy = py;
    }

    // Convert reversed path to waypoints (skipping the start node itself)
    for (int i = pathLen - 2; i >= 0; i--) {
      // Use exact targeted coords for the final step to standing position, and cell centers for intermediate steps
      if (i == 0) {
        _targetWaypoints[_targetWaypointCount][0] = tx;
        _targetWaypoints[_targetWaypointCount][1] = ty;
      } else {
        _targetWaypoints[_targetWaypointCount][0] = (float)pathX[i] + 0.5f;
        _targetWaypoints[_targetWaypointCount][1] = (float)pathY[i] + 0.5f;
      }
      _targetWaypointCount++;
      if (_targetWaypointCount >= 16) break; // safety cap
    }
  } else {
    // Fallback: direct straight step
    _targetWaypoints[_targetWaypointCount][0] = tx;
    _targetWaypoints[_targetWaypointCount][1] = ty;
    _targetWaypointCount = 1;
  }

  _isMoving = true;
  _isPaused = false;
}

bool LightDisplay::_isCollisionTile(int gx, int gy, const Store& store) {
  // Shed bottom tiles are (6, 5) and (7, 5)
  if (gy == 5 && (gx == 6 || gx == 7)) {
    return true;
  }
  
  // Query active trees
  store.lock();
  const auto& all = store.getAll();
  int sensorIdx = 0;
  for (auto const& kv : all) {
    int tgx = 0, tgy = 0;
    getSlotCoords(sensorIdx, tgx, tgy);
    sensorIdx++;
    
    // Bottom 2 tiles count for collision
    if (gy == tgy + 1 && (gx == tgx || gx == tgx + 1)) {
      store.unlock();
      return true;
    }
  }
  store.unlock();
  return false;
}
