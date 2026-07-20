/**
 * @file qscreenhillsonfb_qws.cpp
 * @brief HillsonOS framebuffer QScreen driver implementation
 *
 * The framebuffer is already mapped by the kernel at 0xF0000000.
 * We just need to point QScreen's data pointer there and set geometry.
 */
#include "include/qscreenhillsonfb_qws.h"

// Syscall numbers
#define SYS_GUI_FB_INFO  70
#define SYS_GUI_FB_BLIT  71

// Framebuffer info struct (must match kernel)
struct fb_info_t {
    void     *fb_addr;
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int bpp;
};

// Global screen pointer
QScreen *qt_screen = 0;

// ============================================================
// QScreen base class
// ============================================================

QScreen::QScreen(int display_id)
    : m_data(0), m_w(0), m_h(0), m_d(0), m_lstep(0), m_size(0), m_mapsize(0),
      m_pixeltype(0), m_dw(0), m_dh(0), m_displayId(display_id),
      m_numCols(0), m_initted(false)
{
    // Clear clut
    for (int i = 0; i < 256; i++) m_clut[i] = 0;
}

QScreen::~QScreen() {}

// ============================================================
// QHillsonFbScreen implementation
// ============================================================

QHillsonFbScreen::QHillsonFbScreen(int display_id)
    : QScreen(display_id)
{
}

QHillsonFbScreen::~QHillsonFbScreen()
{
    disconnect();
}

bool QHillsonFbScreen::connect(const char * /*displaySpec*/)
{
    // Get framebuffer info from kernel via syscall
    fb_info_t fb;
    int r = -1;
    __asm__ volatile("int $0x80"
        : "=a"(r)
        : "a"(SYS_GUI_FB_INFO), "b"(&fb)
        : "memory", "cc");

    if (r != 0) {
        // Fallback: use known defaults for 1024x768x32
        fb.fb_addr = (void*)0xF0000000;
        fb.width   = 1024;
        fb.height  = 768;
        fb.pitch   = 4096;
        fb.bpp     = 32;
    }

    // Set QScreen parameters from framebuffer info
    m_data  = (unsigned char*)fb.fb_addr;
    m_w     = fb.width;
    m_h     = fb.height;
    m_d     = fb.bpp;
    m_lstep = fb.pitch;
    m_dw    = fb.width;
    m_dh    = fb.height;
    m_size  = fb.pitch * fb.height;
    m_mapsize = m_size;

    // 32bpp RGB framebuffer
    m_pixeltype = 0;  // NormalPixel
    m_numCols = 0;

    return m_data != 0;
}

bool QHillsonFbScreen::initDevice()
{
    if (!m_data) return false;

    // Clear screen to black
    unsigned int *fb = (unsigned int*)m_data;
    int pixels = m_size / 4;
    for (int i = 0; i < pixels; i++) {
        fb[i] = 0xFF000000;  // opaque black
    }

    m_initted = true;
    return true;
}

void QHillsonFbScreen::disconnect()
{
    // Nothing to unmap — framebuffer is permanently mapped by kernel
    m_data = 0;
    m_initted = false;
}

void QHillsonFbScreen::setMode(int w, int h, int d)
{
    // Fixed mode with this framebuffer — ignored
    (void)w; (void)h; (void)d;
}

void QHillsonFbScreen::shutdownDevice()
{
    // Nothing to restore
}

void QHillsonFbScreen::blank(bool on)
{
    if (!m_data) return;

    if (on) {
        // Blank: fill with black
        unsigned int *fb = (unsigned int*)m_data;
        int pixels = m_size / 4;
        for (int i = 0; i < pixels; i++) fb[i] = 0xFF000000;
    }
}
