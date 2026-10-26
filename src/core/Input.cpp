// Input.cpp : detect key press and return InputEvent
#include "Input.h"
#include "Power.h"
#include "PowerKey.h"
#include "../config.h"
#include <M5Unified.h>

namespace Input {
    InputEvent poll() {
        enum class Who : uint8_t { A, B };

        static bool     active   = false;   // One or more btn is pressed
        static bool     consumed = false;   // The event for the active btn has already be sent
        static bool     bHeld    = false;   // B is held, and BLong already sent (bHeld used to power off)
        static Who      who      = Who::A;  // Which button is pressed
        static uint32_t startT   = 0;

        M5.update();
        Power::pollUnplug();                // Settings "Off on unplug"
        PowerKey::poll();                   // Double click on lateral button, power off the stick
        const bool a = M5.BtnA.isPressed();
        const bool b = M5.BtnB.isPressed();
        const uint32_t now = millis();

        if (!active) {                      // Wait for a new input
            if (a || b) {
                active   = true;
                consumed = false;
                bHeld    = false;
                startT   = now;
                who      = a ? Who::A : Who::B;
            }
            return InputEvent::None;
        }

        if (!consumed) {
            if (a && b) {                   // A+B pressed
                consumed = true;
                return InputEvent::AB;
            }
            if (a != b) who = a ? Who::A : Who::B;

            if ((a || b) && (now - startT >= Config::LONG_PRESS_MS)) {   // Long A | Long B
                consumed = true;
                bHeld = (who == Who::B);
                return (who == Who::A) ? InputEvent::ALong : InputEvent::BLong;
            }
        } else if (bHeld) {
            // After long B, if B is still pressed, power off the stick
            if (a)                                                    bHeld = false;
            else if (b && now - startT >= Config::SHUTDOWN_HOLD_MS)   Power::shutdown();
        }

        if (!a && !b) {                     // End of pression
            active = false;
            if (!consumed) {                // If not consumed, it was a short pression, send the event
                return (who == Who::A) ? InputEvent::AClick : InputEvent::BClick;
            }
        }
        return InputEvent::None;
    }
}
