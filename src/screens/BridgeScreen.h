// BridgeScreen.h : Wi-Fi screen (Bridge).
#pragma once
#include "../core/Screen.h"
#include "../net/Bridge.h"

class BridgeScreen : public Screen {
public:
    void onEnter() override;
    void onLeave() override;
    void onInput(InputEvent e) override;
    void update() override;
    void draw(M5Canvas& c) override;

private:
    Bridge::State shown = Bridge::State::Off;
    char          lastInfo[40] = {0};
    char          lastLabel[20] = {0};
};
