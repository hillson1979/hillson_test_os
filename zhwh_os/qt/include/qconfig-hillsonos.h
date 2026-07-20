/**
 * @file qconfig-hillsonos.h
 * @brief Qt/Embedded feature configuration for HillsonOS
 *
 * Based on qconfig-minimal.h from Qt/Embedded 3.3.8b.
 * Disables everything not essential for a bare-metal framebuffer GUI.
 */

#ifndef QT_H
#endif

// --- Core: keep signals/slots, properties ---
// Qt without properties and signals/slots is not Qt

// --- Disable I/O subsystems ---
#define QT_NO_NETWORK
#define QT_NO_SQL
#define QT_NO_SOUND
#define QT_NO_PRINTER
#define QT_NO_PICTURE

// --- Disable image I/O (no PNG/JPEG/BMP decoders) ---
#define QT_NO_IMAGEIO
#define QT_NO_IMAGEIO_BMP
#define QT_NO_IMAGEIO_PPM
#define QT_NO_IMAGEIO_XBM
#define QT_NO_IMAGEIO_XPM

// --- Disable complex widgets and features ---
#define QT_NO_ACTION
#define QT_NO_STYLE
#define QT_NO_EFFECTS
#define QT_NO_CURSOR
#define QT_NO_QWS_CURSOR
#define QT_NO_PALETTE
#define QT_NO_COLORNAMES
#define QT_NO_TRANSLATION
#define QT_NO_MIME
#define QT_NO_SESSIONMANAGER
#define QT_NO_ACCEL
#define QT_NO_SEMIMODAL
#define QT_NO_DIR
#define QT_NO_FONTDATABASE
#define QT_NO_BDF
#define QT_NO_TRANSFORMATIONS
#define QT_NO_LAYOUT
#define QT_NO_DRAWUTIL
#define QT_NO_TEXTSTREAM
#define QT_NO_DATASTREAM
#define QT_NO_VALIDATOR
#define QT_NO_RANGECONTROL
#define QT_NO_WHEELEVENT
#define QT_NO_BEZIER
#define QT_NO_SYNTAXHIGHLIGHTER
#define QT_NO_QWS_SAVEFONTS
#define QT_NO_COP
#define QT_NO_QWS_MANAGER
#define QT_NO_QWS_GFX_SPEED
#define QT_NO_QWS_MOUSE_AUTO
#define QT_NO_ASYNC_IO
#define QT_NO_ASYNC_IMAGE_IO
#define QT_NO_DIRECTPAINTER
#define QT_NO_QWS_KEYBOARD  // We provide our own via QWSKeyboardHandler subclass

// --- Disable STL ---
#ifndef QT_NO_STL
#define QT_NO_STL
#endif

// --- Disable regexp ---
#define QT_NO_REGEXP
#define QT_NO_REGEXP_CAPTURE
#define QT_NO_REGEXP_WILDCARD

// --- Disable complex string features ---
#define QT_NO_SPRINTF
#define QT_NO_QUUID_STRING
#define QT_NO_DATESTRING
#define QT_NO_SIGNALMAPPER
#define QT_NO_TEMPLATE_VARIANT

// --- Disable image processing ---
#define QT_NO_IMAGE_TRUECOLOR
#define QT_NO_IMAGE_SMOOTHSCALE
#define QT_NO_IMAGE_TEXT
#define QT_NO_IMAGE_DITHER_TO_1
#define QT_NO_IMAGE_HEURISTIC_MASK
#define QT_NO_IMAGE_MIRROR

// --- Disable process management ---
#define QT_NO_PROCESS
#define QT_NO_WMATRIX

// --- Keep enabled (essential) ---
// QT_NO_BUTTON is NOT defined — we need buttons
// QT_NO_FRAME is NOT defined — we need frames
// QT_NO_DIALOG is NOT defined — we need dialogs
// Properties (signals/slots) are NOT disabled

#endif // QCONFIG_HILLSONOS_H
