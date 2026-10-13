#include "kernels.h"
#include "runtime.h"
#ifndef SGEMM_SCALAR_ONLY
#include <riscv_vector.h>
#endif

/* Netlib BLAS SGEMM convention: column-major A/B/C and N/T operands. */
static float matrix(const float *a, int leading, int row, int column, int transpose) {
    return transpose ? a[column + row * leading] : a[row + column * leading];
}

#ifdef SGEMM_SCALAR_ONLY
void sgemm_reference(char ta, char tb, int m, int n, int k, float alpha,
                     const float *a, int lda, const float *b, int ldb,
                     float beta, float *c, int ldc) {
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) c[i + j * ldc] = beta * c[i + j * ldc];
        for (int inner = 0; inner < k; ++inner) {
            float factor = alpha * matrix(b, ldb, inner, j, tb != 'N');
            for (int i = 0; i < m; ++i)
                c[i + j * ldc] += matrix(a, lda, i, inner, ta != 'N') * factor;
        }
    }
}
#else
void sgemm_reference(char ta, char tb, int m, int n, int k, float alpha,
                     const float *a, int lda, const float *b, int ldb,
                     float beta, float *c, int ldc);

void sgemm_rvv(char ta, char tb, int m, int n, int k, float alpha,
               const float *a, int lda, const float *b, int ldb,
               float beta, float *c, int ldc) {
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m;) {
            size_t vl = __riscv_vsetvl_e32m1(m - i);
            vfloat32m1_t acc = __riscv_vfmul_vf_f32m1(__riscv_vle32_v_f32m1(c + i + j * ldc, vl), beta, vl);
            for (int inner = 0; inner < k; ++inner) {
                float factor = alpha * matrix(b, ldb, inner, j, tb != 'N');
                vfloat32m1_t av = ta == 'N' ? __riscv_vle32_v_f32m1(a + i + inner * lda, vl) :
                    __riscv_vlse32_v_f32m1(a + inner + i * lda, sizeof(float) * lda, vl);
                acc = __riscv_vfadd_vv_f32m1(acc, __riscv_vfmul_vf_f32m1(av, factor, vl), vl);
            }
            __riscv_vse32_v_f32m1(c + i + j * ldc, acc, vl);
            i += vl;
        }
    }
}

static float a[64 * 64], b[64 * 64], c[64 * 64], expected[64 * 64];
static uint32_t measure(int vector) {
    uint32_t cycles[5];
    for (unsigned repeat = 0; repeat < 6; ++repeat) {
        for (unsigned i = 0; i < 64 * 64; ++i) c[i] = ((int)(i % 5) - 2) * 0.25f;
        uint32_t begin = simd_cycle();
        if (vector) sgemm_rvv('N', 'N', 64, 64, 64, 0.5f, a, 64, b, 64, 0.25f, c, 64);
        else sgemm_reference('N', 'N', 64, 64, 64, 0.5f, a, 64, b, 64, 0.25f, c, 64);
        uint32_t elapsed = simd_cycle() - begin;
        if (memcmp(c, expected, sizeof(c))) return 0;
        if (repeat) cycles[repeat - 1] = elapsed;
    }
    for (unsigned i = 1; i < 5; ++i)
        for (unsigned j = i; j && cycles[j] < cycles[j - 1]; --j) {
            uint32_t value = cycles[j]; cycles[j] = cycles[j - 1]; cycles[j - 1] = value;
        }
    return cycles[2];
}
int main(void) {
    uint32_t begin = simd_cycle();
    for (unsigned i = 0; i < 64 * 64; ++i) {
        a[i] = ((int)(i % 23) - 11) * 0.125f;
        b[i] = ((int)(i % 17) - 8) * 0.0625f;
        c[i] = expected[i] = ((int)(i % 5) - 2) * 0.25f;
    }
    sgemm_reference('N', 'N', 64, 64, 64, 0.5f, a, 64, b, 64, 0.25f, expected, 64);
    uint32_t scalar = measure(0), vector = measure(1);
    if (!scalar || !vector) return 1;
    simd_puts("SGEMM scalar median cycles: "); simd_u32(scalar); simd_puts("\n");
    simd_puts("SGEMM RVV median cycles: "); simd_u32(vector); simd_puts("\n");
    for (unsigned transpose = 0; transpose < 4; ++transpose) {
        memset(c, 0, sizeof(c)); memset(expected, 0, sizeof(expected));
        char ta = transpose & 1 ? 'T' : 'N', tb = transpose & 2 ? 'T' : 'N';
        sgemm_reference(ta, tb, 9, 7, 11, 0.5f, a, 64, b, 64, 0, expected, 64);
        sgemm_rvv(ta, tb, 9, 7, 11, 0.5f, a, 64, b, 64, 0, c, 64);
        if (memcmp(c, expected, sizeof(c))) return 1;
    }
    simd_puts("SGEMM N/T reference checks passed\n");
    simd_puts("SGEMM end-to-end cycles: "); simd_u32(simd_cycle() - begin); simd_puts("\n");
    return 0;
}
#endif
