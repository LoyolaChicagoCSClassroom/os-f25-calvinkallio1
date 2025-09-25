#include <stdint.h>

#define W 80
#define H 2

static int row = 0;
static int col = 0;
char *vram = (char*)0xB8000;


void clear_line(int r) {

	for (int i = 0; i < W; i++){
	
		int idx = (r * W + x) * 2;
		vram[i] = ' ';
		vram[i + 1]  = 0x07;
	}
}

void scroll() {

	for (int i = 1; i < H; i++) {

		for (int j = 0; j < W; j++){
		
			int distance = (y - 1) * W * 2 + j * 2;
			int source = i * W * 2 + j * 2;
			vram[distance] = vram[source];
			vram[distance + 1] = vram[source + 1];
		}
	}
	clear_line(H - 1);
	row = H - 1;
	col = 0;
}

void putc(int data) {

	if (data == '\n'){
		col = 0;
		row++;
	} else {
		
		int i = (row * W + col) * 2;
		vram[i] = char(data);
		vram[i + 1] = attr;
		if (col >= W) {
			col = 0;
			row++;
		}
	}
	if (row >= H){
		scroll();
	}
}
