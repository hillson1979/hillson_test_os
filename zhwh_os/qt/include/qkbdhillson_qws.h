/**
 * @file qkbdhillson_qws.h
 * @brief Keyboard driver for HillsonOS — reads PS/2 Set1 scancodes via syscall 72
 *
 * Uses the pc101KeyM mapping table from Qt/Embedded 3.3.8b.
 * Calls SYS_GUI_INPUT_READ (syscall 72, type=1) for keyboard events.
 */
#ifndef QKBD_HILLSON_QWS_H
#define QKBD_HILLSON_QWS_H

#include "qnamespace_qt.h"

// Qt/Embedded key map entry (from qkbdpc101_qws.h)
struct QWSKeyMap {
    unsigned short key_code;       // Qt::Key enum value
    unsigned short unicode;        // Normal unicode
    unsigned short shift_unicode;  // Shift-modified unicode
    unsigned short ctrl_unicode;   // Ctrl-modified unicode
};

// Keyboard handler base class
class QWSKeyboardHandler {
public:
    QWSKeyboardHandler();
    virtual ~QWSKeyboardHandler();

    // Inject a key event into Qt's event system
    void processKeyEvent(int unicode, int keycode, int modifiers,
                         bool isPress, bool autoRepeat);

    // Accessors for QApplication event loop
    int  lastUnicode()  const { return m_prevUnicode; }
    int  lastKeyCode()  const { return m_prevKeyCode; }
    int  lastModifiers() const { return m_modifiers; }
    bool isShift() const { return m_shift; }
    bool isCtrl()  const { return m_ctrl; }
    bool isAlt()   const { return m_alt; }

    // Transform arrow keys for current orientation
    int transformDirKey(int key);

protected:
    bool m_shift, m_alt, m_ctrl, m_caps;
    int  m_modifiers;
    int  m_extended;  // 0=none, 1=E0 prefix, 2=E1 prefix
    int  m_prevUnicode, m_prevKeyCode;
};

// ---- HillsonOS PC101 Keyboard Handler ----

class QHillsonKeyboardHandler : public QWSKeyboardHandler {
public:
    QHillsonKeyboardHandler();
    ~QHillsonKeyboardHandler();

    // Called from event loop each iteration — polls syscall 72
    // Returns true if a key event was processed
    bool poll();

    // Process a single Set1 scancode byte
    // Returns true if a make event was dispatched (for poll() dedup)
    bool doKey(unsigned char scancode);

private:
    // Map Set1 scancode index → Qt key definition
    static const QWSKeyMap m_keyMap[];
    static const int m_keyMapSize;

    int m_scancode;  // accumulated scancode for multi-byte sequences
};

// Global keyboard handler (set by QApplication init)
extern QHillsonKeyboardHandler *qt_keyboard_handler;

#endif // QKBD_HILLSON_QWS_H
