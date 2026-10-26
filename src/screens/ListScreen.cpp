// ListScreen.cpp : passwords list ("Add" row first, then entries sorted by the Sort setting).
#include "ListScreen.h"
#include "../core/ScreenManager.h"
#include "../ui/Ui.h"
#include "../ui/Theme.h"
#include "../ui/Fonts.h"
#include "../vault/Vault.h"

namespace {
constexpr uint8_t VISIBLE = 5;
constexpr int16_t ROW_H   = 25;
constexpr int16_t FIRST_Y = 17;    // Center of the 1st row

inline uint16_t total() { return (uint16_t)(Vault::count() + 1); }   // +1 = Add button
}

void ListScreen::onEnter() {
    sel = 0;       // The position is not kept (see Projet.md)
    top = 0;
    const uint32_t focus = Vault::takeFocus();      // except right after a creation
    if (focus != 0) {
        for (size_t i = 0; i < Vault::count(); i++) {
            if (Vault::at(i)->id == focus) {
                sel = (uint16_t)(i + 1);
                top = (sel >= VISIBLE) ? sel - VISIBLE + 1 : 0;
                break;
            }
        }
    }
    invalidate();
}

void ListScreen::onInput(InputEvent e) {
    if (e == InputEvent::AB) e = InputEvent::ALong;      // A+B validates, like A long
    const uint16_t count = total();
    switch (e) {
        case InputEvent::AClick:
            sel = (sel + 1) % count;
            if (sel == 0) top = 0;
            else if (sel >= top + VISIBLE) top = sel - VISIBLE + 1;
            break;
        case InputEvent::BClick:
            sel = (sel + count - 1) % count;
            if (sel == count - 1) top = (count > VISIBLE) ? count - VISIBLE : 0;
            else if (sel < top) top = sel;
            break;
        case InputEvent::BLong:
            ScreenManager::go(ScreenId::Apps);
            return;
        case InputEvent::ALong: {
            if (sel == 0) {
                ScreenManager::go(ScreenId::Create);
                return;
            }
            const Vault::Item* en = Vault::at(sel - 1);
            if (!en) break;
            const uint32_t openId = en->id;
            Vault::markUsed(openId);                       // Opening an entry makes it the most recent
            Vault::select(openId);
            ScreenManager::go(ScreenId::Entry);
            return;
        }
        default: return;
    }
    invalidate();
}

void ListScreen::draw(M5Canvas& c) {
    Fonts::apply(c);
    c.setTextDatum(middle_left);
    const uint16_t count = total();

    for (uint8_t row = 0; row < VISIBLE; row++) {
        uint16_t idx = top + row;
        if (idx >= count) break;

        int cy = FIRST_Y + row * ROW_H;
        if (idx == sel) {
            c.fillRoundRect(7, cy - 11, 226, 22, 5, Theme::fg);
            c.setTextColor(Theme::bg, Theme::fg);
        } else {
            c.setTextColor(Theme::fg, Theme::bg);
        }

        if (idx == 0) {
            c.drawString("+ Add Password", 23, cy);
        } else if (const Vault::Item* en = Vault::at(idx - 1)) {
            Ui::drawTextFit(c, en->title, 13, cy, 212);
        }
    }
}
