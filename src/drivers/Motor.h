#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

/**
 * Motor - PWM-controlled motor driver with deadband and acceleration ramping
 * 
 * Features:
 * - Deadband: Prevents motor creep at low inputs
 * - Ramping: Limits acceleration to prevent vehicle flip
 * - Speed range: -100 to +100
 */
class Motor {
public:
    /**
     * Constructor
     * @param pwmPin    ESP32 GPIO pin for PWM signal
     * @param dirPin1   Direction pin 1
     * @param dirPin2   Direction pin 2
     * @param channel   LEDC channel (0-15)
     */
    Motor(int pwmPin, int dirPin1, int dirPin2, int channel);
    
    void setup();
    
    /**
     * Set motor speed with deadband and ramping applied
     * @param speed Target speed (-100 to 100)
     */
    void setSpeed(int16_t speed);
    
    int16_t getCurrentSpeed() const { return lastSpeed; }

private:
    int _pwmPin, _dir1, _dir2, _channel;
    int16_t lastSpeed;
    uint32_t lastUpdateTime;
    
    /**
     * Apply deadband to input to prevent motor creep
     * Input between -MOTOR_DEADBAND and +MOTOR_DEADBAND becomes 0
     * 
     * @param input Raw speed input
     * @return Speed after deadband application
     */
    int16_t applyDeadband(int16_t input);
    
    /**
     * Apply ramping/acceleration limiting to smooth out control
     * Prevents sudden full acceleration which could flip vehicle
     * 
     * @param targetSpeed Desired speed after deadband
     * @return Speed limited by max ramp rate
     */
    int16_t applyRamping(int16_t targetSpeed);
};

#endif
