//page.h

#ifndef PAGE_H
#define PAGE_H

struct ppage {
	struct ppage *next;
	struct ppage *prev;
	void *physical_addr;
};

#endif
