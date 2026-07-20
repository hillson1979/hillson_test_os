/**
 * @file qusbmonitor.cpp
 * @brief USB device monitor widget implementation
 */
#include "qusbmonitor.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"

extern "C" {
#include "libuser_minimal.h"
}

#define USB_TEXT_MAX 8192

QUsbMonitor::QUsbMonitor(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    m_text = new char[USB_TEXT_MAX];
    m_text[0] = 0;
    m_textLen = 0;
    m_scrollOffset = 0;
    m_bgColor = COLOR_BLACK;
    refresh();
}

QUsbMonitor::~QUsbMonitor() {
    delete[] m_text;
}

void QUsbMonitor::refresh() {
    m_text[0] = 0;
    m_textLen = 0;

    // Read EHCI debug buffer via syscall 77 ecx=1
    char logbuf[4096];
    logbuf[0] = 0;
    __asm__ volatile("int $0x80"::"a"(77),"b"(logbuf),"c"(1):"memory");

    // Read klog (syscall 77 ecx=0)
    char kbuf[4096];
    kbuf[0] = 0;
    __asm__ volatile("int $0x80"::"a"(77),"b"(kbuf),"c"(0):"memory");

    // Read USB mouse info via syscall 76
    int info = 0;
    __asm__ volatile("int $0x80":"=a"(info):"a"(76),"b"(0):"memory");

    uint8_t ep = info & 0xFF;
    uint8_t maxpkt = (info >> 8) & 0xFF;
    uint8_t interval = (info >> 16) & 0xFF;
    int setproto = (info >> 24) & 0xFF;

    // Read DMA bytes
    int dma1 = 0, dma2 = 0, efl = 0, eqh = 0;
    __asm__ volatile("int $0x80":"=a"(dma1):"a"(76),"b"(1):"memory");
    __asm__ volatile("int $0x80":"=a"(dma2):"a"(76),"b"(2):"memory");
    __asm__ volatile("int $0x80":"=a"(efl):"a"(76),"b"(3):"memory");
    __asm__ volatile("int $0x80":"=a"(eqh):"a"(76),"b"(4):"memory");

    // Format output
    int p = 0;
    auto w = [&](const char *s) {
        while (*s && p < USB_TEXT_MAX - 1) m_text[p++] = *s++;
    };
    auto wh = [&](uint32_t v) {
        for (int n = 28; n >= 0; n -= 4)
            m_text[p++] = "0123456789ABCDEF"[(v >> n) & 0xF];
    };
    auto wd = [&](int v) {
        if (v < 0) { w("-"); v = -v; }
        char tmp[12]; int i = 0;
        if (v == 0) tmp[i++] = '0';
        else while (v) { tmp[i++] = '0' + v % 10; v /= 10; }
        while (i) m_text[p++] = tmp[--i];
    };
    auto nl = [&]() { m_text[p++] = '\n'; };

    w("===== USB Monitor =====\n\n");
    w("--- Mouse Endpoint Info ---\n");
    w("EP addr: 0x"); wh(ep); nl();
    w("MaxPkt: "); wd(maxpkt); nl();
    w("Interval: "); wd(interval); nl();
    w("SetProto result: "); wd(setproto); nl();
    w("DMA[0..3]: 0x"); wh(dma1); nl();
    w("DMA[4..7]: 0x"); wh(dma2); nl();
    if (efl || eqh) {
        w("EHCI FL: 0x"); wh(efl);
        w("  QH: 0x"); wh(eqh); nl();
    }
    nl();

    w("--- EHCI Debug Log ---\n");
    // Copy first ~2000 chars of EHCI log
    int copied = 0;
    char *src = logbuf;
    while (*src && copied < 2000 && p < USB_TEXT_MAX - 2) {
        m_text[p++] = *src++;
        copied++;
    }
    if (copied >= 2000) w("\n... (truncated)");
    if (copied == 0) w("(no EHCI debug data)\n");
    nl(); nl();

    w("--- Kernel Log (USB lines) ---\n");
    // Filter kernel log for USB-related lines
    char *ls = kbuf;
    int kl_copied = 0;
    while (*ls && kl_copied < 1000 && p < USB_TEXT_MAX - 2) {
        char *le = ls;
        while (*le && *le != '\n') le++;
        int len = le - ls;
        int hasUsb = 0;
        for (int i = 0; i < len - 1; i++) {
            if ((ls[i] == 'U' && ls[i+1] == 'S') ||  // USB
                (ls[i] == 'E' && ls[i+1] == 'H') ||   // EHCI
                (ls[i] == 'u' && ls[i+1] == 's')) {    // usb
                hasUsb = 1; break;
            }
        }
        if (hasUsb) {
            for (int i = 0; i <= len && p < USB_TEXT_MAX - 1; i++)
                m_text[p++] = ls[i];
            kl_copied += len;
        }
        ls = (*le) ? le + 1 : le;
    }
    if (kl_copied == 0) w("(no USB lines in kernel log)\n");

    m_text[p] = 0;
    m_textLen = p;
    m_scrollOffset = 0;
}

void QUsbMonitor::scrollUp() {
    if (m_scrollOffset > 0) m_scrollOffset--;
}

void QUsbMonitor::scrollDown() {
    m_scrollOffset++;
}

void QUsbMonitor::paintEvent(QPainter *painter) {
    painter->setColor(COLOR_BLACK);
    painter->fillRect(0, 0, m_w, m_h);

    if (!m_text || m_textLen == 0) {
        painter->setColor(COLOR_GRAY);
        painter->drawText(8, 8, "No USB data. Press Refresh.");
        return;
    }

    painter->setColor(0x0000FF00);  // green text on black
    int ly = 4;
    int col = 0;
    int maxLines = (m_h - 4) / 10;

    // Skip scrolled lines
    int skipLines = m_scrollOffset;
    char *p = m_text;
    while (skipLines > 0 && *p) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        skipLines--;
    }

    int drawnLines = 0;
    while (*p && drawnLines < maxLines && ly < m_h - 8) {
        if (*p == '\n') {
            ly += 10;
            col = 0;
            drawnLines++;
            p++;
            continue;
        }
        if (*p == '\r') { p++; continue; }
        // Draw character
        if (col < 120) {
            char tmp[2] = {*p, 0};
            painter->drawText(4 + col * 8, ly, tmp);
        }
        col++;
        p++;
    }
}

QDesktopWindow *createUsbMonitorApp(QDesktop *desktop) {
    int ww = 700, wh = 460;
    QUsbMonitor *mon = new QUsbMonitor(nullptr, "usbmon");
    mon->setGeometry(0, 0, ww - 2*QDesktopWindow::BORDER_W,
                     wh - QDesktopWindow::TITLE_H - 3*QDesktopWindow::BORDER_W);

    QDesktopWindow *win = desktop->addWindow("USB Monitor", mon, ww, wh);
    return win;
}

extern "C" void usbmon_refresh(void *w) { ((QUsbMonitor*)w)->refresh(); }
