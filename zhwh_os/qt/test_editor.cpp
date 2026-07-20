#include "qpainter.h"
#include "qtextedit.h"

extern "C" {
#include "libuser_minimal.h"
int usb_mouse_info(uint8_t *ep, uint8_t *maxpkt, uint8_t *interval);
}

#define SYS_GUI_INPUT_READ 72
#define SYS_YIELD 3

typedef struct { uint32_t type; int x; int y; uint32_t pressed; } input_event_t;

static int read_input(input_event_t *ev, int type) {
    int r; __asm__ volatile("int $0x80":"=a"(r):"a"(SYS_GUI_INPUT_READ),"b"(ev),"c"(type):"memory","cc"); return r;
}
static void yield_cpu() { __asm__ volatile("int $0x80"::"a"(SYS_YIELD):"memory","cc"); }

int main(void) {
    printf("EDIT START\n");
    fb_info_t fb;
    if (gui_get_fb_info(&fb) != 0) return -1;
    uint32_t *fb_virt = (uint32_t*)0xF0000000;
    QPainter painter(fb_virt, fb.width, fb.height, fb.pitch);
    painter.clear(COLOR_DARK_GRAY);

    uint8_t a=0,b=0,c=0; usb_mouse_info(&a,&b,&c);
    char ub[40]; int ui=0;
    const char *u = "USB: ep=0x"; while(*u) ub[ui++]=*u++;
    ub[ui++]="0123456789ABCDEF"[a>>4]; ub[ui++]="0123456789ABCDEF"[a&0xF];
    u=" mx="; while(*u) ub[ui++]=*u++;
    if(b>=100)ub[ui++]='0'+b/100; if(b>=10)ub[ui++]='0'+(b/10)%10; ub[ui++]='0'+b%10;
    u=" int="; while(*u) ub[ui++]=*u++;
    if(c>=100)ub[ui++]='0'+c/100; if(c>=10)ub[ui++]='0'+(c/10)%10; ub[ui++]='0'+c%10;
    ub[ui]=0;
    painter.setColor(0x00FFFF00); painter.fillRect(0,0,fb.width,16);
    painter.setColor(COLOR_BLACK); painter.drawText(4,4,ub);

    QTextEdit *editor = new QTextEdit(nullptr, "editor");
    editor->setGeometry(0, 16, fb.width, fb.height-36);
    int statusY = fb.height - 20;
    painter.setColor(COLOR_DARK_BLUE); painter.fillRect(0,statusY,fb.width,20);
    painter.setColor(COLOR_WHITE); painter.drawText(8,statusY+6,"ESC:Exit | Arrows | Shift | Backspace | F1:Log");
    editor->render(&painter);

    bool e0p=false, sh=false;
    int mx=fb.width/2, my=fb.height/2;
    char logbuf[4096]={0};
    char log_usb[4096]={0};
    bool show_log = false;

    while(1){
        yield_cpu();
        input_event_t me; int mr = read_input(&me, 2);
        if(mr==1){ mx=me.x; my=me.y;
            if(mx<0)mx=0; if(my<0)my=0;
            if(mx>=(int)fb.width)mx=fb.width-1; if(my>=(int)fb.height)my=fb.height-1; }
        bool need_render = false;
        if(mr==1 && me.pressed) { editor->setCursorScreenPos(mx,my); need_render=true; }

        input_event_t ke; int kr = read_input(&ke, 1);
        if(kr>0){
            int sc=ke.x;
            if(sc==0xE0){e0p=true;}
            else if(e0p){sc|=0xE000;e0p=false;goto pk;}
            else{pk:
                if(sc==0x2A||sc==0x36||sc==0xAA||sc==0xB6){sh=!(sc&0x80);}
                else if(!(sc&0x80)){
                    if(sc==0x01)break;
                    if(sc==0x3B){show_log=!show_log;if(show_log){
                        __asm__ volatile("int $0x80"::"a"(77),"b"(logbuf):"memory");
                        // Filter USB lines
                        int wi=0; char *ls=logbuf; while(*ls&&wi<4095){
                            char *le=ls; while(*le&&*le!='\n')le++;
                            int len=le-ls; if(len>0){
                                // Check if line contains "USB" or "usb" or "usb"
                                int has=0; for(int i=0;i<len-1;i++){
                                    if((ls[i]=='U'||ls[i]=='u')&&(ls[i+1]=='S'||ls[i+1]=='s')){has=1;break;} // USB
                                    if(ls[i]=='E'&&ls[i+1]=='H'){has=1;break;} // EHCI
                                }
                                if(has){for(int i=0;i<=len&&wi<4095;i++)log_usb[wi++]=ls[i];}
                            }
                            ls=(*le)?le+1:le;
                        }
                        log_usb[wi]=0;
                    }need_render=true;}
                    else {int bs=sc;if(sc&0xE000)bs=sc&0xFF;editor->keyPress(bs,sh);need_render=true;}
                }
            }
        }

        if(need_render){
            if(show_log){
                painter.setColor(COLOR_BLACK);painter.fillRect(0,16,fb.width,fb.height-36);
                painter.setColor(COLOR_GREEN);int ly=18;
                int col=0;
                char *dp = show_log ? log_usb : logbuf;
                for(char*p=dp;*p&&ly<(int)fb.height-30;p++){
                    if(*p=='\n'){ly+=10;col=0;continue;}
                    if(*p=='\r')continue;
                    char t[2]={*p,0};painter.drawText(4+col*8,ly,t);
                    col++; if(col>=120){ly+=10;col=0;}
                }
            }else{ editor->render(&painter); }
        }

        static int lcx=-1,lcy=-1;
        volatile uint32_t *fbp=fb_virt; int pp=fb.pitch/4;
        if(mx!=lcx||my!=lcy||need_render){
            if(lcx>=0&&lcx<(int)fb.width&&lcy>=0&&lcy<(int)fb.height){
                for(int dy=-8;dy<=8;dy++)if(lcy+dy>=0&&lcy+dy<(int)fb.height)fbp[(lcy+dy)*pp+lcx]^=0x00FFFFFF;
                for(int dx=-8;dx<=8;dx++)if(lcx+dx>=0&&lcx+dx<(int)fb.width)fbp[lcy*pp+(lcx+dx)]^=0x00FFFFFF;
            }
            for(int dy=-8;dy<=8;dy++)if(my+dy>=0&&my+dy<(int)fb.height)fbp[(my+dy)*pp+mx]^=0x00FFFFFF;
            for(int dx=-8;dx<=8;dx++)if(mx+dx>=0&&mx+dx<(int)fb.width)fbp[my*pp+(mx+dx)]^=0x00FFFFFF;
            lcx=mx;lcy=my;
        }

        painter.setColor(COLOR_DARK_BLUE);painter.fillRect(0,statusY,fb.width,20);
        painter.setColor(COLOR_WHITE);
        char st[40];int si=0;
        st[si++]='L';st[si++]='n';st[si++]=' ';
        int ln=editor->cursorLine()+1;
        if(ln>=100)st[si++]='0'+ln/100;if(ln>=10)st[si++]='0'+(ln/10)%10;st[si++]='0'+ln%10;
        st[si++]=' ';st[si++]='C';st[si++]=' ';
        int cl=editor->cursorCol()+1;
        if(cl>=100)st[si++]='0'+cl/100;if(cl>=10)st[si++]='0'+(cl/10)%10;st[si++]='0'+cl%10;
        st[si]=0;
        painter.drawText(8,statusY+6,st);

        // Show DMA bytes
        int dma=0;__asm__ volatile("int $0x80":"=a"(dma):"a"(76),"b"(1):"memory");
        char db[20];int di=0;for(int i=0;i<4;i++){int bt=(dma>>(i*8))&0xFF;db[di++]="0123456789ABCDEF"[bt>>4];db[di++]="0123456789ABCDEF"[bt&0xF];db[di++]=' ';}db[di]=0;
        painter.drawText(400,statusY+6,db);
        // Show EHCI addr via syscall 76 ebx=3 (fl) and ebx=4 (qh)
        int efl=0,eqh=0;
        __asm__ volatile("int $0x80":"=a"(efl):"a"(76),"b"(3):"memory");
        __asm__ volatile("int $0x80":"=a"(eqh):"a"(76),"b"(4):"memory");
        if(efl){ char eb[50];int ei=0;eb[ei++]='F';eb[ei++]=':';for(int n=28;n>=0;n-=4)eb[ei++]="0123456789ABCDEF"[(efl>>n)&0xF];eb[ei++]=' ';eb[ei++]='Q';eb[ei++]=':';for(int n=28;n>=0;n-=4)eb[ei++]="0123456789ABCDEF"[(eqh>>n)&0xF];eb[ei]=0;painter.drawText(4,statusY-10,eb);}
    }

    delete editor;
    printf("EDIT DONE\n");
    return 0;
}
