// main.cpp : entry point. Starts the hardware, loads settings and key, then runs the current screen.
#include <M5Unified.h>
#include "config.h"
#include "core/Activity.h"
#include "core/Input.h"
#include "core/PowerKey.h"
#include "core/ScreenManager.h"
#include "core/Settings.h"
#include "hid/Hid.h"
#include "screens/AppsScreen.h"
#include "screens/BridgeScreen.h"
#include "screens/CreateScreen.h"
#include "screens/EntryScreen.h"
#include "screens/ListScreen.h"
#include "screens/LoginScreen.h"
#include "screens/SettingsScreen.h"
#include "security/Auth.h"
#include "security/KeyStore.h"
#include "storage/Storage.h"
#include "ui/PowerOff.h"
#include "ui/Theme.h"
#include "ui/Ui.h"
#include "vault/Vault.h"

// One screen = one object, created once here.
static LoginScreen    loginScreen;
static AppsScreen     appsScreen;
static ListScreen     listScreen;
static EntryScreen    entryScreen;
static CreateScreen   createScreen;
static BridgeScreen   bridgeScreen;
static SettingsScreen settingsScreen;

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);

    Theme::set(TFT_BLACK, TFT_WHITE);    // Temporary colors, until settings are read
    Ui::begin();

    if (!Storage::begin()) {
        Ui::showMessage(Ui::Msg::Error, "Storage failed", true);
    }
    Settings::load();                    // Theme, brightness, USB keyboard layout (before the login screen)
    KeyStore::load();                    // Encrypted master key file (missing = first launch)
    Hid::begin();                        // The stick shows up as a USB keyboard on the computer
    PowerKey::begin();                   // Double click on the side button
    Activity::touch();

    ScreenManager::add(ScreenId::Login,    &loginScreen);
    ScreenManager::add(ScreenId::Apps,     &appsScreen);
    ScreenManager::add(ScreenId::List,     &listScreen);
    ScreenManager::add(ScreenId::Entry,    &entryScreen);
    ScreenManager::add(ScreenId::Create,   &createScreen);
    ScreenManager::add(ScreenId::Bridge,   &bridgeScreen);
    ScreenManager::add(ScreenId::Settings, &settingsScreen);
    ScreenManager::go(ScreenId::Login);
}

// Auto-lock: after N seconds without button press or website action, clear the vault from RAM
// and go back to the login screen. (N = "Auto-lock" setting, 0 = never.)
static void autoLock() {
    const uint16_t s = Settings::get().autoLock;
    if (s == 0 || !Auth::isUnlocked()) return;
    if (Activity::idleMs() < (uint32_t)s * 1000UL) return;
    Vault::clear();
    Auth::lock();
    ScreenManager::go(ScreenId::Login);
    Activity::touch();
}

// Idle login screen: real power off of the stick (not only the screen).
// Not during the very first launch (welcome, guide, PIN choice): nothing to protect yet.
static void loginPowerOff() {
    if (Auth::isUnlocked() || KeyStore::state() == KeyStore::State::Missing) return;
    if (Activity::idleMs() < Config::LOGIN_POWER_OFF_MS) return;
    PowerOff::run();                     // Logo, eyes closing, brightness going down, power off (never returns)
}

void loop() {
    const InputEvent e = Input::poll();
    loginPowerOff();
    if (e != InputEvent::None || M5.BtnA.isPressed() || M5.BtnB.isPressed()) Activity::touch();
    autoLock();
    ScreenManager::tick(e);
    delay(10);
}
