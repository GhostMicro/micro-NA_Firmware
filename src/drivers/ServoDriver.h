#ifndef SERVO_DRIVER_H
#define SERVO_DRIVER_H

#include <Arduino.h>
#include <ESP32Servo.h>

class ServoDriver {
public:
    ServoDriver(int pin);
    ServoDriver(int pin, int initialAngle);
    void setup();
    void write(int angle); // 0-180

private:
    int _pin;
    int _centerAngle;
    Servo _servo;
};

#endif
