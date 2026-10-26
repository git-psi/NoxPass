#pragma once
#include <M5Unified.h>

namespace Ui {
    enum class Msg : uint8_t { Info, Error };

    void begin();
    M5Canvas& canvas();     // The image we draw into (screen size)
    void clear();           // Fills it with the theme background color
    void present();         // Shows it on the screen

    // "Info" or "Error" message in the middle of the screen. Blocking: stays Config::MESSAGE_MS (or until a press),
    // or, if waitKey, until a press. The calling screen must invalidate itself afterwards.
    void showMessage(Msg type, const char* text, bool waitKey = false);

    // Draws s, truncated with ".." to fit in maxW pixels.
    // Font, datum and colors must already be set.
    void drawTextFit(M5Canvas& c, const char* s, int16_t x, int16_t y, int16_t maxW);

    // Shows a message without waiting (before a long computation). The screen then stays frozen.
    void showBusy(const char* text);
}