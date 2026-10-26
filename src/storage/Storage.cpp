// Storage.cpp : files in flash memory (LittleFS), with atomic write.
#include "Storage.h"
#include <LittleFS.h>
#ifdef ARDUINO_ARCH_ESP32
#include <Arduino.h>
#endif
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

    // Reads path back and compares it with data, in small blocks (the file can be big).
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

long sizeOf(const char* path) {
    if (!mounted) return -2;
    char tmp[48];
    tmpName(path, tmp, sizeof(tmp));
    const char* p = path;
    if (!LittleFS.exists(path)) {
        if (!LittleFS.exists(tmp)) return -1;
        p = tmp;
    }
    File f = LittleFS.open(p, FILE_READ);
    if (!f || f.isDirectory()) return -2;
    long n = (long)f.size();
    f.close();
    return n;
}

ReadResult readExact(const char* path, uint8_t* buf, size_t len) {
    if (!mounted) return ReadResult::Bad;
    char tmp[48];
    tmpName(path, tmp, sizeof(tmp));

    if (!LittleFS.exists(path)) {
        if (!LittleFS.exists(tmp)) return ReadResult::Missing;
        if (!readFileExact(tmp, buf, len)) return ReadResult::Bad;
        LittleFS.rename(tmp, path);                    // We finish the interrupted replacement
        return ReadResult::Ok;
    }
    return readFileExact(path, buf, len) ? ReadResult::Ok : ReadResult::Bad;
}

bool writeAtomic(const char* path, const uint8_t* data, size_t len) {
    if (!mounted) return false;
    char tmp[48];
    tmpName(path, tmp, sizeof(tmp));

    File f = LittleFS.open(tmp, FILE_WRITE);           // Truncates if it already exists
    if (!f) return false;
    bool ok = f.write(data, len) == len;
    f.close();
    if (ok) ok = matches(tmp, data, len);
    if (!ok) { LittleFS.remove(tmp); return false; }

    if (!LittleFS.rename(tmp, path)) {                 // Fallback if the rename doesn't replace
        LittleFS.remove(path);
        ok = LittleFS.rename(tmp, path);
    }
    return ok;
}

// --- available room ------------------------------------------------------
namespace {
    constexpr size_t FLASH_MARGIN = 16384;     // 4 KB blocks, metadata, small files
    constexpr size_t HEAP_MARGIN  = 24576;     // Wi-Fi stack, mbedtls, screen...
}

#ifdef ARDUINO_ARCH_ESP32
size_t freeFlash() { return mounted ? LittleFS.totalBytes() - LittleFS.usedBytes() : 0; }
size_t maxHeap()   { return ESP.getMaxAllocHeap(); }
#else
size_t freeFlash() { return (size_t)1 << 30; }     // Off board (PC tests): no limit
size_t maxHeap()   { return (size_t)1 << 30; }
#endif

bool hasRoom(size_t fileBytes) { return freeFlash() >= fileBytes + FLASH_MARGIN; }

bool hasHeapFor(size_t bytes) { return maxHeap() >= bytes + HEAP_MARGIN; }

// --- chunked read / write ----------------------------------------
bool exists(const char* path) { return mounted && LittleFS.exists(path); }
bool removeFile(const char* path) { return mounted && LittleFS.remove(path); }
bool renameFile(const char* from, const char* to) { return mounted && LittleFS.rename(from, to); }
bool wipeAll() { return mounted && LittleFS.format(); }
void tmpPath(const char* path, char* out, size_t cap) { tmpName(path, out, cap); }

bool FileReader::open(const char* path) {
    close();
    if (!mounted) return false;
    File* f = new File(LittleFS.open(path, FILE_READ));
    if (!*f || f->isDirectory()) { delete f; return false; }
    h_ = f;
    return true;
}
size_t FileReader::size() { return h_ ? ((File*)h_)->size() : 0; }
bool FileReader::seek(size_t offset) { return h_ && ((File*)h_)->seek(offset); }
bool FileReader::read(uint8_t* buf, size_t n) { return h_ && (n == 0 || ((File*)h_)->read(buf, n) == n); }
void FileReader::close() { if (h_) { ((File*)h_)->close(); delete (File*)h_; h_ = nullptr; } }

bool FileWriter::begin(const char* path) {
    abort();
    if (!mounted || strlen(path) >= sizeof(path_)) return false;
    strcpy(path_, path);
    tmpName(path_, tmp_, sizeof(tmp_));
    File* f = new File(LittleFS.open(tmp_, FILE_WRITE));       // Truncates if it already exists
    if (!*f) { delete f; return false; }
    h_ = f;
    ok_ = true;
    return true;
}
bool FileWriter::write(const uint8_t* data, size_t n) {
    if (!h_ || !ok_) return false;
    if (n && ((File*)h_)->write(data, n) != n) ok_ = false;
    return ok_;
}
bool FileWriter::finish() {
    if (!h_) return false;
    ((File*)h_)->close(); delete (File*)h_; h_ = nullptr;
    return ok_;
}
bool FileWriter::commit() {
    if (h_ || !ok_) return false;                               // finish() first
    bool ok = LittleFS.rename(tmp_, path_);
    if (!ok) { LittleFS.remove(path_); ok = LittleFS.rename(tmp_, path_); }   // Fallback if the rename doesn't replace
    if (ok) tmp_[0] = 0;
    ok_ = false;
    return ok;
}
void FileWriter::abort() {
    if (h_) { ((File*)h_)->close(); delete (File*)h_; h_ = nullptr; }
    if (tmp_[0]) LittleFS.remove(tmp_);                       // Unfinished or refused temporary file
    tmp_[0] = 0;
    ok_ = false;
}

}
