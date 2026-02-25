#include <stdint.h>
#include "fat.h"
#include "page.h"
#include "rprintf.h"
#include "putc.h"
#define MAX_PDI0_VA 0x00400000u

extern uint32_t _end_kernel;

uint32_t align_up1(uint32_t x) { return ((x + 0xFFFu) & 0xFFFFF000u); }
uint32_t align_down1(uint32_t x) { return (x & 0xFFFFF000u); }

typedef void (*entry_fn_t)(void);

int map_range(uint32_t va, uint32_t size_bytes) {

    uint32_t pages = (size_bytes + 4095u) / 4096u;
    struct ppage *plist = allocate_physical_pages(pages);
    if (!plist) return -1;
    if (!map_pages((void*)va, plist, pd)) return -2;
    return 0;

}

//loads and jumps into a flat binary
int load_and_exec_flat(char *name) {

    uint32_t load_va = 0x00200000u;
    uint32_t stack_va = 0x00300000u;;
    uint32_t stack_size = 0x00010000u;
    
    esp_printf(putc, "load_and_exec_flat(%s)\n", name);
    esp_printf(putc, "_end_kernel=0x%x load_va=0x%x stack_va=0x%x stack_size=0x%x\n", (uint32_t)&_end_kernel, load_va, stack_va, stack_size);


    if (stack_va + stack_size >= MAX_PDI0_VA) {

        esp_printf(putc, "load VA exceeds 4MB\n");
        return -10;

    }

    esp_printf(putc, "open %s\n", name);
    struct root_directory_entry *rde = fatOpen((char*)name);
    if (!rde) {

        esp_printf(putc, "Not found\n");
        return -1;

    }

    uint32_t size = rde->file_size;
    esp_printf(putc, "size=%x\n", size);
    if (size == 0) return -2;

    uint32_t map_size = align_up1(size);
    if (load_va + map_size >= MAX_PDI0_VA) {

        esp_printf(putc, "Program too big for PDI0\n");
        return -3;

    }

    esp_printf(putc, "map code @0x%x (%u bytes)\n", load_va, map_size);
    int mr = map_range(load_va, map_size);
    if (mr) {

        esp_printf(putc, "Map code failed\n");
        return -3;

    }

    esp_printf(putc, "Map stack @0x%x (%u bytes)\n", stack_va, stack_size);
    mr = map_range(stack_va, stack_size);
    if (mr) {

        esp_printf(putc, "map stack failed\n");
        return -5;

    }
    
    esp_printf(putc, "mapped ok\n");

    esp_printf(putc, "read...\n");
    int n = fatRead(rde, (char*)load_va, (int)size);
    esp_printf(putc, "read=%d\n", n);
    if (n <= 0 || (uint32_t)n != size) {

        esp_printf(putc, "short read\n");
        return -6;

    }

    uint8_t *p = (uint8_t*)load_va;
    esp_printf(putc, "First bytes: %x %x %x %x %x %x %x %x", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
    
    volatile uint16_t *vram = (volatile uint16_t*)0xB8000;
    vram[1] = (uint16_t)((0x2F << 8) | 'K');
    esp_printf(putc, "wrote K before jump\n");
    uint32_t new_esp = stack_va + stack_size - 4;
    esp_printf(putc, "jumping to 0x%x\n with esp=0x%x\n", load_va, new_esp);
    asm volatile("cli");
    asm volatile(
            "mov %0, %%esp \n"
            "jmp *%1       \n"
            :
            : "r"(new_esp), "r"((entry_fn_t)load_va)
            : "memory"
        );
    esp_printf(putc, "JUMP FAILED");

    return 0;

}
