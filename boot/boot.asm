; ─────────────────────────────────────────────────────────
; MULTIBOOT HEADER
; GRUB reads this to confirm: "yes, this is a valid kernel"
; Must be within the first 8KB of the binary!
; ─────────────────────────────────────────────────────────

; These constants are part of the Multiboot specification
MBOOT_PAGE_ALIGN    equ 1<<0    ; bit 0: align loaded modules on page boundaries
MBOOT_MEM_INFO      equ 1<<1    ; bit 1: provide memory map info to kernel
MBOOT_HEADER_MAGIC  equ 0x1BADB002 ; GRUB looks for this exact magic number
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
; Checksum: magic + flags + checksum must equal 0 (mod 2^32)
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

; .multiboot section: placed first so GRUB finds it quickly
section .multiboot
align 4
    dd MBOOT_HEADER_MAGIC  ; dd = "define doubleword" (32-bit value)
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM

; ─────────────────────────────────────────────────────────
; BSS SECTION — Uninitialized data (our stack lives here)
; resb N = "reserve N bytes" — filled with zeros at runtime
; ─────────────────────────────────────────────────────────
section .bss
align 16                    ; stack must be 16-byte aligned (ABI requirement)
stack_bottom:
    resb 16384              ; reserve 16 KiB for our stack
stack_top:                  ; stack grows DOWN: top is the high address

; ─────────────────────────────────────────────────────────
; TEXT SECTION — Executable code
; ─────────────────────────────────────────────────────────
section .text
global _start              ; export _start so linker can find it
extern kernel_main         ; import kernel_main from our C file

_start:
    ; GRUB hands us control here (32-bit Protected Mode)
    ; eax = 0x2BADB002 (Multiboot magic — proves GRUB loaded us)
    ; ebx = address of Multiboot info structure (memory map, etc.)

    cli                     ; Clear Interrupts — disable hardware interrupts for now
                             ; without this, random interrupts would crash us

    mov esp, stack_top      ; set ESP (Stack Pointer) to top of our stack
                             ; ESP must be set before ANY function calls

    ; Optional: pass Multiboot info to our C function
    ; push ebx              ; push multiboot info pointer as argument
    ; push eax              ; push magic number as argument

    call kernel_main        ; jump to our C code!
                             ; if kernel_main ever returns (it shouldn't),
                             ; fall through to .hang

.hang:
    hlt                     ; Halt the CPU — wait for an interrupt
                             ; Since interrupts are disabled, this stops forever
    jmp .hang               ; safety net: if somehow we wake up, halt again