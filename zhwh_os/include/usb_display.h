#ifndef USB_DISPLAY_H
#define USB_DISPLAY_H
extern int g_usb_uhci_count;
extern int g_usb_ehci_count;
extern int g_usb_uhci2_count;
extern int g_usb_mouse_count;
extern int g_ehci_qh_caps;
extern int g_ehci_qh_caps2;
extern int g_ehci_mouse_port;
extern int g_ehci_port_owner;
extern int g_ehci_port_enabled;
extern int g_ehci_port_speed;
extern int g_ehci_fl_entry;      /* Frame List[0] */
extern int g_ehci_td0_token;     /* TD0 token */
extern unsigned g_ehci_portsc;   /* PORTSC full */
extern unsigned g_ehci_qh_ovl;   /* QH overlay token */
extern unsigned g_ehci_qh_nxt;   /* QH next_qTD */
extern unsigned g_ehci_qh_alt;   /* QH alt next_qTD */
extern unsigned g_ehci_td_nxt;   /* TD0 next ptr */
extern unsigned g_ehci_eecp;     /* EECP offset */
extern unsigned g_ehci_legsup;   /* USBLEGSUP value */
extern unsigned g_ehci_qh_b0;    /* QH overlay buf0 */
extern unsigned g_ehci_qh_cur;   /* QH overlay cur ptr */
#endif
