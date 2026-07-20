/**
 * @file qsysinfo.cpp
 * @brief System devices & drivers info display
 */
#include "qsysinfo.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"

extern "C" {
#include "libuser_minimal.h"
int lspci(void);
int net_ifconfig(void);
}

#define SYS_TEXT_MAX 16384

QSysInfo::QSysInfo(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    m_textMax = SYS_TEXT_MAX;
    m_text = new char[m_textMax];
    m_text[0] = 0;
    m_textLen = 0;
    m_scrollOffset = 0;
    m_bgColor = 0x00101020;  // dark blue-black
    refresh();
}

QSysInfo::~QSysInfo() {
    delete[] m_text;
}

void QSysInfo::appendText(const char *s) {
    while (*s && m_textLen < m_textMax - 1)
        m_text[m_textLen++] = *s++;
    m_text[m_textLen] = 0;
}

void QSysInfo::appendHex(uint32_t v) {
    char buf[12]; int i = 0;
    buf[i++] = '0'; buf[i++] = 'x';
    if (!v) buf[i++] = '0';
    else for (int n = 28; n >= 0; n -= 4) {
        int d = (v >> n) & 0xF;
        if (d || i > 2) buf[i++] = "0123456789ABCDEF"[d];
    }
    buf[i] = 0;
    appendText(buf);
}

void QSysInfo::appendDec(int v) {
    char buf[12]; int i = 0;
    if (v < 0) { appendText("-"); v = -v; }
    if (!v) buf[i++] = '0';
    else while (v) { buf[i++] = '0' + v % 10; v /= 10; }
    while (i) m_text[m_textLen++] = buf[--i];
}

void QSysInfo::appendNL() { if (m_textLen < m_textMax - 1) m_text[m_textLen++] = '\n'; m_text[m_textLen] = 0; }

void QSysInfo::appendSection(const char *title) {
    appendNL();
    appendText("===== "); appendText(title); appendText(" ====="); appendNL();
}

void QSysInfo::captureKlog(const char *filter) {
    char kbuf[8192];
    kbuf[0] = 0;
    __asm__ volatile("int $0x80"::"a"(77),"b"(kbuf),"c"(0):"memory");

    char *ls = kbuf;
    int copied = 0;
    while (*ls && copied < 2000 && m_textLen < m_textMax - 100) {
        char *le = ls;
        while (*le && *le != '\n') le++;
        int len = le - ls;
        bool match = !filter;
        if (filter) {
            for (int i = 0; i < len && !match; i++) {
                bool m = true;
                for (int j = 0; filter[j] && m; j++)
                    if (i + j >= len || (ls[i + j] != filter[j] &&
                        !(ls[i+j] >= 'a' && ls[i+j] <= 'z' && ls[i+j] - 'a' + 'A' == filter[j]) &&
                        !(ls[i+j] >= 'A' && ls[i+j] <= 'Z' && ls[i+j] - 'A' + 'a' == filter[j])))
                        m = false;
                if (m) match = true;
            }
        }
        if (match && len > 0) {
            for (int i = 0; i < len && m_textLen < m_textMax - 1; i++)
                m_text[m_textLen++] = ls[i];
            if (m_textLen < m_textMax - 1) m_text[m_textLen++] = '\n';
            copied += len;
        }
        ls = (*le) ? le + 1 : le;
    }
    if (copied == 0) { appendText("  (no data)"); appendNL(); }
    m_text[m_textLen] = 0;
}

void QSysInfo::captureSyscallOutput(int (*fn)(void), const char *filter) {
    // Call the function (prints to klog), then read klog with filter
    if (fn) fn();
    captureKlog(filter);
}

