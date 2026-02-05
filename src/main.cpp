#include "BatteryManager.h"
#include "ConfigManager.h"
#include "EncryptionManager.h"
#include "FailsafeManager.h"
#include "HAL.h"
#include "HMACValidator.h"
#include "JoystickCalibrator.h"
#include "MemoryProfiler.h"
#include "OTAUpdater.h"
#include "RSSIManager.h"
#include "RateLimitManager.h"
#include "KeyExchangeManager.h"
#include "NavigationManager.h"
#include "WaypointManager.h"
#include "TelemetryWebSocket.h"
#include "DepthManager.h"
#include <ESPAsyncWebServer.h>
#include "vehicles/Copter.h"
#include "vehicles/Plane.h"
#include "vehicles/Rover.h"
#include "vehicles/Sub.h"
#include "vehicles/Vehicle.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_now.h>
#include <mbedtls/base64.h>

// Helper macro for safe object allocation
#define SAFE_NEW(ptr, type, ...)                                               \
  {                                                                            \
    ptr = new type(__VA_ARGS__);                                               \
  }

// Global managers and objects
FailsafeManager failsafeManager;
ConfigManager *configManager = nullptr;
BatteryManager *batteryManager = nullptr;
RSSIManager *rssiManager = nullptr;
JoystickCalibrator *joystickCalibrator = nullptr;
Vehicle *vehicle = nullptr;

// Protocol state
NAPacket latestPacket;
uint32_t packetSequence = 0;
uint8_t encryptionKey[32] = {0}; // Phase 9: Pre-shared key (PSK)
uint8_t hmacSecret[32] = {0};    // Phase 9: HMAC secret
uint32_t lastRateLimitRefill = 0;
const uint32_t REFILL_INTERVAL_MS = 10;

// Phase 10: GPS Serial
HardwareSerial GPSSerial(2); // UART2

// Phase 11: Web Server
AsyncWebServer server(80);

// ESP-NOW Callback
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  // Phase 10: Handshake Packet Handling
  if (len == sizeof(NAHandshakePacket)) {
    NAHandshakePacket* hpkt = (NAHandshakePacket*)incomingData;
    if (hpkt->protocolVersion == PROTOCOL_VERSION) {
       if (hpkt->type == PACKET_TYPE_HANDSHAKE_INIT) {
          // 2-Way Handshake:
          // Received INIT with Peer's Public Key.
          // 1. Generate Our Pair
          // 2. Compute Secret (using Peer's Key)
          // 3. Respond with Our Public Key
          Serial.println("[KX] Handshake Init (with Key) Received");
          
          KeyExchangeManager& kx = KeyExchangeManager::getInstance();
          kx.reset();
          
          if (kx.generateKeyPair()) {
              // Compute Secret immediately using the key from the packet
              if (kx.computeSharedSecret(hpkt->publicKey)) {
                  // Success! Get the secret
                  uint8_t secret[32];
                  kx.getSharedSecret(secret);
                  
                  // Apply keys
                  EncryptionManager_init(secret);
                  HMACValidator_init(secret);
                  
                  // Send Response with Our Public Key
                  NAHandshakePacket resp;
                  resp.protocolVersion = PROTOCOL_VERSION;
                  resp.type = PACKET_TYPE_HANDSHAKE_PUBKEY;
                  kx.getPublicKey(resp.publicKey);
                  resp.checksum = NA_CRC16((uint8_t*)&resp, sizeof(NAHandshakePacket) - 2);
                  
                  esp_now_send(mac, (uint8_t*)&resp, sizeof(resp));
                  Serial.println("[KX] 2-Way Handshake Complete! Secure Link Established.");
              } else {
                  Serial.println("[KX] Compute Secret Failed");
              }
          } else {
              Serial.println("[KX] Key Gen Failed");
          }
       } else if (hpkt->type == PACKET_TYPE_HANDSHAKE_PUBKEY) {
          // Received Peer Public Key -> Compute Secret
          Serial.println("[KX] Peer Public Key Received");
          if (KeyExchangeManager::getInstance().computeSharedSecret(hpkt->publicKey)) {
              uint8_t secret[32];
              KeyExchangeManager::getInstance().getSharedSecret(secret);
              
              // Apply new keys to security managers
              EncryptionManager_init(secret);
              HMACValidator_init(secret);
              
              Serial.println("[KX] Key Exchange Success! Secure Link Established.");
          } else {
             Serial.println("[KX] Key Computation Failed");
          }
       }
    }
  }
  else if (len == sizeof(NAPacket)) {
    NAPacket pkt;
    memcpy(&pkt, incomingData, sizeof(NAPacket));

    // Phase 9 Security Hardening: Apply Rate Limit FIRST to prevent CPU exhaustion
    if (RateLimitManager_checkCommand(pkt.mode) != RATE_LIMIT_ALLOWED) {
      failsafeManager.recordPacketReceived(millis(), false);
      return; 
    }

    bool valid = true;
    ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();

    // Enforce Encryption if enabled in config or if session is established
    bool requireEncryption = sec.encryptionEnabled || EncryptionManager_isReady();

    if (pkt.encryptionFlag == 1) {
      if (!EncryptionManager_isReady()) {
        valid = false;
      } else {
        // Decrypt in place (CTR mode)
        uint16_t payloadLen = 10; // throttle to buttons
        if (EncryptionManager_decrypt((uint8_t *)&pkt.throttle, payloadLen,
                                      pkt.iv, (uint8_t *)&pkt.throttle)) {
          // Validate HMAC
          if (!HMACValidator_validate((uint8_t *)&pkt.throttle, payloadLen,
                                      pkt.hmac)) {
            valid = false;
          }
        } else {
          valid = false;
        }
      }
    } else if (requireEncryption) {
      // REJECT unencrypted packets if system is in secure mode
      valid = false;
    }

    if (valid && NA_PACKET_IS_VALID(&pkt)) {
      memcpy(&latestPacket, &pkt, sizeof(NAPacket));
      failsafeManager.recordPacketReceived(millis(), true);
      if (rssiManager)
        rssiManager->updateRSSI(-60);
      if (vehicle)
        vehicle->setInputs(&latestPacket);
    } else {
      failsafeManager.recordPacketReceived(millis(), false);
    }
  }
}

