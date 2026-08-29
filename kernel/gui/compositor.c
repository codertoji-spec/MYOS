#include "../include/gui/compositor.h"
#include "../include/gui/font8x8.h"

// ── Framebuffer state ─────────────────────────────────────────────────────────
static BootInfo  *bi;
static uint32_t  *fb;
static uint32_t   fb_width;
static uint32_t   fb_height;
static uint32_t   fb_pitch;

// ── Desktop / taskbar geometry ────────────────────────────────────────────────
#define TASKBAR_H   32
#define TITLE_H     22
#define BORDER      2

// Desktop palette
#define COL_DESKTOP    0x00243447   // dark blue-grey (like GNOME Adwaita dark)
#define COL_TASKBAR    0x00191F28   // very dark navy
#define COL_TASK_BTN   0x00303848   // slightly lighter for buttons
#define COL_TASK_TXT   0x00E0E8F0   // light text
#define COL_WIN_TITLE  0x00264D73   // blue title bar (like classic Linux)
#define COL_WIN_BODY   0x00121820   // dark window content area
#define COL_WIN_BORDER 0x004080A0   // accent border
#define COL_WIN_TXT    0x00D0D8E0   // window title text
#define COL_CLOSE_BTN  0x00C0392B   // red close button

// ── Init ──────────────────────────────────────────────────────────────────────
void compositor_init(BootInfo *boot_info) {
    bi        = boot_info;
    fb        = (uint32_t *)bi->FramebufferBase;
    fb_width  = bi->Framebuffer->HorizontalResolution;
    fb_height = bi->Framebuffer->VerticalResolution;
    fb_pitch  = bi->Framebuffer->PixelsPerScanLine;
}

// ── Metrics ───────────────────────────────────────────────────────────────────
int compositor_screen_width(void)  { return (int)fb_width; }
int compositor_screen_height(void) { return (int)fb_height; }

// ── Primitives ────────────────────────────────────────────────────────────────
static inline void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < (int)fb_width && y >= 0 && y < (int)fb_height)
        fb[y * fb_pitch + x] = color;
}

void compositor_draw_rect(int x, int y, int width, int height, uint32_t color) {
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            put_pixel(x + j, y + i, color);
}

// Draw a 1-pixel border rectangle (outline only)
static void draw_rect_outline(int x, int y, int w, int h, uint32_t color) {
    for (int i = x; i < x+w; i++) { put_pixel(i, y, color); put_pixel(i, y+h-1, color); }
    for (int i = y; i < y+h; i++) { put_pixel(x, i, color); put_pixel(x+w-1, i, color); }
}

void compositor_draw_char(int x, int y, char c, uint32_t fg_color, uint32_t bg_color) {
    if ((int)c < 0 || (int)c > 127) c = '?';
    uint8_t *glyph = (uint8_t *)font8x8_basic[(int)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << (7 - col)))
                put_pixel(x + col, y + row, fg_color);
            else if (bg_color != 0xFF000000)  // 0xFF000000 = transparent sentinel
                put_pixel(x + col, y + row, bg_color);
        }
    }
}

void compositor_draw_string(int x, int y, const char *str, uint32_t fg_color, uint32_t bg_color) {
    int cx = x, cy = y;
    while (*str) {
        if (*str == '\n') { cy += 10; cx = x; }
        else { compositor_draw_char(cx, cy, *str, fg_color, bg_color); cx += 8; }
        str++;
    }
}

void compositor_render(void) { /* direct-draw, no double buffer */ }

// ── Desktop ───────────────────────────────────────────────────────────────────
void compositor_draw_desktop(void) {
    // Background: solid dark blue-grey
    compositor_draw_rect(0, 0, fb_width, fb_height, COL_DESKTOP);

    // Subtle horizontal gradient lines for visual depth (every 4 rows slightly lighter)
    for (int y = 0; y < (int)fb_height - TASKBAR_H; y += 4) {
        uint32_t shade = (y % 8 == 0) ? 0x00263040 : COL_DESKTOP;
        for (int x = 0; x < (int)fb_width; x++)
            put_pixel(x, y, shade);
    }

    // OS logo / watermark text in center (subtle, dark)
    int lx = (int)fb_width / 2 - 40;
    int ly = (int)fb_height / 2 - 8;
    compositor_draw_string(lx, ly, "MyOS", 0x00304050, 0xFF000000);
}

// ── Taskbar ───────────────────────────────────────────────────────────────────
void compositor_draw_taskbar(void) {
    int ty = (int)fb_height - TASKBAR_H;

    // Main bar background
    compositor_draw_rect(0, ty, fb_width, TASKBAR_H, COL_TASKBAR);

    // Top separator line (accent color)
    for (int x = 0; x < (int)fb_width; x++)
        put_pixel(x, ty, COL_WIN_BORDER);

    // ── Left: App launcher button ──────────────────────────────────────
    compositor_draw_rect(4, ty + 4, 72, TASKBAR_H - 8, COL_TASK_BTN);
    draw_rect_outline(4, ty + 4, 72, TASKBAR_H - 8, COL_WIN_BORDER);
    compositor_draw_string(12, ty + 9, "* MyOS", COL_TASK_TXT, 0xFF000000);

    // ── Center: Open windows (Terminal button) ─────────────────────────
    compositor_draw_rect(84, ty + 4, 90, TASKBAR_H - 8, COL_TASK_BTN);
    draw_rect_outline(84, ty + 4, 90, TASKBAR_H - 8, COL_WIN_BORDER);
    compositor_draw_string(92, ty + 9, "Terminal", COL_TASK_TXT, 0xFF000000);

    // ── Right: System tray / time area ────────────────────────────────
    int rx = (int)fb_width - 100;
    compositor_draw_string(rx, ty + 9, "MyOS v1.0", COL_WIN_BORDER, 0xFF000000);
}

