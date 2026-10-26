#include "Activity.h"
#include <Arduino.h>

// Simple clock to remember last user activity
namespace Activity {
    static uint32_t last = 0;

    void     touch()  { last = millis(); }
    uint32_t idleMs() { return millis() - last; }
}
