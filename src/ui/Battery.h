#pragma once
#include <M5Unified.h>

namespace Battery {
    bool refresh();                                  // true if the display must be redrawn
    void draw(M5Canvas& c, int16_t x, int16_t y);    // x,y = top-left corner (20x9 px in total)
    void drawTopRight(M5Canvas& c);                  // Standard position of screens 1 and 3
}