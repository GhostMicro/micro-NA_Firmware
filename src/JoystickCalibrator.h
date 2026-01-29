#ifndef JOYSTICK_CALIBRATOR_H
#define JOYSTICK_CALIBRATOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "ConfigManager.h"

/**
 * JoystickCalibrator - Multi-axis joystick calibration and mapping
 * 
 * Calibration process on Land Station:
 * 1. Move throttle to minimum
 * 2. Move throttle to center
 * 3. Move throttle to maximum
 * 4. Repeat for Roll, Pitch, Yaw
 * 5. Store calibration in NVS via ConfigManager
 * 
 * During operation:
 * - Raw ADC values (0-1023) mapped to scaled output (-1000 to +1000)
 * - Deadzone applied to prevent stick drift
 * - Smooth transitions with min/max scaling
 */
class JoystickCalibrator {
public:
    enum CalibrationAxis {
        AXIS_THROTTLE = 0,
        AXIS_ROLL = 1,
        AXIS_PITCH = 2,
        AXIS_YAW = 3
    };
    
    enum CalibrationStep {
        STEP_IDLE = 0,
        STEP_MIN = 1,
        STEP_CENTER = 2,
        STEP_MAX = 3,
        STEP_COMPLETE = 4
    };
    
    JoystickCalibrator(ConfigManager* configManager);
    
    /**
     * Start calibration sequence on specified axis
     */
    void beginCalibration(CalibrationAxis axis);
    
    /**
     * Record calibration point (min/center/max)
     * @param rawValue raw ADC value (0-1023)
     */
    void recordCalibrationPoint(uint16_t rawValue);
    
    /**
     * Get current calibration step
     */
    CalibrationStep getCurrentStep();
    
    /**
     * Get current calibration axis
     */
    CalibrationAxis getCurrentAxis();
    
    /**
     * Cancel calibration without saving
     */
    void cancelCalibration();
    
    /**
     * Save calibration to NVS
     */
    bool saveCalibration();
    
    /**
     * Convert raw ADC value to scaled output (-1000 to +1000)
     * Applies calibration mapping and deadzone
     */
    int16_t mapJoystickAxis(CalibrationAxis axis, uint16_t rawValue);
    
    /**
     * Apply deadzone to axis value
     * @param axis calibration axis
     * @param value scaled value (-1000 to +1000)
     * @return dead-zoned value
     */
    int16_t applyDeadzone(CalibrationAxis axis, int16_t value);
    
    /**
     * Export calibration status to JSON
     */
    void exportStatus(JsonDocument& doc);
    
    /**
     * Reset calibration to defaults
     */
    void resetCalibration();
    
private:
    ConfigManager* configManager;
    
    CalibrationAxis currentAxis;
    CalibrationStep currentStep;
    
    // Temporary calibration data during process
    ConfigManager::JoystickCalibration tempCalibration;
    ConfigManager::DeadzoneConfig deadzoneConfig;
    
    /**
     * Get min/center/max pointers for current axis
     */
    void getAxisPointers(int16_t*& minPtr, int16_t*& centerPtr, int16_t*& maxPtr);
    
    /**
     * Map raw value using min/center/max calibration
     */
    int16_t mapAxis(uint16_t rawValue, int16_t minVal, int16_t centerVal, int16_t maxVal);
};

#endif // JOYSTICK_CALIBRATOR_H
