// Battery.cpp : small 5-level battery icon, with "+" when the stick is charging.
#include "Battery.h"
#include "Theme.h"
#include "../config.h"

namespace Battery {

static uint8_t  level    = 0;      // 0..4 (5 states)
static bool     charging = false;
static uint32_t lastT    = 0;
static bool     first    = true;

bool refresh() {
    uint32_t now = millis();
    if (!first && now - lastT < 5000) return false;
    first = false;
    lastT = now;

    int pct = M5.Power.getBatteryLevel();
    uint8_t newLevel = constrain(pct / 20, 0, 4);
    bool newCharging = (M5.Power.isCharging() == M5.Power.is_charging);

    bool changed = (newLevel != level) || (newCharging != charging);
    level = newLevel;
    charging = newCharging;
    return changed;
}

void draw(M5Canvas& c, int16_t x, int16_t y) {
    const int w = 18, h = 9;                              // Body
    c.drawRoundRect(x, y, w, h, 2, Theme::fg);
    c.fillRect(x + w, y + 3, 2, h - 6, Theme::fg);        // Terminal

    // Inside: 1 px border + 1 px margin on each side => 14 px max
    const int fillMax = w - 4;
    const int fillW   = fillMax * (level + 1) / 5;        // 2, 5, 8, 11, 14
    c.fillRect(x + 2, y + 2, fillW, h - 4, Theme::fg);

    if (charging) {                                       // Small "+" on the left
        c.drawFastHLine(x - 9, y + h / 2, 5, Theme::fg);
        c.drawFastVLine(x - 7, y + h / 2 - 2, 5, Theme::fg);
    }
}

void drawTopRight(M5Canvas& c) {
    draw(c, Config::SCREEN_W - 31, 8);
}

}