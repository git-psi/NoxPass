// Hid.cpp : USB keyboard. Translates each character into a key (AZERTY or QWERTY) and "types" it on the computer.
#include "Hid.h"
#include <string.h>
#include <Arduino.h>
#include "USB.h"
#include "USBHIDKeyboard.h"

// Two functions of the USB library (TinyUSB) giving the link state with the computer.
extern "C" bool tud_mounted(void);
extern "C" bool tud_suspended(void);

namespace Hid {

    namespace {
        constexpr uint8_t MOD_SHIFT = 0x02;     // Left Shift
        constexpr uint8_t MOD_ALTGR = 0x40;     // AltGr

        // A physical key, identified by its USB code (position on a US keyboard),
        struct Key { uint8_t usage; char base; char shift; char altgr; };

        // French AZERTY (PC): letters and symbols
        // Some characters (é è ç à ù ° £ µ § ¤ ²) don't exist in ASCII
        // "Dead" keys (^ ` ~) are voluntarily not handled: their result depends on the system (they wait for a 2nd keystroke), a wrong character would be worse than a skipped one.
        const Key AZERTY[] = {
            {0x04,'q','Q',0}, {0x05,'b','B',0}, {0x06,'c','C',0}, {0x07,'d','D',0},
            {0x08,'e','E',0}, {0x09,'f','F',0}, {0x0A,'g','G',0}, {0x0B,'h','H',0},
            {0x0C,'i','I',0}, {0x0D,'j','J',0}, {0x0E,'k','K',0}, {0x0F,'l','L',0},
            {0x10,',','?',0},
            {0x11,'n','N',0}, {0x12,'o','O',0}, {0x13,'p','P',0}, {0x14,'a','A',0},
            {0x15,'r','R',0}, {0x16,'s','S',0}, {0x17,'t','T',0}, {0x18,'u','U',0},
            {0x19,'v','V',0}, {0x1A,'z','Z',0}, {0x1B,'x','X',0}, {0x1C,'y','Y',0},
            {0x1D,'w','W',0},
            // digit row: in AZERTY, digits need Shift
            {0x1E,'&','1',0}, {0x1F,0,'2',0}, {0x20,'"','3','#'}, {0x21,'\'','4','{'},
            {0x22,'(','5','['}, {0x23,'-','6','|'}, {0x24,0,'7',0}, {0x25,'_','8','\\'},
            {0x26,0,'9',0}, {0x27,0,'0','@'},
            {0x2C,' ',' ',0},
            {0x2D,')',0,']'}, {0x2E,'=','+','}'},
            {0x30,'$',0,0}, {0x31,'*',0,0},
            {0x33,'m','M',0}, {0x34,0,'%',0},
            {0x36,';','.',0}, {0x37,':','/',0}, {0x38,'!',0,0},
            {0x64,'<','>',0},
        };

        // American QWERTY: symbols and digits (letters are handled apart)
        const Key QWERTY[] = {
            {0x1E,'1','!',0}, {0x1F,'2','@',0}, {0x20,'3','#',0}, {0x21,'4','$',0},
            {0x22,'5','%',0}, {0x23,'6','^',0}, {0x24,'7','&',0}, {0x25,'8','*',0},
            {0x26,'9','(',0}, {0x27,'0',')',0},
            {0x2C,' ',' ',0},
            {0x2D,'-','_',0}, {0x2E,'=','+',0}, {0x2F,'[','{',0}, {0x30,']','}',0},
            {0x31,'\\','|',0}, {0x33,';',':',0}, {0x34,'\'','"',0}, {0x35,'`','~',0},
            {0x36,',','<',0}, {0x37,'.','>',0}, {0x38,'/','?',0},
        };

        Layout current = Layout::Qwerty;

        bool scan(const Key* table, size_t n, char c, uint8_t& mod, uint8_t& usage) {
            for (size_t i = 0; i < n; i++) {
                if (table[i].base  == c) { mod = 0;         usage = table[i].usage; return true; }
                if (table[i].shift == c) { mod = MOD_SHIFT; usage = table[i].usage; return true; }
                if (table[i].altgr == c) { mod = MOD_ALTGR; usage = table[i].usage; return true; }
            }
            return false;
        }

    }

    bool encode(char c, Layout l, uint8_t& mod, uint8_t& usage) {
        if (c < 0x20 || c > 0x7E) return false;
        if (l == Layout::Azerty) return scan(AZERTY, sizeof(AZERTY) / sizeof(AZERTY[0]), c, mod, usage);

        if (c >= 'a' && c <= 'z') { mod = 0;         usage = 0x04 + (c - 'a'); return true; }
        if (c >= 'A' && c <= 'Z') { mod = MOD_SHIFT; usage = 0x04 + (c - 'A'); return true; }
        return scan(QWERTY, sizeof(QWERTY) / sizeof(QWERTY[0]), c, mod, usage);
    }

    void setLayout(Layout l) { current = l; }

    static USBHIDKeyboard keyboard;

    bool available() { return true; }

    // A computer is plugged in and active: true as soon as it recognized the keyboard, false if the cable is removed
    // or the computer is asleep.
    bool hostActive() { return tud_mounted() && !tud_suspended(); }

    void begin() {
        keyboard.begin();
        USB.productName(Config::USB_PRODUCT);        // Names the computer shows for the stick
        USB.manufacturerName(Config::USB_MANUFACTURER);
        USB.begin();
    }

    constexpr uint32_t GAP_MS     = 26;    // Between two keys
    constexpr uint32_t MOD_GAP_MS = 50;    // After a Shift / AltGr change, it solve some issue

    static uint8_t curMod = 0, curKey = 0;      // What the computer currently thinks is pressed

    static void report(uint8_t mod, uint8_t usage) {
        KeyReport r;
        memset(&r, 0, sizeof(r));
        r.modifiers = mod;
        r.keys[0]   = usage;
        keyboard.sendReport(&r);
        const bool modChanged = (mod != curMod);
        curMod = mod; curKey = usage;
        delay(modChanged ? MOD_GAP_MS : GAP_MS);
    }

    static void releaseAll() { if (curMod || curKey) report(0, 0); }

    static void stroke(uint8_t mod, uint8_t usage) {
        if (usage == curKey) report(curMod, 0);
        if (mod != curMod) report(mod, 0);
        report(mod, usage);
    }

    int type(const char* text, size_t len) {
        int skipped = 0;
        for (size_t i = 0; i < len; i++) {
            uint8_t mod, usage;
            if (!encode(text[i], current, mod, usage)) { skipped++; continue; }
            stroke(mod, usage);
        }
        releaseAll();           // Nothing stays pressed (otherwise auto-repeat on the computer)
        return skipped;
    }
}
