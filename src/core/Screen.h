// Screen.h : base class of all screens.
// A screen gets the buttons (onInput), runs its periodic logic (update) and draws itself (draw).
// It is redrawn only if it asked with invalidate(): this keeps the display smooth.
#pragma once
#include <M5Unified.h>
#include "Input.h"

class Screen {
public:
    virtual ~Screen() {}
    virtual void onEnter() { dirty = true; }    // Called when entering the screen
    virtual void onLeave() {}                   // Called just before leaving it
    virtual void onInput(InputEvent e) = 0;     // A button was pressed
    virtual void update() {}                    // Periodic logic (battery, network...)
    virtual void draw(M5Canvas& c) = 0;         // Draws the whole screen

    void invalidate() { dirty = true; }                          // Ask for a redraw
    bool consumeDirty() { bool d = dirty; dirty = false; return d; }

protected:
    bool dirty = true;
};
