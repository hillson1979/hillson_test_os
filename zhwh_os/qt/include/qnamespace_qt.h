/**
 * @file qnamespace_qt.h
 * @brief Qt namespace — key codes, modifiers, and global enums for HillsonOS
 *
 * Extracted from Qt/Embedded 3.3.8b qnamespace.h.
 * Standalone: no Qt header dependencies. Freestanding C++ compatible.
 */
#ifndef QNAMESPACE_QT_H
#define QNAMESPACE_QT_H

class Qt {
public:
    // --- Key codes (matching Qt 3.3.8b pc101KeyM table indices) ---
    enum Key {
        Key_Escape      = 0x1000,
        Key_Tab         = 0x1001,
        Key_Backspace   = 0x1003,
        Key_Return      = 0x1004,
        Key_Enter       = 0x1005,
        Key_Insert      = 0x1006,
        Key_Delete      = 0x1007,
        Key_Pause       = 0x1008,
        Key_Print       = 0x1009,
        Key_SysReq      = 0x100A,
        Key_Home        = 0x1010,
        Key_End         = 0x1011,
        Key_Left        = 0x1012,
        Key_Up          = 0x1013,
        Key_Right       = 0x1014,
        Key_Down        = 0x1015,
        Key_PageUp      = 0x1016,
        Key_PageDown    = 0x1017,
        Key_Shift       = 0x1020,
        Key_Control     = 0x1021,
        Key_Meta        = 0x1022,
        Key_Alt         = 0x1023,
        Key_CapsLock    = 0x1024,
        Key_NumLock     = 0x1025,
        Key_ScrollLock  = 0x1026,

        Key_F1  = 0x1030, Key_F2  = 0x1031, Key_F3  = 0x1032, Key_F4  = 0x1033,
        Key_F5  = 0x1034, Key_F6  = 0x1035, Key_F7  = 0x1036, Key_F8  = 0x1037,
        Key_F9  = 0x1038, Key_F10 = 0x1039, Key_F11 = 0x103A, Key_F12 = 0x103B,

        // ASCII range
        Key_Space   = 0x20,
        Key_Exclam  = 0x21,
        Key_NumberSign = 0x23,
        Key_Dollar  = 0x24,
        Key_Percent = 0x25,
        Key_Ampersand = 0x26,
        Key_Apostrophe = 0x27,
        Key_ParenLeft  = 0x28,
        Key_ParenRight = 0x29,
        Key_Asterisk = 0x2A,
        Key_Plus     = 0x2B,
        Key_Comma    = 0x2C,
        Key_Minus    = 0x2D,
        Key_Period   = 0x2E,
        Key_Slash    = 0x2F,
        Key_0 = 0x30, Key_1 = 0x31, Key_2 = 0x32, Key_3 = 0x33, Key_4 = 0x34,
        Key_5 = 0x35, Key_6 = 0x36, Key_7 = 0x37, Key_8 = 0x38, Key_9 = 0x39,
        Key_Colon = 0x3A,
        Key_Semicolon = 0x3B,
        Key_Less      = 0x3C,
        Key_Equal     = 0x3D,
        Key_Greater   = 0x3E,
        Key_Question  = 0x3F,
        Key_At        = 0x40,
        Key_A = 0x41, Key_B = 0x42, Key_C = 0x43, Key_D = 0x44, Key_E = 0x45,
        Key_F = 0x46, Key_G = 0x47, Key_H = 0x48, Key_I = 0x49, Key_J = 0x4A,
        Key_K = 0x4B, Key_L = 0x4C, Key_M = 0x4D, Key_N = 0x4E, Key_O = 0x4F,
        Key_P = 0x50, Key_Q = 0x51, Key_R = 0x52, Key_S = 0x53, Key_T = 0x54,
        Key_U = 0x55, Key_V = 0x56, Key_W = 0x57, Key_X = 0x58, Key_Y = 0x59,
        Key_Z = 0x5A,
        Key_BracketLeft  = 0x5B,
        Key_Backslash    = 0x5C,
        Key_BracketRight = 0x5D,
        Key_AsciiCircum  = 0x5E,
        Key_Underscore   = 0x5F,
        Key_QuoteLeft    = 0x60,
        Key_BraceLeft    = 0x7B,
        Key_Bar          = 0x7C,
        Key_BraceRight   = 0x7D,
        Key_AsciiTilde   = 0x7E,

        Key_unknown = 0xFFFF,
    };

