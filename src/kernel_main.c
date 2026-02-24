#include <stdint.h>
#include "putc.h"
#include "interrupt.h"
#include "page.h"
#include "rprintf.h"
#include "fat.h"

#define MULTIBOOT2_HEADER_MAGIC         0xe85250d6

const unsigned int multiboot_header[]  __attribute__((section(".multiboot"))) = {MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16+MULTIBOOT2_HEADER_MAGIC), 0, 12};

extern uint32_t _end_kernel;

uint32_t align_down(uint32_t x) { return x & 0xFFFFF000u; }
uint32_t align_up(uint32_t x) { return (x + 0xFFFu) & 0xFFFFF000u; }

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


void main(void) {	

	remap_pic();
	load_gdt();
	init_idt();
	asm("sti");

	for (int i = 0; i < 1024; i++){
		((uint32_t*)pd)[i] = 0;
	}

	{

		static struct ppage one;
		one.next = 0;
		one.prev = 0;
		one.physical_addr = (void*)0x000B8000;
		if (!map_pages((void*)0x000B8000, &one, pd)) {
			esp_printf(putc, "VGA Memory Allocation Failed\n");
			while(1) {}
		}
	}

	uint32_t start = 0x00100000;
	uint32_t end = align_up((uint32_t)&_end_kernel);

	for (uint32_t addr = start; addr < end; addr += 0x1000){

		static struct ppage one;
		one.next = 0;
		one.prev = 0;
		one.physical_addr = (void*)addr;
		if (!map_pages((void*)addr, &one, pd)) {
			esp_printf(putc, "Kernel allocation failed\n");
			while(1) {}
	
		}
	
	}

	uint32_t esp;
	asm("mov %%esp,%0" : "=r"(esp));

	uint32_t stack = align_down(esp) - 0x3000;
	for (uint32_t addr = stack; addr < stack + 0x4000; addr += 0x1000){

		static struct ppage one;
		one.next = 0;
		one.prev = 0;
		one.physical_addr = (void*)addr;
		if (!map_pages((void*)addr, &one, pd)){

			esp_printf(putc, "Stack memory allocation failed\n");
			while(1) {}

		}

	}

	load_page_directory(pd);
	enable_paging();

	esp_printf(putc, "O.K.\n");

	esp_printf(putc, "Memory successfully allocated\n");

	while (1){
		
		uint8_t status = inb(0x64);
		if (status & 0x01) {

			uint8_t sc = inb(0x60);

			if (sc & 0x80) continue;
			char ch = scancode_to_ascii[sc];
			if (ch) putc(ch);
		
		}

	}
}
