#ifndef RSSI_MANAGER_H
#define RSSI_MANAGER_H

#include <Arduino.h>
#include <esp_now.h>

/**
 * RSSIManager - Signal strength monitoring from ESP-NOW
 * 
 * RSSI (Received Signal Strength Indicator):
 * - Range: -120 to 0 dBm (typical: -50 to -80 dBm good signal)
 * - ESP-NOW callback provides RSSI in packet info
 * - Converted to percentage: 0% = -120dBm (very bad), 100% = 0dBm (ideal)
 */
class RSSIManager {
public:
    RSSIManager();
    
    /**
     * Update RSSI value from ESP-NOW packet
     * @param rssi signal strength in dBm (negative value)
     */
    void updateRSSI(int8_t rssi);
    
    /**
     * Get current RSSI as percentage (0-100%)
     * @return percentage 0-100
     */
    uint8_t getRSSIPercentage();
    
    /**
     * Get current RSSI in dBm
     * @return RSSI in dBm (negative value)
     */
    int8_t getRSSI_dBm();
    
    /**
     * Get signal quality assessment
     * @return "EXCELLENT", "GOOD", "FAIR", "POOR", or "NO_SIGNAL"
     */
    const char* getSignalQuality();
    
    /**
     * Check if signal is lost (no packets received recently)
     * @return true if last update > 1000ms ago
     */
    bool isSignalLost();
    
private:
    int8_t currentRSSI_dBm;
    uint32_t lastUpdateTime;
    
    static const int8_t RSSI_MIN;       // -120 dBm (very bad)
    static const int8_t RSSI_MAX;       // 0 dBm (ideal)
    static const int8_t RSSI_EXCELLENT; // -50 dBm
    static const int8_t RSSI_GOOD;      // -60 dBm
    static const int8_t RSSI_FAIR;      // -75 dBm
    static const uint32_t SIGNAL_TIMEOUT; // 1000ms
    
    /**
     * Convert dBm to percentage
     */
    uint8_t dbmToPercentage(int8_t rssi_dbm);
};

#endif // RSSI_MANAGER_H
