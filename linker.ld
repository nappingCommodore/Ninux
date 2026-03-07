/* Tell the linker where execution begins — must match our _start in boot.asm */
ENTRY(_start)

SECTIONS
{
    /* Start address: 1MB = 0x100000
       This is where GRUB loads our kernel in physical memory.
       The dot '.' is the "location counter" — current address. */
    . = 1M;

    /* .text — Executable code
       BLOCK(4K): align the start of this section to a 4KB boundary
       ALIGN(4K): align each input section within */
    .text BLOCK(4K) : ALIGN(4K)
    {
        /* CRITICAL: .multiboot must come FIRST in .text
           so GRUB finds it within the first 8KB of the file */
        *(.multiboot)   /* Multiboot header from boot.asm */
        *(.text)        /* All code sections from all .o files */
    }

    /* .rodata — Read-only data (string literals, const arrays) */
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)      /* e.g., the "Hello Kernel!" string constant */
    }

    /* .data — Initialized global/static variables */
    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    /* .bss — Uninitialized data (zero-filled at load time)
       Our stack (from boot.asm) lives here */
    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)       /* "Common" symbols (uninitialized globals in C) */
        *(.bss)         /* BSS sections including our stack */
    }

    /* Discard debug sections to keep binary small */
    /DISCARD/ : { *(.comment) *(.note*) }
}