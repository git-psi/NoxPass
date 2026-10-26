// Auth.cpp : master key creation, unlock by PIN, PIN change.
#include "Auth.h"
#include "Crypto.h"
#include <string.h>

namespace Auth {

namespace {
    constexpr uint8_t  VERSION   = 1;
    constexpr size_t   SALT_LEN  = 16;
    constexpr size_t   OFF_VER   = 0;
    constexpr size_t   OFF_ITER  = 1;
    constexpr size_t   OFF_SALT  = 5;
    constexpr size_t   OFF_NONCE = 21;
    constexpr size_t   OFF_WRAP  = 33;
    constexpr size_t   OFF_TAG   = 65;
    constexpr size_t   AAD_LEN   = OFF_NONCE;           // version + iterations + salt

    // Safety bounds: a corrupted blob must not be able to block the device for hours.
    constexpr uint32_t ITER_MIN  = 1;
    constexpr uint32_t ITER_MAX  = 2000000;

    uint8_t masterKey[KEY_LEN];
    bool    unlocked = false;

    void put32(uint8_t* p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }
    uint32_t get32(const uint8_t* p)   { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }

    bool pinOk(size_t len) { return len >= Config::PIN_MIN && len <= Config::PIN_MAX; }

    // Encrypts mk with a KEK derived from pin and this salt. Fills the whole blob.
    bool wrap(const char* pin, size_t pinLen, const uint8_t mk[KEY_LEN],
              uint32_t iterations, KeyBlob& out) {
        if (iterations < ITER_MIN || iterations > ITER_MAX) return false;
        uint8_t* d = out.data;
        d[OFF_VER] = VERSION;
        put32(d + OFF_ITER, iterations);
        Crypto::randomBytes(d + OFF_SALT, SALT_LEN);        // New salt at each call
        Crypto::randomBytes(d + OFF_NONCE, Crypto::NONCE_LEN);

        uint8_t kek[KEY_LEN];
        bool ok = Crypto::pbkdf2((const uint8_t*)pin, pinLen, d + OFF_SALT, SALT_LEN, iterations, kek)
               && Crypto::gcmEncrypt(kek, d + OFF_NONCE, d, AAD_LEN, mk, KEY_LEN,
                                     d + OFF_WRAP, d + OFF_TAG);
        Crypto::wipe(kek, sizeof(kek));
        return ok;
    }
}

bool create(const char* pin, size_t pinLen, KeyBlob& out, uint32_t iterations) {
    lock();
    if (!pinOk(pinLen)) return false;

    uint8_t mk[KEY_LEN];
    Crypto::randomBytes(mk, KEY_LEN);
    bool ok = wrap(pin, pinLen, mk, iterations, out);
    if (ok) { memcpy(masterKey, mk, KEY_LEN); unlocked = true; }
    Crypto::wipe(mk, sizeof(mk));
    return ok;
}

bool unlock(const char* pin, size_t pinLen, const KeyBlob& blob) {
    lock();
    const uint8_t* d = blob.data;
    if (d[OFF_VER] != VERSION) return false;
    const uint32_t iterations = get32(d + OFF_ITER);
    if (iterations < ITER_MIN || iterations > ITER_MAX) return false;

    uint8_t kek[KEY_LEN];
    bool ok = Crypto::pbkdf2((const uint8_t*)pin, pinLen, d + OFF_SALT, SALT_LEN, iterations, kek)
           && Crypto::gcmDecrypt(kek, d + OFF_NONCE, d, AAD_LEN, d + OFF_WRAP, KEY_LEN,
                                 d + OFF_TAG, masterKey);   // Valid tag <=> correct PIN
    Crypto::wipe(kek, sizeof(kek));
    unlocked = ok;
    if (!ok) Crypto::wipe(masterKey, sizeof(masterKey));
    return ok;
}

bool changePin(const char* newPin, size_t newLen, KeyBlob& out, uint32_t iterations) {
    if (!unlocked || !pinOk(newLen)) return false;
    return wrap(newPin, newLen, masterKey, iterations, out);
}

void lock() {
    Crypto::wipe(masterKey, sizeof(masterKey));
    unlocked = false;
}

bool isUnlocked()      { return unlocked; }
const uint8_t* key()   { return unlocked ? masterKey : nullptr; }

}