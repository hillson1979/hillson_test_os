/**
 * @file qtextedit.h
 * @brief QTextEdit — text editor widget with cursor and keyboard input
 *
 * Renders the document with:
 *  - Line numbers (4-char wide gutter)
 *  - Blinking cursor
 *  - Scroll support (cursor stays visible)
 *
 * Keyboard: PS/2 Set 2 scancodes → actions
 */

#ifndef QTEXTEDIT_H
#define QTEXTEDIT_H

#include "qwidget.h"
#include "qtextdocument.h"

class QTextEdit : public QWidget {
public:
    QTextEdit(QWidget *parent, const char *name);
    ~QTextEdit() override;

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "QTextEdit"; }

    // --- Keyboard input ---
    // scancode = PS/2 Set 2 scancode
    // shift = true if shift is held
    // Returns true if the key was handled
    bool keyPress(int scancode, bool shift);

    // --- Document ---
    QTextDocument *document() { return m_doc; }
    void setPlainText(const char *text);

    // --- Cursor ---
    int cursorLine() const { return m_cursorLine; }
    int cursorCol() const  { return m_cursorCol; }
    void setCursorScreenPos(int px, int py);  // pixel → (line,col)

    // --- Scroll ---
    void ensureCursorVisible();

private:
    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorUp();
    void moveCursorDown();
    void moveCursorHome();
    void moveCursorEnd();

    QTextDocument *m_doc;
    int m_cursorLine;
    int m_cursorCol;
    int m_scrollLine;     // first visible line of the document
    int m_visibleLines;   // how many lines fit in the widget

    // Gutter width for line numbers (pixels)
    static const int GUTTER_W = 32;

    // Convert scancode + shift → ASCII char
    // Returns 0 if scancode doesn't produce a character
    static char scancodeToAscii(int scancode, bool shift);
};

#endif // QTEXTEDIT_H
