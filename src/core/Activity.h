// Last user activity (to lock after inactivity)
#pragma once
#include <stdint.h>

namespace Activity {
    void     touch();   // When user do something
    uint32_t idleMs();
}
