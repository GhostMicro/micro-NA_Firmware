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

// ESP-NOW Callback
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(NAPacket)) {
    NAPacket pkt;
    memcpy(&pkt, incomingData, sizeof(NAPacket));

    bool valid = true;

    // Phase 9: Decryption & Validation
    if (pkt.encryptionFlag == 1) {
      if (!EncryptionManager_isReady()) {
        valid = false;
      } else {
        // Decrypt in place (CTR mode)
        uint8_t plaintext[sizeof(NAPacket)];
        // Length to decrypt: throttle to buttons (throttle is at offset 3)
        uint16_t payloadLen =
            10; // throttle(2)+roll(2)+pitch(2)+yaw(2)+mode(1)+buttons(1)
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
    serializeJson(res, Serial);
    Serial.println();
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

  if (vehicle)
    vehicle->loop();

  MemoryProfiler_recordTaskTime("main_loop", micros() - (currentTime * 1000));

  if (currentTime - lastTelemetryTime > 1000) {
    lastTelemetryTime = currentTime;
    telemetry.protocolVersion = PROTOCOL_VERSION;
    telemetry.uptime = currentTime;
    if (batteryManager)
      telemetry.batteryVoltage =
          batteryManager->getVoltageMillivolts() / 1000.0f;

    ConfigManager::SecurityConfig sec = configManager->getSecurityConfig();
    if (sec.encryptionEnabled) {
      telemetry.encryptionFlag = 1;
      EncryptionManager_generateIV(telemetry.iv);
      // Encrypt batteryVoltage, rssi, uptime, status (relative offset 2, len
      // 11)
      EncryptionManager_encrypt((uint8_t *)&telemetry.batteryVoltage, 11,
                                telemetry.iv,
                                (uint8_t *)&telemetry.batteryVoltage);
      HMACValidator_generate((uint8_t *)&telemetry.batteryVoltage, 11,
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
  }

  uint32_t loopElapsed = millis() - currentTime;
  uint32_t delayTime = (loopElapsed < 20) ? (20 - loopElapsed) : 0;
  delay(delayTime);
}