void QSysInfo::refresh() {
    m_textLen = 0;
    m_text[0] = 0;
    m_scrollOffset = 0;

    // ---- Framebuffer Info ----
    appendSection("Display / Framebuffer");
    fb_info_t fb;
    if (gui_get_fb_info(&fb) == 0) {
        appendText("Resolution: "); appendDec(fb.width); appendText(" x "); appendDec(fb.height); appendNL();
        appendText("BPP: "); appendDec(fb.bpp); appendNL();
        appendText("Pitch: "); appendDec(fb.pitch); appendText(" bytes"); appendNL();
        appendText("FB Address: "); appendHex((uint32_t)fb.fb_addr); appendNL();
    } else {
        appendText("  Not available"); appendNL();
    }

    // ---- USB Info ----
    appendSection("USB Devices");
    int info = 0;
    __asm__ volatile("int $0x80":"=a"(info):"a"(76),"b"(0):"memory");
    if (info) {
        appendText("Mouse EP: "); appendHex(info & 0xFF); appendNL();
        appendText("MaxPkt: "); appendDec((info >> 8) & 0xFF); appendNL();
        appendText("Interval: "); appendDec((info >> 16) & 0xFF); appendNL();
        appendText("SetProto: "); appendDec((info >> 24) & 0xFF); appendNL();
        // DMA bytes
        int dma1 = 0, dma2 = 0;
        __asm__ volatile("int $0x80":"=a"(dma1):"a"(76),"b"(1):"memory");
        __asm__ volatile("int $0x80":"=a"(dma2):"a"(76),"b"(2):"memory");
        appendText("DMA[0..3]: "); appendHex(dma1); appendNL();
        appendText("DMA[4..7]: "); appendHex(dma2); appendNL();
    } else {
        appendText("  No USB mouse detected"); appendNL();
    }
    // EHCI debug log (USB lines)
    char elog[4096]; elog[0] = 0;
    __asm__ volatile("int $0x80"::"a"(77),"b"(elog),"c"(1):"memory");
    if (elog[0]) {
        appendText("--- EHCI Log ---"); appendNL();
        char *p = elog; int n = 0;
        while (*p && n < 1500 && m_textLen < m_textMax - 2) {
            m_text[m_textLen++] = *p++;
            n++;
        }
        appendNL();
    }

    // ---- PCI Devices ----
    appendSection("PCI Devices");
    captureSyscallOutput(lspci, nullptr);

    // ---- Network ----
    appendSection("Network Interfaces");
    captureSyscallOutput(net_ifconfig, nullptr);

    // ---- Memory ----
    appendSection("Memory");
    // Call SYS_GET_MEM_STATS (syscall 4) — prints to klog
    __asm__ volatile("int $0x80"::"a"(4):"memory");
    captureKlog("MEM");  // filter for memory-related lines

    // ---- Keyboard ----
    appendSection("Input Devices");
    appendText("Keyboard: PS/2 (IRQ1)"); appendNL();
    appendText("Mouse: USB HID"); appendNL();

    m_text[m_textLen] = 0;
}

void QSysInfo::scrollUp() { if (m_scrollOffset > 0) m_scrollOffset--; }
void QSysInfo::scrollDown() { m_scrollOffset++; }

void QSysInfo::paintEvent(QPainter *painter) {
    painter->setColor(m_bgColor);
    painter->fillRect(0, 0, m_w, m_h);

    if (!m_text || m_textLen == 0) {
        painter->setColor(COLOR_GRAY);
        painter->drawText(8, 8, "No system info. Press F5 to refresh.");
        return;
    }

    int maxLines = (m_h - 4) / 10;
    int ly = 4, col = 0, drawn = 0;
    char *p = m_text;
    int skip = m_scrollOffset;
    while (skip > 0 && *p) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        skip--;
    }
    while (*p && drawn < maxLines && ly < m_h - 8) {
        if (*p == '\n') { ly += 10; col = 0; drawn++; p++; continue; }
        if (*p == '\r') { p++; continue; }
        // Color sections differently
        if (*p == '=' && col == 0) painter->setColor(0x00FFFF00);
        else if (*p == '-' && col == 0) painter->setColor(0x0080C0FF);
        else painter->setColor(0x0000FF00);
        if (col < 120) { char t[2] = {*p, 0}; painter->drawText(4 + col*8, ly, t); }
        col++; p++;
    }
}

QDesktopWindow *createSysInfoApp(QDesktop *desktop) {
    int ww = 720, wh = 480;
    QSysInfo *info = new QSysInfo(nullptr, "sysinfo");
    info->setGeometry(0, 0,
        ww - 2*QDesktopWindow::BORDER_W,
        wh - QDesktopWindow::TITLE_H - 3*QDesktopWindow::BORDER_W);

    return desktop->addWindow("System Info", info, ww, wh);
}

// Helper for desktop.cpp F5 refresh
extern "C" void sysinfo_refresh(void *w) { ((QSysInfo*)w)->refresh(); }
