#include "../include/shell.h"
#include "../include/keyboard.h"
#include "../include/mouse.h"
#include "../include/gui/compositor.h"
#include "../include/fs/vfs.h"
#include "../include/fs/fat32.h"
#include "../include/fs/elf.h"

extern void serial_write(const char *str);

// Whether the terminal window is open or minimized
static int terminal_visible = 1;


// ── Terminal window geometry (set by shell_init based on screen size) ─────────
static int WIN_X, WIN_Y, WIN_W, WIN_H;
#define TITLE_H   22
#define BORDER     2
#define CHAR_W     8
#define CHAR_H    10
#define PAD_X      6   // padding inside content area
#define PAD_Y      4

// Derived content area
#define CONTENT_X  (WIN_X + PAD_X)
#define CONTENT_Y  (WIN_Y + TITLE_H + PAD_Y)
#define CONTENT_W  (WIN_W - PAD_X * 2)
#define CONTENT_H  (WIN_H - TITLE_H - PAD_Y * 2)

static int COLS;   // chars per row
static int ROWS;   // rows visible

// ── Terminal palette ──────────────────────────────────────────────────────────
#define FG_NORMAL   0x00AAFFAA   // green text
#define FG_PROMPT   0x00FFFF44   // yellow prompt
#define FG_ERROR    0x00FF6666   // red errors
#define FG_DIR      0x004488FF   // blue directory names
#define FG_HEADER   0x0088DDFF   // cyan headers
#define BG_TERM     0x00121820   // dark background

// ── Terminal scroll buffer ────────────────────────────────────────────────────
#define MAX_ROWS 200
#define MAX_COLS  80

static char  sbuf[MAX_ROWS][MAX_COLS]; // character data
static uint32_t sbuf_col[MAX_ROWS][MAX_COLS]; // fg color per char
static int   s_rows = 0;    // total lines written
static int   s_view = 0;    // top visible row
static int   s_col  = 0;    // current column on last row

// Command input
static char  cmd_buf[256];
static int   cmd_len = 0;

// ── Scroll buffer operations ──────────────────────────────────────────────────
static void sbuf_newline(void) {
    if (s_rows < MAX_ROWS - 1) {
        s_rows++;
    } else {
        // Scroll: shift everything up
        for (int r = 0; r < MAX_ROWS - 1; r++) {
            for (int c = 0; c < MAX_COLS; c++) {
                sbuf[r][c]     = sbuf[r+1][c];
                sbuf_col[r][c] = sbuf_col[r+1][c];
            }
        }
        // Clear new last row
        for (int c = 0; c < MAX_COLS; c++) {
            sbuf[MAX_ROWS-1][c]     = ' ';
            sbuf_col[MAX_ROWS-1][c] = BG_TERM;
        }
    }
    s_col = 0;
    // Auto-scroll view to bottom
    s_view = s_rows - ROWS + 1;
    if (s_view < 0) s_view = 0;
}

static void sbuf_putchar(char c, uint32_t color) {
    if (c == '\n') { sbuf_newline(); return; }
    if (c == '\r') { s_col = 0; return; }
    if (s_col >= MAX_COLS) { sbuf_newline(); }
    sbuf[s_rows][s_col]     = c;
    sbuf_col[s_rows][s_col] = color;
    s_col++;
}

// ── Redraw terminal content ───────────────────────────────────────────────────
static void term_redraw(void) {
    if (!terminal_visible) return;
    // Clear content area
    compositor_draw_rect(CONTENT_X - PAD_X, CONTENT_Y - PAD_Y,
                         WIN_W - BORDER, WIN_H - TITLE_H - BORDER, BG_TERM);

    for (int r = 0; r < ROWS; r++) {
        int buf_row = s_view + r;
        if (buf_row > s_rows) break;
        for (int c = 0; c < COLS; c++) {
            char ch = (buf_row == s_rows && c >= s_col) ? ' ' : sbuf[buf_row][c];
            uint32_t col = (ch == ' ') ? BG_TERM : sbuf_col[buf_row][c];
            compositor_draw_char(CONTENT_X + c * CHAR_W,
                                 CONTENT_Y + r * CHAR_H,
                                 ch, col, BG_TERM);
        }
    }

    // Draw prompt line at bottom of visible area (current input)
    int prompt_row = (s_rows - s_view);
    if (prompt_row < ROWS) {
        int px = CONTENT_X;
        int py = CONTENT_Y + prompt_row * CHAR_H;
        // "myos$ " prompt
        const char *prompt = "myos$ ";
        for (int i = 0; prompt[i]; i++) {
            compositor_draw_char(px, py, prompt[i], FG_PROMPT, BG_TERM);
            px += CHAR_W;
        }
        // input so far
        for (int i = 0; i < cmd_len; i++) {
            compositor_draw_char(px, py, cmd_buf[i], FG_NORMAL, BG_TERM);
            px += CHAR_W;
        }
        // blinking cursor block
        compositor_draw_rect(px, py, CHAR_W - 1, CHAR_H - 1, FG_PROMPT);
    }
}

