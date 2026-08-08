/* 
/**
 *    Copyright 2025, 2026 (c)
 * 
 *    file: usb_hid.c
 *

/**
 *  \file usb_hid.c
 *
 *  The Universal Serial Bus (USB) driver code is split into several files:
 *    - usb.c       => main entry point and general functions
 *    - usb_msd.c   => functions to handle Mass Storage Devices (MSD)
 *    - usb_hid.c   => functions to handle Human Interaction Devices (HID)
 *    - usb_hub.c   => functions to handle USB hubs
 *    - usb_ioctl.c => functions to handle ioctl() calls
 *    - usb_ohci.c  => OHCI layer
 *    - usb_uhci.c  => UHCI layer
 *    - usb_ehci.c  => EHCI layer
 */

//#define __DEBUG

#define KQUEUE_DEFINE_INLINES   1
#define KQUEUE_SIZE             KEY_BUF_SIZE

#include <kernel/errno.h>
#include <kernel/pciio.h>
#include <kernel/asm.h>
#include <kernel/usb.h>
#include <kernel/usb_hid.h>
#include <kernel/mouse.h>
#include <kernel/kbd.h>
#include <kernel/kqueue.h>
#include <kernel/keycodes.h>
#include <kernel/hillsonos.h>
#include <mm/kheap.h>
#include <usb_mouse.h>

#ifndef HID_SUBCLASS_BOOT
#define HID_SUBCLASS_BOOT 0x01
#endif

//volatile struct task_t *hid_task;
struct usb_hid_dev_t hid_list;
kernel_mutex_t usb_hid_tablock;

/* Exported for usb_hcd.c periodic poll */
struct usb_hid_dev_t *g_first_mouse_hid = NULL;

// defined in drivers/mouse.c
extern mouse_buttons_t cur_button_state;

// defined in usb_keytable.c
extern char usb_keycodes[];

static void hid_parse_mouse_report_desc(struct usb_hid_dev_t *hid,
                                        uint8_t *rdesc, int len)
{
    uint16_t logical_max = 0;
    uint16_t report_size = 0, report_count = 0;
    uint16_t bits = 0;
    int saw_mouse = 0;
    int saw_x = 0, saw_y = 0;
    int saw_abs_input = 0;

    hid->logical_max_x = 1023;
    hid->logical_max_y = 767;
    hid->report_len = 4;
    hid->report_id = 0;

    for (int i = 0; i < len; i++) {
        uint8_t b = rdesc[i];
        if (b == 0x09 && i + 1 < len) {
            uint8_t usage = rdesc[++i];
            if (usage == 0x02) saw_mouse = 1;
            if (usage == 0x30) saw_x = 1;
            if (usage == 0x31) saw_y = 1;
        } else if (b == 0x85 && i + 1 < len) {
            hid->report_id = rdesc[++i];
        } else if (b == 0x75 && i + 1 < len) {
            report_size = rdesc[++i];
        } else if (b == 0x95 && i + 1 < len) {
            report_count = rdesc[++i];
        } else if (b == 0x25 && i + 1 < len) {
            logical_max = rdesc[++i];
        } else if (b == 0x26 && i + 2 < len) {
            logical_max = (uint16_t)rdesc[i + 1] |
                          ((uint16_t)rdesc[i + 2] << 8);
            i += 2;
        } else if (b == 0x81 && i + 1 < len) {
            uint8_t flags = rdesc[++i];
            if (report_size && report_count)
                bits += report_size * report_count;
            if (!(flags & 0x04) && saw_x && saw_y) {
                saw_abs_input = 1;
                if (logical_max) {
                    hid->logical_max_x = logical_max;
                    hid->logical_max_y = logical_max;
                }
            }
        }
    }

    if (bits)
        hid->report_len = (bits + 7) / 8;
    if (hid->report_id)
        hid->report_len++;
    if (hid->report_len < 4)
        hid->report_len = 4;
    if (hid->report_len > sizeof(hid->buf))
        hid->report_len = sizeof(hid->buf);

    hid->is_mouse = saw_mouse || (hid->iface && hid->iface->desc.protocol == 2);
    hid->mouse_absolute = hid->is_mouse && saw_abs_input;

    usb_printk("usb: HID mouse parse is_mouse=%d abs=%d report_id=%d report_len=%d logical=%u,%u\n",
        hid->is_mouse, hid->mouse_absolute, hid->report_id, hid->report_len,
        hid->logical_max_x, hid->logical_max_y);
}


