#include "runtime.h"
#include <stdint.h>

static uint32_t a[8] = {0x80000000u, 0xffffffffu, 0, 7, 19, 0xfffffff4u, 0x7fffffffu, 93};
static uint32_t b[8] = {0xffffffffu, 0, 3, 2, 5, 7, 2, 11};
static uint32_t out[8];

static int division(void) {
    int failed = 0;
#define CHECK_DIV(INSN, EXPR) do { \
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t" \
        "vle32.v v1, (%0)\n\t" "vle32.v v2, (%1)\n\t" \
        INSN " v3, v1, v2\n\t" "vse32.v v3, (%2)" \
        : : "r"(a), "r"(b), "r"(out) : "v1", "v2", "v3", "memory"); \
    for (unsigned i = 0; i < 8; ++i) { \
        uint32_t expected = (EXPR); failed += out[i] != expected; \
    } \
} while (0)
    CHECK_DIV("vdivu.vv", b[i] ? a[i] / b[i] : UINT32_MAX);
    CHECK_DIV("vremu.vv", b[i] ? a[i] % b[i] : a[i]);
    CHECK_DIV("vdiv.vv", !b[i] ? UINT32_MAX :
        a[i] == 0x80000000u && b[i] == UINT32_MAX ? a[i] : (uint32_t)((int32_t)a[i] / (int32_t)b[i]));
    CHECK_DIV("vrem.vv", !b[i] ? a[i] :
        a[i] == 0x80000000u && b[i] == UINT32_MAX ? 0u : (uint32_t)((int32_t)a[i] % (int32_t)b[i]));
#undef CHECK_DIV
    return failed;
}

static int masks(void) {
    uint32_t population, first, prefix, carry;
    __asm__ volatile(
        "vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%4)\n\t" "vle32.v v2, (%5)\n\t"
        "vmslt.vx v0, v1, zero\n\t"
        "vcpop.m %0, v0\n\t" "vfirst.m %1, v0\n\t"
        "vmsbf.m v3, v0\n\t" "vmv.x.s %2, v3\n\t"
        "vmadc.vvm v3, v1, v2, v0\n\t" "vmv.x.s %3, v3\n\t"
        "vmerge.vvm v3, v1, v2, v0\n\t" "vse32.v v3, (%6)"
        : "=&r"(population), "=&r"(first), "=&r"(prefix), "=&r"(carry)
        : "r"(a), "r"(b), "r"(out) : "v0", "v1", "v2", "v3", "memory");
    unsigned expectedPopulation = 0, expectedCarry = 0;
    int failed = 0;
    for (unsigned i = 0; i < 8; ++i) {
        unsigned bit = (int32_t)a[i] < 0;
        expectedPopulation += bit;
        expectedCarry |= (unsigned)(((uint64_t)a[i] + b[i] + bit) >> 32) << i;
        failed += out[i] != (bit ? b[i] : a[i]);
    }
    failed += population != expectedPopulation || first != 0 || (prefix & 255u) != 0 ||
        (carry & 255u) != expectedCarry;
    return failed;
}

static int floating(void) {
    static float x[8] = {1.0f, 2.25f, 4.0f, 6.25f, 9.0f, 16.0f, 25.0f, 64.0f};
    static float roots[8] = {1.0f, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 8.0f};
    static float f[8];
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%0)\n\t" "vfsqrt.v v3, v1\n\t" "vse32.v v3, (%1)"
        : : "r"(x), "r"(f) : "v1", "v3", "memory");
    int failed = 0;
    for (unsigned i = 0; i < 8; ++i) failed += f[i] != roots[i];
    __asm__ volatile("vle32.v v1, (%0)\n\t" "vle32.v v2, (%1)\n\t"
        "vfdiv.vv v3, v1, v2\n\t" "vse32.v v3, (%2)"
        : : "r"(x), "r"(roots), "r"(f) : "v1", "v2", "v3", "memory");
    for (unsigned i = 0; i < 8; ++i) failed += f[i] != roots[i];
    __asm__ volatile("vle32.v v1, (%0)\n\t" "vfcvt.rtz.x.f.v v3, v1\n\t" "vse32.v v3, (%1)"
        : : "r"(roots), "r"(out) : "v1", "v3", "memory");
    for (unsigned i = 0; i < 8; ++i) failed += out[i] != (uint32_t)roots[i];
    return failed;
}

