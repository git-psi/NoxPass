// CreateScreen.h : screen 5, password creation (title -> username -> size)
#pragma once
#include "../core/Screen.h"
#include <string>

class CreateScreen : public Screen {
public:
    void onEnter() override;
    void onInput(InputEvent e) override;
    void draw(M5Canvas& c) override;

private:
    enum class Stage : uint8_t { Title, Username, Size };

    Stage       stage = Stage::Title;
    std::string text;            // Text being typed
    std::string title;           // Values already validated
    std::string username;
    uint8_t     cur  = 0;        // Position: saved presets first (first character of a username), then the alphabet
    uint8_t     size = 0;        // Password size

    size_t presets() const;      // Presets offered right now (only for the first character of the username)
    void append();
    void advance(bool useTemp);  // Validates the step (useTemp: take "Temp")
    void finish();
};