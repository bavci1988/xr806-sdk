#!/usr/bin/env python3
"""Rebuild the WLAN core ROM image from an ESPHome serial log.

The esphome-xr806 option `xr806: diagnose_wlan_rom_dump: true` prints the ROM as
lines `WROM <address> <hex>` and ends with `ROM dump done: <size> bytes, CRC-32 <crc>`.
Usage: wlan_rom_from_log.py <log file> <output .bin>
"""

import re
import sys
import zlib

LINE = re.compile(r"WROM ([0-9A-Fa-f]{8}) ([0-9A-Fa-f]+)")
DONE = re.compile(r"ROM dump done: (\d+) bytes, CRC-32 ([0-9A-Fa-f]{8})")


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    chunks: dict[int, bytes] = {}
    size = crc = None
    with open(sys.argv[1], encoding="utf-8", errors="replace") as log:
        for text in log:
            if m := LINE.search(text):
                chunks[int(m.group(1), 16)] = bytes.fromhex(m.group(2))
            elif m := DONE.search(text):
                size, crc = int(m.group(1)), int(m.group(2), 16)
    if not chunks:
        print("no WROM lines found")
        return 1
    end = max(a + len(d) for a, d in chunks.items())
    image = bytearray(end)
    missing = []
    address = 0
    while address < end:
        data = chunks.get(address)
        if data is None:
            missing.append(address)
            address += 64
            continue
        image[address : address + len(data)] = data
        address += len(data)
    with open(sys.argv[2], "wb") as out:
        out.write(image)
    print(f"{len(image)} bytes written, {len(missing)} missing 64-byte lines")
    if missing:
        print("missing:", " ".join(f"0x{a:05X}" for a in missing[:20]))
    if crc is not None:
        actual = zlib.crc32(image) & 0xFFFFFFFF
        print(f"CRC-32 {actual:08X}, log says {crc:08X} ({size} bytes):", "OK" if actual == crc else "MISMATCH")
    return 0 if not missing else 1


if __name__ == "__main__":
    sys.exit(main())
