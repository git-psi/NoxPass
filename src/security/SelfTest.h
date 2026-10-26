// SelfTest.h : vérifications de la crypto + benchmark. TEMPORAIRE (Config::DEV_SELFTEST).
#pragma once

namespace SelfTest {
    // nullptr si tout passe, sinon le nom du premier test en échec. Ne touche pas à l'écran.
    const char* check();

    // Sur l'appareil : check() + benchmark PBKDF2, résultats affichés à l'écran.
    void runOnDevice();
}