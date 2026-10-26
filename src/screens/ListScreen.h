// ListScreen.h : passwords list.
#pragma once
#include "../core/Screen.h"

class ListScreen : public Screen {
public:
    void onEnter() override;
    void onInput(InputEvent e) override;
    void draw(M5Canvas& c) override;

private:
    uint16_t sel = 0;   // 0 = Add button
    uint16_t top = 0;   // First visible item
};