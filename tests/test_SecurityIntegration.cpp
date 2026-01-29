/**
 * test_SecurityIntegration.cpp
 * 
 * Integration tests for Phase 9 Security Foundation
 * Tests the interaction between:
 * - RateLimitManager (limit incoming commands)
 * - EncryptionManager (decrypt payload)
 * - HMACValidator (authenticate payload)
 * - NAPacket (modified to support encryption flag + IV)
 * 
 * Covers:
 * 1. Rate-limited command flow (allowed vs blocked)
 * 2. Encrypted packet validation (valid vs tampered)
 * 3. HMAC authentication (valid vs forged HMAC)
 * 4. End-to-end secure packet handling
 * 5. Graceful degradation under attack
 */

#include <unity.h>
#include "EncryptionManager.h"
#include "RateLimitManager.h"
#include "HMACValidator.h"
#include "NAPacket.h"

// Mock packet for testing
typedef struct __attribute__((packed)) {
    uint8_t throttle;
    uint8_t roll;
    uint8_t pitch;
    uint8_t yaw;
} MockPayload;

void setUp(void) {
    // Reset all managers before each test
    RateLimitManager_reset();
    HMACValidator_reset();
}

void tearDown(void) {
}

// ============================================================================
// Test Group 1: Rate Limiting Integration
// ============================================================================

void test_SecurityIntegration_rate_limit_allows_valid_commands(void) {
    // ARRANGE
    RateLimitManager_init(100);
    
    // ACT - Allow 50 commands in quick succession
    RateLimitStatus results[50];
    for (int i = 0; i < 50; i++) {
        results[i] = RateLimitManager_checkCommand(1);  // commandType = 1
    }
    
    // ASSERT
    for (int i = 0; i < 50; i++) {
        TEST_ASSERT_EQUAL(RATE_LIMIT_OK, results[i]);
    }
}

void test_SecurityIntegration_rate_limit_blocks_excess_commands(void) {
    // ARRANGE
    RateLimitManager_init(50);  // Small capacity
    
    // ACT - Send 100+ commands (should block after 50)
    RateLimitStatus firstBatch[50];
    for (int i = 0; i < 50; i++) {
        firstBatch[i] = RateLimitManager_checkCommand(1);
    }
    
    // Second batch should be blocked
    RateLimitStatus secondBatch[10];
    for (int i = 0; i < 10; i++) {
        secondBatch[i] = RateLimitManager_checkCommand(1);
    }
    
    // ASSERT
    for (int i = 0; i < 50; i++) {
        TEST_ASSERT_EQUAL(RATE_LIMIT_OK, firstBatch[i]);
    }
    for (int i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL(RATE_LIMIT_EXCEEDED, secondBatch[i]);
    }
    
    // Verify statistics
    RateLimitStats stats = RateLimitManager_getStats();
    TEST_ASSERT_EQUAL(50, stats.totalAllowed);
    TEST_ASSERT_EQUAL(10, stats.totalBlocked);
}

void test_SecurityIntegration_rate_limit_recovery_after_refill(void) {
    // ARRANGE
    RateLimitManager_init(10);
    
    // ACT - Exhaust tokens
    for (int i = 0; i < 10; i++) {
        RateLimitManager_checkCommand(1);
    }
    uint8_t tokensBefore = RateLimitManager_getTokens();
    
    // Refill and check again
    RateLimitManager_refill();
    uint8_t tokensAfter = RateLimitManager_getTokens();
    
    // ASSERT
    TEST_ASSERT_EQUAL(0, tokensBefore);
    TEST_ASSERT_GREATER_THAN(0, tokensAfter);
}

// ============================================================================
// Test Group 2: Encryption & Decryption Flow
// ============================================================================

