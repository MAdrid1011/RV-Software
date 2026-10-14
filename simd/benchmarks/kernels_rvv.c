#include "kernels.h"
#include <riscv_vector.h>

void gemm_i32(const uint32_t *restrict a, const uint32_t *restrict b, uint32_t *restrict out) {
    for (unsigned i = 0; i < GEMM_N; ++i) {
        for (unsigned j = 0; j < GEMM_N;) {
            const size_t vl = __riscv_vsetvl_e32m1(GEMM_N - j);
            vuint32m1_t acc = __riscv_vmv_v_x_u32m1(0, vl);
            for (unsigned k = 0; k < GEMM_N; ++k) {
                const vuint32m1_t row = __riscv_vle32_v_u32m1(b + k * GEMM_N + j, vl);
                acc = __riscv_vadd_vv_u32m1(acc, __riscv_vmul_vx_u32m1(row, a[i * GEMM_N + k], vl), vl);
            }
            __riscv_vse32_v_u32m1(out + i * GEMM_N + j, acc, vl);
            j += vl;
        }
    }
}
void fir_i32(const uint32_t *restrict input, const uint32_t *restrict taps, uint32_t *restrict out) {
    for (unsigned i = 0; i < FIR_N;) {
        const size_t vl = __riscv_vsetvl_e32m1(FIR_N - i);
        vuint32m1_t acc = __riscv_vmv_v_x_u32m1(0, vl);
        for (unsigned k = 0; k < FIR_TAPS; ++k) {
            const vuint32m1_t x = __riscv_vle32_v_u32m1(input + i + k, vl);
            acc = __riscv_vadd_vv_u32m1(acc, __riscv_vmul_vx_u32m1(x, taps[k], vl), vl);
        }
        __riscv_vse32_v_u32m1(out + i, acc, vl);
        i += vl;
    }
}
void gemm_f32(const float *restrict a, const float *restrict b, float *restrict out) {
    for (unsigned i = 0; i < GEMM_N; ++i) {
        for (unsigned j = 0; j < GEMM_N;) {
            const size_t vl = __riscv_vsetvl_e32m1(GEMM_N - j);
            vfloat32m1_t acc = __riscv_vfmv_v_f_f32m1(0, vl);
            for (unsigned k = 0; k < GEMM_N; ++k) {
                const vfloat32m1_t row = __riscv_vle32_v_f32m1(b + k * GEMM_N + j, vl);
                acc = __riscv_vfadd_vv_f32m1(acc, __riscv_vfmul_vf_f32m1(row, a[i * GEMM_N + k], vl), vl);
            }
            __riscv_vse32_v_f32m1(out + i * GEMM_N + j, acc, vl);
            j += vl;
        }
    }
}
void fir_f32(const float *restrict input, const float *restrict taps, float *restrict out) {
    for (unsigned i = 0; i < FIR_N;) {
        const size_t vl = __riscv_vsetvl_e32m1(FIR_N - i);
        vfloat32m1_t acc = __riscv_vfmv_v_f_f32m1(0, vl);
        for (unsigned k = 0; k < FIR_TAPS; ++k) {
            const vfloat32m1_t x = __riscv_vle32_v_f32m1(input + i + k, vl);
            acc = __riscv_vfadd_vv_f32m1(acc, __riscv_vfmul_vf_f32m1(x, taps[k], vl), vl);
        }
        __riscv_vse32_v_f32m1(out + i, acc, vl);
        i += vl;
    }
}
void fft_f32(const float *restrict input, float *restrict out) {
    for (unsigned i = 0; i < FFT_N; ++i) {
        unsigned reverse = 0, bits = i;
        for (unsigned bit = 0; bit < 10; ++bit) { reverse = (reverse << 1) | (bits & 1); bits >>= 1; }
        out[reverse] = input[i]; out[FFT_N + reverse] = input[FFT_N + i];
    }
    for (unsigned length = 2; length <= FFT_N; length *= 2) {
        const unsigned half = length / 2, stride = FFT_N / length;
        for (unsigned block = 0; block < FFT_N; block += length) {
            for (unsigned j = 0; j < half;) {
                const size_t vl = __riscv_vsetvl_e32m1(half - j);
                const vfloat32m1_t wr = __riscv_vlse32_v_f32m1(fft_twiddle + 2 * j * stride, 8 * stride, vl);
                const vfloat32m1_t wi = __riscv_vlse32_v_f32m1(fft_twiddle + 2 * j * stride + 1, 8 * stride, vl);
                const unsigned a = block + j, b = a + half;
                const vfloat32m1_t br = __riscv_vle32_v_f32m1(out + b, vl);
                const vfloat32m1_t bi = __riscv_vle32_v_f32m1(out + FFT_N + b, vl);
                const vfloat32m1_t ar = __riscv_vle32_v_f32m1(out + a, vl);
                const vfloat32m1_t ai = __riscv_vle32_v_f32m1(out + FFT_N + a, vl);
                const vfloat32m1_t tr = __riscv_vfsub_vv_f32m1(__riscv_vfmul_vv_f32m1(br, wr, vl),
                    __riscv_vfmul_vv_f32m1(bi, wi, vl), vl);
                const vfloat32m1_t ti = __riscv_vfadd_vv_f32m1(__riscv_vfmul_vv_f32m1(br, wi, vl),
                    __riscv_vfmul_vv_f32m1(bi, wr, vl), vl);
                __riscv_vse32_v_f32m1(out + a, __riscv_vfadd_vv_f32m1(ar, tr, vl), vl);
                __riscv_vse32_v_f32m1(out + FFT_N + a, __riscv_vfadd_vv_f32m1(ai, ti, vl), vl);
                __riscv_vse32_v_f32m1(out + b, __riscv_vfsub_vv_f32m1(ar, tr, vl), vl);
                __riscv_vse32_v_f32m1(out + FFT_N + b, __riscv_vfsub_vv_f32m1(ai, ti, vl), vl);
                j += vl;
            }
        }
    }
}
