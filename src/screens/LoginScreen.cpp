// LoginScreen.cpp : PIN input with the digit wheel (A = next, B = previous, A long = validate the
// digit, B long = erase, A+B = validate the PIN).
#include "LoginScreen.h"
#include "SecurityFlow.h"
#include "../core/Activity.h"
#include "../core/ScreenManager.h"
#include "../security/Auth.h"
#include "../security/KeyStore.h"
#include "../ui/Battery.h"
#include "../ui/Fonts.h"
#include "../ui/Logo.h"
#include "../ui/Theme.h"
#include "../ui/Ui.h"
#include "../vault/Vault.h"

void LoginScreen::reset() {
    memset(pin, 0, sizeof(pin));
    len = 0;
    cur = SecurityFlow::startDigit();
    invalidate();
}

void LoginScreen::onEnter() {
    firstRun = (KeyStore::state() == KeyStore::State::Missing);
    intro = firstRun ? 0 : 2;                         // First launch: welcome, then guide, then PIN choice
    confirming = false;
    memset(firstPin, 0, sizeof(firstPin));
    firstLen = 0;
    reset();
}

// Device unlocked: loads the vault, then opens the list.
void LoginScreen::enterVault() {
    if (Vault::load() != Vault::LoadResult::Ok) {
        Auth::lock();                                 // Leave nothing unlocked
        Vault::clear();
        Ui::showMessage(Ui::Msg::Error, "Vault damaged");
        reset();
        return;
    }
    reset();
    ScreenManager::go(ScreenId::List);
}

void LoginScreen::validate() {
    char msg[32];

    if (KeyStore::state() == KeyStore::State::Corrupt) {
        // NEVER overwrite an unreadable key file: data would be lost.
        Ui::showMessage(Ui::Msg::Error, "Key file damaged");
        reset();
        return;
    }
    if (len < Config::PIN_MIN || len > Config::PIN_MAX) {
        snprintf(msg, sizeof(msg), "PIN: %u to %u digits", Config::PIN_MIN, Config::PIN_MAX);
        Ui::showMessage(Ui::Msg::Error, msg);
        reset();
        return;
    }

    if (firstRun && !confirming) {                    // 1st input: remember it and ask to repeat it
        memcpy(firstPin, pin, sizeof(firstPin));
        firstLen = len;
        confirming = true;
        reset();
        return;
    }
    if (firstRun && (len != firstLen || memcmp(pin, firstPin, len) != 0)) {
        Ui::showMessage(Ui::Msg::Error, "The two PINs differ");
        confirming = false;
        memset(firstPin, 0, sizeof(firstPin));
        firstLen = 0;
        reset();
        return;
    }

    if (firstRun) {                                   // Vault creation
        memset(firstPin, 0, sizeof(firstPin));
        Ui::showBusy("Creating vault...");
        Auth::KeyBlob b;
        if (Auth::create(pin, len, b) && KeyStore::save(b)) {
            enterVault();
        } else {
            Auth::lock();
            Ui::showMessage(Ui::Msg::Error, "Save failed");
            reset();
        }
        return;
    }

    Ui::showBusy("Unlocking...");
    if (Auth::unlock(pin, len, KeyStore::blob())) {
        enterVault();
    } else {
        Ui::showMessage(Ui::Msg::Error, "Wrong PIN");
        reset();
    }
}

