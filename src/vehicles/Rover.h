#ifndef ROVER_H
#define ROVER_H

#include "Vehicle.h"
#include "../drivers/Motor.h"

class Rover : public Vehicle {
public:
    Rover();
    void setup() override;
    void loop() override;
    void setInputs(NAPacket* packet) override;
    String getName() const override;
    
private:
    Motor* motorLeft;
    Motor* motorRight;
    NAPacket currentInputs;
    
    void drive(int16_t throttle, int16_t steering);
};

#endif
