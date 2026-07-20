/**
 * @file qterminal.cpp
 * @brief Virtual terminal — command-line interface in a desktop window
 *
 * Supports: help, lspci, net, usb, mem, clear, echo, fb
 */
#include "qterminal.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"

extern "C" {
#include "libuser_minimal.h"
int lspci(void);
int net_ifconfig(void);
int net_ping(const char *ip);
int rtl8139_init_user(void);
int e1000_init_user(const char *dev);
int net_set_device(const char *name);
int net_ifup(const char *dev);
int net_arp(const char *dev, int scan);
int net_dump_regs(const char *dev);
}

#define TERM_BUF_MAX 32768

// PS/2 Set1 scancode → ASCII (US QWERTY, no shift handling here — shift passed in)
static char sc2a(int sc, bool sh) {
    static const char t1[] = {
        0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
        '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
        0,'a','s','d','f','g','h','j','k','l',';','\'','`',
        0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '
    };
    static const char t2[] = {
        0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
        '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
        0,'A','S','D','F','G','H','J','K','L',':','"','~',
        0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '
    };
    if (sc >= (int)sizeof(t1)) return 0;
    return sh ? t2[sc] : t1[sc];
}

// Hex helpers
static char hex(uint8_t n) { return "0123456789ABCDEF"[n & 0xF]; }

QTerminal::QTerminal(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    m_bgColor = 0x00101018;
    m_bufMax = TERM_BUF_MAX;
    m_buf = new char[m_bufMax];
    m_buf[0] = 0; m_bufLen = 0;
    m_cmdLen = 0; m_cmdBuf[0] = 0;
    m_scrollOffset = 0;
    appendOutput("HillsonOS Terminal v1.0\nType 'help' for commands.\n");
    appendPrompt();
}

QTerminal::~QTerminal() { delete[] m_buf; }

void QTerminal::appendOutput(const char *s) {
    while (*s && m_bufLen < m_bufMax - 2) m_buf[m_bufLen++] = *s++;
    m_buf[m_bufLen] = 0;
}

void QTerminal::appendPrompt() {
    appendOutput("\n> ");
}

void QTerminal::scrollUp() { if (m_scrollOffset > 0) m_scrollOffset--; }
void QTerminal::scrollDown() { m_scrollOffset++; }

void QTerminal::executeCommand(const char *cmd) {
    // Echo command
    appendOutput(cmd);
    appendOutput("\n");
    runBuiltin(cmd);
    m_cmdLen = 0; m_cmdBuf[0] = 0;
    appendPrompt();
}

void QTerminal::runBuiltin(const char *cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    if (cmd[0]=='h' && cmd[1]=='e' && cmd[2]=='l' && cmd[3]=='p') {
        appendOutput("Commands:\n");
        appendOutput("  help lspci net usb mem fb clear echo\n");
        appendOutput("  net.init rtl  - init RTL8139 NIC\n");
        appendOutput("  net.init e1k  - init E1000 NIC\n");
        appendOutput("  net.up        - bring interface up\n");
        appendOutput("  net.arp       - ARP table\n");
        appendOutput("  net.regs      - dump NIC registers\n");
        appendOutput("  ping <ip>     - ping IP address\n");
        return;
    }
    if (cmd[0]=='c' && cmd[1]=='l' && cmd[2]=='e' && cmd[3]=='a' && cmd[4]=='r') {
        m_bufLen = 0; m_buf[0] = 0;
        return;
    }
    if (cmd[0]=='e' && cmd[1]=='c' && cmd[2]=='h' && cmd[3]=='o') {
        const char *a = cmd + 4;
        while (*a == ' ') a++;
        appendOutput(a); appendOutput("\n");
        return;
    }
    if (cmd[0]=='l' && cmd[1]=='s' && cmd[2]=='p' && cmd[3]=='c') {
        appendOutput("--- PCI Devices ---\n");
        lspci();
        return;
    }
    if (cmd[0]=='n' && cmd[1]=='e' && cmd[2]=='t') {
        appendOutput("--- Network ---\n");
        net_ifconfig();
        return;
    }
    if (cmd[0]=='u' && cmd[1]=='s' && cmd[2]=='b') {
        int info = 0;
        __asm__ volatile("int $0x80":"=a"(info):"a"(76),"b"(0):"memory");
        appendOutput("USB Mouse: EP=0x"); char hx[3]; hx[0]=hex(info>>4);hx[1]=hex(info);hx[2]=0; appendOutput(hx);
        appendOutput(" MaxPkt="); char ds[4]; int v=(info>>8)&0xFF; ds[0]='0'+v/100;ds[1]='0'+(v/10)%10;ds[2]='0'+v%10;ds[3]=0; appendOutput(ds);
        appendOutput(" Interval="); v=(info>>16)&0xFF; ds[0]='0'+v/100;ds[1]='0'+(v/10)%10;ds[2]='0'+v%10; appendOutput(ds);
        appendOutput("\n");
        return;
    }
    if (cmd[0]=='m' && cmd[1]=='e' && cmd[2]=='m') {
        appendOutput("--- Memory Stats ---\n");
        __asm__ volatile("int $0x80"::"a"(4):"memory");
        return;
    }
    if (cmd[0]=='f' && cmd[1]=='b') {
        fb_info_t fb;
        if (gui_get_fb_info(&fb)==0) {
            appendOutput("Framebuffer: "); char t[32];
            int p=0;t[p++]='0'+fb.width/1000;t[p++]='0'+(fb.width/100)%10;t[p++]='0'+(fb.width/10)%10;t[p++]='0'+fb.width%10;
            t[p++]='x'; int h=fb.height; t[p++]='0'+h/1000;t[p++]='0'+(h/100)%10;t[p++]='0'+(h/10)%10;t[p++]='0'+h%10;
            t[p++]=' '; t[p++]='0'+fb.bpp/10; t[p++]='0'+fb.bpp%10; t[p++]='b';t[p++]='p';t[p++]='p';t[p]=0;
            appendOutput(t); appendOutput("\n");
        }
        return;
    }
    if (cmd[0]=='p' && cmd[1]=='i' && cmd[2]=='n' && cmd[3]=='g') {
        const char *a = cmd + 4;
        while (*a == ' ') a++;
        appendOutput("Pinging "); appendOutput(a); appendOutput("...\n");
        net_ping(a);
        return;
    }
    if (cmd[0]=='n' && cmd[1]=='e' && cmd[2]=='t' && cmd[3]=='.') {
        if (cmd[4]=='i' && cmd[5]=='n' && cmd[6]=='i' && cmd[7]=='t') {
            const char *a = cmd + 9;
            while (*a == ' ') a++;
            if (a[0]=='r' && a[1]=='t' && a[2]=='l') {
                appendOutput("Init RTL8139...\n"); rtl8139_init_user();
            } else if (a[0]=='e' && a[1]=='1') {
                appendOutput("Init E1000...\n"); e1000_init_user("eth0");
            } else appendOutput("Usage: net.init rtl|e1k\n");
            return;
        }
        if (cmd[4]=='u' && cmd[5]=='p') { appendOutput("Bringing interface up...\n"); net_ifup("eth0"); return; }
        if (cmd[4]=='a' && cmd[5]=='r' && cmd[6]=='p') { appendOutput("ARP table:\n"); net_arp("eth0", 0); return; }
        if (cmd[4]=='r' && cmd[5]=='e' && cmd[6]=='g') { appendOutput("NIC registers:\n"); net_dump_regs("eth0"); return; }
        appendOutput("Unknown net command\n");
        return;
    }
    appendOutput("Unknown: "); appendOutput(cmd); appendOutput("\n");
}

