/**
 * @file ps2mouse.c — PS/2 mouse driver for Dell laptop touchpad
 */
#include <stdint.h>
#include "printf.h"
#include "x86/io.h"

#define PS2_DATA   0x60
#define PS2_STATUS 0x64
#define PS2_CMD    0x64

static int ps2_mouse_x = 512, ps2_mouse_y = 384;
static uint8_t ps2_mouse_btn = 0;
static int ps2_ok = 0;
static int ps2_cycle = 0;
static uint8_t ps2_buf[3];

static void ps2_wait_in(void)  { while(inb(PS2_STATUS)&2); }
static void ps2_wait_out(void) { for(int i=0;i<100000;i++) if(inb(PS2_STATUS)&1) return; }

static uint8_t ps2_read(void) { ps2_wait_out(); return inb(PS2_DATA); }
static void ps2_write(uint8_t v) { ps2_wait_in(); outb(PS2_CMD, v); }

static void ps2_write_data(uint8_t v) { ps2_wait_in(); outb(PS2_DATA, v); }
static uint8_t ps2_read_ack(void) { uint8_t r=ps2_read(); return r==0xFA?0:r; }

int ps2mouse_init(void) {
    printf("[PS2] Mouse init start\n");
    // Disable ports
    ps2_write(0xAD); // disable keyboard
    ps2_write(0xA7); // disable mouse
    // Flush output
    while(inb(PS2_STATUS)&1) inb(PS2_DATA);
    // Enable auxiliary device
    ps2_write(0xA8);
    // Enable IRQ12
    uint8_t cfg;
    ps2_write(0x20); cfg=ps2_read();
    printf("[PS2] Config=0x%x\n",cfg);
    cfg|=2; // enable IRQ12
    ps2_write(0x60); ps2_write_data(cfg);
    // Enable mouse port
    ps2_write(0xA8);
    // Reset mouse
    ps2_write_data(0xFF); if(ps2_read_ack()!=0){printf("[PS2] Reset failed\n");return-1;}
    uint8_t r=ps2_read(); printf("[PS2] Reset resp=0x%x\n",r);
    // Set defaults
    ps2_write_data(0xF6); ps2_read_ack(); // set defaults
    ps2_write_data(0xF4); ps2_read_ack(); // enable data reporting
    // Enable IRQ12 in IOAPIC
    extern void ioapicenable(int irq, int cpunum);
    ioapicenable(12,0);
    // Enable keyboard port too
    ps2_write(0xAE);
    ps2_ok=1;
    printf("[PS2] Mouse init done\n");
    return 0;
}

void ps2mouse_handler(void) {
    if(!ps2_ok)return;
    uint8_t d=inb(PS2_DATA);
    // 3-byte packet assembly
    if(ps2_cycle==0){
        if(!(d&8))return; // bit3 must be set
        ps2_buf[0]=d; ps2_cycle=1;
    }else if(ps2_cycle==1){
        ps2_buf[1]=d; ps2_cycle=2;
    }else{
        ps2_buf[2]=d; ps2_cycle=0;
        // Update position
        uint8_t btn=ps2_buf[0]&7;
        int dx=ps2_buf[1]; if(ps2_buf[0]&0x10) dx-=256;
        int dy=ps2_buf[2]; if(ps2_buf[0]&0x20) dy-=256;
        ps2_mouse_x+=dx; if(ps2_mouse_x<0)ps2_mouse_x=0; if(ps2_mouse_x>1024)ps2_mouse_x=1024;
        ps2_mouse_y-=dy; if(ps2_mouse_y<0)ps2_mouse_y=0; if(ps2_mouse_y>768)ps2_mouse_y=768;
        ps2_mouse_btn=btn;
    }
}

int ps2mouse_poll(int *x, int *y, int *btn) {
    if(!ps2_ok)return 0;
    // Check if data available (non-blocking)
    static int lastx=-1, lasty=-1, lastbtn=-1;
    if(ps2_mouse_x!=lastx || ps2_mouse_y!=lasty || ps2_mouse_btn!=lastbtn){
        *x=ps2_mouse_x; *y=ps2_mouse_y; *btn=ps2_mouse_btn;
        lastx=ps2_mouse_x; lasty=ps2_mouse_y; lastbtn=ps2_mouse_btn;
        return 1;
    }
    return 0;
}
