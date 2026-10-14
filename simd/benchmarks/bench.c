#include "kernels.h"
#include "runtime.h"

static uint32_t ia[GEMM_N * GEMM_N], ib[GEMM_N * GEMM_N], io[GEMM_N * GEMM_N], ir[GEMM_N * GEMM_N];
static float fa[GEMM_N * GEMM_N], fb[GEMM_N * GEMM_N], fo[GEMM_N * GEMM_N], fr[GEMM_N * GEMM_N];
static uint32_t ix[FIR_N + FIR_TAPS], it[FIR_TAPS];
static float fx[FIR_N + FIR_TAPS], ft[FIR_TAPS];
static float fft_in[FFT_N * 2];
static uint32_t seed = 0x12345678u;
static uint32_t random_word(void) { seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5; return seed; }

static void run_gemm_i(void) { gemm_i32(ia, ib, io); }
static void run_fir_i(void) { fir_i32(ix, it, io); }
static void run_gemm_f(void) { gemm_f32(fa, fb, fo); }
static void run_fir_f(void) { fir_f32(fx, ft, fo); }
static void run_fft_f(void) { fft_f32(fft_in, fo); }
static int measure(const char *name, void (*run)(void), const void *actual, const void *expected, size_t bytes) {
    uint32_t elapsed[5];
    run();
    for (unsigned r = 0; r < 5; ++r) {
        const uint32_t begin = simd_cycle();
        run();
        elapsed[r] = simd_cycle() - begin;
        if (memcmp(actual, expected, bytes)) {
            simd_puts("FAIL "); simd_puts(name); simd_puts("\n");
            return 1;
        }
    }
    for (unsigned i = 1; i < 5; ++i)
        for (unsigned j = i; j > 0 && elapsed[j] < elapsed[j - 1]; --j) {
            const uint32_t swap = elapsed[j]; elapsed[j] = elapsed[j - 1]; elapsed[j - 1] = swap;
        }
    simd_puts("SIMD_RESULT "); simd_puts(name); simd_puts(" "); simd_u32(elapsed[2]); simd_puts("\n");
    return 0;
}

int main(void) {
    const uint32_t begin = simd_cycle();
    for (unsigned i = 0; i < GEMM_N * GEMM_N; ++i) {
        ia[i] = random_word(); ib[i] = random_word();
        fa[i] = (float)((int32_t)(random_word() % 1024) - 512) * 0.03125f;
        fb[i] = (float)((int32_t)(random_word() % 1024) - 512) * 0.03125f;
    }
    for (unsigned i = 0; i < FIR_N + FIR_TAPS; ++i) {
        ix[i] = random_word(); fx[i] = (float)((int32_t)(random_word() % 1024) - 512) * 0.03125f;
    }
    for (unsigned i = 0; i < FIR_TAPS; ++i) {
        it[i] = random_word(); ft[i] = (float)((int32_t)(random_word() % 1024) - 512) * 0.03125f;
    }
    for (unsigned i = 0; i < 2 * FFT_N; ++i) fft_in[i] = (float)((int32_t)(random_word() % 256) - 128) * 0.03125f;
    int error = 0;
    ref_gemm_i32(ia, ib, ir);
    error |= measure("gemm-i32", run_gemm_i, io, ir, sizeof(ir));
    ref_fir_i32(ix, it, ir);
    error |= measure("fir-i32", run_fir_i, io, ir, sizeof(ir));
    ref_gemm_f32(fa, fb, fr);
    error |= measure("gemm-f32", run_gemm_f, fo, fr, sizeof(fr));
    ref_fir_f32(fx, ft, fr);
    error |= measure("fir-f32", run_fir_f, fo, fr, sizeof(fr));
    ref_fft_f32(fft_in, fr);
    error |= measure("fft-f32", run_fft_f, fo, fr, 2 * FFT_N * sizeof(float));
    simd_puts("SIMD_END_TO_END "); simd_u32(simd_cycle() - begin); simd_puts("\n");
    return error;
}
