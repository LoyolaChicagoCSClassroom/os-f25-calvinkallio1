#include <stdint.h>
#include "src/putc.h"
#include "src/syscall.h"


static inline int sys_putc(int ch) {

    int ret;
    asm volatile(
            "int $0x80"
            : "=a" (ret)
            : "a"(SYS_PUTC), "b"(ch)
            : "memory"
            );
    return ret;

}
__attribute__((noreturn))
void _start(void) {

    volatile uint16_t *vram = (volatile uint16_t*)0xB8000;

    for (int i = 0; i < 80*25; i++){

        vram[i] = (0x07 << 8) | ' ';

    }

    vram[0] = (0x4F << 8) | 'X';

    while(1) {}

}
