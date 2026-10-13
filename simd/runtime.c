#include "runtime.h"

uint32_t simd_cycle(void) {
    uint32_t value;
    __asm__ volatile("csrr %0, cycle" : "=r"(value) :: "memory");
    return value;
}
uint64_t simd_cycle64(void) {
    uint32_t hi, low, again;
    do {
        __asm__ volatile("csrr %0, cycleh" : "=r"(hi));
        __asm__ volatile("csrr %0, cycle" : "=r"(low));
        __asm__ volatile("csrr %0, cycleh" : "=r"(again));
    } while (hi != again);
    return ((uint64_t)hi << 32) | low;
}
void simd_puts(const char *text) {
    volatile uint8_t *uart = (volatile uint8_t *)0xa00003f8u;
    while (*text) *uart = (uint8_t)*text++;
}
void simd_u32(uint32_t value) {
    char buf[11];
    unsigned i = 10;
    buf[i] = 0;
    do { buf[--i] = '0' + value % 10; value /= 10; } while (value);
    simd_puts(buf + i);
}
void simd_exit(int code) {
    *(volatile uint32_t *)0xaffff000u = code ? ((uint32_t)code << 1) | 1u : 1u;
    for (;;) __asm__ volatile("j .");
}
void *memcpy(void *dest, const void *src, size_t size) {
    uint8_t *d = dest;
    const uint8_t *s = src;
    for (size_t i = 0; i < size; ++i) d[i] = s[i];
    return dest;
}
void *memset(void *dest, int value, size_t size) {
    uint8_t *d = dest;
    for (size_t i = 0; i < size; ++i) d[i] = (uint8_t)value;
    return dest;
}
int memcmp(const void *a, const void *b, size_t size) {
    const uint8_t *x = a, *y = b;
    for (size_t i = 0; i < size; ++i) if (x[i] != y[i]) return x[i] < y[i] ? -1 : 1;
    return 0;
}
size_t strlen(const char *str) { size_t size = 0; while (str[size]) ++size; return size; }
size_t strnlen(const char *str, size_t limit) {
    size_t size = 0;
    while (size < limit && str[size]) ++size;
    return size;
}
