// WebApi.cpp : JSON content exchanged with the website, and application of the requested changes.
#include "WebApi.h"
#include "../core/Settings.h"
#include "../vault/Presets.h"
#include <ArduinoJson.h>
#include <string.h>
#include <algorithm>
#include <stdio.h>

namespace WebApi {

namespace {

constexpr size_t MAX_OPS = 200;      // Max number of vault changes in a single request

std::string num(unsigned long v) { char b[16]; snprintf(b, sizeof(b), "%lu", v); return b; }

// Are settings a and b different?
bool differ(const Settings::Values& a, const Settings::Values& b) {
    return a.theme != b.theme || a.layout != b.layout || a.pinDigits != b.pinDigits || a.pinShuffle != b.pinShuffle
        || a.offOnUnplug != b.offOnUnplug || a.sortAlpha != b.sortAlpha || a.brightness != b.brightness
        || a.autoLock != b.autoLock;
}

// Reads an entry sent by the website: {"title":..., "fields":[{"t":type,"l":name,"v":value}, ...]}.
bool readEntry(JsonObjectConst o, Vault::Entry& e) {
    e.title = (const char*)(o["title"] | "");
    JsonArrayConst fa = o["fields"];
    for (JsonObjectConst fo : fa) {
        const int t = fo["t"] | 0;
        if (t < Vault::T_USERNAME || t > Vault::T_LAST) return false;
        Vault::Field f;
        f.type = (uint8_t)t;
        if (Vault::hasLabel(f.type)) f.label = (const char*)(fo["l"] | "");   // The name only exists for Text/Secret
        f.value = (const char*)(fo["v"] | "");
        e.fields.push_back(f);
        if (e.fields.size() > Vault::MAX_FIELDS) return false;
    }
    return true;
}

}  // namespace

// Quoted text, with special characters escaped.
std::string jsonString(const std::string& s) {
    std::string out = "\"";
    for (size_t i = 0; i < s.size(); i++) {
        const unsigned char c = (unsigned char)s[i];
        if (c == '"')       out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c < 0x20)  { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
        else                out += (char)c;
    }
    return out + "\"";
}

std::string vaultHead() { return "{\"entries\":["; }

std::string entryTitleJson(const Vault::Item& it) {
    return "{\"id\":" + num(it.id) + ",\"title\":" + jsonString(it.title) + "}";
}

std::string entryJson(const Vault::Entry& e) {
    std::string s = "{\"id\":" + num(e.id) + ",\"title\":" + jsonString(e.title) + ",\"fields\":[";
    for (size_t k = 0; k < e.fields.size(); k++) {
        const Vault::Field& f = e.fields[k];
        if (k) s += ',';
        s += "{\"t\":" + num(f.type);
        if (Vault::hasLabel(f.type)) s += ",\"l\":" + jsonString(f.label);
        s += ",\"v\":" + jsonString(f.value) + "}";
    }
    return s + "]}";
}

// End of the list: settings, vault limits and themes list.
std::string vaultTail() {
    const Settings::Values& v = Settings::get();
    std::string s = "],\"settings\":{\"theme\":\"";
    s += Settings::themeKey(v.theme) ? Settings::themeKey(v.theme) : "dark";
    s += "\",\"layout\":\"";
    s += v.layout == 1 ? "qwerty" : "azerty";
    s += "\",\"pinDigits\":";
    s += v.pinDigits ? "true" : "false";
    s += ",\"pinShuffle\":";
    s += v.pinShuffle ? "true" : "false";
    s += ",\"offOnUnplug\":";
    s += v.offOnUnplug ? "true" : "false";
    s += ",\"sort\":\"";
    s += v.sortAlpha ? "alpha" : "recent";
    s += "\"";
    s += ",\"brightness\":" + num(v.brightness);
    s += ",\"autoLock\":" + num(v.autoLock);
    s += "},\"presets\":[";
    Presets::load();
    for (size_t i = 0; i < Presets::count(); i++) { if (i) s += ','; s += jsonString(Presets::at(i)); }
    s += "],\"limits\":{\"title\":" + num(Vault::MAX_TITLE)
       + ",\"presets\":" + num(Presets::MAX_COUNT)
       + ",\"label\":" + num(Vault::MAX_LABEL)
       + ",\"value\":" + num(Vault::MAX_VALUE)
       + ",\"fields\":" + num(Vault::MAX_FIELDS)
       + ",\"entries\":" + num(Vault::MAX_ENTRIES) + "}";
    // Themes list: the same table as the stick Settings screen (Settings.cpp).
    s += ",\"themes\":[";
    for (uint8_t i = 0; i < Settings::THEME_COUNT; i++) {
        char c[24];
        if (i) s += ',';
        s += "{\"k\":" + jsonString(Settings::themeKey(i)) + ",\"n\":" + jsonString(Settings::themeName(i));
        snprintf(c, sizeof(c), ",\"bg\":\"#%06lX\"", (unsigned long)Settings::themeBg(i)); s += c;
        snprintf(c, sizeof(c), ",\"fg\":\"#%06lX\"}", (unsigned long)Settings::themeFg(i)); s += c;
    }
    s += "]}";
    return s;
}

// Applies a website request. See WebApi.h for the format and the guarantees.
bool apply(const char* body, size_t len, std::string& error, int& count) {
    count = 0;
    JsonDocument doc;
    if (deserializeJson(doc, body, len)) { error = "Bad request"; return false; }

    // 1) Settings: validated BEFORE touching the vault.
    Settings::Values sv = Settings::get();
    bool settingsChanged = false;
    JsonObjectConst js = doc["settings"];
    if (!js.isNull()) {
        if (js["theme"].is<const char*>()) {
            const int ti = Settings::themeFromKey(js["theme"].as<const char*>());
            if (ti < 0) { error = "Bad theme"; return false; }
            sv.theme = (uint8_t)ti;
        }
        if (js["layout"].is<const char*>()) {
            const std::string t = js["layout"].as<const char*>();
            if (t == "azerty") sv.layout = 0; else if (t == "qwerty") sv.layout = 1; else { error = "Bad keyboard layout"; return false; }
        }
        if (js["pinDigits"].is<bool>()) sv.pinDigits = js["pinDigits"].as<bool>();
        if (js["pinShuffle"].is<bool>()) sv.pinShuffle = js["pinShuffle"].as<bool>();
        if (js["offOnUnplug"].is<bool>()) sv.offOnUnplug = js["offOnUnplug"].as<bool>();
        if (js["sort"].is<const char*>()) {
            const std::string t = js["sort"].as<const char*>();
            if (t == "recent") sv.sortAlpha = false; else if (t == "alpha") sv.sortAlpha = true; else { error = "Bad sort order"; return false; }
        }
        if (js["brightness"].is<int>()) {
            const int br = js["brightness"].as<int>();
            if (br < 0 || br > 100) { error = "Bad brightness"; return false; }
            sv.brightness = (uint8_t)br;
        }
        if (js["autoLock"].is<int>()) {
            const int a = js["autoLock"].as<int>();
            if (a < 0 || a > 65535) { error = "Bad auto-lock"; return false; }
            sv.autoLock = (uint16_t)a;
        }
        if (!Settings::valid(sv)) { error = "Bad setting"; return false; }
        settingsChanged = differ(sv, Settings::get());
    }

    // Presets (saved usernames): the whole list, validated BEFORE touching the vault.
    std::vector<std::string> pv;
    bool presetsChanged = false;
    if (doc["presets"].is<JsonArrayConst>()) {
        for (JsonVariantConst x : doc["presets"].as<JsonArrayConst>()) {
            const std::string t = x.is<const char*>() ? x.as<const char*>() : "";
            if (!Presets::valid(t)) { error = "Bad preset: 1 to 128 plain characters"; return false; }
            if (std::find(pv.begin(), pv.end(), t) == pv.end()) pv.push_back(t);
        }
        if (pv.size() > Presets::MAX_COUNT) { error = "Too many presets"; return false; }
        Presets::load();
        presetsChanged = pv.size() != Presets::count();
        for (size_t i = 0; i < pv.size() && !presetsChanged; i++) presetsChanged = pv[i] != Presets::at(i);
    }

    // 2) Vault: all or nothing.
    JsonArrayConst ops = doc["ops"];
    if (ops.size() > MAX_OPS) { error = "Too many changes at once"; return false; }
    int n = 0;
    if (ops.size() > 0) {
        Vault::beginBatch();
        for (JsonObjectConst op : ops) {
            const std::string kind = (const char*)(op["op"] | "");
            const uint32_t id = op["id"].as<uint32_t>();
            bool ok = false;
            if (kind == "add" || kind == "upd") {
                Vault::Entry e;
                if (!readEntry(op, e)) { error = "Invalid entry"; Vault::abortBatch(); return false; }
                const std::string title = e.title.empty() ? "(untitled)" : e.title;
                ok = (kind == "add") ? Vault::add(e) : Vault::update(id, e);
                if (!ok) {
                    error = (Vault::lastError() == Vault::Error::Full) ? "Vault is full"
                          : (Vault::lastError() == Vault::Error::NotPlain) ? "Only plain ASCII characters are allowed: " + title
                          : "Rejected: " + title;
                    Vault::abortBatch(); return false;
                }
            } else if (kind == "del") {
                ok = Vault::remove(id);
                if (!ok) { error = "Entry not found"; Vault::abortBatch(); return false; }
            } else {
                error = "Unknown operation"; Vault::abortBatch(); return false;
            }
            n++;
        }
        if (!Vault::commitBatch()) {
            error = (Vault::lastError() == Vault::Error::Full) ? "Not enough space: nothing was changed" : "Could not save the vault";
            Vault::abortBatch();                     // Flash was not touched: back to the previous state
            return false;
        }
    }

    // 3) Presets, then settings (the vault is already saved).
    if (presetsChanged) {
        if (!Presets::set(pv)) { error = "Presets not saved"; count = n; return false; }
        n++;
    }
    if (settingsChanged) {
        if (!Settings::set(sv)) { error = "Settings not saved"; count = n; return false; }
        n++;
    }
    count = n;
    return true;
}

}
