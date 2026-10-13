#include "kernels.h"
#ifndef KERNEL_PREFIX
#define KERNEL_PREFIX
#endif
#define JOIN_(a, b) a##b
#define JOIN(a, b) JOIN_(a, b)
#define KERNEL(name) JOIN(KERNEL_PREFIX, name)

void KERNEL(gemm_i32)(const uint32_t *restrict a, const uint32_t *restrict b, uint32_t *restrict out) {
    for (unsigned i = 0; i < GEMM_N; ++i) {
        for (unsigned j = 0; j < GEMM_N; ++j) out[i * GEMM_N + j] = 0;
        for (unsigned k = 0; k < GEMM_N; ++k)
            for (unsigned j = 0; j < GEMM_N; ++j)
                out[i * GEMM_N + j] += a[i * GEMM_N + k] * b[k * GEMM_N + j];
    }
}
void KERNEL(fir_i32)(const uint32_t *restrict input, const uint32_t *restrict taps, uint32_t *restrict out) {
    for (unsigned i = 0; i < FIR_N; ++i) out[i] = 0;
    for (unsigned k = 0; k < FIR_TAPS; ++k)
        for (unsigned i = 0; i < FIR_N; ++i) out[i] += input[i + k] * taps[k];
}
void KERNEL(gemm_f32)(const float *restrict a, const float *restrict b, float *restrict out) {
    for (unsigned i = 0; i < GEMM_N; ++i) {
        for (unsigned j = 0; j < GEMM_N; ++j) out[i * GEMM_N + j] = 0.0f;
        for (unsigned k = 0; k < GEMM_N; ++k)
            for (unsigned j = 0; j < GEMM_N; ++j)
                out[i * GEMM_N + j] = out[i * GEMM_N + j] + a[i * GEMM_N + k] * b[k * GEMM_N + j];
    }
}
void KERNEL(fir_f32)(const float *restrict input, const float *restrict taps, float *restrict out) {
    for (unsigned i = 0; i < FIR_N; ++i) out[i] = 0.0f;
    for (unsigned k = 0; k < FIR_TAPS; ++k)
        for (unsigned i = 0; i < FIR_N; ++i) out[i] = out[i] + input[i + k] * taps[k];
}
void KERNEL(fft_f32)(const float *restrict input, float *restrict out) {
    for (unsigned i = 0; i < FFT_N; ++i) {
        unsigned reverse = 0, bits = i;
        for (unsigned bit = 0; bit < 10; ++bit) { reverse = (reverse << 1) | (bits & 1); bits >>= 1; }
        out[reverse] = input[i];
        out[FFT_N + reverse] = input[FFT_N + i];
    }
    for (unsigned length = 2; length <= FFT_N; length *= 2) {
        const unsigned half = length / 2, stride = FFT_N / length;
        for (unsigned block = 0; block < FFT_N; block += length) {
            for (unsigned j = 0; j < half; ++j) {
                const float wr = fft_twiddle[2 * j * stride], wi = fft_twiddle[2 * j * stride + 1];
                const unsigned a = block + j, b = a + half;
                const float br = out[b], bi = out[FFT_N + b];
                const float tr = br * wr - bi * wi, ti = br * wi + bi * wr;
                const float ar = out[a], ai = out[FFT_N + a];
                out[a] = ar + tr; out[FFT_N + a] = ai + ti;
                out[b] = ar - tr; out[FFT_N + b] = ai - ti;
            }
        }
    }
}
