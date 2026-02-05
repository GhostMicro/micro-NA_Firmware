#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

/**
 * ConfigManager - ESP32 NVS (Non-Volatile Storage) configuration management
 *
 * Persistent storage for:
 * - PID tuning values (Kp, Ki, Kd)
 * - Motor configuration (min PWM, max acceleration)
 * - Vehicle pairing (MAC address of paired Land Station)
 * - Joystick calibration (min/center/max per axis)
 * - Deadzone settings (prevent stick drift)
 *
 * Uses Preferences API (NVS) - survives power cycles and resets
 */
class ConfigManager {
public:
  // PID structure
  struct PIDConfig {
    float kp = 1.2f;
    float ki = 0.05f;
    float kd = 0.4f;
  };

  // Motor configuration
  struct MotorConfig {
    uint8_t minPWM = 40;   // Minimum PWM to overcome friction
    uint8_t maxRamp = 5;   // Max % change per 20ms
    uint8_t deadband = 10; // Dead zone in ±100 range
  };

  // Joystick calibration per axis (throttle, roll, pitch, yaw)
  struct JoystickCalibration {
    int16_t minThrottle = 0;
    int16_t centerThrottle = 512;
    int16_t maxThrottle = 1023;

    int16_t minRoll = 0;
    int16_t centerRoll = 512;
    int16_t maxRoll = 1023;

    int16_t minPitch = 0;
    int16_t centerPitch = 512;
    int16_t maxPitch = 1023;

    int16_t minYaw = 0;
    int16_t centerYaw = 512;
    int16_t maxYaw = 1023;
  };

  // Deadzone configuration (% of range)
  struct DeadzoneConfig {
    uint8_t throttle = 5; // 5% deadzone
    uint8_t roll = 5;
    uint8_t pitch = 5;
    uint8_t yaw = 5;
  };

  // Phase 9: Security configuration
  struct SecurityConfig {
    bool encryptionEnabled = false;
    uint8_t sharedSecret[32] = {0};
    bool hmacEnabled = true;
    bool rateLimitEnabled = true;
    uint16_t rateLimitCPS = 100;
  };

  ConfigManager();

  /**
   * Initialize NVS storage
   * @return true if initialization successful
   */
  bool begin();

  // ===== PID Configuration =====
  PIDConfig getPIDConfig();
  void setPIDConfig(const PIDConfig &config);
  void resetPIDConfig();

  // ===== Motor Configuration =====
  MotorConfig getMotorConfig();
  void setMotorConfig(const MotorConfig &config);
  void resetMotorConfig();

  // ===== Vehicle Pairing =====
  String getPairedMACAddress();
  void setPairedMACAddress(const String &macAddress);
  void clearPairing();

  // ===== Joystick Calibration =====
  JoystickCalibration getJoystickCalibration();
  void setJoystickCalibration(const JoystickCalibration &calib);
  void resetJoystickCalibration();
  bool isJoystickCalibrated();

  // ===== Deadzone Configuration =====
  DeadzoneConfig getDeadzoneConfig();
  void setDeadzoneConfig(const DeadzoneConfig &config);
  void resetDeadzoneConfig();

  // ===== Security Configuration (Phase 9) =====
  SecurityConfig getSecurityConfig();
  void setSecurityConfig(const SecurityConfig &config);
  void resetSecurityConfig();

  /**
   * Export all configuration to JSON
   */
  void exportToJSON(JsonDocument &doc);

  /**
   * Import configuration from JSON
   */
  bool importFromJSON(const JsonDocument &doc);

  /**
   * Reset all configuration to defaults
   */
  void resetAll();

  // ===== Generic Static Accessors (Standard Requirement) =====
  static int getInt(const char *key, int defaultValue = 0);
  static void setInt(const char *key, int value);
  static float getFloat(const char *key, float defaultValue = 0.0f);
  static void setFloat(const char *key, float value);

private:
  Preferences prefs;
  static const char *NAMESPACE;
};

#endif // CONFIG_MANAGER_H
