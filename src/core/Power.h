// Power.h : power off the stick
#pragma once

namespace Power {
    // Lock the screen, show the short owl sleep animation then power off the stick
    void shutdown();

    // Check if the stick has been unplug from a device, if yes and the related settings active, then shutdown the stick
    void pollUnplug();
}
