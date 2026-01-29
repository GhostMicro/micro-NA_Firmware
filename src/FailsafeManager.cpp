#include "FailsafeManager.h"
#include <ArduinoJson.h>

FailsafeManager::FailsafeManager()
    : currentState(FAILSAFE_IDLE), previousState(FAILSAFE_IDLE),
      lastPacketTime(0), stateChangeTime(0), ledBlinkTime(0), totalPackets(0),
      invalidHmacPackets(0) {}

void FailsafeManager::setup() {
  pinMode(STATUS_LED_PIN, OUTPUT);
  digitalWrite(STATUS_LED_PIN, LOW);
  lastPacketTime = millis();
  stateChangeTime = millis();
}

void FailsafeManager::recordPacketReceived(uint32_t timestamp, bool hmacValid) {
  if (timestamp == 0)
    timestamp = millis();

  totalPackets++;
  if (!hmacValid) {
    invalidHmacPackets++;
    // If HMAC is invalid, we don't update lastPacketTime for ARMING
    // but we might update it to prevent IMMEDIATE emergency if we want?
    // Usually, invalid HMAC should be treated as NO PACKET.
    return;
  }

  lastPacketTime = timestamp;
}

void FailsafeManager::update(uint32_t currentTime) {
  if (currentTime == 0)
    currentTime = millis();

  uint32_t timeSincePacket = currentTime - lastPacketTime;
  FailsafeState newState = currentState;

  // State machine transitions
  if (timeSincePacket < SIGNAL_LOSS_THRESHOLD) {
    newState = FAILSAFE_ARMED;
  } else if (timeSincePacket < FAILSAFE_THRESHOLD) {
    newState = FAILSAFE_SIGNAL_LOSS;
  } else {
    newState = FAILSAFE_EMERGENCY;
  }

  // Log state changes
  if (newState != currentState) {
    previousState = currentState;
    currentState = newState;
    stateChangeTime = currentTime;
    logStateTransition(currentTime);
  }

  // Update LED indicator
  updateStatusLED(currentTime);
}

uint32_t FailsafeManager::getTimeSinceLastPacket(uint32_t currentTime) const {
  if (currentTime == 0)
    currentTime = millis();
  return currentTime - lastPacketTime;
}

const char *FailsafeManager::getStateString() const {
  switch (currentState) {
  case FAILSAFE_IDLE:
    return "IDLE";
  case FAILSAFE_ARMED:
    return "ARMED";
  case FAILSAFE_SIGNAL_LOSS:
    return "SIGNAL_LOSS";
  case FAILSAFE_EMERGENCY:
    return "EMERGENCY";
  default:
    return "UNKNOWN";
  }
}

void FailsafeManager::updateStatusLED(uint32_t currentMillis) {
  bool ledState = LOW;

  switch (currentState) {
  case FAILSAFE_IDLE:
    ledState = LOW;
    break;
  case FAILSAFE_ARMED:
    ledState = ((currentMillis / 1000) % 2) == 0 ? HIGH : LOW;
    break;
  case FAILSAFE_SIGNAL_LOSS:
    ledState = ((currentMillis / 200) % 2) == 0 ? HIGH : LOW;
    break;
  case FAILSAFE_EMERGENCY:
    ledState = ((currentMillis / 100) % 2) == 0 ? HIGH : LOW;
    break;
  }

  digitalWrite(STATUS_LED_PIN, ledState);
}

void FailsafeManager::logStateTransition(uint32_t currentTime) {
  JsonDocument doc;
  doc["t"] = 1;
  doc["msg"] = "Failsafe state change";
  doc["from"] = (int)previousState;
  doc["to"] = (int)currentState;
  doc["state"] = getStateString();
  doc["timeSince"] = currentTime - lastPacketTime;
  doc["badHmac"] = invalidHmacPackets;

  serializeJson(doc, Serial);
  Serial.println();
}
