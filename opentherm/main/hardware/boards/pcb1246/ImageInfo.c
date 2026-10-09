#include <stdint.h>

// ──────────────────────────────────────────────────────────────
// The image's identity, at a fixed address: 0x300, right after the CRP word.
//
// The PCB1245 gateway carries this firmware inside its own and keeps the board running
// it. It decides whether to flash by reading these 32 bytes over ISP and comparing them
// with the same 32 bytes of the image it carries: equal is "already running this", and
// anything else -- another build, the old KC2 firmware, an erased part -- is flashed.
// So the gateway never parses this; it only compares it, and logs `version`.
//
// Past the first 512 bytes on purpose: in ISP mode the boot ROM is mapped over them,
// so the ROM's compare and read commands do not see the image there.
//
//   magic    "KCOT", so a part that happens to hold zeros here is not a match
//   length   bytes in the image (the .bin)
//   crc32    CRC-32 (zlib) of the .bin, computed with this field zero
//   version  the firmware version, NUL-padded
//
// length and crc32 are written after linking by tools/lpc_image_info.py: they are facts
// about the finished image, which the compiler never sees. crc32 is what makes two
// builds of the same version -- every 0.0.0-dev -- different images.
// ──────────────────────────────────────────────────────────────

#define IMAGE_INFO_MAGIC 0x544F434Bu   // "KCOT" in memory order

struct ImageInfo
{
    uint32_t magic;
    uint32_t length;
    uint32_t crc32;
    char     version[20];
};

_Static_assert(sizeof(struct ImageInfo) == 32, "the gateway compares exactly 32 bytes");
_Static_assert(sizeof(STRUX_PROJECT_VER) <= 20, "version does not fit the image info");

__attribute__((section(".image_info"), used))
const struct ImageInfo imageInfo = { IMAGE_INFO_MAGIC, 0, 0, STRUX_PROJECT_VER };
