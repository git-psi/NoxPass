// Web.cpp : web server of the stick (website page, JSON API, captive portal).
#include "Web.h"
#include "WebPage.h"
#include "WebApi.h"
#include "../ui/Logo.h"
#include "Bridge.h"
#include "../core/Activity.h"
#include "../core/Settings.h"
#include "../security/Crypto.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <string>

namespace Web {

namespace {

constexpr int MAX_BODY = 32768;     // Max size of a website request (bytes)

WebServer* srv     = nullptr;   // HTTP server (created by begin, deleted by end)
DNSServer* dns     = nullptr;   // Fake DNS that always returns the stick address (captive portal)
bool       changed = false;     // The website changed the vault or the settings (read by consumeChanged)

uint32_t clientIp() { return (uint32_t)srv->client().remoteIP(); }

// The device whose IP is the one of the current Wi-Fi peer.
bool isPeer() {
    const uint32_t ip = clientIp();
    return ip != 0 && ip == Bridge::peerIp();
}

// Data access: only the device accepted on the M5Stick, with the X-Bridge header.
// (This custom header forces a CORS "preflight" that we don't satisfy: another web page opened
// on the phone can't write into the vault.)
bool approved() {
    return Bridge::state() == Bridge::State::Connected && isPeer() && srv->header("X-Bridge") == "1";
}

void sendJson(int code, const std::string& s) {
    srv->sendHeader("Cache-Control", "no-store");
    srv->send(code, "application/json", s.c_str());
}

void redirectHome() {                      // Captive portal: any address -> the stick page
    const String url = "http://" + WiFi.softAPIP().toString() + "/";
    srv->sendHeader("Location", url, true);
    srv->sendHeader("Cache-Control", "no-store");
    srv->sendHeader("Connection", "close");           // Frees the connection quickly: phones open several at once
    srv->send(302, "text/plain", "");
}

void handleIndex() {
    srv->sendHeader("Cache-Control", "no-store");
    srv->send_P(200, "text/html; charset=utf-8", INDEX_HTML);
}

void handleStatus() {
    const char* s = "waiting";
    switch (Bridge::state()) {
        case Bridge::State::Pending:   s = "pending";   break;
        case Bridge::State::Connected: s = "connected"; break;
        default: break;
    }
    // Theme colors: the waiting page looks like the stick even before being approved (nothing secret).
    char col[48];
    const uint8_t th = Settings::get().theme;
    snprintf(col, sizeof(col), ",\"bg\":\"#%06X\",\"fg\":\"#%06X\"", (unsigned)Settings::themeBg(th), (unsigned)Settings::themeFg(th));
    std::string j = std::string("{\"s\":\"") + s + "\",\"me\":" + (isPeer() ? "true" : "false") + col + "}";
    sendJson(200, j);
}

void handleVault() {
    if (!approved()) { sendJson(403, "{\"error\":\"Not approved on the M5Stick\"}"); return; }
    Activity::touch();
    srv->setContentLength(CONTENT_LENGTH_UNKNOWN);
    srv->sendHeader("Cache-Control", "no-store");
    srv->send(200, "application/json", "");
    std::string head = WebApi::vaultHead();
    srv->sendContent(head.c_str(), head.size());
    for (size_t i = 0; i < Vault::count(); i++) {
        std::string e = WebApi::entryTitleJson(*Vault::at(i));
        if (i) e = "," + e;
        srv->sendContent(e.c_str(), e.size());
    }
    std::string tail = WebApi::vaultTail();
    srv->sendContent(tail.c_str(), tail.size());
    srv->sendContent("");                  // End of transfer
}

// The content of ONE entry, on demand: GET /api/entry?id=N
void handleEntry() {
    if (!approved()) { sendJson(403, "{\"error\":\"Not approved on the M5Stick\"}"); return; }
    Activity::touch();
    const String a = srv->arg("id");
    char* end = nullptr;
    const unsigned long id = strtoul(a.c_str(), &end, 10);
    Vault::Entry e;                          // Read and decrypted from flash, only for this answer
    if (!(a.length() && end && *end == 0) || !Vault::read((uint32_t)id, e)) { sendJson(404, "{\"error\":\"Entry not found\"}"); return; }
    std::string j = WebApi::entryJson(e);
    Vault::wipe(e);
    sendJson(200, j);
    if (!j.empty()) Crypto::wipe(&j[0], j.size());           // Wipes the clear copy
}

void handleApply() {
    if (!approved()) { sendJson(403, "{\"ok\":false,\"error\":\"Not approved on the M5Stick\"}"); return; }
    if (srv->clientContentLength() > MAX_BODY) { sendJson(413, "{\"ok\":false,\"error\":\"Too much data\"}"); return; }
    Activity::touch();

    const String body = srv->arg("plain");
    std::string error;
    int count = 0;
    const bool ok = WebApi::apply(body.c_str(), body.length(), error, count);
    if (count > 0) {                       // Even a partial failure (settings not saved) may have changed things
        changed = true;
        char m[40];
        snprintf(m, sizeof(m), "Saved %d change%s", count, count > 1 ? "s" : "");
        if (ok) Bridge::notify(m);
    }
    if (ok) {
        char r[40];
        snprintf(r, sizeof(r), "{\"ok\":true,\"n\":%d}", count);
        sendJson(200, r);
    } else {
        Bridge::notify(error.c_str());
        sendJson(400, "{\"ok\":false,\"error\":" + WebApi::jsonString(error) + "}");
    }
}

void handleLogo() {
    srv->sendHeader("Cache-Control", "max-age=3600");   // Kept by the browser: stays displayed even if the stick becomes unreachable
    srv->send(200, "image/svg+xml", Logo::svg());
}

void handleNoContent() { srv->send(204, "text/plain", ""); }     // No page icon

}  // namespace

bool begin() {
    if (srv) return true;

    dns = new DNSServer();
    dns->setErrorReplyCode(DNSReplyCode::NoError);
    dns->setTTL(30);                                // The phone doesn't keep these fake addresses for long
    dns->start(53, "*", WiFi.softAPIP());          // All domain names point to the stick

    srv = new WebServer(80);
    static const char* keys[] = { "X-Bridge" };
    srv->collectHeaders(keys, 1);
    srv->on("/",            HTTP_GET,  handleIndex);
    srv->on("/api/status",  HTTP_GET,  handleStatus);
    srv->on("/api/vault",   HTTP_GET,  handleVault);
    srv->on("/api/entry",   HTTP_GET,  handleEntry);
    srv->on("/api/apply",   HTTP_POST, handleApply);
    srv->on("/logo.svg",    HTTP_GET,  handleLogo);
    srv->on("/favicon.ico", HTTP_GET,  handleNoContent);
    // Captive portal: any other address (including the ones phones test to know if they have Internet:
    // /generate_204, /hotspot-detect.html, /connecttest.txt...) redirects to the stick page.
    srv->onNotFound(redirectHome);
    srv->begin();
    changed = false;
    return true;
}

void end() {
    if (srv) { srv->stop(); delete srv; srv = nullptr; }
    if (dns) { dns->stop(); delete dns; dns = nullptr; }
}

void loop() {
    if (dns) dns->processNextRequest();
    if (srv) srv->handleClient();
}

bool consumeChanged() {
    const bool c = changed;
    changed = false;
    return c;
}

}
