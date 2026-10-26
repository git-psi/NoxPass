// Auth.h : key hierarchy   PIN -> PBKDF2 -> KEK -> master key -> vault
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../config.h"

namespace Auth {
    constexpr size_t KEY_LEN   = 32;
    constexpr size_t BLOB_SIZE = 81;

    // Master key encrypted by the KEK, ready to be written as is in flash.
    // Layout: version(1) | iterations u32 LE(4) | salt(16) | nonce(12) | encrypted key(32) | tag(16)
    // The first 21 bytes are authenticated (AAD): modifying them invalidates the tag.
    struct KeyBlob { uint8_t data[BLOB_SIZE]; };

    // First launch: generates a random master key, protects it with the PIN.
    // Leaves the device unlocked. false if the PIN doesn't respect PIN_MIN..PIN_MAX.
    bool create(const char* pin, size_t pinLen, KeyBlob& out,
                uint32_t iterations = Config::PBKDF2_ITERATIONS);

    // true if the PIN is correct (valid GCM tag); the master key is then loaded in RAM.
    bool unlock(const char* pin, size_t pinLen, const KeyBlob& blob);

    // Unlocked device required. New salt, new KEK, SAME master key:
    // the vault is not touched. The caller writes the new blob in flash.
    bool changePin(const char* newPin, size_t newLen, KeyBlob& out,
                   uint32_t iterations = Config::PBKDF2_ITERATIONS);

    void lock();                  // Wipes the master key from RAM
    bool isUnlocked();
    const uint8_t* key();         // Master key (32 bytes), nullptr if locked
}