int usb_hid_set_protocol(struct usb_dev_t *usb, uint8_t protocol,
                         uint16_t interface)
{
    struct usb_transfer_t transfer;

    usb_setup_transfer(usb, usb->endpoints, &transfer, USB_TRANSFER_CTRL);
    /* bmReqType=0x21 (Class, Interface, Host→Device), bReq=SET_PROTOCOL(0x0B),
     * wValue=protocol (0=boot), wIndex=interface */
    usb_setup_transaction(&transfer, 0x21, 0x0B, 0, protocol, interface, 0);
    usb_in_transaction(&transfer, 1, 0, 0);  /* Status Stage (IN, 0-length) */
    usb_schedule_transfer(&transfer);
    usb_wait_transfer(&transfer);
    usb_delete_transfer(&transfer);

    /* Keep the result visible through SYS_USB_MOUSE_INFO for the physical
     * machine diagnostics. */
    extern int g_usb_setproto_result;
    g_usb_setproto_result = transfer.success ? 1 : 0;
    return transfer.success;
}


static void usb_handle_mouse_input(void *__hid)
{
    /* Button lookup: 5 buttons (bits 0..4) = 32 entries.
     * The mouse.h API only defines L/M/R, but our HID descriptor
     * reports 5 buttons.  Map buttons 4-5 to extended codes. */
#define BTN4  0x08
#define BTN5  0x10
    static uint8_t b0[32] =
    {
        0,
        MOUSE_LBUTTON_DOWN,
        MOUSE_RBUTTON_DOWN,
        MOUSE_LBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        MOUSE_MBUTTON_DOWN,
        MOUSE_MBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        /* btn4 + combos */
        BTN4,
        BTN4 | MOUSE_LBUTTON_DOWN,
        BTN4 | MOUSE_RBUTTON_DOWN,
        BTN4 | MOUSE_LBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN4 | MOUSE_MBUTTON_DOWN,
        BTN4 | MOUSE_MBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        BTN4 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN4 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        /* btn5 + combos */
        BTN5,
        BTN5 | MOUSE_LBUTTON_DOWN,
        BTN5 | MOUSE_RBUTTON_DOWN,
        BTN5 | MOUSE_LBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN5 | MOUSE_MBUTTON_DOWN,
        BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        /* btn4+btn5 + combos */
        BTN4 | BTN5,
        BTN4 | BTN5 | MOUSE_LBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_RBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_LBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_MBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN,
        BTN4 | BTN5 | MOUSE_MBUTTON_DOWN | MOUSE_RBUTTON_DOWN | MOUSE_LBUTTON_DOWN,
    };
#undef BTN4
#undef BTN5

    /*
     * Parse 4-byte HID mouse report:
     *   byte 0: buttons (bits 0..4), padding (bits 5..7)
     *   byte 1: X displacement (INT8, relative)
     *   byte 2: Y displacement (INT8, relative)
     *   byte 3: Wheel        (INT8, relative)
     *
     * See: https://wiki.osdev.org/USB_Human_Interface_Devices
     */

    struct usb_hid_dev_t *hid = __hid;
    uint8_t *buf = hid->buf;

    usb_printk("usb mouse report: %02x %02x %02x %02x %02x %02x %02x %02x\n",
               buf[0], buf[1], buf[2], buf[3],
               buf[4], buf[5], buf[6], buf[7]);

    if (hid->mouse_absolute) {
        int off = (hid->report_id && buf[0] == hid->report_id) ? 1 : 0;
        uint8_t raw_buttons = buf[off];
        uint16_t raw_x = (uint16_t)buf[off + 1] | ((uint16_t)buf[off + 2] << 8);
        uint16_t raw_y = (uint16_t)buf[off + 3] | ((uint16_t)buf[off + 4] << 8);
        uint16_t max_x = hid->logical_max_x ? hid->logical_max_x : 1023;
        uint16_t max_y = hid->logical_max_y ? hid->logical_max_y : 767;

        hid->abs_x = ((uint32_t)raw_x * 1023u) / max_x;
        hid->abs_y = ((uint32_t)raw_y * 767u) / max_y;
        if (hid->abs_x < 0) hid->abs_x = 0;
        if (hid->abs_y < 0) hid->abs_y = 0;
        if (hid->abs_x > 1023) hid->abs_x = 1023;
        if (hid->abs_y > 767) hid->abs_y = 767;

        cur_button_state = b0[raw_buttons & 0x1F];
        hid->new_data = 1;
        usb_printk("usb mouse abs: x=%d y=%d buttons=%x\n",
                   hid->abs_x, hid->abs_y,
                   (uint32_t)(raw_buttons & 0x1F));
        unblock_kernel_task(mouse_task);
        return;
    }

    int raw_dx;
    int raw_dy;
    int wheel;

    int actual_len = hid->transfer.actual_length;
    if (actual_len == 6) {
        /* This mouse uses a 6-byte relative report despite accepting boot
         * protocol: buttons, signed 16-bit X, signed 16-bit Y, wheel. Convert
         * it to the canonical 4-byte layout consumed by the GUI syscall. */
        raw_dx = (int16_t)((uint16_t)buf[1] |
                           ((uint16_t)buf[2] << 8));
        raw_dy = (int16_t)((uint16_t)buf[3] |
                           ((uint16_t)buf[4] << 8));
        wheel = (int8_t)buf[5];
        hid->report_len = 6;
    } else {
        raw_dx = (int8_t)buf[1];
        raw_dy = (int8_t)buf[2];
        wheel = (int8_t)buf[3];
    }

    /* The public mouse report remains byte-sized. Clamp unusually large
     * 16-bit deltas instead of allowing them to wrap through int8_t. */
    if (raw_dx > 127) raw_dx = 127;
    if (raw_dx < -127) raw_dx = -127;
    if (raw_dy > 127) raw_dy = 127;
    if (raw_dy < -127) raw_dy = -127;

    buf[1] = (uint8_t)(int8_t)raw_dx;
    /* SYS_GUI_INPUT_READ historically applies y -= report.y. Store the
     * opposite sign here so a positive device Y delta moves the screen cursor
     * downward, matching this physical mouse's relative-axis convention. */
    buf[2] = (uint8_t)(int8_t)(-raw_dy);
    buf[3] = (uint8_t)(int8_t)wheel;
    if (actual_len == 6) {
        buf[4] = 0;
        buf[5] = 0;
    }

    int dx = raw_dx;
    int dy = raw_dy;     /* Final screen-space delta after syscall conversion */
    uint8_t buttons = b0[buf[0] & 0x1F];  /* 5-button mask */

    cur_button_state = buttons;
    usb_printk("usb mouse input: dx=%d dy=%d wheel=%d buttons=%x\n",
               dx, dy, wheel, (uint32_t)(buf[0] & 0x1F));
    add_mouse_packet(dx, dy, buttons);
    hid->new_data = 1;  /* Signal usb_mouse_periodic_poll */
    unblock_kernel_task(mouse_task);
}


