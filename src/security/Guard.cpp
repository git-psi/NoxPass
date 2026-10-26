#include "Guard.h"
#include "../storage/Storage.h"
#include <Arduino.h>

namespace Guard {

namespace {
    constexpr const char* PATH = "/tries.bin";
    constexpr uint8_t  FREE_FAILS = 3;       // les 3 premiers essais ratés n'entraînent aucune attente
    constexpr uint32_t BASE_WAIT  = 15;      // puis 15 s, 30 s, 60 s, ... plafonné
    constexpr uint32_t MAX_WAIT   = 900;     // 15 min

    uint8_t  count = 0;                      // essais commencés depuis la dernière réussite
    uint32_t timerStartMs = 0;

    uint32_t waitFor(uint8_t c) {
        if (c <= FREE_FAILS) return 0;
        uint32_t shift = c - FREE_FAILS - 1;
        if (shift > 6) shift = 6;
        uint32_t w = BASE_WAIT << shift;
        return w > MAX_WAIT ? MAX_WAIT : w;
    }

    void save() {
        uint8_t b[2] = { count, (uint8_t)~count };
        Storage::writeAtomic(PATH, b, sizeof(b));
    }
}

void begin() {
    uint8_t b[2] = {0, 0};
    switch (Storage::readExact(PATH, b, sizeof(b))) {
        case Storage::ReadResult::Ok:      count = (b[1] == (uint8_t)~b[0]) ? b[0] : 10; break;
        case Storage::ReadResult::Missing: count = 0;  break;
        default:                           count = 10; break;   // fichier abîmé : on reste prudent
    }
    timerStartMs = millis();
}

uint32_t remainingSeconds() {
    uint32_t need = waitFor(count);
    uint32_t elapsed = (millis() - timerStartMs) / 1000;
    return elapsed >= need ? 0 : need - elapsed;
}

void attemptStarted()   { if (count < 255) count++; save(); }
void attemptSucceeded() { if (count != 0) { count = 0; save(); } }
void attemptFailed()    { timerStartMs = millis(); }

}