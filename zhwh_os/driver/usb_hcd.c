/**
 * @file usb_hcd.c
 * @brief USB Host Controller Driver (UHCI - USB 1.1)
 *
 * Implements UHCI (Universal Host Controller Interface)
 * This is the simpler USB 1.1 controller found in older systems
 */

#include <stdint.h>
#include "string.h"
#include "pci.h"
#include "printf.h"
#include "x86/io.h"
#include "page.h"
#include "usb.h"
#include "usb_hcd.h"

// 外部声明 DMA 分配函数
extern void *dma_alloc_coherent(size_t size, uint32_t *dma_handle);

#define ACTLEN_MASK   0x7FF
#define UHCI_LINK_QH  0x02


// UHCI I/O ports
#define UHCI_USBCMD        0x00  // USB Command register
#define UHCI_USBSTS        0x02  // USB Status register
#define UHCI_FRNUM        0x06  // Frame Number register
#define UHCI_USBLEGSUP     0x0C  // Legacy Support register
#define UHCI_FLBASEADD     0x08  // Frame List Base Address
#define UHCI_PORTSC1       0x10  // Port 1 Status/Control
#define UHCI_PORTSC2       0x12  // Port 2 Status/Control

// UHCI Register bits
#define UHCI_USBCMD_RUN            (1 << 0)  // Run/Stop
#define UHCI_USBCMD_HCRESET        (1 << 1)  // Host Controller Reset
#define UHCI_USBCMD_FLS            (1 << 2)  // Frame List Size

#define UHCI_USBSTS_USBINT         (1 << 0)  // Interrupt
#define UHCI_USBSTS_ERROR          (1 << 1)  // Error interrupt
#define UHCI_USBSTS_RD             (1 << 2)  // Resume Detect
#define UHCI_USBSTS_HSE            (1 << 3)  // Host System Error
#define UHCI_USBSTS_HCPE           (1 << 4)  // Host Controller Process Error

// UHCI Port Status/Control bits
#define UHCI_PORTSC_CCS            (1 << 0)  // Current Connect Status
#define UHCI_PORTSC_CSC            (1 << 1)  // Connect Status Change
#define UHCI_PORTSC_PED            (1 << 2)  // Port Enable/Disabled
#define UHCI_PORTSC_PEC            (1 << 3)  // Port Enable/Disable Change
#define UHCI_PORTSC_LSS            (1 << 7)  // Low Speed Status
#define UHCI_PORTSC_PR             (1 << 9)  // Port Reset

// Frame List Size (1024 entries for UHCI)
#define UHCI_FRAME_LIST_COUNT  1024

// Queue Head (QH) structure
typedef struct uhci_qh {
    uint32_t link_ptr;        // Next queue head or terminate
    uint32_t element_ptr;     // Next transfer descriptor or terminate
} __attribute__((packed)) uhci_qh_t;

// Transfer Descriptor (TD) structure
typedef struct uhci_td {
    uint32_t link_ptr;        // Next TD
    uint32_t ctrl_status;     // Control and status
    uint32_t token;           // Token
    uint32_t buffer;          // Buffer pointer
} __attribute__((packed)) uhci_td_t;

// TD Control/Status bits
#define UHCI_TD_CTRL_ACT           (1 << 23)  // Active
#define UHCI_TD_CTRL_IOC           (1 << 24)  // Interrupt on Complete
#define UHCI_TD_CTRL_IOS           (1 << 25)  // Isochronous Select
#define UHCI_TD_CTRL_LS            (1 << 26)  // Low Speed
#define UHCI_TD_CTRL_CERR_MASK     0x03       // Error Count bits
#define UHCI_TD_CTRL_CERR_SHIFT    25          // CORRECTED: CERR is at bits 27-25, not 27-29
#define UHCI_TD_CTRL_SPD           (1 << 29)  // Short Packet Detect
#define UHCI_TD_CTRL_ERR_MASK      0x03       // Error bits
#define UHCI_TD_CTRL_ERR_SHIFT     30

// TD Token bits
#define UHCI_TD_TOKEN_PID_MASK     0xFF       // PID
#define UHCI_TD_TOKEN_PID_SHIFT    0
#define UHCI_TD_TOKEN_DEVADDR_MASK 0x7F       // Device Address
#define UHCI_TD_TOKEN_DEVADDR_SHIFT 8
#define UHCI_TD_TOKEN_ENDPT_MASK   0x0F       // Endpoint
#define UHCI_TD_TOKEN_ENDPT_SHIFT  15
#define UHCI_TD_TOKEN_MAXLEN_MASK  0x7FF      // Max Length
#define UHCI_TD_TOKEN_MAXLEN_SHIFT 21

// PIDs
#define USB_PID_SETUP   0x2D
#define USB_PID_IN      0x69
#define USB_PID_OUT     0xE1

// 🔥 添加缺失的 TD Control 位定义
#ifndef UHCI_TD_CTRL_ACT
#define UHCI_TD_CTRL_ACT           (1 << 23)  // Active
#define UHCI_TD_CTRL_IOC           (1 << 24)  // Interrupt on Complete
#define UHCI_TD_CTRL_IOS           (1 << 25)  // Isochronous Select
#define UHCI_TD_CTRL_LS            (1 << 26)  // Low Speed
#define UHCI_TD_CTRL_CERR_MASK     0x03       // Error Count bits
#define UHCI_TD_CTRL_CERR_SHIFT    25          // CORRECTED: CERR is at bits 27-25, not 27-29
#define UHCI_TD_CTRL_SPD           (1 << 29)  // Short Packet Detect
#define UHCI_TD_CTRL_ERR_MASK      0x03       // Error bits
#define UHCI_TD_CTRL_ERR_SHIFT     30
#endif

// Link pointer terminate bit
#define UHCI_LINK_TERMINATE  0x01

// 🔥 Link pointer Depth-First bit (bit 2) - TD must use this!
#define UHCI_TD_LINK_DF 0x04

// 🔥 TD Token DATA Toggle bit
#define UHCI_TD_TOKEN_DATA_TOGGLE (1 << 19)
#define UHCI_TD_TOKEN_DATA_TOGGLE_SHIFT 19

// 🔥 QH Head Pointer Low Speed bit
#define UHCI_QH_HEAD_LS (1 << 26)

// UHCI Controller state
typedef struct uhci_controller {
    uint16_t base_io;        // Base I/O port
    uint16_t irq;            // IRQ number
    volatile uint32_t *frame_list;   // 🔥 Frame list (virtual address) - MUST be uint32_t array!
    uint32_t frame_list_phys; // Frame list (physical address)
    uhci_qh_t *qh_pool;     // Queue head pool (virtual address)
    uint32_t qh_pool_phys;   // QH pool (physical address)
    uhci_td_t *td_pool;      // Transfer descriptor pool (virtual address)
    uint32_t td_pool_phys;    // TD pool (physical address)
    int qh_next;             // Next free QH
    int td_next;             // Next free TD
    int initialized;         // Initialization flag
    int device_low_speed;    // 🔥 Port 0 device speed: 1=low speed, 0=full speed
    uhci_qh_t *intr_qh;      // 🔥 Persistent interrupt QH (virtual address)
    uint32_t intr_qh_phys;   // 🔥 Persistent interrupt QH (physical address)
    int intr_qh_active;      // 🔥 Is interrupt QH linked to frame list?
    uint8_t port_connected[2]; // 🔥 Track connection status for each port (0/1)
    uint8_t port_enabled[2];   // 🔥 Track enable status for each port (0/1)
} uhci_controller_t;

static uhci_controller_t uhci_controllers[USB_MAX_CONTROLLERS];
int num_uhci_controllers = 0;

