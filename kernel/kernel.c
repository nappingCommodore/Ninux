/*
 * kernel.c — Our first kernel!
 * This is "freestanding" C — no standard library available.
 * We can't use printf(), malloc(), or any stdlib functions.
 * We build everything from scratch.
 */

/* We define our own integer types since <stdint.h> might not be available */
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long     uint64_t;
typedef unsigned int           size_t;

/* ─── VGA Constants ─── */
#define VGA_ADDRESS   (uint16_t*)0xB8000  /* VGA text buffer base address */
#define VGA_WIDTH     80
#define VGA_HEIGHT    25

/* VGA colors (4-bit) — these are the 16 classic CGA colors */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_LIGHT_BROWN   = 14,
    VGA_COLOR_WHITE         = 15,
} vga_color;

/* ─── Terminal State ─── */
static size_t    terminal_row;     /* current cursor row    (0 to 24) */
static size_t    terminal_col;     /* current cursor column (0 to 79) */
static uint8_t   terminal_color;  /* packed foreground + background color */
static uint16_t* terminal_buffer; /* pointer to VGA memory */

/* ─── Helper: Pack foreground + background into 1 byte ─── */
static inline uint8_t vga_entry_color(vga_color fg, vga_color bg) {
    /* Background occupies upper 4 bits, foreground lower 4 bits */
    return fg | (bg << 4);
}

/* ─── Helper: Build a 16-bit VGA cell from char + color ─── */
static inline uint16_t vga_entry(uint8_t ch, uint8_t color) {
    /* Character in lower byte, color attribute in upper byte */
    return (uint16_t)ch | ((uint16_t)color << 8);
}

/* ─── Initialize the terminal ─── */
void terminal_initialize(void) {
    terminal_row    = 0;
    terminal_col    = 0;
    terminal_color  = vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    terminal_buffer = VGA_ADDRESS;

    /* Clear the entire screen by writing spaces everywhere */
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            /* index = row * width + col (2D array flattened to 1D) */
            terminal_buffer[row * VGA_WIDTH + col] = vga_entry(' ', terminal_color);
        }
    }
}

/* ─── Set the current text color ─── */
void terminal_setcolor(uint8_t color) {
    terminal_color = color;
}

/* ─── Write a single character at (col, row) ─── */
void terminal_putentryat(uint8_t ch, uint8_t color, size_t col, size_t row) {
    terminal_buffer[row * VGA_WIDTH + col] = vga_entry(ch, color);
}

/* ─── Scroll screen up by one line ─── */
void terminal_scroll(void) {
    /* Move every row up by one */
    for (size_t row = 1; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            terminal_buffer[(row - 1) * VGA_WIDTH + col] =
                terminal_buffer[row * VGA_WIDTH + col];
        }
    }
    /* Clear the last row */
    for (size_t col = 0; col < VGA_WIDTH; col++) {
        terminal_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] =
            vga_entry(' ', terminal_color);
    }
}

/* ─── Put a single character with full newline/scroll handling ─── */
void terminal_putchar(char ch) {
    if (ch == '\n') {
        /* Newline: move to next row, reset column */
        terminal_col = 0;
        terminal_row++;
    } else {
        /* Regular char: write to buffer at current position */
        terminal_putentryat((uint8_t)ch, terminal_color, terminal_col, terminal_row);
        terminal_col++;

        /* Wrap to next line if we've reached the end of a row */
        if (terminal_col == VGA_WIDTH) {
            terminal_col = 0;
            terminal_row++;
        }
    }

    /* If we've gone past the last row, scroll the screen up */
    if (terminal_row == VGA_HEIGHT) {
        terminal_scroll();
        terminal_row = VGA_HEIGHT - 1;
    }
}

/* ─── Write a string (null-terminated, like all C strings) ─── */
void terminal_writestring(const char* str) {
    /* Walk the string until we hit the null terminator '\0' */
    for (size_t i = 0; str[i] != '\0'; i++) {
        terminal_putchar(str[i]);
    }
}

/* ─── Write a number in hexadecimal (useful for debugging) ─── */
void terminal_write_hex(uint32_t num) {
    const char* hex_chars = "0123456789ABCDEF";
    terminal_writestring("0x");
    for (int i = 7; i >= 0; i--) {
        /* Extract each nibble (4 bits) from most significant to least */
        terminal_putchar(hex_chars[(num >> (i * 4)) & 0xF]);
    }
}

/* ─── Write a decimal number ─── */
void terminal_write_dec(uint32_t num) {
    if (num == 0) { terminal_putchar('0'); return; }
    char buf[12];
    int i = 0;
    while (num > 0) { buf[i++] = '0' + (num % 10); num /= 10; }
    while (--i >= 0) terminal_putchar(buf[i]);
}

/* ════════════════════════════════════════════════
   KERNEL MAIN — This is where our kernel starts!
   Called from boot.asm after stack is set up.
   ════════════════════════════════════════════════ */
void kernel_main(void) {
    terminal_initialize();

    /* Print a welcome banner */
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    terminal_writestring("╔══════════════════════════════════╗\n");
    terminal_writestring("║      Ninux v0.1 — x86         ║\n");
    terminal_writestring("╚══════════════════════════════════╝\n\n");

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK));
    terminal_writestring("[OK] Kernel loaded successfully!\n");
    terminal_writestring("[OK] VGA text mode initialized (80x25)\n");

    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    terminal_writestring("\nHello, Kernel World!\n");
    terminal_writestring("Running on bare metal (no OS below us).\n\n");

    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK));
    terminal_writestring("VGA Buffer address: ");
    terminal_write_hex((uint32_t)0xB8000);
    terminal_putchar('\n');

    /* Kernel is done setting up — halt forever.
       In a real kernel, you'd start scheduling here. */
    while (1) {
        /* __asm__ volatile("hlt"); — uncomment if using GCC with asm support */
    }
}