// ── Window frame ──────────────────────────────────────────────────────────────
void compositor_draw_window(int x, int y, int w, int h, const char *title) {
    // Outer border
    draw_rect_outline(x - 1, y - 1, w + 2, h + 2, COL_WIN_BORDER);

    // Title bar
    compositor_draw_rect(x, y, w, TITLE_H, COL_WIN_TITLE);

    // Gradient effect on title bar (darker top strip)
    compositor_draw_rect(x, y, w, 2, 0x00305878);

    // Title text
    compositor_draw_string(x + 10, y + 7, title, COL_WIN_TXT, 0xFF000000);

    // Close button [×]
    int bx = x + w - 20;
    int by = y + 3;
    compositor_draw_rect(bx, by, 16, 16, COL_CLOSE_BTN);
    draw_rect_outline(bx, by, 16, 16, 0x00E05040);
    compositor_draw_char(bx + 4, by + 4, 'x', 0x00FFFFFF, 0xFF000000);

    // Content area (dark)
    compositor_draw_rect(x, y + TITLE_H, w, h - TITLE_H, COL_WIN_BODY);

    // Left and bottom inner border
    draw_rect_outline(x, y + TITLE_H, w, h - TITLE_H, 0x00304050);
}

// ── Key handler (no longer used for rendering, shell owns the output) ─────────
void compositor_handle_key(char c) {
    (void)c; // Shell now handles all key input
}

// ── Mouse Cursor ──────────────────────────────────────────────────────────────
static const uint8_t cursor_shape[19][12] = {
    {1,0,0,0,0,0,0,0,0,0,0,0},
    {1,1,0,0,0,0,0,0,0,0,0,0},
    {1,2,1,0,0,0,0,0,0,0,0,0},
    {1,2,2,1,0,0,0,0,0,0,0,0},
    {1,2,2,2,1,0,0,0,0,0,0,0},
    {1,2,2,2,2,1,0,0,0,0,0,0},
    {1,2,2,2,2,2,1,0,0,0,0,0},
    {1,2,2,2,2,2,2,1,0,0,0,0},
    {1,2,2,2,2,2,2,2,1,0,0,0},
    {1,2,2,2,2,2,2,2,2,1,0,0},
    {1,2,2,2,2,2,2,2,2,2,1,0},
    {1,2,2,2,2,2,2,1,1,1,1,1},
    {1,2,2,2,1,2,2,1,0,0,0,0},
    {1,2,2,1,0,1,2,2,1,0,0,0},
    {1,2,1,0,0,1,2,2,1,0,0,0},
    {1,1,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,1,2,2,1,0,0},
    {0,0,0,0,0,0,0,1,2,1,0,0},
    {0,0,0,0,0,0,0,0,1,0,0,0},
};
#define CURSOR_W 12
#define CURSOR_H 19

static uint32_t cursor_bg[CURSOR_H * CURSOR_W];
static int mouse_cursor_x = 400;
static int mouse_cursor_y = 300;
static int cursor_drawn   = 0;

static void cursor_save_bg(int x, int y) {
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            int px = x + col, py = y + row;
            cursor_bg[row * CURSOR_W + col] =
                (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height)
                ? fb[py * fb_pitch + px] : 0;
        }
}
static void cursor_restore_bg(int x, int y) {
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            int px = x + col, py = y + row;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height)
                fb[py * fb_pitch + px] = cursor_bg[row * CURSOR_W + col];
        }
}
static void cursor_draw(int x, int y) {
    for (int row = 0; row < CURSOR_H; row++)
        for (int col = 0; col < CURSOR_W; col++) {
            uint8_t v = cursor_shape[row][col];
            if (!v) continue;
            uint32_t color = (v == 1) ? 0x00FFFFFF : 0x00000000;
            int px = x + col, py = y + row;
            if (px >= 0 && px < (int)fb_width && py >= 0 && py < (int)fb_height)
                fb[py * fb_pitch + px] = color;
        }
}

void compositor_set_cursor(int x, int y) {
    if (!fb) return;
    if (cursor_drawn) cursor_restore_bg(mouse_cursor_x, mouse_cursor_y);
    mouse_cursor_x = x;
    mouse_cursor_y = y;
    cursor_save_bg(mouse_cursor_x, mouse_cursor_y);
    cursor_draw(mouse_cursor_x, mouse_cursor_y);
    cursor_drawn = 1;
}


static int doom_x_table[1920];
static int doom_y_table[1200];
static int doom_table_w = 0;
static int doom_table_h = 0;

void compositor_blit_doom(uint32_t *src, int src_w, int src_h) {
    if (!fb) return;
    
    int screen_w = fb_width;
    int screen_h = fb_height;
    if (screen_w > 1920) screen_w = 1920;
    if (screen_h > 1200) screen_h = 1200;

    if (doom_table_w != screen_w) {
        for (int x = 0; x < screen_w; x++) {
            doom_x_table[x] = (x * src_w) / screen_w;
        }
        doom_table_w = screen_w;
    }
    if (doom_table_h != screen_h) {
        for (int y = 0; y < screen_h; y++) {
            doom_y_table[y] = (y * src_h) / screen_h;
        }
        doom_table_h = screen_h;
    }
    
    for (int y = 0; y < screen_h; y++) {
        uint32_t *dest_row = &fb[y * fb_pitch];
        uint32_t *src_row = &src[doom_y_table[y] * src_w];
        
        for (int x = 0; x < screen_w; x++) {
            dest_row[x] = src_row[doom_x_table[x]];
        }
    }
}
