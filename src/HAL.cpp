/**
 * Hardware Abstraction Layer (HAL) Implementation for ESP32
 *
 * @file HAL.cpp
 * @version 1.0
 */

#include "HAL.h"
#include "driver/ledc.h"
#include <Arduino.h>
#include <Wire.h>

// ============================================================================
// Global State
// ============================================================================

// PWM/Timer channel tracking
#define MAX_PWM_CHANNELS 16
static struct {
  bool allocated;
  uint8_t pin;
  uint32_t frequency;
  uint8_t dutyCycle;
} pwm_channels[MAX_PWM_CHANNELS] = {};

// I2C state
static bool i2c_initialized = false;
static TwoWire *i2c_bus = nullptr;

// ADC resolution tracking
#define MAX_ADC_PINS 8
static struct {
  bool initialized;
  uint8_t resolution;
} adc_pins[MAX_ADC_PINS] = {};

// ============================================================================
// GPIO Pin Operations
// ============================================================================

bool HAL_PinInit(uint8_t pin, HAL_PinMode mode) {
  uint8_t pinMode_arg = OUTPUT;
  switch (mode) {
  case HAL_PIN_INPUT:
    pinMode_arg = INPUT;
    break;
  case HAL_PIN_INPUT_PULLUP:
    pinMode_arg = INPUT_PULLUP;
    break;
  case HAL_PIN_INPUT_PULLDOWN:
    pinMode_arg = INPUT_PULLDOWN; // ESP32-specific
    break;
  case HAL_PIN_OUTPUT:
    pinMode_arg = OUTPUT;
    break;
  }
  pinMode(pin, pinMode_arg);
  return true;
}

HAL_PinLevel HAL_PinRead(uint8_t pin) { return (HAL_PinLevel)digitalRead(pin); }

bool HAL_PinWrite(uint8_t pin, HAL_PinLevel level) {
  digitalWrite(pin, level);
  return true;
}

bool HAL_PinToggle(uint8_t pin) {
  digitalWrite(pin, !digitalRead(pin));
  return true;
}

bool HAL_PinDeinit(uint8_t pin) {
  // No explicit cleanup needed on Arduino/ESP32
  return true;
}

// ============================================================================
// PWM / Timer Operations (ESP32 LEDC)
// ============================================================================

int HAL_TimerAllocate(uint8_t pin, uint32_t frequency, uint8_t initialDuty) {
  // Find free channel
  int channel = -1;
  for (int i = 0; i < MAX_PWM_CHANNELS; i++) {
    if (!pwm_channels[i].allocated) {
      channel = i;
      break;
    }
  }

  if (channel == -1) {
    return -1; // No free channels
  }

  // Configure LEDC timer
  uint32_t freq_hz = frequency;
  uint8_t resolution = 8; // 8-bit resolution (0-255)
  if (frequency >= 20000) {
    resolution = 8; // 20kHz for motors
  } else if (frequency == 50) {
    resolution = 12; // 12-bit for servo PWM
  }

  // Determine timer/speed mode based on frequency
  ledc_timer_t timer = (frequency >= 20000) ? LEDC_TIMER_0 : LEDC_TIMER_1;
  ledc_mode_t speed_mode = LEDC_LOW_SPEED_MODE;

  // Configure timer
  ledc_timer_config_t timer_conf = {
      .speed_mode = speed_mode,
      .duty_resolution = (ledc_timer_bit_t)resolution,
      .timer_num = timer,
      .freq_hz = freq_hz,
      .clk_cfg = LEDC_AUTO_CLK,
  };

  if (ledc_timer_config(&timer_conf) != ESP_OK) {
    return -1;
  }

  // Configure channel
  ledc_channel_config_t channel_conf = {
      .gpio_num = pin,
      .speed_mode = speed_mode,
      .channel = (ledc_channel_t)channel,
      .intr_type = LEDC_INTR_DISABLE,
      .timer_sel = timer,
      .duty =
          (initialDuty * ((1 << resolution) - 1)) / 100, // Convert % to counts
      .hpoint = 0,
  };

  if (ledc_channel_config(&channel_conf) != ESP_OK) {
    return -1;
  }

  // Update channel tracking
  pwm_channels[channel].allocated = true;
  pwm_channels[channel].pin = pin;
  pwm_channels[channel].frequency = frequency;
  pwm_channels[channel].dutyCycle = initialDuty;

  return channel;
}

bool HAL_TimerSetDuty(uint8_t channel, uint8_t dutyCycle) {
  if (channel >= MAX_PWM_CHANNELS || !pwm_channels[channel].allocated) {
    return false;
  }

  uint8_t resolution = (pwm_channels[channel].frequency >= 20000) ? 8 : 12;
  uint32_t duty_counts = (dutyCycle * ((1 << resolution) - 1)) / 100;

  ledc_set_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel, duty_counts);
  ledc_update_duty(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel);

  pwm_channels[channel].dutyCycle = dutyCycle;
  return true;
}

int HAL_TimerGetDuty(uint8_t channel) {
  if (channel >= MAX_PWM_CHANNELS || !pwm_channels[channel].allocated) {
    return -1;
  }
  return pwm_channels[channel].dutyCycle;
}

bool HAL_TimerSetFrequency(uint8_t channel, uint32_t frequency) {
  if (channel >= MAX_PWM_CHANNELS || !pwm_channels[channel].allocated) {
    return false;
  }

  // Frequency changes require timer reconfiguration - not implemented yet
  // Return false for now to prevent runtime errors
  return false;
}

