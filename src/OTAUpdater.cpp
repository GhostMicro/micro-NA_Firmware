#include "OTAUpdater.h"
#include <Arduino.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <string.h>

// OTA Configuration
#define OTA_TIMEOUT_MS 300000 // 5 minutes download timeout
#define OTA_BUFFER_SIZE 4096  // 4KB download buffer
#define OTA_MAX_RETRIES 3

// Internal state
static struct {
  OTAStatus status;
  OTAErrorCode lastError;
  uint32_t bytesDownloaded;
  uint32_t totalSize;
  uint8_t progress;
  uint32_t downloadStartTime;
  uint8_t expectedSHA256[32];
  bool initialized;

  // Statistics
  uint32_t totalAttempts;
  uint32_t successfulUpdates;
  uint32_t failedUpdates;
  uint32_t rollbacks;
  uint32_t lastUpdateTime;
} otaState = {.status = OTA_STATUS_IDLE,
              .lastError = OTA_ERR_NONE,
              .bytesDownloaded = 0,
              .totalSize = 0,
              .progress = 0,
              .downloadStartTime = 0,
              .initialized = false,
              .totalAttempts = 0,
              .successfulUpdates = 0,
              .failedUpdates = 0,
              .rollbacks = 0,
              .lastUpdateTime = 0};

// Internal error logging
static void logOTAEvent(const char *event, const char *details) {
  Serial.printf("[OTA] %s: %s\n", event, details ? details : "");
}

/**
 * Initialize OTA manager
 */
bool OTAUpdater_init(void) {
  if (otaState.initialized)
    return true;

  // Note: SPIFFS is used for general storage, OTA uses a dedicated partition
  // via Update library
  otaState.initialized = true;
  logOTAEvent("INIT", "OTA manager ready");
  return true;
}

/**
 * Start firmware download and update process
 */
bool OTAUpdater_startDownload(const char *url, const uint8_t *expectedSHA256) {
  if (!otaState.initialized)
    return false;
  if (!url) {
    otaState.lastError = OTA_ERR_INVALID_URL;
    return false;
  }

  otaState.status = OTA_STATUS_DOWNLOADING;
  otaState.bytesDownloaded = 0;
  otaState.progress = 0;
  otaState.downloadStartTime = millis();
  otaState.lastError = OTA_ERR_NONE;
  otaState.totalAttempts++;

  if (expectedSHA256) {
    memcpy(otaState.expectedSHA256, expectedSHA256, 32);
  } else {
    memset(otaState.expectedSHA256, 0, 32); // No verification if NULL
  }

  logOTAEvent("START", url);

  HTTPClient http;
  // For Phase 9, we disable certificate validation for easier development
  // In production, we should provide the root CA
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    logOTAEvent("ERROR", "HTTP GET failed");
    otaState.status = OTA_STATUS_ERROR;
    otaState.lastError = OTA_ERR_DOWNLOAD_FAILED;
    otaState.failedUpdates++;
    http.end();
    return false;
  }

  otaState.totalSize = http.getSize();
  if (otaState.totalSize == 0) {
    logOTAEvent("ERROR", "Invalid content length");
    otaState.status = OTA_STATUS_ERROR;
    otaState.lastError = OTA_ERR_INVALID_FIRMWARE;
    http.end();
    return false;
  }

  if (!Update.begin(otaState.totalSize)) {
    logOTAEvent("ERROR", "Update.begin failed");
    otaState.status = OTA_STATUS_ERROR;
    otaState.lastError = OTA_ERR_FLASH_ERROR;
    http.end();
    return false;
  }

  WiFiClient *stream = http.getStreamPtr();
  uint8_t buffer[OTA_BUFFER_SIZE];

  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0); // 0 = SHA256

  uint32_t writtenTotal = 0;

  while (http.connected() && (writtenTotal < otaState.totalSize)) {
    size_t size = stream->available();
    if (size > 0) {
      int c =
          stream->readBytes(buffer, min((size_t)size, (size_t)OTA_BUFFER_SIZE));

      if (Update.write(buffer, c) != (size_t)c) {
        logOTAEvent("ERROR", "Flash write mismatch");
        otaState.status = OTA_STATUS_ERROR;
        otaState.lastError = OTA_ERR_FLASH_ERROR;
        break;
      }

      mbedtls_sha256_update(&sha_ctx, buffer, c);
      writtenTotal += c;
      otaState.bytesDownloaded = writtenTotal;
      otaState.progress = (uint8_t)((writtenTotal * 100) / otaState.totalSize);

      // Periodically log progress (or handle in loop)
      if (otaState.progress % 10 == 0) {
        yield(); // Feed watchdog
      }
    }

    if (millis() - otaState.downloadStartTime > OTA_TIMEOUT_MS) {
      logOTAEvent("ERROR", "Download timeout");
      otaState.status = OTA_STATUS_ERROR;
      otaState.lastError = OTA_ERR_TIMEOUT;
      break;
    }
  }

  uint8_t calculatedHash[32];
  mbedtls_sha256_finish(&sha_ctx, calculatedHash);
  mbedtls_sha256_free(&sha_ctx);

  if (otaState.status == OTA_STATUS_ERROR) {
    Update.abort();
    http.end();
    otaState.failedUpdates++;
    return false;
  }

  otaState.status = OTA_STATUS_VERIFYING;

  // Verify SHA256 if expected hash was provided
  bool hashValid = true;
  bool hasExpectedHash = false;
  for (int i = 0; i < 32; i++)
    if (otaState.expectedSHA256[i] != 0)
      hasExpectedHash = true;

  if (hasExpectedHash) {
    if (!OTAUpdater_verifySHA256(
            NULL, 0,
            otaState.expectedSHA256)) { // This is just a placeholder call
      // Comparison
      int mismatch = 0;
      for (int i = 0; i < 32; i++)
        mismatch |= (calculatedHash[i] ^ otaState.expectedSHA256[i]);
      if (mismatch != 0)
        hashValid = false;
    }
  }

  if (!hashValid) {
    logOTAEvent("ERROR", "SHA256 mismatch");
    otaState.status = OTA_STATUS_ERROR;
    otaState.lastError = OTA_ERR_SIGNATURE_MISMATCH;
    Update.abort();
    http.end();
    otaState.failedUpdates++;
    return false;
  }

  otaState.status = OTA_STATUS_FLASHING;
  if (Update.end(true)) {
    if (Update.isFinished()) {
      logOTAEvent("SUCCESS", "OTA update finished");
      otaState.status = OTA_STATUS_SUCCESS;
      otaState.successfulUpdates++;
      otaState.lastUpdateTime = millis();
      http.end();
      return true;
    }
  }

  logOTAEvent("ERROR", "Update.end failed");
  otaState.status = OTA_STATUS_ERROR;
  otaState.lastError = OTA_ERR_FLASH_ERROR;
  otaState.failedUpdates++;
  http.end();
  return false;
}

