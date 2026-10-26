// Keystore.cpp : the encrypted master key, kept in RAM and in /keys.bin.
#include "KeyStore.h"
#include "../storage/Storage.h"
#include <string.h>

namespace KeyStore {

static Auth::KeyBlob cached;
static State st = State::Missing;

State load() {
    switch (Storage::readExact(Storage::KEYS_PATH, cached.data, Auth::BLOB_SIZE)) {
        case Storage::ReadResult::Ok:      st = State::Ok;      break;
        case Storage::ReadResult::Missing: st = State::Missing; break;
        default:                           st = State::Corrupt; break;
    }
    return st;
}

State state() { return st; }
Auth::KeyBlob& blob() { return cached; }

bool save(const Auth::KeyBlob& b) {
    if (!Storage::writeAtomic(Storage::KEYS_PATH, b.data, Auth::BLOB_SIZE)) return false;
    memcpy(cached.data, b.data, Auth::BLOB_SIZE);
    st = State::Ok;
    return true;
}

}