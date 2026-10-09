"""Write the LPC boot checksum into vector table entry 7 of an ELF file, in place.

The LPC11U6x boot ROM only starts a flash image whose first eight vector table words
sum to zero. Patching the ELF at build time makes it, and the hex and bin made from it,
bootable no matter which tool flashes them.
"""

import struct
import sys

PT_LOAD = 1


def vector_table_offset(elf: bytes) -> int:
    """File offset of the loadable segment placed at physical address 0 (the flash start)."""
    if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
        raise ValueError("not a 32-bit little-endian ELF file")

    (ph_offset,) = struct.unpack_from("<I", elf, 0x1C)
    ph_size, ph_count = struct.unpack_from("<HH", elf, 0x2A)

    for i in range(ph_count):
        p_type, p_offset, _vaddr, p_paddr, p_filesz = struct.unpack_from(
            "<IIIII", elf, ph_offset + i * ph_size)
        if p_type == PT_LOAD and p_paddr == 0 and p_filesz >= 32:
            return p_offset

    raise ValueError("no loadable segment at address 0")


def main(path: str) -> None:
    with open(path, "rb") as f:
        elf = bytearray(f.read())

    offset = vector_table_offset(elf)
    checksum = (-sum(struct.unpack_from("<7I", elf, offset))) & 0xFFFFFFFF
    struct.pack_into("<I", elf, offset + 7 * 4, checksum)

    with open(path, "wb") as f:
        f.write(elf)

    print(f"lpc_checksum: vector[7] = 0x{checksum:08X}")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: lpc_checksum.py <firmware.elf>")
    main(sys.argv[1])
