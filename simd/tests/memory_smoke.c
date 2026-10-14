#include "runtime.h"
#include <riscv_vector.h>

static uint32_t source[96] __attribute__((aligned(64)));
static uint32_t destination[96] __attribute__((aligned(64)));
static uint8_t bytes[96] __attribute__((aligned(64)));
static uint8_t copy[96] __attribute__((aligned(64)));

int main(void) {
    unsigned failures = 0;
    for (unsigned i = 0; i < 96; ++i) {
        source[i] = i * 17 + 3;
        destination[i] = 0xdeadbeef;
        bytes[i] = i * 7 + 11;
        copy[i] = 0;
    }
    for (unsigned i = 0; i < 83;) {
        size_t vl = __riscv_vsetvl_e32m1(83 - i);
        vuint32m1_t a = __riscv_vle32_v_u32m1(source + i + 1, vl);
        a = __riscv_vadd_vx_u32m1(a, 5, vl);
        __riscv_vse32_v_u32m1(destination + i + 1, a, vl);
        i += vl;
    }
    for (unsigned i = 0; i < 96; ++i)
        failures += destination[i] != (i >= 1 && i < 84 ? source[i] + 5 : 0xdeadbeef);
    for (unsigned i = 0; i < 77;) {
        size_t vl = __riscv_vsetvl_e8m2(77 - i);
        vuint8m2_t a = __riscv_vle8_v_u8m2(bytes + i + 3, vl);
        __riscv_vse8_v_u8m2(copy + i + 5, a, vl);
        i += vl;
    }
    for (unsigned i = 0; i < 96; ++i)
        failures += copy[i] != (i >= 5 && i < 82 ? bytes[i - 2] : 0);
    __asm__ volatile(
        "vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v2, (%1)\n\t"
        "vmv.v.i v0, 5\n\t"
        "vle32.v v2, (%0), v0.t\n\t"
        "vse32.v v2, (%1)"
        : : "r"(source), "r"(destination) : "memory", "v0", "v2");
    for (unsigned i = 0; i < 8; ++i)
        failures += destination[i] != (i == 0 || i == 2 ? source[i] : source[i] + 5);
    // Whole-register transfers are independent of the current SEW, LMUL and VL.
    __asm__ volatile("vsetivli zero, 3, e8, mf4, ta, ma\n\t"
        "vl2re32.v v8, (%0)\n\t" "vs2r.v v8, (%1)"
        : : "r"(source), "r"(destination) : "memory", "v8", "v9");
    for (unsigned i = 0; i < 16; ++i) failures += destination[i] != source[i];
    __asm__ volatile("vsetivli zero, 19, e8, m1, tu, mu\n\t"
        "vlm.v v0, (%0)\n\t" "vsm.v v0, (%1)"
        : : "r"(bytes), "r"(copy) : "memory", "v0");
    for (unsigned i = 0; i < 3; ++i) failures += copy[i] != bytes[i];
    static uint32_t interleaved[48], separated[48];
    static uint32_t offsets[8] = {28, 0, 24, 4, 20, 8, 16, 12};
    for (unsigned i = 0; i < 48; ++i) { interleaved[i] = i * 19 + 9; separated[i] = 0; }
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vlseg3e32.v v8, (%0)\n\t" "vsseg3e32.v v8, (%1)"
        : : "r"(interleaved), "r"(separated) : "memory", "v8", "v9", "v10");
    for (unsigned i = 0; i < 24; ++i) failures += separated[i] != interleaved[i];
    __asm__ volatile("vlse32.v v1, (%0), %2\n\t" "vsse32.v v1, (%1), %2"
        : : "r"(interleaved), "r"(separated), "r"(8) : "memory", "v1");
    for (unsigned i = 0; i < 8; ++i) failures += separated[2 * i] != interleaved[2 * i];
    __asm__ volatile("vle32.v v2, (%0)\n\t" "vluxei32.v v1, (%1), v2\n\t"
        "vsoxei32.v v1, (%2), v2"
        : : "r"(offsets), "r"(interleaved), "r"(separated) : "memory", "v1", "v2");
    for (unsigned i = 0; i < 8; ++i) failures += separated[i] != interleaved[i];
    __asm__ volatile("vle32ff.v v1, (%0)\n\t" "vse32.v v1, (%1)"
        : : "r"(interleaved), "r"(separated) : "memory", "v1");
    static uint32_t repeated[16], overlapping[24], overlapping_expected[24];
    for (unsigned i = 0; i < 16; ++i) repeated[i] = (i / 2) * 4;
    for (unsigned i = 0; i < 24; ++i) {
        overlapping[i] = 0;
        overlapping_expected[i] = 0;
    }
    // Indexed segments must write one complete element before the next. These
    // offsets alias both repeated elements and neighboring segment fields.
    __asm__ volatile("vsetivli zero, 16, e32, m2, tu, mu\n\t"
        "vlseg3e32.v v8, (%0)\n\t" "vle32.v v2, (%1)\n\t"
        "vsoxseg3ei32.v v8, (%2), v2"
        : : "r"(interleaved), "r"(repeated), "r"(overlapping)
        : "memory", "v2", "v3", "v8", "v9", "v10", "v11", "v12", "v13");
    for (unsigned i = 0; i < 16; ++i)
        for (unsigned field = 0; field < 3; ++field)
            overlapping_expected[i / 2 + field] = interleaved[i * 3 + field];
    for (unsigned i = 0; i < 24; ++i) failures += overlapping[i] != overlapping_expected[i];
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%0)\n\t" "vle32.v v2, (%1)\n\t"
        "vsoxei32.v v1, (%2), v2"
        : : "r"(interleaved), "r"(repeated), "r"(overlapping) : "memory", "v1", "v2");
    for (unsigned i = 0; i < 8; ++i) overlapping_expected[i / 2] = interleaved[i];
    for (unsigned i = 0; i < 24; ++i) failures += overlapping[i] != overlapping_expected[i];
    simd_puts("SIMD memory failures: "); simd_u32(failures); simd_puts("\n");
    return failures != 0;
}
