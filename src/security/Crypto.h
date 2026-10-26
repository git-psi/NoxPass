// Crypto.h : primitives only (no notion of PIN or vault here).
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Crypto {
    constexpr size_t KEY_LEN       = 32;                       // AES-256 / PBKDF2 output
    constexpr size_t NONCE_LEN     = 12;                       // GCM 96 bits
    constexpr size_t TAG_LEN       = 16;
    constexpr size_t SEAL_OVERHEAD = NONCE_LEN + TAG_LEN;      // 28 bytes

    void randomBytes(uint8_t* out, size_t n);
    void wipe(void* p, size_t n);                              // Non optimizable wipe

    // PBKDF2-HMAC-SHA256, 32 bytes output. false if invalid parameters.
    bool pbkdf2(const uint8_t* pw, size_t pwLen,
                const uint8_t* salt, size_t saltLen,
                uint32_t iterations, uint8_t out[KEY_LEN]);

    // Raw AES-256-GCM: the nonce is provided by the caller (never reused with the same key).
    bool gcmEncrypt(const uint8_t key[KEY_LEN], const uint8_t nonce[NONCE_LEN],
                    const uint8_t* aad, size_t aadLen,
                    const uint8_t* in, size_t len, uint8_t* out, uint8_t tag[TAG_LEN]);
    // false if the tag is invalid (wrong key or modified data): out is then wiped.
    bool gcmDecrypt(const uint8_t key[KEY_LEN], const uint8_t nonce[NONCE_LEN],
                    const uint8_t* aad, size_t aadLen,
                    const uint8_t* in, size_t len, const uint8_t tag[TAG_LEN], uint8_t* out);

    // Streaming SHA-256 (to authenticate a file read/written in chunks). h comes from sha256Begin().
    void* sha256Begin();                                       // nullptr on failure
    void  sha256Update(void* h, const uint8_t* data, size_t n);
    void  sha256End(void* h, uint8_t out[32]);                 // Frees h

    // Sealed format: nonce(12) || cipher || tag(16). Fresh random nonce at each call.
    // out must be inLen + SEAL_OVERHEAD bytes. in and out must be distinct.
    bool seal(const uint8_t key[KEY_LEN], const uint8_t* aad, size_t aadLen,
              const uint8_t* in, size_t inLen, uint8_t* out);
    // inLen = size of the sealed block. Writes inLen - SEAL_OVERHEAD bytes in out.
    bool open(const uint8_t key[KEY_LEN], const uint8_t* aad, size_t aadLen,
              const uint8_t* in, size_t inLen, uint8_t* out);
}