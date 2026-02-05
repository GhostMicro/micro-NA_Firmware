#ifndef NAVIGATION_MANAGER_H
#define NAVIGATION_MANAGER_H

#include <Arduino.h>
#include "WaypointManager.h"
#include <TinyGPS++.h>

// PID Constants (Tunable)
#define NAV_YAW_KP 2.0f
#define NAV_YAW_KI 0.0f
#define NAV_YAW_KD 0.1f

#define WP_RADIUS_METERS 5.0f   // Distance to consider WP reached
#define MAX_NAV_OUTPUT 500      // Max steering override

struct NavigationState {
    float distanceToTarget; // Meters
    float bearingToTarget;  // Degrees
    float headingError;     // Degrees (-180 to 180)
    uint8_t currentWaypointIndex;
    bool isMissionActive;
    bool isWaypointReached;
    bool isRTLActive;       // Phase 14: RTL status
    float homeLat;          // Phase 14: Home coordinates
    float homeLng;
};

class NavigationManager {
public:
    static NavigationManager& getInstance();

    void init();
    void update(float currentLat, float currentLng, float currentHeading);
    
    // GPS Feed
    void feedGPS(char c); // Feed NMEA chars
    bool isGPSLocked();
    void getGPSLocation(float& lat, float& lng);
    float getGPSCourse(); // Returns course in degrees (0-360)

    // Control Output
    bool getNavigationOutput(int16_t& throttleOut, int16_t& yawOut);
    
    // Mission Control
    void startMission();
    void stopMission();
    void setHome(float lat, float lng); // Phase 14: Set home location
    void executeRTL();                  // Phase 14: Return to home
    
    NavigationState getState() { return _state; }

private:
    NavigationManager();
    
    TinyGPSPlus _gps;
    NavigationState _state;
    
    // PID State
    float _prevError;
    float _integral;
    
    // Helper Math
    float calculateBearing(float lat1, float lng1, float lat2, float lng2);
    float calculateDistance(float lat1, float lng1, float lat2, float lng2);
    float normalizeAngle(float angle);
};

#endif // NAVIGATION_MANAGER_H
