#ifndef ZIRCON_SIMD_KERNELS_H
#define ZIRCON_SIMD_KERNELS_H
#include <stdint.h>
#define GEMM_N 64
#define FIR_N 4096
#define FIR_TAPS 32
#define FFT_N 1024
void gemm_i32(const uint32_t *a, const uint32_t *b, uint32_t *out);
void fir_i32(const uint32_t *input, const uint32_t *taps, uint32_t *out);
void gemm_f32(const float *a, const float *b, float *out);
void fir_f32(const float *input, const float *taps, float *out);
void fft_f32(const float *input, float *out);
void ref_gemm_i32(const uint32_t *a, const uint32_t *b, uint32_t *out);
void ref_fir_i32(const uint32_t *input, const uint32_t *taps, uint32_t *out);
void ref_gemm_f32(const float *a, const float *b, float *out);
void ref_fir_f32(const float *input, const float *taps, float *out);
void ref_fft_f32(const float *input, float *out);
extern const float fft_twiddle[FFT_N];
#endif
