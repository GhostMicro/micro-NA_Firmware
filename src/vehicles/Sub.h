#ifndef SUB_H
#define SUB_H

#include "Vehicle.h"
#include "../drivers/Motor.h"
#include "../drivers/ServoDriver.h"

class Sub : public Vehicle {
public:
    Sub();
    void setup() override;
    void loop() override;
    void setInputs(NAPacket* packet) override;
    void getMixedOutput(uint8_t* motorPwm, uint8_t motorCount) override;
    String getName() const override;
    
private:
    Motor* forwardMotor;
    Motor* yawMotor;
    Motor* verticalMotor;
    ServoDriver* trimBallast;
    NAPacket currentInputs;
    
    void updateThrusters(int16_t throttle, int16_t steering, int16_t depth, int16_t yaw);
};

#endif
