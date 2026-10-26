// SecurityFlow.h : sensitive actions (blocking, like Ui::showMessage) and common PIN input elements.
//   changePin()      : new PIN (typed twice), then validation by holding A 3 s
//   factoryReset()   : erases EVERYTHING (vault, PIN, settings), validation by holding A 3 s, then reboots
//   wipeAndRestart() : the erasing itself (also used by the login screen reset)
// The calling screen must invalidate itself afterwards.
#pragma once
#include <M5Unified.h>

namespace SecurityFlow {
    void changePin();
    void factoryReset();

    // Erases everything then reboots (never returns). Only returns if the erasing failed (error message shown).
    void wipeAndRestart();

    // Gauge screen (the whole screen fills from left to right up to x pixels, text inverted on the filled part).
    // Used by the Settings reset (hold A) and the login screen reset (hold A+B).
    void drawHold(const char* title, const char* l1, const char* l2, const char* l3, const char* l4, int x);

    // PIN wheel: digit it starts on (0, or random if the "Shuffle PIN pad" setting is on).
    uint8_t startDigit();

    // Draws the validated digits (in the middle) and the 5 digits wheel centered on `cur` (at the bottom).
    // pin[0..len-1]: validated digits. Fonts and colors are set here.
    void drawPinPad(M5Canvas& c, const char* pin, uint8_t len, uint8_t cur);
}
