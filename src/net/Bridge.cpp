// Bridge.cpp : Wi-Fi access point, random credentials, tracking of the connected device.
#include "Bridge.h"
#include "../config.h"
#include "../security/Crypto.h"
#include <WiFi.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

static_assert(Config::WIFI_PASS_LEN >= 8 && Config::WIFI_PASS_LEN <= 32, "WPA2 : 8 a 63 caracteres");

namespace Bridge {
namespace {

// 32 unambiguous characters (no 0/O, no 1/I/L): a random byte & 31 is uniform.
const char ALPHABET[33] = "ABCDEFGHJKMNPQRSTUVWXYZ23456789-";

State    st = State::Off;                      // Current Bridge state
char     gSsid[16];                            // Wi-Fi name (random at each start)
char     gPass[Config::WIFI_PASS_LEN + 1];     // Wi-Fi password (same)
char     gQr[80];                              // Content of the connection QR code

uint8_t  peerMac[6];                           // Associated device (waiting or accepted)
uint8_t  peerAid  = 0;                         // Its Wi-Fi association number (to disconnect it)
uint32_t gPeerIp   = 0;                        // Its IP address, once the stick gave it one
bool     lost     = false;                     // The accepted device just left (read once by consumeLost)
char     label[20];                            // Text shown to designate the device (IP, otherwise MAC address)

char     infoBuf[40];                          // Info shown a few seconds on the Bridge screen
uint32_t infoUntil = 0;                        // Time when it disappears (0 = none)

// Data written by the Wi-Fi event task, read by poll().
struct Events {
    bool     conn, disc, ip;
    uint8_t  cMac[6], cAid, dMac[6];
    uint32_t ipAddr;
};
Events     ev;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
bool       hooked = false;

void onWifiEvent(arduino_event_id_t id, arduino_event_info_t info) {
    portENTER_CRITICAL(&mux);
    switch (id) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            ev.conn = true;
            memcpy(ev.cMac, info.wifi_ap_staconnected.mac, 6);
            ev.cAid = info.wifi_ap_staconnected.aid;
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            ev.disc = true;
            memcpy(ev.dMac, info.wifi_ap_stadisconnected.mac, 6);
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            ev.ip     = true;
            ev.ipAddr = info.wifi_ap_staipassigned.ip.addr;
            break;
        default: break;
    }
    portEXIT_CRITICAL(&mux);
}

void makeCredentials() {
    uint8_t r[2 + Config::WIFI_PASS_LEN];
    Crypto::randomBytes(r, sizeof(r));

    snprintf(gSsid, sizeof(gSsid), "Bridge-%03X", ((r[0] << 8) | r[1]) & 0xFFF);
    for (uint8_t i = 0; i < Config::WIFI_PASS_LEN; i++) gPass[i] = ALPHABET[r[2 + i] & 31];
    gPass[Config::WIFI_PASS_LEN] = 0;
    snprintf(gQr, sizeof(gQr), "WIFI:T:WPA;S:%s;P:%s;;", gSsid, gPass);
    Crypto::wipe(r, sizeof(r));
}

void refreshLabel() {
    if (gPeerIp) {
        snprintf(label, sizeof(label), "%u.%u.%u.%u",
                 (unsigned)(gPeerIp & 255), (unsigned)((gPeerIp >> 8) & 255),
                 (unsigned)((gPeerIp >> 16) & 255), (unsigned)(gPeerIp >> 24));
    } else {
        snprintf(label, sizeof(label), "%02X:%02X:%02X:%02X:%02X:%02X",
                 peerMac[0], peerMac[1], peerMac[2], peerMac[3], peerMac[4], peerMac[5]);
    }
}

void clearPeer() {
    memset(peerMac, 0, sizeof(peerMac));
    peerAid = 0;
    gPeerIp  = 0;
    label[0] = 0;
}

}  // namespace

bool start() {
    if (st != State::Off) return true;

    makeCredentials();
    clearPeer();
    lost = false;
    infoUntil = 0;
    memset(&ev, 0, sizeof(ev));

    if (!hooked) { WiFi.onEvent(onWifiEvent); hooked = true; }

    WiFi.persistent(false);
    WiFi.mode(WIFI_AP);
    // channel 1, visible SSID, only 1 device at a time
    if (!WiFi.softAP(gSsid, gPass, 1, 0, 1)) {
        WiFi.mode(WIFI_OFF);
        return false;
    }
    // The stick DHCP server must announce the stick itself as DNS server: otherwise the phone
    // asks another DNS (or none), doesn't see the captive portal and shows "no Internet access".
    esp_netif_t* ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
    if (ap) {
        esp_netif_dns_info_t dns;
        memset(&dns, 0, sizeof(dns));
        dns.ip.type = ESP_IPADDR_TYPE_V4;
        dns.ip.u_addr.ip4.addr = (uint32_t)WiFi.softAPIP();
        uint8_t offerDns = 0x02;                       // "OFFER_DNS" of the DHCP server
        esp_netif_dhcps_stop(ap);
        esp_netif_set_dns_info(ap, ESP_NETIF_DNS_MAIN, &dns);
        esp_netif_dhcps_option(ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &offerDns, sizeof(offerDns));
        esp_netif_dhcps_start(ap);
    }
    st = State::Waiting;
    return true;
}

void stop() {
    if (st == State::Off) return;
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
    st = State::Off;
    clearPeer();
    lost = false;
    infoUntil = 0;
    Crypto::wipe(gPass, sizeof(gPass));
    Crypto::wipe(gQr, sizeof(gQr));
}

void poll() {
    Events e;
    portENTER_CRITICAL(&mux);
    e = ev;
    memset(&ev, 0, sizeof(ev));
    portEXIT_CRITICAL(&mux);

    if (st == State::Off) return;

    if (e.conn && st == State::Waiting) {
        memcpy(peerMac, e.cMac, 6);
        peerAid = e.cAid;
        gPeerIp  = 0;
        refreshLabel();
        st = State::Pending;
    }
    if (e.ip && (st == State::Pending || st == State::Connected)) {
        gPeerIp = e.ipAddr;
        refreshLabel();
    }
    if (e.disc && st != State::Waiting && memcmp(e.dMac, peerMac, 6) == 0) {
        if (st == State::Connected) lost = true;
        clearPeer();
        st = State::Waiting;
    }
}

State       state()      { return st; }
const char* ssid()       { return gSsid; }
const char* password()   { return gPass; }
const char* qrPayload()  { return gQr; }
const char* peerLabel()  { return label; }

bool accept() {
    if (st != State::Pending) return false;
    st = State::Connected;
    return true;
}

void drop() {
    if (st != State::Pending && st != State::Connected) return;
    if (peerAid) esp_wifi_deauth_sta(peerAid);
    clearPeer();
    st = State::Waiting;
}

bool consumeLost() {
    const bool l = lost;
    lost = false;
    return l;
}

uint32_t peerIp()     { return (st == State::Pending || st == State::Connected) ? gPeerIp : 0; }

void notify(const char* text) {
    strncpy(infoBuf, text, sizeof(infoBuf) - 1);
    infoBuf[sizeof(infoBuf) - 1] = 0;
    infoUntil = millis() + Config::BRIDGE_INFO_MS;
    if (infoUntil == 0) infoUntil = 1;
}

const char* info() {
    if (infoUntil == 0) return nullptr;
    if ((int32_t)(millis() - infoUntil) >= 0) { infoUntil = 0; return nullptr; }
    return infoBuf;
}

}
