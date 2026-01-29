/**
 * Hardware Abstraction Layer (HAL)
 * 
 * Provides platform-agnostic interface for:
 * - GPIO pins (digital I/O)
 * - PWM/Timer functionality
 * - I2C bus communication
 * - UART/Serial communication
 * - Analog-to-digital conversion
 * 
 * Enables easy porting to different microcontrollers.
 * 
 * @file HAL.h
 * @version 1.0
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// GPIO Pin Operations
// ============================================================================

/**
 * Pin mode enumeration
 */
typedef enum {
    HAL_PIN_INPUT,           // Digital input
    HAL_PIN_INPUT_PULLUP,    // Input with pull-up resistor
    HAL_PIN_INPUT_PULLDOWN,  // Input with pull-down resistor
    HAL_PIN_OUTPUT,          // Digital output
} HAL_PinMode;

/**
 * Pin logic levels
 */
typedef enum {
    HAL_PIN_LOW = 0,
    HAL_PIN_HIGH = 1,
} HAL_PinLevel;

/**
 * Initialize GPIO pin with specified mode
 * @param pin GPIO pin number
 * @param mode Pin mode (input, output, pullup, etc)
 * @return true on success, false on error
 */
bool HAL_PinInit(uint8_t pin, HAL_PinMode mode);

/**
 * Read digital input
 * @param pin GPIO pin number
 * @return Pin level (HAL_PIN_LOW or HAL_PIN_HIGH)
 */
HAL_PinLevel HAL_PinRead(uint8_t pin);

/**
 * Write digital output
 * @param pin GPIO pin number
 * @param level Pin level (HAL_PIN_LOW or HAL_PIN_HIGH)
 * @return true on success, false on error
 */
bool HAL_PinWrite(uint8_t pin, HAL_PinLevel level);

/**
 * Toggle pin output (if HIGH → LOW, if LOW → HIGH)
 * @param pin GPIO pin number
 * @return true on success, false on error
 */
bool HAL_PinToggle(uint8_t pin);

/**
 * Deinitialize GPIO pin
 * @param pin GPIO pin number
 * @return true on success, false on error
 */
bool HAL_PinDeinit(uint8_t pin);

// ============================================================================
// PWM / Timer Operations
// ============================================================================

/**
 * Timer channel allocation status
 */
typedef struct {
    uint8_t channel;         // Channel ID (0-15)
    bool allocated;          // Is channel in use?
    uint32_t frequency;      // PWM frequency in Hz
    uint8_t dutyCycle;       // Duty cycle 0-100%
} HAL_TimerChannel;

/**
 * Allocate a timer channel for PWM output
 * @param pin GPIO pin number for PWM output
 * @param frequency PWM frequency in Hz (e.g., 20000 for motor, 50 for servo)
 * @param initialDuty Initial duty cycle 0-100%
 * @return Channel ID (0-15) on success, -1 on error
 */
int HAL_TimerAllocate(uint8_t pin, uint32_t frequency, uint8_t initialDuty);

/**
 * Set PWM duty cycle
 * @param channel Timer channel (0-15)
 * @param dutyCycle Duty cycle 0-100%
 * @return true on success, false on error
 */
bool HAL_TimerSetDuty(uint8_t channel, uint8_t dutyCycle);

/**
 * Get current PWM duty cycle
 * @param channel Timer channel (0-15)
 * @return Duty cycle 0-100%, or -1 on error
 */
int HAL_TimerGetDuty(uint8_t channel);

/**
 * Change PWM frequency
 * @param channel Timer channel (0-15)
 * @param frequency New frequency in Hz
 * @return true on success, false on error
 */
bool HAL_TimerSetFrequency(uint8_t channel, uint32_t frequency);

/**
 * Release/deallocate a timer channel
 * @param channel Timer channel (0-15)
 * @return true on success, false on error
 */
bool HAL_TimerRelease(uint8_t channel);

/**
 * Release all allocated timer channels
 * @return Number of channels released
 */
uint8_t HAL_TimerReleaseAll(void);

// ============================================================================
// I2C Bus Operations
// ============================================================================

/**
 * I2C transmission error codes
 */
typedef enum {
    HAL_I2C_OK = 0,           // Success
    HAL_I2C_TIMEOUT = 1,      // Timeout waiting for response
    HAL_I2C_NO_ACK = 2,       // Address/data not acknowledged
    HAL_I2C_COLLISION = 3,    // Bus collision
    HAL_I2C_BUS_ERROR = 4,    // General bus error
} HAL_I2CError;

/**
 * Initialize I2C bus
 * @param sda GPIO pin for SDA (data line)
 * @param scl GPIO pin for SCL (clock line)
 * @param frequency I2C clock frequency in Hz (typically 100000 or 400000)
 * @return true on success, false on error
 */
