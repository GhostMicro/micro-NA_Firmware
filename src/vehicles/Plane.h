#ifndef PLANE_H
#define PLANE_H

#include "Vehicle.h"
#include "../drivers/Motor.h"
#include "../drivers/ServoDriver.h"

class Plane : public Vehicle {
public:
    Plane();
    void setup() override;
    void loop() override;
    void setInputs(NAPacket* packet) override;
    String getName() const override;
    
private:
    Motor* motor;           // Throttle motor
    ServoDriver* ailerons;  // Left/Right wing control
    ServoDriver* elevator;  // Pitch control
    NAPacket currentInputs;
    
    void updateControls(int16_t throttle, int16_t roll, int16_t pitch);
};

#endif
