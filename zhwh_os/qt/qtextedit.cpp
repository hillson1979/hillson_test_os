/**
 * @file qtextedit.cpp
 * @brief QTextEdit implementation — cursor, keyboard, rendering
 */

#include "qtextedit.h"
#include "qpainter.h"

extern "C" {
#include "libuser_minimal.h"
}

// ============================================================
//  PS/2 Set 1 → ASCII mapping (US QWERTY)
//  内核键盘驱动使用 Set 1 扫描码
//  释放码 = 扫描码 | 0x80
// ============================================================

// Normal (no shift) — indices 0-57
static const char sc1_normal[64] = {
    0,    0x1B, '1', '2', '3', '4', '5', '6',   // 00-07
    '7',  '8',  '9', '0', '-', '=', '\b','\t',   // 08-0F
    'q',  'w',  'e', 'r', 't', 'y', 'u', 'i',    // 10-17
    'o',  'p',  '[', ']','\n',  0,  'a', 's',    // 18-1F
    'd',  'f',  'g', 'h', 'j', 'k', 'l', ';',    // 20-27
    '\'', '`',   0,'\\', 'z', 'x', 'c', 'v',     // 28-2F
    'b',  'n',  'm', ',', '.', '/',  0,  '*',    // 30-37
     0,  ' ',   0,   0,   0,   0,   0,   0,      // 38-3F
};

// Shifted
static const char sc1_shifted[64] = {
    0,    0x1B, '!', '@', '#', '$', '%', '^',   // 00-07
    '&',  '*',  '(', ')', '_', '+', '\b','\t',   // 08-0F
    'Q',  'W',  'E', 'R', 'T', 'Y', 'U', 'I',    // 10-17
    'O',  'P',  '{', '}','\n',  0,  'A', 'S',    // 18-1F
    'D',  'F',  'G', 'H', 'J', 'K', 'L', ':',    // 20-27
    '"',  '~',   0, '|', 'Z', 'X', 'C', 'V',     // 28-2F
    'B',  'N',  'M', '<', '>', '?',  0,  '*',    // 30-37
     0,  ' ',   0,   0,   0,   0,   0,   0,      // 38-3F
};

// PS/2 Set 1 extended keys (E0-prefixed)
#define SC_E0_UP    0x48
#define SC_E0_DOWN  0x50
#define SC_E0_LEFT  0x4B
#define SC_E0_RIGHT 0x4D
#define SC_E0_DEL   0x53
#define SC_E0_HOME  0x47
#define SC_E0_END   0x4F

// ============================================================
//  QTextEdit
// ============================================================
QTextEdit::QTextEdit(QWidget *parent, const char *name)
    : QWidget(parent, name)
    , m_cursorLine(0)
    , m_cursorCol(0)
    , m_scrollLine(0)
    , m_visibleLines(20)
{
    m_bgColor = COLOR_WHITE;
    m_doc = new QTextDocument();
    // Default text
    m_doc->setPlainText(
        "Welcome to zhwh_os Text Editor!\n"
        "\n"
        "This is a C++ text editor widget running\n"
        "on a custom operating system.\n"
        "\n"
        "Features:\n"
        "- Line numbers\n"
        "- Cursor navigation (arrows, home, end)\n"
        "- Typing with Shift support\n"
        "- Backspace / Delete\n"
        "- Scrolling\n"
        "\n"
        "Built on: QObject → QWidget → QTextEdit\n"
    );
}

QTextEdit::~QTextEdit() {
    delete m_doc;
}

void QTextEdit::setPlainText(const char *text) {
    m_doc->setPlainText(text);
    m_cursorLine = 0;
    m_cursorCol = 0;
    m_scrollLine = 0;
}

// ============================================================
//  Cursor movement
// ============================================================
void QTextEdit::moveCursorLeft() {
    if (m_cursorCol > 0) {
        m_cursorCol--;
    } else if (m_cursorLine > 0) {
        m_cursorLine--;
        m_cursorCol = m_doc->lineLength(m_cursorLine);
    }
}

void QTextEdit::moveCursorRight() {
    if (m_cursorCol < m_doc->lineLength(m_cursorLine)) {
        m_cursorCol++;
    } else if (m_cursorLine < m_doc->lineCount() - 1) {
        m_cursorLine++;
        m_cursorCol = 0;
    }
}

