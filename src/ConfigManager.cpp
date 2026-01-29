#include "ConfigManager.h"

const char *ConfigManager::NAMESPACE = "na_config";

ConfigManager::ConfigManager() {}

bool ConfigManager::begin() {
  bool success = prefs.begin(NAMESPACE, false); // false = read/write mode

  if (success) {
    JsonDocument doc;
    doc["msg"] = "ConfigManager initialized";
    doc["ns"] = NAMESPACE;
    serializeJson(doc, Serial);
    Serial.println();
  } else {
    JsonDocument doc;
    doc["err"] = "ConfigManager init failed";
    serializeJson(doc, Serial);
    Serial.println();
  }

  return success;
}

// ===== PID Configuration =====
ConfigManager::PIDConfig ConfigManager::getPIDConfig() {
  PIDConfig config;
  config.kp = prefs.getFloat("pid_kp", 1.2f);
  config.ki = prefs.getFloat("pid_ki", 0.05f);
  config.kd = prefs.getFloat("pid_kd", 0.4f);
  return config;
}

void ConfigManager::setPIDConfig(const PIDConfig &config) {
  prefs.putFloat("pid_kp", config.kp);
  prefs.putFloat("pid_ki", config.ki);
  prefs.putFloat("pid_kd", config.kd);

  JsonDocument doc;
  doc["msg"] = "PID config saved";
  doc["kp"] = config.kp;
  doc["ki"] = config.ki;
  doc["kd"] = config.kd;
  serializeJson(doc, Serial);
  Serial.println();
}

void ConfigManager::resetPIDConfig() {
  prefs.remove("pid_kp");
  prefs.remove("pid_ki");
  prefs.remove("pid_kd");
  Serial.println("{\"msg\":\"PID config reset to defaults\"}");
}

// ===== Motor Configuration =====
ConfigManager::MotorConfig ConfigManager::getMotorConfig() {
  MotorConfig config;
  config.minPWM = prefs.getUChar("mot_min", 40);
  config.maxRamp = prefs.getUChar("mot_ramp", 5);
  config.deadband = prefs.getUChar("mot_db", 10);
  return config;
}

void ConfigManager::setMotorConfig(const MotorConfig &config) {
  prefs.putUChar("mot_min", config.minPWM);
  prefs.putUChar("mot_ramp", config.maxRamp);
  prefs.putUChar("mot_db", config.deadband);

  JsonDocument doc;
  doc["msg"] = "Motor config saved";
  doc["minPWM"] = config.minPWM;
  doc["maxRamp"] = config.maxRamp;
  doc["deadband"] = config.deadband;
  serializeJson(doc, Serial);
  Serial.println();
}

void ConfigManager::resetMotorConfig() {
  prefs.remove("mot_min");
  prefs.remove("mot_ramp");
  prefs.remove("mot_db");
  Serial.println("{\"msg\":\"Motor config reset to defaults\"}");
}

// ===== Vehicle Pairing =====
String ConfigManager::getPairedMACAddress() {
  return prefs.getString("paired_mac", "");
}

void ConfigManager::setPairedMACAddress(const String &macAddress) {
  prefs.putString("paired_mac", macAddress);

  JsonDocument doc;
  doc["msg"] = "Vehicle pairing saved";
  doc["mac"] = macAddress;
  serializeJson(doc, Serial);
  Serial.println();
}

void ConfigManager::clearPairing() {
  prefs.remove("paired_mac");
  Serial.println("{\"msg\":\"Vehicle pairing cleared\"}");
}

// ===== Joystick Calibration =====
ConfigManager::JoystickCalibration ConfigManager::getJoystickCalibration() {
  JoystickCalibration calib;

  // Throttle
  calib.minThrottle = prefs.getShort("cal_t_min", 0);
  calib.centerThrottle = prefs.getShort("cal_t_ctr", 512);
  calib.maxThrottle = prefs.getShort("cal_t_max", 1023);

  // Roll
  calib.minRoll = prefs.getShort("cal_r_min", 0);
  calib.centerRoll = prefs.getShort("cal_r_ctr", 512);
  calib.maxRoll = prefs.getShort("cal_r_max", 1023);

  // Pitch
  calib.minPitch = prefs.getShort("cal_p_min", 0);
  calib.centerPitch = prefs.getShort("cal_p_ctr", 512);
  calib.maxPitch = prefs.getShort("cal_p_max", 1023);

  // Yaw
  calib.minYaw = prefs.getShort("cal_y_min", 0);
  calib.centerYaw = prefs.getShort("cal_y_ctr", 512);
  calib.maxYaw = prefs.getShort("cal_y_max", 1023);

  return calib;
}

