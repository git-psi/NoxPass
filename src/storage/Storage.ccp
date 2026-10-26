#include "Storage.h"
#include <LittleFS.h>
#include <stdio.h>
#include <string.h>

namespace Storage {

namespace {
    bool mounted = false;

    void tmpName(const char* path, char* out, size_t cap) { snprintf(out, cap, "%s.tmp", path); }

    bool readFileExact(const char* path, uint8_t* buf, size_t len) {
        File f = LittleFS.open(path, FILE_READ);
        if (!f || f.isDirectory()) return false;
        bool ok = (f.size() == len) && (f.read(buf, len) == len);
        f.close();
        return ok;
    }

    // Relit path et le compare à data, par petits blocs (le fichier peut être grand).
    bool matches(const char* path, const uint8_t* data, size_t len) {
        File f = LittleFS.open(path, FILE_READ);
        if (!f || f.isDirectory() || f.size() != len) return false;
        uint8_t chunk[64];
        size_t done = 0;
        while (done < len) {
            size_t n = len - done < sizeof(chunk) ? len - done : sizeof(chunk);
            if (f.read(chunk, n) != n || memcmp(chunk, data + done, n) != 0) { f.close(); return false; }
            done += n;
        }
        f.close();
        return true;
    }
}

bool begin() {
    mounted = LittleFS.begin(true);
    return mounted;
}

ReadResult readExact(const char* path, uint8_t* buf, size_t len) {
    if (!mounted) return ReadResult::Bad;
    char tmp[48];
    tmpName(path, tmp, sizeof(tmp));

    if (!LittleFS.exists(path)) {
        if (!LittleFS.exists(tmp)) return ReadResult::Missing;
        if (!readFileExact(tmp, buf, len)) return ReadResult::Bad;
        LittleFS.rename(tmp, path);                    // on termine le remplacement interrompu
        return ReadResult::Ok;
    }
    return readFileExact(path, buf, len) ? ReadResult::Ok : ReadResult::Bad;
}

bool writeAtomic(const char* path, const uint8_t* data, size_t len) {
    if (!mounted) return false;
    char tmp[48];
    tmpName(path, tmp, sizeof(tmp));

    File f = LittleFS.open(tmp, FILE_WRITE);           // tronque s'il existe déjà
    if (!f) return false;
    bool ok = f.write(data, len) == len;
    f.close();
    if (ok) ok = matches(tmp, data, len);
    if (!ok) { LittleFS.remove(tmp); return false; }

    if (!LittleFS.rename(tmp, path)) {                 // repli si le renommage ne remplace pas
        LittleFS.remove(path);
        ok = LittleFS.rename(tmp, path);
    }
    return ok;
}

}