// Presets.h : saved usernames / emails, offered first when typing a username on the stick.
// Encrypted with the master key in /presets.bin; edited from the website only.
#pragma once
#include <stddef.h>
#include <string>
#include <vector>

namespace Presets {

    constexpr size_t MAX_COUNT = 16;
    constexpr size_t MAX_LEN   = 128;     // Same as a vault value

    // Plain printable ASCII (the stick types on a USB keyboard), 1..MAX_LEN characters.
    bool valid(const std::string& s);

    void load();                          // Reads the file once per unlock (does nothing if locked)
    size_t count();
    const std::string& at(size_t i);

    // Replaces the whole list (duplicates are dropped) and saves. false if invalid or the write failed.
    bool set(const std::vector<std::string>& list);

    void clear();                         // Wipes the list from RAM (lock)
}
