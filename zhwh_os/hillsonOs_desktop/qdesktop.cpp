/**
 * @file qdesktop.cpp
 * @brief QDesktop — root desktop widget implementation
 */
#include "qdesktop.h"
#include "qpainter.h"

extern "C" void term_keyPress(void *w, int sc, bool sh);
extern "C" void term_qtKeyPress(void *w, int qtKey, int uni, bool sh);

QDesktop::QDesktop(int fbWidth, int fbHeight)
    : QWidget(nullptr, "desktop")
{
    m_width = fbWidth;
    m_height = fbHeight;
    setGeometry(0, 0, fbWidth, fbHeight);
    m_bgColor = 0x00336699;  // teal wallpaper

    m_numWindows = 0;
    m_focusedWindow = nullptr;
    m_numIcons = 0;

    m_dragging = false;
    m_dragWindow = nullptr;
    m_dragZone = QDesktopWindow::HIT_NONE;
    m_tickCount = 0;
    m_showDesktop = false;

    for (int i = 0; i < MAX_WINDOWS; i++) m_windows[i] = nullptr;
}

QDesktop::~QDesktop() {
    for (int i = 0; i < m_numWindows; i++) delete m_windows[i];
}

QDesktopWindow *QDesktop::window(int idx) const {
    if (idx < 0 || idx >= m_numWindows) return nullptr;
    return m_windows[idx];
}

QDesktopWindow *QDesktop::addWindow(const char *title, QWidget *content, int w, int h) {
    if (m_numWindows >= MAX_WINDOWS) return nullptr;

    // Center first window, cascade subsequent ones
    int wx, wy;
    if (m_numWindows == 0) {
        wx = (m_width - w) / 2;
        wy = (m_height - TASKBAR_H - h) / 2;
    } else {
        int cascade = ((m_numWindows - 1) % 6) * 30;
        wx = 100 + cascade;
        wy = 60 + cascade;
    }
    if (wx + w > m_width - 20) wx = m_width - w - 20;
    if (wy + h > m_height - TASKBAR_H - 20) wy = m_height - TASKBAR_H - h - 20;
    if (wx < 0) wx = 0;
    if (wy < 0) wy = 0;

    QDesktopWindow *win = new QDesktopWindow(this, "window", title);
    win->setGeometry(wx, wy, w, h);
    win->setContent(content);

    m_windows[m_numWindows++] = win;
    focusWindow(win);
    return win;
}

void QDesktop::removeWindow(QDesktopWindow *win) {
    for (int i = 0; i < m_numWindows; i++) {
        if (m_windows[i] == win) {
            if (m_focusedWindow == win) m_focusedWindow = nullptr;
            // Shift remaining windows
            for (int j = i; j < m_numWindows - 1; j++)
                m_windows[j] = m_windows[j + 1];
            m_windows[--m_numWindows] = nullptr;
            if (m_numWindows > 0 && !m_focusedWindow)
                focusWindow(m_windows[m_numWindows - 1]);
            return;
        }
    }
}

void QDesktop::bringToFront(QDesktopWindow *win) {
    int idx = -1;
    for (int i = 0; i < m_numWindows; i++) {
        if (m_windows[i] == win) { idx = i; break; }
    }
    if (idx >= 0 && idx < m_numWindows - 1) {
        for (int j = idx; j < m_numWindows - 1; j++)
            m_windows[j] = m_windows[j + 1];
        m_windows[m_numWindows - 1] = win;
    }
}

void QDesktop::focusWindow(QDesktopWindow *win) {
    if (m_focusedWindow) m_focusedWindow->setFocused(false);
    m_focusedWindow = win;
    if (m_focusedWindow) m_focusedWindow->setFocused(true);
    bringToFront(win);
}

void QDesktop::addIcon(int x, int y, const char *name, uint32_t color,
                       void (*onClick)(void *userData), void *userData) {
    if (m_numIcons >= MAX_ICONS) return;
    DesktopIcon *ic = &m_icons[m_numIcons++];
    ic->x = x; ic->y = y; ic->w = 64; ic->h = 80;
    ic->name = name;
    ic->color = color;
    ic->onClick = onClick;
    ic->userData = userData;
}

