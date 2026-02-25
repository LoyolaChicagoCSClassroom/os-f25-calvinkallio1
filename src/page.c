#include <stdint.h>
#include "page.h"

struct ppage physical_page_array[128]; //128 pages, each 2mb in length
struct ppage *pfa_head;
struct page_directory_entry pd[1024] __attribute__((aligned(4096)));
struct page pt[1024] __attribute__((aligned(4096)));
extern uint8_t _end_kernel;

uint32_t align_up_4k(uint32_t x){

    return (x + 0xFFFu) & ~0xFFFu;

}

void init_pfa_list(void){
	
	pfa_head = &physical_page_array[0];
	pfa_head->prev = 0;
	pfa_head->next = 0;
    uint32_t free_phys_start = align_up_4k((uint32_t)&_end_kernel);

    for (int i = 0; i < 128; i++){

        physical_page_array[i].physical_addr = (void*)(free_phys_start + (uint32_t)i * 0x1000u);

    }

	for (int i = 1; i < 128; i++){
		physical_page_array[i].prev = &physical_page_array[i-1];
		physical_page_array[i].next = 0;
		physical_page_array[i - 1].next = &physical_page_array[i];
	}

}

struct ppage *allocate_physical_pages(unsigned int npages){

	struct ppage *newHead = pfa_head;
	struct ppage *tail = newHead;

	//Walk to npageth node
	for (int i = 1; i < npages; i++){
		if (tail->next == 0) return 0; //Not enough pages
		tail = tail->next;
	}

	struct ppage *remainingPages = tail->next;

	tail->next = 0;
	newHead->prev = 0;
	pfa_head = remainingPages;

	return newHead;


}

struct ppage *free = 0;

void free_physical_pages(struct ppage *ppage_list){

	struct ppage *tail = ppage_list;

	//navigate to end of linked list
	while (tail->next) {
		tail = tail->next;
	}

	tail->next = free;
	if (free) free->prev = tail;

	ppage_list->prev = 0;
	free = ppage_list;
	pfa_head = free;
	

}

/*Implements a similar memset function to the one found in the standard c libraries
fills n bytes with value v, starting at dst
*/

void memset8(void *dst, uint8_t v, uint32_t n) {

	uint8_t *p = (uint8_t*)dst;
	for (uint32_t i = 0; i < n; i++) p[i] = v;

}

void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd){
	
	uint32_t va_start = (uint32_t)vaddr;
    uint32_t va = va_start;
	uint32_t pdi = (va >> 22) & 0x3FF; //top 10 bits aka page directory index

	if (pdi != 0) {
		return 0;
	}

	if (!pd[0].present) {

		memset8(pt, 0, 4096);

		pd[0].present = 1;
		pd[0].rw = 1;
		pd[0].user = 0;
		pd[0].writethru = 0;
		pd[0].cachedisabled = 0;
		pd[0].accessed = 0;
		pd[0].pagesize = 0;
		pd[0].ignored = 0;
		pd[0].os_specific = 0;
		pd[0].frame = ((uint32_t)pt) >> 12;
		
	}

	//map all pages in pglist
	struct ppage *curr = pglist;
	while (curr) {

		uint32_t pti = (va >> 12) & 0x3FF;
        if (pti == 0 && va != va_start) return 0;

        uint32_t pa = (uint32_t)curr->physical_addr;

		pt[pti].present = 1;
		pt[pti].rw = 1;
		pt[pti].user = 0;
		pt[pti].accessed = 0;
		pt[pti].dirty = 0;
		pt[pti].frame = pa >> 12;
        
        asm volatile("invlpg (%0)" : : "r"(va) : "memory");

		va += 0x1000;
		curr = curr->next;

	}

	return vaddr;

}

void load_page_directory(struct page_directory_entry *pd) {

	asm("mov %0, %%cr3" : : "r"(pd) : "memory");
	
}

void enable_paging(void) {

	asm(

		"mov %%cr0, %%eax\n"
		"or $0x80000001, %%eax\n"
		"mov %%eax, %%cr0\n"
		:
		:
		: "eax", "memory"

	);
	
}
