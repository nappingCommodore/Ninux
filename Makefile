# ─── Compiler & Tools ─────────────────────────────────────
# If you have a proper i686-elf cross-compiler, use:
#   CC  = i686-elf-gcc
#   LD  = i686-elf-gcc
# For quick start on Linux, we use gcc with 32-bit flags:

CC	= i686-elf-gcc
AS	= nasm
LD	= i686-elf-gcc

# ─── Compiler Flags ───────────────────────────────────────
# -m32:                compile for 32-bit x86
# -std=gnu99:          C99 standard with GNU extensions
# -ffreestanding:      no standard library (we're the OS!)
# -O2:                 optimization level 2
# -Wall:               enable all warnings
# -Wextra:             enable extra warnings
# -fno-stack-protector: no stack canaries (we don't have them yet)
CFLAGS = \
    -m32               \
    -std=gnu99         \
    -ffreestanding     \
    -O2                \
    -Wall              \
    -Wextra            \
    -fno-stack-protector

# -m32:      link for 32-bit
# -nostdlib: don't link standard libraries
# -T:        use our custom linker script
LDFLAGS = \
    -m32               \
    -nostdlib          \
    -T linker.ld        

# ─── Output Binary ────────────────────────────────────────
KERNEL  = ninux.bin
OBJECTS = boot/boot.o kernel/kernel.o

# ─── Default target: build the kernel ────────────────────
all: $(KERNEL)

# Link all object files into the final kernel binary
$(KERNEL): $(OBJECTS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS) -lgcc

# Compile the Assembly boot file with NASM
# -f elf32 = output ELF 32-bit object file format
boot/boot.o: boot/boot.asm
	$(AS) -f elf32 boot/boot.asm -o boot/boot.o

# Compile the C kernel file
kernel/kernel.o: kernel/kernel.c
	$(CC) -c kernel/kernel.c -o kernel/kernel.o $(CFLAGS)

# ─── Run in QEMU emulator ─────────────────────────────────
run: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

# ─── Debug: Run QEMU with GDB stub ────────────────────────
debug: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL) -s -S &
	gdb -ex "target remote :1234" -ex "symbol-file $(KERNEL)"

# ─── Inspect the binary ───────────────────────────────────
objdump: $(KERNEL)
	objdump -d -M intel $(KERNEL) | less

# ─── Clean up build artifacts ─────────────────────────────
clean:
	rm -f $(OBJECTS) $(KERNEL)

.PHONY: all run debug clean objdump