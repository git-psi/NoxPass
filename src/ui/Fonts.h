#pragma once
#include <M5Unified.h>

// Single font of the application. To change the style everywhere: edit this file.
namespace Fonts {
    // 1.0 = native size (~14 px/character). Try 0.85 for a more compact look.
    constexpr float SCALE = 1.0f;

    inline void apply(M5Canvas& c) {
        c.setFont(&fonts::FreeMonoBold9pt7b);
        c.setTextSize(SCALE);
    }
}