/**
 * @file qterminal.h
 * @brief Virtual terminal widget — command input and output
 */
#ifndef QTERMINAL_H
#define QTERMINAL_H

#include "qwidget.h"

class QDesktop;
class QDesktopWindow;

QDesktopWindow *createTerminalApp(QDesktop *desktop);

class QTerminal : public QWidget {
public:
    QTerminal(QWidget *parent, const char *name);
    ~QTerminal() override;

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QTerminal"; }

    bool keyPress(int scancode, bool shift);
    void scrollUp();
    void scrollDown();

private:
    void executeCommand(const char *cmd);
    void appendOutput(const char *s);
    void appendPrompt();
    void runBuiltin(const char *cmd);

    char *m_buf;       // output buffer
    int   m_bufLen;
    int   m_bufMax;
    char  m_cmdBuf[256];
    int   m_cmdLen;
    int   m_scrollOffset;
};

#endif // QTERMINAL_H