/**
 * Handle incoming serial commands (JSON via Web Configurator)
 */
void handleSerialCommand() {
  if (!Serial.available())
    return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, cmd);

  if (error) {
    JsonDocument errDoc;
    errDoc["err"] = "JSON parse failed";
    serializeJson(errDoc, Serial);
    Serial.println();
    return;
  }

  const char *command = doc["c"];
  if (!command)
    return;

  // Rate Limiting
  if (RateLimitManager_checkCommand(1) != RATE_LIMIT_ALLOWED) {
    JsonDocument errDoc;
    errDoc["err"] = "Rate limit exceeded";
    serializeJson(errDoc, Serial);
    Serial.println();
    return;
  }

  // HMAC Validation for JSON
  bool hmacValid = true;
  if (!doc["hmac"].isNull()) {
    uint8_t receivedHmac[32];
    size_t decodedLen = 0;
    const char *hmacStr = doc["hmac"];
    int ret =
        mbedtls_base64_decode(receivedHmac, sizeof(receivedHmac), &decodedLen,
                              (const unsigned char *)hmacStr, strlen(hmacStr));

    if (ret != 0 || decodedLen != 32) {
      hmacValid = false;
    } else {
      JsonDocument valDoc;
      valDoc.set(doc);
      valDoc.remove("hmac");
      String cmdToValidate;
      serializeJson(valDoc, cmdToValidate);

      if (!HMACValidator_validate((uint8_t *)cmdToValidate.c_str(),
                                  cmdToValidate.length(), receivedHmac)) {
        hmacValid = false;
      }
    }

    if (!hmacValid) {
      JsonDocument errDoc;
      errDoc["err"] = "HMAC validation failed";
      serializeJson(errDoc, Serial);
      Serial.println();
      return;
    }
  }

  failsafeManager.recordPacketReceived(millis(), hmacValid);

  // Router
  if (strcmp(command, "sm") == 0) {
    if (!doc["t"].isNull())
      latestPacket.throttle = doc["t"];
    if (!doc["s"].isNull())
      latestPacket.roll = doc["s"];
    if (!doc["p"].isNull())
      latestPacket.pitch = doc["p"];
    if (!doc["y"].isNull())
      latestPacket.yaw = doc["y"];
    latestPacket.protocolVersion = PROTOCOL_VERSION;
    latestPacket.encryptionFlag = 0;
    latestPacket.sequenceNumber = ++packetSequence;
    NA_UPDATE_PACKET_CHECKSUM(&latestPacket);
    if (vehicle)
      vehicle->setInputs(&latestPacket);
    Serial.println("{\"ok\":true}");

  } else if (strcmp(command, "ping") == 0) {
    JsonDocument pongDoc;
    pongDoc["ok"] = true;
    pongDoc["uptime"] = millis();
    RateLimitStats stats = RateLimitManager_getStats();
    pongDoc["rl_allowed"] = stats.totalCommandsAllowed;
    pongDoc["rl_blocked"] = stats.totalCommandsBlocked;
    serializeJson(pongDoc, Serial);
    Serial.println();

  } else if (strcmp(command, "get_security_config") == 0) {
    if (configManager) {
      ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();
      JsonDocument res;
      res["c"] = "get_security_config";
      res["ok"] = true;
      res["encryption_enabled"] = sec.encryptionEnabled;
      res["hmac_enabled"] = sec.hmacEnabled;
      res["rate_limit_enabled"] = sec.rateLimitEnabled;
      res["rate_limit_cps"] = sec.rateLimitCPS;
      serializeJson(res, Serial);
      Serial.println();
    }
  } else if (strcmp(command, "set_security_config") == 0) {
    if (configManager) {
      ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();
      if (!doc["encryption_enabled"].isNull())
        sec.encryptionEnabled = doc["encryption_enabled"];
      if (!doc["hmac_enabled"].isNull())
        sec.hmacEnabled = doc["hmac_enabled"];
      if (!doc["rate_limit_enabled"].isNull())
        sec.rateLimitEnabled = doc["rate_limit_enabled"];
      if (!doc["rate_limit_cps"].isNull())
        sec.rateLimitCPS = doc["rate_limit_cps"];
      if (!doc["shared_secret"].isNull()) {
        const char *secret = doc["shared_secret"];
        memset(sec.sharedSecret, 0, 32);
        strncpy((char *)sec.sharedSecret, secret, 31);
      }
      configManager->setSecurityConfig(sec);
      EncryptionManager_init(sec.sharedSecret);
      HMACValidator_init(sec.sharedSecret);
      RateLimitManager_init(sec.rateLimitCPS);
      Serial.println("{\"ok\":true}");
    }
  } else if (strcmp(command, "start_ota_update") == 0) {
    const char *url = doc["url"];
    if (url) {
      bool ok = OTAUpdater_startDownload(url, NULL);
      JsonDocument res;
      res["ok"] = ok;
      if (!ok)
        res["msg"] = OTAUpdater_getErrorMessage();
      serializeJson(res, Serial);
      Serial.println();
    }
  } else if (strcmp(command, "get_ota_progress") == 0) {
    JsonDocument res;
    res["status"] = (int)OTAUpdater_getStatus();
    res["progress"] = OTAUpdater_getProgress();
    res["bytes"] = OTAUpdater_getBytesDownloaded();
    res["total"] = OTAUpdater_getTotalSize();
    res["total"] = OTAUpdater_getTotalSize();
    serializeJson(res, Serial);
    Serial.println();
  } else if (strcmp(command, "upload_wp") == 0) {
    if (!doc["lat"].isNull() && !doc["lng"].isNull()) {
         uint16_t speed = doc["speed"] | 1500;
         WaypointManager::getInstance().addWaypoint(doc["lat"], doc["lng"], doc["alt"] | 0, speed);
         Serial.println("{\"ok\":true, \"msg\":\"WP Added\"}");
    }
  } else if (strcmp(command, "start_mission") == 0) {
      if (WaypointManager::getInstance().getWaypointCount() > 0) {
          NavigationManager::getInstance().startMission();
          Serial.println("{\"ok\":true, \"msg\":\"Mission Started\"}");
      } else {
          Serial.println("{\"ok\":false, \"msg\":\"No Waypoints\"}");
      }
  } else if (strcmp(command, "stop_mission") == 0) {
      NavigationManager::getInstance().stopMission();
      Serial.println("{\"ok\":true}");
  } else if (strcmp(command, "clear_mission") == 0) {
      WaypointManager::getInstance().clearMission();
      NavigationManager::getInstance().stopMission();
      Serial.println("{\"ok\":true}");
  } else if (strcmp(command, "rtl") == 0) {
      NavigationManager::getInstance().executeRTL();
      Serial.println("{\"ok\":true}");
  } else if (strcmp(command, "set_depth") == 0) {
      if (!doc["d"].isNull()) {
          DepthManager::getInstance().setTargetDepth(doc["d"]);
          DepthManager::getInstance().setDiving(true);
          Serial.println("{\"ok\":true}");
      }
  } else if (strcmp(command, "kx_init") == 0) {
      // Phase 2: Secure Serial Handshake (Step 1: Generate & Send PubKey)
      KeyExchangeManager& kx = KeyExchangeManager::getInstance();
      kx.reset();
      if (kx.generateKeyPair()) {
          uint8_t pubKey[64];
          kx.getPublicKey(pubKey);
          
          char b64PubKey[128];
          size_t b64Len = 0;
          mbedtls_base64_encode((unsigned char*)b64PubKey, sizeof(b64PubKey), &b64Len, pubKey, 64);
          
          JsonDocument res;
          res["c"] = "kx_init";
          res["ok"] = true;
          res["pub"] = b64PubKey;
          serializeJson(res, Serial);
          Serial.println();
      } else {
          Serial.println("{\"ok\":false, \"err\":\"KeyGen Failed\"}");
      }
  } else if (strcmp(command, "kx_fin") == 0) {
      // Phase 2: Secure Serial Handshake (Step 2: Receive Peer Key & Compute Secret)
      const char* peerKeyB64 = doc["pub"];
      if (peerKeyB64) {
          uint8_t peerPubKey[64];
          size_t decodedLen = 0;
          int ret = mbedtls_base64_decode(peerPubKey, sizeof(peerPubKey), &decodedLen, (const unsigned char*)peerKeyB64, strlen(peerKeyB64));
          
          if (ret == 0 && decodedLen == 64) {
              if (KeyExchangeManager::getInstance().computeSharedSecret(peerPubKey)) {
                  uint8_t secret[32];
                  KeyExchangeManager::getInstance().getSharedSecret(secret);
                  
                  EncryptionManager_init(secret);
                  HMACValidator_init(secret);
                  
                  Serial.println("{\"ok\":true, \"msg\":\"KX Complete\"}");
              } else {
                  Serial.println("{\"ok\":false, \"err\":\"Compute Failed\"}");
              }
          } else {
              Serial.println("{\"ok\":false, \"err\":\"B64 Decode Failed\"}");
          }
      }
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  failsafeManager.setup();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK)
    return;
  esp_now_register_recv_cb(OnDataRecv);

  SAFE_NEW(configManager, ConfigManager);
  if (configManager) {
    configManager->begin();
    ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();
    EncryptionManager_init(sec.sharedSecret);
    HMACValidator_init(sec.sharedSecret);
    RateLimitManager_init(sec.rateLimitCPS);
  }
  
  // Phase 10: Init Key Exchange
  if (!KeyExchangeManager::getInstance().init()) {
      Serial.println("KeyExchange Init Failed");
  }
  
  // Phase 10: Init Navigation and GPS
  GPSSerial.begin(9600, SERIAL_8N1, 16, 17); // RX=16, TX=17
  NavigationManager::getInstance().init();
  DepthManager::getInstance().begin();
  
  // Phase 11: Init WebSockets
  TelemetryWebSocket::getInstance().begin(&server);
  server.begin(); // Start Web Server

  SAFE_NEW(batteryManager, BatteryManager);
  if (batteryManager)
    batteryManager->setup();
  SAFE_NEW(rssiManager, RSSIManager);
  SAFE_NEW(joystickCalibrator, JoystickCalibrator, configManager);

  OTAUpdater_init();
  MemoryProfiler_init();

  // Create vehicle instance
