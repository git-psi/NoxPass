// CreateScreen.cpp : password creation from the stick (title -> username -> size).
#include "CreateScreen.h"
#include "../core/ScreenManager.h"
#include "../ui/Ui.h"
#include "../ui/Theme.h"
#include "../ui/Fonts.h"
#include "../vault/Vault.h"
#include "../security/Crypto.h"
#include "../config.h"

namespace {

// Input alphabet: lowercase, uppercase, digits, then space (last).
const char ALPHABET[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 ";
constexpr uint8_t N_CHARS = sizeof(ALPHABET) - 1;       // 63
constexpr uint8_t SPACE_INDEX = N_CHARS - 1;

constexpr int TEXT_W = 216;

// --- Password generator ---------------------------------------------
// Without look-alike characters (0 O o 1 l I): it may have to be read and retyped.
// 24 lowercase + 24 uppercase + 8 digits + 24 ASCII symbols (all typable by the stick) = 80 characters.
const char POOL[] = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789!@#$%^&*()-_=+[]{};:,.?/";
constexpr size_t POOL_N = sizeof(POOL) - 1;

std::string makePassword(size_t len) {
    uint8_t rnd[64];
    size_t  pos = sizeof(rnd);
    std::string out;

    for (;;) {                                           // Start again while a category is missing
        out.clear();
        bool lower = false, upper = false, digit = false, symbol = false;
        while (out.size() < len) {
            if (pos >= sizeof(rnd)) { Crypto::randomBytes(rnd, sizeof(rnd)); pos = 0; }
            const uint8_t b = rnd[pos++];
            if (b >= 240) continue;                      // 240 = 3 x 80: rejected to stay uniform
            const char ch = POOL[b % POOL_N];
            lower |= (ch >= 'a' && ch <= 'z');
            upper |= (ch >= 'A' && ch <= 'Z');
            digit |= (ch >= '2' && ch <= '9');
            symbol |= !((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '2' && ch <= '9'));
            out += ch;
        }
        if (lower && upper && digit && symbol) break;
    }
    Crypto::wipe(rnd, sizeof(rnd));
    return out;
}

// "Temp" followed by 3 random digits (e.g. Temp471), to tell several temporary entries apart.
std::string tempName() {
    uint16_t r;
    do {
        Crypto::randomBytes((uint8_t*)&r, sizeof(r));
    } while (r >= 60000);                                // 60000 = 60 x 1000: rejected to stay uniform
    char buf[12];
    snprintf(buf, sizeof(buf), "Temp%03u", (unsigned)(r % 1000));
    return buf;
}

// --- Drawing -----------------------------------------------------------------
void drawSpaceIcon(M5Canvas& c, int cx, int cy, uint16_t col) {   // A flat "U": the space key
    c.drawFastHLine(cx - 6, cy + 3, 13, col);
    c.drawFastVLine(cx - 6, cy - 2, 6, col);
    c.drawFastVLine(cx + 6, cy - 2, 6, col);
}

}  // namespace

void CreateScreen::onEnter() {
    stage = Stage::Title;
    text.clear(); title.clear(); username.clear();
    cur  = 0;
    size = Config::PASSWORD_DEFAULT_LEN;
    invalidate();
}

void CreateScreen::append() {
    const size_t limit = (stage == Stage::Title) ? Vault::MAX_TITLE : Vault::MAX_VALUE;
    if (text.size() >= limit) {
        char m[24];
        snprintf(m, sizeof(m), "Max %u chars", (unsigned)limit);
        Ui::showMessage(Ui::Msg::Error, m);
        return;
    }
    text += ALPHABET[cur];
    cur = 0;
}

void CreateScreen::advance(bool useTemp) {
    const std::string value = useTemp ? tempName() : text;
    if (stage == Stage::Title) { title = value;    stage = Stage::Username; }
    else                       { username = value; stage = Stage::Size; }
    text.clear();
    cur = 0;
}

void CreateScreen::finish() {
    std::string pw = makePassword(size);

    Vault::Entry e;
    e.title = title;
    e.fields.resize(2);
    e.fields[0].type = Vault::T_USERNAME; e.fields[0].value = username;
    e.fields[1].type = Vault::T_PASSWORD; e.fields[1].value = pw;
    Crypto::wipe(&pw[0], pw.size());                    // The password copy is no longer needed

    if (Vault::add(e)) {
        Vault::setFocus(Vault::lastAdded());            // The list focuses on the new entry
        ScreenManager::go(ScreenId::List);
    } else {
        Ui::showMessage(Ui::Msg::Error, Vault::lastError() == Vault::Error::Full ? "Vault is full" : "Save failed");
    }
    Vault::wipe(e);                                     // Wipes the password from RAM
}

void CreateScreen::onInput(InputEvent e) {
    if (stage == Stage::Size) {
        switch (e) {
            case InputEvent::AClick: size = (size >= Config::PASSWORD_MAX_LEN) ? Config::PASSWORD_MIN_LEN : size + 1; break;
            case InputEvent::BClick: size = (size <= Config::PASSWORD_MIN_LEN) ? Config::PASSWORD_MAX_LEN : size - 1; break;
            case InputEvent::ALong:
            case InputEvent::AB:     finish(); return;
            case InputEvent::BLong:  ScreenManager::go(ScreenId::List); return;   // Cancel
            default: return;
        }
        invalidate();
        return;
    }

    switch (e) {
        case InputEvent::AClick: cur = (cur + 1) % N_CHARS; break;
        case InputEvent::BClick: cur = (cur + N_CHARS - 1) % N_CHARS; break;
        case InputEvent::ALong:  append(); break;
        case InputEvent::BLong:
            if (text.empty()) { ScreenManager::go(ScreenId::List); return; }       // Cancels the creation
            text.erase(text.size() - 1);
            break;
        case InputEvent::AB:     advance(text.empty()); break;                     // Empty: "Temp"
        default: return;
    }
    invalidate();
}

void CreateScreen::draw(M5Canvas& c) {
    const int cx = Config::SCREEN_W / 2;

    Fonts::apply(c);
    c.setTextDatum(middle_center);
    c.setTextColor(Theme::fg, Theme::bg);

    const char* header = stage == Stage::Title ? "Title"
                       : stage == Stage::Username ? "Username/Email" : "Password size";
    c.drawString(header, cx, 22);

    if (stage == Stage::Size) {
        c.setTextSize(Fonts::SCALE * 2.0f);
        char n[8];
        snprintf(n, sizeof(n), "%u", (unsigned)size);
        c.drawString(n, cx, 67);
    } else {
        // Already validated text: only the last characters fit, preceded by ".." if needed
        const int perLine = max(1, TEXT_W / max(1, (int)c.textWidth("W")));
        std::string shown = text;
        if ((int)shown.size() > perLine) shown = ".." + shown.substr(shown.size() - (perLine - 2));
        c.drawString(shown.c_str(), cx, 67);

        // Row of 5: next on the right, previous on the left, more and more faded
        const int rowY = 107, rowDx = 20;
        const uint16_t colors[5] = { Theme::dim, Theme::mid, Theme::fg, Theme::mid, Theme::dim };
        for (int k = 0; k < 5; k++) {
            const int d = k - 2;
            const uint8_t idx = (cur + N_CHARS * 2 + d) % N_CHARS;
            if (idx == SPACE_INDEX) {
                drawSpaceIcon(c, cx + d * rowDx, rowY, colors[k]);
            } else {
                char s[2] = { ALPHABET[idx], 0 };
                c.setTextColor(colors[k], Theme::bg);
                c.drawString(s, cx + d * rowDx, rowY);
            }
        }
        c.drawRoundRect(cx - 10, rowY - 10, 20, 20, 4, Theme::fg);
    }
}