/**
 * @file main.cpp
 * @brief C++ test program — verifies C++ works on zhwh_os
 *
 * Demonstrates: classes, inheritance, virtual functions, new/delete,
 * global constructors, and a basic Qt-like widget tree.
 *
 * Output goes to serial console (via printf → write syscall).
 */

// Pull in syscall wrappers and basic types (C-compatible)
extern "C" {
#include "libuser_minimal.h"
}

// ============================================================
//  A small C++ GUI toolkit demo (Qt-inspired naming)
// ============================================================

// Forward
class Widget;

// --- Geometry ---
struct Rect {
    int x, y, w, h;
};

struct Point {
    int x, y;
};

// --- Base Widget class (like QWidget) ---
class Widget {
protected:
    Rect    m_rect;
    Widget *m_parent;
    Widget *m_next;       // linked-list sibling
    char   *m_name;

public:
    Widget(Widget *parent = nullptr, const char *name = "");
    virtual ~Widget();

    virtual void paint();
    virtual const char *className() const { return "Widget"; }

    void setGeometry(int x, int y, int w, int h);
    Rect geometry() const { return m_rect; }
    const char *name() const { return m_name ? m_name : "(null)"; }

    // Simple linked-list for children (real Qt uses a tree, we keep it light)
    Widget *next() const { return m_next; }
    void setNext(Widget *w) { m_next = w; }
};

// --- Button class (like QPushButton) ---
class Button : public Widget {
private:
    char *m_text;
    bool  m_pressed;

public:
    Button(Widget *parent, const char *name, const char *text);
    ~Button() override;

    void paint() override;
    const char *className() const override { return "Button"; }

    void click();
    const char *text() const { return m_text; }
};

// --- Label class (like QLabel) ---
class Label : public Widget {
private:
    char *m_labelText;

public:
    Label(Widget *parent, const char *name, const char *text);
    ~Label() override;

    void paint() override;
    const char *className() const override { return "Label"; }

    void setText(const char *t);
    const char *labelText() const { return m_labelText; }
};

// --- Window class (like QMainWindow) ---
class Window : public Widget {
private:
    Widget *m_children;  // linked list head
    char   *m_title;

public:
    Window(const char *title);
    ~Window() override;

    void paint() override;
    const char *className() const override { return "Window"; }

    void addChild(Widget *child);
    const char *title() const { return m_title; }
};

// ============================================================
//  Global constructor test — this MUST print before main()
// ============================================================
class GlobalTest {
public:
    GlobalTest(const char *msg) {
        printf("[CXX-GLOBAL-CTOR] %s\n", msg);
    }
};

static GlobalTest g_globalTest("Global constructor test PASSED");

// ============================================================
//  Helper: poor-man's strdup since we have no libc
// ============================================================
static char *my_strdup(const char *s) {
    if (!s) return nullptr;
    unsigned int len = 0;
    while (s[len]) len++;
    char *d = new char[len + 1];
    for (unsigned int i = 0; i <= len; i++) d[i] = s[i];
    return d;
}

// ============================================================
//  Widget implementation
// ============================================================
Widget::Widget(Widget *parent, const char *name)
    : m_parent(parent), m_next(nullptr), m_name(nullptr)
{
    m_rect = {0, 0, 0, 0};
    m_name = my_strdup(name);
    if (m_parent && m_parent->className()[0] == 'W') {  // Window hack
        // parent is Window — we'll be added via addChild
    }
}

Widget::~Widget() {
    if (m_name) delete[] m_name;
}

