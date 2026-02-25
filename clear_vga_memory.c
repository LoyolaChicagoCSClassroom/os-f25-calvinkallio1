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
void main(void) {

    for (int i = 0; i < 80 * 25; i++) sys_putc(' ');

    while (1) {}

}
