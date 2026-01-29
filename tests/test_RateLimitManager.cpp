/**
 * Unit Tests for RateLimitManager
 * Tests token bucket rate limiting algorithm
 * 
 * @file test_RateLimitManager.cpp
 * @framework Unity Test Framework (PlatformIO)
 */

#include <unity.h>
#include "../RateLimitManager.h"

// ============================================================================
// Test Fixtures
// ============================================================================

void setUp(void) {
    // Initialize rate limiter with full capacity
    TEST_ASSERT_TRUE(RateLimitManager_init(RATE_LIMIT_CAPACITY));
}

void tearDown(void) {
    // Reset state for next test
    RateLimitManager_reset();
}

// ============================================================================
// Initialization Tests
// ============================================================================

void test_RateLimitManager_init_success(void) {
    // Should initialize with specified tokens
    TEST_ASSERT_TRUE(RateLimitManager_init(50));
    TEST_ASSERT_EQUAL_UINT8(50, RateLimitManager_getTokens());
}

void test_RateLimitManager_init_capacity_exceeded(void) {
    // Should fail if initialTokens > capacity
    TEST_ASSERT_FALSE(RateLimitManager_init(RATE_LIMIT_CAPACITY + 1));
}

// ============================================================================
// Token Bucket Tests
// ============================================================================

void test_RateLimitManager_single_command_allowed(void) {
    // First command should be allowed
    RateLimitStatus status = RateLimitManager_checkCommand(0);
    TEST_ASSERT_EQUAL(RATE_LIMIT_ALLOWED, status);
    
    // One token should be consumed
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 1, RateLimitManager_getTokens());
}

void test_RateLimitManager_multiple_commands_allowed(void) {
    // Multiple commands should consume tokens
    for (int i = 0; i < 10; i++) {
        RateLimitStatus status = RateLimitManager_checkCommand(i % 256);
        TEST_ASSERT_EQUAL(RATE_LIMIT_ALLOWED, status);
    }
    
    // Should have 90 tokens left
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 10, RateLimitManager_getTokens());
}

void test_RateLimitManager_exhaustion_blocks_commands(void) {
    // Consume all tokens
    for (int i = 0; i < RATE_LIMIT_CAPACITY; i++) {
        RateLimitManager_checkCommand(0);
    }
    
    // Next command should be blocked
    RateLimitStatus status = RateLimitManager_checkCommand(0);
    TEST_ASSERT_EQUAL(RATE_LIMIT_EXCEEDED, status);
}

void test_RateLimitManager_refill_restores_tokens(void) {
    // Consume some tokens
    for (int i = 0; i < 10; i++) {
        RateLimitManager_checkCommand(0);
    }
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 10, RateLimitManager_getTokens());
    
    // Refill
    uint8_t tokens = RateLimitManager_refill();
    
    // Should restore 1 token (call once per 10ms)
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 9, tokens);
}

void test_RateLimitManager_refill_caps_at_capacity(void) {
    // Start with full capacity
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY, RateLimitManager_getTokens());
    
    // Refill should not exceed capacity
    uint8_t tokens = RateLimitManager_refill();
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY, tokens);
    TEST_ASSERT_LESS_THAN_OR_EQUAL(RATE_LIMIT_CAPACITY, RateLimitManager_getTokens());
}

void test_RateLimitManager_stats_tracking(void) {
    // Make some allowed requests
    RateLimitManager_checkCommand(0);
    RateLimitManager_checkCommand(1);
    RateLimitManager_checkCommand(2);
    
    RateLimitStats stats = RateLimitManager_getStats();
    
    TEST_ASSERT_EQUAL_UINT32(3, stats.totalCommandsAllowed);
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 3, stats.currentTokens);
}

void test_RateLimitManager_stats_blocked_tracking(void) {
    // Exhaust tokens
    for (int i = 0; i < RATE_LIMIT_CAPACITY; i++) {
        RateLimitManager_checkCommand(0);
    }
    
    // Try to exceed limit
    RateLimitManager_checkCommand(0);
    RateLimitManager_checkCommand(1);
    
    RateLimitStats stats = RateLimitManager_getStats();
    
    TEST_ASSERT_EQUAL_UINT32(RATE_LIMIT_CAPACITY, stats.totalCommandsAllowed);
    TEST_ASSERT_EQUAL_UINT32(2, stats.totalCommandsBlocked);
}

