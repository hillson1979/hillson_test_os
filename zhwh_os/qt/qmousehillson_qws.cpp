/**
 * @file qmousehillson_qws.cpp
 * @brief HillsonOS mouse driver — USB mouse via syscall 72 type=2
 */
#include "include/qmousehillson_qws.h"

#define SYS_GUI_INPUT_READ 72

struct input_event_t {
    unsigned int type;    // 2=mouse
    int x;                // mouse X
    int y;                // mouse Y
    unsigned int pressed; // button bitmask (bit0=L, bit1=R, bit2=M)
};

// ============================================================
// QWSMouseHandler base class
// ============================================================

QWSMouseHandler::QWSMouseHandler()
    : m_mousePos(512, 384)  // default: center of 1024x768
{
}

QWSMouseHandler::~QWSMouseHandler() {}

void QWSMouseHandler::mouseChanged(const QPoint &pos, int /*buttonState*/)
{
    m_mousePos = pos;
    // In full Qt, this sends to QWSServer::sendMouseEvent.
}

void QWSMouseHandler::limitToScreen(QPoint &pt)
{
    // Clamped by poll()
}

// ============================================================
// QHillsonMouseHandler implementation
// ============================================================

// Global
QHillsonMouseHandler *qt_mouse_handler = 0;

QHillsonMouseHandler::QHillsonMouseHandler()
    : QWSMouseHandler(), m_oldButtons(0)
{
}

QHillsonMouseHandler::~QHillsonMouseHandler() {}

bool QHillsonMouseHandler::poll(int screenWidth, int screenHeight)
{
    input_event_t ev;
    int r = 0;
    __asm__ volatile("int $0x80"
        : "=a"(r)
        : "a"(SYS_GUI_INPUT_READ), "b"(&ev), "c"(2)  // type=2 (mouse)
        : "memory", "cc");

    if (r != 1) return false;

    int mx = ev.x;
    int my = ev.y;
    int bt = (int)ev.pressed;

    // Clamp to screen
    if (mx < 0) mx = 0;
    if (my < 0) my = 0;
    if (screenWidth  > 0 && mx >= screenWidth)  mx = screenWidth  - 1;
    if (screenHeight > 0 && my >= screenHeight) my = screenHeight - 1;

    bool changed = (mx != m_mousePos._x() || my != m_mousePos._y() || bt != m_oldButtons);

    if (changed) {
        m_mousePos = QPoint(mx, my);
        m_oldButtons = bt;
        mouseChanged(m_mousePos, bt);
    }

    return changed;
}
