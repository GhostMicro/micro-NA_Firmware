#include "JoystickCalibrator.h"

JoystickCalibrator::JoystickCalibrator(ConfigManager* configManager)
    : configManager(configManager), currentAxis(AXIS_THROTTLE), currentStep(STEP_IDLE) {
    
    if (configManager) {
        tempCalibration = configManager->getJoystickCalibration();
        deadzoneConfig = configManager->getDeadzoneConfig();
    }
}

void JoystickCalibrator::beginCalibration(CalibrationAxis axis) {
    currentAxis = axis;
    currentStep = STEP_MIN;
    
    StaticJsonDocument<128> doc;
    doc["msg"] = "Calibration started";
    doc["axis"] = (int)axis;
    doc["axis_name"] = (axis == AXIS_THROTTLE ? "THROTTLE" : 
                        axis == AXIS_ROLL ? "ROLL" :
                        axis == AXIS_PITCH ? "PITCH" : "YAW");
    doc["step"] = "Move to MINIMUM";
    serializeJson(doc, Serial);
    Serial.println();
}

void JoystickCalibrator::recordCalibrationPoint(uint16_t rawValue) {
    if (currentStep == STEP_IDLE) return;
    
    int16_t *minPtr, *centerPtr, *maxPtr;
    getAxisPointers(minPtr, centerPtr, maxPtr);
    
    if (currentStep == STEP_MIN) {
        *minPtr = rawValue;
        currentStep = STEP_CENTER;
        Serial.println("{\"msg\":\"Min recorded. Move to CENTER\"}");
    } else if (currentStep == STEP_CENTER) {
        *centerPtr = rawValue;
        currentStep = STEP_MAX;
        Serial.println("{\"msg\":\"Center recorded. Move to MAXIMUM\"}");
    } else if (currentStep == STEP_MAX) {
        *maxPtr = rawValue;
        currentStep = STEP_COMPLETE;
        
        StaticJsonDocument<128> doc;
        doc["msg"] = "Calibration complete for axis";
        doc["axis"] = (int)currentAxis;
        serializeJson(doc, Serial);
        Serial.println();
    }
}

JoystickCalibrator::CalibrationStep JoystickCalibrator::getCurrentStep() {
    return currentStep;
}

JoystickCalibrator::CalibrationAxis JoystickCalibrator::getCurrentAxis() {
    return currentAxis;
}

void JoystickCalibrator::cancelCalibration() {
    currentStep = STEP_IDLE;
    Serial.println("{\"msg\":\"Calibration cancelled\"}");
}

bool JoystickCalibrator::saveCalibration() {
    if (!configManager) {
        Serial.println("{\"err\":\"ConfigManager not available\"}");
        return false;
    }
    
    configManager->setJoystickCalibration(tempCalibration);
    currentStep = STEP_IDLE;
    return true;
}

void JoystickCalibrator::getAxisPointers(int16_t*& minPtr, int16_t*& centerPtr, int16_t*& maxPtr) {
    switch (currentAxis) {
        case AXIS_THROTTLE:
            minPtr = &tempCalibration.minThrottle;
            centerPtr = &tempCalibration.centerThrottle;
            maxPtr = &tempCalibration.maxThrottle;
            break;
        case AXIS_ROLL:
            minPtr = &tempCalibration.minRoll;
            centerPtr = &tempCalibration.centerRoll;
            maxPtr = &tempCalibration.maxRoll;
            break;
        case AXIS_PITCH:
            minPtr = &tempCalibration.minPitch;
            centerPtr = &tempCalibration.centerPitch;
            maxPtr = &tempCalibration.maxPitch;
            break;
        case AXIS_YAW:
            minPtr = &tempCalibration.minYaw;
            centerPtr = &tempCalibration.centerYaw;
            maxPtr = &tempCalibration.maxYaw;
            break;
    }
}

int16_t JoystickCalibrator::mapAxis(uint16_t rawValue, int16_t minVal, int16_t centerVal, int16_t maxVal) {
    // Handle inverted axes (min > max)
    if (minVal > maxVal) {
        // Swap values
        int16_t temp = minVal;
        minVal = maxVal;
        maxVal = temp;
    }
    
    int16_t mappedValue = 0;
    
    if (rawValue < centerVal) {
        // Map lower half: minVal to centerVal -> -1000 to 0
        if (centerVal != minVal) {
            mappedValue = (int16_t)(((rawValue - centerVal) * -1000) / (centerVal - minVal));
        }
    } else if (rawValue > centerVal) {
        // Map upper half: centerVal to maxVal -> 0 to +1000
        if (maxVal != centerVal) {
            mappedValue = (int16_t)(((rawValue - centerVal) * 1000) / (maxVal - centerVal));
        }
    }
    
    // Constrain to valid range
    return constrain(mappedValue, -1000, 1000);
}

int16_t JoystickCalibrator::mapJoystickAxis(CalibrationAxis axis, uint16_t rawValue) {
    int16_t minVal, centerVal, maxVal;
    
    switch (axis) {
        case AXIS_THROTTLE:
            minVal = tempCalibration.minThrottle;
            centerVal = tempCalibration.centerThrottle;
            maxVal = tempCalibration.maxThrottle;
            break;
        case AXIS_ROLL:
            minVal = tempCalibration.minRoll;
            centerVal = tempCalibration.centerRoll;
            maxVal = tempCalibration.maxRoll;
            break;
        case AXIS_PITCH:
            minVal = tempCalibration.minPitch;
            centerVal = tempCalibration.centerPitch;
            maxVal = tempCalibration.maxPitch;
            break;
        case AXIS_YAW:
            minVal = tempCalibration.minYaw;
            centerVal = tempCalibration.centerYaw;
            maxVal = tempCalibration.maxYaw;
            break;
    }
    
    int16_t mapped = mapAxis(rawValue, minVal, centerVal, maxVal);
    return applyDeadzone(axis, mapped);
}

int16_t JoystickCalibrator::applyDeadzone(CalibrationAxis axis, int16_t value) {
    uint8_t deadzone = 5;  // Default 5%
    
    switch (axis) {
        case AXIS_THROTTLE:
            deadzone = deadzoneConfig.throttle;
            break;
        case AXIS_ROLL:
            deadzone = deadzoneConfig.roll;
            break;
        case AXIS_PITCH:
            deadzone = deadzoneConfig.pitch;
            break;
        case AXIS_YAW:
            deadzone = deadzoneConfig.yaw;
            break;
    }
    
    // Convert percentage to absolute value (5% of 1000 = 50)
    int16_t deadzoneBand = (deadzone * 1000) / 100;
    
    // Apply deadzone
    if (value > -deadzoneBand && value < deadzoneBand) {
        return 0;
    }
    
    return value;
}

void JoystickCalibrator::exportStatus(JsonDocument& doc) {
    doc["calibrating"] = (currentStep != STEP_IDLE);
    doc["axis"] = (int)currentAxis;
    doc["step"] = (int)currentStep;
}

void JoystickCalibrator::resetCalibration() {
    if (configManager) {
        configManager->resetJoystickCalibration();
        tempCalibration = configManager->getJoystickCalibration();
    }
    currentStep = STEP_IDLE;
    Serial.println("{\"msg\":\"Joystick calibration reset to defaults\"}");
}