// ── Public output API ─────────────────────────────────────────────────────────
void shell_putchar(char c) {
    sbuf_putchar(c, FG_NORMAL);
}

void shell_puts(const char *s) {
    while (*s) shell_putchar(*s++);
}

static void shell_puts_color(const char *s, uint32_t color) {
    while (*s) {
        if (*s == '\n') sbuf_newline();
        else sbuf_putchar(*s, color);
        s++;
    }
}

static void shell_put_uint(uint32_t n) {
    if (n == 0) { shell_putchar('0'); return; }
    char tmp[12]; int i = 0;
    while (n) { tmp[i++] = '0' + (n % 10); n /= 10; }
    for (int j = i-1; j >= 0; j--) shell_putchar(tmp[j]);
}

// ── Commands ──────────────────────────────────────────────────────────────────
static void cmd_help(void) {
    shell_puts_color("Available commands:\n", FG_HEADER);
    shell_puts("  help           - show this message\n");
    shell_puts("  ls             - list files on disk\n");
    shell_puts("  cat <file>     - print file contents\n");
    shell_puts("  mallu <file>   - execute an ELF binary\n");
    shell_puts("  clear          - clear the terminal\n");
    shell_puts("  uname          - show system information\n");
}

static void ls_callback(const char *name, int is_dir) {
    if (is_dir) {
        shell_puts_color(name, FG_DIR);
        shell_puts_color("/  ", FG_DIR);
    } else {
        shell_puts(name);
        shell_puts("  ");
    }
}

static void cmd_ls(void) {
    shell_puts_color("/  (root filesystem)\n", FG_HEADER);
    fat32_listdir(ls_callback);
    shell_puts("\n");
}

static void cmd_cat(const char *filename) {
    if (!filename || !filename[0]) {
        shell_puts_color("Usage: cat <filename>\n", FG_ERROR); return;
    }
    vfs_node_t *node = vfs_open(filename);
    if (!node) {
        shell_puts_color("cat: not found: ", FG_ERROR);
        shell_puts(filename); shell_puts("\n"); return;
    }
    uint8_t buf[512]; uint32_t offset = 0, br;
    do {
        br = vfs_read(node, offset, sizeof(buf)-1, buf);
        for (uint32_t i = 0; i < br; i++) {
            char c = (char)buf[i];
            if (c == '\r') continue;
            if (c == '\n') { sbuf_newline(); continue; }
            if (c < 32 || c > 126) c = '.';
            sbuf_putchar(c, FG_NORMAL);
        }
        offset += br;
    } while (br == sizeof(buf)-1);
    shell_puts("\n");
}

static void cmd_mallu(const char *filename) {
    if (!filename || !filename[0]) {
        shell_puts_color("Usage: mallu <filename>\n", FG_ERROR); return;
    }
    vfs_node_t *node = vfs_open(filename);
    if (!node) {
        shell_puts_color("mallu: not found: ", FG_ERROR);
        shell_puts(filename); shell_puts("\n"); return;
    }
    shell_puts("Loading: "); shell_puts(filename); shell_puts("\n");
    extern void thread_create_user(void (*entry)(void), void *pml4);
    void *pml4 = (void*)0;
    uint64_t entry = elf_load(node, &pml4);
    if (entry) {
        terminal_visible = 0;
        thread_create_user((void (*)(void))entry, pml4);
        shell_puts_color("Process spawned.\n", FG_HEADER);
    } else {
        shell_puts_color("Failed to load ELF.\n", FG_ERROR);
    }
}

static void cmd_clear(void) {
    s_rows = 0; s_col = 0; s_view = 0;
    for (int r = 0; r < MAX_ROWS; r++)
        for (int c = 0; c < MAX_COLS; c++) {
            sbuf[r][c]     = ' ';
            sbuf_col[r][c] = BG_TERM;
        }
}

static void cmd_uname(void) {
    shell_puts_color("MyOS  1.0  x86_64  Custom Kernel\n", FG_HEADER);
}