void test_RateLimitManager_reset_restores_state(void) {
    // Consume tokens
    for (int i = 0; i < 50; i++) {
        RateLimitManager_checkCommand(0);
    }
    
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY - 50, RateLimitManager_getTokens());
    
    // Reset
    RateLimitManager_reset();
    
    // Should restore full capacity
    TEST_ASSERT_EQUAL_UINT8(RATE_LIMIT_CAPACITY, RateLimitManager_getTokens());
    
    // Stats should also reset
    RateLimitStats stats = RateLimitManager_getStats();
    TEST_ASSERT_EQUAL_UINT32(0, stats.totalCommandsAllowed);
    TEST_ASSERT_EQUAL_UINT32(0, stats.totalCommandsBlocked);
}

// ============================================================================
// Per-Command Rate Limiting Tests
// ============================================================================

void test_RateLimitManager_set_command_limit(void) {
    // Set per-command limit
    TEST_ASSERT_TRUE(RateLimitManager_setCommandLimit(5, 10));  // Command 5: max 10/sec
    
    // Should still respect global rate limit
    RateLimitStatus status = RateLimitManager_checkCommand(5);
    TEST_ASSERT_EQUAL(RATE_LIMIT_ALLOWED, status);
}

void test_RateLimitManager_set_command_limit_invalid(void) {
    // Reject limit > 1000 per second
    TEST_ASSERT_FALSE(RateLimitManager_setCommandLimit(5, 1001));
}

// ============================================================================
// DoS Attack Simulation Tests
// ============================================================================

void test_RateLimitManager_simple_dos_attack(void) {
    // Simulate flood of commands in short time
    int blockedCount = 0;
    
    for (int i = 0; i < (RATE_LIMIT_CAPACITY + 50); i++) {
        RateLimitStatus status = RateLimitManager_checkCommand(i % 256);
        if (status != RATE_LIMIT_ALLOWED) {
            blockedCount++;
        }
    }
    
    // Should block excess commands
    TEST_ASSERT_GREATER_THAN(0, blockedCount);
    
    // Stats should reflect blocks
    RateLimitStats stats = RateLimitManager_getStats();
    TEST_ASSERT_GREATER_THAN(0, stats.totalCommandsBlocked);
}

void test_RateLimitManager_refill_prevents_sustained_dos(void) {
    // Consume capacity
    for (int i = 0; i < RATE_LIMIT_CAPACITY; i++) {
        RateLimitManager_checkCommand(0);
    }
    
    // Should be blocked
    RateLimitStatus status1 = RateLimitManager_checkCommand(0);
    TEST_ASSERT_EQUAL(RATE_LIMIT_EXCEEDED, status1);
    
    // After one refill cycle (10ms = 1 new token)
    RateLimitManager_refill();
    
    // Now one command should be allowed
    RateLimitStatus status2 = RateLimitManager_checkCommand(0);
    TEST_ASSERT_EQUAL(RATE_LIMIT_ALLOWED, status2);
}

// ============================================================================
// Test Suite Registration
// ============================================================================

void test_group_rate_limiting_setup(void) {
    RUN_TEST(test_RateLimitManager_init_success);
    RUN_TEST(test_RateLimitManager_init_capacity_exceeded);
    RUN_TEST(test_RateLimitManager_single_command_allowed);
    RUN_TEST(test_RateLimitManager_multiple_commands_allowed);
    RUN_TEST(test_RateLimitManager_exhaustion_blocks_commands);
    RUN_TEST(test_RateLimitManager_refill_restores_tokens);
    RUN_TEST(test_RateLimitManager_refill_caps_at_capacity);
    RUN_TEST(test_RateLimitManager_stats_tracking);
    RUN_TEST(test_RateLimitManager_stats_blocked_tracking);
    RUN_TEST(test_RateLimitManager_reset_restores_state);
    RUN_TEST(test_RateLimitManager_set_command_limit);
    RUN_TEST(test_RateLimitManager_set_command_limit_invalid);
    RUN_TEST(test_RateLimitManager_simple_dos_attack);
    RUN_TEST(test_RateLimitManager_refill_prevents_sustained_dos);
}
