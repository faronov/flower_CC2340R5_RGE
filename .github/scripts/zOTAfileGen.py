#!/usr/bin/env python3
"""Zigbee OTA file generator — replacement for TI's zOTAfileGen (Windows-only)."""
import struct
import sys
import os


def main():
    if len(sys.argv) < 5:
        print(f"Usage: {sys.argv[0]} <input.bin> <output_dir> <mfg_code> <image_type> [file_version]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_dir = sys.argv[2]
    mfg_code = int(sys.argv[3], 16)
    image_type = int(sys.argv[4], 16)
    file_version = int(sys.argv[5], 16) if len(sys.argv) > 5 else 0

    with open(input_path, 'rb') as f:
        image_data = f.read()

    # Zigbee OTA header (56 bytes minimal)
    header_string = os.path.basename(input_path).encode('ascii')[:32].ljust(32, b'\x00')
    header_len = 56
    sub_element_hdr = struct.pack('<HI', 0x0000, len(image_data))
    total_size = header_len + len(sub_element_hdr) + len(image_data)

    header = struct.pack('<IHHHHHI',
        0x0BEEF11E,   # OTA upgrade file identifier
        0x0100,       # Header version
        header_len,   # Header length
        0x0000,       # Field control
        mfg_code,     # Manufacturer code
        image_type,   # Image type
        file_version  # File version
    )
    header += struct.pack('<H', 0x0002)     # Zigbee stack version
    header += header_string                  # Header string (32 bytes)
    header += struct.pack('<I', total_size)  # Total image size

    out_name = f"{mfg_code:04X}-{image_type:04X}-{file_version:08X}.zigbee"
    out_path = os.path.join(output_dir, out_name)

    with open(out_path, 'wb') as f:
        f.write(header)
        f.write(sub_element_hdr)
        f.write(image_data)

    print(f"Created Zigbee OTA file: {out_path} ({total_size} bytes)")


if __name__ == '__main__':
    main()
