#include "Motor.h"

// Motor Configuration Constants
#define MOTOR_DEADBAND 10      // ±10 out of ±100 range
#define MOTOR_MAX_RAMP 5       // Max 5% change per 20ms cycle = 25% per 100ms
#define MOTOR_MIN_PWM 40       // Minimum PWM to overcome static friction

Motor::Motor(int pwmPin, int dirPin1, int dirPin2, int channel) 
    : _pwmPin(pwmPin), _dir1(dirPin1), _dir2(dirPin2), _channel(channel),
      lastSpeed(0), lastUpdateTime(0) {
}

void Motor::setup() {
    pinMode(_pwmPin, OUTPUT);
    pinMode(_dir1, OUTPUT);
    pinMode(_dir2, OUTPUT);
    
    // ESP32 PWM Setup (LEDC)
    ledcSetup(_channel, 1000, 8); // 1kHz, 8-bit
    ledcAttachPin(_pwmPin, _channel);
    
    lastUpdateTime = millis();
}

int16_t Motor::applyDeadband(int16_t input) {
    // Constraint input to valid range
    if (input > 100) input = 100;
    if (input < -100) input = -100;
    
    // Apply deadband: zero out small inputs
    if (input > -MOTOR_DEADBAND && input < MOTOR_DEADBAND) {
        return 0;
    }
    
    return input;
}

int16_t Motor::applyRamping(int16_t targetSpeed) {
    uint32_t currentTime = millis();
    uint32_t timeDelta = currentTime - lastUpdateTime;
    lastUpdateTime = currentTime;
    
    // Prevent division issues on first call
    if (timeDelta == 0) timeDelta = 1;
    
    // Calculate maximum change allowed based on time elapsed
    // MOTOR_MAX_RAMP % per 20ms = 5% per 20ms
    // For 20ms cycle: maxChange = 5
    // For 50ms cycle: maxChange = 12
    int16_t maxChange = (MOTOR_MAX_RAMP * timeDelta) / 20;
    if (maxChange < 1) maxChange = 1;
    
    // Limit acceleration
    int16_t speedDelta = targetSpeed - lastSpeed;
    if (speedDelta > maxChange) {
        speedDelta = maxChange;
    } else if (speedDelta < -maxChange) {
        speedDelta = -maxChange;
    }
    
    return lastSpeed + speedDelta;
}

void Motor::setSpeed(int16_t speed) {
    // Step 1: Apply deadband to prevent motor creep
    speed = applyDeadband(speed);
    
    // Step 2: Apply ramping to limit acceleration
    speed = applyRamping(speed);
    
    // Store for next call
    lastSpeed = speed;
    
    // Step 3: Convert to PWM output
    int pwmVal = 0;
    
    if (speed > 0) {
        // Forward direction
        digitalWrite(_dir1, HIGH);
        digitalWrite(_dir2, LOW);
        
        // Map -100..100 to 0..255 PWM with minimum threshold
        // Ensure minimum PWM to overcome friction
        pwmVal = map(speed, 1, 100, MOTOR_MIN_PWM, 255);
        
    } else if (speed < 0) {
        // Reverse direction
        digitalWrite(_dir1, LOW);
        digitalWrite(_dir2, HIGH);
        
        // Map -100..-1 to MOTOR_MIN_PWM..255
        pwmVal = map(abs(speed), 1, 100, MOTOR_MIN_PWM, 255);
        
    } else {
        // Stop
        digitalWrite(_dir1, LOW);
        digitalWrite(_dir2, LOW);
        pwmVal = 0;
    }
    
    // Constrain final PWM value
    if (pwmVal > 255) pwmVal = 255;
    if (pwmVal < 0) pwmVal = 0;
    
    ledcWrite(_channel, pwmVal);
}
