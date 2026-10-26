// PowerKey.cpp : double click detection on the side button (see PowerKey.h)
#include "PowerKey.h"
#include "../config.h"
#include <M5Unified.h>
#include <M5PM1.h>
#include "Power.h"

namespace PowerKey {

namespace {
    M5PM1    pm1;                // The chip power supply
    bool     active   = false;
    bool     wasDown  = false;
    uint32_t lastClick = 0;

    constexpr uint32_t DOUBLE_MS   = 450;

    template <class P> auto readKey(P& p, int) -> decltype(p.btnGetState((bool*)nullptr), bool()) {
        bool v = false; p.btnGetState(&v); return v;
    }
    template <class P> auto readKey(P& p, long) -> decltype(p.btnGetState((uint8_t*)nullptr), bool()) {
        uint8_t v = 0; p.btnGetState(&v); return v != 0;
    }
    template <class P> bool readKey(P&, ...) { return M5.BtnPWR.isPressed(); }
    }

    void begin() {
        pm1.begin(&M5.In_I2C);
        if (!Config::POWER_KEY_ANIMATION) return;
        pm1.setDoubleOffDisable(true);       // Disable the chip shutdown to use our
        active = true;
    }

    void poll() {
        if (!active) return;
        const bool down = readKey(pm1, 0);
        const uint32_t now = millis();
        if (!down && wasDown) {
            if (lastClick != 0 && now - lastClick <= DOUBLE_MS) {
                lastClick = 0;
                Power::shutdown();
            }
            lastClick = now;
        }
        wasDown = down;
    }

}
