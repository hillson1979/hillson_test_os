/**
 * @file qkbdhillson_qws.cpp
 * @brief HillsonOS keyboard driver — Set1 scancodes via syscall 72
 *
 * pc101KeyM table extracted from Qt/Embedded 3.3.8b qkbdpc101_qws.cpp.
 * Maps PS/2 Set1 scancodes to Qt::Key codes + Unicode.
 */
#include "include/qkbdhillson_qws.h"

#define SYS_GUI_INPUT_READ 72

// Input event struct (must match kernel and desktop)
struct input_event_t {
    unsigned int type;    // 1=keyboard, 2=mouse
    int x;                // scancode (keyboard) or X (mouse)
    int y;                // 0 (keyboard) or Y (mouse)
    unsigned int pressed; // 1 (keyboard) or buttons (mouse)
};

// ============================================================
// QWSKeyboardHandler base class
// ============================================================

QWSKeyboardHandler::QWSKeyboardHandler()
    : m_shift(false), m_alt(false), m_ctrl(false), m_caps(false),
      m_modifiers(0), m_extended(0), m_prevUnicode(0), m_prevKeyCode(0)
{
}

QWSKeyboardHandler::~QWSKeyboardHandler() {}

void QWSKeyboardHandler::processKeyEvent(int unicode, int keycode, int modifiers,
                                          bool isPress, bool /*autoRepeat*/)
{
    // Minimal: store for polling by QApplication
    m_prevUnicode  = unicode;
    m_prevKeyCode  = keycode;
    m_modifiers    = modifiers;
    // In full Qt, this sends to QWSServer::processKeyEvent.
    // Here the QApplication poll loop reads these directly.
}

int QWSKeyboardHandler::transformDirKey(int key)
{
    // Stub — no screen rotation on HillsonOS
    return key;
}

// ============================================================
// PC101 Key Map (Set1 scancode index → Qt key definition)
// Extracted from Qt/Embedded 3.3.8b qkbdpc101_qws.cpp
// ============================================================

