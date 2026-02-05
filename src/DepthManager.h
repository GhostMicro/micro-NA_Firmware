#ifndef DEPTH_MANAGER_H
#define DEPTH_MANAGER_H

#include <Arduino.h>

class DepthManager {
public:
    static DepthManager& getInstance();
    
    void begin();
    void update();
    
    void setTargetDepth(float meters) { _targetDepth = meters; }
    float getActualDepth() const { return _actualDepth; }
    
    float getVerticalOutput() const { return _verticalOutput; }
    
    bool isDiving() const { return _isDiving; }
    void setDiving(bool diving) { _isDiving = diving; }

    // Failsafe: Returns true if we should resurface immediately
    bool checkFailsafe();

private:
    DepthManager();
    float _targetDepth = 0.0f;
    float _actualDepth = 0.0f;
    float _verticalOutput = 0.0f; // -1.0 to 1.0 (Down to Up)
    bool _isDiving = false;
    
    uint32_t _lastUpdate = 0;
    
    // PID Parameters (To be tuned in-water)
    float _kp = 1.0f;
    float _ki = 0.1f;
    float _kd = 0.5f;
    float _integral = 0.0f;
    float _lastError = 0.0f;
};

#endif
