/**
 * @file qmousehillson_qws.h
 * @brief Mouse driver for HillsonOS — reads USB mouse via syscall 72
 *
 * Calls SYS_GUI_INPUT_READ (syscall 72, type=2) for mouse data.
 */
#ifndef QMOUSE_HILLSON_QWS_H
#define QMOUSE_HILLSON_QWS_H

#include "qpoint_qt.h"

// Mouse handler base class
class QWSMouseHandler {
public:
    QWSMouseHandler();
    virtual ~QWSMouseHandler();

    // Send a mouse event — called by subclass when data arrives
    void mouseChanged(const QPoint &pos, int buttonState);

    // Current mouse position
    const QPoint &pos() const { return m_mousePos; }

    // Constrain to screen bounds
    void limitToScreen(QPoint &pt);

protected:
    QPoint m_mousePos;  // references QWSServer::mousePosition in real Qt
};

// ---- HillsonOS Mouse Handler ----

class QHillsonMouseHandler : public QWSMouseHandler {
public:
    QHillsonMouseHandler();
    ~QHillsonMouseHandler();

    // Called from event loop each iteration — polls syscall 72 type=2
    // Returns true if mouse state changed
    bool poll(int screenWidth, int screenHeight);

private:
    int m_oldButtons;
};

// Global mouse handler (set by QApplication init)
extern QHillsonMouseHandler *qt_mouse_handler;

#endif // QMOUSE_HILLSON_QWS_H