/**
 * @brief Read UHCI register
 */
static inline uint16_t uhci_read_reg(uhci_controller_t *ctrl, uint16_t reg) {
    return inw(ctrl->base_io + reg);
}

/**
 * @brief Write UHCI register
 */
static inline void uhci_write_reg(uhci_controller_t *ctrl, uint16_t reg, uint16_t value) {
    outw(ctrl->base_io + reg, value);
}

/**
 * @brief Allocate a Queue Head
 */
static uhci_qh_t *uhci_alloc_qh(uhci_controller_t *ctrl) {
    if (ctrl->qh_next >= 256) {
        return NULL;  // Pool exhausted
    }
    uhci_qh_t *qh = &ctrl->qh_pool[ctrl->qh_next++];
    memset(qh, 0, sizeof(uhci_qh_t));
    qh->link_ptr = UHCI_LINK_TERMINATE;
    qh->element_ptr = UHCI_LINK_TERMINATE;
    return qh;
}

/**
 * @brief Allocate a Transfer Descriptor
 */
static uhci_td_t *uhci_alloc_td(uhci_controller_t *ctrl) {
    if (ctrl->td_next >= 1024) {
        return NULL;  // Pool exhausted
    }
    uhci_td_t *td = &ctrl->td_pool[ctrl->td_next++];
    memset(td, 0, sizeof(uhci_td_t));
    td->link_ptr = UHCI_LINK_TERMINATE;
    return td;
}

/**
 * @brief Reset UHCI controller
 */
static int uhci_reset(uhci_controller_t *ctrl) {
    printf("[USB] Resetting UHCI controller at I/O 0x%x\n", ctrl->base_io);

    // Set reset bit
    uint16_t cmd = uhci_read_reg(ctrl, UHCI_USBCMD);
    uhci_write_reg(ctrl, UHCI_USBCMD, cmd | UHCI_USBCMD_HCRESET);

    // Wait for reset to complete (max 100ms)
    for (int i = 0; i < 10000; i++) {
        cmd = uhci_read_reg(ctrl, UHCI_USBCMD);
        if (!(cmd & UHCI_USBCMD_HCRESET)) {
            printf("[USB] UHCI reset complete\n");
            return 0;
        }
        // Small delay
        for (volatile int j = 0; j < 1000; j++);
    }

    printf("[USB] ERROR: UHCI reset timeout\n");
    return -1;
}

/**
 * @brief Initialize UHCI controller
 */
static int uhci_init_controller(pci_dev_t *pci_dev, int controller_id) {
    if (controller_id >= USB_MAX_CONTROLLERS) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    memset(ctrl, 0, sizeof(uhci_controller_t));

    // Get I/O base from BAR4
    // PCI BAR format for I/O space: bit 0 = 1, bits [31:2] = base address
    // Example: 0x00000C01 → I/O base = 0x0C00
    extern uint32_t pci_read_config_dword(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
    uint32_t bar4 = pci_read_config_dword(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, PCI_BAR4);

    printf("[USB]   BAR4 raw value: 0x%08x\n", bar4);

    // Extract I/O base address (mask off bits [1:0])
    ctrl->base_io = bar4 & ~0x3;

    printf("[USB]   I/O base: 0x%x (from BAR4)\n", ctrl->base_io);

    // Get IRQ line from PCI configuration
    extern uint8_t pci_read_config_byte(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
    uint8_t irq_line = pci_read_config_byte(pci_dev->bus_id, pci_dev->dev_id, pci_dev->fn_id, PCI_INTERRUPT_LINE);
    ctrl->irq = irq_line;

    printf("[USB] Initializing UHCI controller %d\n", controller_id);
    printf("[USB]   I/O base: 0x%x, IRQ: %d\n", ctrl->base_io, ctrl->irq);
    printf("[USB]   Vendor:Device = 0x%x:0x%x\n",
           pci_dev->header.vendor_id, pci_dev->header.device_id);

    // Enable bus mastering and I/O space
    uint16_t pci_cmd = pci_read_config_word(pci_dev->bus_id, pci_dev->dev_id,
                                            pci_dev->fn_id, PCI_COMMAND);
    pci_cmd |= PCI_COMMAND_IO | PCI_COMMAND_MASTER;
    pci_write_config_word(pci_dev->bus_id, pci_dev->dev_id,
                          pci_dev->fn_id, PCI_COMMAND, pci_cmd);

    // Disable legacy support
    uhci_write_reg(ctrl, UHCI_USBLEGSUP, 0x8F00);

    // Reset controller
    if (uhci_reset(ctrl) != 0) {
        return -1;
    }

    // Allocate frame list (1024 entries, must be aligned on 4KB boundary)
    // Using DMA coherent allocator for proper physical/virtual mapping
    // Frame list: 1024 entries * 4 bytes = 4KB (page aligned)
    uint32_t frame_list_phys;
    ctrl->frame_list = (volatile uint32_t *)dma_alloc_coherent(4096, &frame_list_phys);
    ctrl->frame_list_phys = frame_list_phys;
    memset((void *)ctrl->frame_list, 0, 4096);
    printf("[USB] Frame list virt=0x%x phys=0x%x\n",
           (uint32_t)ctrl->frame_list, ctrl->frame_list_phys);

    // QH pool: 256 entries * 16 bytes = 4KB
    uint32_t qh_pool_phys;
    ctrl->qh_pool = (uhci_qh_t *)dma_alloc_coherent(4096, &qh_pool_phys);
    ctrl->qh_pool_phys = qh_pool_phys;
    memset(ctrl->qh_pool, 0, 4096);

    // TD pool: 1024 entries * 16 bytes = 16KB
    uint32_t td_pool_phys;
    ctrl->td_pool = (uhci_td_t *)dma_alloc_coherent(16384, &td_pool_phys);
    ctrl->td_pool_phys = td_pool_phys;
    memset(ctrl->td_pool, 0, 16384);

    printf("[USB] QH pool virt=0x%x phys=0x%x\n",
           (uint32_t)ctrl->qh_pool, ctrl->qh_pool_phys);
    printf("[USB] TD pool virt=0x%x phys=0x%x\n",
           (uint32_t)ctrl->td_pool, ctrl->td_pool_phys);

    uhci_qh_t *qh = uhci_alloc_qh(ctrl);
    if (!qh) {
        printf("[USB] ERROR: Failed to allocate QH\n");
        return -1;
    }

    // Set all frame list entries to point to the QH (with QH bit set)
    // 🔥 frame_list is now uint32_t array, not struct array!
    for (int i = 0; i < UHCI_FRAME_LIST_COUNT; i++) {
        // 计算 QH 的物理地址
        uint32_t qh_phys = ctrl->qh_pool_phys + ((uint32_t)qh - (uint32_t)ctrl->qh_pool);
        ctrl->frame_list[i] = qh_phys | 0x02;  // QH select (bit 1 = QH type)
    }

    // 🔥 Set frame list base address (32位物理地址）
    // UHCI FLBASEADD is 32-bit, MUST be written with outl (not two 16-bit writes)
    outl(ctrl->base_io + UHCI_FLBASEADD, ctrl->frame_list_phys);

    // 验证读取
    uint32_t flbase_readback = inl(ctrl->base_io + UHCI_FLBASEADD);
    printf("[USB]   Setting FLBASEADD=0x%x (readback=0x%x)\n",
           ctrl->frame_list_phys, flbase_readback);

    // 🔍 验证：打印帧列表的前几个 entry
    printf("[USB] Frame List verification (first 4 entries):\n");
    for (int i = 0; i < 4; i++) {
        printf("[USB]   entry %d: link=0x%x\n", i, ctrl->frame_list[i]);
    }
    printf("[USB]   Setting FLBASEADD low=0x%x high=0x%x (frame_list_phys=0x%x)\n",
           (uint16_t)(ctrl->frame_list_phys & 0xFFFF),
           (uint16_t)(ctrl->frame_list_phys >> 16),
           ctrl->frame_list_phys);

    // Clear status bits
    uhci_write_reg(ctrl, UHCI_USBSTS, 0x1F);

    // 🔥 使用IOAPIC启用UHCI中断（像键盘中断一样）
    extern void ioapicenable(int irq, int cpunum);
    printf("[USB] Enabling IRQ %d for UHCI controller via IOAPIC\n", ctrl->irq);
    ioapicenable(ctrl->irq, 0);  // 路由到CPU 0

    // Start controller
    uint16_t cmd = uhci_read_reg(ctrl, UHCI_USBCMD);
    cmd |= UHCI_USBCMD_RUN;
    cmd &= ~UHCI_USBCMD_FLS;  // 1024 frame list
    uhci_write_reg(ctrl, UHCI_USBCMD, cmd);

    // 🔥 验证控制器是否正在运行
    printf("[USB]   Started controller, cmd=0x%x\n", cmd);

    ctrl->initialized = 1;
    num_uhci_controllers++;

    // 🔥 Allocate persistent interrupt QH
    ctrl->intr_qh = uhci_alloc_qh(ctrl);
    if (!ctrl->intr_qh) {
        printf("[USB] ERROR: Failed to allocate interrupt QH\n");
        return -1;
    }

    // Calculate physical address
    ctrl->intr_qh_phys = ctrl->qh_pool_phys + ((uint32_t)ctrl->intr_qh - (uint32_t)ctrl->qh_pool);

    // Initialize QH (but don't link to frame list yet - wait for first interrupt transfer)
    ctrl->intr_qh->link_ptr = UHCI_LINK_TERMINATE;
    ctrl->intr_qh->element_ptr = UHCI_LINK_TERMINATE;
    ctrl->intr_qh_active = 0;

    // Initialize port status tracking for hot-plug detection
    ctrl->port_connected[0] = 0;
    ctrl->port_connected[1] = 0;
    ctrl->port_enabled[0] = 0;
    ctrl->port_enabled[1] = 0;

    printf("[USB] Interrupt QH allocated: virt=0x%x phys=0x%x\n",
           (uint32_t)ctrl->intr_qh, ctrl->intr_qh_phys);

    printf("[USB] UHCI controller initialized successfully\n");
    return 0;
}

/**
 * @brief Scan PCI bus for UHCI controllers
 */
int usb_hcd_init(void) {
    printf("[USB] Scanning for USB controllers...\n");

    pci_dev_t **devices = pci_get_devices();
    if (!devices) {
        printf("[USB] ERROR: No PCI devices found\n");
        return -1;
    }

    // 调试：列出所有 PCI 设备的 class 信息
    printf("[USB] PCI devices (looking for Class=0x0C, Subclass=0x03):\n");
    for (int i = 0; devices[i] != NULL; i++) {
        pci_dev_t *dev = devices[i];
        // 直接读取 PCI 配置空间的 Class Code (偏移 0x0A-0x0C)
        // 这样可以避免结构体对齐问题
        extern uint8_t pci_read_config_byte(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
        uint8_t class = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x0B);
        uint8_t subclass = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x0A);
        uint8_t prog_if = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x09);

        printf("[USB]   Device %d: Class=0x%02x, Subclass=0x%02x, ProgIF=0x%02x\n",
               i, (unsigned)class, (unsigned)subclass, (unsigned)prog_if);
    }

    int controller_id = 0;

    // UHCI controllers have class 0x0C (Serial Bus), subclass 0x03 (USB), prog if 0x00
    for (int i = 0; devices[i] != NULL; i++) {
        pci_dev_t *dev = devices[i];

        // 直接读取 PCI 配置空间，避免结构体问题
        extern uint8_t pci_read_config_byte(unsigned bus, unsigned dev, unsigned fn, unsigned reg);
        uint8_t class = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x0B);
        uint8_t subclass = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x0A);
        uint8_t prog_if = pci_read_config_byte(dev->bus_id, dev->dev_id, dev->fn_id, 0x09);

        printf("[USB] Checking device %d: 0x%02x:0x%02x:0x%02x\n",
               i, (unsigned)class, (unsigned)subclass, (unsigned)prog_if);

        if (class == 0x0C && subclass == 0x03 && prog_if == 0x00) {

            printf("[USB] Found UHCI controller\n");
            pci_print_device(dev);

            if (uhci_init_controller(dev, controller_id) == 0) {
                controller_id++;
            }
        }
    }

    if (num_uhci_controllers == 0) {
        printf("[USB] WARNING: No USB controllers found\n");
        printf("[USB] HINT: QEMU needs '-device piix3-usb-uhci' parameter\n");
        return -1;
    }

    printf("[USB] Found %d USB controller(s)\n", num_uhci_controllers);
    return 0;
}