bool HAL_I2CInit(uint8_t sda, uint8_t scl, uint32_t frequency);

/**
 * Scan I2C bus for devices
 * @param addresses Pointer to array for device addresses (size >= 127)
 * @return Number of devices found
 */
uint8_t HAL_I2CScan(uint8_t* addresses);

/**
 * Write data to I2C device (master transmit)
 * @param slaveAddr 7-bit I2C slave address
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 * @param timeout Timeout in milliseconds
 * @return HAL_I2CError code
 */
HAL_I2CError HAL_I2CWrite(uint8_t slaveAddr, const uint8_t* data, 
                          uint8_t length, uint32_t timeout);

/**
 * Read data from I2C device (master receive)
 * @param slaveAddr 7-bit I2C slave address
 * @param buffer Pointer to receive buffer
 * @param length Number of bytes to read
 * @param timeout Timeout in milliseconds
 * @return Number of bytes read, or -1 on error
 */
int HAL_I2CRead(uint8_t slaveAddr, uint8_t* buffer, uint8_t length, 
                uint32_t timeout);

/**
 * Write then read (write register address, then read data)
 * @param slaveAddr 7-bit I2C slave address
 * @param regAddr Register address byte
 * @param buffer Pointer to receive buffer
 * @param length Number of bytes to read
 * @param timeout Timeout in milliseconds
 * @return Number of bytes read, or -1 on error
 */
int HAL_I2CReadReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t* buffer, 
                   uint8_t length, uint32_t timeout);

/**
 * Deinitialize I2C bus
 * @return true on success, false on error
 */
bool HAL_I2CDeinit(void);

// ============================================================================
// ADC / Analog Input Operations
// ============================================================================

/**
 * Initialize ADC on specified pin
 * @param pin GPIO pin number (ADC-capable)
 * @param resolution Resolution in bits (8, 10, 12)
 * @return true on success, false on error
 */
bool HAL_ADCInit(uint8_t pin, uint8_t resolution);

/**
 * Read analog value from ADC
 * @param pin GPIO pin number
 * @param rawValue Pointer to store raw ADC reading
 * @return true on success, false on error
 */
bool HAL_ADCRead(uint8_t pin, uint16_t* rawValue);

/**
 * Convert raw ADC value to voltage
 * @param rawValue Raw ADC reading
 * @param refVoltage Reference voltage (typically 3.3V)
 * @return Voltage in volts
 */
float HAL_ADCToVoltage(uint16_t rawValue, float refVoltage);

/**
 * Deinitialize ADC on specified pin
 * @param pin GPIO pin number
 * @return true on success, false on error
 */
bool HAL_ADCDeinit(uint8_t pin);

// ============================================================================
// Serial / UART Operations
// ============================================================================

/**
 * Initialize UART/Serial port
 * @param baudRate Serial baud rate (typically 115200)
 * @return true on success, false on error
 */
bool HAL_SerialInit(uint32_t baudRate);

/**
 * Write data to serial port
 * @param data Pointer to data buffer
 * @param length Number of bytes to write
 * @return Number of bytes written
 */
uint16_t HAL_SerialWrite(const uint8_t* data, uint16_t length);

/**
 * Read data from serial port
 * @param buffer Pointer to receive buffer
 * @param maxLength Maximum bytes to read
 * @return Number of bytes read (0 if none available)
 */
uint16_t HAL_SerialRead(uint8_t* buffer, uint16_t maxLength);

/**
 * Check if data is available on serial port
 * @return Number of bytes available to read
 */
uint16_t HAL_SerialAvailable(void);

/**
 * Flush serial output buffer
 * @return true on success, false on error
 */
bool HAL_SerialFlush(void);

/**
 * Deinitialize serial port
 * @return true on success, false on error
 */
bool HAL_SerialDeinit(void);

// ============================================================================
// System/Timing Functions
// ============================================================================

/**
 * Get system uptime in milliseconds
 * @return Milliseconds since boot (uint32_t, will wrap after ~49 days)
 */
uint32_t HAL_GetMillis(void);

/**
 * Get system uptime in microseconds
 * @return Microseconds since boot (uint32_t, will wrap after ~71 minutes)
 */
uint32_t HAL_GetMicros(void);

/**
 * Delay execution
 * @param milliseconds Number of milliseconds to delay
 */
void HAL_Delay(uint32_t milliseconds);

/**
 * Delay execution with microsecond precision
 * @param microseconds Number of microseconds to delay
 */
void HAL_DelayMicros(uint32_t microseconds);

/**
 * Get platform name/identifier
 * @return String like "ESP32", "STM32F4", "RP2040", etc
 */
const char* HAL_GetPlatformName(void);

/**
 * Get platform-specific information
 * @return String with version, RAM, flash, etc
 */
const char* HAL_GetPlatformInfo(void);

#endif  // HAL_H
