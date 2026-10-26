// Settings.h : device settings (theme, brightness, USB keyboard, PIN, lock, sort...).
// Stored in CLEAR in /settings.bin: they must be readable before the PIN is typed and are not secret.
#pragma once
#include <stdint.h>

namespace Settings {

    constexpr const char* PATH = "/settings.bin";

    struct Values {
        uint8_t  theme;        // Index in the theme table (0 = dark, 1 = light, ...)
        uint8_t  layout;       // 0 = AZERTY, 1 = QWERTY (computer keyboard)
        bool     pinDigits;    // true = PIN digits visible
        bool     sortAlpha;    // false = list sorted by recent use, true = alphabetical
        bool     offOnUnplug;  // true = the stick powers off (with animation) when unplugged from USB
        bool     pinShuffle;   // true = the PIN wheel starts on a random digit at each input
        uint8_t  brightness;   // 0..100 (%)
        uint16_t autoLock;     // Seconds of inactivity before lock: 0 (never), 30, 60 or 300
    };

    // ---- Themes: ONE table (Settings.cpp), used by the stick Settings screen AND sent to the website.
    constexpr uint8_t THEME_COUNT = 36;
    const char* themeKey(uint8_t i);          // "dark", "light", ... (nullptr if out of table)
    const char* themeName(uint8_t i);         // "Dark", "Light", ...
    uint32_t    themeBg(uint8_t i);           // 0xRRGGBB
    uint32_t    themeFg(uint8_t i);
    int         themeFromKey(const char* k);  // -1 if unknown

    // ---- Stick Settings screen: list of settings and their possible values.
    // A switch is a setting with 2 values (0 = Off, 1 = On).
    constexpr uint8_t ITEM_COUNT = 8;
    const char* itemName(uint8_t item);
    bool        itemIsSwitch(uint8_t item);
    uint8_t     optionCount(uint8_t item);
    const char* optionLabel(uint8_t item, uint8_t option);
    uint8_t     optionCurrent(uint8_t item);
    bool        applyOption(uint8_t item, uint8_t option);   // Apply + save; false if the write fails
    // Live preview while browsing values (theme and brightness only): applies WITHOUT saving.
    void        preview(uint8_t item, uint8_t option);
    void        revertPreview();                              // Back to the really saved settings

    bool          valid(const Values& v);
    const Values& get();

    // Reads /settings.bin (defaults if missing or unreadable), then applies: theme, brightness and keyboard.
    void load();

    // Validates, applies, saves (and re-sorts the list if the sort changed). false if invalid, or if the write
    // failed (values stay applied).
    bool set(const Values& v);
}
