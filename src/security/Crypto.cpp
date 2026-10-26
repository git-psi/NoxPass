// Crypto.cpp : PBKDF2-HMAC-SHA256, AES-256-GCM, SHA-256 and random (mbedTLS).
#include "Crypto.h"
#include <string.h>
#include "mbedtls/md.h"
#include "mbedtls/gcm.h"
#include "mbedtls/platform_util.h"

#if defined(ESP_PLATFORM)
  #include <esp_random.h>
  #if __has_include(<bootloader_random.h>)
    #include <bootloader_random.h>
    #define HAVE_BOOTLOADER_RANDOM 1
  #else
    #warning "bootloader_random.h introuvable : sans Wi-Fi/BT le RNG peut ne pas etre un vrai aleatoire"
  #endif
#else
  #include <stdio.h>      // PC test build only
#endif

namespace Crypto {

void wipe(void* p, size_t n) { mbedtls_platform_zeroize(p, n); }

void randomBytes(uint8_t* out, size_t n) {
#if defined(ESP_PLATFORM)
    // Without active radio, the ESP32 RNG is a real TRNG only with this entropy source.
  #ifdef HAVE_BOOTLOADER_RANDOM
    bootloader_random_enable();
  #endif
    esp_fill_random(out, n);
  #ifdef HAVE_BOOTLOADER_RANDOM
    bootloader_random_disable();
  #endif
#else
    FILE* f = fopen("/dev/urandom", "rb");
    size_t r = f ? fread(out, 1, n, f) : 0;
    if (f) fclose(f);
    (void)r;
#endif
}

// ---------------------------------------------------------------------------
// HMAC-SHA256 with precomputed ipad/opad states: each call only costs
// 2 SHA-256 compressions (instead of 4 with mbedtls_md_hmac_reset).
// ---------------------------------------------------------------------------
namespace {

struct Hmac {
    mbedtls_md_context_t inner0, outer0, a, b;

    Hmac() {
        mbedtls_md_init(&inner0); mbedtls_md_init(&outer0);
        mbedtls_md_init(&a);      mbedtls_md_init(&b);
    }
    ~Hmac() {
        mbedtls_md_free(&inner0); mbedtls_md_free(&outer0);
        mbedtls_md_free(&a);      mbedtls_md_free(&b);
    }

    bool init(const uint8_t* key, size_t keyLen) {
        const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
        if (!info) return false;

        uint8_t k[64] = {0}, pad[64];
        bool ok = true;
        if (keyLen > 64) ok = (mbedtls_md(info, key, keyLen, k) == 0);   // Key > block: hash it
        else if (keyLen) memcpy(k, key, keyLen);

        ok = ok
          && mbedtls_md_setup(&inner0, info, 0) == 0 && mbedtls_md_setup(&outer0, info, 0) == 0
          && mbedtls_md_setup(&a, info, 0) == 0      && mbedtls_md_setup(&b, info, 0) == 0;

        if (ok) {
            for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x36;
            ok = mbedtls_md_starts(&inner0) == 0 && mbedtls_md_update(&inner0, pad, 64) == 0;
        }
        if (ok) {
            for (int i = 0; i < 64; i++) pad[i] = k[i] ^ 0x5c;
            ok = mbedtls_md_starts(&outer0) == 0 && mbedtls_md_update(&outer0, pad, 64) == 0;
        }
        wipe(k, sizeof(k));
        wipe(pad, sizeof(pad));
        return ok;
    }

