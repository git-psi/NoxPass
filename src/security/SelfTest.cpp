#include "SelfTest.h"
#include "Crypto.h"
#include "Auth.h"
#include <string.h>
#include <stdio.h>

#ifdef ARDUINO
  #include <M5Unified.h>
  #include "../ui/Ui.h"
#endif

namespace {

bool hexEq(const uint8_t* got, const char* hex) {
    char buf[65];
    for (int i = 0; i < 32; i++) snprintf(buf + i * 2, 3, "%02x", got[i]);
    return strcmp(buf, hex) == 0;
}

bool pbkdf2Is(const char* pw, size_t pwLen, const char* salt, uint32_t it, const char* hex) {
    uint8_t out[32];
    return Crypto::pbkdf2((const uint8_t*)pw, pwLen, (const uint8_t*)salt, strlen(salt), it, out)
        && hexEq(out, hex);
}

}

namespace SelfTest {

const char* check() {
    // --- PBKDF2-HMAC-SHA256 : vecteurs de référence ---------------------------------
    if (!pbkdf2Is("password", 8, "salt", 1,
        "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b")) return "PBKDF2 c=1";
    if (!pbkdf2Is("password", 8, "salt", 2,
        "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43")) return "PBKDF2 c=2";
    if (!pbkdf2Is("password", 8, "salt", 4096,
        "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a")) return "PBKDF2 c=4096";
    {   // mot de passe de 100 et 64 octets (cas "clé plus longue qu'un bloc" / pile un bloc)
        char l[100]; memset(l, 'a', sizeof(l));
        if (!pbkdf2Is(l, 100, "salt", 3,
            "338c89a22703aed3e2d593f2febdd48122aedc8ee805d48b5aa2a6b2c2d3db60")) return "PBKDF2 pw100";
        if (!pbkdf2Is(l, 64, "salt", 3,
            "f8a04d441cbf044fcfcc492ce74e7b1cef6fdeaf4526d687edbc819f0300855f")) return "PBKDF2 pw64";
    }
    if (!pbkdf2Is("123456", 6, "saltsaltsaltsalt", 1000,
        "2d9904ec28cb7284c78c349b18b09767403210b7f8f85bd2b6e9d42c84492cf7")) return "PBKDF2 pin";

    // --- AES-256-GCM : aller-retour, puis toute altération doit être refusée ---------
    {
        uint8_t key[32], bad[32];
        Crypto::randomBytes(key, 32);
        memcpy(bad, key, 32); bad[7] ^= 1;
        const uint8_t aad[3] = { 1, 2, 3 };
        const char* msg = "vault test 0123456789";
        const size_t n = strlen(msg);

        uint8_t sealed[64], plain[64];
        if (!Crypto::seal(key, aad, 3, (const uint8_t*)msg, n, sealed)) return "GCM seal";
        const size_t sn = n + Crypto::SEAL_OVERHEAD;

        if (!Crypto::open(key, aad, 3, sealed, sn, plain) || memcmp(plain, msg, n)) return "GCM roundtrip";
        if (Crypto::open(bad, aad, 3, sealed, sn, plain))                           return "GCM wrong key";
        if (Crypto::open(key, aad, 2, sealed, sn, plain))                           return "GCM wrong aad";
        for (size_t i = 0; i < sn; i++) {                       // chaque octet modifié => refus
            sealed[i] ^= 0x80;
            bool accepted = Crypto::open(key, aad, 3, sealed, sn, plain);
            sealed[i] ^= 0x80;
            if (accepted) return "GCM tamper";
        }
        uint8_t s2[64];                                          // deux scellés = deux nonces
        Crypto::seal(key, aad, 3, (const uint8_t*)msg, n, s2);
        if (!memcmp(s2, sealed, Crypto::NONCE_LEN)) return "GCM nonce reuse";
    }

    // --- Auth : création, déverrouillage, mauvais PIN, changement de PIN -------------
    {
        const uint32_t IT = 500;                                 // rapide : on teste la logique
        Auth::KeyBlob blob, blob2;
        uint8_t mk[32];

        if (Auth::create("12345", 5, blob, IT))              return "Auth short PIN";
        if (!Auth::create("123456", 6, blob, IT))            return "Auth create";
        if (!Auth::isUnlocked())                             return "Auth unlocked after create";
        memcpy(mk, Auth::key(), 32);

        Auth::lock();
        if (Auth::isUnlocked() || Auth::key())               return "Auth lock";
        if (Auth::unlock("654321", 6, blob))                 return "Auth wrong PIN";
        if (Auth::isUnlocked())                              return "Auth stays locked";
        if (!Auth::unlock("123456", 6, blob))                return "Auth unlock";
        if (memcmp(mk, Auth::key(), 32))                     return "Auth same master key";

        if (!Auth::changePin("9876543", 7, blob2, IT))       return "Auth changePin";
        if (!memcmp(blob.data + 5, blob2.data + 5, 16))      return "Auth new salt";
        Auth::lock();
        if (Auth::unlock("123456", 6, blob2))                return "Auth old PIN refused";
        if (!Auth::unlock("9876543", 7, blob2))              return "Auth new PIN";
        if (memcmp(mk, Auth::key(), 32))                     return "Auth master key kept";

        Auth::lock();
        for (size_t i = 0; i < Auth::BLOB_SIZE; i++) {           // blob altéré => refus
            blob2.data[i] ^= 0x01;
            bool accepted = Auth::unlock("9876543", 7, blob2);
            blob2.data[i] ^= 0x01;
            if (accepted) return "Auth blob tamper";
        }
        Auth::lock();
    }
    return nullptr;
}

#ifdef ARDUINO
void runOnDevice() {
    const char* fail = check();
    if (fail) {
        char m[48];
        snprintf(m, sizeof(m), "FAIL %s", fail);
        Ui::showMessage(Ui::Msg::Error, m, true);
        return;
    }
    Ui::showMessage(Ui::Msg::Info, "Crypto self-test OK", true);

    // Benchmark : 3000 itérations, extrapolation linéaire.
    uint8_t salt[16] = {0}, out[32];
    const uint32_t N = 3000;
    uint32_t t0 = millis();
    Crypto::pbkdf2((const uint8_t*)"123456", 6, salt, 16, N, out);
    uint32_t ms = millis() - t0;
    if (ms == 0) ms = 1;
    uint32_t perSec = (uint32_t)((uint64_t)N * 1000 / ms);

    char m[64];
    snprintf(m, sizeof(m), "%u it/s  %u = %.1f s",
             (unsigned)perSec, (unsigned)Config::PBKDF2_ITERATIONS,
             (double)Config::PBKDF2_ITERATIONS / perSec);
    Ui::showMessage(Ui::Msg::Info, m, true);
    Serial.printf("[bench] PBKDF2 %u it/s -> %u iterations = %.2f s\n", (unsigned)perSec,
                  (unsigned)Config::PBKDF2_ITERATIONS, (double)Config::PBKDF2_ITERATIONS / perSec);
}
#endif

}