// Web.h : web server of the M5Stick (captive portal + website API). Only active during the Bridge screen.
#pragma once

namespace Web {
    bool begin();            // After Bridge::start(): DNS "everything to me", HTTP server on port 80
    void end();
    void loop();             // Call often
    bool consumeChanged();   // true once if the website changed the vault or the settings (the screen must redraw)
}