/**
 * @brief UHCI IRQ handler
 *
 * Called when UHCI controller generates an interrupt (USBINT or ERROR)
 * Processes completed transfer descriptors, especially for periodic mouse transfers
 */
void uhci_irq_handler(void) {
    // Find the UHCI controller (assuming only one for now)
    if (num_uhci_controllers == 0) {
        return;
    }

    uhci_controller_t *ctrl = &uhci_controllers[0];

    // Read status register
    uint16_t status = uhci_read_reg(ctrl, UHCI_USBSTS);

    // Check if interrupt is from this controller
    if (!(status & (UHCI_USBSTS_USBINT | UHCI_USBSTS_ERROR))) {
        return;  // Not our interrupt
    }

    // Clear the interrupt by writing to status register
    uhci_write_reg(ctrl, UHCI_USBSTS, status);

    // Process completed periodic transfers (mouse data)
    // The interrupt QH's TD should have completed
    if (ctrl->intr_qh_active && ctrl->intr_qh) {
        // The periodic polling TD is linked to intr_qh
        // Check if there's an active TD that completed
        uint32_t element_ptr = ctrl->intr_qh->element_ptr;

        // If element_ptr is not TERMINATE and not pointing to a QH, it's a TD
        if (!(element_ptr & UHCI_LINK_TERMINATE) && !(element_ptr & 0x02)) {
            uint32_t td_phys = element_ptr & ~0x0F;
            uint32_t td_offset = td_phys - ctrl->td_pool_phys;
            uhci_td_t *td = (uhci_td_t *)((uint32_t)ctrl->td_pool + td_offset);

            // Check if TD completed (Active bit cleared)
            if (!(td->ctrl_status & UHCI_TD_CTRL_ACT)) {
                uint32_t errors = (td->ctrl_status >> UHCI_TD_CTRL_ERR_SHIFT) & UHCI_TD_CTRL_ERR_MASK;

                if (errors == 0) {
                    // Success - read actual length transferred
                    uint32_t actlen = (td->ctrl_status & ACTLEN_MASK) + 1;

                    // Debug: show we got data
                    static int irq_count = 0;
                    if (++irq_count <= 20) {
                        printf("[USB IRQ] Transfer complete: %d bytes, ctrl_status=0x%x\n",
                               actlen, td->ctrl_status);
                    }

                    // Note: The actual data processing happens in usb_mouse_periodic_poll()
                    // which is called from user space or main loop
                    // We just ensure the TD is marked for re-arm there
                } else {
                    printf("[USB IRQ] Transfer error: errors=0x%x, ctrl_status=0x%x\n",
                           errors, td->ctrl_status);
                }
            }
        }
    }

    // Check for error conditions
    if (status & UHCI_USBSTS_ERROR) {
        printf("[USB IRQ] Error status: 0x%x\n", status);
    }

    // Check for Host System Error
    if (status & UHCI_USBSTS_HSE) {
        printf("[USB IRQ] Host System Error - PCI error occurred\n");
    }

    // Check for Process Error
    if (status & UHCI_USBSTS_HCPE) {
        printf("[USB IRQ] Host Controller Process Error\n");
    }
}

