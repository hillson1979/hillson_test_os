/**
 * @file qsysinfo.h
 * @brief QSysInfo — system devices & drivers information widget
 */
#ifndef QSYSINFO_H
#define QSYSINFO_H

#include "qwidget.h"

class QDesktop;
class QDesktopWindow;

QDesktopWindow *createSysInfoApp(QDesktop *desktop);

class QSysInfo : public QWidget {
public:
    QSysInfo(QWidget *parent, const char *name);
    ~QSysInfo() override;

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QSysInfo"; }

    void refresh();
    void scrollUp();
    void scrollDown();

private:
    void appendText(const char *s);
    void appendHex(uint32_t v);
    void appendDec(int v);
    void appendNL();
    void appendSection(const char *title);
    void captureKlog(const char *filter);
    void captureSyscallOutput(int (*syscall_fn)(void), const char *filter);

    char *m_text;
    int   m_textLen;
    int   m_textMax;
    int   m_scrollOffset;
};

#endif // QSYSINFO_H
