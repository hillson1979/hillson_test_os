/**
 * @file qscreenhillsonfb_qws.h
 * @brief QScreen driver for HillsonOS direct framebuffer (0xF0000000)
 *
 * HillsonOS already has the framebuffer identity-mapped by the kernel.
 * No mmap/fd/ioctl needed — just set the pointer and geometry.
 */
#ifndef QSCREEN_HILLSONFB_QWS_H
#define QSCREEN_HILLSONFB_QWS_H

#include "qcolor_qt.h"
#include "qpoint_qt.h"
#include "qrect_qt.h"

// Minimal QScreen base class (extracted from Qt/Embedded 3.3.8b qgfx_qws.h)
class QScreen {
public:
    QScreen(int display_id);
    virtual ~QScreen();

    // Pure virtual — every screen driver must implement
    virtual bool connect(const char *displaySpec) = 0;
    virtual bool initDevice() = 0;
    virtual void disconnect() = 0;
    virtual void setMode(int w, int h, int d) = 0;

    // Accessors (populated by connect())
    int width()        const { return m_w; }
    int height()       const { return m_h; }
    int depth()        const { return m_d; }
    int linestep()     const { return m_lstep; }
    unsigned char *base() const { return m_data; }
    int screenSize()   const { return m_size; }
    int pixelType()    const { return m_pixeltype; }
    int deviceWidth()  const { return m_dw; }
    int deviceHeight() const { return m_dh; }
    QRgb *clut()       { return m_clut; }
    int numCols()      { return m_numCols; }

    // Optional overrides
    virtual void shutdownDevice() {}
    virtual void blank(bool) {}

protected:
    unsigned char *m_data;   // framebuffer pointer
    int m_w, m_h;            // width, height
    int m_d;                  // bits per pixel
    int m_lstep;              // line step in bytes
    int m_size;               // total framebuffer size
    int m_mapsize;
    int m_pixeltype;
    int m_dw, m_dh;           // device width/height
    int m_displayId;
    QRgb m_clut[256];
    int m_numCols;
    bool m_initted;
};

// ---- HillsonOS Framebuffer Screen Driver ----

class QHillsonFbScreen : public QScreen {
public:
    QHillsonFbScreen(int display_id = 0);
    ~QHillsonFbScreen();

    bool connect(const char *displaySpec) override;
    bool initDevice() override;
    void disconnect() override;
    void setMode(int w, int h, int d) override;
    void shutdownDevice() override;
    void blank(bool on) override;
};

// Global screen pointer (set by QApplication init)
extern QScreen *qt_screen;

#endif // QSCREEN_HILLSONFB_QWS_H
