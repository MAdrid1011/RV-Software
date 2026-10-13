#include "runtime.h"
#ifdef STREAM_RVV
#include <riscv_vector.h>
#endif
#ifndef STREAM_PREFIX
#define STREAM_PREFIX
#endif
#define JOIN_(a, b) a##b
#define JOIN(a, b) JOIN_(a, b)

void JOIN(STREAM_PREFIX, stream_add)(const uint32_t *restrict x,
    const uint32_t *restrict y, uint32_t *restrict out, size_t n) {
#ifdef STREAM_RVV
    for (size_t i = 0; i < n;) {
        const size_t vl = __riscv_vsetvl_e32m1(n - i);
        __riscv_vse32_v_u32m1(out + i, __riscv_vadd_vv_u32m1(
            __riscv_vle32_v_u32m1(x + i, vl), __riscv_vle32_v_u32m1(y + i, vl), vl), vl);
        i += vl;
    }
#else
    for (size_t i = 0; i < n; ++i) out[i] = x[i] + y[i];
#endif
}

void JOIN(STREAM_PREFIX, stream_saxpy)(float a, const float *restrict x,
    const float *restrict y, float *restrict out, size_t n) {
#ifdef STREAM_RVV
    for (size_t i = 0; i < n;) {
        const size_t vl = __riscv_vsetvl_e32m1(n - i);
        __riscv_vse32_v_f32m1(out + i, __riscv_vfmacc_vf_f32m1(
            __riscv_vle32_v_f32m1(y + i, vl), a, __riscv_vle32_v_f32m1(x + i, vl), vl), vl);
        i += vl;
    }
#else
    for (size_t i = 0; i < n; ++i) {
        float result;
        __asm__("fmadd.s %0, %1, %2, %3" : "=f"(result) : "f"(a), "f"(x[i]), "f"(y[i]));
        out[i] = result;
    }
#endif
}
