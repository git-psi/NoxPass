// ScreenManager.h : manage current screen, and send to it input
#pragma once
#include "Screen.h"

// List of all screens
enum class ScreenId : uint8_t {
    Login, Apps, List, Entry, Create, Bridge, Settings,
    COUNT
};

namespace ScreenManager {
    void add(ScreenId id, Screen* screen);
    void go(ScreenId id);
    void tick(InputEvent e);
}