void QTextEdit::moveCursorUp() {
    if (m_cursorLine > 0) {
        m_cursorLine--;
        int maxCol = m_doc->lineLength(m_cursorLine);
        if (m_cursorCol > maxCol) m_cursorCol = maxCol;
    }
}

void QTextEdit::moveCursorDown() {
    if (m_cursorLine < m_doc->lineCount() - 1) {
        m_cursorLine++;
        int maxCol = m_doc->lineLength(m_cursorLine);
        if (m_cursorCol > maxCol) m_cursorCol = maxCol;
    }
}

void QTextEdit::moveCursorHome() {
    m_cursorCol = 0;
}

void QTextEdit::moveCursorEnd() {
    m_cursorCol = m_doc->lineLength(m_cursorLine);
}

void QTextEdit::setCursorScreenPos(int px, int py) {
    int relX = px - m_x;
    int relY = py - m_y;
    printf("[EDITOR] click at screen(%d,%d) → line=%d col=%d\n",
           px, py, m_scrollLine + ((relY - 2) / QPainter::charHeight()),
           (relX - (32 + 4)) / QPainter::charWidth());
    int textX = GUTTER_W + 4;
    int textY = 2;
    int col = (relX - textX) / QPainter::charWidth();
    int line = (relY - textY) / QPainter::charHeight();
    if (col < 0) col = 0;
    if (line < 0) line = 0;
    m_cursorLine = m_scrollLine + line;
    m_cursorCol = col;
    if (m_cursorLine >= m_doc->lineCount())
        m_cursorLine = m_doc->lineCount() - 1;
    if (m_cursorLine < 0) m_cursorLine = 0;
    if (m_cursorCol < 0) m_cursorCol = 0;
    if (m_cursorCol > m_doc->lineLength(m_cursorLine))
        m_cursorCol = m_doc->lineLength(m_cursorLine);
    printf("[EDITOR] cursor final: line=%d col=%d (doc has %d lines)\n",
           m_cursorLine, m_cursorCol, m_doc->lineCount());
}

void QTextEdit::ensureCursorVisible() {
    if (m_cursorLine < m_scrollLine) {
        m_scrollLine = m_cursorLine;
    }
    if (m_cursorLine >= m_scrollLine + m_visibleLines) {
        m_scrollLine = m_cursorLine - m_visibleLines + 1;
    }
    if (m_scrollLine < 0) m_scrollLine = 0;
}

// ============================================================
//  Scancode → ASCII (PS/2 Set 1)
// ============================================================
char QTextEdit::scancodeToAscii(int scancode, bool shift) {
    // 清除释放位 (bit 7)
    scancode &= 0x7F;
    if (scancode < 64) {
        return shift ? sc1_shifted[scancode] : sc1_normal[scancode];
    }
    return 0;
}

// ============================================================
//  Keyboard input
// ============================================================
bool QTextEdit::keyPress(int scancode, bool shift) {
    // First, check for special keys (E0 prefix is handled by caller)
    switch (scancode) {
        case SC_E0_LEFT:  moveCursorLeft();  ensureCursorVisible(); return true;
        case SC_E0_RIGHT: moveCursorRight(); ensureCursorVisible(); return true;
        case SC_E0_UP:    moveCursorUp();    ensureCursorVisible(); return true;
        case SC_E0_DOWN:  moveCursorDown();  ensureCursorVisible(); return true;
        case SC_E0_HOME:  moveCursorHome();  return true;
        case SC_E0_END:   moveCursorEnd();   return true;
        case SC_E0_DEL:
            m_doc->deleteForward(m_cursorLine, m_cursorCol);
            return true;
    }

    // Backspace (Set 1: 0x0E)
    if (scancode == 0x0E) {
        if (m_cursorCol > 0 || m_cursorLine > 0) {
            moveCursorLeft();
            m_doc->deleteForward(m_cursorLine, m_cursorCol);
        }
        return true;
    }

    // Enter (Set 1: 0x1C)
    if (scancode == 0x1C) {
        m_doc->insertNewline(m_cursorLine, m_cursorCol);
        m_cursorLine++;
        m_cursorCol = 0;
        ensureCursorVisible();
        return true;
    }

    // Tab (Set 1: 0x0F)
    if (scancode == 0x0F) {
        m_doc->insertChar(m_cursorLine, m_cursorCol, '\t');
        m_cursorCol++;
        return true;
    }

    // Printable characters
    char ch = scancodeToAscii(scancode, shift);
    if (ch >= 32 || ch == '\t') {
        m_doc->insertChar(m_cursorLine, m_cursorCol, ch);
        m_cursorCol++;
        return true;
    }

    return false;
}