bool HAL_TimerRelease(uint8_t channel) {
  if (channel >= MAX_PWM_CHANNELS || !pwm_channels[channel].allocated) {
    return false;
  }

  ledc_stop(LEDC_HIGH_SPEED_MODE, (ledc_channel_t)channel, 0);
  pwm_channels[channel].allocated = false;
  pwm_channels[channel].pin = 0;
  pwm_channels[channel].frequency = 0;
  pwm_channels[channel].dutyCycle = 0;

  return true;
}

uint8_t HAL_TimerReleaseAll(void) {
  uint8_t count = 0;
  for (int i = 0; i < MAX_PWM_CHANNELS; i++) {
    if (pwm_channels[i].allocated && HAL_TimerRelease(i)) {
      count++;
    }
  }
  return count;
}

// ============================================================================
// I2C Bus Operations
// ============================================================================

bool HAL_I2CInit(uint8_t sda, uint8_t scl, uint32_t frequency) {
  if (i2c_initialized) {
    return true; // Already initialized
  }

  i2c_bus = &Wire;
  Wire.begin(sda, scl);
  Wire.setClock(frequency);
  i2c_initialized = true;
  return true;
}

uint8_t HAL_I2CScan(uint8_t *addresses) {
  if (!i2c_initialized || !addresses) {
    return 0;
  }

  uint8_t count = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      addresses[count++] = addr;
    }
  }
  return count;
}

HAL_I2CError HAL_I2CWrite(uint8_t slaveAddr, const uint8_t *data,
                          uint8_t length, uint32_t timeout) {
  if (!i2c_initialized || !data) {
    return HAL_I2C_BUS_ERROR;
  }

  uint32_t start = millis();
  Wire.beginTransmission(slaveAddr);
  Wire.write(data, length);
  uint8_t status = Wire.endTransmission();

  if (status == 0) {
    return HAL_I2C_OK;
  } else if (status == 4) {
    return HAL_I2C_BUS_ERROR;
  } else {
    return HAL_I2C_NO_ACK;
  }
}

int HAL_I2CRead(uint8_t slaveAddr, uint8_t *buffer, uint8_t length,
                uint32_t timeout) {
  if (!i2c_initialized || !buffer) {
    return -1;
  }

  size_t received = Wire.requestFrom(slaveAddr, (size_t)length);
  if (received != length) {
    return -1; // Didn't receive expected number of bytes
  }

  for (int i = 0; i < received; i++) {
    buffer[i] = Wire.read();
  }

  return (int)received;
}

int HAL_I2CReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t *buffer,
                   uint8_t length, uint32_t timeout) {
  // Write register address
  if (HAL_I2CWrite(slaveAddr, &regAddr, 1, timeout) != HAL_I2C_OK) {
    return -1;
  }

  // Read data
  return HAL_I2CRead(slaveAddr, buffer, length, timeout);
}

bool HAL_I2CDeinit(void) {
  // ESP32 doesn't provide a clean deinit function
  // Just mark as uninitialized
  i2c_initialized = false;
  return true;
}

// ============================================================================
// ADC / Analog Input Operations
// ============================================================================

bool HAL_ADCInit(uint8_t pin, uint8_t resolution) {
  if (pin >= MAX_ADC_PINS) {
    return false;
  }

  analogReadResolution(resolution);
  adc_pins[pin].initialized = true;
  adc_pins[pin].resolution = resolution;
  return true;
}

bool HAL_ADCRead(uint8_t pin, uint16_t *rawValue) {
  if (!adc_pins[pin].initialized || !rawValue) {
    return false;
  }

  *rawValue = analogRead(pin);
  return true;
}

float HAL_ADCToVoltage(uint16_t rawValue, float refVoltage) {
  // ESP32 is 12-bit ADC by default (0-4095)
  return (float)rawValue * refVoltage / 4095.0f;
}

bool HAL_ADCDeinit(uint8_t pin) {
  if (pin >= MAX_ADC_PINS) {
    return false;
  }

  adc_pins[pin].initialized = false;
  return true;
}

// ============================================================================
// Serial / UART Operations
// ============================================================================

bool HAL_SerialInit(uint32_t baudRate) {
  Serial.begin(baudRate);
  delay(100); // Give serial time to initialize
  return true;
}

uint16_t HAL_SerialWrite(const uint8_t *data, uint16_t length) {
  if (!data)
    return 0;
  return Serial.write(data, length);
}

uint16_t HAL_SerialRead(uint8_t *buffer, uint16_t maxLength) {
  if (!buffer)
    return 0;

  uint16_t count = 0;
  while (Serial.available() && count < maxLength) {
    buffer[count++] = Serial.read();
  }
  return count;
}

uint16_t HAL_SerialAvailable(void) { return Serial.available(); }

bool HAL_SerialFlush(void) {
  Serial.flush();
  return true;
}

bool HAL_SerialDeinit(void) {
  Serial.end();
  return true;
}

// ============================================================================
// System/Timing Functions
// ============================================================================

uint32_t HAL_GetMillis(void) { return millis(); }

uint32_t HAL_GetMicros(void) { return micros(); }

void HAL_Delay(uint32_t milliseconds) { delay(milliseconds); }

void HAL_DelayMicros(uint32_t microseconds) { delayMicroseconds(microseconds); }

const char *HAL_GetPlatformName(void) { return "ESP32"; }

const char *HAL_GetPlatformInfo(void) {
  static char info[128];
  snprintf(info, sizeof(info), "ESP32 Rev %d, %d MB Flash, %d KB RAM",
           ESP.getChipRevision(), ESP.getFlashChipSize() / 1024 / 1024,
           ESP.getFreePsram() / 1024);
  return info;
}