void QDesktop::updateDrag(int mx, int my) {
    if (!m_dragging || !m_dragWindow) return;
    int dx = mx - m_dragStartX;
    int dy = my - m_dragStartY;

    if (m_dragZone == QDesktopWindow::HIT_TITLEBAR) {
        // Move window
        int nx = m_dragWinX + dx;
        int ny = m_dragWinY + dy;
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;
        if (nx + m_dragWindow->width() > m_width)
            nx = m_width - m_dragWindow->width();
        if (ny + m_dragWindow->height() > m_height - TASKBAR_H)
            ny = m_height - TASKBAR_H - m_dragWindow->height();
        m_dragWindow->setGeometry(nx, ny,
            m_dragWindow->width(), m_dragWindow->height());
    } else {
        // Resize window
        int nw = m_dragWinW, nh = m_dragWinH;
        int nx = m_dragWinX, ny = m_dragWinY;
        switch (m_dragZone) {
            case QDesktopWindow::HIT_EDGE_E:  nw = m_dragWinW + dx; break;
            case QDesktopWindow::HIT_EDGE_S:  nh = m_dragWinH + dy; break;
            case QDesktopWindow::HIT_EDGE_W:  nx = m_dragWinX + dx; nw = m_dragWinW - dx; break;
            case QDesktopWindow::HIT_EDGE_N:  ny = m_dragWinY + dy; nh = m_dragWinH - dy; break;
            case QDesktopWindow::HIT_EDGE_SE: nw = m_dragWinW + dx; nh = m_dragWinH + dy; break;
            case QDesktopWindow::HIT_EDGE_NE: ny = m_dragWinY + dy; nw = m_dragWinW + dx; nh = m_dragWinH - dy; break;
            case QDesktopWindow::HIT_EDGE_SW: nx = m_dragWinX + dx; nw = m_dragWinW - dx; nh = m_dragWinH + dy; break;
            case QDesktopWindow::HIT_EDGE_NW: nx = m_dragWinX + dx; ny = m_dragWinY + dy; nw = m_dragWinW - dx; nh = m_dragWinH - dy; break;
            default: break;
        }
        if (nw < 200) nw = 200;
        if (nh < 150) nh = 150;
        m_dragWindow->setGeometry(nx, ny, nw, nh);
    }
}

void QDesktop::toggleShowDesktop() {
    if (m_showDesktop) {
        // Restore all windows
        for (int i = 0; i < m_numWindows; i++) {
            if (m_windows[i] && m_savedWinVisible[i]) {
                m_windows[i]->setGeometry(m_savedWinX[i], m_savedWinY[i],
                                          m_savedWinW[i], m_savedWinH[i]);
                m_windows[i]->show();
            }
        }
        m_showDesktop = false;
    } else {
        // Minimize all windows
        for (int i = 0; i < m_numWindows; i++) {
            if (m_windows[i]) {
                m_savedWinX[i] = m_windows[i]->x();
                m_savedWinY[i] = m_windows[i]->y();
                m_savedWinW[i] = m_windows[i]->width();
                m_savedWinH[i] = m_windows[i]->height();
                m_savedWinVisible[i] = m_windows[i]->isVisible();
                m_windows[i]->hide();
            }
        }
        m_showDesktop = true;
    }
}

