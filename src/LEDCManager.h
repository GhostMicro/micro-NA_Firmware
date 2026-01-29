#ifndef LEDC_MANAGER_H
#define LEDC_MANAGER_H

#include <Arduino.h>
#include <driver/ledc.h>

/**
 * LEDCManager - Centralized PWM channel allocation to prevent conflicts
 * 
 * ESP32 has 16 LEDC channels (8 low-speed, 8 high-speed)
 * Allocation:
 * - Channels 0-5: Motors (6 channels)
 * - Channels 6-7: Servo drivers (2 channels)
 * - Remaining: Available for future use
 * 
 * Frequency: 20kHz for motors, 50Hz for servos
 */

class LEDCManager {
public:
    enum LEDCChannel {
        CHANNEL_MOTOR_0 = 0,
        CHANNEL_MOTOR_1 = 1,
        CHANNEL_MOTOR_2 = 2,
        CHANNEL_MOTOR_3 = 3,
        CHANNEL_MOTOR_4 = 4,
        CHANNEL_MOTOR_5 = 5,
        CHANNEL_SERVO_0 = 6,
        CHANNEL_SERVO_1 = 7,
    };

    static LEDCManager& getInstance() {
        static LEDCManager instance;
        return instance;
    }

    /**
     * Allocate a LEDC channel for motor PWM
     * @param pin GPIO pin for PWM output
     * @param channel LEDC channel (0-5 for motors)
     * @return true if successful
     */
    bool allocateMotorChannel(uint8_t pin, LEDCChannel channel);

    /**
     * Allocate a LEDC channel for servo PWM
     * @param pin GPIO pin for PWM output
     * @param channel LEDC channel (6-7 for servos)
     * @return true if successful
     */
    bool allocateServoChannel(uint8_t pin, LEDCChannel channel);

    /**
     * Set PWM value for allocated channel
     * @param channel LEDC channel
     * @param value 0-255 for motors, 0-180 for servos
     */
    void setPWM(LEDCChannel channel, uint16_t value);

    /**
     * Get PWM value for channel
     */
    uint16_t getPWM(LEDCChannel channel);

    /**
     * Check if channel is allocated
     */
    bool isAllocated(LEDCChannel channel) const;

    /**
     * Release LEDC channel and cleanup
     */
    void releaseChannel(LEDCChannel channel);

    /**
     * Release all channels
     */
    void releaseAll();

private:
    LEDCManager() : allocatedChannels(0) {}
    
    static const uint8_t TOTAL_CHANNELS = 8;
    static const uint16_t MOTOR_FREQUENCY = 20000;  // 20kHz for motors
    static const uint16_t SERVO_FREQUENCY = 50;      // 50Hz for servos
    static const uint8_t MOTOR_RESOLUTION = 8;      // 8-bit (0-255)
    static const uint8_t SERVO_RESOLUTION = 8;      // 8-bit (0-255 maps to 0-180°)

    uint8_t allocatedChannels;                        // Bitmask of allocated channels
    uint8_t channelPins[TOTAL_CHANNELS];             // GPIO pins for each channel
    uint16_t channelValues[TOTAL_CHANNELS];          // Current PWM values
    bool isMotorChannel[TOTAL_CHANNELS];             // true = motor, false = servo

    /**
     * Configure LEDC timer and channel
     */
    void configureChannel(LEDCChannel channel, uint8_t pin, uint16_t frequency, bool isMotor);
};

#endif // LEDC_MANAGER_H
