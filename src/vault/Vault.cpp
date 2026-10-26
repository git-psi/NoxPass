// Vault.cpp : the vault (read, atomic write, sorted list, usage counters).
#include "Vault.h"
#include "Presets.h"
#include "../security/Auth.h"
#include "../security/Crypto.h"
#include "../storage/Storage.h"
#include "../core/Settings.h"
#include <algorithm>
#include <string.h>

// Vault file (v3): only the TITLES live in RAM; each entry is encrypted separately in flash
// and only read (and decrypted) when opened.
//
//   header : 'P' 'V' 3 | number of entries (u16)
//   entries: id (u32) | size (u16) | encrypted entry (nonce|cipher|tag, authenticated with its id)
//   footer : SHA-256 of everything before, encrypted -> any alteration, deletion or reordering is detected
//
// Writing = copy the file into a temporary file (unchanged entries are copied as is,
// without decrypting), read it back to check it, then rename it: the old one stays intact until it's done.

namespace Vault {

namespace {

// Files: the vault, and the usage counters (separate small file: it's the one rewritten at each opening).
constexpr const char* PATH_VAULT = "/vault3.bin";
constexpr const char* PATH_USAGE = "/usage.bin";

// Authenticated data "labels" (AAD): each type of encrypted block has its own, so a block
// can't be replaced by another one.
const uint8_t AAD_USAGE[8] = { 'P','M','U','S','A','G','E','1' };
const uint8_t AAD_FOOT[8]  = { 'P','M','F','O','O','T','0','3' };

constexpr size_t  HDR        = 5;                          // File header: 'P' 'V' 3 + number of entries
constexpr size_t  REC_HDR    = 6;                          // Entry header: id + size
constexpr size_t  FOOT       = 32 + Crypto::SEAL_OVERHEAD; // Footer: encrypted SHA-256
constexpr size_t  MAX_SEALED = 4096;                       // An encrypted entry never exceeds this (real max ~2.6 KB)
constexpr long    USAGE_MAX  = 262144;                     // Max accepted size for the counters file

const uint8_t* activeKey() { return Auth::key(); }          // Master key (nullptr if locked)

enum PendKind : uint8_t { P_ADD, P_UPD, P_DEL };
struct Pending { uint32_t id; PendKind kind; Entry e; };

std::vector<Item>     items;                        // In file order
std::vector<uint16_t> order;                        // Indexes in items, sorted by lastUsed descending
std::vector<Pending>  pend;                         // Queued changes (batch, or during a write)
uint32_t              counter = 0;                  // Global usage counter
uint32_t              selectedId = 0;
uint32_t              focusId = 0;                  // Focus requested for the list (one shot)
bool                  batch = false;                // true: changes are not written to flash
Error                 lastErr = Error::None;        // Why the last write was refused
uint32_t              lastId = 0;                   // id of the last added entry

void wipeString(std::string& s) {
    if (!s.empty()) Crypto::wipe(&s[0], s.size());
    s.clear();
}

void wipeEntry(Entry& e) {
    wipeString(e.title);
    for (size_t k = 0; k < e.fields.size(); k++) { wipeString(e.fields[k].label); wipeString(e.fields[k].value); }
    e.fields.clear();
}

// --- (de)serialization -----------------------------------------------------
struct Writer {
    std::vector<uint8_t> b;
    void u8(uint8_t v)   { b.push_back(v); }
    void u16(uint16_t v) { u8(v & 0xFF); u8(v >> 8); }
    void u32(uint32_t v) { u16(v & 0xFFFF); u16(v >> 16); }
    void str(const std::string& s) { u8((uint8_t)s.size()); b.insert(b.end(), s.begin(), s.end()); }
};

struct Reader {
    const uint8_t* p; size_t n; size_t i; bool ok;
    Reader(const uint8_t* data, size_t len) : p(data), n(len), i(0), ok(true) {}
    uint8_t  u8()  { if (i + 1 > n) { ok = false; return 0; } return p[i++]; }
    uint16_t u16() { uint16_t a = u8(); a |= (uint16_t)u8() << 8; return a; }
    uint32_t u32() { uint32_t a = u16(); a |= (uint32_t)u16() << 16; return a; }
    bool str(std::string& s, size_t maxLen) {
        uint8_t l = u8();
        if (!ok || l > maxLen || i + l > n) { ok = false; return false; }
        s.assign((const char*)p + i, l);
        i += l;
        return true;
    }
};

bool validEntry(const Entry& e) {
    if (e.title.empty() || e.title.size() > MAX_TITLE) return false;
    if (e.fields.empty() || e.fields.size() > MAX_FIELDS) return false;
    for (size_t k = 0; k < e.fields.size(); k++) {
        const Field& f = e.fields[k];
        if (f.type < T_USERNAME || f.type > T_LAST) return false;
        if (f.value.size() > MAX_VALUE) return false;
        if (hasLabel(f.type) ? (f.label.empty() || f.label.size() > MAX_LABEL) : !f.label.empty()) return false;
    }
    return true;
}

// Allowed text: printable ASCII only (the stick can type and display these characters on a USB keyboard).
bool plainText(const std::string& s) {
    for (size_t i = 0; i < s.size(); i++) { const unsigned char c = (unsigned char)s[i]; if ((c < 0x20 && c != 0x0A) || c > 0x7E) return false; }   // Line break stays allowed (notes)
    return true;
}
bool plainEntry(const Entry& e) {
    if (!plainText(e.title)) return false;
    for (size_t k = 0; k < e.fields.size(); k++) if (!plainText(e.fields[k].value) || !plainText(e.fields[k].label)) return false;
    return true;
}

// Content of an entry (without its id, which is next to it, in clear, in the file).
void serializeRecord(const Entry& e, Writer& w) {
    w.str(e.title);
    w.u8((uint8_t)e.fields.size());
    for (size_t k = 0; k < e.fields.size(); k++) {
        w.u8(e.fields[k].type);
        if (hasLabel(e.fields[k].type)) w.str(e.fields[k].label);   // Label stored only for Text/Secret
        w.str(e.fields[k].value);
    }
}

size_t recordPlainSize(const Entry& e) {
    size_t n = 1 + e.title.size() + 1;
    for (size_t k = 0; k < e.fields.size(); k++) {
        n += 1 + 1 + e.fields[k].value.size();
        if (hasLabel(e.fields[k].type)) n += 1 + e.fields[k].label.size();
    }
    return n;
}

bool parseRecord(const uint8_t* d, size_t len, Entry& e) {
    Reader r(d, len);
    if (!r.str(e.title, MAX_TITLE)) return false;
    const uint8_t fc = r.u8();
    if (!r.ok || fc == 0 || fc > MAX_FIELDS) return false;
    for (uint8_t k = 0; k < fc; k++) {
        Field f;
        f.type = r.u8();
        if (!r.ok || f.type < T_USERNAME || f.type > T_LAST) return false;
        if (hasLabel(f.type) && !r.str(f.label, MAX_LABEL)) return false;
        if (!r.str(f.value, MAX_VALUE)) return false;
        e.fields.push_back(f);
    }
    return r.ok && r.i == r.n;
}

void recordAad(uint32_t id, uint8_t aad[12]) {       // Binds the encrypted entry to its id: impossible to swap it with another one
    static const uint8_t T[8] = { 'P','M','R','E','C','0','0','3' };
    memcpy(aad, T, 8);
    aad[8] = id & 0xFF; aad[9] = (id >> 8) & 0xFF; aad[10] = (id >> 16) & 0xFF; aad[11] = (id >> 24) & 0xFF;
}

// out = id | size | encrypted entry: the record as written in the file.
bool sealRecord(uint32_t id, const Entry& e, std::vector<uint8_t>& out) {
    const uint8_t* key = activeKey();
    if (!key) return false;
    Writer w;
    w.b.reserve(recordPlainSize(e));
    serializeRecord(e, w);
    const size_t sealed = w.b.size() + Crypto::SEAL_OVERHEAD;
    bool ok = sealed <= MAX_SEALED;
    if (ok) {
        out.assign(REC_HDR + sealed, 0);
        out[0] = id & 0xFF; out[1] = (id >> 8) & 0xFF; out[2] = (id >> 16) & 0xFF; out[3] = (id >> 24) & 0xFF;
        out[4] = sealed & 0xFF; out[5] = (sealed >> 8) & 0xFF;
        uint8_t aad[12]; recordAad(id, aad);
        ok = Crypto::seal(key, aad, sizeof(aad), w.b.data(), w.b.size(), out.data() + REC_HDR);
    }
    Crypto::wipe(w.b.data(), w.b.size());
    return ok;
}

// --- single block encrypted file (usage counters) ----------------------------------------
bool writeSealed(const char* path, const uint8_t* aad, std::vector<uint8_t>& plain) {
    const uint8_t* key = activeKey();
    if (!key) return false;
    std::vector<uint8_t> out(plain.size() + Crypto::SEAL_OVERHEAD);
    return Crypto::seal(key, aad, 8, plain.data(), plain.size(), out.data())
        && Storage::writeAtomic(path, out.data(), out.size());
}

// 0 = ok, 1 = missing, 2 = corrupted or unreadable
int readSealed(const char* path, const uint8_t* aad, std::vector<uint8_t>& plain) {
    const uint8_t* key = activeKey();
    if (!key) return 2;
    const long sz = Storage::sizeOf(path);
    if (sz == -1) return 1;
    if (sz < (long)Crypto::SEAL_OVERHEAD + 1 || sz > USAGE_MAX) return 2;
    if (!Storage::hasHeapFor((size_t)sz * 2)) return 2;       // Not enough RAM to read: don't try

    std::vector<uint8_t> in((size_t)sz);
    if (Storage::readExact(path, in.data(), in.size()) != Storage::ReadResult::Ok) return 2;
    plain.assign(in.size() - Crypto::SEAL_OVERHEAD, 0);
    return Crypto::open(key, aad, 8, in.data(), in.size(), plain.data()) ? 0 : 2;
}

// --- full file read (at boot and to check a write) -----------------------
// Authenticates the whole file and rebuilds the titles list. Only one entry in RAM at a time.
bool scan(const char* path, std::vector<Item>& out) {
    out.clear();
    const uint8_t* key = activeKey();
    if (!key) return false;
    Storage::FileReader r;
    if (!r.open(path)) return false;
    const size_t total = r.size();
    if (total < HDR + FOOT || total > MAX_FILE_BYTES) return false;

    uint8_t h[HDR];
    if (!r.read(h, HDR) || h[0] != 'P' || h[1] != 'V' || h[2] != 3) return false;
    const uint16_t n = (uint16_t)(h[3] | (h[4] << 8));
    if (n > MAX_ENTRIES) return false;

    void* sha = Crypto::sha256Begin();
    if (!sha) return false;
    Crypto::sha256Update(sha, h, HDR);

    bool ok = true;
    size_t pos = HDR;
    std::vector<uint8_t> buf, plain;
    out.reserve(n);
    for (uint16_t i = 0; i < n && ok; i++) {
        uint8_t rh[REC_HDR];
        ok = (pos + REC_HDR <= total - FOOT) && r.read(rh, REC_HDR);
        if (!ok) break;
        const uint32_t id  = (uint32_t)rh[0] | ((uint32_t)rh[1] << 8) | ((uint32_t)rh[2] << 16) | ((uint32_t)rh[3] << 24);
        const size_t   len = (size_t)rh[4] | ((size_t)rh[5] << 8);
        ok = len > Crypto::SEAL_OVERHEAD + 1 && len <= MAX_SEALED && pos + REC_HDR + len <= total - FOOT;
        if (!ok) break;
        buf.resize(len);
        ok = r.read(buf.data(), len);
        if (!ok) break;
        Crypto::sha256Update(sha, rh, REC_HDR);
        Crypto::sha256Update(sha, buf.data(), len);

        plain.assign(len - Crypto::SEAL_OVERHEAD, 0);
        uint8_t aad[12]; recordAad(id, aad);
        Entry e;
        ok = id != 0 && Crypto::open(key, aad, sizeof(aad), buf.data(), len, plain.data())
          && parseRecord(plain.data(), plain.size(), e) && validEntry(e);
        if (ok) {
            Item it;
            memset(&it, 0, sizeof(it));
            it.id = id;
            memcpy(it.title, e.title.data(), e.title.size());
            it.offset = (uint32_t)(pos + REC_HDR);
            it.len = (uint16_t)len;
            out.push_back(it);
            pos += REC_HDR + len;
        }
        Crypto::wipe(plain.data(), plain.size());
        wipeEntry(e);
    }
    ok = ok && pos == total - FOOT;

    uint8_t hash[32];
    Crypto::sha256End(sha, hash);                      // Always frees the context
    if (ok) {
        uint8_t foot[FOOT], got[32];
        ok = r.read(foot, FOOT) && Crypto::open(key, AAD_FOOT, sizeof(AAD_FOOT), foot, FOOT, got)
          && memcmp(got, hash, 32) == 0;
        Crypto::wipe(got, sizeof(got));
    }
    if (!ok) out.clear();
    return ok;
}

// --- usage counters ----------------------------------------------------------------------
bool saveUsage() {
    Writer w;
    w.u32(counter);
    w.u16((uint16_t)items.size());
    for (size_t i = 0; i < items.size(); i++) { w.u32(items[i].id); w.u32(items[i].lastUsed); }
    return writeSealed(PATH_USAGE, AAD_USAGE, w.b);
}

void loadUsage() {                                   // Not vital: without it, the order is just lost
    std::vector<uint8_t> plain;
    if (readSealed(PATH_USAGE, AAD_USAGE, plain) != 0) return;
    Reader r(plain.data(), plain.size());
    uint32_t global = r.u32();
    const uint16_t n = r.u16();
    for (uint16_t i = 0; i < n && r.ok; i++) {
        const uint32_t id = r.u32(), used = r.u32();
        if (!r.ok) break;
        for (size_t k = 0; k < items.size(); k++) {
            if (items[k].id == id) { items[k].lastUsed = used; break; }
        }
        if (used > global) global = used;
    }
    counter = global;
}

void rebuild() {
    order.clear();
    for (size_t i = 0; i < items.size(); i++) order.push_back((uint16_t)i);
    if (Settings::get().sortAlpha) {                  // Alphabetical order (case insensitive)
        std::sort(order.begin(), order.end(), [](uint16_t a, uint16_t b) {
            const char* x = items[a].title; const char* y = items[b].title;
            for (; *x && *y; x++, y++) {
                const int cx = (*x >= 'A' && *x <= 'Z') ? *x + 32 : (unsigned char)*x;
                const int cy = (*y >= 'A' && *y <= 'Z') ? *y + 32 : (unsigned char)*y;
                if (cx != cy) return cx < cy;
            }
            if (*x != *y) return *x == 0;                 // Shortest first
            return a < b;
        });
        return;
    }
    std::sort(order.begin(), order.end(), [](uint16_t a, uint16_t b) {
        if (items[a].lastUsed != items[b].lastUsed) return items[a].lastUsed > items[b].lastUsed;
        return a < b;
    });
}

// --- queued changes -----------------------------------------------------------------------
Pending* pendFind(uint32_t id) {
    for (size_t i = 0; i < pend.size(); i++) if (pend[i].id == id) return &pend[i];
    return nullptr;
}

void pendClear() {
    for (size_t i = 0; i < pend.size(); i++) wipeEntry(pend[i].e);
    pend.clear();
}

const Item* findItem(uint32_t id) {
    for (size_t i = 0; i < items.size(); i++) if (items[i].id == id) return &items[i];
    return nullptr;
}

bool idTaken(uint32_t id) { return findItem(id) != nullptr || pendFind(id) != nullptr; }

// Does the entry still exist once the queued changes are applied?
bool liveExists(uint32_t id) {
    const Pending* p = pendFind(id);
    if (p) return p->kind != P_DEL;
    return findItem(id) != nullptr;
}

size_t liveCount() {
    size_t n = items.size();
    for (size_t i = 0; i < pend.size(); i++) {
        if (pend[i].kind == P_ADD) n++;
        else if (pend[i].kind == P_DEL) n--;
    }
    return n;
}

// Queues a new entry (it will be written by commitPending). Gives it a random id and puts it
// first in the list (most recent).
bool queueAdd(Entry& e) {
    if (!validEntry(e)) return false;
    if (!plainEntry(e)) { lastErr = Error::NotPlain; return false; }
    if (liveCount() >= MAX_ENTRIES) { lastErr = Error::Full; return false; }
    uint32_t id;
    do { Crypto::randomBytes((uint8_t*)&id, sizeof(id)); } while (id == 0 || idTaken(id));
    e.lastUsed = ++counter;
    lastId = id;
    Pending p; p.id = id; p.kind = P_ADD; p.e = e;
    pend.push_back(p);
    return true;
}

// Rewrites the file with the queued changes. Nothing is modified in flash or RAM if it fails.
bool commitPending() {
    const uint8_t* key = activeKey();
    if (!key) { lastErr = Error::Write; return false; }

    // 1) how many entries, what size? (clean refusal BEFORE writing)
    size_t newCount = 0, est = HDR + FOOT;
    for (size_t i = 0; i < items.size(); i++) {
        const Pending* p = pendFind(items[i].id);
        if (p && p->kind == P_DEL) continue;
        newCount++;
        est += REC_HDR + ((p && p->kind == P_UPD) ? recordPlainSize(p->e) + Crypto::SEAL_OVERHEAD : items[i].len);
    }
    for (size_t i = 0; i < pend.size(); i++) {
        if (pend[i].kind != P_ADD) continue;
        newCount++;
        est += REC_HDR + recordPlainSize(pend[i].e) + Crypto::SEAL_OVERHEAD;
    }
    const bool tooMany  = newCount > MAX_ENTRIES;                                   // Too many entries
    const bool tooBig   = est > MAX_FILE_BYTES;                                     // File too big
    const bool noFlash  = !Storage::hasRoom(est);                                   // No more flash room
    const bool noRam    = !Storage::hasHeapFor(newCount * sizeof(Item) * 2 + 3 * MAX_SEALED);   // No more RAM
    if (tooMany || tooBig || noFlash || noRam) { lastErr = Error::Full; return false; }

    // 2) write into the temporary file
    Storage::FileReader old;
    if (!items.empty() && !old.open(PATH_VAULT)) { lastErr = Error::Write; return false; }
    Storage::FileWriter w;
    void* sha = Crypto::sha256Begin();
    bool ok = sha && w.begin(PATH_VAULT);
    auto emit = [&](const uint8_t* d, size_t n) {
        if (!ok) return;
        ok = w.write(d, n);
        Crypto::sha256Update(sha, d, n);
    };

    const uint8_t hdr[HDR] = { 'P', 'V', 3, (uint8_t)(newCount & 0xFF), (uint8_t)(newCount >> 8) };
    emit(hdr, HDR);

    std::vector<uint8_t> rec;
    uint8_t chunk[256];
    for (size_t i = 0; i < items.size() && ok; i++) {
        const Item& it = items[i];
        const Pending* p = pendFind(it.id);
        if (p && p->kind == P_DEL) continue;
        if (p && p->kind == P_UPD) {
            ok = sealRecord(it.id, p->e, rec);
            if (ok) emit(rec.data(), rec.size());
        } else {                                         // Unchanged: copied as is, without decrypting
            ok = old.seek(it.offset - REC_HDR);
            for (size_t left = REC_HDR + it.len; ok && left > 0;) {
                const size_t n = left < sizeof(chunk) ? left : sizeof(chunk);
                ok = old.read(chunk, n);
                if (ok) emit(chunk, n);
                left -= n;
            }
        }
    }
    for (size_t i = 0; i < pend.size() && ok; i++) {
        if (pend[i].kind != P_ADD) continue;
        ok = sealRecord(pend[i].id, pend[i].e, rec);
        if (ok) emit(rec.data(), rec.size());
    }
    old.close();

    uint8_t hash[32];
    Crypto::sha256End(sha, hash);
    if (ok) {
        uint8_t foot[FOOT];
        ok = Crypto::seal(key, AAD_FOOT, sizeof(AAD_FOOT), hash, sizeof(hash), foot) && w.write(foot, FOOT);
    }
    ok = ok && w.finish();

    // 3) full read back of the temporary file (decrypts everything): the old one is replaced only if it's good
    std::vector<Item> ni;
    char tmp[48];
    Storage::tmpPath(PATH_VAULT, tmp, sizeof(tmp));
    ok = ok && scan(tmp, ni) && ni.size() == newCount;
    if (!ok) { w.abort(); lastErr = Error::Write; return false; }
    if (!w.commit()) { w.abort(); lastErr = Error::Write; return false; }

    // 4) done: the RAM list takes the place of the old one (usage counters are kept)
    for (size_t i = 0; i < ni.size(); i++) {
        const Pending* p = pendFind(ni[i].id);
        if (p && p->kind == P_ADD) { ni[i].lastUsed = p->e.lastUsed; continue; }
        const Item* o = findItem(ni[i].id);
        if (o) ni[i].lastUsed = o->lastUsed;
    }
    items.swap(ni);
    pendClear();
    rebuild();
    saveUsage();
    return true;
}

}  // namespace

bool isObfuscated(uint8_t type) { return type == T_PASSWORD || type == T_CARD || type == T_CVV || type == T_SECRET; }

bool hasLabel(uint8_t type) { return type == T_TEXT || type == T_SECRET; }

const char* fieldName(const Field& f) {
    switch (f.type) {
        case T_USERNAME: return "Username / Email";
        case T_PASSWORD: return "Password";
        case T_TEXT:
        case T_SECRET:   return f.label.c_str();
        case T_URL:      return "URL";
        case T_CARD:     return "Card number";
        case T_EXPIRY:   return "Expiry";
        case T_CVV:      return "CVV";
        case T_NOTE:     return "Note";
    }
    return "?";
}

void wipe(Entry& e) { wipeEntry(e); }

void clear() {
    pendClear();
    batch = false;
    if (!items.empty()) Crypto::wipe(items.data(), items.size() * sizeof(Item));
    std::vector<Item>().swap(items);
    std::vector<uint16_t>().swap(order);
    counter = 0;
    selectedId = 0;
    Presets::clear();                          // Saved usernames too (lock)
}

LoadResult load() {
    clear();
    char tmp[48];
    Storage::tmpPath(PATH_VAULT, tmp, sizeof(tmp));
    if (!Storage::exists(PATH_VAULT) && Storage::exists(tmp)) {          // Interrupted replacement: recover the complete file
        std::vector<Item> t;
        if (scan(tmp, t)) Storage::renameFile(tmp, PATH_VAULT); else Storage::removeFile(tmp);
    }

    if (Storage::exists(PATH_VAULT)) {
        std::vector<Item> t;
        if (!scan(PATH_VAULT, t)) { clear(); return LoadResult::Corrupt; }   // NOTHING is written: data may be recoverable
        items.swap(t);
        loadUsage();
        rebuild();
        return LoadResult::Ok;
    }
    rebuild();                                           // No file: empty vault (first launch)
    return LoadResult::Ok;
}

size_t count() { return order.size(); }

const Item* at(size_t index) { return index < order.size() ? &items[order[index]] : nullptr; }

bool read(uint32_t id, Entry& out) {
    const Item* it = findItem(id);
    const uint8_t* key = activeKey();
    if (!it || !key) return false;
    Storage::FileReader r;
    std::vector<uint8_t> buf(it->len);
    if (!r.open(PATH_VAULT) || !r.seek(it->offset) || !r.read(buf.data(), buf.size())) return false;
    std::vector<uint8_t> plain(buf.size() - Crypto::SEAL_OVERHEAD, 0);
    uint8_t aad[12]; recordAad(id, aad);
    Entry e;
    bool ok = Crypto::open(key, aad, sizeof(aad), buf.data(), buf.size(), plain.data())
           && parseRecord(plain.data(), plain.size(), e);
    Crypto::wipe(plain.data(), plain.size());
    if (!ok) { wipeEntry(e); return false; }
    e.id = id;
    e.lastUsed = it->lastUsed;
    out = e;
    return true;
}

Error lastError() { return lastErr; }
uint32_t lastAdded() { return lastId; }


bool add(Entry e) {
    lastErr = Error::None;
    const bool queued = queueAdd(e);
    wipeEntry(e);                                                   // The local copy is no longer needed (one copy is queued)
    if (!queued) return false;
    if (batch) return true;
    const bool ok = commitPending();
    pendClear();
    return ok;
}

bool update(uint32_t id, Entry e) {
    lastErr = Error::None;
    e.id = id;
    if (!validEntry(e) || !liveExists(id)) return false;
    if (!plainEntry(e)) { lastErr = Error::NotPlain; return false; }
    Pending* p = pendFind(id);
    if (p) { e.lastUsed = p->e.lastUsed; p->e = e; }                // Entry added or already modified in this batch
    else { Pending n; n.id = id; n.kind = P_UPD; n.e = e; pend.push_back(n); }
    wipeEntry(e);                                                   // The local copy is no longer needed (one copy is queued)
    if (batch) return true;
    const bool ok = commitPending();
    pendClear();
    return ok;
}

bool remove(uint32_t id) {
    lastErr = Error::None;
    if (!liveExists(id)) return false;
    Pending* p = pendFind(id);
    if (p && p->kind == P_ADD) {                                    // Added in this batch then deleted: nothing to write
        wipeEntry(p->e);
        pend.erase(pend.begin() + (p - &pend[0]));
    } else if (p) {
        wipeEntry(p->e);
        p->kind = P_DEL;
    } else {
        Pending n; n.id = id; n.kind = P_DEL; pend.push_back(n);
    }
    if (batch) return true;
    const bool ok = commitPending();
    pendClear();
    return ok;
}

void beginBatch() { batch = true; }

bool commitBatch() {
    lastErr = Error::None;
    batch = false;
    if (pend.empty()) return true;
    const bool ok = commitPending();
    pendClear();
    return ok;
}

void abortBatch() {                                  // Nothing was written: simply drop the queued changes
    batch = false;
    pendClear();
}

void resort() { rebuild(); }

void markUsed(uint32_t id) {
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].id == id) {
            if (items[i].lastUsed != 0 && items[i].lastUsed == counter) return;   // Already the most recent: nothing to rewrite
            items[i].lastUsed = ++counter;
            rebuild();
            saveUsage();
            return;
        }
    }
}

void     setFocus(uint32_t id) { focusId = id; }
uint32_t takeFocus()           { uint32_t f = focusId; focusId = 0; return f; }
void     select(uint32_t id) { selectedId = id; }
uint32_t selected()          { return selectedId; }

}