bool QDesktop::handleMouse(int mx, int my, int buttons, bool *needRender) {
    if (!buttons) {
        // Mouse release: end drag
        if (m_dragging) {
            m_dragging = false;
            m_dragWindow = nullptr;
            *needRender = true;
        }
        return false;
    }

    bool isLeft  = (buttons & 1) != 0;
    bool isRight = (buttons & 2) != 0;

    // Right-click on empty desktop: toggle show desktop
    if (isRight && !isLeft) {
        // Check if click is on empty desktop (not on any window or icon)
        bool hitWindow = false;
        for (int i = m_numWindows - 1; i >= 0; i--) {
            if (m_windows[i] && m_windows[i]->isVisible() &&
                m_windows[i]->hitTest(mx, my) != QDesktopWindow::HIT_NONE) {
                hitWindow = true;
                break;
            }
        }
        if (!hitWindow && my < taskbarY()) {
            toggleShowDesktop();
            *needRender = true;
            return true;
        }
        // Right-click on window title bar: close that window
        for (int i = m_numWindows - 1; i >= 0; i--) {
            QDesktopWindow *win = m_windows[i];
            if (win && win->isVisible()) {
                QDesktopWindow::HitZone zone = win->hitTest(mx, my);
                if (zone == QDesktopWindow::HIT_TITLEBAR ||
                    zone == QDesktopWindow::HIT_CLOSE) {
                    focusWindow(win);
                    removeWindow(win);
                    delete win;
                    *needRender = true;
                    return true;
                }
            }
        }
        return false;
    }

    // Left-click handling
    if (!isLeft) return false;

    // Check taskbar first
    if (my >= taskbarY()) {
        // Show Desktop button (bottom-right)
        if (mx >= m_width - 56 && mx <= m_width - 8) {
            toggleShowDesktop();
            *needRender = true;
            return true;
        }
        // Taskbar: click on window buttons
        int bx = 4;
        for (int i = 0; i < m_numWindows; i++) {
            if (mx >= bx && mx < bx + 120 && m_windows[i]) {
                focusWindow(m_windows[i]);
                *needRender = true;
                return true;
            }
            bx += 124;
        }
        *needRender = true;
        return true;
    }

    // Check desktop icons
    for (int i = 0; i < m_numIcons; i++) {
        DesktopIcon *ic = &m_icons[i];
        if (mx >= ic->x && mx < ic->x + ic->w &&
            my >= ic->y && my < ic->y + ic->h) {
            if (ic->onClick) ic->onClick(ic->userData);
            *needRender = true;
            return true;
        }
    }

    // Check windows (topmost = last in array = drawn last)
    for (int i = m_numWindows - 1; i >= 0; i--) {
        QDesktopWindow *win = m_windows[i];
        QDesktopWindow::HitZone zone = win->hitTest(mx, my);
        if (zone != QDesktopWindow::HIT_NONE) {
            focusWindow(win);

            if (zone == QDesktopWindow::HIT_CLOSE) {
                removeWindow(win);
                delete win;
                *needRender = true;
                return true;
            }

            if (zone == QDesktopWindow::HIT_TITLEBAR) {
                // Start drag
                m_dragging = true;
                m_dragWindow = win;
                m_dragZone = zone;
                m_dragStartX = mx;
                m_dragStartY = my;
                m_dragWinX = win->x();
                m_dragWinY = win->y();
                return true;
            }

            if (zone >= QDesktopWindow::HIT_EDGE_N &&
                zone <= QDesktopWindow::HIT_EDGE_SW) {
                // Start resize
                m_dragging = true;
                m_dragWindow = win;
                m_dragZone = zone;
                m_dragStartX = mx;
                m_dragStartY = my;
                m_dragWinX = win->x();
                m_dragWinY = win->y();
                m_dragWinW = win->width();
                m_dragWinH = win->height();
                return true;
            }

            // Content area click — pass to content
            *needRender = true;
            return true;
        }
    }

    return false;
}

bool QDesktop::handleKey(int scancode, bool shift, bool *needRender) {
    // Legacy wrapper — new code should use handleQtKey
    (void)scancode; (void)shift; (void)needRender;
    return false;
}

bool QDesktop::handleQtKey(int qtKey, int unicode, bool shift, bool *needRender) {
    // Forward to focused window's content widget
    if (m_focusedWindow && m_focusedWindow->content()) {
        QWidget *content = m_focusedWindow->content();
        const char *cn = content->className();
        // QTerminal (check before QTextEdit — both start with "QTe")
        if (cn[0] == 'Q' && cn[1] == 'T' && cn[2] == 'e' && cn[3] == 'r') {
            term_qtKeyPress(content, qtKey, unicode, shift);
            *needRender = true;
            return true;
        }
        // QTextEdit
        if (cn[0] == 'Q' && cn[1] == 'T' && cn[2] == 'e') {
            extern bool qtextedit_qtKeyPress(void *editor, int qtKey, int uni, bool sh);
            qtextedit_qtKeyPress(content, qtKey, unicode, shift);
            *needRender = true;
            return true;
        }
    }

    return false;
}

void QDesktop::paintWallpaper(QPainter *painter) {
    painter->setColor(m_bgColor);
    painter->fillRect(0, 0, m_width, m_height - TASKBAR_H);

    // Keyboard help text (bottom-right of desktop area)
    int hy = m_height - TASKBAR_H - 80;
    painter->setColor(0x0080A0C0);
    const char *help[] = {
        "Arrows: Move cursor  Enter/Space: Click",
        "Tab: Switch window   ESC: Close window",
        "F5: Refresh          Right-click: Show desktop",
        nullptr
    };
    for (int i = 0; help[i]; i++) {
        int tw = painter->textWidth(help[i]);
        painter->drawText(m_width - tw - 16, hy + i * 14, help[i]);
    }

    // Debug: show last scancode
    extern int g_lastScancode;  // set by desktop.cpp
    if (g_lastScancode) {
        painter->setColor(0x00FFFF00);
        char dbg[32]; int di = 0;
        dbg[di++] = 'S'; dbg[di++] = 'C'; dbg[di++] = ':'; dbg[di++] = '0'; dbg[di++] = 'x';
        for (int n = 28; n >= 0; n -= 4)
            dbg[di++] = "0123456789ABCDEF"[(g_lastScancode >> n) & 0xF];
        dbg[di] = 0;
        painter->drawText(8, hy, dbg);
    }
}