    // out = HMAC(key, p1 || p2). out can be the same as p1.
    bool run(const uint8_t* p1, size_t l1, const uint8_t* p2, size_t l2, uint8_t out[32]) {
        uint8_t h[32];
        bool ok = mbedtls_md_clone(&a, &inner0) == 0
               && mbedtls_md_update(&a, p1, l1) == 0
               && (l2 == 0 || mbedtls_md_update(&a, p2, l2) == 0)
               && mbedtls_md_finish(&a, h) == 0
               && mbedtls_md_clone(&b, &outer0) == 0
               && mbedtls_md_update(&b, h, 32) == 0
               && mbedtls_md_finish(&b, out) == 0;
        wipe(h, sizeof(h));
        return ok;
    }
};

} // namespace

bool pbkdf2(const uint8_t* pw, size_t pwLen, const uint8_t* salt, size_t saltLen,
            uint32_t iterations, uint8_t out[KEY_LEN]) {
    if (iterations == 0 || !salt || saltLen == 0) return false;

    Hmac h;
    if (!h.init(pw, pwLen)) return false;

    uint8_t u[32], t[32];
    const uint8_t blockIndex[4] = { 0, 0, 0, 1 };      // A single output block (32 bytes)

    bool ok = h.run(salt, saltLen, blockIndex, 4, u);
    if (ok) {
        memcpy(t, u, 32);
        for (uint32_t i = 1; i < iterations && ok; i++) {
            ok = h.run(u, 32, nullptr, 0, u);
            for (int j = 0; j < 32; j++) t[j] ^= u[j];
        }
    }
    if (ok) memcpy(out, t, 32);
    wipe(u, sizeof(u));
    wipe(t, sizeof(t));
    return ok;
}

// ---------------------------------------------------------------------------
// AES-256-GCM
// ---------------------------------------------------------------------------
bool gcmEncrypt(const uint8_t key[KEY_LEN], const uint8_t nonce[NONCE_LEN],
                const uint8_t* aad, size_t aadLen,
                const uint8_t* in, size_t len, uint8_t* out, uint8_t tag[TAG_LEN]) {
    mbedtls_gcm_context g;
    mbedtls_gcm_init(&g);
    bool ok = mbedtls_gcm_setkey(&g, MBEDTLS_CIPHER_ID_AES, key, 256) == 0
           && mbedtls_gcm_crypt_and_tag(&g, MBEDTLS_GCM_ENCRYPT, len, nonce, NONCE_LEN,
                                        aad, aadLen, in, out, TAG_LEN, tag) == 0;
    mbedtls_gcm_free(&g);
    return ok;
}

bool gcmDecrypt(const uint8_t key[KEY_LEN], const uint8_t nonce[NONCE_LEN],
                const uint8_t* aad, size_t aadLen,
                const uint8_t* in, size_t len, const uint8_t tag[TAG_LEN], uint8_t* out) {
    mbedtls_gcm_context g;
    mbedtls_gcm_init(&g);
    bool ok = mbedtls_gcm_setkey(&g, MBEDTLS_CIPHER_ID_AES, key, 256) == 0
           && mbedtls_gcm_auth_decrypt(&g, len, nonce, NONCE_LEN,
                                       aad, aadLen, tag, TAG_LEN, in, out) == 0;
    mbedtls_gcm_free(&g);
    return ok;
}

bool seal(const uint8_t key[KEY_LEN], const uint8_t* aad, size_t aadLen,
          const uint8_t* in, size_t inLen, uint8_t* out) {
    randomBytes(out, NONCE_LEN);
    return gcmEncrypt(key, out, aad, aadLen, in, inLen,
                      out + NONCE_LEN, out + NONCE_LEN + inLen);
}

bool open(const uint8_t key[KEY_LEN], const uint8_t* aad, size_t aadLen,
          const uint8_t* in, size_t inLen, uint8_t* out) {
    if (inLen < SEAL_OVERHEAD) return false;
    const size_t n = inLen - SEAL_OVERHEAD;
    return gcmDecrypt(key, in, aad, aadLen, in + NONCE_LEN, n, in + NONCE_LEN + n, out);
}

// --- Streaming SHA-256 -------------------------------------------------------
void* sha256Begin() {
    mbedtls_md_context_t* c = new mbedtls_md_context_t;
    mbedtls_md_init(c);
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!info || mbedtls_md_setup(c, info, 0) != 0 || mbedtls_md_starts(c) != 0) {
        mbedtls_md_free(c); delete c; return nullptr;
    }
    return c;
}

void sha256Update(void* h, const uint8_t* data, size_t n) {
    if (h) mbedtls_md_update((mbedtls_md_context_t*)h, data, n);
}

void sha256End(void* h, uint8_t out[32]) {
    mbedtls_md_context_t* c = (mbedtls_md_context_t*)h;
    if (!c) { for (int i = 0; i < 32; i++) out[i] = 0; return; }
    mbedtls_md_finish(c, out);
    mbedtls_md_free(c);
    delete c;
}

}
