#ifndef RATE_LIMIT_MANAGER_H
#define RATE_LIMIT_MANAGER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * RateLimitManager - Token Bucket Rate Limiting
 * 
 * Prevents DoS attacks by limiting command rate:
 * - Max 100 commands per second
 * - Refill: +1 token every 10ms
 * - Per-command-type limits (optional)
 * 
 * @file RateLimitManager.h
 */

#define RATE_LIMIT_CAPACITY     100     // Max tokens
#define RATE_LIMIT_REFILL_PER_MS  1    // Tokens per 10ms = 100/sec
#define RATE_LIMIT_REFILL_INTERVAL  10 // Milliseconds

/**
 * Status codes for rate limit checks
 */
typedef enum {
    RATE_LIMIT_ALLOWED = 0,        // Command allowed
    RATE_LIMIT_EXCEEDED = 1,       // Rate limit exceeded
    RATE_LIMIT_COOLDOWN = 2,       // Temporary cooldown
    RATE_LIMIT_BLOCKED = 3         // Blacklist active
} RateLimitStatus;

/**
 * Initialize rate limiter
 * @param initialTokens Starting token count (usually RATE_LIMIT_CAPACITY)
 * @return true if initialization successful
 */
bool RateLimitManager_init(uint8_t initialTokens);

/**
 * Check if command is allowed under rate limit
 * @param commandType Command ID (0-255)
 * @return RATE_LIMIT_ALLOWED if allowed, else error code
 */
RateLimitStatus RateLimitManager_checkCommand(uint8_t commandType);

/**
 * Refill tokens (call every 10ms in main loop)
 * @return Current token count after refill
 */
uint8_t RateLimitManager_refill(void);

/**
 * Get current token count
 * @return Number of tokens available
 */
uint8_t RateLimitManager_getTokens(void);

/**
 * Set per-command rate limit
 * @param commandType Command ID
 * @param maxPerSecond Max calls per second
 * @return true if set successfully
 */
bool RateLimitManager_setCommandLimit(uint8_t commandType, uint16_t maxPerSecond);

/**
 * Reset rate limiter to initial state
 */
void RateLimitManager_reset(void);

/**
 * Get rate limit statistics
 */
typedef struct {
    uint8_t currentTokens;
    uint8_t capacity;
    uint32_t totalCommandsAllowed;
    uint32_t totalCommandsBlocked;
    uint32_t lastRefillTime;
} RateLimitStats;

RateLimitStats RateLimitManager_getStats(void);

#endif // RATE_LIMIT_MANAGER_H
