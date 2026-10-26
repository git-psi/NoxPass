// SecurityFlow.cpp : PIN change, reset, and PIN input (wheel).
#include "SecurityFlow.h"
#include "../config.h"
#include "../core/Activity.h"
#include "../core/Input.h"
#include "../core/Settings.h"
#include "../security/Auth.h"
#include "../security/Crypto.h"
#include "../security/KeyStore.h"
#include "../storage/Storage.h"
#include "../ui/Fonts.h"
#include "../ui/Theme.h"
#include "../ui/Ui.h"
#include "../vault/Vault.h"
#include <Arduino.h>
#include <esp_random.h>
#include <stdio.h>
#include <string.h>

namespace {

constexpr uint32_t HOLD_MS = 3000;     // How long A must be held to confirm

// Validation screen: hold A for 3 s. The whole screen fills from left to right.
// true = held until the end. false = cancelled (B).
bool holdToConfirm(const char* title, const char* l1, const char* l2) {
    // The screen shows right away. The long press that opened it (A still pressed) doesn't count:
    // the gauge only starts on a new press.
    bool armed = !M5.BtnA.isPressed();
    bool holding = false;
    uint32_t t0 = 0;
    int lastX = -1;
    for (;;) {
        const InputEvent e = Input::poll();
        if (e == InputEvent::BClick || e == InputEvent::BLong) return false;
        Activity::touch();

        bool down = M5.BtnA.isPressed();
        if (!armed) { if (!down) armed = true; down = false; }       // First wait for the release of the original press
        if (down && !holding) { holding = true; t0 = millis(); }
        if (!down) holding = false;
        const uint32_t held = holding ? millis() - t0 : 0;
        if (held >= HOLD_MS) return true;

        const int x = (int)((uint64_t)Config::SCREEN_W * held / HOLD_MS);   // Width already filled
        if (x != lastX) {                                                   // Redraw only if the gauge moved
            lastX = x;
            SecurityFlow::drawHold(title, l1, l2, "Hold A for 3 s", "B: cancel", x);
        }
        delay(15);
    }
}

// PIN input, like on the login screen: A = next digit, B = previous,
// A long = validate the digit, B long = erase (cancels if nothing to erase), A+B = finish.
// true = PIN typed (pin/len filled), false = cancelled.
bool readPin(const char* title, char* pin, uint8_t& len) {
    memset(pin, 0, Config::PIN_MAX + 1);
    len = 0;
    uint8_t cur = SecurityFlow::startDigit();
    bool redraw = true;
    for (;;) {
        if (redraw) {
            redraw = false;
            M5Canvas& c = Ui::canvas();
            Ui::clear();
            Fonts::apply(c);
            c.setTextDatum(middle_center);
            c.setTextColor(Theme::mid, Theme::bg);
            c.drawString(title, Config::SCREEN_W / 2, 22);
            SecurityFlow::drawPinPad(c, pin, len, cur);
            Ui::present();
        }
        const InputEvent e = Input::poll();
        Activity::touch();
        switch (e) {
            case InputEvent::AClick: cur = (cur + 1) % 10; redraw = true; break;
            case InputEvent::BClick: cur = (cur + 9) % 10; redraw = true; break;
            case InputEvent::ALong:
                if (len < Config::PIN_MAX) { pin[len++] = '0' + cur; cur = SecurityFlow::startDigit(); redraw = true; }
                break;
            case InputEvent::BLong:
                if (len == 0) return false;
                pin[--len] = 0; redraw = true; break;
            case InputEvent::AB:
                if (len >= Config::PIN_MIN) return true;
                { char m[32]; snprintf(m, sizeof(m), "PIN: %u to %u digits", Config::PIN_MIN, Config::PIN_MAX);
                  Ui::showMessage(Ui::Msg::Error, m); redraw = true; }
                break;
            default: break;
        }
        delay(10);
    }
}

}  // namespace

