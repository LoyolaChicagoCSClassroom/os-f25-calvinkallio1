#include <stdint.h>
#include "putc.h"
#include "interrupt.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
	uint8_t rv;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
	return rv;
}

//scancode to ascii taken from chatGPT
static const char scancode_to_ascii[128] = {
    /*00*/ 0,   27,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    /*0F*/ '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',  0,
    /*1E*/ 'a','s','d','f','g','h','j','k','l',';','\'','`',  0, '\\',
    /*2C*/ 'z','x','c','v','b','n','m',',','.','/',  0,   0,   0,  ' ',
    /*38*/ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /*48–7F*/ 0
};

void main() {	

	remap_pic();
	load_gdt();
	init_idt();
	asm("sti");

	while (1){
		uint8_t status = inb(0x64);
		if (status & 0x01) {
			uint8_t scancode = inb(0x60);
			char data = (scancode < 128) ? scancode_to_ascii[scancode] : 0;
			if(data) putc(data);
		}
	}
}
