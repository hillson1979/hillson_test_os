/**
 * @file qdesktop.h
 * @brief QDesktop — root desktop widget (wallpaper, icons, taskbar, window list)
 */
#ifndef QDESKTOP_H
#define QDESKTOP_H

#include "qwidget.h"
#include "qdesktopwindow.h"

class QPainter;

#define MAX_WINDOWS 16
#define MAX_ICONS   8

struct DesktopIcon {
    int x, y, w, h;
    const char *name;
    uint32_t color;
    void (*onClick)(void *userData);
    void *userData;
};

class QDesktop : public QWidget {
public:
    QDesktop(int fbWidth, int fbHeight);
    ~QDesktop() override;

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QDesktop"; }

    // Window management
    QDesktopWindow *addWindow(const char *title, QWidget *content, int w, int h);
    void removeWindow(QDesktopWindow *win);
    void focusWindow(QDesktopWindow *win);
    QDesktopWindow *focusedWindow() const { return m_focusedWindow; }

    // Icon management
    void addIcon(int x, int y, const char *name, uint32_t color,
                 void (*onClick)(void *userData), void *userData);

    // Input handling (returns true if event was consumed)
    // buttons: bit0=left, bit1=right, bit2=middle
    bool handleMouse(int mx, int my, int buttons, bool *needRender);
    void updateDrag(int mx, int my);
    bool isDragging() const { return m_dragging; }
    bool handleKey(int scancode, bool shift, bool *needRender);

    // Show desktop (minimize all / restore all)
    void toggleShowDesktop();
    bool isShowingDesktop() const { return m_showDesktop; }

    // Taskbar
    int taskbarY() const { return m_height - TASKBAR_H; }
    static const int TASKBAR_H = 30;

    // Accessors
    QDesktopWindow *window(int idx) const;
    int windowCount() const { return m_numWindows; }

    int fbWidth() const { return m_width; }
    int fbHeight() const { return m_height; }

private:
    void bringToFront(QDesktopWindow *win);
    void paintTaskbar(QPainter *painter);
    void paintIcons(QPainter *painter);
    void paintWallpaper(QPainter *painter);

    int m_width, m_height;
    QDesktopWindow *m_windows[MAX_WINDOWS];
    int m_numWindows;
    QDesktopWindow *m_focusedWindow;

    DesktopIcon m_icons[MAX_ICONS];
    int m_numIcons;

    // Drag/resize state
    bool m_dragging;
    int  m_dragStartX, m_dragStartY;
    int  m_dragWinX, m_dragWinY, m_dragWinW, m_dragWinH;
    QDesktopWindow *m_dragWindow;
    QDesktopWindow::HitZone m_dragZone;

    // Show desktop (minimize all)
    bool m_showDesktop;
    int  m_savedWinX[MAX_WINDOWS], m_savedWinY[MAX_WINDOWS];
    int  m_savedWinW[MAX_WINDOWS], m_savedWinH[MAX_WINDOWS];
    bool m_savedWinVisible[MAX_WINDOWS];

    // Clock
    int m_tickCount;
};

#endif // QDESKTOP_H
