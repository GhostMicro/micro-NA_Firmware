#include "ServoDriver.h"

ServoDriver::ServoDriver(int pin) : _pin(pin), _centerAngle(90) {}

ServoDriver::ServoDriver(int pin, int initialAngle) : _pin(pin), _centerAngle(initialAngle) {}

void ServoDriver::setup() {
    _servo.attach(_pin);
    _servo.write(_centerAngle);
    delay(50);  // Allow servo to settle
}

void ServoDriver::write(int angle) {
    // Constrain to valid servo range (0-180 degrees)
    int originalAngle = angle;
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    // Log if trying to exceed bounds
    if (angle != originalAngle) {
        Serial.printf("[SERVO] Constrained %d to %d\n", originalAngle, angle);
    }
    
    _servo.write(angle);
}
