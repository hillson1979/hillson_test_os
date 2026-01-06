#!/bin/bash
timeout 15 qemu-system-i386 -cdrom os.iso -m 512M -serial file:/tmp/serial.log
cat /tmp/serial.log