#if defined(VEHICLE_TYPE_ROVER)
  SAFE_NEW(vehicle, Rover);
#elif defined(VEHICLE_TYPE_PLANE)
  SAFE_NEW(vehicle, Plane);
#elif defined(VEHICLE_TYPE_SUB)
  SAFE_NEW(vehicle, Sub);
#else
  SAFE_NEW(vehicle, Copter);
#endif

  if (vehicle)
    vehicle->setup();
}

uint32_t lastTelemetryTime = 0;
NATelemetry telemetry;

void loop() {
  uint32_t currentTime = millis();
  failsafeManager.update(currentTime);
  handleSerialCommand();

  if (currentTime - lastRateLimitRefill >= REFILL_INTERVAL_MS) {
    RateLimitManager_refill();
    lastRateLimitRefill = currentTime;
  }
  
  // Phase 10: Autonomous Navigation Logic
  // 1. Read GPS
  while (GPSSerial.available() > 0) {
      NavigationManager::getInstance().feedGPS(GPSSerial.read());
  }
  
  // 2. Update Nav Manager (Using GPS Course as Heading for now)
  // TODO: Use Compass if available
  float lat = 0, lng = 0;
  NavigationManager::getInstance().getGPSLocation(lat, lng);
  // Note: TinyGPS course is 0-360, need to check if we need conversion. 
  // NavManager uses it for diff.
  // We need to pass current GPS course if moving.
  // Using GPS Course as heading (valid if moving > 1-2 m/s normally)
  float currentHeading = NavigationManager::getInstance().getGPSCourse();
  NavigationManager::getInstance().update(lat, lng, currentHeading);

  // 3. Apply Auto Inputs if Mode is Auto
  if (latestPacket.mode & MODE_AUTO) {
      int16_t navThrottle = 0;
      int16_t navYaw = 0;
      if (NavigationManager::getInstance().getNavigationOutput(navThrottle, navYaw)) {
          latestPacket.throttle = navThrottle;
          latestPacket.roll = navYaw; // Use Roll channel for Steering
      } else if (NavigationManager::getInstance().getState().isRTLActive) {
          // RTL Reached Home
          NavigationManager::getInstance().stopMission();
          latestPacket.throttle = 0;
          latestPacket.roll = 0;
          Serial.println("[Nav] RTL Mission Complete: Reached Home.");
      }
  }
  
  // Phase 14: RTL Triggers (Battery & Failsafe)
  if (batteryManager && batteryManager->getVoltageMillivolts() < 3400) { // RTL_VOLTAGE_MV
      if (!NavigationManager::getInstance().getState().isRTLActive) {
          Serial.println("[Battery] Low voltage! Triggering RTL.");
          NavigationManager::getInstance().executeRTL();
      }
  }

  // Set Home on first valid GPS fix
  static bool homeSet = false;
  if (!homeSet && NavigationManager::getInstance().isGPSLocked()) {
      float hLat, hLng;
      NavigationManager::getInstance().getGPSLocation(hLat, hLng);
      NavigationManager::getInstance().setHome(hLat, hLng);
      homeSet = true;
  }
  
  // Phase 13: Sub-Surface Logic
  DepthManager::getInstance().update();
  
  // Apply Depth Hold if bit set (reusing a spare bit in mode for now or status)
  // For now, let's just update the manager

  if (vehicle)
    vehicle->loop();

  MemoryProfiler_recordTaskTime("main_loop", micros() - (currentTime * 1000));

  if (currentTime - lastTelemetryTime >= 50) { // 20Hz Telemetry
    lastTelemetryTime = currentTime;
    telemetry.protocolVersion = PROTOCOL_VERSION;
    telemetry.uptime = currentTime;
    if (batteryManager)
      telemetry.batteryVoltage =
          batteryManager->getVoltageMillivolts() / 1000.0f;
    
    // Populate GPS if available
    if (NavigationManager::getInstance().isGPSLocked()) {
        float lat, lng;
        NavigationManager::getInstance().getGPSLocation(lat, lng);
        telemetry.latitude = lat;
        telemetry.longitude = lng;
        telemetry.status |= 0x02; // Set gps_lock bit
    } else {
        telemetry.latitude = 0;
        telemetry.longitude = 0;
        telemetry.status &= ~0x02;
    }

    ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();
    if (sec.encryptionEnabled) {
      telemetry.encryptionFlag = 1;
      EncryptionManager_generateIV(telemetry.iv);
      // Encrypt batteryVoltage, rssi, uptime, lat, lng, status (relative offset 2, len 19)
      // battery(4)+rssi(2)+uptime(4)+lat(4)+lng(4)+status(1) = 19 bytes
      EncryptionManager_encrypt((uint8_t *)&telemetry.batteryVoltage, 19,
                                telemetry.iv,
                                (uint8_t *)&telemetry.batteryVoltage);
      HMACValidator_generate((uint8_t *)&telemetry.batteryVoltage, 19,
                             telemetry.hmac);
    } else {
      telemetry.encryptionFlag = 0;
    }

    NA_UPDATE_TELEMETRY_CHECKSUM(&telemetry);

    JsonDocument telDoc;
    telDoc["t"] = 2;
    telDoc["v"] = batteryManager
                      ? batteryManager->getVoltageMillivolts() / 1000.0f
                      : 0.0f;
    if (rssiManager)
      telDoc["r"] = rssiManager->getRSSIPercentage();
    MemoryStats memStats = MemoryProfiler_getMemoryStats();
    telDoc["heap"] = (int)memStats.memoryUtilization;

    serializeJson(telDoc, Serial);
    Serial.println();

    uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    esp_now_send(broadcastAddress, (uint8_t *)&telemetry, sizeof(telemetry));
    
    // Phase 11: WebSocket Broadcast
    TelemetryWebSocket::getInstance().broadcast(telemetry);
  }
  
  // Clean up WS clients periodically
  TelemetryWebSocket::getInstance().cleanUp();

  uint32_t loopElapsed = millis() - currentTime;
  uint32_t delayTime = (loopElapsed < 20) ? (20 - loopElapsed) : 0;
  delay(delayTime);
}
