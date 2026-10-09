"""Fill in the image info (length and CRC-32) of a finished LPC image, in place.

The image info is 32 bytes at 0x300 (see main/hardware/boards/pcb1246/ImageInfo.c):
magic, length, crc32, version. The compiler leaves length and crc32 zero because they
are facts about the finished image. This writes them into the .bin, and the same bytes
into the ELF, so the .bin, the .hex made after this, and a debugger flashing the ELF
all carry an identical image.

Run after lpc_checksum.py and after the .bin is made. The vector checksum covers only
the first eight words, so writing here does not disturb it.
"""

import struct
import sys
import zlib

INFO_ADDR = 0x300
MAGIC = 0x544F434B  # "KCOT"
PT_LOAD = 1


def flash_segment_offset(elf: bytes) -> int:
    """File offset of the loadable segment placed at physical address 0."""
    (ph_offset,) = struct.unpack_from("<I", elf, 0x1C)
    ph_size, ph_count = struct.unpack_from("<HH", elf, 0x2A)
    for i in range(ph_count):
        p_type, p_offset, _vaddr, p_paddr, p_filesz = struct.unpack_from(
            "<IIIII", elf, ph_offset + i * ph_size)
        if p_type == PT_LOAD and p_paddr == 0 and p_filesz >= INFO_ADDR + 32:
            return p_offset
    raise ValueError("no loadable segment at address 0 covering the image info")


def main(elf_path: str, bin_path: str) -> None:
    with open(bin_path, "rb") as f:
        image = bytearray(f.read())
    with open(elf_path, "rb") as f:
        elf = bytearray(f.read())

    (magic,) = struct.unpack_from("<I", image, INFO_ADDR)
    if magic != MAGIC:
        raise SystemExit(f"lpc_image_info: no image info at 0x{INFO_ADDR:X} (magic 0x{magic:08X})")

    struct.pack_into("<II", image, INFO_ADDR + 4, len(image), 0)
    crc = zlib.crc32(image) & 0xFFFFFFFF
    struct.pack_into("<I", image, INFO_ADDR + 8, crc)

    info = image[INFO_ADDR:INFO_ADDR + 32]
    offset = flash_segment_offset(elf) + INFO_ADDR
    if elf[offset:offset + 4] != info[:4]:
        raise SystemExit("lpc_image_info: the ELF and the .bin disagree about the image info")
    elf[offset:offset + 32] = info

    with open(bin_path, "wb") as f:
        f.write(image)
    with open(elf_path, "wb") as f:
        f.write(elf)

    version = info[12:].split(b"\0", 1)[0].decode()
    print(f"lpc_image_info: {version}, {len(image)} bytes, crc32 0x{crc:08X}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit("usage: lpc_image_info.py <firmware.elf> <firmware.bin>")
    main(sys.argv[1], sys.argv[2])
