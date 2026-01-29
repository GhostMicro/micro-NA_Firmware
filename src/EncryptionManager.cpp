#include "EncryptionManager.h"
#include <stdio.h>
#include <string.h>

#include "mbedtls/aes.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"

/**
 * EncryptionManager - AES-256 CTR mode encryption/decryption
 *
 * Phase 9 Full Implementation:
 * - AES-256 CTR mode using hardware-accelerated mbedtls
 * - CSPRNG IV generation using mbedtls entropy + ctr_drbg
 * - PBKDF2 key derivation from passwords
 *
 * @file EncryptionManager.cpp
 */

// Internal state
static struct {
  uint8_t key[AES_256_KEY_SIZE];
  bool initialized;
  char lastError[128];

  mbedtls_entropy_context entropy;
  mbedtls_ctr_drbg_context ctr_drbg;
} gEncryptionState = {.initialized = false, .lastError = {0}};

// ============================================================================
// Internal Helpers
// ============================================================================

static void set_last_error(const char *action, int ret) {
  char buf[128];
  mbedtls_strerror(ret, buf, sizeof(buf));
  snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
           "%s failed: -0x%04X (%s)", action, -ret, buf);
}

// ============================================================================
// Public API Implementation
// ============================================================================

bool EncryptionManager_init(const uint8_t *key) {
  if (!key) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "NULL key provided");
    return false;
  }

  // Initialize random number generator
  mbedtls_entropy_init(&gEncryptionState.entropy);
  mbedtls_ctr_drbg_init(&gEncryptionState.ctr_drbg);

  const char *personalization = "NA_Framework_v1";
  int ret = mbedtls_ctr_drbg_seed(
      &gEncryptionState.ctr_drbg, mbedtls_entropy_func,
      &gEncryptionState.entropy, (const uint8_t *)personalization,
      strlen(personalization));
  if (ret != 0) {
    set_last_error("RNG Seed", ret);
    return false;
  }

  memcpy(gEncryptionState.key, key, AES_256_KEY_SIZE);
  gEncryptionState.initialized = true;

  return true;
}

bool EncryptionManager_generateIV(uint8_t *iv) {
  if (!iv) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "NULL IV buffer provided");
    return false;
  }

  if (!gEncryptionState.initialized) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "Encryption not initialized");
    return false;
  }

  int ret =
      mbedtls_ctr_drbg_random(&gEncryptionState.ctr_drbg, iv, AES_IV_SIZE);
  if (ret != 0) {
    set_last_error("Generate IV", ret);
    return false;
  }

  return true;
}

bool EncryptionManager_encrypt(const uint8_t *plaintext, uint16_t len,
                               const uint8_t *iv, uint8_t *ciphertext) {

  if (!plaintext || !iv || !ciphertext) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "NULL pointer in encrypt params");
    return false;
  }

  if (len > AES_MAX_PAYLOAD) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "Payload too large: %u > %u", len, AES_MAX_PAYLOAD);
    return false;
  }

  if (!gEncryptionState.initialized) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "Encryption not initialized");
    return false;
  }

  mbedtls_aes_context aes_ctx;
  mbedtls_aes_init(&aes_ctx);

  int ret = mbedtls_aes_setkey_enc(&aes_ctx, gEncryptionState.key, 256);
  if (ret != 0) {
    set_last_error("AES setkey", ret);
    mbedtls_aes_free(&aes_ctx);
    return false;
  }

  size_t nc_off = 0;
  uint8_t stream_block[16] = {0};
  uint8_t iv_copy[16];
  memcpy(iv_copy, iv, 16);

  ret = mbedtls_aes_crypt_ctr(&aes_ctx, len, &nc_off, iv_copy, stream_block,
                              plaintext, ciphertext);

  mbedtls_aes_free(&aes_ctx);

  if (ret != 0) {
    set_last_error("AES encrypt", ret);
    return false;
  }

  return true;
}

bool EncryptionManager_decrypt(const uint8_t *ciphertext, uint16_t len,
                               const uint8_t *iv, uint8_t *plaintext) {
  // CTR mode is symmetric: decrypt is identical to encrypt
  return EncryptionManager_encrypt(ciphertext, len, iv, plaintext);
}

bool EncryptionManager_deriveKey(const char *password, uint16_t passwordLen,
                                 const uint8_t *salt, uint32_t iterations,
                                 uint8_t *derivedKey) {

  if (!password || !salt || !derivedKey) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "NULL pointer in derive params");
    return false;
  }

  if (iterations < 10000) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "Too few iterations: %lu < 10000", iterations);
    return false;
  }

  // Use HMAC-SHA256 for PBKDF2
  mbedtls_md_context_t md_ctx;
  mbedtls_md_init(&md_ctx);

  const mbedtls_md_info_t *md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info == NULL) {
    snprintf(gEncryptionState.lastError, sizeof(gEncryptionState.lastError),
             "SHA256 not supported in mbedtls");
    mbedtls_md_free(&md_ctx);
    return false;
  }

  int ret = mbedtls_md_setup(&md_ctx, md_info, 1); // 1 = using HMAC
  if (ret != 0) {
    set_last_error("MD setup", ret);
    mbedtls_md_free(&md_ctx);
    return false;
  }

  ret = mbedtls_pkcs5_pbkdf2_hmac(&md_ctx, (const uint8_t *)password,
                                  passwordLen, salt, 16, iterations,
                                  AES_256_KEY_SIZE, derivedKey);

  mbedtls_md_free(&md_ctx);

  if (ret != 0) {
    set_last_error("PBKDF2", ret);
    return false;
  }

  return true;
}

bool EncryptionManager_isReady(void) { return gEncryptionState.initialized; }

const char *EncryptionManager_lastError(void) {
  return (const char *)gEncryptionState.lastError;
}