void ConfigManager::setJoystickCalibration(const JoystickCalibration &calib) {
  prefs.putShort("cal_t_min", calib.minThrottle);
  prefs.putShort("cal_t_ctr", calib.centerThrottle);
  prefs.putShort("cal_t_max", calib.maxThrottle);

  prefs.putShort("cal_r_min", calib.minRoll);
  prefs.putShort("cal_r_ctr", calib.centerRoll);
  prefs.putShort("cal_r_max", calib.maxRoll);

  prefs.putShort("cal_p_min", calib.minPitch);
  prefs.putShort("cal_p_ctr", calib.centerPitch);
  prefs.putShort("cal_p_max", calib.maxPitch);

  prefs.putShort("cal_y_min", calib.minYaw);
  prefs.putShort("cal_y_ctr", calib.centerYaw);
  prefs.putShort("cal_y_max", calib.maxYaw);

  Serial.println("{\"msg\":\"Joystick calibration saved\"}");
}

void ConfigManager::resetJoystickCalibration() {
  prefs.remove("cal_t_min");
  prefs.remove("cal_t_ctr");
  prefs.remove("cal_t_max");
  prefs.remove("cal_r_min");
  prefs.remove("cal_r_ctr");
  prefs.remove("cal_r_max");
  prefs.remove("cal_p_min");
  prefs.remove("cal_p_ctr");
  prefs.remove("cal_p_max");
  prefs.remove("cal_y_min");
  prefs.remove("cal_y_ctr");
  prefs.remove("cal_y_max");
  prefs.remove("cal_done");
  Serial.println("{\"msg\":\"Joystick calibration reset to defaults\"}");
}

bool ConfigManager::isJoystickCalibrated() {
  return prefs.getBool("cal_done", false);
}

// ===== Deadzone Configuration =====
ConfigManager::DeadzoneConfig ConfigManager::getDeadzoneConfig() {
  DeadzoneConfig config;
  config.throttle = prefs.getUChar("dz_t", 5);
  config.roll = prefs.getUChar("dz_r", 5);
  config.pitch = prefs.getUChar("dz_p", 5);
  config.yaw = prefs.getUChar("dz_y", 5);
  return config;
}

void ConfigManager::setDeadzoneConfig(const DeadzoneConfig &config) {
  prefs.putUChar("dz_t", config.throttle);
  prefs.putUChar("dz_r", config.roll);
  prefs.putUChar("dz_p", config.pitch);
  prefs.putUChar("dz_y", config.yaw);

  JsonDocument doc;
  doc["msg"] = "Deadzone config saved";
  doc["t"] = config.throttle;
  doc["r"] = config.roll;
  doc["p"] = config.pitch;
  doc["y"] = config.yaw;
  serializeJson(doc, Serial);
  Serial.println();
}

void ConfigManager::resetDeadzoneConfig() {
  prefs.remove("dz_t");
  prefs.remove("dz_r");
  prefs.remove("dz_p");
  prefs.remove("dz_y");
  Serial.println("{\"msg\":\"Deadzone config reset to defaults\"}");
}

// ===== Security Configuration (Phase 9) =====
ConfigManager::SecurityConfig ConfigManager::getSecurityConfig() {
  SecurityConfig config;
  config.encryptionEnabled = prefs.getBool("sec_enc", false);
  prefs.getBytes("sec_key", config.sharedSecret, 32);
  config.hmacEnabled = prefs.getBool("sec_hmac", true);
  config.rateLimitEnabled = prefs.getBool("sec_rl_en", true);
  config.rateLimitCPS = prefs.getUShort("sec_rl_cps", 100);
  return config;
}

