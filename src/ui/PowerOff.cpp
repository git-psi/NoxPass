// PowerOff.cpp : owl power off animation. See PowerOff.h.
#include "PowerOff.h"
#include <M5Unified.h>
#include <esp_sleep.h>
#include "Logo.h"
#include "Theme.h"
#include "Ui.h"
#include "../config.h"
#include "../core/Settings.h"

namespace PowerOff {

void run() {
    constexpr int SCALE = 3;                                   // 29 x 3 = 87 px wide, 42 x 3 = 126 px tall
    const int x = (Config::SCREEN_W - Logo::W * SCALE) / 2;
    const int y = (Config::SCREEN_H - Logo::H * SCALE) / 2;

    // Ceiling = brightness of the setting (never higher). At 0 %, the backlight is at minimum (1): the animation stays visible.
    const uint8_t pct   = Settings::get().brightness;
    const int     start = pct == 0 ? 1 : (pct * 255 + 50) / 100;

    M5Canvas& c = Ui::canvas();
    const uint32_t t0 = millis();
    int lastClosed = -1;
    for (;;) {
        uint32_t dt = millis() - t0;
        if (dt > DURATION_MS) dt = DURATION_MS;

        // The eyes close in CLOSE_MS (smooth start and end), then stay closed
        // while the brightness finishes going down.
        uint32_t cl = (dt * 1000UL) / CLOSE_MS;                            // 0..1000
        if (cl > 1000) cl = 1000;
        const uint32_t eased = cl * cl * (3000UL - 2UL * cl) / 1000000UL;  // Smoothing: 3x² - 2x³, over 0..1000
        const uint8_t closed = (uint8_t)((eased * 255UL) / 1000UL);
        if (closed != lastClosed) {                            // Redraw only if the eyes moved
            Ui::clear();
            Logo::drawClosing(c, x, y, Theme::fg, Theme::bg, SCALE, closed);
            Ui::present();
            lastClosed = closed;
        }

        // Fixed ramp 100 % -> 0 over the whole duration, capped at the setting brightness:
        // at 50 %, it stays at 50 % during the 1st half then goes down to 0 during the 2nd (same duration whatever the value).
        const int ramp = (int)((255UL * (DURATION_MS - dt)) / DURATION_MS);
        M5.Display.setBrightness((uint8_t)(ramp < start ? ramp : start));
        if (dt >= DURATION_MS) break;
        delay(10);
    }

    M5.Display.setBrightness(0);
    M5.Display.sleep();
    M5.Power.powerOff();

    // If the chip couldn't cut the power (for example the stick is powered by USB), we don't stay
    // on with a black screen: deep sleep (minimal consumption) until the next restart.
    delay(200);
    esp_deep_sleep_start();
}

}
