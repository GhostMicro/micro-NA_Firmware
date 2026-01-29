#include "SensorManager.h"

SensorManager::SensorManager() {}

bool SensorManager::initI2C() {
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(100);
    Serial.println("[I2C] Bus initialized");
    scanI2CBus();
    return true;
}

bool SensorManager::detectMPU6050() {
    // Timeout-protected detection (100ms max)
    const uint32_t timeout = millis() + 100;
    Wire.beginTransmission(MPU6050_ADDR);
    byte error = Wire.endTransmission();
    
    if (millis() > timeout) {
        Serial.println("[I2C] MPU6050 detection timeout");
        return false;
    }
    return error == 0;
}

bool SensorManager::detectPCA9685() {
    // Timeout-protected detection (100ms max)
    const uint32_t timeout = millis() + 100;
    Wire.beginTransmission(PCA9685_ADDR);
    byte error = Wire.endTransmission();
    
    if (millis() > timeout) {
        Serial.println("[I2C] PCA9685 detection timeout");
        return false;
    }
    return error == 0;
}

bool SensorManager::detectOLED() {
    // Timeout-protected detection (500ms max) - OLED init can be slow
    const uint32_t timeout = millis() + 500;
    Wire.beginTransmission(OLED_ADDR);
    byte error = Wire.endTransmission();
    
    if (millis() > timeout) {
        Serial.println("[OLED] Detection timeout - falling back to telemetry-only mode");
        return false;
    }
    
    if (error == 0) {
        return true;
    } else {
        Serial.println("[OLED] Not detected (0x3C) - telemetry will use serial only");
        return false;
    }
}

void SensorManager::scanI2CBus() {
    Serial.println("[I2C] Scanning bus...");
    int deviceCount = 0;
    for (byte i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        byte error = Wire.endTransmission();
        if (error == 0) {
            Serial.printf("[I2C] Device found at 0x%02X\n", i);
            deviceCount++;
        }
    }
    Serial.printf("[I2C] Total devices: %d\n", deviceCount);
}
