/**
 * @file qterminal.cpp
 * @brief Virtual terminal �?command-line interface in a desktop window
 *
 * Supports: help, lspci, net, usb, mem, clear, echo, fb
 */
#include "qterminal.h"
#include "qdesktop.h"
#include "qdesktopwindow.h"
#include "qpainter.h"
#include "../qt/include/qnamespace_qt.h"

extern "C" {
#include "libuser_minimal.h"
int lsdisk(const char *path, char *buf, int max);
int lspci(void);
int net_ifconfig(void);
int net_ping(const char *ip);
int rtl8139_init_user(void);
int e1000_init_user(const char *dev);
int net_set_device(const char *name);
int net_ifup(const char *dev);
int net_arp(const char *dev, int scan);
int net_dump_regs(const char *dev);
int net_send_udp(const char *ip, int port, const char *data, int len);
int execv(const char *path, char *const argv[]);
int spawn(const char *path, const char *arg);
void yield(void);
int open(const char *path, int flags);
int close(int fd);
int read(int fd, char *buf, int len);
void exit(int code);
}

#define TERM_BUF_MAX 131072

// PS/2 Set1 scancode �?ASCII (US QWERTY, no shift handling here �?shift passed in)
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

static void decstr(int v, char *out) {
    char tmp[16];
    int p = 0;
    int o = 0;
    if (v < 0) {
        out[o++] = '-';
        v = -v;
    }
    do {
        tmp[p++] = (char)('0' + (v % 10));
        v /= 10;
    } while (v && p < (int)sizeof(tmp));
    while (p > 0)
        out[o++] = tmp[--p];
    out[o] = 0;
}

static int termLineCount(const char *s) {
    int lines = 1;
    if (!s || !*s)
        return 0;
    while (*s) {
        if (*s == '\n')
            lines++;
        s++;
    }
    return lines;
}

QTerminal::QTerminal(QWidget *parent, const char *name)
    : QWidget(parent, name)
{
    m_bgColor = 0x00101018;
    m_bufMax = TERM_BUF_MAX;
    m_buf = new char[m_bufMax];
    m_buf[0] = 0; m_bufLen = 0;
    m_cmdLen = 0; m_cmdBuf[0] = 0;
    m_scrollOffset = 0;
    m_histCount = 0; m_histIdx = -1;
    for (int i = 0; i < 20; i++) m_history[i][0] = 0;
    appendOutput("HillsonOS Terminal v1.0\nType 'help' for commands.\n");
    appendPrompt();
}

QTerminal::~QTerminal() { delete[] m_buf; }

void QTerminal::appendOutput(const char *s) {
    while (*s) {
        if (m_bufLen >= m_bufMax - 2) {
            /* 缓冲满了: 删除�?1024 字节到下一换行 */
            int cut = 4096;
            while (cut < m_bufLen && m_buf[cut] != '\n') cut++;
            if (cut >= m_bufLen) cut = m_bufLen / 2;
            int remain = m_bufLen - cut;
            for (int i = 0; i < remain; i++) m_buf[i] = m_buf[cut + i];
            m_bufLen = remain;
        }
        m_buf[m_bufLen++] = *s++;
    }
    m_buf[m_bufLen] = 0;

    int maxLines = (m_h > 12) ? ((m_h - 4) / 10) : 1;
    int totalLines = termLineCount(m_buf);
    m_scrollOffset = totalLines > maxLines ? (totalLines - maxLines) : 0;
}

void QTerminal::appendPrompt() {
    appendOutput("\n> ");
}

void QTerminal::scrollUp() { if (m_scrollOffset > 0) m_scrollOffset--; }
void QTerminal::scrollDown() {
    int maxLines = (m_h > 12) ? ((m_h - 4) / 10) : 1;
    int totalLines = termLineCount(m_buf);
    int maxOffset = totalLines > maxLines ? (totalLines - maxLines) : 0;
    if (m_scrollOffset < maxOffset) m_scrollOffset++;
}

