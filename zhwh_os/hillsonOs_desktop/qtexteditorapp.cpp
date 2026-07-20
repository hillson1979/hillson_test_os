/**
 * @file qtexteditorapp.cpp
 * @brief Text editor application wrapped in a desktop window
 */
#include "qtexteditorapp.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"
#include "qtextedit.h"
#include "../qt/include/qnamespace_qt.h"

extern "C" {
#include "libuser_minimal.h"
}

// Helper to forward key events to QTextEdit
bool qtextedit_keyPress(void *editor, int scancode, bool shift) {
    if (!editor) return false;
    return ((QTextEdit*)editor)->keyPress(scancode, shift);
}

bool qtextedit_qtKeyPress(void *editor, int qtKey, int uni, bool /*shift*/) {
    if (!editor) return false;
    QTextEdit *edit = (QTextEdit*)editor;
    int line = edit->cursorLine();
    int col  = edit->cursorCol();

    // Enter
    if (qtKey == Qt::Key_Return || qtKey == Qt::Key_Enter) {
        edit->document()->insertNewline(line, col);
        edit->ensureCursorVisible();
        return true;
    }
    // Backspace
    if (qtKey == Qt::Key_Backspace) {
        if (col > 0) {
            edit->document()->deleteChar(line, col);
        } else if (line > 0) {
            edit->document()->deleteNewline(line - 1);
        }
        edit->ensureCursorVisible();
        return true;
    }
    // Tab — insert 4 spaces
    if (qtKey == Qt::Key_Tab) {
        for (int i = 0; i < 4; i++) edit->document()->insertChar(line, col + i, ' ');
        edit->ensureCursorVisible();
        return true;
    }
    // Arrow keys
    if (qtKey == Qt::Key_Left)  { edit->moveCursorLeft();  return true; }
    if (qtKey == Qt::Key_Right) { edit->moveCursorRight(); return true; }
    if (qtKey == Qt::Key_Up)    { edit->moveCursorUp();    return true; }
    if (qtKey == Qt::Key_Down)  { edit->moveCursorDown();  return true; }
    // Delete
    if (qtKey == Qt::Key_Delete) {
        edit->document()->deleteForward(line, col);
        edit->ensureCursorVisible();
        return true;
    }
    // Printable
    if (uni >= 32 && uni <= 126) {
        edit->document()->insertChar(line, col, (char)uni);
        edit->ensureCursorVisible();
        return true;
    }
    return false;
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
