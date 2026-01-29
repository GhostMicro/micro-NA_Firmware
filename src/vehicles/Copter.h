#ifndef COPTER_H
#define COPTER_H

#include "Vehicle.h"
#include "../drivers/Motor.h"

class Copter : public Vehicle {
public:
    Copter();
    void setup() override;
    void loop() override;
    void setInputs(NAPacket* packet) override;
    String getName() const override;
    
private:
    Motor* motors[4];  // FR, FL, BL, BR
    NAPacket currentInputs;
    
    void updateMotors(int16_t throttle, int16_t roll, int16_t pitch, int16_t yaw);
    void mixMotors(int16_t* motorOutputs);
};

#endif
