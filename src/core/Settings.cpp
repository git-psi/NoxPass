// Settings.cpp : device settings (file read / write, apply, theme table, and description of the stick Settings screen rows).
#include "Settings.h"
#include <string.h>
#include <M5Unified.h>
#include "../config.h"
#include "../hid/Hid.h"
#include "../storage/Storage.h"
#include "../ui/Theme.h"
#include "../vault/Vault.h"

namespace Settings {

namespace {
    // /settings.bin file (10 bytes, in clear: it must be readable before the PIN is typed)
    // theme | keyboard | PIN digits | auto-lock (u16) | brightness | PIN shuffle | off on unplug | sort | checksum (XOR of all previous bytes)
    // A missing or altered file simply gives the default values.
    constexpr size_t  FILE_LEN = 10;

    // Default values
    Values defaults() {
        Values v;
        v.theme       = 0;
        v.layout      = Config::HID_QWERTY ? 1 : 0;
        v.pinDigits   = Config::PIN_SHOW_DIGITS;
        v.sortAlpha   = false;
        v.offOnUnplug = false;
        v.pinShuffle  = false;
        v.brightness  = 100;
        v.autoLock    = Config::AUTOLOCK_DEFAULT_S;
        return v;
    }

    // Themes: a background color and foreground color (0xRRGGBB)
    struct ThemeDef { const char* key; const char* name; uint32_t bg, fg; };
    const ThemeDef THEMES[THEME_COUNT] = {
        // Light and dark basics. Dark backgrounds are kept close to black: the screen has few shades.
        {"dark",     "Dark",     0x000000, 0xFFFFFF}, {"light",    "Light",    0xFFFFFF, 0x000000},
        {"matrix",   "Matrix",   0x000702, 0x00FF41}, {"lime",     "Lime",     0x020A06, 0x7CFF6B},
        {"amber",    "Amber",    0x0A0A0A, 0xFFB000}, {"paper",    "Paper",    0xE8E1D5, 0x20251F},
        {"violet",   "Violet",   0x0A0710, 0xC77DFF}, {"dusk",     "Dusk",     0x05080C, 0xF2AA4C},
        {"ice",      "Ice",      0x02070A, 0xA8E6FF}, {"sand",     "Sand",     0xD8C3A5, 0x35291F},
        // Inspired by apps and editors
        {"discord",  "Discord",  0x16181B, 0x7289DA}, {"nord",     "Nord",     0x161A22, 0x88C0D0},
        {"midnight", "Midnight", 0x030310, 0x8C9EFF},
        // Bright colors
        {"neon",     "Neon",     0x050008, 0xFF2BD6}, {"sakura",   "Sakura",   0xF8E8EE, 0xB03060},
        {"ruby",     "Ruby",     0x0A0000, 0xFF5252},
        // Greens, and the high contrast one
        {"mint",     "Mint",     0x040B08, 0x7CFFCB}, {"forest",   "Forest",   0x050C05, 0x8FD694},
        {"canary",   "Canary",   0x000000, 0xFFFF00},
    };

