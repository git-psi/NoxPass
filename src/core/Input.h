// Input.h : transform button into events
#pragma once
#include <stdint.h>

// 4 events: Short A, Short B, Long A, Long B, A + B in the same time
enum class InputEvent : uint8_t { None, AClick, BClick, ALong, BLong, AB };

namespace Input {
    // Call M5.update() et return an InputEvent
    // Check also, USB connection and the boot button (the small one, not the A or B). -> Even on not refreshed screen
    InputEvent poll();
}
