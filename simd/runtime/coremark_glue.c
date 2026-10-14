#include "runtime.h"
void ioe_init(void) {}
void putch(char ch) {
    *(volatile uint8_t *)0xa00003f8u = (uint8_t)ch;
}
void halt(int code) { simd_exit(code); }
