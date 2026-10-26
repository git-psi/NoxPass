#pragma once
#include "Auth.h"

// Keeps the (encrypted) key blob in RAM and in /keys.bin.
namespace KeyStore {
    enum class State : uint8_t { Missing, Ok, Corrupt };

    State load();                          // Call once at boot
    State state();
    Auth::KeyBlob& blob();                 // Valid if state() == Ok
    bool save(const Auth::KeyBlob& b);     // Atomic write, then state() == Ok
}