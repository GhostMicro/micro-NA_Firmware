#include "BatteryManager.h"

const float BatteryManager::DIVIDER_RATIO = 3.5f;
const float BatteryManager::ADC_REF_VOLTAGE = 3.3f;
const uint16_t BatteryManager::ADC_MAX = 4095;
const uint16_t BatteryManager::MIN_VOLTAGE_MV = 3000;
const uint16_t BatteryManager::MAX_VOLTAGE_MV = 4200;

BatteryManager::BatteryManager() : sampleIndex(0) {
    memset(adcSamples, 0, sizeof(adcSamples));
}

void BatteryManager::setup() {
    // Configure ADC for battery voltage
    pinMode(BATTERY_PIN, INPUT);
    analogSetPinAttenuation(BATTERY_PIN, ADC_11db);  // 11dB attenuation = max 3.3V
    
    // Take initial samples to fill buffer
    for (int i = 0; i < SMOOTH_SAMPLES; i++) {
        adcSamples[i] = analogRead(BATTERY_PIN);
        delay(5);
    }
    
    Serial.println("{\"msg\":\"BatteryManager initialized\"}");
}

uint16_t BatteryManager::getRawADC() {
    return analogRead(BATTERY_PIN);
}

uint16_t BatteryManager::smoothADC() {
    // Replace oldest sample with new reading
    adcSamples[sampleIndex] = analogRead(BATTERY_PIN);
    sampleIndex = (sampleIndex + 1) % SMOOTH_SAMPLES;
    
    // Calculate average
    uint32_t sum = 0;
    for (int i = 0; i < SMOOTH_SAMPLES; i++) {
        sum += adcSamples[i];
    }
    return sum / SMOOTH_SAMPLES;
}

uint16_t BatteryManager::getVoltageMillivolts() {
    uint16_t adcValue = smoothADC();
    
    // Convert ADC to voltage
    // ADC: 0-4095 maps to 0-3.3V
    // Actual voltage: ADC_voltage * divider_ratio
    float adcVoltage = (adcValue / (float)ADC_MAX) * ADC_REF_VOLTAGE;
    float batteryVoltage = adcVoltage * DIVIDER_RATIO;
    
    // Convert to millivolts
    uint16_t voltageMillivolts = (uint16_t)(batteryVoltage * 1000.0f);
    
    return voltageMillivolts;
}

uint8_t BatteryManager::getBatteryPercentage() {
    uint16_t voltageMillivolts = getVoltageMillivolts();
    
    // Clamp to valid range
    if (voltageMillivolts <= MIN_VOLTAGE_MV) {
        return 0;
    }
    if (voltageMillivolts >= MAX_VOLTAGE_MV) {
        return 100;
    }
    
    // Linear mapping: 3.0V = 0%, 4.2V = 100%
    uint8_t percentage = (uint8_t)(((voltageMillivolts - MIN_VOLTAGE_MV) * 100) / 
                                   (MAX_VOLTAGE_MV - MIN_VOLTAGE_MV));
    
    return constrain(percentage, 0, 100);
}

bool BatteryManager::isLow() {
    return getBatteryPercentage() < 10;
}
