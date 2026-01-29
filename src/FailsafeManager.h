#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include <Arduino.h>
#include <stdint.h>

/**
 * Failsafe State Machine
 *
 * Manages vehicle safety state and timeout-based failsafe transitions:
 * - IDLE: Initial state, no packets received
 * - ARMED: Actively receiving valid commands (< 500ms timeout)
 * - SIGNAL_LOSS: Warning state, signal interrupted (500-2000ms)
 * - EMERGENCY: Critical failsafe triggered (> 2000ms), motors disabled
 */
enum FailsafeState {
  FAILSAFE_IDLE = 0,
  FAILSAFE_ARMED = 1,
  FAILSAFE_SIGNAL_LOSS = 2,
  FAILSAFE_EMERGENCY = 3
};

class FailsafeManager {
public:
  FailsafeManager();

  /**
   * Initialize failsafe manager (call in setup())
   */
  void setup();
  void init() { setup(); }

  /**
   * Record that a valid packet was received
   * @param timestamp Optional custom timestamp (millis)
   * @param hmacValid Optional flag if HMAC was valid (Phase 9)
   */
  void recordPacketReceived(uint32_t timestamp = 0, bool hmacValid = true);

  /**
   * Update failsafe state machine
   * @param currentTime Current system time (millis)
   */
  void update(uint32_t currentTime = 0);

  /**
   * Get current failsafe state
   */
  FailsafeState getState() const { return currentState; }

  /**
   * Check if vehicle is currently armed
   */
  bool isArmed() const { return currentState == FAILSAFE_ARMED; }

  /**
   * Check if failsafe is actively triggered
   */
  bool isFailsafeActive() const { return currentState == FAILSAFE_EMERGENCY; }

  /**
   * Check if signal is lost (warning state)
   */
  bool isSignalLost() const { return currentState == FAILSAFE_SIGNAL_LOSS; }

  /**
   * Get time since last valid packet (milliseconds)
   */
  uint32_t getTimeSinceLastPacket(uint32_t currentTime = 0) const;

  /**
   * Get human-readable state name
   */
  const char *getStateString() const;

private:
  FailsafeState currentState;
  FailsafeState previousState;
  uint32_t lastPacketTime;
  uint32_t stateChangeTime;
  uint32_t ledBlinkTime;
  uint32_t totalPackets;
  uint32_t invalidHmacPackets;

  static const uint32_t SIGNAL_LOSS_THRESHOLD = 500; // 500ms
  static const uint32_t FAILSAFE_THRESHOLD = 2000;   // 2000ms
  static const uint8_t STATUS_LED_PIN = 2;           // GPIO 2

  /**
   * Update LED indicator based on current state
   */
  void updateStatusLED(uint32_t currentTime);

  /**
   * Log state transition to serial
   */
  void logStateTransition(uint32_t currentTime);
};

#endif // FAILSAFE_MANAGER_H
