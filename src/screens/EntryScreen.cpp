// EntryScreen.cpp : detail of an entry (fields, clear display, USB typing, deletion).
#include "EntryScreen.h"
#include "../core/ScreenManager.h"
#include "../ui/Ui.h"
#include "../ui/Theme.h"
#include "../ui/Fonts.h"
#include "../vault/Vault.h"
#include "../hid/Hid.h"
#include "../config.h"

namespace {

constexpr int16_t ROW_H        = 44;    // Row height
constexpr uint8_t VISIBLE      = 3;     // Rows visible at once
constexpr int     TEXT_W       = 216;   // Usable width for the clear text
constexpr int     REVEAL_LINES = 3;     // Text lines per clear page

// The opened entry is read and decrypted from flash on opening, and wiped from RAM when leaving the screen.
Vault::Entry gEntry;
uint32_t     gId   = 0;
bool         gHave = false;

void releaseEntry() {
    Vault::wipe(gEntry);
    gEntry = Vault::Entry();
    gId = 0;
    gHave = false;
}

// The entry selected in the list (read on demand), or nullptr if not found.
const Vault::Entry* current() {
    const uint32_t id = Vault::selected();
    if (gHave && gId == id) return &gEntry;
    releaseEntry();
    if (id == 0 || !Vault::read(id, gEntry)) return nullptr;
    gId = id; gHave = true;
    return &gEntry;
}

// A+B: types the field value on the computer (USB HID), hidden or not.
void typeField(const Vault::Entry* en, uint8_t index) {
    if (!Hid::available()) {
        Ui::showMessage(Ui::Msg::Error, "USB keyboard off in this build");
        return;
    }
    const Vault::Field& f = en->fields[index];
    Ui::showBusy("Typing...");
    const int skipped = Hid::type(f.value.c_str(), f.value.size());
    if (skipped > 0) {
        char m[40];
        snprintf(m, sizeof(m), "Typed, %d chars skipped", skipped);
        Ui::showMessage(Ui::Msg::Info, m);
    } else {
        Ui::showMessage(Ui::Msg::Info, "Typed");
    }
}

// Text truncated with ".." to fit in maxW pixels.
std::string fitted(M5Canvas& c, const std::string& s, int maxW) {
    if (c.textWidth(s.c_str()) <= maxW) return s;
    for (size_t n = s.size(); n > 1; n--) {
        std::string t = s.substr(0, n - 1) + "..";
        if (c.textWidth(t.c_str()) <= maxW) return t;
    }
    return s;
}

// Writes s with digits in a more discreet color than the rest (letters, symbols).
// Monospace font: each series of digits / non-digits is placed after the previous one.
// centered = true: s is centered on x; otherwise x is its left edge. y = vertical middle.
void drawDigitsDim(M5Canvas& c, const char* s, int x, int y, bool centered,
                   uint16_t normal, uint16_t digit, uint16_t back) {
    int cx = centered ? x - c.textWidth(s) / 2 : x;
    c.setTextDatum(middle_left);
    const size_t n = strlen(s);
    size_t i = 0;
    while (i < n) {
        const bool isDigit = (s[i] >= '0' && s[i] <= '9');
        size_t j = i;
        while (j < n && (s[j] >= '0' && s[j] <= '9') == isDigit) j++;
        const std::string run(s + i, j - i);
        c.setTextColor(isDigit ? digit : normal, back);
        c.drawString(run.c_str(), cx, y);
        cx += c.textWidth(run.c_str());
        i = j;
    }
}

}  // namespace

uint8_t EntryScreen::rows() const {
    const Vault::Entry* e = current();
    return e ? (uint8_t)(e->fields.size() + 1) : 1;
}

void EntryScreen::onLeave() { releaseEntry(); }          // Secrets leave RAM with the screen

void EntryScreen::onEnter() {
    sel = 0; top = 0; armed = false; reveal = false; page = 0;
    releaseEntry();                                         // Re-read the entry from flash
    if (!current()) { ScreenManager::go(ScreenId::List); return; }
    invalidate();
}

void EntryScreen::scrollTo() {
    if (sel < top) top = sel;
    else if (sel >= top + VISIBLE) top = sel - VISIBLE + 1;
}

// --- Clear ------------------------------------------------------------------
void EntryScreen::startReveal() {
    const Vault::Entry* e = current();
    if (!e || sel >= e->fields.size()) return;
    reveal = true;
    page   = 0;
    invalidate();
}

void EntryScreen::onRevealInput(InputEvent e) {
    const Vault::Entry* en = current();
    if (!en || sel >= en->fields.size()) { reveal = false; invalidate(); return; }

    Fonts::apply(Ui::canvas());
    const int perLine = max(1, TEXT_W / max(1, (int)Ui::canvas().textWidth("W")));
    const int perPage = perLine * REVEAL_LINES;
    const int pages   = ((int)en->fields[sel].value.size() + perPage - 1) / perPage;

    if (pages > 1 && e == InputEvent::AClick)      page = (page + 1) % pages;
    else if (pages > 1 && e == InputEvent::BClick) page = (page + pages - 1) % pages;
    else                                           reveal = false;   // Any other key: back
    invalidate();
}

