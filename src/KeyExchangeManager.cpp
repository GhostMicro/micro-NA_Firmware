#include "KeyExchangeManager.h"

KeyExchangeManager& KeyExchangeManager::getInstance() {
    static KeyExchangeManager instance;
    return instance;
}

KeyExchangeManager::KeyExchangeManager() : _state(KX_STATE_IDLE), _initialized(false) {
    memset(_lastError, 0, sizeof(_lastError));
}

bool KeyExchangeManager::init() {
    if (_initialized) return true;

    mbedtls_ecdh_init(&_ctx);
    mbedtls_ctr_drbg_init(&_ctr_drbg);
    mbedtls_entropy_init(&_entropy);

    const char *pers = "na_key_exchange";
    int ret = mbedtls_ctr_drbg_seed(&_ctr_drbg, mbedtls_entropy_func, &_entropy,
                                    (const unsigned char *)pers, strlen(pers));

    if (ret != 0) {
        setError("RNG Seed Failed");
        return false;
    }

    // Setup ECDH context for secp256r1 (NIST P-256)
    ret = mbedtls_ecdh_setup(&_ctx, MBEDTLS_ECP_DP_SECP256R1);
    if (ret != 0) {
        setError("ECDH Setup Failed");
        return false;
    }

    _initialized = true;
    return true;
}

void KeyExchangeManager::reset() {
    if (_initialized) {
        mbedtls_ecdh_free(&_ctx);
        // Re-init context for next use
        mbedtls_ecdh_init(&_ctx);
        mbedtls_ecdh_setup(&_ctx, MBEDTLS_ECP_DP_SECP256R1);
    }
    _state = KX_STATE_IDLE;
    memset(_sharedSecret, 0, KEY_EXCHANGE_SHARED_SECRET_SIZE);
}

bool KeyExchangeManager::generateKeyPair() {
    if (!_initialized && !init()) return false;

    _state = KX_STATE_GENERATING_KEYS;
    
    int ret = mbedtls_ecp_gen_keypair(&_ctx.grp, &_ctx.d, &_ctx.Q,
                                      mbedtls_ctr_drbg_random, &_ctr_drbg);
    
    if (ret != 0) {
        setError("Key Gen Failed");
        _state = KX_STATE_FAILED;
        return false;
    }

    _state = KX_STATE_WAIT_FOR_PEER_PUBKEY;
    return true;
}

bool KeyExchangeManager::getPublicKey(uint8_t* buffer) {
    if (_state == KX_STATE_IDLE || _state == KX_STATE_FAILED) return false;

    // Export public point Q to raw bytes (X || Y)
    // mbedtls stores Q as (X, Y, Z). for affine coordinates Z=1
    size_t len = 0;
    // We write uncompressed point: 0x04 || X || Y (65 bytes)
    // But our protocol expects raw 64 bytes (X || Y) to save space if we strip 0x04 header
    
    uint8_t tempBuf[65];
    int ret = mbedtls_ecp_point_write_binary(&_ctx.grp, &_ctx.Q, MBEDTLS_ECP_PF_UNCOMPRESSED, 
                                             &len, tempBuf, sizeof(tempBuf));
                                             
    if (ret != 0) {
        setError("Export PubKey Failed");
        return false;
    }

    // Skip the 0x04 header (uncompressed format marker)
    memcpy(buffer, tempBuf + 1, 64);
    return true;
}

bool KeyExchangeManager::computeSharedSecret(const uint8_t* peerPublicKey) {
    if (!_initialized) return false;
    
    _state = KX_STATE_COMPUTING_SECRET;

    // Import peer's public key
    // We need to add back the 0x04 header
    uint8_t tempBuf[65];
    tempBuf[0] = 0x04;
    memcpy(tempBuf + 1, peerPublicKey, 64);

    int ret = mbedtls_ecp_point_read_binary(&_ctx.grp, &_ctx.Qp, tempBuf, 65);
    if (ret != 0) {
        setError("Import Peer Key Failed");
        _state = KX_STATE_FAILED;
        return false;
    }

    // Compute shared secret
    size_t len = 0;
    uint8_t secretBuf[32]; // raw X coordinate of shared point
    
    ret = mbedtls_ecdh_calc_secret(&_ctx, &len, secretBuf, sizeof(secretBuf),
                                   mbedtls_ctr_drbg_random, &_ctr_drbg);

    if (ret != 0) {
        setError("Secret Compute Failed");
        _state = KX_STATE_FAILED;
        return false;
    }
    
    memcpy(_sharedSecret, secretBuf, 32);
    _state = KX_STATE_KEY_ESTABLISHED;
    
    // Free contexts to save memory as soon as we are done
    // But keep the shared secret
    // Note: In a real "Zero Trust", we might want to derive session keys immediately and wipe master secret
    // For now we store the raw secret to be passed to EncryptionManager
    
    return true;
}

bool KeyExchangeManager::getSharedSecret(uint8_t* buffer) {
    if (_state != KX_STATE_KEY_ESTABLISHED) return false;
    memcpy(buffer, _sharedSecret, KEY_EXCHANGE_SHARED_SECRET_SIZE);
    return true;
}

void KeyExchangeManager::setError(const char* msg) {
    strncpy(_lastError, msg, sizeof(_lastError) - 1);
    Serial.printf("[KeyExchange] Error: %s\n", msg);
}
