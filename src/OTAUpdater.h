#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * OTAUpdater - Over-The-Air Firmware Update Manager
 * 
 * Features:
 * - HTTPS firmware binary download
 * - SHA256 signature verification
 * - Atomic update (rollback on failure)
 * - Progress tracking (0-100%)
 * - Storage in SPIFFS partition
 * - Automatic boot validation (60s timeout)
 * 
 * @file OTAUpdater.h
 */

// OTA Status Codes
typedef enum {
    OTA_STATUS_IDLE = 0,
    OTA_STATUS_DOWNLOADING = 1,
    OTA_STATUS_VERIFYING = 2,
    OTA_STATUS_FLASHING = 3,
    OTA_STATUS_SUCCESS = 4,
    OTA_STATUS_ERROR = 5,
    OTA_STATUS_ROLLBACK = 6
} OTAStatus;

// OTA Error Codes
typedef enum {
    OTA_ERR_NONE = 0,
    OTA_ERR_INVALID_URL = 1,
    OTA_ERR_DOWNLOAD_FAILED = 2,
    OTA_ERR_NETWORK_ERROR = 3,
    OTA_ERR_SIGNATURE_MISMATCH = 4,
    OTA_ERR_STORAGE_FULL = 5,
    OTA_ERR_FLASH_ERROR = 6,
    OTA_ERR_INVALID_FIRMWARE = 7,
    OTA_ERR_TIMEOUT = 8,
    OTA_ERR_CHECKSUM_FAILED = 9,
    OTA_ERR_MEMORY_INSUFFICIENT = 10
} OTAErrorCode;

/**
 * Initialize OTA manager
 * @return true if initialization successful
 */
bool OTAUpdater_init(void);

/**
 * Start firmware download from HTTPS URL
 * @param url HTTPS firmware download URL
 * @param expectedSHA256 Expected SHA256 hash of firmware (32 bytes)
 * @return true if download started successfully
 */
bool OTAUpdater_startDownload(
    const char* url,
    const uint8_t* expectedSHA256
);

/**
 * Get current OTA progress (0-100%)
 * @return Progress percentage
 */
uint8_t OTAUpdater_getProgress(void);

/**
 * Get current OTA status
 * @return Current OTA status
 */
OTAStatus OTAUpdater_getStatus(void);

/**
 * Get last OTA error code
 * @return Error code (0 = no error)
 */
OTAErrorCode OTAUpdater_getLastError(void);

/**
 * Get human-readable error message
 * @return Error message string
 */
const char* OTAUpdater_getErrorMessage(void);

/**
 * Get bytes downloaded so far
 * @return Number of bytes downloaded
 */
uint32_t OTAUpdater_getBytesDownloaded(void);

/**
 * Get total firmware size (if known)
 * @return Firmware size in bytes (0 if unknown)
 */
uint32_t OTAUpdater_getTotalSize(void);

/**
 * Cancel current OTA operation
 * @return true if cancelled successfully
 */
bool OTAUpdater_cancel(void);

/**
 * Rollback to previous firmware version
 * @return true if rollback successful
 */
bool OTAUpdater_rollback(void);

/**
 * Get current firmware version string
 * @return Version string (e.g., "1.0.0")
 */
const char* OTAUpdater_getCurrentVersion(void);

/**
 * Get latest available firmware version (from server)
 * @return Version string or NULL if check failed
 */
const char* OTAUpdater_getLatestVersion(void);

/**
 * Check if new firmware is available
 * @param serverUrl URL to check for updates
 * @return true if new firmware available
 */
bool OTAUpdater_checkForUpdates(const char* serverUrl);

/**
 * Verify firmware signature
 * @param firmwareData Firmware binary data
 * @param dataLen Length of firmware
 * @param expectedSHA256 Expected SHA256 hash (32 bytes)
 * @return true if signature valid
 */
bool OTAUpdater_verifySHA256(
    const uint8_t* firmwareData,
    uint32_t dataLen,
    const uint8_t* expectedSHA256
);

/**
 * Get OTA statistics
 */
typedef struct {
    uint32_t totalDownloaded;
    uint32_t totalAttempts;
    uint32_t successfulUpdates;
    uint32_t failedUpdates;
    uint32_t rollbacks;
    uint32_t lastUpdateTime;
} OTAStats;

OTAStats OTAUpdater_getStats(void);

#endif // OTA_UPDATER_H