    // --- Modifiers ---
    enum Modifier {
        NoModifier      = 0x0000,
        ShiftModifier   = 0x0200,
        ControlModifier = 0x0400,
        AltModifier     = 0x0800,
        MetaModifier    = 0x1000,
        KeypadModifier  = 0x2000,
        KeyButtonMask   = 0x3F00,
    };

    // --- Button state ---
    enum ButtonState {
        NoButton    = 0x0000,
        LeftButton  = 0x0001,
        RightButton = 0x0002,
        MidButton   = 0x0004,
        MouseButtonMask = 0x0007,
    };

    // --- Alignment ---
    enum AlignmentFlags {
        AlignLeft       = 0x0001,
        AlignRight      = 0x0002,
        AlignHCenter    = 0x0004,
        AlignTop        = 0x0020,
        AlignBottom     = 0x0040,
        AlignVCenter    = 0x0080,
        AlignCenter     = AlignHCenter | AlignVCenter,
    };

    // --- Orientation ---
    enum Orientation {
        Horizontal = 0,
        Vertical   = 1,
    };

    // --- Focus policy ---
    enum FocusPolicy {
        NoFocus       = 0,
        TabFocus      = 0x1,
        ClickFocus    = 0x2,
        StrongFocus   = 0x8,
        WheelFocus    = 0xF,
    };

    // --- Window flags ---
    enum WindowFlags {
        Widget      = 0x0000,
        Window      = 0x0001,
        Dialog      = 0x0003 | Window,
        Popup       = 0x0005 | Window,
        Desktop     = 0x0007 | Window,
        Tool        = 0x0009 | Window,
        ToolTip     = 0x000B | Window,
        SplashScreen = 0x000D | Window,
        WStyle_Customize    = 0x0100,
        WStyle_NormalBorder = 0x0000,
        WStyle_DialogBorder = 0x0400,
        WStyle_NoBorder     = 0x0800,
        WStyle_Title        = 0x1000,
        WStyle_SysMenu      = 0x2000,
        WStyle_Minimize     = 0x4000,
        WStyle_Maximize     = 0x8000,
        WStyle_MinMax       = WStyle_Minimize | WStyle_Maximize,
        WStyle_Tool         = 0x100000,
        WStyle_StaysOnTop   = 0x200000,
        WType_Mask          = 0x000F,
        WStyle_Mask         = 0xFFFF00,
    };

    // --- Pen style ---
    enum PenStyle {
        NoPen,
        SolidLine,
        DashLine,
        DotLine,
        DashDotLine,
        DashDotDotLine,
    };

    // --- Brush style ---
    enum BrushStyle {
        NoBrush,
        SolidPattern,
        Dense1Pattern, Dense2Pattern, Dense3Pattern, Dense4Pattern,
        Dense5Pattern, Dense6Pattern, Dense7Pattern,
        HorPattern, VerPattern, CrossPattern,
        BDiagPattern, FDiagPattern, DiagCrossPattern,
    };

    // --- RasterOp ---
    enum RasterOp {
        CopyROP,
        OrROP,
        XorROP,
        NotAndROP,
        EraseROP = NotAndROP,
        NotCopyROP,
        NotOrROP,
        NotXorROP,
        AndROP,
        NotEraseROP = AndROP,
        NotROP,
        ClearROP,
        SetROP,
        NopROP,
        AndNotROP,
        OrNotROP,
        NandROP,
        NorROP,
        LastROP = NorROP,
    };

    // --- Background mode ---
    enum BackgroundMode {
        TransparentMode,
        OpaqueMode,
    };

    // --- Text flags ---
    enum TextFlags {
        SingleLine      = 0x0080,
        DontClip        = 0x0100,
        ExpandTabs      = 0x0200,
        ShowPrefix      = 0x0400,
        WordBreak       = 0x0800,
        BreakAnywhere   = 0x1000,
        DontPrint       = 0x2000,
        IncludeTrailingSpaces = 0x40000000,
        NoAccel         = 0x8000,
    };

    // --- Arrow type ---
    enum ArrowType {
        UpArrow,
        DownArrow,
        LeftArrow,
        RightArrow,
    };

    // --- Date/Time format ---
    enum DateFormat {
        TextDate,
        ISODate,
        LocalDate,
    };
    enum TimeSpec {
        LocalTime,
        UTC,
        OffsetFromUTC,
    };
};

// Convenience alias (common in Qt code)
typedef Qt::Key QKey;

#endif // QNAMESPACE_QT_H
