#include "runtime.h"

static uint32_t words[16] __attribute__((aligned(64)));
static volatile uint32_t selector;

int main(void) {
    unsigned failures = 0;
    for (unsigned i = 0; i < 16; ++i) words[i] = 0x12345678u + i * 17u;
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
                     "vle32.v v8, (%0)\n\t"
                     "vmseq.vv v0, v8, v8"
                     : : "r"(words) : "memory", "v0", "v8");
    for (unsigned iteration = 0; iteration < 512; ++iteration) {
        uint32_t load0, load1, load2, move, population;
        __asm__ volatile("lw %0, 0(%5)\n\t"
                         "lw %1, 4(%5)\n\t"
                         "lw %2, 8(%5)\n\t"
                         "vmv.x.s %3, v8\n\t"
                         "vcpop.m %4, v0\n\t"
                         "add %3, %3, %0\n\t"
                         "add %4, %4, %1"
                         : "=&r"(load0), "=&r"(load1), "=&r"(load2),
                           "=&r"(move), "=&r"(population)
                         : "r"(words) : "memory");
        failures += load0 != words[0] || load1 != words[1] || load2 != words[2] ||
                    move != words[0] * 2u || population != words[1] + 8u;
        selector = iteration & 1u;
        if (selector) {
            __asm__ volatile("vmv.x.s %0, v8" : "=r"(move));
            failures += move != words[0];
        } else {
            __asm__ volatile("vcpop.m %0, v0" : "=r"(population));
            failures += population != 8u;
        }
    }
    simd_puts("SIMD scalar wakeup failures: ");
    simd_u32(failures);
    simd_puts("\n");
    return failures != 0;
}
