// Hid.h : the device acts as a USB keyboard
#pragma once
#include <stdint.h>
#include <stddef.h>

namespace Hid {
    enum class Layout : uint8_t { Azerty, Qwerty };

    // False if this firmware is not built in USB "keyboard" mode (see platformio.ini).
    bool available();

    // True while a computer is plugged in USB and active; false if cable removed, simple charger or computer asleep.
    bool hostActive();

    void begin();                         // Starts USB (once, at boot)
    void setLayout(Layout l);             // Computer keyboard layout

    // Types the text. Returns the number of characters that don't exist in the chosen
    int type(const char* text, size_t len);

    // Character -> (modifiers, USB key code). false if impossible.
    bool encode(char c, Layout l, uint8_t& modifiers, uint8_t& usage);
}
