/**
 * @file test_gui.cpp
 * @brief GUI test — draws widgets on the real framebuffer
 *
 * Creates a desktop-style window with:
 *  - Title bar with name
 *  - A label
 *  - Two buttons (OK / Cancel) with signal/slot
 */

extern "C" {
#include "libuser_minimal.h"
}
#include "qpainter.h"
#include "qwidget.h"

// ============================================================
//  TitleBar — simple window title bar widget
// ============================================================
class TitleBar : public QWidget {
    char *m_title;
public:
    TitleBar(QWidget *parent, const char *name, const char *title)
        : QWidget(parent, name), m_title(nullptr)
    {
        // Copy title
        if (title) {
            unsigned int len = 0;
            while (title[len]) len++;
            m_title = new char[len + 1];
            for (unsigned int i = 0; i <= len; i++) m_title[i] = title[i];
        }
        setGeometry(0, 0, parent ? parent->width() : 400, 24);
        m_bgColor = COLOR_DARK_BLUE;
    }

    ~TitleBar() { if (m_title) delete[] m_title; }

    void paintEvent(QPainter *painter) override {
        // Blue title bar background
        painter->setColor(m_bgColor);
        painter->fillRect(m_x, m_y, m_w, m_h);

        // Title text in white
        if (m_title) {
            painter->setColor(COLOR_WHITE);
            int tw = 0;
            const char *p = m_title;
            while (*p++) tw++;
            tw *= QPainter::charWidth();
            int tx = m_x + (m_w - tw) / 2;
            painter->drawText(tx, m_y + 8, m_title);
        }
    }

    const char *className() const override { return "TitleBar"; }
};

// ============================================================
//  DesktopWindow — a window with title bar, label, and buttons
// ============================================================
class DesktopWindow : public QWidget {
    TitleBar    *m_titleBar;
    QLabel      *m_statusLabel;
    QPushButton *m_btnOk;
    QPushButton *m_btnCancel;
    int          m_clickCount;

public:
    DesktopWindow(const char *title);
    ~DesktopWindow();

    void paintEvent(QPainter *painter) override;
    const char *className() const override { return "DesktopWindow"; }

    void onOkClicked();
    void onCancelClicked();
};

// Slot handlers (free functions)
static void slotOkClicked(QObject *sender, void *data) {
    DesktopWindow *win = (DesktopWindow *)data;
    (void)sender;
    if (win) win->onOkClicked();
}

static void slotCancelClicked(QObject *sender, void *data) {
    DesktopWindow *win = (DesktopWindow *)data;
    (void)sender;
    if (win) win->onCancelClicked();
}

DesktopWindow::DesktopWindow(const char *title)
    : QWidget(nullptr, title), m_clickCount(0)
{
    setGeometry(200, 150, 400, 300);
    m_bgColor = COLOR_LIGHT_GRAY;

    // Create child widgets
    m_titleBar = new TitleBar(this, "titlebar", title);

    m_statusLabel = new QLabel(this, "status", "Welcome to zhwh_os GUI!");
    m_statusLabel->setGeometry(12, 34, 376, 20);

    m_btnOk = new QPushButton(this, "btnOk", "OK");
    m_btnOk->setGeometry(200, 250, 80, 28);

    m_btnCancel = new QPushButton(this, "btnCancel", "Cancel");
    m_btnCancel->setGeometry(296, 250, 80, 28);

    // Connect button clicks → window methods
    QObject::connect(m_btnOk,     "clicked", this, slotOkClicked,     this);
    QObject::connect(m_btnCancel, "clicked", this, slotCancelClicked, this);

    printf("[GUI] Window '%s' created, children=%d, connections=%d\n",
           title, childCount(), connectionCount());
}

DesktopWindow::~DesktopWindow() {
    // Children auto-deleted by QObject
}

void DesktopWindow::paintEvent(QPainter *painter) {
    // Window background
    painter->setColor(m_bgColor);
    painter->fillRect(m_x, m_y, m_w, m_h);

    // Window border
    painter->setColor(COLOR_DARK_GRAY);
    painter->drawRect(m_x, m_y, m_w, m_h);
}

void DesktopWindow::onOkClicked() {
    m_clickCount++;
    printf("[GUI] OK clicked! count=%d\n", m_clickCount);

    char buf[40];
    // Simple itoa
    int n = m_clickCount;
    char tmp[16];
    int i = 0;
    do { tmp[i++] = '0' + (n % 10); n /= 10; } while (n > 0);
    // Build string
    char *p = buf;
    const char *prefix = "OK clicked: ";
    while (*prefix) *p++ = *prefix++;
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';

    m_statusLabel->setText(buf);
}

void DesktopWindow::onCancelClicked() {
    printf("[GUI] Cancel clicked!\n");
    if (m_clickCount > 0) {
        m_clickCount--;
    }
    char buf[32];
    char *p = buf;
    const char *msg = "Cancelled. count=";
    while (*msg) *p++ = *msg++;
    int n = m_clickCount;
    char tmp[8];
    int i = 0;
    do { tmp[i++] = '0' + (n % 10); n /= 10; } while (n > 0);
    while (i > 0) *p++ = tmp[--i];
    *p = '\0';
    m_statusLabel->setText(buf);
}

// ============================================================
//  main
// ============================================================
int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  GUI Test — Framebuffer Rendering\n");
    printf("========================================\n\n");

    // 1. Get framebuffer info
    fb_info_t fb;
    int ret = gui_get_fb_info(&fb);
    if (ret != 0) {
        printf("[GUI] ERROR: Failed to get framebuffer info! ret=%d\n", ret);
        return -1;
    }

    printf("[GUI] Framebuffer: %dx%d, pitch=%d, bpp=%d\n",
           fb.width, fb.height, fb.pitch, fb.bpp);

    // 2. Create painter on framebuffer
    QPainter painter((uint32_t *)fb.fb_addr, fb.width, fb.height, fb.pitch);

    // 3. Clear screen to dark gray
    painter.clear(COLOR_DARK_GRAY);
    printf("[GUI] Screen cleared\n");

    // 4. Create desktop window
    DesktopWindow *win = new DesktopWindow("zhwh_os Desktop Demo");

    // 5. Render everything
    win->render(&painter);
    printf("[GUI] Window rendered\n");

    printf("\n========================================\n");
    printf("  GUI is now visible on screen!\n");
    printf("  Window at (200,150) 400x300\n");
    printf("  Title bar, label, OK + Cancel buttons\n");
    printf("========================================\n");

    // Stay alive — the screen shows our work
    while (1) {
        __asm__ volatile("hlt");
    }

    return 0;
}
