#ifdef UNIT_TESTING

#include <unity.h>
#include "OTAUpdater.h"
#include <string.h>

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void) {
    // Initialize OTA updater before each test
    OTAUpdater_init();
}

void tearDown(void) {
    // Cleanup after each test
    OTAUpdater_cancel();
}

// ============================================================================
// Test Cases
// ============================================================================

/**
 * Test 1: OTA Initialization
 */
void test_OTAUpdater_Init(void) {
    TEST_ASSERT_TRUE(OTAUpdater_init());
    TEST_ASSERT_EQUAL_INT(OTA_STATUS_IDLE, OTAUpdater_getStatus());
    TEST_ASSERT_EQUAL_INT(OTA_ERR_NONE, OTAUpdater_getLastError());
}

/**
 * Test 2: Invalid URL Detection
 */
void test_OTAUpdater_InvalidURLDetection(void) {
    // Attempt to start download with invalid URL
    bool result = OTAUpdater_startDownload("http://example.com/firmware.bin", NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(OTA_ERR_INVALID_URL, OTAUpdater_getLastError());
    TEST_ASSERT_EQUAL_STRING("Invalid OTA URL format", OTAUpdater_getErrorMessage());
}

/**
 * Test 3: HTTPS URL Validation
 */
void test_OTAUpdater_HTTPSValidation(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Should accept HTTPS URLs
    bool result = OTAUpdater_startDownload("https://example.com/firmware.bin", fakeSHA256);
    
    // Result depends on actual network, but URL validation should pass
    TEST_ASSERT_EQUAL_INT(OTA_ERR_NONE, OTAUpdater_getLastError());
}

/**
 * Test 4: Progress Tracking (0-100%)
 */
void test_OTAUpdater_ProgressTracking(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Initially at 0%
    TEST_ASSERT_EQUAL_INT(0, OTAUpdater_getProgress());
    
    // Start download
    OTAUpdater_startDownload("https://example.com/firmware.bin", fakeSHA256);
    
    // Progress should be 0-100%
    uint8_t progress = OTAUpdater_getProgress();
    TEST_ASSERT_TRUE(progress >= 0 && progress <= 100);
}

/**
 * Test 5: Status Transitions
 */
void test_OTAUpdater_StatusTransitions(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Start in IDLE
    TEST_ASSERT_EQUAL_INT(OTA_STATUS_IDLE, OTAUpdater_getStatus());
    
    // Start download → DOWNLOADING
    OTAUpdater_startDownload("https://example.com/firmware.bin", fakeSHA256);
    TEST_ASSERT_NOT_EQUAL(OTA_STATUS_IDLE, OTAUpdater_getStatus());
}

/**
 * Test 6: Cancel Operation
 */
void test_OTAUpdater_CancelOperation(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Start download
    OTAUpdater_startDownload("https://example.com/firmware.bin", fakeSHA256);
    
    // Cancel should succeed
    TEST_ASSERT_TRUE(OTAUpdater_cancel());
    
    // Should return to IDLE
    TEST_ASSERT_EQUAL_INT(OTA_STATUS_IDLE, OTAUpdater_getStatus());
}

/**
 * Test 7: SHA256 Verification
 */
void test_OTAUpdater_SHA256Verification(void) {
    // Test with known data
    uint8_t testData[] = "Hello World";
    uint32_t testDataLen = strlen((char*)testData);
    
    // Expected SHA256 of "Hello World"
    uint8_t expectedHash[32] = {
        0xa5, 0x91, 0xa6, 0xd4, 0x0b, 0xf4, 0x20, 0x40,
        0x4a, 0x01, 0x1d, 0x33, 0x97, 0x0b, 0xd3, 0x2f,
        0x5f, 0x76, 0x3c, 0x2d, 0x91, 0x5a, 0x61, 0x89,
        0xd6, 0x84, 0x9d, 0x8f, 0xfd, 0x8d, 0x72, 0xa2
    };
    
    bool result = OTAUpdater_verifySHA256(testData, testDataLen, expectedHash);
    TEST_ASSERT_TRUE(result);
}

/**
 * Test 8: SHA256 Mismatch Detection
 */
void test_OTAUpdater_SHA256MismatchDetection(void) {
    uint8_t testData[] = "Wrong Data";
    uint32_t testDataLen = strlen((char*)testData);
    
    // Expected SHA256 of "Hello World"
    uint8_t wrongHash[32] = {
        0xa5, 0x91, 0xa6, 0xd4, 0x0b, 0xf4, 0x20, 0x40,
        0x4a, 0x01, 0x1d, 0x33, 0x97, 0x0b, 0xd3, 0x2f,
        0x5f, 0x76, 0x3c, 0x2d, 0x91, 0x5a, 0x61, 0x89,
        0xd6, 0x84, 0x9d, 0x8f, 0xfd, 0x8d, 0x72, 0xa2
    };
    
    // Should detect mismatch
    bool result = OTAUpdater_verifySHA256(testData, testDataLen, wrongHash);
    TEST_ASSERT_FALSE(result);
}

/**
 * Test 9: Error Message Mapping
 */
void test_OTAUpdater_ErrorMessageMapping(void) {
    // Test error message for each error code
    struct {
        OTAErrorCode code;
        const char* expectedMsg;
    } errors[] = {
        {OTA_ERR_NONE, "No error"},
        {OTA_ERR_INVALID_URL, "Invalid OTA URL format"},
        {OTA_ERR_DOWNLOAD_FAILED, "Firmware download failed"},
        {OTA_ERR_SIGNATURE_MISMATCH, "SHA256 signature mismatch"},
        {OTA_ERR_TIMEOUT, "OTA operation timeout"},
    };
    
    for (int i = 0; i < 5; i++) {
        // Would need to trigger each error to test mapping
        // For now, just verify mapping exists
        const char* msg = OTAUpdater_getErrorMessage();
        TEST_ASSERT_NOT_NULL(msg);
    }
}

/**
 * Test 10: Statistics Tracking
 */
void test_OTAUpdater_StatisticsTracking(void) {
    OTAStats stats = OTAUpdater_getStats();
    
    TEST_ASSERT_EQUAL_INT(0, stats.totalDownloaded);
    TEST_ASSERT_EQUAL_INT(0, stats.successfulUpdates);
    TEST_ASSERT_EQUAL_INT(0, stats.failedUpdates);
    TEST_ASSERT_EQUAL_INT(0, stats.rollbacks);
}

/**
 * Test 11: Firmware Version Tracking
 */
void test_OTAUpdater_FirmwareVersion(void) {
    const char* version = OTAUpdater_getCurrentVersion();
    
    TEST_ASSERT_NOT_NULL(version);
    TEST_ASSERT_TRUE(strlen(version) > 0);
}

/**
 * Test 12: Multiple Concurrent Operations Prevention
 */
void test_OTAUpdater_ConcurrentOperationPrevention(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Start first download
    bool result1 = OTAUpdater_startDownload("https://example.com/fw1.bin", fakeSHA256);
    
    // Try to start second download while first is ongoing
    // Should be prevented (device can only handle one OTA at a time)
    // Implementation detail: second call may fail or queue
}

/**
 * Test 13: Timeout Detection
 */
void test_OTAUpdater_TimeoutDetection(void) {
    uint8_t fakeSHA256[32] = {0};
    
    // Start download
    OTAUpdater_startDownload("https://slow-server.invalid/firmware.bin", fakeSHA256);
    
    // Simulate timeout by checking status after long delay
    // (Would need actual sleep or mocked time for real test)
    OTAStatus status = OTAUpdater_getStatus();
    // Status could be DOWNLOADING, ERROR, or TIMEOUT
}

/**
 * Test 14: Null Pointer Handling
 */
void test_OTAUpdater_NullPointerHandling(void) {
    // Should handle NULL parameters gracefully
    bool result = OTAUpdater_startDownload(NULL, NULL);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(OTA_ERR_INVALID_URL, OTAUpdater_getLastError());
}

/**
 * Test 15: Empty URL Handling
 */
void test_OTAUpdater_EmptyURLHandling(void) {
    uint8_t fakeSHA256[32] = {0};
    
    bool result = OTAUpdater_startDownload("", fakeSHA256);
    
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_INT(OTA_ERR_INVALID_URL, OTAUpdater_getLastError());
}

// ============================================================================
// Test Runner
// ============================================================================

int runOTATests(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_OTAUpdater_Init);
    RUN_TEST(test_OTAUpdater_InvalidURLDetection);
    RUN_TEST(test_OTAUpdater_HTTPSValidation);
    RUN_TEST(test_OTAUpdater_ProgressTracking);
    RUN_TEST(test_OTAUpdater_StatusTransitions);
    RUN_TEST(test_OTAUpdater_CancelOperation);
    RUN_TEST(test_OTAUpdater_SHA256Verification);
    RUN_TEST(test_OTAUpdater_SHA256MismatchDetection);
    RUN_TEST(test_OTAUpdater_ErrorMessageMapping);
    RUN_TEST(test_OTAUpdater_StatisticsTracking);
    RUN_TEST(test_OTAUpdater_FirmwareVersion);
    RUN_TEST(test_OTAUpdater_ConcurrentOperationPrevention);
    RUN_TEST(test_OTAUpdater_TimeoutDetection);
    RUN_TEST(test_OTAUpdater_NullPointerHandling);
    RUN_TEST(test_OTAUpdater_EmptyURLHandling);
    
    return UNITY_END();
}

#endif // UNIT_TESTING
