/**
 * @file qtexteditorapp.cpp
 * @brief Text editor application wrapped in a desktop window
 */
#include "qtexteditorapp.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"
#include "qtextedit.h"

extern "C" {
#include "libuser_minimal.h"
}

// Helper to forward key events to QTextEdit
bool qtextedit_keyPress(void *editor, int scancode, bool shift) {
    if (!editor) return false;
    return ((QTextEdit*)editor)->keyPress(scancode, shift);
}

QDesktopWindow *createTextEditorApp(QDesktop *desktop) {
    int ww = 700, wh = 460;
    QTextEdit *edit = new QTextEdit(nullptr, "editor");
    edit->setGeometry(0, 0, ww - 2*QDesktopWindow::BORDER_W,
                      wh - QDesktopWindow::TITLE_H - 3*QDesktopWindow::BORDER_W);
    edit->setBackgroundColor(COLOR_WHITE);

    QDesktopWindow *win = desktop->addWindow("Text Editor", edit, ww, wh);

    // Status bar is drawn by the editor widget itself at bottom
    return win;
}