static void toggle_led(struct usb_hid_dev_t *hid, uint8_t bit)
{
    hid->leds ^= (1 << bit);
    usb_ctrl_out(hid->iface->usb, &hid->leds, 0x21, 9, 2, 0, hid->iface->desc.interfacenum, 1);
}


static inline int key_in_buf(uint8_t key, uint8_t *buf)
{
    return (key == buf[2] || key == buf[3] || key == buf[4] ||
            key == buf[5] || key == buf[6] || key == buf[7]);
}


static void usb_handle_kbd_input(void *__hid)
{
    // handle keyboard input
    // See: https://wiki.osdev.org/USB_Human_Interface_Devices

    struct usb_hid_dev_t *hid = __hid;
    volatile int unblock = 0;
    volatile int i;
    uint8_t key;

    // check for packets with errors
    for(i = 2; i < 8; i++)
    {
        if(hid->buf[i] == 1 || hid->buf[i] == 2 || hid->buf[i] == 3)
        {
            return;
        }
    }

    /*
    usb_printk("usb_handle_kbd_input: ");
    for(i = 0; i < 8; i++) usb_printk("%d ", hid->buf[i]);
    usb_printk("\n");
    */

#define BRK                     (KEYCODE_BREAK_MASK << 8)

#define BUFBIT(buf, bit)        (buf[0] & (1 << bit))

#define PROCESS_MODIFIER(bit, code)                             \
    if(BUFBIT(hid->buf, bit) != BUFBIT(hid->last_packet, bit)) {\
        kbdbuf_enqueue(&kbd_queue, code | (BUFBIT(hid->buf, bit) ? 0 : BRK));\
        unblock = 1;                                            \
    }

    // process CTRL, ALT, SHIFT
    // TODO: process the GUI/Windows keys
    PROCESS_MODIFIER(0, KEYCODE_LCTRL);
    PROCESS_MODIFIER(1, KEYCODE_LSHIFT);
    PROCESS_MODIFIER(2, KEYCODE_LALT);
    PROCESS_MODIFIER(4, KEYCODE_RCTRL);
    PROCESS_MODIFIER(5, KEYCODE_RSHIFT);
    PROCESS_MODIFIER(6, KEYCODE_RALT);

#undef PROCESS_MODIFIER
#undef BUFBIT

#define DONE()                  \
    unblock = 1;                \
    hid->last_key_pressed = 0;  \
    hid->last_key_counter = 0;

    // next, process key presses
    for(i = 2; i < 8; i++)
    {
        key = hid->buf[i];

        // scancodes < 3 are errors
        // See: https://aeb.win.tue.nl/linux/kbd/scancodes-14.html
        if(key > 3)
        {
            // check if the key was newly pressed
            if(!key_in_buf(key, hid->last_packet))
            {
                switch(usb_keycodes[key])
                {
                    // switch LEDs if needed
                    case KEYCODE_NUM:
                        toggle_led(hid, 0);
                        kbdbuf_enqueue(&kbd_queue, KEYCODE_NUM);
                        DONE();
                        break;

                    case KEYCODE_CAPS:
                        toggle_led(hid, 1);
                        kbdbuf_enqueue(&kbd_queue, KEYCODE_CAPS);
                        DONE();
                        break;

                    case KEYCODE_SCROLL:
                        toggle_led(hid, 2);
                        kbdbuf_enqueue(&kbd_queue, KEYCODE_SCROLL);
                        DONE();
                        break;

                    default:
                        if(usb_keycodes[key])
                        {
                            kbdbuf_enqueue(&kbd_queue, usb_keycodes[key]);
                            DONE();
                        }
                        break;
                }
            }
            else
            {
                hid->last_key_pressed = key;
                hid->last_key_counter++;
            }
        }
    }

#undef DONE

    for(i = 2; i < 8; i++)
    {
        key = hid->last_packet[i];

        // check for key releases
        if(key && !key_in_buf(key, hid->buf))
        {
            if(key == hid->last_key_pressed)
            {
                hid->last_key_pressed = 0;
                hid->last_key_counter = 0;
            }

            switch(usb_keycodes[key])
            {
                case KEYCODE_NUM:
                    kbdbuf_enqueue(&kbd_queue, KEYCODE_NUM | BRK);
                    unblock = 1;
                    break;

                case KEYCODE_CAPS:
                    kbdbuf_enqueue(&kbd_queue, KEYCODE_CAPS | BRK);
                    unblock = 1;
                    break;

                case KEYCODE_SCROLL:
                    kbdbuf_enqueue(&kbd_queue, KEYCODE_SCROLL | BRK);
                    unblock = 1;
                    break;

                default:
                    if(usb_keycodes[key])
                    {
                        unblock = 1;
                        kbdbuf_enqueue(&kbd_queue, usb_keycodes[key] | BRK);
                    }
                    break;
            }
        }
    }

#undef BRK

    for(i = 0; i < 8; i++)
    {
        hid->last_packet[i] = hid->buf[i];
    }

    // delay for a bit before sending repeat key presses
    if(hid->last_key_counter > 5 && usb_keycodes[hid->last_key_pressed])
    {
        unblock = 1;
        kbdbuf_enqueue(&kbd_queue, usb_keycodes[hid->last_key_pressed]);
    }

    if(unblock)
    {
        unblock_kernel_task(kbd_task);
    }
}


