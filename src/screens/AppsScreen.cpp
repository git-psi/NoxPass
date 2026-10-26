// AppsScreen.cpp : apps menu (a row of squares with a letter).
#include "AppsScreen.h"
#include "../config.h"
#include "../core/Power.h"
#include "../core/ScreenManager.h"
#include "../ui/Battery.h"
#include "../ui/Fonts.h"
#include "../ui/Theme.h"

namespace {
struct App { char letter; const char* name; };
const App APPS[] = {
    { 'P', "Password" },
    { 'B', "Bridge"   },
    { 'S', "Settings" },
    { 'Q', "Quit"     },
};
constexpr uint8_t N = sizeof(APPS) / sizeof(APPS[0]);
}

void AppsScreen::onEnter() { sel = 0; invalidate(); }

void AppsScreen::onInput(InputEvent e) {
    if (e == InputEvent::AB) e = InputEvent::ALong;      // A+B validates, like A long
    switch (e) {
        case InputEvent::AClick: sel = (sel + 1) % N; break;
        case InputEvent::BClick: sel = (sel + N - 1) % N; break;
        case InputEvent::BLong:  Power::shutdown(); return;
        case InputEvent::ALong:
            switch (APPS[sel].letter) {
                case 'P': ScreenManager::go(ScreenId::List); break;
                case 'B': ScreenManager::go(ScreenId::Bridge); break;
                case 'S': ScreenManager::go(ScreenId::Settings); break;
                case 'Q': Power::shutdown(); break;
            }
            invalidate();
            return;
        default: return;
    }
    invalidate();
}

void AppsScreen::update() {
    if (Battery::refresh()) invalidate();
}

void AppsScreen::draw(M5Canvas& c) {
    const int size  = 27, gap = 9;
    const int total = N * size + (N - 1) * gap;
    const int x0    = (Config::SCREEN_W - total) / 2;
    const int y0    = 61;

    Battery::drawTopRight(c);

    Fonts::apply(c);
    c.setTextDatum(middle_center);
    c.setTextColor(Theme::fg, Theme::bg);
    c.drawString(APPS[sel].name, Config::SCREEN_W / 2, 44);

    for (uint8_t i = 0; i < N; i++) {
        int x = x0 + i * (size + gap);
        char s[2] = { APPS[i].letter, 0 };
        if (i == sel) {
            c.fillRoundRect(x, y0, size, size, 4, Theme::fg);
            c.setTextColor(Theme::bg, Theme::fg);
        } else {
            c.drawRoundRect(x, y0, size, size, 4, Theme::fg);
            c.setTextColor(Theme::fg, Theme::bg);
        }
        c.drawString(s, x + size / 2, y0 + size / 2);
    }
}