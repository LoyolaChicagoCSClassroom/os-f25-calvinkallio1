#include <stdio.h>
#include "page.h"

struct ppage physical_page_array[128]; //128 pages, each 2mb in length
struct ppage *pfa_head;

void init_pfa_list(void){
	
	pfa_head = &physical_page_array[0];
	pfa_head->prev = NULL;
	pfa_head->next = NULL;

	for (int i = 1; i < 128; i++){
		physical_page_array[i].prev = &physical_page_array[i-1];
		physical_page_array[i].next = NULL;
		physical_page_array[i - 1].next = &physical_page_array[i];
	}

}

struct ppage *allocate_physical_pages(unsigned int npages){

	struct ppage *newHead = pfa_head;
	struct ppage *tail = newHead;

	//Walk to npageth node
	for (int i = 1; i < npages; i++){
		if (tail->next == NULL) return NULL; //Not enough pages
		tail = tail->next;
	}

	struct ppage *remainingPages = tail->next;

	tail->next = NULL;
	newHead->prev = NULL;
	pfa_head = remainingPages;

	return newHead;


}

struct ppage *free = NULL;

void free_physical_pages(struct ppage *ppage_list){

	struct ppage *tail = ppage_list;

	//navigate to end of linked list
	while (tail->next) {
		tail = tail->next;
	}

	tail->next = free;
	if (free) free->prev = tail;

	ppage_list->prev = NULL;
	free = ppage_list;
	pfa_head = free;
	

}
