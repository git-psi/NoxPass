// WebApi.h : the JSON content exchanged with the website (knows nothing about the HTTP server: testable alone).
#pragma once
#include <string>
#include <stddef.h>
#include "../vault/Vault.h"

namespace WebApi {

    // List: {"entries":[{"id","title"}, ...],"settings":{...},"limits":{...}}: ONLY the titles,
    // sent in several chunks (vaultHead, one entryTitleJson per entry separated by commas, vaultTail).
    // The content of an entry (entryJson) is only sent on request, when opened on the website.
    std::string jsonString(const std::string& s);
    std::string vaultHead();
    std::string entryTitleJson(const Vault::Item& it);   // {"id":..,"title":..}
    std::string entryJson(const Vault::Entry& e);        // {"id","title","fields":[...]}: values in clear
    std::string vaultTail();

    // Write: {"ops":[{"op":"add|upd|del", ...}],"settings":{...}}. ALL OR NOTHING.
    // true = everything applied and saved; count = number of changes.
    // false = nothing changed in the vault (except if error = "Settings not saved"); error = readable message.
    bool apply(const char* body, size_t len, std::string& error, int& count);
}
