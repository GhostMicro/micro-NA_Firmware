#include "Sub.h"
#include "DepthManager.h"

Sub::Sub() : forwardMotor(nullptr), yawMotor(nullptr), verticalMotor(nullptr), trimBallast(nullptr) {
    memset(&currentInputs, 0, sizeof(NAPacket));
}

void Sub::setup() {
    // Forward thruster
    forwardMotor = new Motor(27, 14, 12, 6);
    forwardMotor->setup();
    
    // Yaw thruster (steering)
    yawMotor = new Motor(26, 13, 11, 7);
    yawMotor->setup();
    
    // Vertical thruster (depth control)
    verticalMotor = new Motor(25, 10, 9, 8);
    verticalMotor->setup();
    
    // Trim ballast servo
    trimBallast = new ServoDriver(23);
    trimBallast->setup();
    
    Serial.println("Sub initialized - 3x Thrusters + Trim ready");
}

void Sub::loop() {
    updateThrusters(currentInputs.throttle, currentInputs.roll, 
                   currentInputs.pitch, currentInputs.yaw);
}

void Sub::setInputs(NAPacket* packet) {
    if (packet) {
        currentInputs.throttle = packet->throttle;
        currentInputs.roll = packet->roll;          // Steering/Yaw
        currentInputs.pitch = packet->pitch;        // Depth/Vertical
        currentInputs.yaw = packet->yaw;            // Trim
    }
}

void Sub::updateThrusters(int16_t throttle, int16_t steering, int16_t depth, int16_t yaw) {
    // Scale inputs
    throttle /= 10;
    steering /= 10;
    depth /= 10;
    yaw /= 10;
    
    // Apply to thrusters
    forwardMotor->setSpeed(throttle);
    yawMotor->setSpeed(steering);
    
    // Depth Control: Manual vs Auto
    if (DepthManager::getInstance().isDiving()) {
        // Map -1.0..1.0 from PDF to -100..100 for motor
        float depthOutput = DepthManager::getInstance().getVerticalOutput() * 100.0f;
        verticalMotor->setSpeed((int16_t)depthOutput);
    } else {
        verticalMotor->setSpeed(depth);
    }
    
    // Trim ballast (0-180 degrees, 90 = neutral)
    int16_t trimAngle = 90 + (yaw / 2);
    trimAngle = constrain(trimAngle, 45, 135);
    trimBallast->write(trimAngle);
}

void Sub::getMixedOutput(uint8_t *motorPwm, uint8_t motorCount) {
  if (motorCount >= 1 && forwardMotor) motorPwm[0] = (uint8_t)abs(forwardMotor->getCurrentSpeed());
  if (motorCount >= 2 && yawMotor) motorPwm[1] = (uint8_t)abs(yawMotor->getCurrentSpeed());
  if (motorCount >= 3 && verticalMotor) motorPwm[2] = (uint8_t)abs(verticalMotor->getCurrentSpeed());
}

String Sub::getName() const { return "SUB"; }
