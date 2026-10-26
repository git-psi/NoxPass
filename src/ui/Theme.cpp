// Theme.cpp : the four current colors (background, text, and two mixes).
#include "Theme.h"

namespace Theme {
    uint16_t bg  = 0x0000;
    uint16_t fg  = 0xFFFF;
    uint16_t mid = 0x7BEF;
    uint16_t dim = 0x39E7;

    static uint16_t blend(uint16_t a, uint16_t b) {
        uint16_t r  = (((a >> 11) & 0x1F) + ((b >> 11) & 0x1F)) / 2;
        uint16_t g  = (((a >> 5)  & 0x3F) + ((b >> 5)  & 0x3F)) / 2;
        uint16_t bl = ((a & 0x1F) + (b & 0x1F)) / 2;
        return (r << 11) | (g << 5) | bl;
    }

    void set(uint16_t background, uint16_t foreground) {
        bg  = background;
        fg  = foreground;
        mid = blend(bg, fg);
        dim = blend(bg, blend(bg, mid));
    }
}