bool QTerminal::keyPress(int sc, bool sh) {
    // Skip release codes
    if (sc & 0x80) return false;
    int raw = sc & 0x7F;
    if (sc & 0xE000) raw = sc & 0xFF;

    // Enter
    if (raw == 0x1C || raw == 0x5A) {
        m_cmdBuf[m_cmdLen] = 0;
        if (m_cmdLen > 0) executeCommand(m_cmdBuf);
        else { appendOutput("\n"); appendPrompt(); }
        return true;
    }
    // Backspace
    if (raw == 0x0E || raw == 0x66) {
        if (m_cmdLen > 0) { m_cmdLen--; m_bufLen--; m_buf[m_bufLen]=0; }
        return true;
    }
    // Printable characters
    char ch = sc2a(raw, sh);
    if (ch >= 32 && ch <= 126) {
        if (m_cmdLen < 250) {
            m_cmdBuf[m_cmdLen++] = ch;
            // appendOutput expects a null-terminated string — wrap ch in a proper buffer
            char tmp[2] = {ch, 0};
            appendOutput(tmp);
        }
        return true;
    }
    return false;
}

void QTerminal::paintEvent(QPainter *painter) {
    int x = m_x, y = m_y;
    painter->setColor(m_bgColor);
    painter->fillRect(x, y, m_w, m_h);
    if (!m_buf || m_bufLen == 0) return;

    painter->setColor(0x0000FF00);
    int maxLines = (m_h - 4) / 10;
    int ly = y + 4, col = 0, drawn = 0;
    char *p = m_buf;
    // Skip scrolled lines
    int skip = m_scrollOffset;
    while (skip > 0 && *p) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        skip--;
    }
    while (*p && drawn < maxLines && ly < m_h - 8) {
        if (*p == '\n') { ly += 10; col = 0; drawn++; p++; continue; }
        if (*p == '\r') { p++; continue; }
        if (col < 120) { char t[2]={*p,0}; painter->drawText(x+4+col*8, ly, t); }
        col++; p++;
    }
}

// Desktop integration
extern "C" void term_keyPress(void *w, int sc, bool sh) { ((QTerminal*)w)->keyPress(sc, sh); }

QDesktopWindow *createTerminalApp(QDesktop *desktop) {
    int ww = 700, wh = 420;
    QTerminal *term = new QTerminal(nullptr, "terminal");
    term->setGeometry(0, 0,
        ww - 2*QDesktopWindow::BORDER_W,
        wh - QDesktopWindow::TITLE_H - 3*QDesktopWindow::BORDER_W);
    return desktop->addWindow("Terminal", term, ww, wh);
}