// ============================================================
//  Rendering
// ============================================================
void QTextEdit::paintEvent(QPainter *painter) {
    int x = m_x, y = m_y, w = m_w, h = m_h;
    const int CHAR_H = QPainter::charHeight();
    const int CHAR_W = QPainter::charWidth();

    // Compute visible lines
    m_visibleLines = (h - 4) / CHAR_H;
    if (m_visibleLines < 1) m_visibleLines = 1;

    // Background
    painter->setColor(m_bgColor);
    painter->fillRect(x, y, w, h);

    // Border
    painter->setColor(COLOR_DARK_GRAY);
    painter->drawRect(x, y, w, h);

    // Gutter (line number area)
    painter->setColor(COLOR_GRAY);
    painter->fillRect(x + 1, y + 1, GUTTER_W - 1, h - 2);

    // Gutter border
    painter->setColor(COLOR_DARK_GRAY);
    painter->drawLine(x + GUTTER_W, y + 1, x + GUTTER_W, y + h - 2);

    // Text area
    int textX = x + GUTTER_W + 4;
    int textY = y + 2;

    int maxLines = m_doc->lineCount();
    for (int i = 0; i < m_visibleLines; i++) {
        int lineIdx = m_scrollLine + i;
        if (lineIdx >= maxLines) break;

        int ly = textY + i * CHAR_H;

        // Line number (right-aligned in gutter)
        painter->setColor(COLOR_DARK_GRAY);
        char numBuf[8];
        int n = lineIdx + 1;
        int di = 0;
        if (n >= 1000) numBuf[di++] = '0' + (n / 1000); n %= 1000;
        if (n >= 100 || di > 0) numBuf[di++] = '0' + (n / 100); n %= 100;
        if (n >= 10 || di > 0) numBuf[di++] = '0' + (n / 10); n %= 10;
        numBuf[di++] = '0' + n;
        numBuf[di] = '\0';
        // Right-align: count chars, then position
        int nw = di * CHAR_W;
        painter->drawText(x + GUTTER_W - 4 - nw, ly, numBuf);

        // Text content
        const char *txt = m_doc->lineText(lineIdx);
        int txtLen = m_doc->lineLength(lineIdx);

        // Truncate to fit
        int maxCols = (w - GUTTER_W - 8) / CHAR_W;
        if (txtLen > maxCols) txtLen = maxCols;

        painter->setColor(COLOR_BLACK);

        // Draw each character
        for (int ci = 0; ci < txtLen; ci++) {
            if (txt[ci] == '\t') {
                // Tab → 4 spaces
                // Skip (just draw spaces)
                painter->drawText(textX + ci * CHAR_W, ly, " ");
            } else {
                char cb[2] = { txt[ci], '\0' };
                painter->drawText(textX + ci * CHAR_W, ly, cb);
            }
        }
    }

    // Cursor
    if (m_cursorLine >= m_scrollLine &&
        m_cursorLine < m_scrollLine + m_visibleLines) {
        int curY = textY + (m_cursorLine - m_scrollLine) * CHAR_H;
        int curX = textX + m_cursorCol * CHAR_W;

        // Check if cursor is within text area
        if (curX < textX + (w - GUTTER_W - 8) && curY < textY + m_visibleLines * CHAR_H) {
            painter->setColor(COLOR_BLACK);
            painter->drawLine(curX, curY, curX, curY + CHAR_H - 1);
        }
    }

    // Scroll info (if needed)
    if (m_scrollLine > 0 || maxLines > m_visibleLines) {
        // Draw scrollbar thumb indicator
        if (maxLines > m_visibleLines) {
            int sbH = (m_visibleLines * (h - 4)) / maxLines;
            if (sbH < 8) sbH = 8;
            int sbY = y + 2 + (m_scrollLine * (h - 4 - sbH)) / (maxLines - m_visibleLines);
            painter->setColor(COLOR_GRAY);
            painter->fillRect(x + w - 8, sbY, 6, sbH);
        }
    }
}
