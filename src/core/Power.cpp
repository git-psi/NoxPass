// Power.cpp : shutdown the stick, and detect when unplug
#include "Power.h"
#include "../hid/Hid.h"
#include "../security/Auth.h"
#include "../ui/PowerOff.h"
#include "../vault/Vault.h"
#include "Settings.h"
#include <M5Unified.h>

namespace Power {

void shutdown() {
    Vault::clear();                    // Remove vault from RAM
    Auth::lock();                      // Remove master key from RAM

    PowerOff::run();                   // Owl animation, shutdown
}

void pollUnplug() {
    static bool seenHost = false;                          // A device has been seen
    
    if (Hid::hostActive()) { seenHost = true; return; }
    if (!Settings::get().offOnUnplug) { seenHost = false; return; }

    if (!seenHost) return;

    shutdown();
}

}
