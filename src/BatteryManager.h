#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>

/**
 * BatteryManager - Voltage monitoring and ADC management
 * 
 * Hardware:
 * - ADC GPIO: 34 (ADC1_CH6) - 12-bit reading (0-4095 range)
 * - Voltage divider: 100k + 47k = 3.5:1 ratio
 * - Battery voltage range: 3.0V to 4.2V (LiPo)
 * - Max ADC voltage: 3.3V
 * - Max battery voltage: 3.3V * 3.5 = 11.55V (but limit to ~5V for testing)
 * 
 * Formula: Voltage = (ADC_reading / 4095) * 3.3 * divider_ratio
 */
class BatteryManager {
public:
    BatteryManager();
    
    /**
     * Initialize ADC for battery voltage reading
     */
    void setup();
    
    /**
     * Read battery voltage from ADC
     * @return voltage in millivolts
     */
    uint16_t getVoltageMillivolts();
    
    /**
     * Get battery percentage (0-100%)
     * Based on voltage: 3.0V = 0%, 4.2V = 100% for LiPo
     * @return percentage 0-100
     */
    uint8_t getBatteryPercentage();
    
    /**
     * Check if battery is low (<10%)
     * @return true if battery is critically low
     */
    bool isLow();
    
    /**
     * Get raw ADC reading
     */
    uint16_t getRawADC();
    
private:
    static const uint8_t BATTERY_PIN = 34;      // GPIO 34 (ADC1_CH6)
    static const float DIVIDER_RATIO;           // 3.5:1 voltage divider
    static const float ADC_REF_VOLTAGE;         // 3.3V ESP32 reference
    static const uint16_t ADC_MAX;              // 12-bit: 4095
    static const uint16_t MIN_VOLTAGE_MV;      // 3000mV (3.0V)
    static const uint16_t MAX_VOLTAGE_MV;      // 4200mV (4.2V)
    static const uint16_t RTL_VOLTAGE_MV;      // 3400mV (3.4V) - Phase 14
    
    /**
     * Smooth ADC readings with moving average
     */
    uint16_t smoothADC();
    static const uint8_t SMOOTH_SAMPLES = 10;
    uint16_t adcSamples[SMOOTH_SAMPLES];
    uint8_t sampleIndex;
};

#endif // BATTERY_MANAGER_H
