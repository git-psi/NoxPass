// config.h : all the tunable constants of the firmware, in one place.
// (Settings changed by the user - theme, brightness... - are in core/Settings.cpp.)
#pragma once
#include <stdint.h>

namespace Config {

    // ---- Screen (M5Stick horizontal, B button on top) ----------------------------------------------
    constexpr uint8_t  SCREEN_ROTATION = 1;
    constexpr int16_t  SCREEN_W        = 240;
    constexpr int16_t  SCREEN_H        = 135;

    // ---- Buttons and durations --------------------------------------------------------------------
    constexpr uint32_t LONG_PRESS_MS      = 450;    // Long press (A long = validate, B long = back)
    constexpr uint32_t SHUTDOWN_HOLD_MS   = 3000;   // B held after a B long => power off
    constexpr uint32_t MESSAGE_MS         = 2000;   // How long an Info / Error message stays on screen
    constexpr uint32_t LOGIN_POWER_OFF_MS = 10000;  // Idle login screen: power off the stick
    constexpr uint32_t RESET_HOLD_MS      = 10000;  // Login screen: A+B held = factory reset
    constexpr uint32_t RESET_SHOW_MS      = 700;    // Under this duration, A+B is a simple PIN validation

    // ---- PIN ------------------------------------------------------------------------------------
    constexpr uint8_t  PIN_MIN         = 6;
    constexpr uint8_t  PIN_MAX         = 20;

    // PBKDF2 cost (iterations): it slows down PIN guesses if the flash is copied.
    // Higher = safer but slower unlock (target: about 1 s on the ESP32-S3).
    constexpr uint32_t PBKDF2_ITERATIONS = 20000;

    // ---- DEFAULT settings (the user changes them later from the stick or the website) ---------------
    constexpr const char* USB_PRODUCT      = "NoxPass";   // Name of the USB keyboard on the computer
    constexpr const char* USB_MANUFACTURER = "NoxPass";
    constexpr bool     HID_QWERTY         = true;   // Computer keyboard: false = AZERTY, true = QWERTY
    constexpr bool     PIN_SHOW_DIGITS    = true;   // false => '*' for validated digits
    constexpr uint16_t AUTOLOCK_DEFAULT_S = 60;     // Auto-lock after inactivity (0 = never)

    // ---- Side button ------------------------------------------------------------------------------
    // Double click on the side button: our power off (with animation) instead of the power chip one.
    // false = original behavior. Needs the M5PM1 library (platformio.ini).
    constexpr bool     POWER_KEY_ANIMATION = true;

    // ---- First launch -----------------------------------------------------------------------------
    // User guide: the QR code opens HELP_URL (START.md); HELP_SHOWN is the short text written next to it
    // (3 lines of 10 characters on the stick screen).
    constexpr const char* HELP_URL     = "https://github.com/git-psi/NoxPass/blob/main/START.md";
    constexpr const char* HELP_SHOWN   = "github.com/git-psi/NoxPass";

    // ---- Bridge (Wi-Fi) ---------------------------------------------------------------------------
    constexpr uint8_t  WIFI_PASS_LEN   = 8;         // WPA2 requires 8 characters minimum
    constexpr uint32_t BRIDGE_INFO_MS  = 3000;      // How long an info stays on the Bridge screen

    // ---- Passwords generated on the device (number of characters) ----------------------------------
    constexpr uint8_t  PASSWORD_MIN_LEN     = 8;
    constexpr uint8_t  PASSWORD_DEFAULT_LEN = 20;
    constexpr uint8_t  PASSWORD_MAX_LEN     = 64;
}
