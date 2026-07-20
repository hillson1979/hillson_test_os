/**
 * @file qtexteditorapp.h
 * @brief Text editor application wrapped in a desktop window
 */
#ifndef QTEXTEDITORAPP_H
#define QTEXTEDITORAPP_H

class QDesktopWindow;
class QDesktop;

// Create a text editor window in the given desktop
QDesktopWindow *createTextEditorApp(QDesktop *desktop);

// Helper: keyPress for QTextEdit (called from qdesktop.cpp)
bool qtextedit_keyPress(void *editor, int scancode, bool shift);
bool qtextedit_qtKeyPress(void *editor, int qtKey, int uni, bool shift);

#endif // QTEXTEDITORAPP_H
