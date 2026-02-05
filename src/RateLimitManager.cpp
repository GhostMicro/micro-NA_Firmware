#include "RateLimitManager.h"
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

/**
 * RateLimitManager - Token Bucket Implementation
 * 
 * Token bucket algorithm:
 * - Capacity: 100 tokens (max 100 commands/sec if refilled at right rate)
 * - Refill: +1 token every 10ms (100 tokens per 1 second)
 * - Cost: 1 token per command
 * - Rejection: Return error code, don't process command
 * 
 * @file RateLimitManager.cpp
 */

typedef struct {
    uint8_t tokens;
    uint8_t capacity;
    uint32_t lastRefillTime;
    uint32_t totalAllowed;
    uint32_t totalBlocked;
    bool initialized;
} RateLimitState;

static RateLimitState gRateLimitState = {
    .tokens = 0,
    .capacity = RATE_LIMIT_CAPACITY,
    .lastRefillTime = 0,
    .totalAllowed = 0,
    .totalBlocked = 0,
    .initialized = false
};

// Per-command limits (optional, initialized to 0 = no limit)
static struct {
    uint16_t maxPerSecond[256];
    uint32_t lastUsed[256];
} gCommandLimits = {{0}};

// ============================================================================
// Public API Implementation
// ============================================================================

bool RateLimitManager_init(uint8_t initialTokens) {
    if (initialTokens > RATE_LIMIT_CAPACITY) {
        return false;
    }
    
    gRateLimitState.tokens = initialTokens;
    gRateLimitState.capacity = RATE_LIMIT_CAPACITY;
    gRateLimitState.lastRefillTime = millis(); 
    gRateLimitState.totalAllowed = 0;
    gRateLimitState.totalBlocked = 0;
    gRateLimitState.initialized = true;
    
    // Initialize per-command limits
    memset(gCommandLimits.maxPerSecond, 0, sizeof(gCommandLimits.maxPerSecond));
    memset(gCommandLimits.lastUsed, 0, sizeof(gCommandLimits.lastUsed));
    
    return true;
}

RateLimitStatus RateLimitManager_checkCommand(uint8_t commandType) {
    if (!gRateLimitState.initialized) {
        return RATE_LIMIT_BLOCKED;
    }
    
    // Auto-refill based on time elapsed instead of manual loop call
    uint32_t now = millis();
    uint32_t elapsed = now - gRateLimitState.lastRefillTime;
    
    // 1 token every 10ms (100 tokens per sec)
    if (elapsed >= 10) {
        uint8_t refillCount = elapsed / 10;
        gRateLimitState.tokens += refillCount;
        if (gRateLimitState.tokens > gRateLimitState.capacity) {
            gRateLimitState.tokens = gRateLimitState.capacity;
        }
        gRateLimitState.lastRefillTime += refillCount * 10;
    }

    // Check global token bucket
    if (gRateLimitState.tokens == 0) {
        gRateLimitState.totalBlocked++;
        return RATE_LIMIT_EXCEEDED;
    }
    
    // Check per-command rate limit (if set)
    if (gCommandLimits.maxPerSecond[commandType] > 0) {
        // Phase 9: Implement per-command tracking
    }
    
    // Command allowed: consume one token
    gRateLimitState.tokens--;
    gRateLimitState.totalAllowed++;
    
    return RATE_LIMIT_ALLOWED;
}

uint8_t RateLimitManager_refill(void) {
    // Deprecated: Now handled automatically in checkCommand
    return gRateLimitState.tokens;
}

uint8_t RateLimitManager_getTokens(void) {
    return gRateLimitState.tokens;
}

bool RateLimitManager_setCommandLimit(uint8_t commandType, uint16_t maxPerSecond) {
    if (maxPerSecond > 1000) {
        return false;  // Max 1000 per second per command
    }
    
    gCommandLimits.maxPerSecond[commandType] = maxPerSecond;
    return true;
}

void RateLimitManager_reset(void) {
    gRateLimitState.tokens = gRateLimitState.capacity;
    gRateLimitState.lastRefillTime = 0;
    gRateLimitState.totalAllowed = 0;
    gRateLimitState.totalBlocked = 0;
}

RateLimitStats RateLimitManager_getStats(void) {
    RateLimitStats stats = {
        .currentTokens = gRateLimitState.tokens,
        .capacity = gRateLimitState.capacity,
        .totalCommandsAllowed = gRateLimitState.totalAllowed,
        .totalCommandsBlocked = gRateLimitState.totalBlocked,
        .lastRefillTime = gRateLimitState.lastRefillTime
    };
    return stats;
}