    uint16_t rgb565(uint32_t c) {
        return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x001F));
    }

    Values cur = defaults();

    // File checksum: XOR of all bytes except the last one.
    uint8_t check(const uint8_t* b, size_t len) {
        uint8_t x = 0xA5;
        for (size_t i = 0; i < len - 1; i++) x ^= b[i];
        return x;
    }

    // Applies the current settings: colors, screen brightness, USB keyboard layout.
    void apply() {
        const ThemeDef& t = THEMES[cur.theme < THEME_COUNT ? cur.theme : 0];
        Theme::set(rgb565(t.bg), rgb565(t.fg));

        // 0% = 1/255, because 0/255 is black screen
        M5.Display.setBrightness(cur.brightness == 0 ? 1 : (uint8_t)((cur.brightness * 255 + 50) / 100));
        Hid::setLayout(cur.layout == 1 ? Hid::Layout::Qwerty : Hid::Layout::Azerty);
    }

    bool save() {
        uint8_t b[FILE_LEN] = { cur.theme, cur.layout, (uint8_t)(cur.pinDigits ? 1 : 0),
                                (uint8_t)(cur.autoLock & 0xFF), (uint8_t)(cur.autoLock >> 8), cur.brightness,
                                (uint8_t)(cur.pinShuffle ? 1 : 0), (uint8_t)(cur.offOnUnplug ? 1 : 0),
                                (uint8_t)(cur.sortAlpha ? 1 : 0), 0 };
        b[FILE_LEN - 1] = check(b, FILE_LEN);
        return Storage::writeAtomic(PATH, b, FILE_LEN);
    }

    }

    bool valid(const Values& v) {
        if (v.theme >= THEME_COUNT || v.layout > 1 || v.brightness > 100) return false;
        return v.autoLock == 0 || v.autoLock == 30 || v.autoLock == 60 || v.autoLock == 300;
    }

    const Values& get() { return cur; }

    const char* themeKey(uint8_t i) { return i < THEME_COUNT ? THEMES[i].key : nullptr; }
    const char* themeName(uint8_t i) { return i < THEME_COUNT ? THEMES[i].name : nullptr; }
    uint32_t themeBg(uint8_t i) { return i < THEME_COUNT ? THEMES[i].bg : 0x000000; }
    uint32_t themeFg(uint8_t i) { return i < THEME_COUNT ? THEMES[i].fg : 0xFFFFFF; }

    int themeFromKey(const char* k) {
        for (uint8_t i = 0; i < THEME_COUNT; i++) if (strcmp(THEMES[i].key, k) == 0) return i;
        return -1;
    }

    void load() {
        cur = defaults();
        uint8_t b[FILE_LEN];
        if (Storage::readExact(PATH, b, FILE_LEN) == Storage::ReadResult::Ok
            && b[FILE_LEN - 1] == check(b, FILE_LEN)) {
            Values v = defaults();
            v.theme       = b[0];
            v.layout      = b[1];
            v.pinDigits   = b[2] != 0;
            v.autoLock    = (uint16_t)(b[3] | (b[4] << 8));
            v.brightness  = b[5];
            v.pinShuffle  = b[6] != 0;
            v.offOnUnplug = b[7] != 0;
            v.sortAlpha   = b[8] != 0;
            if (valid(v)) cur = v;
        }
        apply();
    }

    bool set(const Values& v) {
        if (!valid(v)) return false;
        const bool resort = v.sortAlpha != cur.sortAlpha;
        cur = v;
        apply();
        if (resort) Vault::resort();               // The new list sort is applied right away
        return save();
    }

    // Stick Settings screen
    // Each row has a number (Item): it is also its position in the list and in NAMES[].
    // To add a setting: a field in Values, a line here (enum + NAMES + the functions below),
    // ITEM_COUNT in Settings.h, and its saving in save() / load().

    namespace {
        enum Item : uint8_t { IT_THEME, IT_LAYOUT, IT_PIN, IT_BRIGHT, IT_LOCK, IT_SHUFFLE, IT_UNPLUG, IT_SORT };

        const char* const NAMES[ITEM_COUNT] = { "Theme", "USB keyboard", "Show PIN digits", "Brightness", "Auto-lock", "Shuffle PIN pad", "Off on unplug", "Sort" };
        const char* const LAYOUTS[2] = { "AZERTY", "QWERTY" };
        const char* const SORTS[2] = { "Recent", "A-Z" };
        const char* const ONOFF[2] = { "Off", "On" };
        const char* const BRIGHT[11] = { "0%", "10%", "20%", "30%", "40%", "50%", "60%", "70%", "80%", "90%", "100%" };
        const char* const LOCKS[4] = { "Never", "30 s", "1 min", "5 min" };
        const uint16_t LOCK_S[4] = { 0, 30, 60, 300 };
    }

    const char* itemName(uint8_t item) { return item < ITEM_COUNT ? NAMES[item] : ""; }

    bool itemIsSwitch(uint8_t item) { return item == IT_PIN || item == IT_SHUFFLE || item == IT_UNPLUG; }

    uint8_t optionCount(uint8_t item) {
        switch (item) {
            case IT_THEME: return THEME_COUNT;
            case IT_BRIGHT: return 11;
            case IT_LOCK: return 4;
            case IT_LAYOUT:
            case IT_PIN:
            case IT_SHUFFLE:
            case IT_UNPLUG:
            case IT_SORT: return 2;
            default: return 0;
        }
    }

    const char* optionLabel(uint8_t item, uint8_t k) {
        if (k >= optionCount(item)) return "";
        switch (item) {
            case IT_THEME:  return THEMES[k].name;
            case IT_LAYOUT: return LAYOUTS[k];
            case IT_SORT:   return SORTS[k];
            case IT_BRIGHT: return BRIGHT[k];
            case IT_LOCK:   return LOCKS[k];
            default:        return ONOFF[k];              // Switche: Off / On
        }
    }

    uint8_t optionCurrent(uint8_t item) {
        switch (item) {
            case IT_THEME:   return cur.theme;
            case IT_LAYOUT:  return cur.layout;
            case IT_PIN:     return cur.pinDigits ? 1 : 0;
            case IT_SHUFFLE: return cur.pinShuffle ? 1 : 0;
            case IT_UNPLUG:  return cur.offOnUnplug ? 1 : 0;
            case IT_SORT:    return cur.sortAlpha ? 1 : 0;
            case IT_BRIGHT:  return (uint8_t)((cur.brightness + 5) / 10);
            case IT_LOCK:
                for (uint8_t k = 0; k < 4; k++) if (LOCK_S[k] == cur.autoLock) return k;
                return 0;
            default: return 0;
        }
    }

    // Live preview (theme and brightness): apply the value, then restore the real state without saving it
    void preview(uint8_t item, uint8_t k) {
        if (k >= optionCount(item) || (item != IT_THEME && item != IT_BRIGHT)) return;
        const Values real = cur;
        if (item == IT_THEME) cur.theme = k; else cur.brightness = (uint8_t)(k * 10);
        apply();
        cur = real;
    }

    void revertPreview() { apply(); }

    bool applyOption(uint8_t item, uint8_t k) {
        if (k >= optionCount(item)) return false;
        Values v = cur;
        switch (item) {
            case IT_THEME:   v.theme = k; break;
            case IT_LAYOUT:  v.layout = k; break;
            case IT_PIN:     v.pinDigits = (k == 1); break;
            case IT_SHUFFLE: v.pinShuffle = (k == 1); break;
            case IT_UNPLUG:  v.offOnUnplug = (k == 1); break;
            case IT_SORT:    v.sortAlpha = (k == 1); break;
            case IT_BRIGHT:  v.brightness = (uint8_t)(k * 10); break;
            case IT_LOCK:    v.autoLock = LOCK_S[k]; break;
            default: return false;
        }
        return set(v);
    }
}
