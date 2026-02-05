#include "NavigationManager.h"
#include <math.h>

NavigationManager& NavigationManager::getInstance() {
    static NavigationManager instance;
    return instance;
}

NavigationManager::NavigationManager() {
    _state.isMissionActive = false;
    _state.currentWaypointIndex = 0;
    _state.isWaypointReached = false;
    _state.isRTLActive = false;
    _state.homeLat = 0;
    _state.homeLng = 0;
    _prevError = 0;
    _integral = 0;
}

void NavigationManager::init() {
    // Reset state
    _state.currentWaypointIndex = 0;
    _state.isMissionActive = false;
}

void NavigationManager::feedGPS(char c) {
    _gps.encode(c);
}

bool NavigationManager::isGPSLocked() {
    return _gps.location.isValid() && _gps.location.age() < 1500;
}

void NavigationManager::getGPSLocation(float& lat, float& lng) {
    if (isGPSLocked()) {
        lat = _gps.location.lat();
        lng = _gps.location.lng();
    }
}

float NavigationManager::getGPSCourse() {
    if (_gps.course.isValid()) {
        return (float)_gps.course.deg();
    }
    return 0.0f;
}

void NavigationManager::startMission() {
    if (WaypointManager::getInstance().getWaypointCount() > 0) {
        _state.isMissionActive = true;
        _state.isRTLActive = false;
        _state.currentWaypointIndex = 0;
        Serial.println("[Nav] Mission Started");
    }
}

void NavigationManager::stopMission() {
    _state.isMissionActive = false;
    _state.isRTLActive = false;
    Serial.println("[Nav] Mission Stopped");
}

void NavigationManager::setHome(float lat, float lng) {
    _state.homeLat = lat;
    _state.homeLng = lng;
    Serial.printf("[Nav] Home Set: %.6f, %.6f\n", lat, lng);
}

void NavigationManager::executeRTL() {
    if (_state.homeLat == 0 && _state.homeLng == 0) {
        Serial.println("[Nav] RTL Failed: Home not set");
        return;
    }
    _state.isRTLActive = true;
    _state.isMissionActive = true;
    _state.currentWaypointIndex = 0; // RTL uses a virtual path/direct to home
    Serial.println("[Nav] RTL Active: Returning Home...");
}

void NavigationManager::update(float currentLat, float currentLng, float currentHeading) {
    if (!_state.isMissionActive) return;

    NAWaypoint targetWP;
    if (!WaypointManager::getInstance().getWaypoint(_state.currentWaypointIndex, targetWP)) {
        stopMission();
        return;
    }

    _state.distanceToTarget = calculateDistance(currentLat, currentLng, targetWP.lat, targetWP.lng);
    _state.bearingToTarget = calculateBearing(currentLat, currentLng, targetWP.lat, targetWP.lng);
    
    float error = _state.bearingToTarget - currentHeading;
    _state.headingError = normalizeAngle(error);

    // Check if reached
    if (_state.distanceToTarget < WP_RADIUS_METERS) {
        Serial.printf("[Nav] Waypoint %d Reached!\n", _state.currentWaypointIndex);
        _state.currentWaypointIndex++;
        if (_state.currentWaypointIndex >= WaypointManager::getInstance().getWaypointCount()) {
            Serial.println("[Nav] Mission Complete");
            stopMission();
        }
    }
}

bool NavigationManager::getNavigationOutput(int16_t& throttleOut, int16_t& yawOut) {
    if (!_state.isMissionActive) return false;

    // Simple PID for Yaw
    float error = _state.headingError;
    
    // P-Term
    float output = error * NAV_YAW_KP;
    
    // Limits
    if (output > MAX_NAV_OUTPUT) output = MAX_NAV_OUTPUT;
    if (output < -MAX_NAV_OUTPUT) output = -MAX_NAV_OUTPUT;

    yawOut = (int16_t)output;
    
    NAWaypoint targetWP;
    WaypointManager::getInstance().getWaypoint(_state.currentWaypointIndex, targetWP);
    throttleOut = targetWP.speed; // Use WP speed as target throttle

    return true;
}

// ============================================================================
// Internal Math Helpers (Haversine & Bearing)
// ============================================================================

#define DEGREES_TO_RADIANS (M_PI / 180.0)
#define RADIANS_TO_DEGREES (180.0 / M_PI)
#define EARTH_RADIUS_M 6371000

float NavigationManager::calculateDistance(float lat1, float lng1, float lat2, float lng2) {
    float dLat = (lat2 - lat1) * DEGREES_TO_RADIANS;
    float dLon = (lng2 - lng1) * DEGREES_TO_RADIANS;
    
    float a = sin(dLat/2) * sin(dLat/2) +
              cos(lat1 * DEGREES_TO_RADIANS) * cos(lat2 * DEGREES_TO_RADIANS) * 
              sin(dLon/2) * sin(dLon/2);
    
    float c = 2 * atan2(sqrt(a), sqrt(1-a));
    return EARTH_RADIUS_M * c;
}

float NavigationManager::calculateBearing(float lat1, float lng1, float lat2, float lng2) {
    float dLon = (lng2 - lng1) * DEGREES_TO_RADIANS;
    float lat1Rad = lat1 * DEGREES_TO_RADIANS;
    float lat2Rad = lat2 * DEGREES_TO_RADIANS;
    
    float y = sin(dLon) * cos(lat2Rad);
    float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
    
    float brng = atan2(y, x) * RADIANS_TO_DEGREES;
    return normalizeAngle(brng);
}

float NavigationManager::normalizeAngle(float angle) {
    while (angle > 180) angle -= 360;
    while (angle < -180) angle += 360;
    return angle;
}
