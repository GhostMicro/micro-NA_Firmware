#include "WaypointManager.h"

WaypointManager& WaypointManager::getInstance() {
    static WaypointManager instance;
    return instance;
}

WaypointManager::WaypointManager() : _homeSet(false) {
    _waypoints.reserve(MAX_WAYPOINTS);
}

bool WaypointManager::addWaypoint(float lat, float lng, float alt, uint16_t speed) {
    if (_waypoints.size() >= MAX_WAYPOINTS) return false;
    
    NAWaypoint wp;
    wp.lat = lat;
    wp.lng = lng;
    wp.alt = alt;
    wp.speed = speed;
    _waypoints.push_back(wp);
    return true;
}

bool WaypointManager::setWaypoint(uint8_t index, const NAWaypoint& wp) {
    if (index >= MAX_WAYPOINTS) return false;
    
    // Resize if needed (allow setting index 0 even if empty, logic wise acts as add if sequential)
    if (index >= _waypoints.size()) {
       if (index == _waypoints.size()) {
           _waypoints.push_back(wp);
           return true;
       }
       return false; // Can't skip indices
    }
    
    _waypoints[index] = wp;
    return true;
}

bool WaypointManager::clearMission() {
    _waypoints.clear();
    // Also clear NVS
    ConfigManager::setInt(WAYPOINT_COUNT_KEY, 0);
    return true;
}

uint8_t WaypointManager::getWaypointCount() {
    return (uint8_t)_waypoints.size();
}

bool WaypointManager::getWaypoint(uint8_t index, NAWaypoint& wp) {
    if (index >= _waypoints.size()) return false;
    wp = _waypoints[index];
    return true;
}

bool WaypointManager::saveToNVS() {
    uint8_t count = (uint8_t)_waypoints.size();
    ConfigManager::setInt(WAYPOINT_COUNT_KEY, count);
    
    for (uint8_t i = 0; i < count; i++) {
        char key[16];
        snprintf(key, 16, "wp_lat_%d", i);
        ConfigManager::setFloat(key, _waypoints[i].lat);
        snprintf(key, 16, "wp_lng_%d", i);
        ConfigManager::setFloat(key, _waypoints[i].lng);
        snprintf(key, 16, "wp_alt_%d", i);
        ConfigManager::setFloat(key, _waypoints[i].alt);
        snprintf(key, 16, "wp_spd_%d", i);
        ConfigManager::setInt(key, _waypoints[i].speed);
    }
    return true; 
}

bool WaypointManager::loadFromNVS() {
    int count = ConfigManager::getInt(WAYPOINT_COUNT_KEY, 0);
    if (count > MAX_WAYPOINTS) count = MAX_WAYPOINTS;
    
    _waypoints.clear();
    for (int i = 0; i < count; i++) {
        NAWaypoint wp;
        char key[16];
        snprintf(key, 16, "wp_lat_%d", i);
        wp.lat = ConfigManager::getFloat(key, 0.0f);
        snprintf(key, 16, "wp_lng_%d", i);
        wp.lng = ConfigManager::getFloat(key, 0.0f);
        snprintf(key, 16, "wp_alt_%d", i);
        wp.alt = ConfigManager::getFloat(key, 0.0f);
        snprintf(key, 16, "wp_spd_%d", i);
        wp.speed = ConfigManager::getInt(key, 1500);
        _waypoints.push_back(wp);
    }
    return true;
}

void WaypointManager::setHome(float lat, float lng) {
    _home.lat = lat;
    _home.lng = lng;
    _home.alt = 0;
    _homeSet = true;
}

bool WaypointManager::getHome(float& lat, float& lng) {
    if (!_homeSet) return false;
    lat = _home.lat;
    lng = _home.lng;
    return true;
}
