#include "DepthManager.h"
#include <MS5837.h>
#include <Wire.h>

MS5837 sensor;

DepthManager& DepthManager::getInstance() {
    static DepthManager instance;
    return instance;
}

DepthManager::DepthManager() {}

void DepthManager::begin() {
    Wire.begin(21, 22); // Default I2C pins for ESP32
    if (!sensor.init()) {
        Serial.println("[Sub] MS5837 Init Failed!");
    } else {
        sensor.setModel(MS5837::MS5837_30BA);
        sensor.setFluidDensity(1029); // kg/m^3 (Saltwater)
        Serial.println("[Sub] MS5837 Ready");
    }
}

void DepthManager::update() {
    uint32_t now = millis();
    if (now - _lastUpdate < 50) return; 
    float dt = (now - _lastUpdate) / 1000.0f;
    _lastUpdate = now;

    sensor.read();
    _actualDepth = sensor.depth();

    if (!_isDiving) {
        _verticalOutput = 1.0f; // Positive = Surface
        return;
    }

    // PID Calculation
    float error = _targetDepth - _actualDepth;
    _integral += error * dt;
    float derivative = (error - _lastError) / dt;
    _lastError = error;

    _verticalOutput = (_kp * error) + (_ki * _integral) + (_kd * derivative);

    // Clamp output to valid range
    if (_verticalOutput > 1.0f) _verticalOutput = 1.0f;
    if (_verticalOutput < -1.0f) _verticalOutput = -1.0f;
}

bool DepthManager::checkFailsafe() {
    // Logic to detect water ingress or signal loss
    // If true, vehicle will override all commands to max "UP"
    return false; 
}
