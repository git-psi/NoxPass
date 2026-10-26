// EntryScreen.h : screen 4, detail of an entry (fields + deletion)
#pragma once
#include "../core/Screen.h"
#include <stdint.h>

class EntryScreen : public Screen {
public:
    void onEnter() override;
    void onLeave() override;
    void onInput(InputEvent e) override;
    void draw(M5Canvas& c) override;

private:
    uint8_t  sel   = 0;        // Selected row: a field, or "Delete" last
    uint8_t  top   = 0;        // First visible row
    bool     armed = false;    // "Delete" was already validated once
    bool     reveal = false;   // A field is shown in clear
    uint8_t  page  = 0;        // Current page of the clear text

    uint8_t rows() const;      // Number of rows (fields + Delete)
    void    scrollTo();
    void    startReveal();
    void    drawReveal(M5Canvas& c);
    void    onRevealInput(InputEvent e);
    void    deleteEntry();
};
