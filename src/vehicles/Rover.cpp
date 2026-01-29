#include "Rover.h"

Rover::Rover() : motorLeft(nullptr), motorRight(nullptr) {
    memset(&currentInputs, 0, sizeof(NAPacket));
}

void Rover::setup() {
    // Initialize left motor (GPIO 26 PWM, 27/14 DIR, Channel 0)
    motorLeft = new Motor(26, 27, 14, 0);
    motorLeft->setup();
    
    // Initialize right motor (GPIO 25 PWM, 13/12 DIR, Channel 1)
    motorRight = new Motor(25, 13, 12, 1);
    motorRight->setup();
    
    Serial.println("Rover initialized - 2x Motors ready");
}

void Rover::loop() {
    // Apply current inputs to motors (with failsafe/deadband/ramping)
    drive(currentInputs.throttle, currentInputs.roll);
}

void Rover::setInputs(NAPacket* packet) {
    if (packet) {
        currentInputs.throttle = packet->throttle;
        currentInputs.roll = packet->roll;      // Steering input
    }
}

void Rover::drive(int16_t throttle, int16_t steering) {
    // Differential drive kinematics (skid-steer)
    // throttle: -1000 to +1000 (forward/backward)
    // steering: -1000 to +1000 (left/right turn)
    
    int16_t leftSpeed = throttle + steering;    // Add steering to left
    int16_t rightSpeed = throttle - steering;   // Subtract steering from right
    
    // Constrain to motor range
    leftSpeed = constrain(leftSpeed, -1000, 1000);
    rightSpeed = constrain(rightSpeed, -1000, 1000);
    
    // Scale to motor range (-100 to 100)
    motorLeft->setSpeed(leftSpeed / 10);
    motorRight->setSpeed(rightSpeed / 10);
}

String Rover::getName() const {
    return "ROVER";
}