/*
static int hid_get_descriptor(struct usb_dev_t *usb,
                              uint8_t type, uint8_t index, uint16_t interface)
{
    struct usb_hid_descriptor_t desc;

    return usb_ctrl_in(usb, &desc, 0x81, 6, type, index, 
                        interface, sizeof(struct usb_hid_descriptor_t));
}
*/


void usb_hid_remove(struct usb_interface_t *iface)
{
    volatile struct usb_hid_dev_t *hid, *next, *prev = &hid_list;

    if(!iface)
    {
        return;
    }

    elevated_priority_lock(&usb_hid_tablock);

    for(hid = hid_list.next; hid != NULL; )
    {
        if(hid->iface == iface)
        {
            remove_interrupt_transfer((struct usb_transfer_t *)&hid->transfer);
            next = hid->next;
            prev->next = (struct usb_hid_dev_t *)next;
            kfree((void *)hid);
            hid = next;
        }
        else
        {
            prev = hid;
            hid = hid->next;
        }
    }

    elevated_priority_unlock(&usb_hid_tablock);
}

static int usb_hid_finish_register(struct usb_interface_t *iface,
                                   struct usb_hid_dev_t *hid,
                                   struct usb_endpoint_t *endpoint)
{
    elevated_priority_lock(&usb_hid_tablock);
    hid->iface = iface;
    hid->next = hid_list.next;
    hid_list.next = hid;
    elevated_priority_unlock(&usb_hid_tablock);

