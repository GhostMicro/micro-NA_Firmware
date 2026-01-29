#include "Plane.h"

Plane::Plane() : motor(nullptr), ailerons(nullptr), elevator(nullptr) {
    memset(&currentInputs, 0, sizeof(NAPacket));
}

void Plane::setup() {
    // Motor for throttle (GPIO 27, Channel 6)
    motor = new Motor(27, 14, 12, 6);
    motor->setup();
    
    // Servo for ailerons (left/right wing)
    ailerons = new ServoDriver(22);
    ailerons->setup();
    
    // Servo for elevator (pitch control)
    elevator = new ServoDriver(23);
    elevator->setup();
    
    Serial.println("Plane initialized - Motor + 2x Servos ready");
}

void Plane::loop() {
    updateControls(currentInputs.throttle, currentInputs.roll, currentInputs.pitch);
}

void Plane::setInputs(NAPacket* packet) {
    if (packet) {
        currentInputs.throttle = packet->throttle;
        currentInputs.roll = packet->roll;
        currentInputs.pitch = packet->pitch;
    }
}

void Plane::updateControls(int16_t throttle, int16_t roll, int16_t pitch) {
    // Scale inputs
    throttle /= 10;  // -100 to 100
    roll /= 10;      // -100 to 100
    pitch /= 10;     // -100 to 100
    
    // Throttle directly to motor
    motor->setSpeed(throttle);
    
    // Aileron mixing: differential control of left/right wings
    // Center is 90°, deflection ±45° = ±2000 microseconds
    int16_t aileronAngle = 90 + (roll / 2);  // -50 to +50 range
    aileronAngle = constrain(aileronAngle, 45, 135);
    ailerons->write(aileronAngle);
    
    // Elevator for pitch control
    int16_t elevatorAngle = 90 + (pitch / 2);  // -50 to +50 range
    elevatorAngle = constrain(elevatorAngle, 45, 135);
    elevator->write(elevatorAngle);
}

String Plane::getName() const {
    return "PLANE";
}
