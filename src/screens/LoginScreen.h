// LoginScreen.h : login screen (PIN input). On the very first launch: welcome, guide, then PIN choice.
#pragma once
#include "../core/Screen.h"
#include "../config.h"

class LoginScreen : public Screen {
public:
    void onEnter() override;
    void onInput(InputEvent e) override;
    void update() override;
    void draw(M5Canvas& c) override;

private:
    char    pin[Config::PIN_MAX + 1];   // Validated digits
    uint8_t len = 0;                    // Their number
    uint8_t cur = 0;                    // Digit shown on the wheel

    bool firstRun = false;   // No vault yet: we choose the PIN (typed twice)
    // First launch only: 0 = welcome (logo), 1 = guide (QR code), 2 = PIN choice
    uint8_t intro = 2;
    bool confirming = false; // First launch: 1st PIN typed, waiting for its repetition
    char firstPin[Config::PIN_MAX + 1];
    uint8_t firstLen = 0;

    void drawSplash(M5Canvas& c);
    void drawGuide(M5Canvas& c);
    bool holdReset();   // A+B held 10 s: factory reset. true if the gesture was used for that (so no validation)
    void reset();       // Resets the input
    void validate();    // Tests the PIN (or creates it on first launch)
    void enterVault();  // PIN is good: loads the vault and opens the list
};
