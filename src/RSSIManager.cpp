#include "RSSIManager.h"

const int8_t RSSIManager::RSSI_MIN = -120;
const int8_t RSSIManager::RSSI_MAX = 0;
const int8_t RSSIManager::RSSI_EXCELLENT = -50;
const int8_t RSSIManager::RSSI_GOOD = -60;
const int8_t RSSIManager::RSSI_FAIR = -75;
const uint32_t RSSIManager::SIGNAL_TIMEOUT = 1000;  // milliseconds

RSSIManager::RSSIManager() : currentRSSI_dBm(RSSI_MIN), lastUpdateTime(0) {}

void RSSIManager::updateRSSI(int8_t rssi) {
    // Clamp to valid range
    currentRSSI_dBm = constrain(rssi, RSSI_MIN, RSSI_MAX);
    lastUpdateTime = millis();
}

uint8_t RSSIManager::dbmToPercentage(int8_t rssi_dbm) {
    // Convert from dBm range to 0-100%
    // -120 dBm = 0%, 0 dBm = 100%
    if (rssi_dbm <= RSSI_MIN) return 0;
    if (rssi_dbm >= RSSI_MAX) return 100;
    
    uint8_t percentage = (uint8_t)(((rssi_dbm - RSSI_MIN) * 100) / (RSSI_MAX - RSSI_MIN));
    return constrain(percentage, 0, 100);
}

uint8_t RSSIManager::getRSSIPercentage() {
    if (isSignalLost()) {
        return 0;
    }
    return dbmToPercentage(currentRSSI_dBm);
}

int8_t RSSIManager::getRSSI_dBm() {
    if (isSignalLost()) {
        return RSSI_MIN;
    }
    return currentRSSI_dBm;
}

const char* RSSIManager::getSignalQuality() {
    if (isSignalLost()) {
        return "NO_SIGNAL";
    }
    
    if (currentRSSI_dBm >= RSSI_EXCELLENT) {
        return "EXCELLENT";
    } else if (currentRSSI_dBm >= RSSI_GOOD) {
        return "GOOD";
    } else if (currentRSSI_dBm >= RSSI_FAIR) {
        return "FAIR";
    } else {
        return "POOR";
    }
}

bool RSSIManager::isSignalLost() {
    uint32_t timeSinceUpdate = millis() - lastUpdateTime;
    return timeSinceUpdate > SIGNAL_TIMEOUT;
}
