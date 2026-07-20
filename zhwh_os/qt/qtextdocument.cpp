/**
 * @file qtextdocument.cpp
 * @brief QTextDocument implementation
 */

#include "qtextdocument.h"

extern "C" {
#include "libuser_minimal.h"
}

// ============================================================
//  Helpers
// ============================================================
static char *strdup_simple(const char *s, int len) {
    if (!s || len <= 0) return nullptr;
    char *d = new char[len + 1];
    for (int i = 0; i < len; i++) d[i] = s[i];
    d[len] = '\0';
    return d;
}

// ============================================================
//  Constructor / Destructor
// ============================================================
QTextDocument::QTextDocument()
    : m_lines(nullptr), m_count(0), m_capacity(0)
{
    // Start with one empty line
    ensureLineCapacity(8);
    m_count = 1;
    m_lines[0].text = nullptr;
    m_lines[0].len  = 0;
    m_lines[0].cap  = 0;
    ensureCharCapacity(0, 0);
}

QTextDocument::~QTextDocument() {
    for (int i = 0; i < m_count; i++) {
        if (m_lines[i].text) delete[] m_lines[i].text;
    }
    if (m_lines) delete[] m_lines;
}

// ============================================================
//  Access
// ============================================================
int QTextDocument::lineLength(int line) const {
    if (line < 0 || line >= m_count) return 0;
    return m_lines[line].len;
}

const char *QTextDocument::lineText(int line) const {
    if (line < 0 || line >= m_count) return "";
    if (!m_lines[line].text) return "";
    return m_lines[line].text;
}

char QTextDocument::charAt(int line, int col) const {
    if (line < 0 || line >= m_count) return '\0';
    if (col < 0 || col >= m_lines[line].len) return '\0';
    return m_lines[line].text[col];
}

int QTextDocument::totalLength() const {
    int total = 0;
    for (int i = 0; i < m_count; i++) total += m_lines[i].len;
    return total;
}

// ============================================================
//  Capacity management
// ============================================================
void QTextDocument::ensureLineCapacity(int minLines) {
    if (m_capacity >= minLines) return;
    int newCap = m_capacity ? m_capacity * 2 : 8;
    if (newCap < minLines) newCap = minLines;

    Line *newLines = new Line[newCap];
    for (int i = 0; i < m_count; i++) {
        newLines[i] = m_lines[i];
    }
    for (int i = m_count; i < newCap; i++) {
        newLines[i].text = nullptr;
        newLines[i].len  = 0;
        newLines[i].cap  = 0;
    }
    if (m_lines) delete[] m_lines;
    m_lines = newLines;
    m_capacity = newCap;
}

void QTextDocument::ensureCharCapacity(int line, int needed) {
    if (line < 0 || line >= m_count) return;
    Line &l = m_lines[line];
    if (l.cap > needed) return;  // already enough

    int newCap = l.cap ? l.cap * 2 : 64;
    if (newCap <= needed) newCap = needed + 64;

    char *newText = new char[newCap + 1];  // +1 for null terminator
    for (int i = 0; i < l.len; i++) newText[i] = l.text[i];
    newText[l.len] = '\0';

    if (l.text) delete[] l.text;
    l.text = newText;
    l.cap = newCap;
}

// ============================================================
//  Line shifting
// ============================================================
void QTextDocument::shiftLinesDown(int from) {
    ensureLineCapacity(m_count + 1);
    for (int i = m_count; i > from; i--) {
        m_lines[i] = m_lines[i - 1];
    }
    m_lines[from].text = nullptr;
    m_lines[from].len  = 0;
    m_lines[from].cap  = 0;
    m_count++;
}

void QTextDocument::shiftLinesUp(int from) {
    if (from < 0 || from >= m_count - 1) return;
    if (m_lines[from].text) delete[] m_lines[from].text;
    for (int i = from; i < m_count - 1; i++) {
        m_lines[i] = m_lines[i + 1];
    }
    m_count--;
}

// ============================================================
//  Editing
// ============================================================
void QTextDocument::insertChar(int line, int col, char ch) {
    if (line < 0 || line >= m_count) return;
    if (col < 0 || col > m_lines[line].len) col = m_lines[line].len;
    if (ch == '\n' || ch == '\r') {
        insertNewline(line, col);
        return;
    }

    ensureCharCapacity(line, m_lines[line].len + 1);
    Line &l = m_lines[line];

    // Shift right
    for (int i = l.len; i > col; i--) {
        l.text[i] = l.text[i - 1];
    }
    l.text[col] = ch;
    l.len++;
    l.text[l.len] = '\0';
}

