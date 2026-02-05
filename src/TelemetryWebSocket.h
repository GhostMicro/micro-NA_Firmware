#ifndef TELEMETRY_WEBSOCKET_H
#define TELEMETRY_WEBSOCKET_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "NAPacket.h"

// Rate limit broadcasts to save bandwidth/CPU
// 20Hz target = 50ms interval
#define WS_BROADCAST_INTERVAL_MS 50

class TelemetryWebSocket {
public:
    static TelemetryWebSocket& getInstance();

    void begin(AsyncWebServer* server);
    void broadcast(const NATelemetry& data);
    void cleanUp(); // Call periodically to clean up clients

private:
    TelemetryWebSocket();
    AsyncWebSocket _ws;
    uint32_t _lastBroadcast;
};

#endif // TELEMETRY_WEBSOCKET_H