namespace SecurityFlow {

uint8_t startDigit() { return Settings::get().pinShuffle ? (uint8_t)(esp_random() % 10) : 0; }

void drawHold(const char* title, const char* l1, const char* l2, const char* l3, const char* l4, int x) {
    const int W = Config::SCREEN_W, H = Config::SCREEN_H;
    M5Canvas& c = Ui::canvas();
    Ui::clear();
    if (x > 0) c.fillRect(0, 0, x, H, Theme::fg);
    Fonts::apply(c);
    c.setTextDatum(middle_center);
    auto lines = [&](uint16_t color) {
        c.setTextColor(color);
        c.drawString(title, W / 2, 18);
        c.drawString(l1, W / 2, 50);
        c.drawString(l2, W / 2, 72);
        c.drawString(l3, W / 2, 102);
        c.drawString(l4, W / 2, 122);
    };
    if (x < W) { c.setClipRect(x, 0, W - x, H); lines(Theme::fg); }       // Empty part: light text on background
    if (x > 0) { c.setClipRect(0, 0, x, H);     lines(Theme::bg); }       // Filled part: inverted text
    c.clearClipRect();
    Ui::present();
}

void drawPinPad(M5Canvas& c, const char* pin, uint8_t len, uint8_t cur) {
    const int cx    = Config::SCREEN_W / 2;
    const int pinY  = 67;     // Vertical middle: validated digits
    const int rowY  = 107;    // Row of 5 digits
    const int rowDx = 20;     // Gap between two digits of the row

    Fonts::apply(c);
    c.setTextDatum(middle_center);

    // Validated digits: centered group, spacing shrinks if the PIN is long
    if (len > 0) {
        const int step = min(22, 220 / len);
        const int x0   = cx - (len - 1) * step / 2;
        c.setTextColor(Theme::fg, Theme::bg);
        for (uint8_t i = 0; i < len; i++) {
            char s[2] = { Settings::get().pinDigits ? pin[i] : '*', 0 };
            c.drawString(s, x0 + i * step, pinY);
        }
    }

    // Row of 5: more and more faded away from the current digit (next on the right, previous on the left)
    const uint16_t colors[5] = { Theme::dim, Theme::mid, Theme::fg, Theme::mid, Theme::dim };
    for (int k = 0; k < 5; k++) {
        const int d = k - 2;                          // -2..+2, negative = left
        char s[2] = { (char)('0' + (cur + 10 + d) % 10), 0 };
        c.setTextColor(colors[k], Theme::bg);
        c.drawString(s, cx + d * rowDx, rowY);
    }
    c.drawRoundRect(cx - 10, rowY - 10, 20, 20, 4, Theme::fg);   // Frame of the current digit
}

void changePin() {
    char p1[Config::PIN_MAX + 1], p2[Config::PIN_MAX + 1];
    uint8_t n1 = 0, n2 = 0;
    if (readPin("New PIN", p1, n1) && readPin("Repeat new PIN", p2, n2)) {
        if (n1 != n2 || memcmp(p1, p2, n1) != 0) {
            Ui::showMessage(Ui::Msg::Error, "The two PINs differ");
        } else if (holdToConfirm("Change PIN", "The PIN will be", "replaced.")) {
            Ui::showBusy("Changing PIN...");
            Auth::KeyBlob b;
            const bool ok = Auth::changePin(p1, n1, b) && KeyStore::save(b);   // New encryption key of the master key
            Ui::showMessage(ok ? Ui::Msg::Info : Ui::Msg::Error, ok ? "PIN changed" : "Save failed");
        }
    }
    Crypto::wipe(p1, sizeof(p1));                     // PINs don't stay in RAM
    Crypto::wipe(p2, sizeof(p2));
}

void wipeAndRestart() {
    Ui::showBusy("Erasing...");
    Vault::clear();
    Auth::lock();
    if (!Storage::wipeAll()) { Ui::showMessage(Ui::Msg::Error, "Erase failed", true); return; }
    Ui::showBusy("Reset done");
    delay(1200);
    ESP.restart();                                    // Restarts like a new stick: choose a new PIN
}

void factoryReset() {
    if (!holdToConfirm("Reset stick", "ALL data and the", "PIN will be erased.")) return;
    wipeAndRestart();
}

}