void Widget::paint() {
    printf("  [%s @ %d,%d %dx%d] paint (base)\n",
           m_name, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
}

void Widget::setGeometry(int x, int y, int w, int h) {
    m_rect.x = x; m_rect.y = y;
    m_rect.w = w; m_rect.h = h;
}

// ============================================================
//  Button implementation
// ============================================================
Button::Button(Widget *parent, const char *name, const char *text)
    : Widget(parent, name), m_pressed(false)
{
    m_text = my_strdup(text);
    setGeometry(0, 0, 80, 28);  // default button size
}

Button::~Button() {
    if (m_text) delete[] m_text;
}

void Button::paint() {
    printf("  [%s btn \"%s\" @ %d,%d %dx%d] paint (button)\n",
           m_name, m_text, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
}

void Button::click() {
    m_pressed = !m_pressed;
    printf("  >>> Button \"%s\" clicked! state=%s\n",
           m_text, m_pressed ? "pressed" : "released");
}

// ============================================================
//  Label implementation
// ============================================================
Label::Label(Widget *parent, const char *name, const char *text)
    : Widget(parent, name)
{
    m_labelText = my_strdup(text);
    setGeometry(0, 0, 120, 20);
}

Label::~Label() {
    if (m_labelText) delete[] m_labelText;
}

void Label::paint() {
    printf("  [%s lbl \"%s\" @ %d,%d %dx%d] paint (label)\n",
           m_name, m_labelText, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
}

void Label::setText(const char *t) {
    if (m_labelText) delete[] m_labelText;
    m_labelText = my_strdup(t);
}

// ============================================================
//  Window implementation
// ============================================================
Window::Window(const char *title)
    : Widget(nullptr, title), m_children(nullptr)
{
    m_title = my_strdup(title);
    setGeometry(0, 0, 400, 300);
}

Window::~Window() {
    // Delete all children
    Widget *child = m_children;
    while (child) {
        Widget *next = child->next();
        delete child;
        child = next;
    }
    if (m_title) delete[] m_title;
}

void Window::paint() {
    printf("[Window \"%s\" @ %d,%d %dx%d] paint START\n",
           m_title, m_rect.x, m_rect.y, m_rect.w, m_rect.h);
    // Paint all children (polymorphic virtual dispatch)
    Widget *child = m_children;
    while (child) {
        child->paint();  // VIRTUAL CALL — the whole point of this test!
        child = child->next();
    }
    printf("[Window \"%s\"] paint END\n", m_title);
}

void Window::addChild(Widget *child) {
    // Insert at head of linked list
    child->setNext(m_children);
    m_children = child;
}

// ============================================================
//  Test: virtual function dispatch through base pointer
// ============================================================
static void test_virtual_dispatch() {
    printf("\n=== Test 1: Virtual Function Dispatch ===\n");

    Widget *w1 = new Button(nullptr, "btnOk",     "OK");
    Widget *w2 = new Label(nullptr,  "lblStatus", "Ready");
    Widget *w3 = new Button(nullptr, "btnCancel", "Cancel");

    Widget *widgets[3] = { w1, w2, w3 };

    for (int i = 0; i < 3; i++) {
        printf("Widget[%d] className=%s, name=%s\n",
               i, widgets[i]->className(), widgets[i]->name());
        widgets[i]->paint();  // virtual dispatch!
    }

    delete w1;
    delete w2;
    delete w3;
}

// ============================================================
//  Test: Window with child widgets
// ============================================================
static void test_window() {
    printf("\n=== Test 2: Window with Children ===\n");

    Window *win = new Window("MyApp v0.1");

    Button *btn = new Button(win, "btnHello", "Say Hello");
    btn->setGeometry(10, 10, 100, 30);
    win->addChild(btn);

    Label *lbl = new Label(win, "lblInfo", "Welcome to zhwh_os C++!");
    lbl->setGeometry(10, 50, 200, 20);
    win->addChild(lbl);

    Button *btn2 = new Button(win, "btnQuit", "Quit");
    btn2->setGeometry(10, 80, 100, 30);
    win->addChild(btn2);

    // Paint everything — each child's paint() is called virtually
    win->paint();

    // Test button click
    btn->click();

    delete win;  // recursively deletes all children
}

// ============================================================
//  Test: new/delete stress (allocate many objects)
// ============================================================
static void test_new_delete() {
    printf("\n=== Test 3: new/delete stress ===\n");

    // Allocate 20 labels
    Widget *objs[20];
    for (int i = 0; i < 20; i++) {
        char name[32];
        // Simple itoa
        int n = i;
        char *p = name + 30;
        *p = '\0';
        do { *--p = '0' + (n % 10); n /= 10; } while (n);
        // copy "lbl" prefix
        char tmp[32];
        tmp[0] = 'l'; tmp[1] = 'b'; tmp[2] = 'l';
        int j = 3;
        while (p && *p) tmp[j++] = *p++;
        tmp[j] = '\0';

        objs[i] = new Label(nullptr, tmp, "test");
    }

    printf("Allocated 20 Label objects\n");

    for (int i = 0; i < 20; i++) {
        printf("  obj[%d] = %s (class=%s)\n",
               i, objs[i]->name(), objs[i]->className());
    }

    for (int i = 0; i < 20; i++) {
        delete objs[i];
    }

    printf("Deleted all 20 objects\n");
}

// ============================================================
//  main
// ============================================================
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  zhwh_os C++ Runtime Test\n");
    printf("========================================\n");
    printf("(If you see Global constructor above,\n");
    printf(" init_array is working correctly)\n\n");

    test_virtual_dispatch();
    test_window();
    test_new_delete();

    printf("\n========================================\n");
    printf("  ALL C++ TESTS PASSED!\n");
    printf("========================================\n");

    return 0;
}
