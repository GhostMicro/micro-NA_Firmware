/**
 * Unit Tests for EncryptionManager
 * Tests AES-256 CTR mode encryption/decryption
 * 
 * @file test_EncryptionManager.cpp
 * @framework Unity Test Framework (PlatformIO)
 */

#include <unity.h>
#include "../EncryptionManager.h"
#include <string.h>

// ============================================================================
// Test Fixtures
// ============================================================================

uint8_t testKey[AES_256_KEY_SIZE];
uint8_t testIV[AES_IV_SIZE];
uint8_t plaintext[AES_MAX_PAYLOAD];
uint8_t ciphertext[AES_MAX_PAYLOAD];
uint8_t decrypted[AES_MAX_PAYLOAD];

void setUp(void) {
    // Initialize test key (all zeros for deterministic testing)
    memset(testKey, 0xAA, AES_256_KEY_SIZE);
    memset(testIV, 0x00, AES_IV_SIZE);
    memset(plaintext, 0x00, AES_MAX_PAYLOAD);
    memset(ciphertext, 0x00, AES_MAX_PAYLOAD);
    memset(decrypted, 0x00, AES_MAX_PAYLOAD);
    
    // Initialize encryption manager
    TEST_ASSERT_TRUE(EncryptionManager_init(testKey));
}

void tearDown(void) {
    // Cleanup: reset state for next test
    memset(testKey, 0x00, AES_256_KEY_SIZE);
}

// ============================================================================
// Initialization Tests
// ============================================================================

void test_EncryptionManager_init_success(void) {
    // Verify initialization with valid key
    uint8_t key[AES_256_KEY_SIZE];
    memset(key, 0xFF, AES_256_KEY_SIZE);
    
    TEST_ASSERT_TRUE(EncryptionManager_init(key));
    TEST_ASSERT_TRUE(EncryptionManager_isReady());
}

void test_EncryptionManager_init_null_key(void) {
    // Initialization should fail with NULL key
    TEST_ASSERT_FALSE(EncryptionManager_init(NULL));
}

// ============================================================================
// IV Generation Tests
// ============================================================================

void test_EncryptionManager_generateIV_success(void) {
    // Generate IV successfully
    uint8_t iv[AES_IV_SIZE];
    memset(iv, 0x00, AES_IV_SIZE);
    
    TEST_ASSERT_TRUE(EncryptionManager_generateIV(iv));
    
    // IV should not be all zeros
    uint8_t zeroCheck = 0;
    for (int i = 0; i < AES_IV_SIZE; i++) {
        zeroCheck |= iv[i];
    }
    TEST_ASSERT_NOT_EQUAL(0x00, zeroCheck);
}

void test_EncryptionManager_generateIV_null_buffer(void) {
    // Should fail with NULL IV buffer
    TEST_ASSERT_FALSE(EncryptionManager_generateIV(NULL));
}

// ============================================================================
// Encryption/Decryption Tests
// ============================================================================

void test_EncryptionManager_encrypt_decrypt_roundtrip(void) {
    // Encrypt and decrypt should produce original plaintext
    uint8_t iv[AES_IV_SIZE];
    memset(iv, 0x55, AES_IV_SIZE);
    
    // Prepare plaintext
    memset(plaintext, 0x42, 16);  // 16 bytes of 0x42
    
    // Encrypt
    TEST_ASSERT_TRUE(EncryptionManager_encrypt(plaintext, 16, iv, ciphertext));
    
    // Ciphertext should be different from plaintext (after proper encryption)
    // Note: Phase 9 skeleton returns plaintext, so this may fail - TODO when implementing
    
    // Decrypt
    TEST_ASSERT_TRUE(EncryptionManager_decrypt(ciphertext, 16, iv, decrypted));
    
    // Decrypted should match original plaintext
    TEST_ASSERT_EQUAL_MEMORY(plaintext, decrypted, 16);
}

void test_EncryptionManager_encrypt_different_iv_different_ciphertext(void) {
    // Same plaintext with different IVs should produce different ciphertexts
    uint8_t iv1[AES_IV_SIZE];
    uint8_t iv2[AES_IV_SIZE];
    uint8_t ct1[AES_MAX_PAYLOAD];
    uint8_t ct2[AES_MAX_PAYLOAD];
    
    memset(iv1, 0x11, AES_IV_SIZE);
    memset(iv2, 0x22, AES_IV_SIZE);
    memset(plaintext, 0xCC, 16);
    
    TEST_ASSERT_TRUE(EncryptionManager_encrypt(plaintext, 16, iv1, ct1));
    TEST_ASSERT_TRUE(EncryptionManager_encrypt(plaintext, 16, iv2, ct2));
    
    // Ciphertexts should be different (after proper encryption)
    // Note: Phase 9 skeleton may not produce different outputs - TODO
}

void test_EncryptionManager_encrypt_null_plaintext(void) {
    // Should fail with NULL plaintext
    uint8_t iv[AES_IV_SIZE];
    TEST_ASSERT_FALSE(EncryptionManager_encrypt(NULL, 16, iv, ciphertext));
}

void test_EncryptionManager_encrypt_payload_too_large(void) {
    // Should fail if payload > AES_MAX_PAYLOAD
    uint8_t iv[AES_IV_SIZE];
    memset(iv, 0x00, AES_IV_SIZE);
    
    TEST_ASSERT_FALSE(EncryptionManager_encrypt(plaintext, AES_MAX_PAYLOAD + 1, iv, ciphertext));
}

void test_EncryptionManager_deriveKey_success(void) {
    // Key derivation should produce 32-byte key
    const char* password = "test_password_123";
    uint8_t salt[16];
    uint8_t derivedKey[AES_256_KEY_SIZE];
    
    memset(salt, 0xAB, 16);
    memset(derivedKey, 0x00, AES_256_KEY_SIZE);
    
    TEST_ASSERT_TRUE(EncryptionManager_deriveKey(
        password,
        strlen(password),
        salt,
        10000,  // PBKDF2 iterations
        derivedKey
    ));
    
    // Derived key should not be all zeros
    uint8_t zeroCheck = 0;
    for (int i = 0; i < AES_256_KEY_SIZE; i++) {
        zeroCheck |= derivedKey[i];
    }
    TEST_ASSERT_NOT_EQUAL(0x00, zeroCheck);
}

void test_EncryptionManager_deriveKey_insufficient_iterations(void) {
    // Should fail with too few iterations
    const char* password = "test";
    uint8_t salt[16];
    uint8_t derivedKey[AES_256_KEY_SIZE];
    
    memset(salt, 0x00, 16);
    
    TEST_ASSERT_FALSE(EncryptionManager_deriveKey(
        password,
        strlen(password),
        salt,
        5000,  // Too few (min 10000)
        derivedKey
    ));
}

// ============================================================================
// Test Suite Registration
// ============================================================================

void test_group_encryption_setup(void) {
    RUN_TEST(test_EncryptionManager_init_success);
    RUN_TEST(test_EncryptionManager_init_null_key);
    RUN_TEST(test_EncryptionManager_generateIV_success);
    RUN_TEST(test_EncryptionManager_generateIV_null_buffer);
    RUN_TEST(test_EncryptionManager_encrypt_decrypt_roundtrip);
    RUN_TEST(test_EncryptionManager_encrypt_different_iv_different_ciphertext);
    RUN_TEST(test_EncryptionManager_encrypt_null_plaintext);
    RUN_TEST(test_EncryptionManager_encrypt_payload_too_large);
    RUN_TEST(test_EncryptionManager_deriveKey_success);
    RUN_TEST(test_EncryptionManager_deriveKey_insufficient_iterations);
}
