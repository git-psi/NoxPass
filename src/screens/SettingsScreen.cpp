// SettingsScreen.cpp : settings screen (switches, values lists, Change PIN / Reset actions).
#include "SettingsScreen.h"
#include "../core/ScreenManager.h"
#include "../core/Settings.h"
#include "../ui/Ui.h"
#include "../ui/Theme.h"
#include "../ui/Fonts.h"
#include "SecurityFlow.h"

namespace {
constexpr uint8_t VISIBLE = 5;
constexpr int16_t ROW_H   = 25;
constexpr int16_t FIRST_Y = 17;     // Center of the 1st row
constexpr int16_t RIGHT_X = 226;    // Right edge of the values
constexpr int16_t NAME_W_SWITCH = 172;   // Name of a switch setting (up to ~15 characters)
constexpr int16_t NAME_W_VALUE  = 120;   // Name of a setting whose value is text (shown on the right)
constexpr uint8_t ACTIONS = 3;      // Action rows after the settings
const char* const ACTION_NAMES[ACTIONS] = { "Import/Export", "Change PIN", "Reset stick" };

// Rounded switch, like on the website: off = outline + square on the left, on = filled + square on the right.
void drawSwitch(M5Canvas& c, int16_t right, int16_t cy, bool on, uint16_t fg, uint16_t bg) {
    const int16_t w = 38, h = 18, x = right - w, y = cy - h / 2;
    if (on) {
        c.fillRoundRect(x, y, w, h, 4, fg);
        c.fillRoundRect(x + w - 15, y + 3, 12, h - 6, 2, bg);
    } else {
        c.drawRoundRect(x, y, w, h, 4, fg);
        c.fillRoundRect(x + 3, y + 3, 12, h - 6, 2, fg);
    }
}
}

uint8_t SettingsScreen::rows() const {
    return inChoice ? Settings::optionCount(item) : Settings::ITEM_COUNT + ACTIONS;
}

void SettingsScreen::select(uint8_t row) {
    sel = row;
    if (sel < top) top = sel;
    else if (sel >= top + VISIBLE) top = sel - VISIBLE + 1;
}

// Theme and brightness apply to the screen as soon as they're hovered; nothing is saved before A long.
void SettingsScreen::previewRow() {
    if (inChoice) Settings::preview(item, sel);
}

void SettingsScreen::onLeave() { Settings::revertPreview(); }

void SettingsScreen::onEnter() {
    inChoice = false;
    item = 0; sel = 0; top = 0;
    invalidate();
}

void SettingsScreen::onInput(InputEvent e) {
    if (e == InputEvent::AB) e = InputEvent::ALong;      // A+B validates, like A long
    const uint8_t n = rows();
    switch (e) {
        case InputEvent::AClick: select((sel + 1) % n); previewRow(); break;
        case InputEvent::BClick: select((sel + n - 1) % n); previewRow(); break;

        case InputEvent::BLong:
            if (inChoice) {                         // Cancel: back to the settings list, preview undone
                Settings::revertPreview();
                inChoice = false;
                select(item);
            } else {
                ScreenManager::go(ScreenId::Apps);
                return;
            }
            break;

        case InputEvent::ALong:
            if (!inChoice) {
                item = sel;
                if (sel >= Settings::ITEM_COUNT) {                   // Action
                    const uint8_t act = sel - Settings::ITEM_COUNT;
                    if (act == 0)      Ui::showMessage(Ui::Msg::Info, "Use Bridge: open the site, then Settings > Import / Export", true);
                    else if (act == 1) SecurityFlow::changePin();
                    else               SecurityFlow::factoryReset();
                } else if (Settings::itemIsSwitch(item)) {                  // Directly on/off
                    if (!Settings::applyOption(item, Settings::optionCurrent(item) ? 0 : 1)) {
                        Ui::showMessage(Ui::Msg::Error, "Not saved");
                    }
                } else {                                             // List of possible values
                    inChoice = true;
                    top = 0;
                    select(Settings::optionCurrent(item));   // The current value: nothing to preview
                }
            } else {
                if (!Settings::applyOption(item, sel)) Ui::showMessage(Ui::Msg::Error, "Not saved");
                inChoice = false;
                select(item);
            }
            break;

        default: return;
    }
    invalidate();
}

void SettingsScreen::draw(M5Canvas& c) {
    Fonts::apply(c);
    c.setTextDatum(middle_left);
    const uint8_t n = rows();

    for (uint8_t row = 0; row < VISIBLE; row++) {
        const uint8_t idx = top + row;
        if (idx >= n) break;

        const int16_t cy = FIRST_Y + row * ROW_H;
        const bool on = (idx == sel);
        const uint16_t fg = on ? Theme::bg : Theme::fg;     // Text
        const uint16_t bg = on ? Theme::fg : Theme::bg;     // Row background
        if (on) c.fillRoundRect(7, cy - 11, 226, 22, 5, Theme::fg);
        c.setTextColor(fg, bg);

        if (!inChoice && idx >= Settings::ITEM_COUNT) {
            c.setTextDatum(middle_left);
            c.drawString(ACTION_NAMES[idx - Settings::ITEM_COUNT], 13, cy);
            c.setTextDatum(middle_right);
            c.drawString(">", RIGHT_X, cy);
        } else if (!inChoice) {
            c.setTextDatum(middle_left);
            // Max width of the name (pixels): a switch (38 px, on the right) leaves more room than a text value.
            Ui::drawTextFit(c, Settings::itemName(idx), 13, cy, Settings::itemIsSwitch(idx) ? NAME_W_SWITCH : NAME_W_VALUE);
            if (Settings::itemIsSwitch(idx)) {
                drawSwitch(c, RIGHT_X, cy, Settings::optionCurrent(idx) == 1, fg, bg);
            } else {
                c.setTextDatum(middle_right);
                c.drawString(Settings::optionLabel(idx, Settings::optionCurrent(idx)), RIGHT_X, cy);
            }
        } else {
            c.setTextDatum(middle_left);
            Ui::drawTextFit(c, Settings::optionLabel(item, idx), 13, cy, 190);
            if (idx == Settings::optionCurrent(item)) c.fillCircle(RIGHT_X - 4, cy, 4, fg);   // Current value
        }
    }
}
