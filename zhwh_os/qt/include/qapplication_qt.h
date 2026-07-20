/**
 * @file qapplication_qt.h
 * @brief QApplication — Qt/Embedded application main class for HillsonOS
 *
 * Manages screen, keyboard, mouse drivers and the main event loop.
 */
#ifndef QAPPLICATION_QT_H
#define QAPPLICATION_QT_H

class QWidget;
class QScreen;
class QHillsonKeyboardHandler;
class QHillsonMouseHandler;

class QApplication {
public:
    /**
     * Constructor — initializes screen, keyboard, and mouse drivers.
     * The QScreen framebuffer is set to 0xF0000000 (kernel-mapped).
     */
    QApplication();

    /**
     * Set the main widget (top-level window).
     * This widget fills the entire screen and receives keyboard events.
     */
    void setMainWidget(QWidget *w);

    /**
     * Enter the main event loop. Does NOT return until the app exits.
     * Polls keyboard/mouse via syscalls and dispatches to widgets.
     * Re-renders dirty widgets to the framebuffer.
     */
    int exec();

    // --- Accessors ---
    QScreen  *screen()  const { return m_screen; }
    QHillsonKeyboardHandler *keyboardHandler() const { return m_kbd; }
    QHillsonMouseHandler    *mouseHandler()    const { return m_mouse; }

    // --- Globals (set during construction) ---
    static QApplication *instance() { return s_instance; }

private:
    QScreen  *m_screen;
    QHillsonKeyboardHandler *m_kbd;
    QHillsonMouseHandler    *m_mouse;
    QWidget  *m_mainWidget;
    bool      m_quit;

    static QApplication *s_instance;

    void processKeyEvent(int unicode, int keycode, int modifiers, bool isPress);
    void processMouseEvent(int x, int y, int buttons);
};

// Convenience function to get framebuffer info
struct FbInfo {
    void *addr;
    unsigned int width, height, pitch, bpp;
};

// Syscall wrappers
int gui_get_fb_info(FbInfo *fb);

#endif // QAPPLICATION_QT_H
