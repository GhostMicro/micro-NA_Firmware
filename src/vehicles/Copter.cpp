#include "Copter.h"

Copter::Copter() {
    for (int i = 0; i < 4; i++) motors[i] = nullptr;
    memset(&currentInputs, 0, sizeof(NAPacket));
}

void Copter::setup() {
    // Initialize Motors (Standard Quad X)
    // Initialize Motors (Standard Quad X) 
    // Format: Motor(pwmPin, dirPin1, dirPin2, channel)
    // Using placeholder direction pins (32, 33, 34, 35 for directions)
    motors[0] = new Motor(16, 32, 33, 0); motors[0]->setup(); // FR
    motors[1] = new Motor(17, 34, 35, 1); motors[1]->setup(); // FL
    motors[2] = new Motor(18, 25, 26, 2); motors[2]->setup(); // BL
    motors[3] = new Motor(19, 27, 14, 3); motors[3]->setup(); // BR

    Serial.println("Copter initialized - 4x Motors ready");
}

void Copter::loop() {
    // Safety timeout check
    // In real implementation, FailsafeManager handles this
}

void Copter::setInputs(NAPacket* packet) {
    if (packet) {
        currentInputs.throttle = packet->throttle;
        currentInputs.roll = packet->roll;
        currentInputs.pitch = packet->pitch;
        currentInputs.yaw = packet->yaw;
        currentInputs.mode = packet->mode;
        currentInputs.buttons = packet->buttons;
    }
}

void Copter::updateMotors(int16_t throttle, int16_t roll, int16_t pitch, int16_t yaw) {
    // Scale inputs from -1000..1000 to -100..100
    throttle /= 10;
    roll /= 10;
    pitch /= 10;
    yaw /= 10;
    
    // X-configuration quadcopter mixing:
    // Motor layout:
    //    2(BL)   1(FL)
    //      \    /
    //       \  /
    //       /  \
    //      /    \
    //    3(BR)   0(FR)
    
    int16_t motorOutputs[4];
    motorOutputs[0] = throttle - roll + pitch - yaw;   // FR
    motorOutputs[1] = throttle + roll + pitch + yaw;   // FL
    motorOutputs[2] = throttle + roll - pitch - yaw;   // BL
    motorOutputs[3] = throttle - roll - pitch + yaw;   // BR
    
    mixMotors(motorOutputs);
}

void Copter::mixMotors(int16_t* motorOutputs) {
    // Find max output to check for saturation
    int16_t maxOutput = 0;
    for (int i = 0; i < 4; i++) {
        int16_t absVal = motorOutputs[i] < 0 ? -motorOutputs[i] : motorOutputs[i];
        if (absVal > maxOutput) maxOutput = absVal;
    }
    
    // Normalize if any motor would saturate
    if (maxOutput > 100) {
        int16_t scale = (100 * 100) / maxOutput;
        for (int i = 0; i < 4; i++) {
            motorOutputs[i] = (motorOutputs[i] * scale) / 100;
        }
    }
    
    // Apply to hardware
    for (int i = 0; i < 4; i++) {
        motors[i]->setSpeed(motorOutputs[i]);
    }
}

void Copter::getMixedOutput(uint8_t *motorPwm, uint8_t motorCount) {
  // Return current motor speeds scaled to 0-255 or 0-100
  for (int i = 0; i < 4 && i < motorCount; i++) {
    if (motors[i]) {
      motorPwm[i] = (uint8_t)abs(motors[i]->getCurrentSpeed());
    } else {
      motorPwm[i] = 0;
    }
  }
}

String Copter::getName() const { return "COPTER"; }
