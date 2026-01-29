#ifndef HMAC_VALIDATOR_H
#define HMAC_VALIDATOR_H

#include <stdint.h>
#include <stdbool.h>

/**
 * HMACValidator - HMAC-SHA256 packet authentication
 * 
 * Prevents spoofing attacks by validating packet authenticity:
 * - HMAC-SHA256 message authentication code
 * - Per-packet validation
 * - Paired device verification
 * - Optional (can be disabled for testing)
 * 
 * @file HMACValidator.h
 */

#define HMAC_SHA256_SIZE    32  // 256 bits / 32 bytes
#define HMAC_MAX_PAYLOAD    64  // Max bytes to authenticate

/**
 * Initialize HMAC validator with shared secret
 * @param secret 32-byte shared secret key
 * @return true if initialization successful
 */
bool HMACValidator_init(const uint8_t* secret);

/**
 * Generate HMAC-SHA256 for packet data
 * @param data Packet data to authenticate
 * @param dataLen Length of data
 * @param hmac Output HMAC (32 bytes)
 * @return true if HMAC generated successfully
 */
bool HMACValidator_generate(
    const uint8_t* data,
    uint16_t dataLen,
    uint8_t* hmac
);

/**
 * Validate packet HMAC
 * @param data Packet data
 * @param dataLen Length of data
 * @param receivedHmac HMAC received with packet (32 bytes)
 * @return true if HMAC is valid (matches expected)
 */
bool HMACValidator_validate(
    const uint8_t* data,
    uint16_t dataLen,
    const uint8_t* receivedHmac
);

/**
 * Constant-time HMAC comparison (prevents timing attacks)
 * @param hmac1 First HMAC (32 bytes)
 * @param hmac2 Second HMAC (32 bytes)
 * @return true if HMACs match, false otherwise
 * 
 * Note: Always compares all bytes even if mismatch found early
 */
bool HMACValidator_constantTimeCompare(
    const uint8_t* hmac1,
    const uint8_t* hmac2
);

/**
 * Get HMAC validator status
 * @return true if initialized and ready
 */
bool HMACValidator_isReady(void);

/**
 * Reset HMAC validator state
 */
void HMACValidator_reset(void);

/**
 * Get last error message
 * @return Error description or NULL
 */
const char* HMACValidator_lastError(void);

#endif // HMAC_VALIDATOR_H