void test_SecurityIntegration_encrypt_decrypt_roundtrip(void) {
    // ARRANGE
    uint8_t key[32] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
                       0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                       0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};
    MockPayload original = {100, 50, -30, 0};
    
    EncryptionManager_init(key);
    
    uint8_t iv[16];
    uint8_t ciphertext[20];
    uint8_t decrypted[20];
    
    // ACT
    EncryptionManager_generateIV(iv);
    EncryptionManager_encrypt((uint8_t*)&original, sizeof(MockPayload), 
                             iv, ciphertext);
    EncryptionManager_decrypt(ciphertext, sizeof(MockPayload), 
                             iv, decrypted);
    
    // ASSERT
    MockPayload* decryptedPayload = (MockPayload*)decrypted;
    TEST_ASSERT_EQUAL(original.throttle, decryptedPayload->throttle);
    TEST_ASSERT_EQUAL(original.roll, decryptedPayload->roll);
    TEST_ASSERT_EQUAL(original.pitch, decryptedPayload->pitch);
    TEST_ASSERT_EQUAL(original.yaw, decryptedPayload->yaw);
}

void test_SecurityIntegration_different_iv_produces_different_ciphertext(void) {
    // ARRANGE
    uint8_t key[32] = {0};
    memset(key, 0x42, 32);
    
    MockPayload payload = {100, 50, -30, 0};
    EncryptionManager_init(key);
    
    uint8_t iv1[16], iv2[16];
    uint8_t cipher1[20], cipher2[20];
    
    // ACT
    EncryptionManager_generateIV(iv1);
    EncryptionManager_generateIV(iv2);
    EncryptionManager_encrypt((uint8_t*)&payload, sizeof(MockPayload), 
                             iv1, cipher1);
    EncryptionManager_encrypt((uint8_t*)&payload, sizeof(MockPayload), 
                             iv2, cipher2);
    
    // ASSERT - IVs should be different
    bool ivsDifferent = false;
    for (int i = 0; i < 16; i++) {
        if (iv1[i] != iv2[i]) {
            ivsDifferent = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(ivsDifferent);
    
    // Ciphertexts should be different (due to different IVs)
    bool ciphersDifferent = false;
    for (int i = 0; i < 20; i++) {
        if (cipher1[i] != cipher2[i]) {
            ciphersDifferent = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(ciphersDifferent);
}

// ============================================================================
// Test Group 3: HMAC Validation Flow
// ============================================================================

void test_SecurityIntegration_hmac_validates_authentic_payload(void) {
    // ARRANGE
    uint8_t secret[32];
    memset(secret, 0xAA, 32);
    
    MockPayload payload = {100, 50, -30, 0};
    uint8_t hmac[32];
    
    HMACValidator_init(secret);
    
    // ACT
    HMACValidator_generate((uint8_t*)&payload, sizeof(MockPayload), hmac);
    bool isValid = HMACValidator_validate((uint8_t*)&payload, 
                                         sizeof(MockPayload), hmac);
    
    // ASSERT
    TEST_ASSERT_TRUE(isValid);
}

void test_SecurityIntegration_hmac_rejects_tampered_payload(void) {
    // ARRANGE
    uint8_t secret[32];
    memset(secret, 0xAA, 32);
    
    MockPayload original = {100, 50, -30, 0};
    MockPayload tampered = {100, 51, -30, 0};  // Changed roll from 50 to 51
    uint8_t hmac[32];
    
    HMACValidator_init(secret);
    
    // ACT
    HMACValidator_generate((uint8_t*)&original, sizeof(MockPayload), hmac);
    // Try to validate with tampered payload
    bool isValid = HMACValidator_validate((uint8_t*)&tampered, 
                                         sizeof(MockPayload), hmac);
    
    // ASSERT
    TEST_ASSERT_FALSE(isValid);
}

void test_SecurityIntegration_hmac_rejects_forged_hmac(void) {
    // ARRANGE
    uint8_t secret[32];
    memset(secret, 0xAA, 32);
    
    MockPayload payload = {100, 50, -30, 0};
    uint8_t forgedHmac[32];
    memset(forgedHmac, 0xFF, 32);  // Forged HMAC (all 0xFF)
    
    HMACValidator_init(secret);
    
    // ACT
    bool isValid = HMACValidator_validate((uint8_t*)&payload, 
                                         sizeof(MockPayload), forgedHmac);
    
    // ASSERT
    TEST_ASSERT_FALSE(isValid);
}

// ============================================================================
// Test Group 4: End-to-End Secure Packet Handling
// ============================================================================

void test_SecurityIntegration_full_secure_packet_flow(void) {
    // ARRANGE - Setup all managers
    uint8_t key[32];
    uint8_t secret[32];
    memset(key, 0x42, 32);
    memset(secret, 0xAA, 32);
    
    EncryptionManager_init(key);
    HMACValidator_init(secret);
    RateLimitManager_init(100);
    
    // Original command packet
    MockPayload originalCommand = {100, 50, -30, 0};
    
    // ACT - Secure transmission flow:
    // 1. Rate limit check
    RateLimitStatus rateLimitStatus = RateLimitManager_checkCommand(1);
    
    // 2. Encrypt payload
    uint8_t iv[16];
    uint8_t ciphertext[20];
    EncryptionManager_generateIV(iv);
    EncryptionManager_encrypt((uint8_t*)&originalCommand, 
                             sizeof(MockPayload), iv, ciphertext);
    
    // 3. Generate HMAC for authentication
    uint8_t hmac[32];
    HMACValidator_generate(ciphertext, sizeof(MockPayload), hmac);
    
    // 4. [Transmission happens here]
    
    // 5. Receiver: Validate HMAC
    bool hmacValid = HMACValidator_validate(ciphertext, 
                                           sizeof(MockPayload), hmac);
    
    // 6. Receiver: Decrypt
    uint8_t decrypted[20];
    EncryptionManager_decrypt(ciphertext, sizeof(MockPayload), 
                             iv, decrypted);
    
    // ASSERT
    TEST_ASSERT_EQUAL(RATE_LIMIT_OK, rateLimitStatus);
    TEST_ASSERT_TRUE(hmacValid);
    
    MockPayload* decryptedCommand = (MockPayload*)decrypted;
    TEST_ASSERT_EQUAL(originalCommand.throttle, decryptedCommand->throttle);
    TEST_ASSERT_EQUAL(originalCommand.roll, decryptedCommand->roll);
}

void test_SecurityIntegration_attack_tampering_detected_by_hmac(void) {
    // ARRANGE
    uint8_t key[32];
    uint8_t secret[32];
    memset(key, 0x42, 32);
    memset(secret, 0xAA, 32);
    
    EncryptionManager_init(key);
    HMACValidator_init(secret);
    
    MockPayload originalCommand = {100, 50, -30, 0};
    
    // Encrypt and generate HMAC
    uint8_t iv[16];
    uint8_t ciphertext[20];
    uint8_t hmac[32];
    
    EncryptionManager_generateIV(iv);
    EncryptionManager_encrypt((uint8_t*)&originalCommand, 
                             sizeof(MockPayload), iv, ciphertext);
    HMACValidator_generate(ciphertext, sizeof(MockPayload), hmac);
    
    // ACT - Attacker modifies ciphertext
    ciphertext[5] ^= 0xFF;  // Flip some bits
    
    // Receiver validates modified ciphertext
    bool hmacValid = HMACValidator_validate(ciphertext, 
                                           sizeof(MockPayload), hmac);
    
    // ASSERT
    TEST_ASSERT_FALSE(hmacValid);  // HMAC validation should fail
}

void test_SecurityIntegration_dos_attack_blocked_by_rate_limit(void) {
    // ARRANGE
    RateLimitManager_init(50);  // Low limit for testing
    
    // ACT - Simulate DoS attack: 200 rapid commands
    RateLimitStatus results[200];
    for (int i = 0; i < 200; i++) {
        results[i] = RateLimitManager_checkCommand(1);
    }
    
    // Count allowed vs blocked
    uint16_t allowedCount = 0, blockedCount = 0;
    for (int i = 0; i < 200; i++) {
        if (results[i] == RATE_LIMIT_OK) allowedCount++;
        else blockedCount++;
    }
    
    // ASSERT
    TEST_ASSERT_EQUAL(50, allowedCount);  // Should allow exactly 50
    TEST_ASSERT_EQUAL(150, blockedCount); // Should block 150
    
    // Verify stats
    RateLimitStats stats = RateLimitManager_getStats();
    TEST_ASSERT_EQUAL(50, stats.totalAllowed);
    TEST_ASSERT_EQUAL(150, stats.totalBlocked);
}

// ============================================================================
// Test Group 5: Error Conditions & Graceful Degradation
// ============================================================================

void test_SecurityIntegration_managers_handle_null_inputs_gracefully(void) {
    // ARRANGE
    uint8_t key[32];
    memset(key, 0x42, 32);
    EncryptionManager_init(key);
    HMACValidator_init(key);
    
    // ACT
    bool encryptResult = EncryptionManager_encrypt(NULL, 0, NULL, NULL);
    bool hmacResult = HMACValidator_generate(NULL, 0, NULL);
    
    // ASSERT
    TEST_ASSERT_FALSE(encryptResult);
    TEST_ASSERT_FALSE(hmacResult);
    
    // Verify error messages are set
    const char* encErr = EncryptionManager_lastError();
    const char* hmacErr = HMACValidator_lastError();
    TEST_ASSERT_NOT_NULL(encErr);
    TEST_ASSERT_NOT_NULL(hmacErr);
}

void test_SecurityIntegration_rate_limit_gracefully_handles_zero_capacity(void) {
    // ARRANGE
    RateLimitManager_init(0);
    
    // ACT
    RateLimitStatus status = RateLimitManager_checkCommand(1);
    
    // ASSERT
    TEST_ASSERT_EQUAL(RATE_LIMIT_EXCEEDED, status);
}

void test_SecurityIntegration_encrypted_packet_structure_validation(void) {
    // ARRANGE - Create NAPacket with encryption flag set
    NAPacket packet;
    packet.protocolVersion = 0x01;
    packet.vehicleType = 1;
    packet.encryptionFlag = 1;  // Encrypted
    packet.throttle = 100;
    packet.roll = 50;
    packet.pitch = -30;
    packet.yaw = 0;
    packet.mode = 0;
    packet.buttons = 0;
    packet.sequenceNumber = 1;
    
    // IV should be present
    memset(packet.iv, 0x42, 16);
    
    // HMAC should be present (optional)
    memset(packet.hmac, 0xAA, 32);
    
    // ACT - Verify packet structure
    TEST_ASSERT_EQUAL(1, packet.encryptionFlag);
    TEST_ASSERT_EQUAL(16, sizeof(packet.iv) / sizeof(packet.iv[0]));
    TEST_ASSERT_EQUAL(32, sizeof(packet.hmac) / sizeof(packet.hmac[0]));
    
    // ASSERT
    TEST_ASSERT_NOT_EQUAL(0, packet.iv[0]);
    TEST_ASSERT_NOT_EQUAL(0, packet.hmac[0]);
}

// ============================================================================
// Test Run Configuration
// ============================================================================

int main(void) {
    UNITY_BEGIN();
    
    // Rate Limiting Integration Tests
    RUN_TEST(test_SecurityIntegration_rate_limit_allows_valid_commands);
    RUN_TEST(test_SecurityIntegration_rate_limit_blocks_excess_commands);
    RUN_TEST(test_SecurityIntegration_rate_limit_recovery_after_refill);
    
    // Encryption Flow Tests
    RUN_TEST(test_SecurityIntegration_encrypt_decrypt_roundtrip);
    RUN_TEST(test_SecurityIntegration_different_iv_produces_different_ciphertext);
    
    // HMAC Validation Tests
    RUN_TEST(test_SecurityIntegration_hmac_validates_authentic_payload);
    RUN_TEST(test_SecurityIntegration_hmac_rejects_tampered_payload);
    RUN_TEST(test_SecurityIntegration_hmac_rejects_forged_hmac);
    
    // End-to-End Tests
    RUN_TEST(test_SecurityIntegration_full_secure_packet_flow);
    RUN_TEST(test_SecurityIntegration_attack_tampering_detected_by_hmac);
    RUN_TEST(test_SecurityIntegration_dos_attack_blocked_by_rate_limit);
    
    // Error Handling Tests
    RUN_TEST(test_SecurityIntegration_managers_handle_null_inputs_gracefully);
    RUN_TEST(test_SecurityIntegration_rate_limit_gracefully_handles_zero_capacity);
    RUN_TEST(test_SecurityIntegration_encrypted_packet_structure_validation);
    
    return UNITY_END();
}
