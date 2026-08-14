#ifndef LIGHT_DISPLAY_H
#define LIGHT_DISPLAY_H

#include <TFT_eSPI.h>
#include "Store.h"

class LightDisplay {
public:
    LightDisplay(TFT_eSPI& tft, int x, int y, int w, int h);
    void init();
    void update(const Store& store, bool forceRedraw = false);
    void setPumpActive(int pumpIdx, bool active, uint32_t durationMs = 0);
    void setCooldownActive(int pumpIdx, bool active);
    void setSelectedPlant(int plantIdx);

private:
    TFT_eSPI& _tft;
    TFT_eSprite* _canvas;
    int _x, _y, _w, _h;

    bool _pumpStates[4];
    bool _cooldownStates[4];
    int _selectedPlant;
    
    // State for change detection
    uint32_t _lastStateHash;

    // Gardener walking and animation state
    float _gardenerX;
    float _gardenerY;
    int _nextWaypointIdx;
    unsigned long _lastUpdateMs;
    
    enum Direction { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT } _dir;
    bool _isMoving;
    int _animStep; // 0 or 1 for step_left, step_right
    unsigned long _lastAnimFrameMs;
    
    // Pause state at waypoints
    bool _isPaused;
    unsigned long _pauseEndMs;

    // Targeted path routing state
    float _targetWaypoints[16][2];
    int _targetWaypointCount;
    int _targetWaypointIdx;
    int _trackedPlantIdx;

    uint32_t _getCurrentStateHash(const Store& store);
    void _drawBackground();
    void _drawContent(const Store& store);
    void _drawShed(int gx, int gy);
    void _drawTree(const uint16_t* tree, int gx, int gy);
    void _drawGardener(const uint16_t* gardener, float gx, float gy, bool mirror);
    void _drawWetFloorSign(float gx, float gy);
    void _drawRain(const uint16_t* rain, int gx, int gy);
    
    bool _isCollisionTile(int gx, int gy, const Store& store);
    bool _updateGardener(const Store& store);
    void _setNewTarget(float tx, float ty, const Store& store);
};

#endif
