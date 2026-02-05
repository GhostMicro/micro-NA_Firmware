#ifndef KEY_EXCHANGE_MANAGER_H
#define KEY_EXCHANGE_MANAGER_H

#include <Arduino.h>
#include <stdint.h>
#include "mbedtls/config.h"
#include "mbedtls/platform.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdh.h"

// Phase 10: Secure Key Exchange (ECDH)
// Curve: secp256r1 (NIST P-256)
// Key Size: 256 bits (32 bytes) public point coordinates

#define KEY_EXCHANGE_PUBKEY_SIZE 64 // 32 bytes X + 32 bytes Y (Raw Point)
#define KEY_EXCHANGE_SHARED_SECRET_SIZE 32
#define KEY_EXCHANGE_TIMEOUT_MS 5000 

enum KeyExchangeState {
    KX_STATE_IDLE,
    KX_STATE_GENERATING_KEYS,
    KX_STATE_WAIT_FOR_PEER_PUBKEY,
    KX_STATE_COMPUTING_SECRET,
    KX_STATE_KEY_ESTABLISHED,
    KX_STATE_FAILED
};

class KeyExchangeManager {
public:
    static KeyExchangeManager& getInstance();

    // Initialization
    bool init();
    void reset();

    // Key Generation (Step 1)
    bool generateKeyPair();
    bool getPublicKey(uint8_t* buffer); // Writes 64 bytes (X || Y)

    // Key Exchange (Step 2)
    bool computeSharedSecret(const uint8_t* peerPublicKey);
    bool getSharedSecret(uint8_t* buffer); // Writes 32 bytes

    // State Management
    KeyExchangeState getState() { return _state; }
    bool isEstablished() { return _state == KX_STATE_KEY_ESTABLISHED; }
    const char* getLastError() { return _lastError; }

private:
    KeyExchangeManager();
    
    KeyExchangeState _state;
    char _lastError[64];
    
    // mbedtls context
    mbedtls_entropy_context _entropy;
    mbedtls_ctr_drbg_context _ctr_drbg;
    mbedtls_ecdh_context _ctx;
    
    bool _initialized;
    uint8_t _sharedSecret[KEY_EXCHANGE_SHARED_SECRET_SIZE];
    
    void setError(const char* msg);
};

#endif // KEY_EXCHANGE_MANAGER_H