void QTextDocument::deleteChar(int line, int col) {
    // Backspace: delete character at (line, col-1)
    if (col > 0) {
        deleteForward(line, col - 1);
    } else if (line > 0) {
        // Join with previous line
        int prevLen = m_lines[line - 1].len;
        int curLen  = m_lines[line].len;
        ensureCharCapacity(line - 1, prevLen + curLen);
        Line &prev = m_lines[line - 1];
        Line &cur  = m_lines[line];
        for (int i = 0; i < curLen; i++) {
            prev.text[prevLen + i] = cur.text[i];
        }
        prev.len += curLen;
        prev.text[prev.len] = '\0';
        shiftLinesUp(line);
    }
}

void QTextDocument::deleteForward(int line, int col) {
    if (line < 0 || line >= m_count) return;
    if (col < 0 || col >= m_lines[line].len) {
        // Delete newline: join with next line
        if (line < m_count - 1) {
            deleteNewline(line);
        }
        return;
    }
    // Shift left
    Line &l = m_lines[line];
    for (int i = col; i < l.len - 1; i++) {
        l.text[i] = l.text[i + 1];
    }
    l.len--;
    l.text[l.len] = '\0';
}

void QTextDocument::insertNewline(int line, int col) {
    if (line < 0 || line >= m_count) return;
    if (col < 0 || col > m_lines[line].len) col = m_lines[line].len;

    shiftLinesDown(line + 1);

    // Split current line at col
    Line &cur  = m_lines[line];
    Line &next = m_lines[line + 1];

    int rightLen = cur.len - col;
    if (rightLen > 0) {
        ensureCharCapacity(line + 1, rightLen);
        for (int i = 0; i < rightLen; i++) {
            next.text[i] = cur.text[col + i];
        }
        next.len = rightLen;
        next.text[next.len] = '\0';
    }
    cur.len = col;
    cur.text[cur.len] = '\0';
}

void QTextDocument::deleteNewline(int line) {
    if (line < 0 || line >= m_count - 1) return;

    Line &cur  = m_lines[line];
    Line &next = m_lines[line + 1];

    ensureCharCapacity(line, cur.len + next.len);
    for (int i = 0; i < next.len; i++) {
        cur.text[cur.len + i] = next.text[i];
    }
    cur.len += next.len;
    cur.text[cur.len] = '\0';

    shiftLinesUp(line + 1);
}

// ============================================================
//  Bulk operations
// ============================================================
void QTextDocument::setPlainText(const char *text) {
    clear();
    if (!text) return;

    int lineIdx = 0;
    int start = 0;
    for (int i = 0; ; i++) {
        if (text[i] == '\n' || text[i] == '\r' || text[i] == '\0') {
            int len = i - start;
            if (len > 0 || text[i] == '\n' || text[i] == '\r') {
                ensureCharCapacity(lineIdx, len);
                m_lines[lineIdx].len = len;
                for (int j = 0; j < len; j++) {
                    m_lines[lineIdx].text[j] = text[start + j];
                }
                m_lines[lineIdx].text[len] = '\0';
                lineIdx++;
                if (lineIdx >= m_count) {
                    ensureLineCapacity(lineIdx + 1);
                    m_count = lineIdx + 1;
                }
            }
            if (text[i] == '\r' && text[i + 1] == '\n') i++;
            start = i + 1;
            if (text[i] == '\0') break;
        }
    }

    // Remove extra lines
    while (m_count > lineIdx) {
        m_count--;
        if (m_lines[m_count].text) {
            delete[] m_lines[m_count].text;
            m_lines[m_count].text = nullptr;
        }
    }

    if (m_count == 0) {
        m_count = 1;
        ensureCharCapacity(0, 0);
        m_lines[0].len = 0;
        if (m_lines[0].text) m_lines[0].text[0] = '\0';
    }
}

void QTextDocument::clear() {
    for (int i = 0; i < m_count; i++) {
        if (m_lines[i].text) {
            delete[] m_lines[i].text;
            m_lines[i].text = nullptr;
            m_lines[i].len  = 0;
            m_lines[i].cap  = 0;
        }
    }
    m_count = 1;
    ensureCharCapacity(0, 0);
    m_lines[0].len = 0;
}
