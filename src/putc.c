#include <stdint.h>
#include "putc.h"

#define ROWS 25
#define COLUMNS 80

int x = 0;
int y = 0;
unsigned short *vram = (unsigned short*)0xB8000;

void clear(int rowToClear){

	for (int i = 0; i < COLUMNS; i++){
	
		vram[rowToClear * COLUMNS + i] = (7 << 8) | ' ';

	}

}

void scroll(void){

	for(int r = 1; r < ROWS; r++){
		for(int c = 0; c < COLUMNS; c++){
			vram[(r-1) * COLUMNS + c] = vram[r * COLUMNS + c]; //magic numbers to grab the data further on and move it back
		}
	}
	clear(ROWS - 1);
	y = ROWS - 1;
	x = 0;

}

int putc(int data){

	if (data == '\r') {
		x = 0;
	} else if (data == '\n') {
		x = 0;
		y++;
	} else if (data == '\t') {
		int tab = (x + 8) & ~7;
		while (x < tab) putc(' ');
	} else {
		vram[y * COLUMNS + x] = (7 << 8) | data;
		x++;
		if (x >= COLUMNS) {
			x = 0;
			y++;
		}
	}

	if (y >= ROWS) scroll();
	
	return data;
}