    if(iface->desc.protocol == 1)       // keyboard
    {
        usb_printk("usb: scheduling interrupt transfer for USB %s\n", "keyboard");
        usb_schedule_inttransfer(iface->usb, 
                                 (struct usb_endpoint_t *)endpoint, 
                                 &hid->transfer, hid->buf, 8,
                                 usb_handle_kbd_input, hid,
                                 endpoint->interval ? endpoint->interval : 10);
    }
    else if(iface->desc.protocol == 2 || hid->is_mouse)  // mouse/tablet
    {
        /* Use endpoint MPS for buffer size (typically 4–8 bytes).
         * The HID report size is determined by the Report Descriptor;
         * MPS is the max the endpoint will send in one packet. */
        int mps = hid->report_len ? hid->report_len : endpoint->mps;
        if (mps < 4) mps = 4;   /* boot mouse: 3 buttons + X + Y minimum */
        if (mps > 8) mps = 8;   /* cap to hid->buf size */
        usb_printk("usb: scheduling interrupt transfer for USB %s (mps=%d)\n",
                   hid->mouse_absolute ? "tablet" : "mouse", mps);
        usb_schedule_inttransfer(iface->usb,
                                 (struct usb_endpoint_t *)endpoint,
                                 &hid->transfer, hid->buf, mps,
                                 usb_handle_mouse_input, hid,
                                 endpoint->interval ? endpoint->interval : 10);
        /* Register with public mouse API */
        usb_mouse_init(iface->usb->bus, iface->usb->num,
                       iface->desc.interfacenum,
                       endpoint->addr, endpoint->mps);
        /* Expose for periodic poll */
        if (!g_first_mouse_hid)
            g_first_mouse_hid = hid;
    }
    else
    {
        usb_printk("usb: unkown HID protocol: %d\n", iface->desc.protocol);
    }

    usb_printk("usb: finished intializing HID device\n");

    return 0;
}

static struct usb_endpoint_t *usb_hid_find_interrupt_in(struct usb_interface_t *iface)
{
    struct usb_endpoint_t *endpoint;

    if (iface->endpoint_interrupt &&
        iface->endpoint_interrupt->direction == USB_ENDPOINT_IN)
        return iface->endpoint_interrupt;

    for (endpoint = iface->usb->endpoints; endpoint != NULL; endpoint = endpoint->next) {
        if (endpoint->type == USB_ENDPOINT_INTERRUPT &&
            endpoint->direction == USB_ENDPOINT_IN)
            return endpoint;
    }

    return NULL;
}

int usb_hid_register_async(struct usb_interface_t *iface,
                           uint8_t *rdesc, int rdesc_len)
{
    struct usb_hid_dev_t *hid;
    struct usb_endpoint_t *endpoint;

    if(!iface || !iface->usb || !iface->usb->endpoints)
        return -EINVAL;

    endpoint = usb_hid_find_interrupt_in(iface);
    if(!endpoint)
    {
        usb_printk("usb: HID has invalid IN endpoints\n");
        return -EINVAL;
    }

    if(!(hid = kmalloc(sizeof(struct usb_hid_dev_t))))
    {
        usb_printk("usb: insufficient memory to init HID device\n");
        return -ENOMEM;
    }

    A_memset(hid, 0, sizeof(struct usb_hid_dev_t));
    hid->iface = iface;

    usb_printk("usb: HID IFACE: num=%d class=0x%02x sub=%d proto=%d eps=%d\n",
        iface->desc.interfacenum,
        iface->desc.class,
        iface->desc.subclass,
        iface->desc.protocol,
        iface->desc.endpoints);
    usb_printk("usb: HID INT EP: addr=0x%02x mps=%d interval=%d type=%d dir=%s\n",
        endpoint->addr, endpoint->mps, endpoint->interval,
        endpoint->type,
        endpoint->direction == USB_ENDPOINT_IN ? "IN" : "OUT");

    if (rdesc && rdesc_len > 0)
        hid_parse_mouse_report_desc(hid, rdesc, rdesc_len);
    else
        usb_printk("usb: HID Report Descriptor unavailable, using boot defaults\n");

    return usb_hid_finish_register(iface, hid, endpoint);
}