static int reduction_permutation(void) {
    uint32_t sum, fpSum;
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%2)\n\t" "vmv.v.i v2, 0\n\t"
        "vredsum.vs v3, v1, v2\n\t" "vmv.x.s %0, v3\n\t"
        "vslideup.vi v2, v1, 2\n\t" "vse32.v v2, (%3)\n\t"
        "vle32.v v1, (%4)\n\t" "vmv.v.i v2, 0\n\t"
        "vfredosum.vs v3, v1, v2\n\t" "vmv.x.s %1, v3"
        : "=&r"(sum), "=&r"(fpSum) : "r"(a), "r"(out), "r"(b)
        : "v1", "v2", "v3", "memory");
    uint32_t expected = 0;
    int failed = 0;
    for (unsigned i = 0; i < 8; ++i) {
        expected += a[i];
        failed += out[i] != (i < 2 ? 0u : a[i - 2]);
    }
    // b contains integer bit patterns, so compare the ordered FP reduction
    // against the same scalar operation sequence without float contraction.
    union { uint32_t u; float f; } value, scalar;
    scalar.f = 0.0f;
    for (unsigned i = 0; i < 8; ++i) { value.u = b[i]; scalar.f += value.f; }
    // Canonical NaN payloads are checked by Spike; the C check accepts NaN.
    failed += sum != expected || ((fpSum & 0x7fffffffu) <= 0x7f800000u && fpSum != scalar.u);
    return failed;
}

static int widths(void) {
    static uint8_t narrow[32], factors[32];
    static uint16_t wide[32];
    for (unsigned i = 0; i < 32; ++i) { narrow[i] = i * 7 + 1; factors[i] = i * 3 + 2; }
    __asm__ volatile("vsetivli zero, 31, e8, m1, tu, mu\n\t"
        "vle8.v v1, (%0)\n\t" "vle8.v v2, (%1)\n\t"
        "vwmulu.vv v4, v1, v2\n\t" "vse16.v v4, (%2)"
        : : "r"(narrow), "r"(factors), "r"(wide) : "v1", "v2", "v4", "v5", "memory");
    int failed = 0;
    for (unsigned i = 0; i < 31; ++i) failed += wide[i] != (unsigned)narrow[i] * factors[i];
    __asm__ volatile("vsetivli zero, 8, e16, m1, tu, mu\n\t"
        "vzext.vf2 v3, v1\n\t" "vse16.v v3, (%1)"
        : : "r"(narrow), "r"(wide) : "v3", "v4", "v5", "memory");
    for (unsigned i = 0; i < 8; ++i) failed += wide[i] != narrow[i];
    return failed;
}