const QWSKeyMap QHillsonKeyboardHandler::m_keyMap[] = {
    // index 0x00
    { Qt::Key_unknown,    0xffff, 0xffff, 0xffff },
    { Qt::Key_Escape,     27,     27,     0xffff },
    { Qt::Key_1,          '1',    '!',    0xffff },
    { Qt::Key_2,          '2',    '@',    0xffff },
    { Qt::Key_3,          '3',    '#',    0xffff },
    { Qt::Key_4,          '4',    '$',    0xffff },
    { Qt::Key_5,          '5',    '%',    0xffff },
    { Qt::Key_6,          '6',    '^',    0xffff },
    { Qt::Key_7,          '7',    '&',    0xffff },
    { Qt::Key_8,          '8',    '*',    0xffff },
    { Qt::Key_9,          '9',    '(',    0xffff },
    { Qt::Key_0,          '0',    ')',    0xffff },
    { Qt::Key_Minus,      '-',    '_',    0xffff },
    { Qt::Key_Equal,      '=',    '+',    0xffff },
    { Qt::Key_Backspace,  8,      8,      0xffff },
    { Qt::Key_Tab,        9,      9,      0xffff },
    // 0x10
    { Qt::Key_Q,          'q',    'Q',    ('Q'-64) },
    { Qt::Key_W,          'w',    'W',    ('W'-64) },
    { Qt::Key_E,          'e',    'E',    ('E'-64) },
    { Qt::Key_R,          'r',    'R',    ('R'-64) },
    { Qt::Key_T,          't',    'T',    ('T'-64) },
    { Qt::Key_Y,          'y',    'Y',    ('Y'-64) },
    { Qt::Key_U,          'u',    'U',    ('U'-64) },
    { Qt::Key_I,          'i',    'I',    ('I'-64) },
    { Qt::Key_O,          'o',    'O',    ('O'-64) },
    { Qt::Key_P,          'p',    'P',    ('P'-64) },
    { Qt::Key_BracketLeft, '[',   '{',    0x1B },
    { Qt::Key_BracketRight,']',   '}',    0x1D },
    { Qt::Key_Return,     '\n',   '\n',   0x0A },
    { Qt::Key_Control,    0xffff, 0xffff, 0xffff },
    { Qt::Key_A,          'a',    'A',    ('A'-64) },
    { Qt::Key_S,          's',    'S',    ('S'-64) },
    // 0x20
    { Qt::Key_D,          'd',    'D',    ('D'-64) },
    { Qt::Key_F,          'f',    'F',    ('F'-64) },
    { Qt::Key_G,          'g',    'G',    ('G'-64) },
    { Qt::Key_H,          'h',    'H',    ('H'-64) },
    { Qt::Key_J,          'j',    'J',    ('J'-64) },
    { Qt::Key_K,          'k',    'K',    ('K'-64) },
    { Qt::Key_L,          'l',    'L',    ('L'-64) },
    { Qt::Key_Semicolon,  ';',    ':',    0xffff },
    { Qt::Key_Apostrophe, '\'',   '"',    0xffff },
    { Qt::Key_QuoteLeft,  '`',    '~',    0xffff },
    { Qt::Key_Shift,      0xffff, 0xffff, 0xffff },
    { Qt::Key_Backslash,  '\\',   '|',    0x1C },
    { Qt::Key_Z,          'z',    'Z',    ('Z'-64) },
    { Qt::Key_X,          'x',    'X',    ('X'-64) },
    { Qt::Key_C,          'c',    'C',    ('C'-64) },
    { Qt::Key_V,          'v',    'V',    ('V'-64) },
    // 0x30
    { Qt::Key_B,          'b',    'B',    ('B'-64) },
    { Qt::Key_N,          'n',    'N',    ('N'-64) },
    { Qt::Key_M,          'm',    'M',    ('M'-64) },
    { Qt::Key_Comma,      ',',    '<',    0xffff },
    { Qt::Key_Period,     '.',    '>',    0xffff },
    { Qt::Key_Slash,      '/',    '?',    0xffff },
    { Qt::Key_Shift,      0xffff, 0xffff, 0xffff }, // RShift
    { Qt::Key_Asterisk,   '*',    '*',    0xffff }, // Keypad *
    { Qt::Key_Alt,        0xffff, 0xffff, 0xffff },
    { Qt::Key_Space,      ' ',    ' ',    0x20 },
    { Qt::Key_CapsLock,   0xffff, 0xffff, 0xffff },
    { Qt::Key_F1,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F2,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F3,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F4,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F5,         0xffff, 0xffff, 0xffff },
    // 0x40
    { Qt::Key_F6,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F7,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F8,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F9,         0xffff, 0xffff, 0xffff },
    { Qt::Key_F10,        0xffff, 0xffff, 0xffff },
    { Qt::Key_NumLock,    0xffff, 0xffff, 0xffff },
    { Qt::Key_ScrollLock, 0xffff, 0xffff, 0xffff },
    { Qt::Key_Home,      0xffff, '7',    0xffff }, // Keypad 7/Home
    { Qt::Key_Up,        0xffff, '8',    0xffff }, // Keypad 8/Up
    { Qt::Key_PageUp,    0xffff, '9',    0xffff }, // Keypad 9/PgUp
    { Qt::Key_Minus,     '-',    '-',    0xffff }, // Keypad -
    { Qt::Key_Left,      0xffff, '4',    0xffff }, // Keypad 4/Left
    { Qt::Key_5,         0xffff, '5',    0xffff }, // Keypad 5
    { Qt::Key_Right,     0xffff, '6',    0xffff }, // Keypad 6/Right
    { Qt::Key_Plus,      '+',    '+',    0xffff }, // Keypad +
    { Qt::Key_End,       0xffff, '1',    0xffff }, // Keypad 1/End
    // 0x50
    { Qt::Key_Down,      0xffff, '2',    0xffff }, // Keypad 2/Down
    { Qt::Key_PageDown,  0xffff, '3',    0xffff }, // Keypad 3/PgDn
    { Qt::Key_Insert,    0xffff, '0',    0xffff }, // Keypad 0/Ins
    { Qt::Key_Delete,    0xffff, '.',    0xffff }, // Keypad ./Del
    { Qt::Key_SysReq,    0xffff, 0xffff, 0xffff },
    { Qt::Key_unknown,   0xffff, 0xffff, 0xffff },
    { Qt::Key_unknown,   0xffff, 0xffff, 0xffff },
    { Qt::Key_F11,       0xffff, 0xffff, 0xffff },
    { Qt::Key_F12,       0xffff, 0xffff, 0xffff },
};

const int QHillsonKeyboardHandler::m_keyMapSize =
    sizeof(m_keyMap) / sizeof(m_keyMap[0]);

// Global keyboard handler
QHillsonKeyboardHandler *qt_keyboard_handler = 0;

// ============================================================
// QHillsonKeyboardHandler implementation
// ============================================================

QHillsonKeyboardHandler::QHillsonKeyboardHandler()
    : QWSKeyboardHandler(), m_scancode(0)
{
}

QHillsonKeyboardHandler::~QHillsonKeyboardHandler() {}

