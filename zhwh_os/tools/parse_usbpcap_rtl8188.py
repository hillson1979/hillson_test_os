import struct
import sys
import hashlib
import re
from collections import Counter


def u16(data, off):
    return struct.unpack_from("<H", data, off)[0]


def u32(data, off):
    return struct.unpack_from("<I", data, off)[0]


def u64(data, off):
    return struct.unpack_from("<Q", data, off)[0]


def packets(path):
    raw = open(path, "rb").read()
    offset = 0
    packet_number = 0
    while offset + 12 <= len(raw):
        block_type, block_length = struct.unpack_from("<II", raw, offset)
        if block_length < 12 or offset + block_length > len(raw):
            raise ValueError("invalid pcapng block at 0x%x" % offset)
        if block_type == 6 and block_length >= 32:
            captured_length = u32(raw, offset + 20)
            timestamp = (u32(raw, offset + 12) << 32) | u32(raw, offset + 16)
            packet_number += 1
            yield packet_number, timestamp, raw[offset + 28:offset + 28 + captured_length]
        offset += block_length


def parse(path):
    pending = {}
    requests = []
    traffic = Counter()
    for frame, timestamp, packet in packets(path):
        if len(packet) < 27:
            continue
        header_length = u16(packet, 0)
        if header_length < 27 or header_length > len(packet):
            continue
        irp_id = u64(packet, 2)
        status = u32(packet, 10)
        info = packet[16]
        bus = u16(packet, 17)
        device = u16(packet, 19)
        endpoint = packet[21]
        transfer = packet[22]
        payload = packet[header_length:]
        stage = packet[27] if header_length >= 28 else -1
        traffic[(bus, device, endpoint, transfer)] += 1
        if transfer != 2:
            continue
        if stage == 0 and len(payload) >= 8:
            bm_request_type, request, value, index, length = struct.unpack_from(
                "<BBHHH", payload, 0)
            record = {
                "frame": frame, "timestamp": timestamp, "irp": irp_id, "status": status,
                "info": info, "bus": bus, "device": device,
                "endpoint": endpoint, "bm": bm_request_type,
                "request": request, "value": value, "index": index,
                "length": length, "parts": [],
            }
            if not (bm_request_type & 0x80) and len(payload) > 8:
                record["parts"].append(
                    (frame, info, endpoint, stage, status, payload[8:]))
            pending[irp_id] = record
            requests.append(record)
        elif irp_id in pending and payload:
            pending[irp_id]["parts"].append(
                (frame, info, endpoint, stage, status, payload))
    return requests, traffic


def request_data(record):
    return b"".join(part[5] for part in record["parts"])


def main(path):
    requests, traffic = parse(path)
    print("USB traffic by device/endpoint/transfer:")
    for key, count in sorted(traffic.items()):
        print("  usb=%d.%d ep=%02x transfer=%d packets=%d" % (*key, count))
    setup_types = Counter(
        (r["bus"], r["device"], r["bm"], r["request"]) for r in requests)
    print("control SETUP count:", len(requests))
    for key, count in setup_types.most_common():
        print("  usb=%d.%d bm=%02x req=%02x count=%d" % (*key, count))
    standard = [r for r in requests if r["request"] == 6 and r["bm"] == 0x80]
    print("descriptor responses:")
    for record in standard:
        print("  f=%d usb=%d.%d value=%04x data=%s" % (
            record["frame"], record["bus"], record["device"], record["value"],
            request_data(record)[:32].hex(" ")))
    realtek = [r for r in requests if r["request"] == 5 and r["bm"] in (0x40, 0xC0)]
    devices = sorted(set((r["bus"], r["device"]) for r in realtek))
    print("standard GET_DESCRIPTOR requests:", len(standard))
    print("Realtek request 0x05 devices:", devices)
    print("Realtek request 0x05 count:", len(realtek))
    firmware = [r for r in realtek if 0x1000 <= r["value"] <= 0x1fff and r["bm"] == 0x40]
    captured_firmware = b"".join(request_data(r) for r in firmware)
    print("firmware-window writes: count=%d bytes=%d lengths=%s" % (
        len(firmware), sum(len(request_data(r)) for r in firmware),
        dict(sorted(Counter(len(request_data(r)) for r in firmware).items()))))
    print("captured firmware sha256:", hashlib.sha256(captured_firmware).hexdigest())
    if len(sys.argv) >= 3:
        source = open(sys.argv[2], "r", encoding="ascii").read()
        body = source[source.index("{") + 1:source.index("};")]
        embedded = bytes(int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", body))
        expected = embedded[32:] + b"\x00" * ((-len(embedded[32:])) & 3)
        print("embedded bytes=%d payload=%d padded=%d sha256=%s" % (
            len(embedded), len(embedded[32:]), len(expected),
            hashlib.sha256(expected).hexdigest()))
        if captured_firmware == expected:
            print("firmware comparison: IDENTICAL")
        else:
            limit = min(len(captured_firmware), len(expected))
            mismatch = next((i for i in range(limit)
                             if captured_firmware[i] != expected[i]), limit)
            print("firmware comparison: DIFFERENT first_mismatch=%d captured=%s embedded=%s" % (
                mismatch,
                captured_firmware[mismatch:mismatch + 16].hex(" "),
                expected[mismatch:mismatch + 16].hex(" ")))
        if len(sys.argv) == 4:
            if len(captured_firmware) != 15232:
                raise ValueError("unexpected captured firmware size")
            header = bytearray(embedded[:32])
            struct.pack_into("<H", header, 12, len(captured_firmware))
            replacement = bytes(header) + captured_firmware
            with open(sys.argv[3], "wb") as output:
                output.write(replacement)
            print("exported replacement firmware: %s bytes=%d" %
                  (sys.argv[3], len(replacement)))
    key_registers = {
        0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005,
        0x0008, 0x0009, 0x001c, 0x0023, 0x0026,
        0x0080, 0x0081, 0x0082, 0x0083, 0x0100,
    }
    focused = [r for r in realtek if r["value"] in key_registers]
    print("focused power/firmware requests:", len(focused))
    for record in focused:
        data = request_data(record)
        shown = data[:64].hex(" ")
        suffix = " ..." if len(data) > 64 else ""
        direction = "IN " if record["bm"] & 0x80 else "OUT"
        print(
            "f=%-6d t=%12.6f usb=%d.%d %-3s reg=%04x len=%-4d data[%d]=%s%s" % (
                record["frame"], record["timestamp"] / 1000000.0,
                record["bus"], record["device"], direction,
                record["value"], record["length"], len(data), shown, suffix))


if __name__ == "__main__":
    if len(sys.argv) not in (2, 3, 4):
        raise SystemExit(
            "usage: parse_usbpcap_rtl8188.py capture.pcapng "
            "[rtl8188eufw.c [replacement.bin]]")
    main(sys.argv[1])
