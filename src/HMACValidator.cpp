#include "HMACValidator.h"
#include <stdio.h>
#include <string.h>

#include "mbedtls/error.h"
#include "mbedtls/md.h"

/**
 * HMACValidator - HMAC-SHA256 Implementation
 *
 * Phase 9 Full Implementation:
 * - HMAC-SHA256 authentication using mbedtls
 * - Constant-time comparison to prevent timing attacks
 *
 * @file HMACValidator.cpp
 */

static struct {
  uint8_t secret[32];
  bool initialized;
  char lastError[128];
} gHMACState = {.initialized = false, .lastError = {0}};

// ============================================================================
// Internal Helpers
// ============================================================================

static void set_last_error(const char *action, int ret) {
  char buf[128];
  mbedtls_strerror(ret, buf, sizeof(buf));
  snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
           "%s failed: -0x%04X (%s)", action, -ret, buf);
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool HMACValidator_init(const uint8_t *secret) {
  if (!secret) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "NULL secret provided");
    return false;
  }

  memcpy(gHMACState.secret, secret, 32);
  gHMACState.initialized = true;

  return true;
}

bool HMACValidator_generate(const uint8_t *data, uint16_t dataLen,
                            uint8_t *hmac) {

  if (!data || !hmac) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "NULL pointer in generate params");
    return false;
  }

  if (dataLen > HMAC_MAX_PAYLOAD) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "Payload too large: %u > %u", dataLen, HMAC_MAX_PAYLOAD);
    return false;
  }

  if (!gHMACState.initialized) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "HMAC not initialized");
    return false;
  }

  const mbedtls_md_info_t *md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info == NULL) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "SHA256 not supported in mbedtls");
    return false;
  }

  int ret =
      mbedtls_md_hmac(md_info, gHMACState.secret, 32, data, dataLen, hmac);
  if (ret != 0) {
    set_last_error("HMAC generate", ret);
    return false;
  }

  return true;
}

bool HMACValidator_validate(const uint8_t *data, uint16_t dataLen,
                            const uint8_t *receivedHmac) {

  if (!data || !receivedHmac) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "NULL pointer in validate params");
    return false;
  }

  if (!gHMACState.initialized) {
    snprintf(gHMACState.lastError, sizeof(gHMACState.lastError),
             "HMAC not initialized");
    return false;
  }

  // Generate expected HMAC
  uint8_t expectedHmac[HMAC_SHA256_SIZE];
  if (!HMACValidator_generate(data, dataLen, expectedHmac)) {
    return false;
  }

  // Compare using constant-time comparison
  return HMACValidator_constantTimeCompare(expectedHmac, receivedHmac);
}

bool HMACValidator_constantTimeCompare(const uint8_t *hmac1,
                                       const uint8_t *hmac2) {

  if (!hmac1 || !hmac2) {
    return false;
  }

  uint8_t result = 0;

  // Compare all bytes, even if first byte differs
  // Prevents timing attacks that could reveal HMAC byte-by-byte
  for (int i = 0; i < HMAC_SHA256_SIZE; i++) {
    result |= (hmac1[i] ^ hmac2[i]);
  }

  return (result == 0);
}

bool HMACValidator_isReady(void) { return gHMACState.initialized; }

void HMACValidator_reset(void) {
  memset(gHMACState.secret, 0x00, 32);
  gHMACState.initialized = false;
  memset(gHMACState.lastError, 0x00, sizeof(gHMACState.lastError));
}

const char *HMACValidator_lastError(void) {
  return (const char *)gHMACState.lastError;
}
