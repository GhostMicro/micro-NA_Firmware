#ifndef WAYPOINT_MANAGER_H
#define WAYPOINT_MANAGER_H

#include <Arduino.h>
#include <vector>
#include "NAPacket.h" // For NAWaypoint definitions
#include "ConfigManager.h"

#define MAX_WAYPOINTS 50
#define WAYPOINT_NVS_KEY "mission_wp"
#define WAYPOINT_COUNT_KEY "mission_cnt"

class WaypointManager {
public:
    static WaypointManager& getInstance();

    // Mission Management
    bool addWaypoint(float lat, float lng, float alt, uint16_t speed);
    bool setWaypoint(uint8_t index, const NAWaypoint& wp);
    bool clearMission();
    
    // Mission Retrieval
    uint8_t getWaypointCount();
    bool getWaypoint(uint8_t index, NAWaypoint& wp);
    const std::vector<NAWaypoint>& getMission() { return _waypoints; }

    // Persistence
    bool saveToNVS();
    bool loadFromNVS();

    // Home Location (Return to Launch)
    void setHome(float lat, float lng);
    bool getHome(float& lat, float& lng);
    bool hasHome() { return _homeSet; }

private:
    WaypointManager();
    
    std::vector<NAWaypoint> _waypoints;
    NAWaypoint _home;
    bool _homeSet;
};

#endif // WAYPOINT_MANAGER_H
