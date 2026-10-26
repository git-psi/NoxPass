#pragma once
#include <stdint.h>

namespace Theme {
    extern uint16_t bg;    // Color 1
    extern uint16_t fg;    // Color 2
    extern uint16_t mid;   // Mix of fg/bg: secondary elements
    extern uint16_t dim;   // Mix of mid/bg: very faded elements

    void set(uint16_t background, uint16_t foreground);
}