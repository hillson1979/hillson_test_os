/**
 * @file qwidget.h
 * @brief QWidget — base class for all UI widgets
 *
 * Inherits QObject for signal/slot and parent-child tree.
 * Adds geometry, visibility, and virtual paintEvent.
 */

#ifndef QWIDGET_H
#define QWIDGET_H

#include "qobject.h"
#include "stdint_compat.h"

class QPainter;

class QWidget : public QObject {
public:
    explicit QWidget(QWidget *parent = nullptr, const char *name = "");
    virtual ~QWidget();

    // --- Geometry ---
    void setGeometry(int x, int y, int w, int h);
    int x() const { return m_x; }
    int y() const { return m_y; }
    int width() const { return m_w; }
    int height() const { return m_h; }

    // --- Visibility ---
    void setVisible(bool v);
    bool isVisible() const { return m_visible; }
    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    // --- Background ---
    void setBackgroundColor(uint32_t rgba) { m_bgColor = rgba; }
    uint32_t backgroundColor() const { return m_bgColor; }

    // --- Painting ---
    // Called by the framework when the widget needs to paint itself.
    // Default fills the rect with background color.
    virtual void paintEvent(QPainter *painter);

    // Recursively paint this widget and all visible children.
    // parentAbsX/Y = parent's absolute screen position (0 for top-level).
    void render(QPainter *painter, int parentAbsX = 0, int parentAbsY = 0);

    // Count direct children
    int childCount() const;

    // --- Hit testing ---
    bool contains(int px, int py) const;

    // --- RTTI ---
    virtual const char *className() const override { return "QWidget"; }

protected:
    int  m_x, m_y, m_w, m_h;
    bool m_visible;
    uint32_t m_bgColor;
};

// ============================================================
//  QLabel — simple text label widget
// ============================================================
class QLabel : public QWidget {
public:
    QLabel(QWidget *parent, const char *name, const char *text = "");
    virtual ~QLabel();

    void setText(const char *text);
    const char *text() const { return m_text; }

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QLabel"; }

private:
    char *m_text;
};

// ============================================================
//  QPushButton — clickable button widget
// ============================================================
class QPushButton : public QWidget {
public:
    QPushButton(QWidget *parent, const char *name, const char *text = "");
    virtual ~QPushButton();

    void setText(const char *text);
    const char *text() const { return m_text; }

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QPushButton"; }

    // Simulate a click (emits "clicked" signal)
    void click();

    // Hit test — returns true and triggers click if pressed
    bool mousePress(int px, int py);

private:
    char *m_text;
    bool  m_pressed;
};

#endif // QWIDGET_H
