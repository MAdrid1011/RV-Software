#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include "../simd/runtime/runtime.h"

extern char __heap_start[], __heap_end[];
static char *heap_next;
void *sbrk(ptrdiff_t increment) {
    if (!heap_next) heap_next = __heap_start;
    char *old = heap_next;
    if ((increment > 0 && increment > __heap_end - old) ||
        (increment < 0 && increment < __heap_start - old)) {
        errno = ENOMEM;
        return (void *)-1;
    }
    heap_next += increment;
    return old;
}
static int uart_put(char value, FILE *file) {
    (void)file;
    *(volatile uint8_t *)0xa00003f8u = (uint8_t)value;
    return (unsigned char)value;
}
static FILE console = FDEV_SETUP_STREAM(uart_put, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const stdout = &console;
FILE *const stderr = &console;
FILE *const stdin = &console;
char **environ;
clock_t clock(void) { return (clock_t)simd_cycle(); }
void _exit(int code) { simd_exit(code); __builtin_unreachable(); }

int workload_main(int argc, char **argv);
int main(void) {
    // Workload iterations change through the upstream command-line interface;
    // datasets, algorithms and verification are the unmodified upstream code.
    char *argv[] = {"coremark-pro", "-i1", "-c1", "-v1", NULL};
    simd_puts("CoreMark-PRO FP32, cycle timer, normalized 1 MHz\n");
    uint64_t start = simd_cycle64();
    int status = workload_main(4, argv);
    printf("End-to-end cycles: %llu\n", (unsigned long long)(simd_cycle64() - start));
    return status;
}
