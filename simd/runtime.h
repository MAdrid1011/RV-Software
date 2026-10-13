#ifndef ZIRCON_SIMD_RUNTIME_H
#define ZIRCON_SIMD_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
uint32_t simd_cycle(void);
uint64_t simd_cycle64(void);
void simd_puts(const char *text);
void simd_u32(uint32_t value);
void simd_exit(int code) __attribute__((noreturn));
void *memcpy(void *dest, const void *src, size_t size);
void *memset(void *dest, int value, size_t size);
int memcmp(const void *a, const void *b, size_t size);
#endif
