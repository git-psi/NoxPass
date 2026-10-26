// AppsScreen.h : apps menu.
#pragma once
#include "../core/Screen.h"

class AppsScreen : public Screen {
public:
    void onEnter() override;
    void onInput(InputEvent e) override;
    void update() override;
    void draw(M5Canvas& c) override;

private:
    uint8_t sel = 0;
};