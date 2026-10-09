/* LPC11U68JBD100: 256K flash, 32K SRAM0, 2K SRAM1 (SRAM1 unused for now).
 *
 * One stack, shared by main() and interrupts, and a small newlib heap for whatever
 * calls malloc directly. */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}

/* The flash range, for "is this a string literal" checks (esp_ptr_in_drom). */
__flash_start = ORIGIN(FLASH);
__flash_end   = ORIGIN(FLASH) + LENGTH(FLASH);

/* Commands run on this stack, from the main loop: a request read off the gateway
 * link is dispatched where it arrives. The deepest path measured (-fstack-usage) is
 * about 1.7 KB before printf's internals and a nested interrupt, so 4 KB. */
STACK_SIZE = 0x1000;
HEAP_SIZE  = 0x1000;

ENTRY(Reset_Handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))

        /* The boot ROM reads the Code Read Protection word at this fixed address. */
        ASSERT(. <= 0x2FC, "vector table overlaps the CRP word");
        . = 0x2FC;
        KEEP(*(.crp))
        ASSERT(. == 0x300, "CRP word is not exactly one word at 0x2FC");

        /* The image's identity, read by the gateway over ISP (see ImageInfo.c). */
        KEEP(*(.image_info))
        ASSERT(. == 0x320, "image info is not exactly 32 bytes at 0x300");

        *(.text*)
        *(.rodata*)

        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        __preinit_array_start = .;
        KEEP(*(.preinit_array))
        __preinit_array_end = .;
        __init_array_start = .;
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
        __fini_array_start = .;
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        __fini_array_end = .;
    } > FLASH

    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > RAM AT > FLASH
    __data_load_start = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    .heap (NOLOAD) :
    {
        . = ALIGN(8);
        end = .;            /* newlib's _sbrk starts the heap here */
        . = . + HEAP_SIZE;
        __heap_end = .;
    } > RAM

    .stack (NOLOAD) :
    {
        . = ALIGN(8);
        . = . + STACK_SIZE;
        __stack_top = .;
    } > RAM
}