// Device reset: A+B held for a long time. Erases the vault, the key and the settings, then reboots.
// A short A+B press (Config::RESET_SHOW_MS) stays a simple PIN validation: we then return false.
bool LoginScreen::holdReset() {
    const uint32_t t0 = millis();
    auto both = [] { M5.update(); return M5.BtnA.isPressed() && M5.BtnB.isPressed(); };

    int lastX = -1;
    while (both() && millis() - t0 < Config::RESET_HOLD_MS) {
        const uint32_t dt = millis() - t0;
        const int x = (int)((uint64_t)Config::SCREEN_W * dt / Config::RESET_HOLD_MS);
        if (dt >= Config::RESET_SHOW_MS && x != lastX) {   // The press lasts: same screen as the Settings reset
            lastX = x;
            SecurityFlow::drawHold("Reset stick", "ALL data and the", "PIN will be erased.",
                                   "Keep holding A+B", "Release to cancel", x);
        }
        delay(30);
    }

    const uint32_t held = millis() - t0;
    if (held < Config::RESET_HOLD_MS) {               // Released before the end
        if (held < Config::RESET_SHOW_MS) return false;   // Short press: normal validation
        while (both()) delay(10);
        Activity::touch();                            // Waiting doesn't count as inactivity (otherwise power off)
        reset();                                      // Cancelled: back to the input
        return true;
    }

    SecurityFlow::wipeAndRestart();                   // Only returns if the erasing failed
    while (both()) delay(10);
    Activity::touch();
    reset();
    return true;
}

void LoginScreen::onInput(InputEvent e) {
    if (intro < 2) {                                  // Welcome, then guide: A goes forward, B goes back
        if (e == InputEvent::AClick || e == InputEvent::ALong || e == InputEvent::AB) { intro++; invalidate(); }
        else if (e == InputEvent::BClick && intro > 0)         { intro--; invalidate(); }
        return;
    }
    switch (e) {
        case InputEvent::AClick: cur = (cur + 1) % 10; break;
        case InputEvent::BClick: cur = (cur + 9) % 10; break;
        case InputEvent::BLong:
            if (len > 0) pin[--len] = 0;
            break;
        case InputEvent::ALong:
            if (len < Config::PIN_MAX) {
                pin[len++] = '0' + cur;
                cur = SecurityFlow::startDigit();
            } else {
                Ui::showMessage(Ui::Msg::Error, "PIN too long");
            }
            break;
        case InputEvent::AB:
            if (!holdReset()) validate();
            return;
        default: return;
    }
    invalidate();
}

void LoginScreen::update() {
    if (Battery::refresh()) invalidate();
}

// First launch welcome page: logo on the left, name on the right.
void LoginScreen::drawSplash(M5Canvas& c) {
    Logo::draw(c, 30, 16, Theme::fg, 2);      // 58x84, vertically centered above the bottom text
    Fonts::apply(c);
    c.setTextDatum(middle_center);
    c.setTextColor(Theme::fg, Theme::bg);
    c.setTextSize(1.5f);
    c.drawString("NoxPass", 170, 62);
    c.setTextSize(Fonts::SCALE);
    c.setTextColor(Theme::mid, Theme::bg);
    c.drawString("Press A to continue", Config::SCREEN_W / 2, 118);
}

// Guide page: white background and black QR code (this is what lets phones read it), whatever the theme.
void LoginScreen::drawGuide(M5Canvas& c) {
    c.fillSprite(TFT_WHITE);
    c.qrcode(Config::HELP_URL, 4, 11, 113);

    const char* shown = Config::HELP_SHOWN;              // Short link, cut in lines of 10 characters
    Fonts::apply(c);
    c.setTextDatum(middle_left);
    const int16_t x = 122;
    c.setTextColor(TFT_BLACK, TFT_WHITE);
    c.drawString("Guide", x, 22);
    const size_t n = strlen(shown);
    for (size_t i = 0, row = 0; i < n && row < 4; i += 10, row++) {
        char part[11];
        strncpy(part, shown + i, 10);
        part[10] = 0;
        c.drawString(part, x, 46 + row * 17);
    }
    c.drawString("A: next", x, 120);
}

void LoginScreen::draw(M5Canvas& c) {
    if (intro == 0) { drawSplash(c); return; }
    if (intro == 1) { drawGuide(c); return; }

    Battery::drawTopRight(c);

    if (firstRun) {
        Fonts::apply(c);
        c.setTextDatum(middle_center);
        c.setTextColor(Theme::mid, Theme::bg);
        c.drawString(confirming ? "Repeat PIN" : "Choose PIN", Config::SCREEN_W / 2, 22);
    }
    SecurityFlow::drawPinPad(c, pin, len, cur);
}
