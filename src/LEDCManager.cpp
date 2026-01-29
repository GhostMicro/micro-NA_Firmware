#include "LEDCManager.h"
#include "driver/ledc.h"
#include <ArduinoJson.h>

bool LEDCManager::allocateMotorChannel(uint8_t pin, LEDCChannel channel) {
  if (channel > CHANNEL_MOTOR_5) {
    Serial.println("[LEDC] Error: Invalid motor channel");
    return false;
  }

  if (allocatedChannels & (1 << channel)) {
    Serial.println("[LEDC] Error: Channel already allocated");
    return false;
  }

  channelPins[channel] = pin;
  isMotorChannel[channel] = true;
  configureChannel(channel, pin, MOTOR_FREQUENCY, true);

  allocatedChannels |= (1 << channel);

  JsonDocument doc;
  doc["msg"] = "Motor LEDC channel allocated";
  doc["ch"] = (int)channel;
  doc["pin"] = pin;
  serializeJson(doc, Serial);
  Serial.println();

  return true;
}

bool LEDCManager::allocateServoChannel(uint8_t pin, LEDCChannel channel) {
  if (channel < CHANNEL_SERVO_0 || channel > CHANNEL_SERVO_1) {
    Serial.println("[LEDC] Error: Invalid servo channel");
    return false;
  }

  if (allocatedChannels & (1 << channel)) {
    Serial.println("[LEDC] Error: Channel already allocated");
    return false;
  }

  channelPins[channel] = pin;
  isMotorChannel[channel] = false;
  configureChannel(channel, pin, SERVO_FREQUENCY, false);

  allocatedChannels |= (1 << channel);

  JsonDocument doc;
  doc["msg"] = "Servo LEDC channel allocated";
  doc["ch"] = (int)channel;
  doc["pin"] = pin;
  serializeJson(doc, Serial);
  Serial.println();

  return true;
}

void LEDCManager::configureChannel(LEDCChannel channel, uint8_t pin,
                                   uint16_t frequency, bool isMotor) {
  ledc_timer_config_t timerConfig = {
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .duty_resolution =
          (ledc_timer_bit_t)(isMotor ? MOTOR_RESOLUTION : SERVO_RESOLUTION),
      .timer_num = (ledc_timer_t)(channel / 2), // 2 channels per timer
      .freq_hz = frequency,
      .clk_cfg = LEDC_AUTO_CLK,
  };
  ledc_timer_config(&timerConfig);

  ledc_channel_config_t channelConfig = {
      .gpio_num = pin,
      .speed_mode = LEDC_LOW_SPEED_MODE,
      .channel = (ledc_channel_t)(channel % 8),
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = (ledc_timer_t)(channel / 2),
      .duty = 0,
      .hpoint = 0,
      .flags = {.output_invert = 0},
  };
  ledc_channel_config(&channelConfig);

  channelValues[channel] = 0;
}

void LEDCManager::setPWM(LEDCChannel channel, uint16_t value) {
  if (!(allocatedChannels & (1 << channel))) {
    Serial.println("[LEDC] Error: Channel not allocated");
    return;
  }

  channelValues[channel] = value;

  if (isMotorChannel[channel]) {
    // Motor: 8-bit (0-255)
    value = constrain(value, 0, 255);
  } else {
    // Servo: 8-bit (0-255 maps to 0-180°)
    value = constrain(value, 0, 255);
  }

  ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, value);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
}

uint16_t LEDCManager::getPWM(LEDCChannel channel) {
  if (!(allocatedChannels & (1 << channel))) {
    return 0;
  }
  return channelValues[channel];
}

bool LEDCManager::isAllocated(LEDCChannel channel) const {
  return allocatedChannels & (1 << channel);
}

void LEDCManager::releaseChannel(LEDCChannel channel) {
  if (!(allocatedChannels & (1 << channel))) {
    return;
  }

  ledc_stop(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, 0);
  allocatedChannels &= ~(1 << channel);
  channelValues[channel] = 0;

  JsonDocument doc;
  doc["msg"] = "LEDC channel released";
  doc["ch"] = (int)channel;
  serializeJson(doc, Serial);
  Serial.println();
}

void LEDCManager::releaseAll() {
  for (int i = 0; i < TOTAL_CHANNELS; i++) {
    if (allocatedChannels & (1 << i)) {
      releaseChannel((LEDCChannel)i);
    }
  }

  Serial.println("{\"msg\":\"All LEDC channels released\"}");
}