// ── String helpers ────────────────────────────────────────────────────────────
static int str_eq(const char *a, const char *b) {
    while (*a && *b) { if (*a++ != *b++) return 0; }
    return *a == *b;
}
static int str_startswith(const char *s, const char *p) {
    while (*p) { if (*s++ != *p++) return 0; } return 1;
}
static const char *skip_spaces(const char *s) {
    while (*s == ' ') s++; return s;
}

// ── Dispatch ──────────────────────────────────────────────────────────────────
static void dispatch(const char *line) {
    line = skip_spaces(line);
    if (!line[0]) return;
    if (str_eq(line, "help"))        cmd_help();
    else if (str_eq(line, "ls"))     cmd_ls();
    else if (str_eq(line, "clear"))  cmd_clear();
    else if (str_eq(line, "uname"))  cmd_uname();
    else if (str_startswith(line, "cat "))    cmd_cat(skip_spaces(line + 4));
    else if (str_startswith(line, "mallu "))  cmd_mallu(skip_spaces(line + 6));
    else if (str_startswith(line, "run "))    cmd_mallu(skip_spaces(line + 4));
    else {
        shell_puts_color("Unknown command: ", FG_ERROR);
        shell_puts(line); shell_puts("\n");
        shell_puts("Type 'help' for a list of commands.\n");
    }
}

// ── Init & Tick ───────────────────────────────────────────────────────────────
void shell_init(void) {
    // Calculate window dimensions from screen size
    int sw = compositor_screen_width();
    int sh = compositor_screen_height();

    WIN_X = 10;
    WIN_Y = 10;
    WIN_W = sw - 20;
    WIN_H = sh - 20 - 32; // leave room for taskbar (32px)

    COLS = (WIN_W - PAD_X * 2) / CHAR_W;
    ROWS = (WIN_H - TITLE_H - PAD_Y * 2) / CHAR_H;
    if (COLS > MAX_COLS) COLS = MAX_COLS;
    if (ROWS > MAX_ROWS) ROWS = MAX_ROWS;

    // Init scroll buffer
    cmd_clear();

    // Draw desktop + taskbar + terminal window
    compositor_draw_desktop();
    compositor_draw_taskbar();
    compositor_draw_window(WIN_X, WIN_Y, WIN_W, WIN_H, "Terminal");

    // Welcome message
    shell_puts_color("MyOS Shell v1.0 — Type 'help' for commands\n\n", FG_HEADER);

    term_redraw();
}

// Helper: is point (px,py) inside rect (rx,ry,rw,rh)?
static int point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return px >= rx && px < rx+rw && py >= ry && py < ry+rh;
}

void shell_tick(void) {
    int sw = compositor_screen_width();
    int sh = compositor_screen_height();

    // ── Mouse click detection ─────────────────────────────────────────────────
    static uint8_t prev_buttons = 0;
    uint8_t buttons = mouse_get_buttons();
    int mx = mouse_get_x();
    int my = mouse_get_y();

    if ((buttons & 1) && !(prev_buttons & 1)) {
        // Close button [x] on terminal title bar
        if (terminal_visible) {
            int close_x = WIN_X + WIN_W - 20;
            int close_y = WIN_Y + 3;
            if (point_in_rect(mx, my, close_x, close_y, 16, 16)) {
                terminal_visible = 0;
                compositor_draw_desktop();
                compositor_draw_taskbar();
                prev_buttons = buttons;
                return;
            }
        }
        // Taskbar Terminal button — reopen terminal
        int tb_y = sh - 32;
        if (point_in_rect(mx, my, 84, tb_y + 4, 90, 24)) {
            if (!terminal_visible) {
                terminal_visible = 1;
                compositor_draw_desktop();
                compositor_draw_taskbar();
                compositor_draw_window(WIN_X, WIN_Y, WIN_W, WIN_H, "Terminal");
                term_redraw();
            }
            prev_buttons = buttons;
            return;
        }
    }
    prev_buttons = buttons;

    // ── Keyboard input ────────────────────────────────────────────────────────
    if (!terminal_visible) return;

    char c = keyboard_getchar();
    if (!c) return;

    if (c == '\n') {
        sbuf_putchar('\n', FG_NORMAL);
        const char *prompt = "myos$ ";
        for (int i = 0; prompt[i]; i++) sbuf_putchar(prompt[i], FG_PROMPT);
        for (int i = 0; i < cmd_len; i++) sbuf_putchar(cmd_buf[i], FG_NORMAL);
        sbuf_newline();
        cmd_buf[cmd_len] = '\0';
        dispatch(cmd_buf);
        cmd_len = 0;
    } else if (c == '\b') {
        if (cmd_len > 0) cmd_len--;
    } else if (cmd_len < 255) {
        cmd_buf[cmd_len++] = c;
    }

    term_redraw();
}

