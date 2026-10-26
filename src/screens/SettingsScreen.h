#pragma once
#include "../core/Screen.h"

// Device settings (same as on the website). Table and values: core/Settings.cpp.
//   A = next, B = previous, A long = on/off switch or opens the values list, B long = back.
class SettingsScreen : public Screen {
public:
    void onEnter() override;
    void onInput(InputEvent e) override;
    void onLeave() override;
    void draw(M5Canvas& c) override;

private:
    bool    inChoice = false;   // false = settings list, true = values list of a setting
    uint8_t item = 0;           // Selected setting (settings list) / opened (values list)
    uint8_t sel  = 0;           // Selected row in the displayed list
    uint8_t top  = 0;           // First visible row

    uint8_t rows() const;
    void    select(uint8_t row);
    void    previewRow();                // Live preview of the hovered value
};
