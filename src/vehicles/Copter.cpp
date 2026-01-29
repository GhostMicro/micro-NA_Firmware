#include "Copter.h"

Copter::Copter() {
    for (int i = 0; i < 4; i++) motors[i] = nullptr;
    memset(&currentInputs, 0, sizeof(NAPacket));
}

void Copter::setup() {
    // Initialize 4 motors
    // Motor 0: FR (Front-Right) - GPIO 16, Channel 2
    motors[0] = new Motor(16, 32, 33, 2);
    motors[0]->setup();
    
    // Motor 1: FL (Front-Left) - GPIO 17, Channel 3
    motors[1] = new Motor(17, 34, 35, 3);
    motors[1]->setup();
    
    // Motor 2: BL (Back-Left) - GPIO 18, Channel 4
    motors[2] = new Motor(18, 36, 39, 4);
    motors[2]->setup();
    
    // Motor 3: BR (Back-Right) - GPIO 19, Channel 5
    motors[3] = new Motor(19, 25, 26, 5);
    motors[3]->setup();
    
    Serial.println("Copter initialized - 4x Motors ready");
}

void Copter::loop() {
    updateMotors(currentInputs.throttle, currentInputs.roll, 
                 currentInputs.pitch, currentInputs.yaw);
}

void Copter::setInputs(NAPacket* packet) {
    if (packet) {
        currentInputs.throttle = packet->throttle;
        currentInputs.roll = packet->roll;
        currentInputs.pitch = packet->pitch;
        currentInputs.yaw = packet->yaw;
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

String Copter::getName() const {
    return "COPTER";
}