/**
 * @brief Read UHCI port status
 */
static uint16_t uhci_read_port_status(uhci_controller_t *ctrl, int port) {
    if (port < 0 || port > 1) return 0;
    uint16_t reg = UHCI_PORTSC1 + (port * 2);
    return inw(ctrl->base_io + reg);
}

/**
 * @brief Write UHCI port status
 */
static void uhci_write_port_status(uhci_controller_t *ctrl, int port, uint16_t val) {
    if (port < 0 || port > 1) return;
    uint16_t reg = UHCI_PORTSC1 + (port * 2);
    outw(ctrl->base_io + reg, val);
}

/**
 * @brief Check if device is connected to port
 */
static int uhci_is_device_connected(uhci_controller_t *ctrl, int port) {
    uint16_t status = uhci_read_port_status(ctrl, port);
    return (status & UHCI_PORTSC_CCS) ? 1 : 0;
}

/**
 * @brief Reset USB port
 */
static int uhci_reset_port(uhci_controller_t *ctrl, int port) {
    printf("[USB] Resetting port %d\n", port);

    // Set reset bit
    uint16_t status = uhci_read_port_status(ctrl, port);
    printf("[USB]   Initial port status: 0x%x\n", status);

    uhci_write_port_status(ctrl, port, status | UHCI_PORTSC_PR);

    // Wait for reset to complete (typically 50ms, use longer for safety)
    for (volatile int i = 0; i < 1000000; i++);  // ~100ms

    // Clear reset bit
    status = uhci_read_port_status(ctrl, port);
    status = status & ~UHCI_PORTSC_PR;
    uhci_write_port_status(ctrl, port, status);

    // Wait a bit for the port to settle
    for (volatile int i = 0; i < 100000; i++);

    // Read back to confirm
    status = uhci_read_port_status(ctrl, port);
    printf("[USB]   Port status after reset: 0x%x\n", status);

    // Check if port is enabled
    if (status & UHCI_PORTSC_PED) {
        printf("[USB] Port %d enabled\n", port);
        return 0;
    }

    // Try to manually enable the port
    printf("[USB]   Trying to manually enable port...\n");
    status = uhci_read_port_status(ctrl, port);
    status |= UHCI_PORTSC_PED;  // Set enable bit
    uhci_write_port_status(ctrl, port, status);

    // Wait and check again
    for (volatile int i = 0; i < 100000; i++);
    status = uhci_read_port_status(ctrl, port);
    printf("[USB]   Port status after manual enable: 0x%x\n", status);

    // 🔥 检测设备速度 (LSS bit)
    int is_low_speed = (status & UHCI_PORTSC_LSS) ? 1 : 0;
    ctrl->device_low_speed = is_low_speed;
    printf("[USB]   Device speed: %s\n", is_low_speed ? "LOW SPEED" : "FULL SPEED");

    if (status & UHCI_PORTSC_PED) {
        printf("[USB] Port %d enabled (manual)\n", port);
        return 0;
    }

    printf("[USB] Port %d not enabled after reset\n", port);
    printf("[USB]   CCS=%d, PED=%d, PR=%d\n",
           (status & UHCI_PORTSC_CCS) ? 1 : 0,
           (status & UHCI_PORTSC_PED) ? 1 : 0,
           (status & UHCI_PORTSC_PR) ? 1 : 0);
    return -1;
}

/**
 * @brief Scan root hub for devices
 */
