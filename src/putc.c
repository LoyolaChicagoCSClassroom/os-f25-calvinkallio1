#include <stdint.h>

#define W 80
#define H 25

static int row = 0;
static int col = 0;

char *vram = (char*)0xB8000;

extern volatile uint8_t vga_attr;

void clear_line(int r) {
    for (int j = 0; j < W; j++) {
        int idx = 2 * (r * W + j);
        vram[idx]     = ' ';
        vram[idx + 1] = (char)vga_attr;
    }
}

void scroll(void) {
    for (int i = 1; i < H; i++) {
        for (int j = 0; j < W; j++) {
            int dst = 2 * ((i - 1) * W + j);
            int src = 2 * (i * W + j);
            vram[dst]     = vram[src];
            vram[dst + 1] = vram[src + 1];
        }
    }
    clear_line(H - 1);
    row = H - 1;
    col = 0;
}

void putc(int data) {
    if (data == '\n') {
        col = 0;
        row++;
    } else if (data == '\r') {
        col = 0;
    } else {
        int idx = 2 * (row * W + col);
        vram[idx]     = (char)data;
        vram[idx + 1] = (char)vga_attr;
        col++;
        if (col >= W) { col = 0; row++; }
    }
    if (row >= H) scroll();
}