bool QHillsonKeyboardHandler::doKey(unsigned char code)
{
    // Handle E0 (extended) and E1 prefixes
    if (code == 0xE0) {
        m_extended = 1;
        return false;
    }
    if (code == 0xE1) {
        m_extended = 2;
        return false;
    }

    int keyIndex = code & 0x7F;       // Strip release bit
    bool isRelease = (code & 0x80);

    // Dedup: ignore repeated make codes for non-modifier keys
    // (QEMU typematic can send duplicate make codes before break)
    static unsigned char s_lastMake = 0;
    static bool s_lastWasRelease = true;
    if (!isRelease && keyIndex < 0x60) {
        bool isModifier = (keyIndex == 0x2A || keyIndex == 0x36  // Shift
                        || keyIndex == 0x1D                     // Ctrl
                        || keyIndex == 0x38                     // Alt
                        || keyIndex == 0x3A);                   // CapsLock
        if (!isModifier) {
            if ((unsigned char)keyIndex == s_lastMake && !s_lastWasRelease) {
                return false;  // skip typematic repeat
            }
            s_lastMake = (unsigned char)keyIndex;
            s_lastWasRelease = false;
        }
    }
    if (isRelease) {
        if (keyIndex == s_lastMake) s_lastWasRelease = true;
    }

    // For extended keys, offset into the extended part of the table
    if (m_extended == 1 && keyIndex < 0x60) {
        // Extended keys: index 0x47=Home, 0x48=Up, 0x49=PGUp, 0x4B=Left,
        // 0x4D=Right, 0x4F=End, 0x50=Down, 0x51=PGDn, 0x52=Ins, 0x53=Del
        // Map some common extended keys
        int extKey = Qt::Key_unknown;
        switch (keyIndex) {
            case 0x1C: extKey = Qt::Key_Enter; break;   // Keypad Enter
            case 0x35: extKey = Qt::Key_Slash; break;    // Keypad /
            case 0x47: extKey = Qt::Key_Home; break;
            case 0x48: extKey = Qt::Key_Up; break;
            case 0x49: extKey = Qt::Key_PageUp; break;
            case 0x4B: extKey = Qt::Key_Left; break;
            case 0x4D: extKey = Qt::Key_Right; break;
            case 0x4F: extKey = Qt::Key_End; break;
            case 0x50: extKey = Qt::Key_Down; break;
            case 0x51: extKey = Qt::Key_PageDown; break;
            case 0x52: extKey = Qt::Key_Insert; break;
            case 0x53: extKey = Qt::Key_Delete; break;
            default: extKey = Qt::Key_unknown; break;
        }

        int mod = m_modifiers;
        processKeyEvent(0, extKey, mod, !isRelease, false);
        m_extended = 0;
        return !isRelease;  // true only for make events
    }

    if (m_extended) {
        m_extended = 0;
        return false;  // E1 prefix keys not handled
    }

    // Bounds check
    if (keyIndex >= m_keyMapSize) return false;

    const QWSKeyMap &km = m_keyMap[keyIndex];

    // Handle modifier keys
    switch (km.key_code) {
        case Qt::Key_Shift:
            m_shift = !isRelease;
            if (m_shift) m_modifiers |= Qt::ShiftModifier;
            else         m_modifiers &= ~Qt::ShiftModifier;
            return false;
        case Qt::Key_Control:
            m_ctrl = !isRelease;
            if (m_ctrl) m_modifiers |= Qt::ControlModifier;
            else        m_modifiers &= ~Qt::ControlModifier;
            return false;
        case Qt::Key_Alt:
            m_alt = !isRelease;
            if (m_alt) m_modifiers |= Qt::AltModifier;
            else       m_modifiers &= ~Qt::AltModifier;
            return false;
        case Qt::Key_CapsLock:
            if (!isRelease) m_caps = !m_caps;
            return false;
        default:
            break;
    }

    if (isRelease) return false;  // Don't process release events for regular keys

    // Determine Unicode value based on modifiers
    int unicode = km.unicode;
    if (m_shift || m_caps) {
        if (km.shift_unicode != 0xffff)
            unicode = km.shift_unicode;
    }
    if (m_ctrl) {
        if (km.ctrl_unicode != 0xffff)
            unicode = km.ctrl_unicode;
    }

    // For letter keys, handle Caps Lock inversion
    if (m_caps && !m_shift) {
        if (unicode >= 'a' && unicode <= 'z')
            unicode = unicode - 'a' + 'A';
        else if (unicode >= 'A' && unicode <= 'Z')
            unicode = unicode - 'A' + 'a';
    }

    processKeyEvent(unicode, km.key_code, m_modifiers, true, false);
    return true;
}

bool QHillsonKeyboardHandler::poll()
{
    input_event_t ev;
    int r = 0;
    __asm__ volatile("int $0x80"
        : "=a"(r)
        : "a"(SYS_GUI_INPUT_READ), "b"(&ev), "c"(1)  // type=1 (keyboard)
        : "memory", "cc");

    if (r == 1 && ev.type == 1) {
        return doKey((unsigned char)(ev.x & 0xFF));
    }
    return false;
}