int init_hid(struct usb_interface_t *iface)
{
    struct usb_hid_dev_t *hid;
    struct usb_endpoint_t *endpoint;

    if(!iface || !iface->usb || !iface->usb->endpoints)
    {
        return -EINVAL;
    }

    endpoint = usb_hid_find_interrupt_in(iface);
    if(!endpoint)
    {
        usb_printk("usb: HID has invalid IN endpoints\n");
        return -EINVAL;
    }

    if(!(hid = kmalloc(sizeof(struct usb_hid_dev_t))))
    {
        usb_printk("usb: insufficient memory to init HID device\n");
        return -ENOMEM;
    }
    
    A_memset(hid, 0, sizeof(struct usb_hid_dev_t));
    hid->iface = iface;

    usb_printk("usb: HID IFACE: num=%d class=0x%02x sub=%d proto=%d eps=%d\n",
        iface->desc.interfacenum,
        iface->desc.class,
        iface->desc.subclass,
        iface->desc.protocol,
        iface->desc.endpoints);

    usb_printk("usb: HID INT EP: addr=0x%02x mps=%d interval=%d type=%d dir=%s\n",
        endpoint->addr, endpoint->mps, endpoint->interval,
        endpoint->type,
        endpoint->direction == USB_ENDPOINT_IN ? "IN" : "OUT");

    /* ---- Read HID Report Descriptor ---- */
    {
        uint8_t rdesc[128];
        int rdesc_len = iface->hid_report_len;
        if (rdesc_len <= 0 || rdesc_len > (int)sizeof(rdesc)) {
            usb_printk("usb: invalid HID report length=%d, using %d\n",
                       rdesc_len, (int)sizeof(rdesc));
            rdesc_len = sizeof(rdesc);
        }
        A_memset(rdesc, 0, sizeof(rdesc));
        int got = usb_ctrl_in(iface->usb, rdesc,
                              0x81, 6,            /* GET_DESCRIPTOR, IN, Interface */
                              0x22, 0,            /* HID Report Descriptor */
                              iface->desc.interfacenum,
                              rdesc_len);
        if (got) {
            usb_printk("usb: HID Report Descriptor (%d bytes):\n", rdesc_len);
            usb_printk("usb: HID RD: ");
            for (int i = 0; i < rdesc_len; i++) {
                if (i > 0 && (i % 16) == 0)
                    usb_printk("\nusb: HID RD: ");
                usb_printk("%02x ", rdesc[i]);
            }
            usb_printk("\n");
            hid_parse_mouse_report_desc(hid, rdesc, rdesc_len);
        } else {
            usb_printk("usb: failed to read HID Report Descriptor\n");
        }
    }

    // set protocol (0=boot; 1=report). Only boot HID interfaces support it.
    if (iface->desc.subclass == HID_SUBCLASS_BOOT) {
        int sp_ret = usb_hid_set_protocol(iface->usb, 0,
                                          iface->desc.interfacenum);
        usb_printk("usb: SET_PROTOCOL(boot) iface=%d ret=%d\n",
                   iface->desc.interfacenum, sp_ret);
        if(!sp_ret)
        {
            usb_printk("usb: failed to set HID protocol\n");
            kfree(hid);
            return -EIO;
        }
    } else {
        usb_printk("usb: HID non-boot iface, keeping report protocol\n");
    }

    /* SET_IDLE: some HID devices STALL the interrupt endpoint
     * if idle rate is not set before first interrupt transfer. */
    {
        struct usb_transfer_t t;
        usb_setup_transfer(iface->usb, iface->usb->endpoints, &t,
                           USB_TRANSFER_CTRL);
        usb_setup_transaction(&t, 0x21, 0x0A, 0, 0,  /* SET_IDLE, duration=0 */
                              iface->desc.interfacenum, 0);
        usb_in_transaction(&t, 1, 0, 0);  /* Status Stage */
        usb_schedule_transfer(&t);
        usb_wait_transfer(&t);
        usb_printk("usb: SET_IDLE success=%d\n", t.success);
        usb_delete_transfer(&t);
    }

    return usb_hid_finish_register(iface, hid, endpoint);
}
