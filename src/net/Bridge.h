// Bridge.h : Wi-Fi access point of the M5Stick + tracking of the connected device (only one at a time).
// This module only handles Wi-Fi and device acceptance; the web server is in Web.h.
#pragma once
#include <stdint.h>

namespace Bridge {

enum class State : uint8_t {
    Off,        // Wi-Fi off
    Waiting,    // Access point open, no device
    Pending,    // A device is associated, waiting for A (accept) / B (refuse)
    Connected   // Device accepted
};

bool  start();                    // Starts the access point (new name + password). false on failure.
void  stop();                     // Turns all Wi-Fi off
void  poll();                     // Call often: handles Wi-Fi events

State       state();
const char* ssid();               // Valid after start()
const char* password();
const char* qrPayload();          // "WIFI:T:WPA;S:...;P:...;;"
const char* peerLabel();          // IP of the device if known, otherwise its MAC address

bool  accept();                   // Pending -> Connected
void  drop();                     // Pending/Connected -> Waiting, the device is disconnected
bool  consumeLost();              // true only once when the ACCEPTED device left
uint32_t peerIp();                // IPv4 (network order, like lwIP) of the associated device (waiting or accepted), 0 if unknown

// Info shown 3 s on the Bridge screen; a new info replaces the previous one.
void        notify(const char* text);
const char* info();               // nullptr if none or expired

}
