#include "runtime.h"

#define LARGE_N 65536
static uint32_t ix[LARGE_N] __attribute__((aligned(64)));
static uint32_t iy[LARGE_N] __attribute__((aligned(64)));
static uint32_t io[LARGE_N] __attribute__((aligned(64)));
static uint32_t ir[LARGE_N] __attribute__((aligned(64)));
static float fx[LARGE_N] __attribute__((aligned(64)));
static float fy[LARGE_N] __attribute__((aligned(64)));
static float fo[LARGE_N] __attribute__((aligned(64)));
static float fr[LARGE_N] __attribute__((aligned(64)));
void stream_add(const uint32_t *, const uint32_t *, uint32_t *, size_t);
void ref_stream_add(const uint32_t *, const uint32_t *, uint32_t *, size_t);
void stream_saxpy(float, const float *, const float *, float *, size_t);
void ref_stream_saxpy(float, const float *, const float *, float *, size_t);

static uint32_t median(uint32_t elapsed[5]) {
    for (unsigned i = 1; i < 5; ++i)
        for (unsigned j = i; j && elapsed[j] < elapsed[j - 1]; --j) {
            const uint32_t swap = elapsed[j]; elapsed[j] = elapsed[j - 1]; elapsed[j - 1] = swap;
        }
    return elapsed[2];
}

static int measure(const char *name, size_t n, int floating) {
    uint32_t elapsed[5];
    if (floating) {
        ref_stream_saxpy(1.25f, fx, fy, fr, n);
        stream_saxpy(1.25f, fx, fy, fo, n);
    } else {
        ref_stream_add(ix, iy, ir, n);
        stream_add(ix, iy, io, n);
    }
    for (unsigned repeat = 0; repeat < 5; ++repeat) {
        const uint32_t begin = simd_cycle();
        if (floating) stream_saxpy(1.25f, fx, fy, fo, n);
        else stream_add(ix, iy, io, n);
        elapsed[repeat] = simd_cycle() - begin;
        if (floating ? memcmp(fo, fr, n * sizeof(float)) : memcmp(io, ir, n * sizeof(uint32_t))) {
            simd_puts("FAIL "); simd_puts(name); simd_puts("\n"); return 1;
        }
    }
    simd_puts("SIMD_STREAM "); simd_puts(name); simd_puts(" ");
    simd_u32(median(elapsed)); simd_puts("\n");
    return 0;
}

int main(void) {
    const uint32_t begin = simd_cycle();
    for (unsigned i = 0; i < LARGE_N; ++i) {
        ix[i] = 0xfedcba98u + i * 19513u; iy[i] = 0x12345678u - i * 131u;
        fx[i] = (float)((int)(i % 1024) - 512) * 0.03125f;
        fy[i] = (float)((int)(i % 256) - 128) * 0.0625f;
    }
    const int error = measure("add-cache", 512, 0) | measure("saxpy-cache", 512, 1) |
        measure("add-large", LARGE_N, 0) | measure("saxpy-large", LARGE_N, 1);
    simd_puts("SIMD_END_TO_END "); simd_u32(simd_cycle() - begin); simd_puts("\n");
    return error;
}
