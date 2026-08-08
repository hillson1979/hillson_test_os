/**
 * @file keycodes.h
 */
#ifndef KERNEL_KEYCODES_H
#define KERNEL_KEYCODES_H

/* Keycode constants used by usb_keytable.c and usb_hid.c */
#define KEYCODE_A          0x1E
#define KEYCODE_B          0x30
#define KEYCODE_C          0x2E
#define KEYCODE_D          0x20
#define KEYCODE_E          0x12
#define KEYCODE_F          0x21
#define KEYCODE_G          0x22
#define KEYCODE_H          0x23
#define KEYCODE_I          0x17
#define KEYCODE_J          0x24
#define KEYCODE_K          0x25
#define KEYCODE_L          0x26
#define KEYCODE_M          0x32
#define KEYCODE_N          0x31
#define KEYCODE_O          0x18
#define KEYCODE_P          0x19
#define KEYCODE_Q          0x10
#define KEYCODE_R          0x13
#define KEYCODE_S          0x1F
#define KEYCODE_T          0x14
#define KEYCODE_U          0x16
#define KEYCODE_V          0x2F
#define KEYCODE_W          0x11
#define KEYCODE_X          0x2D
#define KEYCODE_Y          0x15
#define KEYCODE_Z          0x2C

#define KEYCODE_0          0x0B
#define KEYCODE_1          0x02
#define KEYCODE_2          0x03
#define KEYCODE_3          0x04
#define KEYCODE_4          0x05
#define KEYCODE_5          0x06
#define KEYCODE_6          0x07
#define KEYCODE_7          0x08
#define KEYCODE_8          0x09
#define KEYCODE_9          0x0A

#define KEYCODE_ENTER      0x1C
#define KEYCODE_ESC        0x01
#define KEYCODE_BACKSPACE  0x0E
#define KEYCODE_TAB        0x0F
#define KEYCODE_SPACE      0x39
#define KEYCODE_MINUS      0x0C
#define KEYCODE_EQUAL      0x0D
#define KEYCODE_LBRACKET   0x1A
#define KEYCODE_RBRACKET   0x1B
#define KEYCODE_BACKSLASH  0x2B
#define KEYCODE_SEMICOLON  0x27
#define KEYCODE_QUOTE      0x28
#define KEYCODE_BACKTICK   0x29
#define KEYCODE_COMMA      0x33
#define KEYCODE_DOT        0x34
#define KEYCODE_SLASH      0x35
#define KEYCODE_CAPSLOCK   0x3A

#define KEYCODE_F1         0x3B
#define KEYCODE_F2         0x3C
#define KEYCODE_F3         0x3D
#define KEYCODE_F4         0x3E
#define KEYCODE_F5         0x3F
#define KEYCODE_F6         0x40
#define KEYCODE_F7         0x41
#define KEYCODE_F8         0x42
#define KEYCODE_F9         0x43
#define KEYCODE_F10        0x44
#define KEYCODE_F11        0x57
#define KEYCODE_F12        0x58

#define KEYCODE_PRINTSCR   0x37
#define KEYCODE_SCROLLLOCK 0x46
#define KEYCODE_PAUSE      0x45
#define KEYCODE_INSERT     0x52
#define KEYCODE_HOME       0x47
#define KEYCODE_PGUP       0x49
#define KEYCODE_DELETE     0x53
#define KEYCODE_END        0x4F
#define KEYCODE_PGDN       0x51
#define KEYCODE_RIGHT      0x4D
#define KEYCODE_LEFT       0x4B
#define KEYCODE_DOWN       0x50
#define KEYCODE_UP         0x48

#define KEYCODE_NUM        0x45
#define KEYCODE_CAPS       0x3A
#define KEYCODE_SCROLL      0x46
#define KEYCODE_NUMLOCK    0x45
#define KEYCODE_KP_DIV     0x35
#define KEYCODE_KP_MULT    0x37
#define KEYCODE_KP_MINUS   0x4A
#define KEYCODE_KP_PLUS    0x4E
#define KEYCODE_KP_ENTER   0x1C
#define KEYCODE_KP_DOT     0x53
#define KEYCODE_KP_0       0x52
#define KEYCODE_KP_1       0x4F
#define KEYCODE_KP_2       0x50
#define KEYCODE_KP_3       0x51
#define KEYCODE_KP_4       0x4B
#define KEYCODE_KP_5       0x4C
#define KEYCODE_KP_6       0x4D
#define KEYCODE_KP_7       0x47
#define KEYCODE_KP_8       0x48
#define KEYCODE_KP_9       0x49

#define KEYCODE_LCTRL      0x1D
#define KEYCODE_LSHIFT     0x2A
#define KEYCODE_LALT       0x38
#define KEYCODE_LGUI       0x5B
#define KEYCODE_RCTRL      0x1D
#define KEYCODE_RSHIFT     0x36
#define KEYCODE_RALT       0x38
#define KEYCODE_RGUI       0x5C

#define KEYCODE_APPS       0x5D
#define KEYCODE_POWER      0x5E
#define KEYCODE_SLEEP      0x5F

#define KEYCODE_MUTE       0x20
#define KEYCODE_VOLUP      0x30
#define KEYCODE_VOLDOWN    0x2E
#define KEYCODE_MEDIA      0x2C

#define KEYCODE_AUD_NEXT   0x19
#define KEYCODE_AUD_PREV   0x10
#define KEYCODE_SELECT     0x77
#define KEYCODE_VOLDN      0x2E
#define KEYCODE_AUD_STOP   0x24
#define KEYCODE_AUD_PLAY   0x22
#define KEYCODE_AUD_MUTE   0x20

#define KEYCODE_BREAK_MASK 0x80

#endif /* KERNEL_KEYCODES_H */
