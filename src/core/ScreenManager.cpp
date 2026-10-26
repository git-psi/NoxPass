// ScreenManager.cpp
#include "ScreenManager.h"
#include "../ui/Ui.h"

namespace ScreenManager {

static Screen* screens[(int)ScreenId::COUNT] = {nullptr};
static Screen* current = nullptr;
static Screen* pending = nullptr;     // go() called, change next tick

void add(ScreenId id, Screen* s) { screens[(int)id] = s; }

void go(ScreenId id) { pending = screens[(int)id]; }

static void applyPending() {
    if (!pending) return;
    if (current) current->onLeave();
    current = pending;
    pending = nullptr;
    current->onEnter();
}

void tick(InputEvent e) {
    applyPending();
    if (!current) return;

    if (e != InputEvent::None) current->onInput(e);
    applyPending();                   // The screen may have asked for a screen change while handling the button
    if (!current) return;

    current->update();

    if (current->consumeDirty()) {    // Redraw only if the screen asked for it
        Ui::clear();
        current->draw(Ui::canvas());
        Ui::present();
    }
}

}