void QDesktop::paintIcons(QPainter *painter) {
    for (int i = 0; i < m_numIcons; i++) {
        DesktopIcon *ic = &m_icons[i];
        // Icon background
        painter->setColor(ic->color);
        painter->fillRect(ic->x + 8, ic->y, ic->w - 16, ic->w - 16);
        // Icon label
        painter->setColor(COLOR_WHITE);
        int tw = painter->textWidth(ic->name);
        painter->drawText(ic->x + (ic->w - tw) / 2, ic->y + ic->w - 10, ic->name);
    }
}

void QDesktop::paintTaskbar(QPainter *painter) {
    int ty = taskbarY();
    // Taskbar background
    painter->setColor(0x00202040);
    painter->fillRect(0, ty, m_width, TASKBAR_H);

    // Window buttons
    int bx = 4;
    for (int i = 0; i < m_numWindows; i++) {
        if (!m_windows[i]) continue;
        bool active = (m_windows[i] == m_focusedWindow);
        painter->setColor(active ? 0x004060A0 : 0x00303050);
        painter->fillRect(bx, ty + 4, 120, TASKBAR_H - 8);
        painter->setColor(COLOR_WHITE);
        painter->drawText(bx + 4, ty + 7, m_windows[i]->title());
        bx += 124;
    }

    // Show Desktop button (bottom-right corner)
    painter->setColor(m_showDesktop ? 0x0080C0FF : 0x00404060);
    painter->fillRect(m_width - 56, ty + 4, 48, TASKBAR_H - 8);
    painter->setColor(COLOR_WHITE);
    painter->drawText(m_width - 52, ty + 7, m_showDesktop ? "[R]" : "[=]");

    // Clock
    m_tickCount++;
    int sec = m_tickCount / 20;  // ~20 ticks/sec approximate
    int min = sec / 60;
    int hr = min / 60;
    sec %= 60; min %= 60; hr %= 24;

    char clockStr[16];
    int ci = 0;
    if (hr < 10) clockStr[ci++] = '0';
    int hi = hr; if (hi >= 10) { clockStr[ci++] = '0' + hi/10; hi%=10; }
    clockStr[ci++] = '0' + hi;
    clockStr[ci++] = ':';
    if (min < 10) clockStr[ci++] = '0';
    clockStr[ci++] = '0' + min/10; clockStr[ci++] = '0' + min%10;
    clockStr[ci++] = ':';
    if (sec < 10) clockStr[ci++] = '0';
    clockStr[ci++] = '0' + sec/10; clockStr[ci++] = '0' + sec%10;
    clockStr[ci] = 0;

    painter->setColor(COLOR_WHITE);
    int tw = painter->textWidth(clockStr);
    painter->drawText(m_width - tw - 70, ty + 7, clockStr);
}

void QDesktop::paintEvent(QPainter *painter) {
    paintWallpaper(painter);

    // Show desktop indicator
    if (m_showDesktop) {
        painter->setColor(COLOR_WHITE);
        const char *msg = "Right-click desktop to restore windows";
        int tw = painter->textWidth(msg);
        painter->drawText((m_width - tw) / 2, m_height / 2 - 20, msg);
    }

    paintIcons(painter);

    // Draw windows from bottom to top (skip hidden)
    for (int i = 0; i < m_numWindows; i++) {
        if (m_windows[i] && m_windows[i]->isVisible()) {
            painter->setClipRect(
                m_windows[i]->x(), m_windows[i]->y(),
                m_windows[i]->width(), m_windows[i]->height());
            m_windows[i]->render(painter,
                m_windows[i]->x(), m_windows[i]->y());
            painter->clearClip();
        }
    }

    // Drag/resize happening — draw outline
    if (m_dragging && m_dragWindow) {
        int wx = m_dragWindow->x(), wy = m_dragWindow->y();
        int ww = m_dragWindow->width(), wh = m_dragWindow->height();
        painter->setColor(0x00FFFF00);
        painter->drawRect(wx, wy, ww, wh);
        painter->drawRect(wx+1, wy+1, ww-2, wh-2);
    }

    paintTaskbar(painter);
}
