// Storage.h : file access (LittleFS). Knows neither the keys nor the vault: bytes, nothing else.
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Storage {
    constexpr const char* KEYS_PATH = "/keys.bin";     // Master key blob (81 bytes)

    enum class ReadResult : uint8_t { Ok, Missing, Bad };

    // Mounts the file system (formats it on the very first boot).
    bool begin();

    // File size: -1 = missing, -2 = unreadable. (Takes into account a complete .tmp left by a power cut.)
    long sizeOf(const char* path);

    // Reads exactly len bytes. Missing = file missing, Bad = present but invalid size/read.
    // If a replacement was interrupted (path missing but path.tmp complete), recovers the .tmp.
    ReadResult readExact(const char* path, uint8_t* buf, size_t len);

    // Atomic write: temporary file -> read back and compare -> rename.
    // On power cut, the old file stays intact.
    bool writeAtomic(const char* path, const uint8_t* data, size_t len);

    // --- Chunked read / write (big files: they are never fully loaded in RAM) ---
    bool exists(const char* path);
    bool removeFile(const char* path);
    bool renameFile(const char* from, const char* to);
    bool wipeAll();                                          // Erases ALL files (complete reset)
    void tmpPath(const char* path, char* out, size_t cap);   // Name of the temporary file of path

    class FileReader {
    public:
        ~FileReader() { close(); }
        bool   open(const char* path);
        size_t size();
        bool   seek(size_t offset);
        bool   read(uint8_t* buf, size_t n);    // Exactly n bytes, otherwise false
        void   close();
    private:
        void* h_ = nullptr;
    };

    // Atomic chunked write: begin() -> write()... -> finish() -> commit() (renames the temporary file).
    // Between finish() and commit(), the temporary file can be read back to check it. abort() erases it.
    class FileWriter {
    public:
        ~FileWriter() { abort(); }
        bool begin(const char* path);
        bool write(const uint8_t* data, size_t n);
        bool finish();
        bool commit();
        void abort();
    private:
        void* h_ = nullptr;
        char  path_[40] = {0};
        char  tmp_[48]  = {0};
        bool  ok_ = false;
    };

    // Available room, to cleanly refuse a too big write instead of crashing.
    size_t freeFlash();                  // Free bytes of the file system
    size_t maxHeap();                    // Biggest RAM block allocatable at once
    // Can a file of fileBytes bytes be written? (the old one stays in flash during the write: margin included)
    bool hasRoom(size_t fileBytes);
    bool hasHeapFor(size_t bytes);       // Only RAM (read)
}