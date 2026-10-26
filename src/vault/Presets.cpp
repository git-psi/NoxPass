// Presets.cpp : saved usernames / emails (one small encrypted file).
#include "Presets.h"
#include "../security/Auth.h"
#include "../security/Crypto.h"
#include "../storage/Storage.h"
#include <algorithm>

// /presets.bin = sealed block (nonce|cipher|tag). Plain content: count (u8), then for each preset
// length (u8) + bytes.

namespace Presets {

namespace {

constexpr const char* PATH = "/presets.bin";
const uint8_t AAD[8] = { 'P','M','P','R','E','S','0','1' };
constexpr size_t MAX_PLAIN = 1 + MAX_COUNT * (1 + MAX_LEN);

std::vector<std::string> list;
bool loaded = false;
const std::string EMPTY;

void wipeList() {
    for (std::string& s : list) if (!s.empty()) Crypto::wipe(&s[0], s.size());
    list.clear();
}

}  // namespace

bool valid(const std::string& s) {
    if (s.empty() || s.size() > MAX_LEN) return false;
    for (unsigned char c : s) if (c < 0x20 || c > 0x7E) return false;
    return true;
}

void load() {
    const uint8_t* key = Auth::key();
    if (loaded || !key) return;
    loaded = true;
    wipeList();

    const long sz = Storage::sizeOf(PATH);
    if (sz < (long)Crypto::SEAL_OVERHEAD + 1 || sz > (long)(MAX_PLAIN + Crypto::SEAL_OVERHEAD)) return;   // Missing or bad: empty
    std::vector<uint8_t> in((size_t)sz), plain((size_t)sz - Crypto::SEAL_OVERHEAD);
    if (Storage::readExact(PATH, in.data(), in.size()) != Storage::ReadResult::Ok) return;
    if (!Crypto::open(key, AAD, sizeof(AAD), in.data(), in.size(), plain.data())) return;

    size_t pos = 1;
    const size_t n = plain[0];
    for (size_t i = 0; i < n && i < MAX_COUNT && pos < plain.size(); i++) {
        const size_t len = plain[pos++];
        if (pos + len > plain.size()) break;
        std::string s((const char*)&plain[pos], len);
        pos += len;
        if (valid(s)) list.push_back(s);
    }
    Crypto::wipe(plain.data(), plain.size());
}

size_t count() { return list.size(); }

const std::string& at(size_t i) { return i < list.size() ? list[i] : EMPTY; }

bool set(const std::vector<std::string>& in) {
    const uint8_t* key = Auth::key();
    if (!key) return false;

    std::vector<std::string> next;
    for (const std::string& s : in) {
        if (!valid(s)) return false;
        if (std::find(next.begin(), next.end(), s) == next.end()) next.push_back(s);
    }
    if (next.size() > MAX_COUNT) return false;

    bool ok = true;
    if (next.empty()) {
        ok = !Storage::exists(PATH) || Storage::removeFile(PATH);
    } else {
        std::vector<uint8_t> plain;
        plain.push_back((uint8_t)next.size());
        for (const std::string& s : next) {
            plain.push_back((uint8_t)s.size());
            plain.insert(plain.end(), s.begin(), s.end());
        }
        std::vector<uint8_t> out(plain.size() + Crypto::SEAL_OVERHEAD);
        ok = Crypto::seal(key, AAD, sizeof(AAD), plain.data(), plain.size(), out.data())
          && Storage::writeAtomic(PATH, out.data(), out.size());
        Crypto::wipe(plain.data(), plain.size());
    }
    if (!ok) return false;

    wipeList();
    list = next;
    loaded = true;
    return true;
}

void clear() {
    wipeList();
    loaded = false;
}

}
