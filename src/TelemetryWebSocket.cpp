#include "TelemetryWebSocket.h"
#include "NavigationManager.h"
#include "DepthManager.h"
#include <ArduinoJson.h>

TelemetryWebSocket& TelemetryWebSocket::getInstance() {
    static TelemetryWebSocket instance;
    return instance;
}

TelemetryWebSocket::TelemetryWebSocket() : _ws("/ws"), _lastBroadcast(0) {}

void TelemetryWebSocket::begin(AsyncWebServer* server) {
    if (!server) return;
    
    _ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        if (type == WS_EVT_CONNECT) {
            Serial.printf("[WS] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        } else if (type == WS_EVT_DISCONNECT) {
            Serial.printf("[WS] Client #%u disconnected\n", client->id());
        }
    });
    
    server->addHandler(&_ws);
    Serial.println("[WS] WebSocket Server Configured at /ws");
}

void TelemetryWebSocket::broadcast(const NATelemetry& data) {
    // Throttling
    if (millis() - _lastBroadcast < WS_BROADCAST_INTERVAL_MS) return;
    _lastBroadcast = millis();
    
    if (_ws.count() == 0) return; // No clients, save CPU

    // Serialize to JSON (Phase 11: Optimized JSON)
    // Format: {t:2, v:12.6, r:-60, s:1, u:1000}
    JsonDocument doc;
    doc["t"] = 2; // Type Telemetry
    doc["v"] = data.batteryVoltage;
    doc["r"] = data.rssi;
    doc["s"] = data.status;
    doc["u"] = data.uptime;
    
    // Phase 12/13: Position and Depth
    float lat, lng;
    NavigationManager::getInstance().getGPSLocation(lat, lng);
    doc["lat"] = lat;
    doc["lng"] = lng;
    doc["alt"] = DepthManager::getInstance().getActualDepth();
    
    // Check Encryption Status
    if (data.encryptionFlag) {
        doc["enc"] = 1;
    }
    
    // We could optimize by sending binary, but JSON is easier for web clients for now.
    // Length is roughly 60-80 bytes.
    String output;
    serializeJson(doc, output);
    
    _ws.textAll(output);
}

void TelemetryWebSocket::cleanUp() {
    _ws.cleanupClients();
}
