// Vault.h : the vault. Only the list of titles is in RAM; each entry is encrypted in flash and decrypted on demand.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string>
#include <vector>

namespace Vault {

    // Field types (see Projet.md > Types)
    // The number is written in the file: never renumber without a migration (see FORMAT_VERSION).
    enum FieldType : uint8_t {
        T_USERNAME = 1,   // Username / Email
        T_PASSWORD = 2,   // Obfuscated
        T_URL      = 3,
        T_CARD     = 4,   // Obfuscated
        T_EXPIRY   = 5,
        T_CVV      = 6,   // Obfuscated
        T_NOTE     = 7,
        T_TEXT     = 8,   // Label stored
        T_SECRET   = 9,   // Label stored, obfuscated
        T_LAST     = T_SECRET
    };

    constexpr size_t   MAX_TITLE   = 32;
    constexpr size_t   MAX_VALUE   = 128;
    constexpr size_t   MAX_LABEL   = 24;
    constexpr uint8_t  MAX_FIELDS  = 16;
    constexpr uint16_t MAX_ENTRIES = 1000;
    constexpr size_t   MAX_FILE_BYTES = 1048576;    // Max size of the encrypted file

    // Why the last write was refused.
    //   Full     = no more room (number of entries, file size, flash or RAM)
    //   Write    = the flash write failed
    //   NotPlain = a non ASCII character was refused
    enum class Error : uint8_t { None, Full, Write, NotPlain };
    Error lastError();

    struct Field {
        uint8_t     type;
        std::string label;     // Only used if hasLabel(type): T_TEXT and T_SECRET
        std::string value;
    };

    struct Entry {
        uint32_t           id = 0;
        std::string        title;
        std::vector<Field> fields;
        uint32_t           lastUsed = 0;    // 0 = never used
    };

    // What stays in RAM for each entry: enough to show the list. Nothing secret here (title only).
    // The content (fields, passwords) stays encrypted in flash and is only read when the entry is opened: read().
    struct Item {
        uint32_t id;
        char     title[MAX_TITLE + 1];
        uint32_t lastUsed;       // 0 = never used
        uint32_t offset;         // Where the entry is in the file (internal)
        uint16_t len;            // Size of the encrypted entry (internal)
    };

    bool        isObfuscated(uint8_t type);
    bool        hasLabel(uint8_t type);             // The label is stored (otherwise deduced from the type)
    const char* fieldName(const Field& f);          // Name deduced from the type, or stored label (Text/Secret)

    enum class LoadResult : uint8_t { Ok, Corrupt };

    // Unlocked device required. Missing = empty vault (Ok). Unreadable or altered = Corrupt:
    // in this case NOTHING is written, to not overwrite recoverable data.
    LoadResult load();
    void       clear();                             // Clears the vault from RAM (lock)

    // List sorted by use: most recent first. Titles only.
    size_t       count();
    const Item*  at(size_t index);

    // Reads and decrypts ONE entry from flash. false if missing or unreadable.
    // The result contains secrets: wipe it with wipe() as soon as it's not needed.
    bool         read(uint32_t id, Entry& out);
    void         wipe(Entry& e);

    bool add(Entry e);                              // id assigned here; becomes the most recent entry; saves
    uint32_t lastAdded();                           // id of the last added entry (to put the focus on it)
    bool update(uint32_t id, Entry e);              // Replaces title and fields (keeps id and order); saves
    bool remove(uint32_t id);                       // Saves

    // Several changes at once, all or nothing (website). Between beginBatch() and commitBatch(),
    // add/update/remove are only queued; commitBatch() rewrites the file ONCE.
    // On failure, abortBatch() drops the queued changes (flash was not touched).
    void beginBatch();
    bool commitBatch();
    void abortBatch();
    void resort();                                  // Re-sorts the list (after a change of the sort setting)
    void markUsed(uint32_t id);                     // Only rewrites the small counters file

    // The list will focus on this entry at its next opening (only once).
    void     setFocus(uint32_t id);
    uint32_t takeFocus();

    // Entry chosen in the list, read by the detail screen.
    void     select(uint32_t id);
    uint32_t selected();
}