void QTerminal::executeCommand(const char *cmd) {
    // Command is already echoed character-by-character in keyPressQt,
    // so we just add a newline (no re-echo of cmd).
    appendOutput("\n");
    runBuiltin(cmd);
    m_cmdLen = 0; m_cmdBuf[0] = 0;
    appendPrompt();
}

void QTerminal::runBuiltin(const char *cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    if (cmd[0]=='l' && cmd[1]=='s' && (cmd[2]==0 || cmd[2]==' ')) {
        const char *a = cmd + 2;
        while (*a == ' ') a++;
        if (!*a) a = "/";
        appendOutput("--- "); appendOutput(a); appendOutput(" ---\n");
        char lbuf[4096];
        int n = lsdisk(a, lbuf, sizeof(lbuf));
        lbuf[sizeof(lbuf) - 1] = 0;
        appendOutput(lbuf);
        if (n < 0 && lbuf[0] == 0)
            appendOutput("ls: failed\n");
        else if (n == 0 && lbuf[0] == 0)
            appendOutput("(empty)\n");
        return;
    }
    if (cmd[0]=='h' && cmd[1]=='e' && cmd[2]=='l' && cmd[3]=='p') {
        appendOutput("Commands:\n");
        appendOutput("  help ls lspci net usb mem fb clear echo exec java log cat\n");
        appendOutput("  log            - show kernel log\n");
        appendOutput("  log usb        - show recent USB driver log\n");
        appendOutput("  log mouse      - show USB mouse/HID state and reports\n");
        appendOutput("  net init rtl  - init RTL8139 NIC\n");
        appendOutput("  net init e1k  - init E1000 NIC\n");
        appendOutput("  usb init      - show USB init state (kernel-managed)\n");
        appendOutput("  usb log       - show USB driver log\n");
        appendOutput("  usb log display on|off - control USB log desktop output\n");
        appendOutput("  usb log save  - save console log to /usb/USB_AI.LOG\n");
        appendOutput("  usb status    - show asynchronous USB state\n");
        appendOutput("  net.if  - ifconfig\n");
        appendOutput("  net.up        - bring interface up\n");
        appendOutput("  net.arp       - ARP table\n");
        appendOutput("  net.regs      - dump NIC registers\n");
        appendOutput("  USB logs auto-send via UDP broadcast :9999 (netdebug)\n");
        appendOutput("  ping <ip>     - ping IP address\n");
        appendOutput("  exec <path>   - run ELF program\n");
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
    if (cmd[0]=='n' && cmd[1]=='e' && cmd[2]=='t' && cmd[3]=='.'&& cmd[4]=='i'&& cmd[5]=='f') {
        appendOutput("--- Network ifconfig---\n");
        net_ifconfig();
        return;
    }
    if (cmd[0]=='n' && cmd[1]=='e' && cmd[2]=='t' && cmd[3]=='l' && cmd[4]=='o' && cmd[5]=='g') {
        const char *a = cmd + 6; while(*a==' ') a++;
        if(a[0]=='o' && a[1]=='f' && a[2]=='f') {
            appendOutput("netlog off\n");
        } else if(*a) {
            char ip[32], kw[64]; int i=0, port=9999; kw[0]=0;
            while(a[i] && a[i]!=' ' && i<30) { ip[i]=a[i]; i++; } ip[i]=0;
            if(a[i]==' ') { i++; port=0; while(a[i]>='0'&&a[i]<='9'){port=port*10+(a[i]-'0');i++;} }
            if(a[i]==' ') { i++; int j=0; while(a[i]&&j<62) kw[j++]=a[i++]; kw[j]=0; }

            /* 1. sys82: read USB log, filter via edx */
            static char kbuf[4096]; int len = 0;
            __asm__ volatile("int $0x80"
                : "=a"(len)
                : "a"(82), "b"(kbuf), "c"(4096), "d"(kw)
                : "memory");

            /* 2. 用和net.send完全一样的方式发�?*/
            if (len > 0) {
                extern int net_send_udp(const char *ip, int port, const char *data, int len);
                int sent = 0;
                while (sent < len) {
                    int chunk = (len - sent > 1400) ? 1400 : (len - sent);
                    net_send_udp(ip, port, kbuf + sent, chunk);
                    sent += chunk;
                }
            }

            char tmp[64]; int p=0; const char *s="netlog -> "; while(*s)tmp[p++]=*s++;
            s=ip; while(*s)tmp[p++]=*s++; tmp[p++]=':';
            int pn=port; char pd[6]; int pi=0; do{pd[pi++]='0'+pn%10;pn/=10;}while(pn);
            while(pi)tmp[p++]=pd[--pi]; tmp[p++]='(';
            pn=len; pi=0; do{pd[pi++]='0'+pn%10;pn/=10;}while(pn);
            while(pi)tmp[p++]=pd[--pi]; tmp[p++]='b'; tmp[p++]=')';
            tmp[p++]='\n'; tmp[p]=0;
            appendOutput(tmp);
        } else {
            appendOutput("usage: netlog <ip> [port]\n");
        }
        return;
    }
    if (cmd[0]=='u' && cmd[1]=='s' && cmd[2]=='b' && cmd[3]==' ') {
        const char *a = cmd + 4; while (*a == ' ') a++;
        if (a[0]=='i' && a[1]=='n' && a[2]=='i' && a[3]=='t' && a[4]==0) {
            appendOutput("USB init is kernel-managed; use 'usb status' to follow progress.\n");
            int ret;
            __asm__ volatile("int $0x80":"=a"(ret):"a"(83):"memory");
            if (ret < 0) appendOutput("USB status query failed.\n");
            return;
        }
        if (a[0]=='l' && a[1]=='o' && a[2]=='g') {
            const char *p = a + 3; while (*p == ' ') p++;
            if (p[0]=='d' && p[1]=='i' && p[2]=='s' && p[3]=='p' &&
                p[4]=='l' && p[5]=='a' && p[6]=='y' && p[7]==' ') {
                const char *mode = p + 8;
                int enabled;
                if (mode[0]=='o' && mode[1]=='n' && mode[2]==0) {
                    __asm__ volatile("int $0x80" : "=a"(enabled) :
                                     "a"(86), "b"(1) : "memory");
                    appendOutput("USB log desktop display: ON\n");
                } else if (mode[0]=='o' && mode[1]=='f' && mode[2]=='f' && mode[3]==0) {
                    __asm__ volatile("int $0x80" : "=a"(enabled) :
                                     "a"(86), "b"(0) : "memory");
                    appendOutput("USB log desktop display: OFF\n");
                } else {
                    appendOutput("Usage: usb log display on|off\n");
                }
                return;
            }
            if (p[0]=='s' && p[1]=='a' && p[2]=='v' && p[3]=='e' && p[4]==0) {
                int ret;
                __asm__ volatile("int $0x80":"=a"(ret):"a"(84):"memory");
                appendOutput(ret == 0 ? "Saved to /usb/USB_AI.LOG\n" :
                                        "Save failed: USB disk/file is not ready.\n");
            } else {
                static char ubuf[8192]; int len;
                __asm__ volatile("int $0x80":"=a"(len):
                                 "a"(82),"b"(ubuf),"c"((int)sizeof(ubuf)),"d"(0):"memory");
                if (len > 0) { ubuf[len] = 0; appendOutput(ubuf); }
                else appendOutput("No USB log available.\n");
            }
            return;
        }
        if (a[0]=='s' && a[1]=='t' && a[2]=='a' && a[3]=='t' &&
            a[4]=='u' && a[5]=='s' && a[6]==0) {
            int state;
            __asm__ volatile("int $0x80":"=a"(state):"a"(85):"memory");
            if (state == 0) appendOutput("USB: off\n");
            else if (state == 1) appendOutput("USB: scanning PCI\n");
            else if (state == 2) appendOutput("USB: xHCI found, asynchronous start pending\n");
            else if (state == 3) appendOutput("USB: controller ready, scanning ports\n");
            else if (state == 4) appendOutput("USB: port scan complete\n");
            else if (state == 5) appendOutput("USB: controller starting in background\n");
            else if (state == 6) appendOutput("USB: controller reset complete, ring setup pending\n");
            else appendOutput("USB: initialization failed\n");
            return;
        }
        appendOutput("Usage: usb init | usb status | usb log | usb log save\n");
        return;
    }
    if (cmd[0]=='u' && cmd[1]=='s' && cmd[2]=='b' && cmd[3]==0) {
        int info = 0;
        __asm__ volatile("int $0x80":"=a"(info):"a"(76),"b"(0):"memory");
        appendOutput("USB Mouse: EP=0x"); char hx[3]; hx[0]=hex(info>>4);hx[1]=hex(info);hx[2]=0; appendOutput(hx);
        appendOutput(" MaxPkt="); char ds[4]; int v=(info>>8)&0xFF; ds[0]='0'+v/100;ds[1]='0'+(v/10)%10;ds[2]='0'+v%10;ds[3]=0; appendOutput(ds);
        appendOutput(" Interval="); v=(info>>16)&0xFF; ds[0]='0'+v/100;ds[1]='0'+(v/10)%10;ds[2]='0'+v%10; appendOutput(ds);
        appendOutput("\n");
        return;
    }
    if (cmd[0]=='k' && cmd[1]=='l' && cmd[2]=='o' && cmd[3]=='g' && cmd[4]=='.') {
        appendOutput("Flushing kernel log via UDP...\n");
        __asm__ volatile("int $0x80"::"a"(81):"memory");
        appendOutput("Done.\n");
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
            const char *a = cmd + 8;
            while (*a == ' ') a++;
            printf("net.init===%s\n", a);
            if (a[0]=='r' && a[1]=='t' && a[2]=='l') {
                appendOutput("Init RTL8139...\n"); rtl8139_init_user();
            } else if (a[0]=='e' && a[1]=='1' && a[2]=='k') {
                appendOutput("Init E1000...\n"); e1000_init_user("eth0");
            } else appendOutput("Usage: net.init rtl|e1k\n");
            return;
        }
        if (cmd[4]=='u' && cmd[5]=='p') { appendOutput("Bringing interface up...\n"); net_ifup("eth0"); return; }
        if (cmd[4]=='a' && cmd[5]=='r' && cmd[6]=='p') { appendOutput("ARP table:\n"); net_arp("eth0", 0); return; }
        if (cmd[4]=='r' && cmd[5]=='e' && cmd[6]=='g') { appendOutput("NIC registers:\n"); net_dump_regs("eth0"); return; }
        if (cmd[4]=='s' && cmd[5]=='e' && cmd[6]=='n' && cmd[7]=='d') {
            /* net.send <ip> <port> <message> */
            const char *a = cmd + 8; while(*a==' ') a++;
            char ip[32]; int i=0, port=0;
            while(a[i] && a[i]!=' ' && i<30) { ip[i]=a[i]; i++; } ip[i]=0;
            while(a[i]==' ') i++;
            while(a[i]>='0' && a[i]<='9') { port=port*10+(a[i]-'0'); i++; }
            while(a[i]==' ') i++;
            const char *msg = a + i;
            if(ip[0] && port>0 && msg[0]) {
                int len=0; while(msg[len]) len++;
                int r = net_send_udp(ip, port, msg, len);
                char buf[80]; int p=0; const char *s="UDP "; while(*s)buf[p++]=*s++;
                if(r>=0){ s="OK -> "; while(*s)buf[p++]=*s++; }
                else { s="FAIL -> "; while(*s)buf[p++]=*s++; }
                s=ip; while(*s)buf[p++]=*s++; buf[p++]=':';
                int pn=port; char pd[6]; int pi=0; do{pd[pi++]='0'+pn%10;pn/=10;}while(pn);
                while(pi)buf[p++]=pd[--pi]; buf[p++]=' '; buf[p++]='(';
                char rv[4]; rv[0]='0'+len; rv[1]='b'; rv[2]=')'; rv[3]=0;
                s=rv; while(*s)buf[p++]=*s++; buf[p++]='\n'; buf[p]=0;
                appendOutput(buf);
            } else {
                appendOutput("usage: net.send <ip> <port> <message>\n");
            }
            return;
        }
        appendOutput("Unknown net command\n");
        return;
    }
    if (cmd[0]=='a' && cmd[1]=='i' && (cmd[2]==0 || cmd[2]==' ')) {
        const char *a = cmd + 2; while (*a == ' ') a++;
        if (!*a) {
            appendOutput("usage: ai <prompt>\n");
            return;
        }
        static char ai_arg[128];
        int ai_len = 0;
        while (a[ai_len] && ai_len < 127) {
            ai_arg[ai_len] = a[ai_len];
            ai_len++;
        }
        ai_arg[ai_len] = 0;
        appendOutput("ai: "); appendOutput(ai_arg); appendOutput("\n");
        int pid = spawn("/boot/ai.elf", ai_arg);
        if (pid < 0)
            pid = spawn("/ai.elf", ai_arg);
        if (pid > 0) {
            char pidbuf[16];
            decstr(pid, pidbuf);
            appendOutput("ai started pid=");
            appendOutput(pidbuf);
            appendOutput(" (see serial output)\n");
        } else {
            appendOutput("ai spawn failed: /boot/ai.elf and /ai.elf not found or not executable\n");
        }
        return;
    }
    if (cmd[0]=='n' && cmd[1]=='e' && cmd[2]=='t' && cmd[3]==' ') {
        const char *a = cmd + 4; while (*a == ' ') a++;
        if (a[0]=='i' && a[1]=='n' && a[2]=='i' && a[3]=='t' && a[4]==' ') {
            a += 5; while (*a == ' ') a++;
            if (a[0]=='r' && a[1]=='t' && a[2]=='l' && a[3]==0) {
                appendOutput("Initializing RTL8139...\n");
                int ret = rtl8139_init_user();
                appendOutput(ret >= 0 ? "RTL8139 initialization completed.\n" :
                                        "RTL8139 initialization failed.\n");
            } else if (a[0]=='e' && a[1]=='1' && a[2]=='k' && a[3]==0) {
                appendOutput("Initializing E1000...\n");
                int ret = e1000_init_user("eth0");
                appendOutput(ret >= 0 ? "E1000 initialization completed.\n" :
                                        "E1000 initialization failed or unsupported.\n");
            } else appendOutput("Usage: net init rtl | net init e1k\n");
            return;
        }
        appendOutput("Usage: net init rtl | net init e1k\n");
        return;
    }
    if (cmd[0]=='j' && cmd[1]=='a' && cmd[2]=='v' && cmd[3]=='a') {
        const char *a = cmd + 4;
        while (*a == ' ') a++;
        const char *cn = *a ? a : "HelloWorld";
        appendOutput("java "); appendOutput(cn); appendOutput("\n");
        int pid = spawn("/jvm.elf", cn);
        if (pid > 0) {
            yield();
            /* �?console.log, 匹配 [Interp] ldc 提取字符�?*/
            int fd = open("/console.log", 0);
            if (fd != 0) {
                char buf[4096]; int total = 0, n;
                while (total < 4000 && (n = read(fd, buf + total, 4000 - total)) > 0) total += n;
                close(fd);
                buf[total] = 0;
                char *p = buf;
                while (*p) {
                    /* 手动匹配 "[Interp] ldc \"" */
                    char *ldc = 0;
                    for (char *s = p; *s; s++) {
                        if (s[0]=='[' && s[1]=='I' && s[2]=='n' && s[3]=='t' && s[4]=='e' &&
                            s[5]=='r' && s[6]=='p' && s[7]==']' && s[8]==' ' && s[9]=='l' &&
                            s[10]=='d' && s[11]=='c' && s[12]==' ') { ldc = s; break; }
                    }
                    if (!ldc) break;
                    ldc += 13;
                    if (*ldc == '"') ldc++;
                    char *e = ldc;
                    while (*e && *e != '"' && *e != '\n') e++;
                    if (e > ldc) { char s = *e; *e = 0; appendOutput(ldc); appendOutput("\n"); *e = s; }
                    p = e + 1;
                }
            }
        } else {
            appendOutput("spawn failed\n");
        }
        return;
    }
    if (cmd[0]=='e' && cmd[1]=='x' && cmd[2]=='e' && cmd[3]=='c') {
        const char *a = cmd + 4;
        while (*a == ' ') a++;
        if (!*a) { appendOutput("Usage: exec <path>\n"); return; }
        execv(a, (char *const *)0);
        appendOutput("exec failed\n");
        return;
    }
    if (cmd[0]=='l' && cmd[1]=='o' && cmd[2]=='g') {
        const char *a = cmd + 3; while (*a == ' ') a++;
        if (a[0]=='m' && a[1]=='o' && a[2]=='u' && a[3]=='s' &&
            a[4]=='e' && a[5]==0) {
            static char ubuf[8192];
            int len;
            __asm__ volatile("int $0x80":"=a"(len):
                             "a"(82), "b"(ubuf), "c"((int)sizeof(ubuf)),
                             "d"(0):"memory");
            appendOutput("--- USB mouse variables/log ---\n");
            int info;
            __asm__ volatile("int $0x80":"=a"(info):
                             "a"(76), "b"(0):"memory");
            appendOutput("endpoint=0x");
            char hx[3]; hx[0]=hex((uint8_t)(info >> 4));
            hx[1]=hex((uint8_t)info); hx[2]=0; appendOutput(hx);
            appendOutput(" maxpkt=");
            char ds[16]; decstr((info >> 8) & 0xFF, ds); appendOutput(ds);
            appendOutput(" interval="); decstr((info >> 16) & 0xFF, ds);
            appendOutput(ds); appendOutput(" setproto=");
            decstr((info >> 24) & 0xFF, ds); appendOutput(ds);
            appendOutput("\n");
            int dma_lo, dma_hi;
            __asm__ volatile("int $0x80":"=a"(dma_lo):
                             "a"(76), "b"(1):"memory");
            __asm__ volatile("int $0x80":"=a"(dma_hi):
                             "a"(76), "b"(2):"memory");
            appendOutput("last-dma=");
            for (int b = 0; b < 4; b++) {
                char byte_hex[3];
                uint8_t value = (uint8_t)((dma_lo >> (b * 8)) & 0xFF);
                byte_hex[0] = hex(value >> 4); byte_hex[1] = hex(value); byte_hex[2] = 0;
                appendOutput(byte_hex); appendOutput(b == 3 ? " " : ":");
            }
            for (int b = 0; b < 4; b++) {
                char byte_hex[3];
                uint8_t value = (uint8_t)((dma_hi >> (b * 8)) & 0xFF);
                byte_hex[0] = hex(value >> 4); byte_hex[1] = hex(value); byte_hex[2] = 0;
                appendOutput(byte_hex); appendOutput(b == 3 ? "\n" : ":");
            }
            if (len > 0) {
            const char *patterns[7] = {
                "mouse", "HID", "hid", "usb_mouse", "ep=3", "dci=3",
                "event high water"
            };
                int line_start = 0;
                for (int i = 0; i <= len; i++) {
                    if (i < len && ubuf[i] != '\n') continue;
                    int match = 0;
                    for (int p = 0; p < 7 && !match; p++) {
                        int plen = 0;
                        while (patterns[p][plen]) plen++;
                        for (int j = line_start; j + plen <= i; j++) {
                            int same = 1;
                            for (int k = 0; k < plen; k++) {
                                if (ubuf[j + k] != patterns[p][k]) {
                                    same = 0;
                                    break;
                                }
                            }
                            if (same) { match = 1; break; }
                        }
                    }
                    if (match) {
                        char saved = ubuf[i];
                        ubuf[i] = 0;
                        appendOutput(ubuf + line_start);
                        appendOutput("\n");
                        ubuf[i] = saved;
                    }
                    line_start = i + 1;
                }
            } else appendOutput("No USB mouse log available.\n");
            return;
        }
        if (a[0]=='u' && a[1]=='s' && a[2]=='b' && a[3]==0) {
            static char ubuf[8192];
            int len;
            __asm__ volatile("int $0x80":"=a"(len):
                             "a"(82), "b"(ubuf), "c"((int)sizeof(ubuf)),
                             "d"(0):"memory");
            if (len > 0) appendOutput(ubuf);
            else appendOutput("No USB log available.\n");
            return;
        }
        const char *log_path = "/kern.log";
        int fd = open(log_path, 0);
        if (fd == 0) { appendOutput("Cannot open\n"); return; }
        /* 只读最�?1024 字节 */
        char lbuf[1024]; int total = 0, n;
        while ((n = read(fd, lbuf + total, 1000 - total)) > 0) total += n;
        close(fd);
        if (total > 1024/2) {
            /* 从中间截�? 跳过前半 */
            int start = total - 800; if (start < 0) start = 0;
            char *p = lbuf + start;
            *(lbuf + total) = 0;
            appendOutput("...(truncated)\n");
            appendOutput(p);
        } else {
            lbuf[total] = 0;
            appendOutput(lbuf);
        }
        return;
    }
    if (cmd[0]=='c' && cmd[1]=='a' && cmd[2]=='t') {
        const char *a = cmd + 3;
        while (*a == ' ') a++;
        if (!*a) { appendOutput("Usage: cat <path>\n"); return; }
        int fd = open(a, 0);
        if (fd < 0) { appendOutput("Cannot open: "); appendOutput(a); appendOutput("\n"); return; }
        char cbuf[256];
        int n;
        while ((n = read(fd, cbuf, 255)) > 0) {
            cbuf[n] = 0;
            appendOutput(cbuf);
        }
        close(fd);
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
    // Up arrow (PS/2: E0 48)
    if (raw == 0x48 && (sc & 0xE000)) {
        return keyPressQt(Qt::Key_Up, 0, false);
    }
    // Down arrow (PS/2: E0 50)
    if (raw == 0x50 && (sc & 0xE000)) {
        return keyPressQt(Qt::Key_Down, 0, false);
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
            // appendOutput expects a null-terminated string �?wrap ch in a proper buffer
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

    painter->setColor(0x0000FF00);
    int maxLines = (m_h - 4) / 10;
    int ly = y + 4, col = 0, drawn = 0;
    char *p = m_buf ? m_buf : (char *)"";
    // Skip scrolled lines
    int skip = m_scrollOffset;
    while (skip > 0 && *p) {
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
        skip--;
    }
    while (*p && drawn < maxLines && ly < y + m_h - 8) {
        if (*p == '\n') { ly += 10; col = 0; drawn++; p++; continue; }
        if (*p == '\r') { p++; continue; }
        if (col < 120) { char t[2]={*p,0}; painter->drawText(x+4+col*8, ly, t); }
        col++; p++;
    }

    /* Keep a static insertion caret visible at the input position. */
    int totalLines = termLineCount(m_buf);
    int maxOffset = totalLines > maxLines ? totalLines - maxLines : 0;
    if (m_scrollOffset == maxOffset && ly < y + m_h - 2) {
        int caretCol = col < 120 ? col : 119;
        painter->fillRect(x + 4 + caretCol * 8, ly, 2, 9);
    }
}
// Desktop integration
extern "C" void term_keyPress(void *w, int sc, bool sh) { ((QTerminal*)w)->keyPress(sc, sh); }
extern "C" void term_qtKeyPress(void *w, int qtKey, int uni, bool sh) { ((QTerminal*)w)->keyPressQt(qtKey, uni, sh); }

bool QTerminal::keyPressQt(int qtKey, int uni, bool /*sh*/) {
    // Enter
    if (qtKey == Qt::Key_Return || qtKey == Qt::Key_Enter) {
        m_cmdBuf[m_cmdLen] = 0;
        if (m_cmdLen > 0) {
            /* Save to history */
            int slot = m_histCount % 20;
            int j = 0;
            while (m_cmdBuf[j] && j < 250) { m_history[slot][j] = m_cmdBuf[j]; j++; }
            m_history[slot][j] = 0;
            m_histCount++;
            m_histIdx = -1;
            executeCommand(m_cmdBuf);
        } else { appendOutput("\n"); appendPrompt(); }
        return true;
    }
    // Up arrow �?history
    if (qtKey == Qt::Key_Up) {
        if (m_histCount == 0) return true;
        if (m_histIdx < 0) m_histIdx = m_histCount - 1;
        else if (m_histIdx > 0) m_histIdx--;
        else m_histIdx = m_histCount - 1;  /* wrap */
        /* Clear current line and show history */
        while (m_cmdLen > 0) { m_cmdLen--; m_bufLen--; m_buf[m_bufLen]=0; }
        int slot = m_histIdx % 20;
        const char *h = m_history[slot];
        while (*h && m_cmdLen < 250) { m_cmdBuf[m_cmdLen++] = *h; char t[2]={*h,0}; appendOutput(t); h++; }
        return true;
    }
    // Down arrow �?newer history
    if (qtKey == Qt::Key_Down) {
        if (m_histCount == 0 || m_histIdx < 0) return true;
        m_histIdx++;
        if (m_histIdx >= m_histCount) { m_histIdx = -1; }
        /* Clear and show */
        while (m_cmdLen > 0) { m_cmdLen--; m_bufLen--; m_buf[m_bufLen]=0; }
        if (m_histIdx >= 0) {
            int slot = m_histIdx % 20;
            const char *h = m_history[slot];
            while (*h && m_cmdLen < 250) { m_cmdBuf[m_cmdLen++] = *h; char t[2]={*h,0}; appendOutput(t); h++; }
        }
        return true;
    }
    // Backspace
    if (qtKey == Qt::Key_Backspace) {
        if (m_cmdLen > 0) { m_cmdLen--; m_bufLen--; m_buf[m_bufLen]=0; }
        return true;
    }
    // Printable characters
    if (uni >= 32 && uni <= 126) {
        if (m_cmdLen < 250) {
            m_cmdBuf[m_cmdLen++] = (char)uni;
            char tmp[2] = {(char)uni, 0};
            appendOutput(tmp);
        }
        return true;
    }
    return false;
}

QDesktopWindow *createTerminalApp(QDesktop *desktop) {
    int ww = 700, wh = 420;
    QTerminal *term = new QTerminal(nullptr, "terminal");
    term->setGeometry(0, 0,
        ww - 2*QDesktopWindow::BORDER_W,
        wh - QDesktopWindow::TITLE_H - 3*QDesktopWindow::BORDER_W);
    return desktop->addWindow("Terminal", term, ww, wh);
}