void EntryScreen::drawReveal(M5Canvas& c) {
    const Vault::Entry* e = current();
    if (!e || sel >= e->fields.size()) return;
    const Vault::Field& f = e->fields[sel];

    Fonts::apply(c);
    c.setTextDatum(middle_center);
    c.setTextColor(Theme::fg, Theme::bg);
    Ui::drawTextFit(c, Vault::fieldName(f), Config::SCREEN_W / 2, 15, TEXT_W);
    c.drawFastHLine(8, 26, Config::SCREEN_W - 16, Theme::fg);

    const int perLine = max(1, TEXT_W / max(1, (int)c.textWidth("W")));
    const int perPage = perLine * REVEAL_LINES;
    const int len     = (int)f.value.size();
    const int pages   = max(1, (len + perPage - 1) / perPage);
    const int start   = page * perPage;
    const int chunk   = min(perPage, len - start);

    const int nLines = max(1, (chunk + perLine - 1) / perLine);
    const int lineH  = c.fontHeight() + 4;
    const int y0     = 74 - (nLines - 1) * lineH / 2;
    for (int i = 0; i < nLines; i++) {
        const int from = start + i * perLine;
        const int n    = min(perLine, len - from);
        if (n <= 0) break;
        drawDigitsDim(c, f.value.substr(from, n).c_str(), Config::SCREEN_W / 2, y0 + i * lineH,
                      true, Theme::fg, Theme::mid, Theme::bg);
    }

    if (pages > 1) {                                         // Page indicator
        char p[12];
        snprintf(p, sizeof(p), "%d/%d", page + 1, pages);
        c.setTextColor(Theme::mid, Theme::bg);
        c.setTextDatum(bottom_right);
        c.drawString(p, Config::SCREEN_W - 8, Config::SCREEN_H - 3);
    }
}

// --- Deletion ---------------------------------------------------------------
void EntryScreen::deleteEntry() {
    const Vault::Entry* e = current();
    if (e && Vault::remove(e->id)) {
        releaseEntry();
        Vault::select(0);
        ScreenManager::go(ScreenId::List);
        return;
    }
    Ui::showMessage(Ui::Msg::Error, "Delete failed");
    armed = false;
    invalidate();
}

// --- Entries ----------------------------------------------------------------
void EntryScreen::onInput(InputEvent e) {
    if (reveal) { onRevealInput(e); return; }

    const Vault::Entry* en = current();
    if (!en) { ScreenManager::go(ScreenId::List); return; }
    const uint8_t n = rows();
    const bool onDelete = (sel == n - 1);

    switch (e) {
        case InputEvent::AClick:
            sel = (sel + 1) % n; armed = false; scrollTo(); break;
        case InputEvent::BClick:
            sel = (sel + n - 1) % n; armed = false; scrollTo(); break;
        case InputEvent::BLong:
            ScreenManager::go(ScreenId::List);
            return;
        case InputEvent::ALong:
        case InputEvent::AB:
            if (!onDelete) {                                   // Field: A long = show, A+B = type over USB
                if (e == InputEvent::ALong) { startReveal(); return; }
                typeField(en, sel);
                break;
            }
            if (!armed) {                                      // 1st time: the row becomes "Sure ? Press again"
                armed = true;
            } else {                                           // 2nd time: deletion
                deleteEntry();
                return;
            }
            break;
        default: return;
    }
    invalidate();
}

void EntryScreen::draw(M5Canvas& c) {
    if (reveal) { drawReveal(c); return; }

    const Vault::Entry* e = current();
    if (!e) return;
    const uint8_t n = rows();

    for (uint8_t row = 0; row < VISIBLE; row++) {
        const uint8_t idx = top + row;
        if (idx >= n) break;

        const int y = row * ROW_H + 2;
        const bool selected = (idx == sel);
        const uint16_t main = selected ? Theme::bg : Theme::fg;
        const uint16_t sub  = selected ? Theme::bg : Theme::mid;
        const uint16_t back = selected ? Theme::fg : Theme::bg;

        if (selected) c.fillRoundRect(7, y, 226, ROW_H - 4, 5, Theme::fg);
        c.setTextDatum(middle_left);

        if (idx == n - 1) {                                   // "Delete" row
            Fonts::apply(c);
            c.setTextColor(main, back);
            c.drawString(armed ? "Sure ? Press again" : "Delete", 14, y + (ROW_H - 4) / 2);
            continue;
        }

        const Vault::Field& f = e->fields[idx];
        Fonts::apply(c);
        c.setTextSize(Fonts::SCALE * 0.75f);                  // More discreet label
        c.setTextColor(sub, back);
        Ui::drawTextFit(c, Vault::fieldName(f), 14, y + 11, 212);

        Fonts::apply(c);
        c.setTextColor(main, back);
        if (Vault::isObfuscated(f.type)) c.drawString("*****", 14, y + 29);
        else
            drawDigitsDim(c, fitted(c, f.value, 212).c_str(), 14, y + 29, false, main, selected ? Theme::dim : Theme::mid, back);
    }
}