void ConfigManager::setSecurityConfig(const SecurityConfig &config) {
  prefs.putBool("sec_enc", config.encryptionEnabled);
  prefs.putBytes("sec_key", config.sharedSecret, 32);
  prefs.putBool("sec_hmac", config.hmacEnabled);
  prefs.putBool("sec_rl_en", config.rateLimitEnabled);
  prefs.putUShort("sec_rl_cps", config.rateLimitCPS);

  JsonDocument doc;
  doc["msg"] = "Security config saved";
  doc["enc"] = config.encryptionEnabled;
  doc["hmac"] = config.hmacEnabled;
  doc["rl"] = config.rateLimitEnabled;
  doc["cps"] = config.rateLimitCPS;
  serializeJson(doc, Serial);
  Serial.println();
}

void ConfigManager::resetSecurityConfig() {
  prefs.remove("sec_enc");
  prefs.remove("sec_key");
  prefs.remove("sec_hmac");
  prefs.remove("sec_rl_en");
  prefs.remove("sec_rl_cps");
  Serial.println("{\"msg\":\"Security config reset to defaults\"}");
}

void ConfigManager::exportToJSON(JsonDocument &doc) {
  PIDConfig pid = getPIDConfig();
  MotorConfig motor = getMotorConfig();
  JoystickCalibration joystick = getJoystickCalibration();
  DeadzoneConfig deadzone = getDeadzoneConfig();
  SecurityConfig security = getSecurityConfig();

  JsonObject pidObj = doc["pid"].to<JsonObject>();
  pidObj["kp"] = pid.kp;
  pidObj["ki"] = pid.ki;
  pidObj["kd"] = pid.kd;

  JsonObject motorObj = doc["motor"].to<JsonObject>();
  motorObj["minPWM"] = motor.minPWM;
  motorObj["maxRamp"] = motor.maxRamp;
  motorObj["deadband"] = motor.deadband;

  JsonObject joyObj = doc["joystick"].to<JsonObject>();
  joyObj["minT"] = joystick.minThrottle;
  joyObj["ctrT"] = joystick.centerThrottle;
  joyObj["maxT"] = joystick.maxThrottle;

  JsonObject dzObj = doc["deadzone"].to<JsonObject>();
  dzObj["t"] = deadzone.throttle;
  dzObj["r"] = deadzone.roll;
  dzObj["p"] = deadzone.pitch;
  dzObj["y"] = deadzone.yaw;

  JsonObject secObj = doc["security"].to<JsonObject>();
  secObj["enc"] = security.encryptionEnabled;
  secObj["hmac"] = security.hmacEnabled;
  secObj["rl"] = security.rateLimitEnabled;
  secObj["cps"] = security.rateLimitCPS;
}

bool ConfigManager::importFromJSON(const JsonDocument &doc) {
  if (!doc["pid"].isNull()) {
    PIDConfig pid;
    pid.kp = doc["pid"]["kp"] | 1.2f;
    pid.ki = doc["pid"]["ki"] | 0.05f;
    pid.kd = doc["pid"]["kd"] | 0.4f;
    setPIDConfig(pid);
  }

  if (!doc["motor"].isNull()) {
    MotorConfig motor;
    motor.minPWM = doc["motor"]["minPWM"] | 40;
    motor.maxRamp = doc["motor"]["maxRamp"] | 5;
    motor.deadband = doc["motor"]["deadband"] | 10;
    setMotorConfig(motor);
  }

  if (!doc["deadzone"].isNull()) {
    DeadzoneConfig deadzone;
    deadzone.throttle = doc["deadzone"]["t"] | 5;
    deadzone.roll = doc["deadzone"]["r"] | 5;
    deadzone.pitch = doc["deadzone"]["p"] | 5;
    deadzone.yaw = doc["deadzone"]["y"] | 5;
    setDeadzoneConfig(deadzone);
  }

  if (!doc["security"].isNull()) {
    SecurityConfig sec;
    sec.encryptionEnabled = doc["security"]["enc"] | false;
    sec.hmacEnabled = doc["security"]["hmac"] | true;
    sec.rateLimitEnabled = doc["security"]["rl"] | true;
    sec.rateLimitCPS = doc["security"]["cps"] | 100;
    setSecurityConfig(sec);
  }

  Serial.println("{\"msg\":\"Configuration imported from JSON\"}");
  return true;
}

void ConfigManager::resetAll() {
  resetPIDConfig();
  resetMotorConfig();
  resetJoystickCalibration();
  resetDeadzoneConfig();
  resetSecurityConfig();
  clearPairing();
  JsonDocument doc;
  doc["msg"] = "ALL configuration reset to defaults";
  serializeJson(doc, Serial);
  Serial.println();
}
