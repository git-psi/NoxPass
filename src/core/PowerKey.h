// PowerKey.h : the boot/on/off btn of the stick
// We remove the original stick interaction to put the animation 
// Short click and long pression stay untouched by the programm
#pragma once

namespace PowerKey {
    void begin();   // One time, just after M5.begin()
    void poll();    // At every btn check (Input::poll)
}
