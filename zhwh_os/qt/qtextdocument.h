/**
 * @file qtextdocument.h
 * @brief QTextDocument — line-based plain-text document model
 *
 * Dynamic array of lines. Each line is a dynamically-allocated char buffer.
 * Supports: insert/delete characters, split/join lines.
 */

#ifndef QTEXTDOCUMENT_H
#define QTEXTDOCUMENT_H

class QTextDocument {
public:
    QTextDocument();
    ~QTextDocument();

    // --- Content access ---
    int  lineCount() const { return m_count; }
    int  lineLength(int line) const;
    const char *lineText(int line) const;
    char charAt(int line, int col) const;

    // --- Total character count ---
    int  totalLength() const;

    // --- Editing ---
    void insertChar(int line, int col, char ch);
    void deleteChar(int line, int col);        // backspace at (line,col-1)
    void deleteForward(int line, int col);      // delete at (line,col)
    void insertNewline(int line, int col);      // split line at cursor
    void deleteNewline(int line);               // join line with next

    // --- Bulk ---
    void setPlainText(const char *text);
    void clear();

private:
    struct Line {
        char *text;
        int   len;       // used length
        int   cap;       // allocated capacity
    };

    Line *m_lines;
    int   m_count;
    int   m_capacity;

    void ensureLineCapacity(int minLines);
    void ensureCharCapacity(int line, int needed);
    void shiftLinesDown(int from);
    void shiftLinesUp(int from);
};

#endif // QTEXTDOCUMENT_H
