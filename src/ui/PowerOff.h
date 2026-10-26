// PowerOff.h : power off animation (big logo, eyes closing, brightness going down), then shutdown.
#pragma once

namespace PowerOff {
    constexpr unsigned DURATION_MS = 500;   // Total duration of the animation (brightness going down to 0)
    constexpr unsigned CLOSE_MS    = 300;   // Time the eyes take to close (then they stay closed)

    void run();                             // Never returns: cuts the power at the end
}