uint8_t OTAUpdater_getProgress(void) { return otaState.progress; }
OTAStatus OTAUpdater_getStatus(void) { return otaState.status; }
OTAErrorCode OTAUpdater_getLastError(void) { return otaState.lastError; }
uint32_t OTAUpdater_getBytesDownloaded(void) {
  return otaState.bytesDownloaded;
}
uint32_t OTAUpdater_getTotalSize(void) { return otaState.totalSize; }

bool OTAUpdater_cancel(void) {
  if (otaState.status == OTA_STATUS_DOWNLOADING) {
    otaState.status = OTA_STATUS_IDLE;
    Update.abort();
    return true;
  }
  return false;
}

bool OTAUpdater_rollback(void) {
  if (Update.canRollBack()) {
    if (Update.rollBack()) {
      otaState.rollbacks++;
      otaState.status = OTA_STATUS_ROLLBACK;
      return true;
    }
  }
  return false;
}

const char *OTAUpdater_getCurrentVersion(void) { return "1.1.0-security"; }
const char *OTAUpdater_getLatestVersion(void) { return "1.1.0-security"; }

bool OTAUpdater_checkForUpdates(const char *serverUrl) {
  // Phase 9: Return false (Manual update via URL preferred)
  return false;
}

bool OTAUpdater_verifySHA256(const uint8_t *firmwareData, uint32_t dataLen,
                             const uint8_t *expectedSHA256) {
  if (!firmwareData || !expectedSHA256)
    return false;
  uint8_t hash[32];
  mbedtls_sha256_ret(firmwareData, dataLen, hash, 0);
  int mismatch = 0;
  for (int i = 0; i < 32; i++)
    mismatch |= (hash[i] ^ expectedSHA256[i]);
  return mismatch == 0;
}

OTAStats OTAUpdater_getStats(void) {
  return (OTAStats){.totalDownloaded = otaState.bytesDownloaded,
                    .totalAttempts = otaState.totalAttempts,
                    .successfulUpdates = otaState.successfulUpdates,
                    .failedUpdates = otaState.failedUpdates,
                    .rollbacks = otaState.rollbacks,
                    .lastUpdateTime = otaState.lastUpdateTime};
}

const char *OTAUpdater_getErrorMessage(void) {
  switch (otaState.lastError) {
  case OTA_ERR_NONE:
    return "None";
  case OTA_ERR_INVALID_URL:
    return "Invalid URL";
  case OTA_ERR_DOWNLOAD_FAILED:
    return "Download failed";
  case OTA_ERR_SIGNATURE_MISMATCH:
    return "SHA256 mismatch";
  case OTA_ERR_FLASH_ERROR:
    return "Flash error";
  case OTA_ERR_TIMEOUT:
    return "Timeout";
  default:
    return "Unknown";
  }
}
