#include <stdint.h>
#include "putc.h"

#define ROWS 24
#define COLUMNS 80

int x = 0;
int y = 0;
unsigned short *vram = (unsigned short*)0xB8000;

void clear(int rowToClear){

	rowToClear = rowToClear - 1;
	int idxToClear = 2 * rowToClear * 160; //gives the starting index of the row to be cleared

	for (int i = idxToClear; i < idxToClear + 160; i++){
	
		vram[i] = ' ';

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

void putc(int data){
	
	if (data == '\n') {
		x = 0;
		y++;
	} else if (data == '\t') {
		int tab = (x + 8) & ~7;
		while (x < tab) putc(' ');
	} else {
		vram[y * COLUMNS + x] = data;
		vram[(y * COLUMNS + x) + 1] = 7;
		x++;
		if (x > COLUMNS) {
			x = 0;
			y++;
		}
	}

	if (y >= ROWS) scroll();

}












