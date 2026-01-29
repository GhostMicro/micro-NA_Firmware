#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>

class SensorManager {
public:
    SensorManager();
    bool initI2C();
    bool detectMPU6050();
    bool detectPCA9685();
    bool detectOLED();
    void scanI2CBus();
    
private:
    static const uint8_t I2C_SDA = 21;
    static const uint8_t I2C_SCL = 22;
    static const uint8_t MPU6050_ADDR = 0x68;
    static const uint8_t PCA9685_ADDR = 0x40;
    static const uint8_t OLED_ADDR = 0x3C;
};

#endif
