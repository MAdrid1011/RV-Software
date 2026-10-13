#include "runtime.h"

static int int_add(int a, int b) {
    int result;
    __asm__ volatile(
        "vsetivli zero, 8, e32, m1, ta, ma\n\t"
        "vmv.v.x v1, %1\n\t"
        "vmv.v.x v2, %2\n\t"
        "vadd.vv v3, v1, v2\n\t"
        "vmv.x.s %0, v3"
        : "=r"(result) : "r"(a), "r"(b) : "v1", "v2", "v3");
    return result;
}

static float fp_add(float a, float b) {
    float result;
    __asm__ volatile(
        "vsetivli zero, 8, e32, m1, ta, ma\n\t"
        "vfmv.v.f v1, %1\n\t"
        "vfmv.v.f v2, %2\n\t"
        "vfadd.vv v3, v1, v2\n\t"
        "vfmv.f.s %0, v3"
        : "=f"(result) : "f"(a), "f"(b) : "v1", "v2", "v3");
    return result;
}

int main(void) {
    int failures = 0;
    for (int i = 0; i < 32; ++i) {
        failures += int_add(i * 17 - 13, 7 - i * 3) != i * 14 - 6;
        failures += fp_add((float)i * 0.5f, 0.25f) != (float)i * 0.5f + 0.25f;
    }
    simd_puts("SIMD arithmetic failures: ");
    simd_u32((uint32_t)failures);
    simd_puts("\n");
    return failures != 0;
}
