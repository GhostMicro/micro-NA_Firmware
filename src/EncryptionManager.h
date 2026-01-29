#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * EncryptionManager - AES-256 CTR mode encryption/decryption
 * 
 * Handles:
 * - AES-256 encryption in CTR mode (stream cipher)
 * - IV generation for each packet
 * - Key derivation (PBKDF2)
 * - Secure memory handling
 * 
 * Note: Phase 9 uses pre-shared keys (Phase 10: ECDH key exchange)
 * 
 * @file EncryptionManager.h
 */

#define AES_256_KEY_SIZE    32  // 256 bits
#define AES_IV_SIZE         16  // 128 bits
#define AES_MAX_PAYLOAD     64  // Max bytes to encrypt

/**
 * Initialize encryption with shared key
 * @param key 32-byte AES-256 key (must be from ConfigManager or ECDH)
 * @return true if initialization successful
 */
bool EncryptionManager_init(const uint8_t* key);

/**
 * Generate random IV for packet encryption
 * @param iv Output buffer (16 bytes)
 * @return true if IV generated successfully
 */
bool EncryptionManager_generateIV(uint8_t* iv);

/**
 * Encrypt plaintext data with AES-256 CTR
 * @param plaintext Input data
 * @param len Length of data (1-AES_MAX_PAYLOAD)
 * @param iv Initialization vector (16 bytes)
 * @param ciphertext Output encrypted data (same length as plaintext)
 * @return true if encryption successful
 */
bool EncryptionManager_encrypt(
    const uint8_t* plaintext,
    uint16_t len,
    const uint8_t* iv,
    uint8_t* ciphertext
);

/**
 * Decrypt ciphertext data with AES-256 CTR
 * @param ciphertext Encrypted data
 * @param len Length of data (1-AES_MAX_PAYLOAD)
 * @param iv Initialization vector (16 bytes)
 * @param plaintext Output decrypted data (same length as ciphertext)
 * @return true if decryption successful
 */
bool EncryptionManager_decrypt(
    const uint8_t* ciphertext,
    uint16_t len,
    const uint8_t* iv,
    uint8_t* plaintext
);

/**
 * Derive encryption key from password using PBKDF2
 * @param password User password
 * @param passwordLen Length of password
 * @param salt Random salt (16 bytes)
 * @param iterations PBKDF2 iterations (min 10000)
 * @param derivedKey Output 32-byte key
 * @return true if key derived successfully
 */
bool EncryptionManager_deriveKey(
    const char* password,
    uint16_t passwordLen,
    const uint8_t* salt,
    uint32_t iterations,
    uint8_t* derivedKey
);

/**
 * Get encryption status
 * @return true if encryption initialized and ready
 */
bool EncryptionManager_isReady(void);

/**
 * Get last error message
 * @return Error description or NULL
 */
const char* EncryptionManager_lastError(void);

#endif // ENCRYPTION_MANAGER_H
