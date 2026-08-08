#!/usr/bin/env python3
"""
Create a FAT32 disk image with a model file in the root directory.
Usage: python mkfat32.py <output.img> <model_file.gguf>
"""
import struct
import sys
import os

def create_fat32_image(image_path, model_path):
    model_name = os.path.basename(model_path)
    model_size = os.path.getsize(model_path)

    # Calculate image size: model + 10% overhead + FAT tables + root dir
    # FAT32 cluster size: for model files, use 8 sectors/cluster (4KB) or larger
    bytes_per_sec = 512
    sec_per_cluster = 8   # 4KB clusters
    cluster_size = bytes_per_sec * sec_per_cluster
    log_size = 64 * 1024
    log_clusters = (log_size + cluster_size - 1) // cluster_size

    # FAT32: each FAT entry = 4 bytes. Each FAT entry covers 1 cluster.
    total_clusters = max(65536, (model_size // cluster_size) + log_clusters + 100)
    sectors_per_fat = (total_clusters * 4 + bytes_per_sec - 1) // bytes_per_sec + 1
    reserved_sec = 32  # boot + fsinfo + backup + reserved
    num_fats = 2
    root_cluster = 2

    total_data_sec = total_clusters * sec_per_cluster
    total_sec = reserved_sec + num_fats * sectors_per_fat + total_data_sec
    img_size = total_sec * bytes_per_sec

    print(f"Image size: {img_size / 1024 / 1024:.1f} MB")
    print(f"Sectors: {total_sec}, Cluster: {cluster_size} bytes")
    print(f"FAT size: {sectors_per_fat} sectors each")

    # Create image
    with open(image_path, 'wb') as f:
        f.truncate(img_size)

        # === Boot Sector (BPB) ===
        bpb = bytearray(bytes_per_sec)
        bpb[0:3] = b'\xEB\x58\x90'  # jmp short
        bpb[3:11] = b'FAT32   '      # OEM name
        struct.pack_into('<H', bpb, 11, bytes_per_sec)
        bpb[13] = sec_per_cluster
        struct.pack_into('<H', bpb, 14, reserved_sec)
        bpb[16] = num_fats
        struct.pack_into('<H', bpb, 17, 0)   # root entries (unused for FAT32)
        struct.pack_into('<H', bpb, 19, 0)   # total sectors (use FAT32 field)
        bpb[21] = 0xF8  # media descriptor (fixed disk)
        struct.pack_into('<H', bpb, 22, 0)   # sectors per track (unused)
        struct.pack_into('<H', bpb, 24, 0)   # num heads (unused)
        struct.pack_into('<I', bpb, 28, 0)   # hidden sectors
        struct.pack_into('<I', bpb, 32, total_sec)  # total sectors (FAT32)
        struct.pack_into('<I', bpb, 36, sectors_per_fat)
        struct.pack_into('<H', bpb, 40, 0)   # flags
        struct.pack_into('<H', bpb, 42, 0)   # version
        struct.pack_into('<I', bpb, 44, root_cluster)
        struct.pack_into('<H', bpb, 48, 1)   # FSInfo sector
        struct.pack_into('<H', bpb, 50, 6)   # backup boot sector
        bpb[64] = 0x80  # drive number
        bpb[66] = 0x29  # extended boot signature
        struct.pack_into('<I', bpb, 67, 0x12345678)  # volume serial
        bpb[71:82] = b'MODELDISK   '  # volume label
        bpb[82:90] = b'FAT32   '      # filesystem type
        bpb[510:512] = b'\x55\xAA'    # boot signature
        f.seek(0)
        f.write(bpb)

        # === FSInfo sector (sector 1) ===
        fsinfo = bytearray(bytes_per_sec)
        struct.pack_into('<I', fsinfo, 0, 0x41615252)  # lead sig
        # reserved bytes 4-483
        struct.pack_into('<I', fsinfo, 484, 0x61417272)  # second sig
        struct.pack_into('<I', fsinfo, 488, total_clusters - root_cluster)  # free clusters
        struct.pack_into('<I', fsinfo, 492, root_cluster + 1)  # next free cluster
        struct.pack_into('<I', fsinfo, 508, 0xAA550000)  # trail sig
        f.seek(bytes_per_sec)
        f.write(fsinfo)

        # === FAT tables ===
        # FAT[0] = 0x0FFFFFF8 (media descriptor)
        # FAT[1] = 0x0FFFFFFF (EOC marker + dirty flag)
        # FAT[2] = 0x0FFFFFFF (root directory, 1 cluster, EOC)
        # FAT[3..N] = 0x00000000 (free)
        fat = bytearray(sectors_per_fat * bytes_per_sec)
        struct.pack_into('<I', fat, 0, 0x0FFFFFF8)
        struct.pack_into('<I', fat, 4, 0x0FFFFFFF)
        struct.pack_into('<I', fat, 8, 0x0FFFFFFF)  # root dir ends at cluster 2
        # Model file FAT chain: sequential clusters starting from cluster 3
        model_clusters = (model_size + cluster_size - 1) // cluster_size
        for i in range(model_clusters):
            entry_offset = (3 + i) * 4
            if i == model_clusters - 1:
                struct.pack_into('<I', fat, entry_offset, 0x0FFFFFFF)  # EOC
            else:
                struct.pack_into('<I', fat, entry_offset, 3 + i + 1)  # next cluster

        log_first_cluster = 3 + model_clusters
        for i in range(log_clusters):
            entry_offset = (log_first_cluster + i) * 4
            next_cluster = 0x0FFFFFFF if i == log_clusters - 1 else log_first_cluster + i + 1
            struct.pack_into('<I', fat, entry_offset, next_cluster)

        # Write FAT #1
        sec_offset = reserved_sec * bytes_per_sec
        f.seek(sec_offset)
        f.write(fat)
        # Write FAT #2 (copy)
        sec_offset += sectors_per_fat * bytes_per_sec
        f.seek(sec_offset)
        f.write(fat)

        # === Data area ===
        data_start_sec = reserved_sec + num_fats * sectors_per_fat

        # Root directory (cluster 2): empty (just end marker)
        root_dir_sec = data_start_sec + (root_cluster - 2) * sec_per_cluster
        f.seek(root_dir_sec * bytes_per_sec)
        # Write a minimal root dir entry marking it as empty
        root_entry = bytearray(32)
        root_entry[0] = 0x00  # last entry marker
        f.write(root_entry)
        f.write(b'\x00' * (cluster_size - 32))

        # === Add model file to root directory ===
        # Replace the empty root dir with an actual entry
        # 8.3 name: truncate model name to fit
        base_name = os.path.splitext(model_name)[0][:8].upper().replace('.', '_')
        ext = os.path.splitext(model_name)[1][1:4].upper()  # e.g., "GGU"
        if not base_name:
            base_name = "MODEL"

        dir_entry = bytearray(32)
        # Name (8.3 format)
        for i, c in enumerate(base_name):
            if i < 8:
                dir_entry[i] = ord(c)
        for i in range(len(base_name), 8):
            dir_entry[i] = 0x20  # space
        for i, c in enumerate(ext):
            if i < 3:
                dir_entry[8 + i] = ord(c)
        for i in range(len(ext), 3):
            dir_entry[8 + i] = 0x20

        dir_entry[11] = 0x20  # archive attribute
        dir_entry[12] = 0x00  # reserved
        # First cluster = 3 (where model data starts)
        struct.pack_into('<H', dir_entry, 20, 0)  # cluster high
        struct.pack_into('<H', dir_entry, 26, 3)  # cluster low
        struct.pack_into('<I', dir_entry, 28, model_size)  # file size

        f.seek(root_dir_sec * bytes_per_sec)
        f.write(dir_entry)

        log_entry = bytearray(32)
        log_entry[0:11] = b'USB_AI  LOG'
        log_entry[11] = 0x20
        struct.pack_into('<H', log_entry, 20, (log_first_cluster >> 16) & 0xFFFF)
        struct.pack_into('<H', log_entry, 26, log_first_cluster & 0xFFFF)
        struct.pack_into('<I', log_entry, 28, log_size)
        f.write(log_entry)

        # === Model file data (starting at cluster 3) ===
        model_data_sec = data_start_sec + (3 - 2) * sec_per_cluster
        f.seek(model_data_sec * bytes_per_sec)

        print(f"Copying model file ({model_size / 1024 / 1024:.1f} MB)...")
        with open(model_path, 'rb') as mf:
            copied = 0
            while True:
                chunk = mf.read(4096 * 1024)  # 4MB at a time
                if not chunk:
                    break
                f.write(chunk)
                copied += len(chunk)
                pct = copied * 100 // model_size
                print(f"\r  {pct}% ({copied / 1024 / 1024:.0f} MB / {model_size / 1024 / 1024:.0f} MB)", end='')
        print()

        log_data_sec = data_start_sec + (log_first_cluster - 2) * sec_per_cluster
        f.seek(log_data_sec * bytes_per_sec)
        f.write(b'\x00' * log_size)

    print(f"Done! Created {image_path}")
    print(f"  Model: {model_name} ({model_size} bytes)")
    print(f"  First cluster: 3")
    print(f"  AI log: USB_AI.LOG ({log_size} bytes, cluster {log_first_cluster})")
    print(f"  Root dir: cluster 2")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python mkfat32.py <output.img> [model_file.gguf]")
        print("  Default model: ai/models/granite-3.0-2b-instruct-Q4_K_M.gguf")
        sys.exit(1)

    out_img = sys.argv[1]
    model = sys.argv[2] if len(sys.argv) > 2 else "ai/models/granite-3.0-2b-instruct-Q4_K_M.gguf"
    create_fat32_image(out_img, model)
