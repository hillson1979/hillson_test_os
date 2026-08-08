# Hillson OS — XHCI Driver Implementation Plan

## Context

The OS currently supports EHCI (USB 2.0) and UHCI (USB 1.1) host controllers. The EHCI driver has issues with QEMU (async transfer timeout). The user wants to add XHCI (USB 3.0+) support, which will provide:
- USB 3.0 SuperSpeed (5 Gbps) device support
- More reliable QEMU compatibility (QEMU's `nec-usb-xhci` and `qemu-xhci` devices)
- Modern USB 3.x hub and device support

The XHCI driver will reuse the existing upper USB stack (`usb.c` — device enumeration, class drivers for HID/MSD/hub) and implement the same `usb_ops_t` vtable interface as EHCI.

## Key Constraints (from kernel analysis)

1. **DMA pool is only 8MB** (phys 0x02800000-0x02FFFFFF), bump allocator, never freed. MUST budget all XHCI allocations: ~16KB for DCBAA+contexts, ~8KB for Cmd Ring + Event Ring, ~16KB per device for transfer rings. Total well under 1MB — safe.
2. **32-bit physical addresses only.** XHCI 64-bit registers: upper 32 bits set to 0. DMA region at 40-48MB — well within 32-bit range.
3. **No interrupts.** System uses polling. XHCI driver must poll Event Ring via `xhci_poll()` called from main loop.
4. **No kernel threads.** `start_kernel_task` is a stub. All work in polling functions.
5. **Spinlocks are no-ops.** Use `__sync_bool_compare_and_swap` for any real synchronization.
6. **No byte-level PCI config writes.** Use read-modify-write with dword/word functions.
7. **`mmio_map` is linear allocator** from 0xF8000000 virtual — no unmap. Consume addresses sparingly.

## Architecture Overview

### Files to Create
1. **`include/kernel/usb_xhci.h`** — XHCI register definitions, TRB structures, context structures (~350 lines)
2. **`driver/usb/usb_xhci.c`** — XHCI driver implementation (~1500 lines)

### Files to Modify
3. **`driver/usb/usb_hcd.c`** (usb_init) — Add XHCI case (prog_if=0x30) to PCI scan
4. **`Makefile`** — Add `driver/usb/usb_xhci.c` to C_SOURCES  
5. **`include/kernel/usb.h`** — Already has USB_SPEED_SUPER=4 and USB_TYPE_XHCI=0x30

## Implementation Phases

### Phase 1: Register and TRB Definitions (usb_xhci.h)

Define all XHCI hardware structures based on the xHCI spec rev 1.2:

**Capability Registers** (offset 0 from MMIO base):
- CAPLENGTH, HCIVERSION, HCSPARAMS1-3, HCCPARAMS1-2
- DBOFF (Doorbell offset), RTSOFF (Runtime Space offset)

**Operational Registers** (offset = CAPLENGTH):
- USBCMD, USBSTS, PAGESIZE, DNCTRL
- CRCR (Command Ring Control Register) — 64-bit
- DCBAAP (Device Context Base Address Array Pointer) — 64-bit
- CONFIG (Configure Register: max slots enabled)

**Runtime Registers** (offset = RTSOFF):
- MFINDEX (Microframe Index)
- Interrupter 0: IMAN, IMOD, ERSTSZ, ERSTBA, ERDP

**Doorbell Array** (offset = DBOFF):
- Doorbell register per device slot (0 = Command Ring doorbell)

**Port Registers** (offset from Operational base):
- USB2 PORTSC (Port Status and Control)
- USB3 PORTSC (Port Status and Control)

**TRB Structures** (all TRBs are 16 bytes):
- DWORD 0-1: Parameter (64-bit: data pointer or command-specific)
- DWORD 2: Status (completion code, TRB transfer length)
- DWORD 3: Control (TRB type, cycle bit, various flags)

TRB types needed:
- Command TRBs: Enable Slot, Disable Slot, Address Device, Configure Endpoint, Evaluate Context, Reset Endpoint, Stop Endpoint, Set TR Dequeue, No Op
- Transfer TRBs: Normal, Setup Stage, Data Stage, Status Stage, Link
- Event TRBs: Transfer Event, Command Completion, Port Status Change

**Context Structures** (32 bytes each, aligned to 64 bytes):
- Slot Context (32 bytes): root hub port, speed, context entries count
- Endpoint Context (32 bytes): EP type, max packet size, TR dequeue pointer, average TRB length
- Input Control Context (32 bytes): add/drop context flags

**Device Context** = Slot Context (32B) + EP Context 0 Out (32B) + EP Context 0 In (32B) + optional EPs

### Phase 2: Controller Initialization (usb_xhci.c)

```
xhci_install():
  1. Map MMIO base (BAR0)
  2. Read CAPLENGTH → caplen
  3. Cap regs at offset 0, Op regs at offset caplen
  4. Read HCSPARAMS1: MaxSlots, MaxPorts, MaxIntrs
  5. Read DBOFF, RTSOFF from capability regs
  6. Stop HC (clear USBCMD.Run)
  7. Wait for HCHalted (USBSTS.HCH)
  8. Reset HC (set USBCMD.HCRST), wait for clear
  9. Program MaxSlots in CONFIG register
  10. Allocate and program DCBAAP (Device Context Base Address Array)
  11. Allocate and program Command Ring (CRCR)
  12. Allocate Event Ring Segment
  13. Initialize Interrupter 0 (ERSTSZ, ERSTBA, IMAN, IMOD)
  14. Start HC (set USBCMD.Run)
  15. Issue No-Op command → verify Command Ring + Event Ring work
  16. Iterate ports: detect connected devices, call ehci-like reset/init
```

Memory allocations:
- DCBAA: MaxSlots × 8 bytes (64-bit pointers, 64B aligned)
- Device Contexts: One per slot (33 × 32B = 1056B, 64B aligned)
- Command Ring: 256 TRBs × 16B = 4096B (4KB page, 64B aligned)
- Event Ring Segment: 256 TRBs × 16B = 4096B (4KB page, 64B aligned)
- Transfer Rings: allocated per endpoint, 32-256 TRBs each

### Phase 3: Ring Management

**TRB Ring abstraction:**
```c
struct xhci_ring_t {
    struct xhci_trb_t *ring;     // virtual address
    uint64_t          phys;      // physical address (for 32-bit: lower 32 bits)
    uint32_t          size;      // number of TRBs
    uint32_t          enqueue;   // producer index
    uint32_t          dequeue;   // consumer index (software copy)
    uint8_t           ccs;       // Cycle State (cycles on wraparound)
};
```

Ring operations:
- `ring_enqueue(ring, trb)`: Write TRB at enqueue index, advance
- `ring_dequeue(ring)`: Read TRB at dequeue index, advance, update ERDP
- Wrap with Link TRB when enqueue reaches size-1

**Command Ring** (producer-driven):
- Software writes command TRBs at enqueue position
- Rings Command Doorbell (DB[0]) after writing
- HC executes and writes Command Completion Event to Event Ring

**Event Ring** (HC-producer, software-consumer):
- HC writes event TRBs at its enqueue position
- Software reads at dequeue position, updates ERDP register
- Handles: Command Completion, Transfer Event, Port Status Change

**Transfer Ring** (producer-driven, per endpoint):
- Software writes transfer TRB chain (Setup→Data→Status for control)
- Rings Endpoint Doorbell (DB[slot_id]) to notify HC
- HC processes TRB chain, generates Transfer Events

### Phase 4: Device Management Commands

**Enable Slot:**
- Command TRB Type=9, no parameters
- Returns Slot ID (1..MaxSlots) in Completion Event TRB
- Allocate Device Context for this slot

**Address Device:**
- Build Input Context: Input Control + Slot Context + EP0 Context (max 3 × 32B)
- Set Slot Context: Root Hub Port Number, Speed (3 = SuperSpeed)
- Set EP0 Context: EP Type=Control, MPS=512, TR Dequeue = EP0 Transfer Ring phys addr
- Command TRB Type=11, Input Context phys addr
- On success: device transitions to Addressed state
- Read output Slot Context for actual device address

**Configure Endpoint:**
- Build Input Context with endpoint contexts for active endpoints
- Command TRB Type=12
- Used for non-EP0 endpoints (interrupt, bulk)

**Evaluate Context:**
- Command TRB Type=13
- Updates endpoint context (e.g., max packet size change)

### Phase 5: Transfer Implementation

**Control Transfer (usb_ctrl_in/out → usb_ops_t):**

For a 3-stage control transfer (GET_DESCRIPTOR example):
1. Setup Stage TRB: TRB_TYPE_SETUP (3), bmRequestType/Request/Value/Index/Length in the 8-byte setup packet
2. Data Stage TRB: TRB_TYPE_DATA (2), IN/OUT direction, transfer length, data buffer pointer
3. Status Stage TRB: TRB_TYPE_STATUS (4), opposite direction, no data
4. Chain with TRB_CHAIN flag (bit 4) on Setup+Data, clear on Status
5. After enqueue, ring EP0 Doorbell (DB[slot_id], target=1 for EP0)
6. Wait for Transfer Event with matching TRB pointer

**Bulk/Interrupt Transfers:**
- Normal TRBs or Data Stage TRBs
- Chained with TRB_CHAIN flag
- Ring Endpoint Doorbell

**Mapping to usb_ops_t:**
- `setup_transfer`: Allocate Transfer Ring for the endpoint
- `setup_transaction`: Build setup TRB and data/status TRBs, chain them
- `schedule_transfer`: Enqueue TRB chain, ring Doorbell
- `wait_transfer`: Poll Event Ring for Transfer Event TRBs
- `poll_transfer`: Check Transfer Ring dequeue position
- `delete_transfer`: Free transfer ring resources

### Phase 6: Port Detection and Device Enumeration

**USB2 Port (PORTSC):**
- Port Connect Status (bit 0), Port Reset (bit 4), Port Speed (bits [13:10])
- Standard EHCI-like port management

**USB3 Port (PORTSC):**
- Similar to USB2 but different bit layout
- Additional SuperSpeed-specific status bits

**Device Connect Flow:**
1. Poll PORTSC for Connect Status Change (CSC)
2. Reset port: Set and clear PORT_RESET
3. Read speed from PORTSC
4. Map to USB_SPEED enum
5. Call usb_create_dev() + usb_setup_device() (reuses existing USB stack!)

### Phase 7: QEMU Test Setup

For testing, the user's QEMU command should be:
```bash
qemu-system-i386 -cdrom os.iso -vga std -display sdl \
  -device nec-usb-xhci,id=xhci \
  -device usb-mouse,bus=xhci.0 \
  -serial file:./output.txt
```

Or with QEMU's native XHCI:
```bash
qemu-system-i386 -cdrom os.iso -vga std -display sdl \
  -device qemu-xhci,id=xhci \
  -device usb-mouse,bus=xhci.0 \
  -serial file:./output.txt
```

## Verification Plan

1. Build with `make clean && make` — verify no compilation errors
2. Run QEMU with `-device nec-usb-xhci` — controller should be detected in PCI scan
3. Verify controller initializes without panic
4. Verify USB mouse connected to XHCI root hub port is enumerated
5. Check output.txt for: "xhci: init OK", "xhci: slot N enabled", "xhci: device addressed"
6. Test with USB HID mouse: mouse movement should produce events
7. Test with USB mass storage (MSD) for bulk transfer verification

## Non-Goals (future work)
- Isochronous (ISOCH) transfers (USB audio/video)
- USB 3.1 Gen 2 (10 Gbps) / USB 3.2 (20 Gbps) advanced features
- Streams protocol
- USB Attached SCSI (UAS) for MSD
- USB4 / Thunderbolt tunneling
- Multiple interrupters / MSI-X