int usb_hcd_scan_ports(int controller_id) {
    if (controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    printf("[USB] Scanning root hub ports...\n");

    int devices_found = 0;

    // UHCI has 2 root ports
    for (int port = 0; port < 2; port++) {
        printf("[USB] Checking port %d...\n", port);

        if (!uhci_is_device_connected(ctrl, port)) {
            printf("[USB] Port %d: No device connected\n", port);
            continue;
        }

        printf("[USB] Port %d: Device connected!\n", port);

        // Reset the port
        if (uhci_reset_port(ctrl, port) != 0) {
            printf("[USB] WARNING: Port %d reset failed\n", port);
            continue;
        }

        devices_found++;
    }

    return devices_found;
}

/**
 * @brief Control transfer implementation
 *
 * USB 控制传输包含三个阶段：
 * 1. SETUP 阶段 - 发送 8 字节的 setup 包
 * 2. DATA 阶段 - 读取/写入数据（如果 wLength > 0）
 * 3. STATUS 阶段 - 完成传输（IN: 发送 OUT 包，OUT: 接收 IN 包）
 */
int usb_control_transfer(int controller_id, uint8_t dev_addr, uint8_t ep,
                         usb_device_request_t *req, void *data) {
    if (controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    printf("[USB] Control transfer: dev=%d, req=0x%x, wValue=0x%x wLen=%d\n",
           dev_addr, req->bRequest, req->wValue, req->wLength);

    // 🔥 确定传输方向和速度
    int is_in_transfer = (req->bmRequestType & 0x80) ? 1 : 0;
    int is_low_speed = ctrl->device_low_speed;
    printf("[USB]   Direction: %s, Speed: %s\n",
           is_in_transfer ? "IN (device->host)" : "OUT (host->device)",
           is_low_speed ? "LOW" : "FULL");

    // 🔥 分配 DMA 缓冲区用于 SETUP 数据包（8 字节）
    uint32_t setup_dma;
    uint8_t *setup_data = (uint8_t *)dma_alloc_coherent(8, &setup_dma);
    if (!setup_data) {
        printf("[USB] ERROR: Failed to allocate SETUP buffer\n");
        return -1;
    }

    // 复制请求数据到 SETUP 缓冲区
    memcpy(setup_data, req, 8);

    // 🔥 分配 DATA 阶段的 DMA 缓冲区（如果需要）
    uint32_t data_dma = 0;
    uint8_t *dma_buffer = NULL;
    if (req->wLength > 0) {
        dma_buffer = (uint8_t *)dma_alloc_coherent(req->wLength, &data_dma);
        if (!dma_buffer) {
            printf("[USB] ERROR: Failed to allocate DATA buffer\n");
            // 释放 SETUP 缓冲区
            // (TODO: 需要 dma_free_coherent，但暂时跳过)
            return -1;
        }
        if (!is_in_transfer) {
            // OUT 传输：复制数据到 DMA 缓冲区
            memcpy(dma_buffer, data, req->wLength);
        }
    }

    // 🔥 根据设备速度确定最大包大小
    // USB 1.1 规范：低速控制端点 max packet = 8，全速 = 64
    int max_packet_size = is_low_speed ? 8 : 64;

    // 计算需要多少个 DATA TD
    int data_td_count = 0;
    if (req->wLength > 0) {
        data_td_count = (req->wLength + max_packet_size - 1) / max_packet_size;
        if (data_td_count == 0) data_td_count = 1;
    }

    // 分配 TD：SETUP + DATA TDs + STATUS
    uhci_td_t *td_setup = uhci_alloc_td(ctrl);
    uhci_td_t *td_data_first = NULL;
    uhci_td_t *td_data_prev = NULL;
    uhci_td_t *td_status = uhci_alloc_td(ctrl);

    if (!td_setup || !td_status) {
        printf("[USB] ERROR: Failed to allocate SETUP/STATUS TDs\n");
        return -1;
    }

    // 分配并链接所有 DATA TD
    for (int i = 0; i < data_td_count; i++) {
        uhci_td_t *td = uhci_alloc_td(ctrl);
        if (!td) {
            printf("[USB] ERROR: Failed to allocate DATA TD %d\n", i);
            return -1;
        }
        if (i == 0) {
            td_data_first = td;
        }
        if (td_data_prev) {
            // 链接前一个 TD 到这个 TD
            uint32_t td_prev_phys = ctrl->td_pool_phys + ((uint32_t)td_data_prev - (uint32_t)ctrl->td_pool);
            uint32_t td_curr_phys = ctrl->td_pool_phys + ((uint32_t)td - (uint32_t)ctrl->td_pool);
            td_data_prev->link_ptr = td_curr_phys | UHCI_TD_LINK_DF;
        }
        td_data_prev = td;
    }

    // ========== SETUP TD ==========
    // PID=SETUP (0x2D), Device Address, Endpoint 0, Length=7 (8-1)
    td_setup->token = (USB_PID_SETUP << UHCI_TD_TOKEN_PID_SHIFT) |
                     (dev_addr << UHCI_TD_TOKEN_DEVADDR_SHIFT) |
                     (0 << UHCI_TD_TOKEN_ENDPT_SHIFT) |
                     (7 << UHCI_TD_TOKEN_MAXLEN_SHIFT);  // 8 bytes, encoded as 7

    td_setup->buffer = setup_dma;
    // 🔥 Active + Error Counter (3) + 根据设备速度设置 LS 位
    uint32_t td_speed_bits = is_low_speed ? UHCI_TD_CTRL_LS : 0;
    td_setup->ctrl_status = UHCI_TD_CTRL_ACT | td_speed_bits | (3 << UHCI_TD_CTRL_CERR_SHIFT);

    printf("[USB]   SETUP TD: virt=0x%x token=0x%x ctrl_status=0x%x buffer=0x%x\n",
           (uint32_t)td_setup, td_setup->token, td_setup->ctrl_status, td_setup->buffer);

    // ========== DATA TDs（如果需要） ==========
    // 构造每个 DATA TD，每个 max_packet_size 字节
    int remaining = req->wLength;
    uint32_t data_offset = 0;
    uhci_td_t *td_data = td_data_first;
    int toggle = 1;  // DATA 阶段 toggle 从 1 开始
    int td_index = 0;

    while (td_data && remaining > 0) {
        int td_len = (remaining > max_packet_size) ? max_packet_size : remaining;

        uint32_t data_pid = is_in_transfer ? USB_PID_IN : USB_PID_OUT;
        td_data->token = (data_pid << UHCI_TD_TOKEN_PID_SHIFT) |
                        (dev_addr << UHCI_TD_TOKEN_DEVADDR_SHIFT) |
                        (0 << UHCI_TD_TOKEN_ENDPT_SHIFT) |
                        ((td_len - 1) << UHCI_TD_TOKEN_MAXLEN_SHIFT);  // 长度编码为 N-1

        td_data->buffer = data_dma + data_offset;
        td_data->ctrl_status = UHCI_TD_CTRL_ACT | td_speed_bits | (3 << UHCI_TD_CTRL_CERR_SHIFT);

        // 🔥 DATA 阶段需要设置 toggle 位，每个 TD 交替
        if (toggle) {
            td_data->token |= UHCI_TD_TOKEN_DATA_TOGGLE;
        }
        toggle ^= 1;  // 下一个 TD 反转 toggle

        printf("[USB]   DATA TD %d: virt=0x%x len=%d token=0x%x ctrl_status=0x%x buffer=0x%x\n",
               td_index, (uint32_t)td_data, td_len, td_data->token, td_data->ctrl_status, td_data->buffer);

        remaining -= td_len;
        data_offset += td_len;
        td_index++;

        // 通过 link_ptr 找到下一个 TD（因为我们已经链接好了）
        uint32_t next_td_phys = td_data->link_ptr & ~0x0F;  // 清除低 4 位（T 和 DF 标志）
        if (next_td_phys != 0) {
            td_data = (uhci_td_t *)((uint32_t)ctrl->td_pool + (next_td_phys - ctrl->td_pool_phys));
        } else {
            td_data = NULL;
        }
    }

    // ========== STATUS TD ==========
    // STATUS 阶段方向相反：IN 传输用 OUT PID，OUT 传输用 IN PID
    uint32_t status_pid = is_in_transfer ? USB_PID_OUT : USB_PID_IN;
    td_status->token = (status_pid << UHCI_TD_TOKEN_PID_SHIFT) |
                      (dev_addr << UHCI_TD_TOKEN_DEVADDR_SHIFT) |
                      (0 << UHCI_TD_TOKEN_ENDPT_SHIFT) |
                      (0x7FF << UHCI_TD_TOKEN_MAXLEN_SHIFT);  // 🔥 STATUS 是 zero-length packet！

    td_status->buffer = 0;  // STATUS 阶段没有数据缓冲区
    // 🔥 Active + Error Counter (3) + 根据设备速度设置 LS 位 + Interrupt on Complete (为了检测完成)
    td_status->ctrl_status = UHCI_TD_CTRL_ACT | td_speed_bits | UHCI_TD_CTRL_IOC | (3 << UHCI_TD_CTRL_CERR_SHIFT);
    // 🔥 STATUS 阶段需要设置 toggle = 1
    td_status->token |= UHCI_TD_TOKEN_DATA_TOGGLE;

    printf("[USB]   STATUS TD: virt=0x%x token=0x%x ctrl_status=0x%x\n",
           (uint32_t)td_status, td_status->token, td_status->ctrl_status);

    // ========== 链接 TD 链 ==========
    // SETUP -> DATA(s) -> STATUS
    uint32_t td_setup_phys = ctrl->td_pool_phys + ((uint32_t)td_setup - (uint32_t)ctrl->td_pool);
    uint32_t td_status_phys = ctrl->td_pool_phys + ((uint32_t)td_status - (uint32_t)ctrl->td_pool);

    // 🔥 将最后一个 DATA TD 链接到 STATUS TD
    if (td_data_first && td_data_prev) {
        uint32_t last_data_phys = ctrl->td_pool_phys + ((uint32_t)td_data_prev - (uint32_t)ctrl->td_pool);
        td_data_prev->link_ptr = td_status_phys | UHCI_TD_LINK_DF;
        td_setup->link_ptr = (ctrl->td_pool_phys + ((uint32_t)td_data_first - (uint32_t)ctrl->td_pool)) | UHCI_TD_LINK_DF;
        printf("[USB]   TD chain: SETUP(0x%x) -> DATA(s) -> STATUS(0x%x)\n",
               td_setup_phys, td_status_phys);
    } else {
        // 没有 DATA 阶段，SETUP 直接链接到 STATUS
        td_setup->link_ptr = td_status_phys | UHCI_TD_LINK_DF;
        printf("[USB]   TD chain: SETUP(0x%x) -> STATUS(0x%x)\n",
               td_setup_phys, td_status_phys);
    }
    td_status->link_ptr = UHCI_LINK_TERMINATE;  // 🔥 使用正确的终止值

    // ========== 将 TD 链接到异步 QH ==========
    // 🔥 frame_list is uint32_t array, entry contains QH pointer directly
    uint32_t first_frame_ptr = ctrl->frame_list[0];
    uint32_t async_qh_phys = first_frame_ptr & ~0x03;  // Clear low 2 bits to get pure address

    if (async_qh_phys == 0) {
        printf("[USB] ERROR: No async QH found!\n");
        return -1;
    }

    // 转换为虚拟地址以便操作
    uint32_t qh_pool_offset = async_qh_phys - ctrl->qh_pool_phys;
    uhci_qh_t *async_qh = (uhci_qh_t *)((uint32_t)ctrl->qh_pool + qh_pool_offset);

    // 保存 QH 原来的 element_ptr
    uint32_t old_element = async_qh->element_ptr;

    // 🔥 对于低速设备，QH 的 head_ptr（link_ptr）需要设置 LS 位
    // 注意：根据 UHCI 规范，LS 位在 QH 的第一个 DWORD（head_ptr / link_ptr）的 bit 26
    if (is_low_speed) {
        // 直接 OR LS 位，保留原有地址和低 2 位（terminate 和 QH type）
        async_qh->link_ptr |= UHCI_QH_HEAD_LS;
        printf("[USB]   Setting QH LS bit for low speed device (link_ptr=0x%x)\n", async_qh->link_ptr);
    }

    // 将 SETUP TD 插入到 QH 的队列头部（使用物理地址）
    async_qh->element_ptr = td_setup_phys;

    printf("[USB]   TD chain linked to async QH (phys=0x%x)\n", async_qh_phys);

    // ========== 等待传输完成 ==========
    printf("[USB]   Waiting for transfer to complete...\n");

    // 简单的轮询等待（最多 100ms）
    int timeout = 100000;
    int completed = 0;

    while (timeout > 0) {
        // 检查 STATUS TD 的 Active 位
        if (!(td_status->ctrl_status & UHCI_TD_CTRL_ACT)) {
            // TD 不再 active - 检查是否成功
            uint32_t errors = (td_status->ctrl_status >> UHCI_TD_CTRL_ERR_SHIFT) & UHCI_TD_CTRL_ERR_MASK;

            if (errors == 0) {
                completed = 1;
                printf("[USB]   Transfer completed successfully!\n");
                break;
            } else {
                printf("[USB] ERROR: Transfer failed, errors=0x%x ctrl_status=0x%x\n",
                       errors, td_status->ctrl_status);
                break;
            }
        }
        timeout--;
        for (volatile int i = 0; i < 10; i++);  // 小延迟
    }

    // 恢复 QH 的原 element_ptr
    async_qh->element_ptr = old_element;

    if (!completed) {
        // 🔍 超时后打印状态
        uint16_t usbsts = uhci_read_reg(ctrl, UHCI_USBSTS);
        uint16_t usbcmd = uhci_read_reg(ctrl, UHCI_USBCMD);
        printf("[USB] ERROR: Transfer timeout! USBSTS=0x%x USBCMD=0x%x FRNUM=0x%x\n",
               usbsts, usbcmd, uhci_read_reg(ctrl, UHCI_FRNUM));
        return -1;
    }

    // 如果是 IN 传输，复制数据回用户缓冲区
    if (is_in_transfer && dma_buffer && req->wLength > 0) {
        memcpy(data, dma_buffer, req->wLength);
        printf("[USB]   Copied %d bytes to user buffer\n", req->wLength);
    }

    // 注意：DMA 缓冲区没有释放（需要 dma_free_coherent）
    // TODO: 实现 dma_free_coherent

    return req->wLength;  // 返回请求的长度
}

/**
 * @brief Interrupt transfer state for polling
 */
static struct {
    int active;              // Is a transfer in progress?
    uint8_t *buffer;         // Data buffer (user pointer)
    uint8_t *dma_buffer;      // 🔥 DMA 缓冲区指针
    int max_length;          // Max length to read
    int bytes_transferred;   // Bytes actually transferred
    uhci_td_t *td;           // Transfer descriptor
    uhci_qh_t *qh;           // Queue Head for this transfer
    uhci_qh_t *intr_qh;       // 🔥 固定的 QH（用于中断端点，避免每次分配）
    int intr_qh_initialized;     // 🔥 标记 intr_qh 是否已初始化
} intr_state;

/**
 * @brief Perform USB interrupt IN transfer
 * 
 * For interrupt transfers (like reading mouse data), we need to:
 * 1. Create an IN TD with the device address, endpoint, and max length
 * 2. Add it to the frame list (or an async QH)
 * 3. Wait for the TD to complete
 * 4. Read back the data
 * 
 * Simplified implementation: We'll use a polling approach where the
 * caller repeatedly checks if the transfer is complete.
 */
int usb_interrupt_transfer(int controller_id, uint8_t dev_addr, uint8_t ep,
                          void *data, int len) {
    if (controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    if (!data || len <= 0 || len > 64) {
        return -1;  // Invalid parameters (interrupt transfers max 64 bytes for low-speed)
    }

    int is_low_speed = ctrl->device_low_speed;

    printf("[USB] Interrupt transfer: dev=%d, ep=%d, len=%d\n",
           dev_addr, ep, len);

    // 🔥 Allocate DMA buffer for interrupt transfer
    uint32_t data_dma;
    uint8_t *dma_buffer = (uint8_t *)dma_alloc_coherent(len, &data_dma);
    if (!dma_buffer) {
        printf("[USB] ERROR: Failed to allocate interrupt DMA buffer\n");
        return -1;
    }
    memset(dma_buffer, 0, len);  // Clear buffer

    // 🔥 Allocate a TD for this interrupt transfer
    // Each transfer needs a new TD, but we reuse the persistent QH
    uhci_td_t *td = uhci_alloc_td(ctrl);
    if (!td) {
        printf("[USB] ERROR: Failed to allocate TD\n");
        return -1;
    }

    // 🔥 Construct TD for IN transfer
    // Token bits: PID=IN (0x69), Device Address, Endpoint, Max Length
    uint32_t td_speed_bits = is_low_speed ? UHCI_TD_CTRL_LS : 0;

    td->token = (USB_PID_IN << UHCI_TD_TOKEN_PID_SHIFT) |
                (dev_addr << UHCI_TD_TOKEN_DEVADDR_SHIFT) |
                (ep << UHCI_TD_TOKEN_ENDPT_SHIFT) |
                ((len - 1) << UHCI_TD_TOKEN_MAXLEN_SHIFT) |
                UHCI_TD_TOKEN_DATA_TOGGLE;  // 🔥 Interrupt endpoints start with toggle=1

    td->buffer = data_dma;
    td->ctrl_status = UHCI_TD_CTRL_ACT | td_speed_bits | UHCI_TD_CTRL_IOC | (3 << UHCI_TD_CTRL_CERR_SHIFT);
    td->link_ptr = UHCI_LINK_TERMINATE;

    printf("[USB]   TD allocated at virt=0x%x phys=0x%x token=0x%x ctrl_status=0x%x\n",
           (uint32_t)td,
           ctrl->td_pool_phys + ((uint32_t)td - (uint32_t)ctrl->td_pool),
           td->token, td->ctrl_status);

    // 🔥 Use the persistent interrupt QH (don't allocate a new one each time!)
    uhci_qh_t *intr_qh = ctrl->intr_qh;
    uint32_t intr_qh_phys = ctrl->intr_qh_phys;

    // Calculate TD physical address
    uint32_t td_phys = ctrl->td_pool_phys + ((uint32_t)td - (uint32_t)ctrl->td_pool);

    // Attach TD to QH
    intr_qh->element_ptr = td_phys;

    // 🔥 Link QH to frame list ONLY on first interrupt transfer
    // This creates a persistent periodic schedule
    if (!ctrl->intr_qh_active) {
        // Link QH every 10 frames (10ms polling interval for mouse)
        for (int i = 0; i < 1024; i += 10) {
            ctrl->frame_list[i] = intr_qh_phys | 0x02;  // 0x02 = QH type
        }
        // uint32_t old = ctrl->frame_list[0];
        // qh->link_ptr = old | UHCI_LINK_QH;

        // for (int i = 0; i < 1024; i += 10) {
        //     ctrl->frame_list[i] = qh_phys | UHCI_LINK_QH;
        // }

        ctrl->intr_qh_active = 1;

        printf("[USB]   Interrupt QH (phys=0x%x) linked to frame_list (every 10 frames)\n", intr_qh_phys);
    }

    // 🔥 Wait for transfer completion (synchronous wait mode)
    int timeout = 100000;
    int completed = 0;

    while (timeout > 0) {
        // Check TD Active bit
        if (!(td->ctrl_status & UHCI_TD_CTRL_ACT)) {
            // TD is no longer active - check if successful
            uint32_t errors = (td->ctrl_status >> UHCI_TD_CTRL_ERR_SHIFT) & UHCI_TD_CTRL_ERR_MASK;

            if (errors == 0) {
                completed = 1;
                printf("[USB]   Interrupt TD completed, status=0x%x\n", td->ctrl_status);
                break;
            } else {
                printf("[USB] ERROR: Interrupt TD failed, errors=0x%x ctrl_status=0x%x\n",
                       errors, td->ctrl_status);
                break;
            }
        }
        timeout--;
        for (volatile int i = 0; i < 10; i++);  // Small delay
    }

    if (!completed) {
        printf("[USB] ERROR: Interrupt transfer timeout!\n");
        return -1;
    }

    // 🔥 Copy data back to user buffer
    memcpy(data, dma_buffer, len);

    printf("[USB]   Data: %02x %02x %02x\n",
           ((uint8_t*)data)[0], ((uint8_t*)data)[1], ((uint8_t*)data)[2]);

    return len;
}

/**
 * @brief Poll for interrupt transfer completion
 * 
 * This function checks if the previously initiated interrupt transfer
 * has completed. For the simplified version, we'll return success immediately
 * and let the upper layers handle timing.
 * 
 * @return 1 if completed with data, 0 if still pending, negative on error
 */
int usb_interrupt_poll(int controller_id) {
    if (controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    if (!intr_state.active) {
        return 0;  // No transfer in progress
    }

    // 🔥 调试：读取 UHCI 状态寄存器
    uint16_t usbsts = uhci_read_reg(ctrl, UHCI_USBSTS);

    // 🔥 调试：每 100 次打印一次 TD 状态
    static int poll_count = 0;
    poll_count++;

    // Check if TD has completed (by checking if Active bit is cleared)
    uint32_t ctrl_status = intr_state.td->ctrl_status;

    if (poll_count % 100 == 0 || !(ctrl_status & UHCI_TD_CTRL_ACT)) {
        printf("[USB] Poll #%d: TD ctrl_status=0x%x USBSTS=0x%x\n",
               poll_count, ctrl_status, usbsts);
    }
    
    if (!(ctrl_status & UHCI_TD_CTRL_ACT)) {
        // TD is no longer active - check if it completed successfully
        uint32_t errors = (ctrl_status >> UHCI_TD_CTRL_ERR_SHIFT) & UHCI_TD_CTRL_ERR_MASK;
        
        if (errors == 0) {
            // Success! Calculate actual bytes transferred
            // For IN transfers, we need to check the actual length
            uint32_t token = intr_state.td->token;
            int actual_len = ((token >> UHCI_TD_TOKEN_MAXLEN_SHIFT) & 0x7FF) + 1;

            // 将数据从 DMA 缓冲区复制回用户缓冲区
            if (intr_state.dma_buffer && intr_state.buffer) {
                memcpy(intr_state.buffer, intr_state.dma_buffer, actual_len);
                printf("[USB]   Copied %d bytes from DMA to user buffer\n", actual_len);
            }

            intr_state.bytes_transferred = actual_len;
            intr_state.active = 0;

            printf("[USB] Interrupt transfer complete: %d bytes\n", actual_len);
            return 1;
        } else {
            printf("[USB] ERROR: Interrupt transfer failed, errors=%d\n", errors);
            intr_state.active = 0;
            return -1;
        }
    }

    // Transfer still active - keep waiting
    return 0;
}

// ======================================
// USB Mouse Periodic Polling with Split Transaction Support
// ======================================

/**
 * @brief USB mouse periodic polling state
 */
typedef struct {
    uhci_qh_t *qh;           // Queue Head for periodic polling
    uhci_td_t *td[2];       // TDs: [0]=START, [1]=COMPLETE (for low-speed split)
    uint8_t *dma_buffer;      // DMA buffer (8 bytes for mouse report)
    uint32_t dma_buffer_phys;  // Physical address of DMA buffer
    uint8_t dev_addr;          // 🔥 Device address for TD token
    uint8_t ep;                // 🔥 Endpoint number for TD token
    int toggle;                // DATA toggle bit (0 or 1)
    int is_low_speed;          // Is this a low-speed device?
    int active;                // Is periodic polling active?
    volatile int data_ready;   // 🔥 Data ready flag (set by IRQ, cleared by read)
    uint8_t last_report[8];   // 🔥 Last mouse report (for interrupt mode)
} usb_mouse_periodic_t;

static usb_mouse_periodic_t mouse_periodic = {0};

int usb_mouse_periodic_init(int controller_id,
                            uint8_t dev_addr,
                            uint8_t ep,
                            int low_speed)
{
    uhci_controller_t *ctrl = &uhci_controllers[controller_id];

    int len = 3; // boot mouse report

    memset(&mouse_periodic, 0, sizeof(mouse_periodic));
    mouse_periodic.toggle = 0; // 🔥 interrupt endpoint starts with DATA0
    mouse_periodic.is_low_speed = low_speed;

    /* DMA buffer */
    mouse_periodic.dma_buffer =
        dma_alloc_coherent(len, &mouse_periodic.dma_buffer_phys);
    memset(mouse_periodic.dma_buffer, 0, len);

    /* Allocate TD */
    uhci_td_t *td = uhci_alloc_td(ctrl);
    memset(td, 0, sizeof(*td));
    mouse_periodic.td[0] = td;

    td->token =
        (USB_PID_IN << UHCI_TD_TOKEN_PID_SHIFT) |
        (dev_addr   << UHCI_TD_TOKEN_DEVADDR_SHIFT) |
        (ep         << UHCI_TD_TOKEN_ENDPT_SHIFT) |
        ((len - 1)  << UHCI_TD_TOKEN_MAXLEN_SHIFT) |
        (mouse_periodic.toggle << UHCI_TD_TOKEN_DATA_TOGGLE_SHIFT);

    td->buffer = mouse_periodic.dma_buffer_phys;

    td->ctrl_status =
        UHCI_TD_CTRL_ACT |
        UHCI_TD_CTRL_IOC |   // 🔥必须
        (low_speed ? UHCI_TD_CTRL_LS : 0) |
        (3 << UHCI_TD_CTRL_CERR_SHIFT);

    td->link_ptr =  UHCI_LINK_TERMINATE;

    uint32_t td_phys =
        ctrl->td_pool_phys +
        ((uint32_t)td - (uint32_t)ctrl->td_pool);


    /* Allocate QH */
    uhci_qh_t *qh = uhci_alloc_qh(ctrl);
    memset(qh, 0, sizeof(*qh));
    mouse_periodic.qh = qh;

    qh->element_ptr = td_phys;
    //qh->ctrl = low_speed ? UHCI_QH_CTRL_LS : 0;
    if (low_speed)
    qh->element_ptr |= UHCI_QH_HEAD_LS;  // 0x04，标记低速

    uint32_t qh_phys =
        ctrl->qh_pool_phys +
        ((uint32_t)qh - (uint32_t)ctrl->qh_pool);

    // 🔥 链式链接：将鼠标 QH 链接到现有的中断 QH
    // 而不是直接覆盖帧列表，避免破坏其他周期性传输
    if (ctrl->intr_qh_active) {
        // 链接到现有中断 QH
        uhci_qh_t *intr_qh = ctrl->intr_qh;
        qh->link_ptr = intr_qh->link_ptr;  // 保存原来的链接
        intr_qh->link_ptr = (qh_phys & ~0xF) | UHCI_LINK_QH;//qh_phys | 0x02;  // 链接鼠标 QH
        printf("[USB Mouse] Linked to interrupt QH (phys=0x%x)\n", qh_phys);
    } else {
        // 没有其他中断 QH，直接链接到帧列表
        // for (int i = 0; i < 1024; i += 10) {
        //     qh->link_ptr = ctrl->frame_list[i];
        //     ctrl->frame_list[i] = qh_phys | 0x02;  // QH type
        // }
        

        for (int i = 0; i < 1024; i += 10) {
            ctrl->frame_list[i] = qh_phys | UHCI_LINK_QH;
        }

        ctrl->intr_qh_active = 1;
        printf("[USB Mouse] Linked to frame_list (every 10 frames)\n");
    }

    asm volatile("mfence" ::: "memory");

    mouse_periodic.active = 1;

    printf("[USB Mouse] Periodic IN scheduled (addr=%d ep=%d)\n",
           dev_addr, ep);

    return 0;
}

/**
 * @brief Poll for mouse data (non-blocking)
 *
 * Checks if the periodic transfer has completed and returns data if available.
 * Automatically resets TDs for next transfer and toggles DATA bit.
 *
 * @param report Output buffer (must be at least 8 bytes)
 * @return Number of bytes read (0 if no data, negative on error)
 */

int usb_mouse_periodic_poll(uint8_t *report)
{
    if (!mouse_periodic.active)
        return 0;

    if (!report)
        return -1;

    uhci_td_t *td = mouse_periodic.td[0];

    /* still active */
    if (td->ctrl_status & UHCI_TD_CTRL_ACT) {
        // Transfer still in progress
        return 0;
    }

    uint32_t errors = (td->ctrl_status >> UHCI_TD_CTRL_ERR_SHIFT) & UHCI_TD_CTRL_ERR_MASK;
    if (errors != 0) {
        printf("[USB Mouse] TD error: errors=0x%x ctrl_status=0x%x\n",
               errors, td->ctrl_status);
        return -1;
    }

    /* Get actual length transferred */
    int actlen = (td->ctrl_status & ACTLEN_MASK) + 1;

    /* Validate data length */
    if (actlen < 3) {
        printf("[USB Mouse] Warning: Short packet (%d bytes)\n", actlen);
        // Still accept short packets for compatibility
        memset(report, 0, 3);
        if (actlen > 0 && mouse_periodic.dma_buffer) {
            memcpy(report, mouse_periodic.dma_buffer, actlen);
        }
    } else {
        /* Copy data from DMA buffer to user buffer */
        memcpy(report, mouse_periodic.dma_buffer, 3);
    }

    /* Debug: Print mouse data (first 10 times) */
    static int poll_count = 0;
    if (++poll_count <= 10) {
        printf("[USB Mouse] POLL #%d: btn=%d x=%d y=%d len=%d\n",
               poll_count, report[0] & 0x07, (int8_t)report[1], (int8_t)report[2], actlen);
        printf("[USB Mouse]   TD ctrl=0x%x token=0x%x\n",
               td->ctrl_status, td->token);
    }

    /* Clear DMA buffer for next transfer */
    memset(mouse_periodic.dma_buffer, 0, 3);

    /* Toggle DATA bit for next transfer */
    if (actlen > 0)
        mouse_periodic.toggle ^= 1;

    td->token =
        (td->token & ~(1 << UHCI_TD_TOKEN_DATA_TOGGLE_SHIFT)) |
        (mouse_periodic.toggle << UHCI_TD_TOKEN_DATA_TOGGLE_SHIFT);

    /* Re-arm TD */
    td->ctrl_status =
        UHCI_TD_CTRL_ACT |
        (mouse_periodic.is_low_speed ? UHCI_TD_CTRL_LS : 0) |
        UHCI_TD_CTRL_IOC | (3 << UHCI_TD_CTRL_CERR_SHIFT);  /* IOC ensures interrupt on completion */

    /* Memory barrier to ensure TD is written before controller reads it */
    asm volatile("mfence" ::: "memory");

    return 3;  /* Boot mouse report is always 3 bytes */
}



/**
 * @brief Get device low-speed flag from UHCI controller
 *
 * This is a helper function to allow other modules (like usb_mouse.c)
 * to access the device speed information without directly accessing the static
 * uhci_controllers array.
 *
 * @param controller_id Controller index
 * @return 1 if low-speed, 0 if full-speed, negative on error
 */
int usb_hcd_get_device_speed(int controller_id) {
    if (controller_id < 0 || controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    return ctrl->device_low_speed;
}

/**
 * @brief Check for USB device connect/disconnect events (hot-plug detection)
 *
 * This function should be called periodically (e.g., from main loop or timer interrupt)
 * to detect when devices are plugged in or unplugged. It will automatically
 * enumerate new devices or clean up disconnected ones.
 *
 * @param controller_id Controller index
 * @return Positive if device state changed, 0 if no change, negative on error
 */
int usb_hcd_poll_hotplug(int controller_id) {
    if (controller_id < 0 || controller_id >= num_uhci_controllers) {
        return -1;
    }

    uhci_controller_t *ctrl = &uhci_controllers[controller_id];
    if (!ctrl->initialized) {
        return -1;
    }

    int state_changed = 0;

    // Check both UHCI root hub ports
    for (int port = 0; port < 2; port++) {
        uint16_t status = uhci_read_port_status(ctrl, port);

        // Check for connection status change
        int now_connected = (status & UHCI_PORTSC_CCS) ? 1 : 0;
        int now_enabled = (status & UHCI_PORTSC_PED) ? 1 : 0;

        // Detect connect/disconnect events
        if (now_connected != ctrl->port_connected[port]) {
            state_changed = 1;
            ctrl->port_connected[port] = now_connected;

            if (now_connected) {
                printf("[USB Hotplug] Device CONNECTED on port %d\n", port);

                // Reset and enumerate the new device
                if (uhci_reset_port(ctrl, port) == 0) {
                    printf("[USB Hotplug] Port %d reset successful\n", port);

                    // Try to enumerate the device
                    extern int usb_enumerate_device(int controller_id, uint8_t port);
                    if (usb_enumerate_device(controller_id, port) >= 0) {
                        printf("[USB Hotplug] Device enumerated successfully on port %d\n", port);
                    } else {
                        printf("[USB Hotplug] WARNING: Device enumeration failed on port %d\n", port);
                    }
                } else {
                    printf("[USB Hotplug] WARNING: Port %d reset failed\n", port);
                }
            } else {
                printf("[USB Hotplug] Device DISCONNECTED from port %d\n", port);
                // TODO: Clean up disconnected device (stop transfers, free resources)
            }
        }

        // Clear port status change bits by writing back to the register
        // According to UHCI spec, writing '1' clears CSC and PEC bits
        uint16_t clear_mask = UHCI_PORTSC_CSC | UHCI_PORTSC_PEC;
        uhci_write_port_status(ctrl, port, status | clear_mask);

        // Update enable status tracking
        ctrl->port_enabled[port] = now_enabled;
    }

    return state_changed;
}
