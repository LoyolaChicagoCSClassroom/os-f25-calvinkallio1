#include <stdint.h>
#include "putc.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

void putc(int data);
void scroll(void);
void clear(int rowToClear);

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

uint8_t inb (uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port));
    return rv;
}

void outb(uint16_t _port, uint8_t val){
	__asm__ __volatile__ ("outb %0, %1" :: "a" (val), "dN" (_port));
}

void main() {	
	
	putc('a');
	while(1);
}
