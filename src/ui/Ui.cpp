// Ui.cpp : the canvas (in-memory image pushed at once to the screen) and the Info / Error messages.
#include "Ui.h"
#include "Theme.h"
#include "Fonts.h"
#include "../config.h"
#include "../core/Input.h"
#include <M5Unified.h>

namespace Ui {

static M5Canvas gCanvas(&M5.Display);

static constexpr int MAX_LINES = 4;
static constexpr int LINE_CAP  = 48;

void begin() {
    M5.Display.setRotation(Config::SCREEN_ROTATION);
    gCanvas.setColorDepth(16);
    gCanvas.createSprite(Config::SCREEN_W, Config::SCREEN_H);
}

M5Canvas& canvas() { return gCanvas; }
void clear()   { gCanvas.fillSprite(Theme::bg); }
void present() { gCanvas.pushSprite(0, 0); }

// Splits text into lines of width <= maxW (line break at spaces).
// Font and size must already be set on c. Returns the number of lines.
static int wrap(M5Canvas& c, const char* text, int maxW, char (*lines)[LINE_CAP]) {
    int n = 0;
    lines[0][0] = 0;
    const char* p = text;

    while (*p) {
        while (*p == ' ') p++;
        if (!*p) break;

        const char* e = p;
        while (*e && *e != ' ') e++;
        char word[LINE_CAP];
        size_t wl = min((size_t)(e - p), sizeof(word) - 1);
        memcpy(word, p, wl);
        word[wl] = 0;
        p = e;

        char trial[LINE_CAP * 2];
        if (lines[n][0]) snprintf(trial, sizeof(trial), "%s %s", lines[n], word);
        else             snprintf(trial, sizeof(trial), "%s", word);

        if (lines[n][0] && c.textWidth(trial) > maxW) {
            if (++n >= MAX_LINES) return MAX_LINES;      // Too long: cut
            snprintf(lines[n], LINE_CAP, "%s", word);
        } else {
            snprintf(lines[n], LINE_CAP, "%s", trial);
        }
    }
    return lines[n][0] ? n + 1 : n;
}

static void drawMessage(Msg type, const char* text) {
    clear();
    Fonts::apply(gCanvas);
    gCanvas.setTextColor(Theme::fg, Theme::bg);
    gCanvas.setTextDatum(middle_center);

    gCanvas.drawString(type == Msg::Info ? "Info" : "Error", Config::SCREEN_W / 2, 15);
    gCanvas.drawFastHLine(8, 26, Config::SCREEN_W - 16, Theme::fg);

    char lines[MAX_LINES][LINE_CAP];
    const int n     = wrap(gCanvas, text, 216, lines);
    const int lineH = gCanvas.fontHeight() + 4;
    const int y0    = 76 - (n - 1) * lineH / 2;
    for (int i = 0; i < n; i++) {
        gCanvas.drawString(lines[i], Config::SCREEN_W / 2, y0 + i * lineH);
    }
    present();
}

void showMessage(Msg type, const char* text, bool waitKey) {
    drawMessage(type, text);
    if (waitKey) {
        while (Input::poll() == InputEvent::None) delay(10);
    } else {
        // Closes after MESSAGE_MS, or as soon as a button is pressed.
        const uint32_t t0 = millis();
        auto down = [] { M5.update(); return M5.BtnA.isPressed() || M5.BtnB.isPressed(); };

        // Ignores the press that triggered the message (still pressed).
        while (down() && millis() - t0 < Config::MESSAGE_MS) delay(10);
        // Waits for a new press or the end of the delay.
        while (!down() && millis() - t0 < Config::MESSAGE_MS) delay(10);
        // If it was a press: waits for its release so the next screen doesn't react to it.
        if (millis() - t0 < Config::MESSAGE_MS) {
            while (down()) delay(10);
        }
    }
}

void showBusy(const char* text) { drawMessage(Msg::Info, text); }

void drawTextFit(M5Canvas& c, const char* s, int16_t x, int16_t y, int16_t maxW) {
    char buf[40];
    strncpy(buf, s, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;

    if (c.textWidth(buf) > maxW) {
        size_t n = strlen(buf);
        while (n > 1) {
            buf[--n] = 0;
            char t[44];
            snprintf(t, sizeof(t), "%s..", buf);
            if (c.textWidth(t) <= maxW) { c.drawString(t, x, y); return; }
        }
    }
    c.drawString(buf, x, y);
}

}