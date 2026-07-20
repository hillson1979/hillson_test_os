/**
 * @file qusbmonitor.h
 * @brief QUsbMonitor — USB device information display widget
 */
#ifndef QUSBMONITOR_H
#define QUSBMONITOR_H

#include "qwidget.h"

class QDesktop;
class QDesktopWindow;

QDesktopWindow *createUsbMonitorApp(QDesktop *desktop);

class QUsbMonitor : public QWidget {
public:
    QUsbMonitor(QWidget *parent, const char *name);
    ~QUsbMonitor() override;

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QUsbMonitor"; }

    void refresh();
    void scrollUp();
    void scrollDown();

private:
    char *m_text;
    int   m_textLen;
    int   m_scrollOffset;
};

#endif // QUSBMONITOR_H