static int extra_operations(void) {
    static uint32_t source[16], output[16], indices[16];
    for (unsigned i = 0; i < 16; ++i) { source[i] = i * 31 + 7; indices[i] = 15 - i; }
    __asm__ volatile("vsetivli zero, 3, e8, mf4, ta, ma\n\t"
        "vl2re32.v v8, (%0)\n\t" "vmv2r.v v10, v8\n\t" "vs2r.v v10, (%1)"
        : : "r"(source), "r"(output) : "memory", "v8", "v9", "v10", "v11");
    int failed = 0;
    for (unsigned i = 0; i < 16; ++i) failed += source[i] != output[i];
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%0)\n\t" "vle32.v v2, (%1)\n\t"
        "vmsltu.vi v0, v2, 12\n\t" "vcompress.vm v3, v1, v0\n\t"
        "vse32.v v3, (%2)"
        : : "r"(source), "r"(indices), "r"(output) : "memory", "v0", "v1", "v2", "v3");
    for (unsigned i = 0; i < 4; ++i) failed += output[i] != source[i + 4];
    static uint16_t index16[8] = {7, 0, 6, 1, 5, 2, 4, 3};
    __asm__ volatile("vle32.v v1, (%0)\n\t" "vle16.v v2, (%1)\n\t"
        "vrgatherei16.vv v3, v1, v2\n\t" "vse32.v v3, (%2)"
        : : "r"(source), "r"(index16), "r"(output) : "memory", "v1", "v2", "v3");
    for (unsigned i = 0; i < 8; ++i) failed += output[i] != source[index16[i]];
    static uint32_t special[8] = {0x3f800000, 0x40000000, 0x40800000, 0x7f800000,
        0, 0x80000000, 0x00000001, 0x7f800001};
    __asm__ volatile("vle32.v v1, (%0)\n\t" "vfrec7.v v3, v1\n\t"
        "vse32.v v3, (%1)" : : "r"(special), "r"(output) : "memory", "v1", "v3");
    __asm__ volatile("vfrsqrt7.v v3, v1\n\t" "vse32.v v3, (%0)"
        : : "r"(output) : "memory", "v3");
    // The complete estimate bits and flags are checked against Spike at retirement.
    __asm__ volatile("vsetivli zero, 8, e32, m1, tu, mu\n\t"
        "vle32.v v1, (%0)\n\t" "vsmul.vv v3, v1, v1\n\t"
        "vse32.v v3, (%1)" : : "r"(a), "r"(output) : "memory", "v1", "v3");
    return failed;
}

static int floating_widths(void) {
    static uint16_t integers[16] = {0, 1, 0xffff, 0x8000, 32767, 50000, 17, 0xffc0,
        256, 512, 1024, 0x9000, 11, 12, 13, 14};
    static uint32_t floats[16] = {0, 0x3f800000, 0xbf800000, 0x3fc00000, 0xbfc00000,
        0x47000000, 0xc7000000, 0xc7000080, 0x47800000, 0x7f800000, 0xff800000,
        0x7fc00000, 0x7f800001, 1, 0x477fff80, 0xc6ffff00};
    static float wide[16];
    static uint16_t narrow[16];
    int failed = 0;
    __asm__ volatile("vsetivli zero, 16, e16, m1, tu, mu\n\t"
        "vle16.v v1, (%0)\n\t" "vfwcvt.f.x.v v4, v1\n\t" "vse32.v v4, (%1)"
        : : "r"(integers), "r"(wide) : "memory", "v1", "v4", "v5");
    for (unsigned i = 0; i < 16; ++i) failed += wide[i] != (float)(int16_t)integers[i];
    __asm__ volatile("vfwcvt.f.xu.v v4, v1\n\t" "vse32.v v4, (%0)"
        : : "r"(wide) : "memory", "v4", "v5");
    for (unsigned i = 0; i < 16; ++i) failed += wide[i] != (float)integers[i];
    for (unsigned rounding = 0; rounding < 5; ++rounding) {
        __asm__ volatile("csrw frm, %2\n\t" "vle32.v v2, (%0)\n\t"
            "vfncvt.x.f.w v4, v2\n\t" "vse16.v v4, (%1)\n\t"
            "vfncvt.xu.f.w v4, v2\n\t" "vse16.v v4, (%1)\n\t"
            "vfncvt.rtz.x.f.w v4, v2\n\t" "vse16.v v4, (%1)\n\t"
            "vfncvt.rtz.xu.f.w v4, v2\n\t" "vse16.v v4, (%1)"
            : : "r"(floats), "r"(narrow), "r"(rounding)
            : "memory", "v2", "v3", "v4");
    }
    __asm__ volatile("csrwi frm, 0" : : : "memory");
    // Spike checks every converted element and accrued flags, including NaNs
    // and values outside the destination INT16 range.
    return failed;
}

int main(void) {
    const int failed = division() + masks() + floating() + reduction_permutation() + widths() + extra_operations() + floating_widths();
    simd_puts("SIMD serial failures: "); simd_u32((uint32_t)failed); simd_puts("\n");
    return failed != 0;
}
