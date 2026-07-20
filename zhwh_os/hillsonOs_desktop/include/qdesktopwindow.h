/**
 * @file qdesktopwindow.h
 * @brief QDesktopWindow — window frame with title bar, borders, drag, resize
 */
#ifndef QDESKTOPWINDOW_H
#define QDESKTOPWINDOW_H

#include "qwidget.h"

class QPainter;
class QPushButton;

class QDesktopWindow : public QWidget {
public:
    enum HitZone {
        HIT_NONE = 0,
        HIT_TITLEBAR,
        HIT_CONTENT,
        HIT_CLOSE,
        HIT_EDGE_N,  HIT_EDGE_S,  HIT_EDGE_E,  HIT_EDGE_W,
        HIT_EDGE_NE, HIT_EDGE_NW, HIT_EDGE_SE, HIT_EDGE_SW
    };

    QDesktopWindow(QWidget *parent, const char *name, const char *title = "Window");
    ~QDesktopWindow() override;

    void setContent(QWidget *content);
    QWidget *content() const { return m_content; }
    void setTitle(const char *title);
    const char *title() const { return m_title; }

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QDesktopWindow"; }

    // Hit test: given global mouse position, return which zone was hit
    HitZone hitTest(int px, int py) const;

    // Window management
    bool isFocused() const { return m_focused; }
    void setFocused(bool f);

    // Close button area (local coords)
    int closeX() const { return m_w - 24; }
    int closeY() const { return 2; }
    int closeW() const { return 20; }
    int closeH() const { return 18; }

    static const int TITLE_H = 22;
    static const int BORDER_W = 2;
    static const int EDGE_SZ = 6;

private:
    char    *m_title;
    QWidget *m_content;
    bool     m_focused;
};

#endif // QDESKTOPWINDOW_H
