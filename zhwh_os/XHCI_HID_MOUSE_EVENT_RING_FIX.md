# xHCI HID Mouse Freeze at Event Ring Wrap

## Scope

This note records the physical-machine xHCI HID mouse freeze that occurred
after the desktop started. It applies to the interrupt-IN HID mouse path in
`driver/usb/usb_xhci.c`. It is separate from FAT32, USB mass-storage, and USB
log-file writing work.

## User-visible symptom

The mouse cursor initially moved normally, then stopped abruptly while the USB
keyboard continued to work. The stop was repeatable after a similar number of
mouse reports. USB log output later showed that report processing reached
numbers `#64`, `#128`, and then stopped after `#192`.

## Evidence

Before the freeze, the driver repeatedly reported successful HID interrupt-IN
completions:

```text
xhci: HID event slot=1 ep=3 cc=13 ptr=... remain=2
xhci: HID report slot=1 ep=3 bytes=6
usb mouse report #192: 00 00 02 00 00 00 00 00
usb mouse input #192: dx=0 dy=2 wheel=0 buttons=0
```

`cc=13` is `CC_SHORT_PACKET`, not a transfer error. The mouse endpoint has an
8-byte maximum packet size and this mouse normally returns a 6-byte report;
therefore `remain=2` and `bytes=6` are expected.

The diagnostic logger prints the first 16 reports and then every 64th report.
The `#64`, `#128`, and `#192` markers made the repeatable ring-wrap boundary
visible without flooding the USB log.

## Root cause

The driver used `xhci_ring_init()` for both Transfer Rings and the Event Ring.
That helper is correct for a Transfer Ring: it reserves the last slot for a
Link TRB with `TC=1`, so the controller can follow the ring back to its start
and toggle its transfer cycle state.

An xHCI Event Ring is different. Its Event Ring Segment Table (ERST) describes
a linear segment of event TRBs. The controller owns every entry in that segment
and wraps according to the ERST length. An Event Ring must not contain a Link
TRB.

Because the Event Ring incorrectly retained the Transfer Ring Link TRB in its
last entry, its first wrap could expose a bogus event / invalid cycle state.
The accumulated controller events from enumeration plus approximately 192 HID
reports reached this boundary, after which the mouse cursor stopped receiving
new events.

## Fix

`xhci_prepare_event_ring()` now clears the memory after allocating the Event
Ring and initializes its software consumer state explicitly:

```c
res = xhci_ring_init(&xhci->event_ring, XHCI_EVENT_RING_SIZE);
if (res < 0)
    return res;

/* Event Ring is an ERST-described linear segment, not a Transfer Ring. */
A_memset(xhci->event_ring.ring, 0,
         XHCI_EVENT_RING_SIZE * sizeof(struct xhci_trb_t));
xhci->event_ring.enqueue = 0;
xhci->event_ring.dequeue = 0;
xhci->event_ring.ccs = 1;
```

The Transfer Ring initialization is deliberately unchanged. Its Link TRB is
still required, including for the HID endpoint transfer ring.

## Supporting HID logging

`usb_handle_mouse_input()` assigns one report number per callback. It logs:

* Reports `#0` through `#15`.
* Every later report whose number is divisible by 64.

The raw HID report and parsed mouse input use the same number. This allows the
following distinction in future debugging:

| Observation | Meaning |
| --- | --- |
| HID event continues, but numbered report does not | xHCI completion did not reach the HID callback. |
| Numbered report continues, but cursor does not move | inspect GUI input consumption or coordinate limits. |
| Numbered reports stop near a repeatable boundary | inspect Event Ring / Transfer Ring wrap and cycle state. |

## Verification

On the physical machine, after rebuilding and booting the changed kernel:

1. Move the USB mouse continuously beyond the prior freeze point.
2. Confirm that the desktop cursor continues moving after report `#192`.
3. Run `log usb` in the desktop terminal and verify later markers, such as
   `usb mouse report #256` and `usb mouse input #256`.
4. Confirm that no `xhci transfer timeout`, `xhci transfer failed`, Event Ring
   full, or endpoint stall message appears during this test.

The physical-machine result after this change was that the mouse continued to
move normally past the prior `#192` freeze point.

## Files involved

* `driver/usb/usb_xhci.c`: Event Ring allocation, ERST programming, event
  consumption, and HID interrupt rearming.
* `driver/usb/usb_hid.c`: HID report parsing and rate-limited report markers.
* `driver/usb/usb_mouse.c`: bridge from HID `new_data` to the GUI input path.
* `syscall.c`: `SYS_GUI_INPUT_READ` consumes mouse reports and updates desktop
  coordinates.

