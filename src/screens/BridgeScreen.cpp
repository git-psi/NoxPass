// BridgeScreen.cpp : Wi-Fi screen (QR code, device acceptance, connection state). The server is in net/Web.
#include "BridgeScreen.h"
#include "../core/ScreenManager.h"
#include "../ui/Ui.h"
#include "../ui/Theme.h"
#include "../ui/Fonts.h"
#include "../config.h"
#include "../net/Web.h"
#include "../core/Activity.h"
#include <string.h>

namespace {
constexpr int16_t CX = Config::SCREEN_W / 2;
}

void BridgeScreen::onEnter() {
    Ui::showBusy("Starting Wi-Fi");
    if (!Bridge::start()) {
        Ui::showMessage(Ui::Msg::Error, "Wi-Fi failed");
        ScreenManager::go(ScreenId::Apps);
        return;
    }
    Web::begin();
    Activity::touch();
    shown = Bridge::state();
    lastInfo[0] = 0;
    lastLabel[0] = 0;
    invalidate();
}

void BridgeScreen::onLeave() { Web::end(); Bridge::stop(); }

void BridgeScreen::onInput(InputEvent e) {
    if (e == InputEvent::BLong) { ScreenManager::go(ScreenId::Apps); return; }

    switch (Bridge::state()) {
        case Bridge::State::Pending:
            if (e == InputEvent::AClick)      Bridge::accept();
            else if (e == InputEvent::BClick) Bridge::drop();     // Refuse
            break;
        case Bridge::State::Connected:
            if (e == InputEvent::BClick)      Bridge::drop();     // Cut, start from scratch
            break;
        default: break;
    }
    // The state is re-read in update()
}

void BridgeScreen::update() {
    Activity::touch();                            // Bridge active: no auto-lock (managed from the other device)
    Web::loop();
    Bridge::poll();
    if (Web::consumeChanged()) invalidate();         // Theme or settings changed from the website

    if (Bridge::consumeLost()) {                 // The accepted device left
        ScreenManager::go(ScreenId::Apps);
        return;
    }

    const Bridge::State s = Bridge::state();
    const char* inf = Bridge::info();
    const char* lab = Bridge::peerLabel();
    if (!inf) inf = "";

    if (s != shown || strcmp(inf, lastInfo) != 0 || strcmp(lab, lastLabel) != 0) {
        shown = s;
        strncpy(lastInfo, inf, sizeof(lastInfo) - 1);   lastInfo[sizeof(lastInfo) - 1] = 0;
        strncpy(lastLabel, lab, sizeof(lastLabel) - 1); lastLabel[sizeof(lastLabel) - 1] = 0;
        invalidate();
    }
}

void BridgeScreen::draw(M5Canvas& c) {
    Fonts::apply(c);
    c.setTextDatum(middle_center);

    switch (Bridge::state()) {
        case Bridge::State::Waiting: {
            // Fixed black on white page, independent of the theme: this is what lets scanners read the QR.
            c.fillSprite(0xFFFF);
            c.qrcode(Bridge::qrPayload(), 4, 11, 113);

            const int16_t x = 122, w = Config::SCREEN_W - x - 4;
            c.setTextDatum(middle_left);
            c.setTextColor(0x0000, 0xFFFF);
            c.drawString("Wi-Fi", x, 26);
            c.drawString("Password", x, 76);
            Ui::drawTextFit(c, Bridge::ssid(), x, 46, w);
            Ui::drawTextFit(c, Bridge::password(), x, 96, w);
            break;
        }
        case Bridge::State::Pending:
            c.setTextColor(Theme::fg, Theme::bg);
            c.drawString("New device", CX, 24);
            c.drawFastHLine(8, 36, Config::SCREEN_W - 16, Theme::fg);
            Ui::drawTextFit(c, Bridge::peerLabel(), CX, 66, Config::SCREEN_W - 16);
            c.setTextColor(Theme::mid, Theme::bg);
            c.drawString("A: Accept", CX, 98);
            c.drawString("B: Refuse", CX, 118);
            break;

        case Bridge::State::Connected: {
            c.setTextColor(Theme::fg, Theme::bg);
            c.drawString("Connected", CX, 24);
            c.drawFastHLine(8, 36, Config::SCREEN_W - 16, Theme::fg);
            c.setTextColor(Theme::mid, Theme::bg);
            Ui::drawTextFit(c, Bridge::peerLabel(), CX, 54, Config::SCREEN_W - 16);

            const char* inf = Bridge::info();
            if (inf) {
                c.setTextColor(Theme::fg, Theme::bg);
                Ui::drawTextFit(c, inf, CX, 86, Config::SCREEN_W - 16);
            }
            c.setTextColor(Theme::mid, Theme::bg);
            c.drawString("B: Disconnect", CX, 118);
            break;
        }
        default: break;
    }
}
