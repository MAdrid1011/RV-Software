#include "runtime.h"
#include <riscv_vector.h>

/* Row-major C[32][32] = A[32][256] * B[256][32]. Each output
 * accumulates k in ascending order with one rounding per fused operation.
 * K-loop timing includes loads, address generation and loop control.
 */
enum { M = 32, K = 256, N = 32, FMA_ELEMENTS = M * K * N };
#ifndef FMA_K_TILE
#define FMA_K_TILE 32
#endif
_Static_assert(FMA_K_TILE >= 2 && K % FMA_K_TILE == 0 && FMA_K_TILE % 2 == 0,
               "K tile must be an even divisor of 256");
#define FMA_STRING_IMPL(value) #value
#define FMA_STRING(value) FMA_STRING_IMPL(value)
#define SEVEN_ROW_NAME "seven-row-tiled-k" FMA_STRING(FMA_K_TILE) "-m4"
static float a[M * K] __attribute__((aligned(64)));
static float b[K * N] __attribute__((aligned(64)));
static float c[M * N] __attribute__((aligned(64)));
static float reference[M * N] __attribute__((aligned(64)));

/* Inline reads keep live vector accumulators out of call-clobbered spills. */
static inline uint32_t ticks(void) {
    uint32_t value;
    __asm__ volatile("rdcycle %0" : "=r"(value) :: "memory");
    return value;
}

__attribute__((target("arch=rv32imf_zicsr_zicntr"), noinline))
static void scalar_reference(void) {
    for (unsigned i = 0; i < M; ++i)
        for (unsigned j = 0; j < N; ++j) {
            float acc = 0;
            for (unsigned k = 0; k < K; ++k)
                __asm__("fmadd.s %0, %1, %2, %0" : "+f"(acc)
                        : "f"(a[i * K + k]), "f"(b[k * N + j]));
            reference[i * N + j] = acc;
        }
}

__attribute__((noinline))
static uint32_t simple_m1(void) {
    uint32_t body = 0;
    for (unsigned i = 0; i < M; ++i)
        for (unsigned j = 0; j < N; j += 8) {
            vfloat32m1_t acc = __riscv_vfmv_v_f_f32m1(0, 8);
            __asm__ volatile("" :: "vr"(acc) : "memory");
            uint32_t begin = ticks();
            for (unsigned k = 0; k < K; ++k) {
                vfloat32m1_t bv = __riscv_vle32_v_f32m1(b + k * N + j, 8);
                acc = __riscv_vfmacc_vf_f32m1(acc, a[i * K + k], bv, 8);
            }
            __asm__ volatile("" :: "vr"(acc) : "memory");
            body += ticks() - begin;
            __riscv_vse32_v_f32m1(c + i * N + j, acc, 8);
        }
    return body;
}

__attribute__((noinline))
static uint32_t blocked_m4(void) {
    uint32_t body = 0;
    for (unsigned i = 0; i < M; i += 4) {
        vfloat32m4_t c0 = __riscv_vfmv_v_f_f32m4(0, N);
        vfloat32m4_t c1 = c0, c2 = c0, c3 = c0;
        __asm__ volatile("" :: "vr"(c0), "vr"(c1), "vr"(c2), "vr"(c3) : "memory");
        uint32_t begin = ticks();
        for (unsigned k = 0; k < K; ++k) {
            vfloat32m4_t bv = __riscv_vle32_v_f32m4(b + k * N, N);
            c0 = __riscv_vfmacc_vf_f32m4(c0, a[(i + 0) * K + k], bv, N);
            c1 = __riscv_vfmacc_vf_f32m4(c1, a[(i + 1) * K + k], bv, N);
            c2 = __riscv_vfmacc_vf_f32m4(c2, a[(i + 2) * K + k], bv, N);
            c3 = __riscv_vfmacc_vf_f32m4(c3, a[(i + 3) * K + k], bv, N);
        }
        __asm__ volatile("" :: "vr"(c0), "vr"(c1), "vr"(c2), "vr"(c3) : "memory");
        body += ticks() - begin;
        __riscv_vse32_v_f32m4(c + (i + 0) * N, c0, N);
        __riscv_vse32_v_f32m4(c + (i + 1) * N, c1, N);
        __riscv_vse32_v_f32m4(c + (i + 2) * N, c2, N);
        __riscv_vse32_v_f32m4(c + (i + 3) * N, c3, N);
    }
    return body;
}

__attribute__((noinline))
static uint32_t cache_tiled_m4(void) {
    uint32_t body = 0;
    for (unsigned kb = 0; kb < K; kb += 32)
        for (unsigned i = 0; i < M; i += 4) {
            vfloat32m4_t c0, c1, c2, c3;
            if (kb) {
                c0 = __riscv_vle32_v_f32m4(c + (i + 0) * N, N);
                c1 = __riscv_vle32_v_f32m4(c + (i + 1) * N, N);
                c2 = __riscv_vle32_v_f32m4(c + (i + 2) * N, N);
                c3 = __riscv_vle32_v_f32m4(c + (i + 3) * N, N);
            } else {
                c0 = __riscv_vfmv_v_f_f32m4(0, N);
                c1 = c0; c2 = c0; c3 = c0;
            }
            __asm__ volatile("" :: "vr"(c0), "vr"(c1), "vr"(c2), "vr"(c3) : "memory");
            uint32_t begin = ticks();
            for (unsigned k = kb; k < kb + 32; ++k) {
                vfloat32m4_t bv = __riscv_vle32_v_f32m4(b + k * N, N);
                c0 = __riscv_vfmacc_vf_f32m4(c0, a[(i + 0) * K + k], bv, N);
                c1 = __riscv_vfmacc_vf_f32m4(c1, a[(i + 1) * K + k], bv, N);
                c2 = __riscv_vfmacc_vf_f32m4(c2, a[(i + 2) * K + k], bv, N);
                c3 = __riscv_vfmacc_vf_f32m4(c3, a[(i + 3) * K + k], bv, N);
            }
            __asm__ volatile("" :: "vr"(c0), "vr"(c1), "vr"(c2), "vr"(c3) : "memory");
            body += ticks() - begin;
            __riscv_vse32_v_f32m4(c + (i + 0) * N, c0, N);
            __riscv_vse32_v_f32m4(c + (i + 1) * N, c1, N);
            __riscv_vse32_v_f32m4(c + (i + 2) * N, c2, N);
            __riscv_vse32_v_f32m4(c + (i + 3) * N, c3, N);
        }
    return body;
}

/* Fixed-VLEN microkernel keeps vtype outside the K loop. Scalar operands
 * are loaded together before the four independent accumulators are issued.
 */
__attribute__((noinline))
static uint32_t scheduled_m4(void) {
    uint32_t body = 0;
    for (unsigned i = 0; i < M; i += 4) {
        const float *ap = a + (i + 2) * K, *bp = b;
        float *cp = c + i * N;
        unsigned count = K;
        uint32_t begin, end;
        __asm__ volatile(
            "vsetvli zero, %[vl], e32, m4, ta, ma\n"
            "vmv.v.i v8, 0\nvmv.v.i v12, 0\nvmv.v.i v16, 0\nvmv.v.i v20, 0\n"
            "rdcycle %[begin]\n1:\n"
            "vle32.v v24, (%[bp])\n"
            "flw ft0, -2048(%[ap])\nflw ft1, -1024(%[ap])\n"
            "flw ft2, 0(%[ap])\nflw ft3, 1024(%[ap])\n"
            "vfmacc.vf v8, ft0, v24\nvfmacc.vf v12, ft1, v24\n"
            "vfmacc.vf v16, ft2, v24\nvfmacc.vf v20, ft3, v24\n"
            "addi %[ap], %[ap], 4\naddi %[bp], %[bp], 128\n"
            "addi %[count], %[count], -1\nbnez %[count], 1b\n"
            "rdcycle %[end]\n"
            "vse32.v v8, (%[cp])\naddi %[cp], %[cp], 128\n"
            "vse32.v v12, (%[cp])\naddi %[cp], %[cp], 128\n"
            "vse32.v v16, (%[cp])\naddi %[cp], %[cp], 128\n"
            "vse32.v v20, (%[cp])\n"
            : [ap] "+r"(ap), [bp] "+r"(bp), [cp] "+r"(cp), [count] "+r"(count),
              [begin] "=&r"(begin), [end] "=&r"(end)
            : [vl] "r"(N)
            : "memory", "ft0", "ft1", "ft2", "ft3", "v8", "v9", "v10", "v11",
              "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
              "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27");
        body += end - begin;
    }
    return body;
}

/* Two K iterations use alternating B groups, balancing physical-register
 * banks while the next Load overlaps the preceding accumulator operations.
 * No packing is required; each output preserves ascending-K FMA order.
 */
__attribute__((noinline))
static uint32_t pipelined_tiled_m4(void) {
    uint32_t body = 0;
    for (unsigned kb = 0; kb < K; kb += 32)
        for (unsigned i = 0; i < M; i += 4) {
            const float *ap = a + (i + 2) * K + kb, *bp = b + kb * N;
            float *cp = c + i * N;
            unsigned count = 16;
            uint32_t begin, end;
            __asm__ volatile("vsetvli zero, %0, e32, m4, ta, ma" :: "r"(N));
            if (!kb) {
                __asm__ volatile("vmv.v.i v8, 0\nvmv.v.i v12, 0\nvmv.v.i v16, 0\nvmv.v.i v20, 0"
                    ::: "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
                    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
            } else {
                __asm__ volatile("vle32.v v8, (%0)\naddi t0, %0, 128\nvle32.v v12, (t0)\n"
                    "addi t0, t0, 128\nvle32.v v16, (t0)\naddi t0, t0, 128\nvle32.v v20, (t0)"
                    :: "r"(cp) : "memory", "t0", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",
                    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23");
            }
            __asm__ volatile(
                "rdcycle %[begin]\n1:\nvle32.v v24, (%[bp])\n"
                "flw ft0, -2048(%[ap])\nflw ft1, -1024(%[ap])\nflw ft2, 0(%[ap])\nflw ft3, 1024(%[ap])\n"
                "addi t0, %[bp], 128\nvle32.v v28, (t0)\n"
                "flw ft4, -2044(%[ap])\nflw ft5, -1020(%[ap])\nflw ft6, 4(%[ap])\nflw ft7, 1028(%[ap])\n"
                "vfmacc.vf v8, ft0, v24\nvfmacc.vf v12, ft1, v24\n"
                "vfmacc.vf v16, ft2, v24\nvfmacc.vf v20, ft3, v24\n"
                "vfmacc.vf v8, ft4, v28\nvfmacc.vf v12, ft5, v28\n"
                "vfmacc.vf v16, ft6, v28\nvfmacc.vf v20, ft7, v28\n"
                "addi %[ap], %[ap], 8\naddi %[bp], %[bp], 256\n"
                "addi %[count], %[count], -1\nbnez %[count], 1b\nrdcycle %[end]\n"
                "vse32.v v8, (%[cp])\naddi %[cp], %[cp], 128\nvse32.v v12, (%[cp])\n"
                "addi %[cp], %[cp], 128\nvse32.v v16, (%[cp])\n"
                "addi %[cp], %[cp], 128\nvse32.v v20, (%[cp])"
                : [ap] "+r"(ap), [bp] "+r"(bp), [cp] "+r"(cp), [count] "+r"(count),
                  [begin] "=&r"(begin), [end] "=&r"(end)
                :: "memory", "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", "v8", "v9", "v10", "v11",
                   "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19",
                   "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
            body += end - begin;
        }
    return body;
}

/* Seven output rows reuse each B group before advancing K. The final panel
 * has four rows; its inactive accumulator groups never access memory.
 */
__attribute__((noinline))
static uint32_t seven_row_tiled_m4(void) {
    uint32_t body = 0;
    for (unsigned kb = 0; kb < K; kb += FMA_K_TILE)
        for (unsigned i = 0; i < M; i += 7) {
            const unsigned rows = i + 7 <= M ? 7 : 4;
            const float *ap0 = a + i * K + kb, *ap3 = ap0 + 3 * K;
            const float *ap6 = rows == 7 ? ap0 + 6 * K : ap0;
            const float *bp = b + kb * N;
            float *cp = c + i * N;
            unsigned count = FMA_K_TILE;
            uint32_t begin, end;
            __asm__ volatile(
                "vsetvli zero, %[vl], e32, m4, ta, ma\n"
                "bnez %[kb], 2f\n"
                "vmv.v.i v0, 0\n"
                "vmv.v.i v4, 0\n"
                "vmv.v.i v8, 0\n"
                "vmv.v.i v12, 0\n"
                "vmv.v.i v16, 0\n"
                "vmv.v.i v20, 0\n"
                "vmv.v.i v24, 0\n"
                "j 3f\n2:\nmv t0, %[cp]\n"
                "vle32.v v0, (t0)\naddi t0, t0, 128\n"
                "vle32.v v4, (t0)\naddi t0, t0, 128\n"
                "vle32.v v8, (t0)\naddi t0, t0, 128\n"
                "vle32.v v12, (t0)\naddi t0, t0, 128\n"
                "beq %[rows], %[four], 3f\n"
                "vle32.v v16, (t0)\naddi t0, t0, 128\n"
                "vle32.v v20, (t0)\naddi t0, t0, 128\n"
                "vle32.v v24, (t0)\n"
                "3:\nrdcycle %[begin]\n1:\nvle32.v v28, (%[bp])\n"
                "flw ft0, 0(%[ap0])\nflw ft1, 1024(%[ap0])\n"
                "addi t0, %[ap0], 1024\nflw ft2, 1024(t0)\n"
                "flw ft3, 0(%[ap3])\n"
                "beq %[rows], %[four], 6f\n"
                "flw ft4, 1024(%[ap3])\naddi t0, %[ap3], 1024\nflw ft5, 1024(t0)\nflw ft6, 0(%[ap6])\n"
                "6:\n"
                "vfmacc.vf v0, ft0, v28\n"
                "vfmacc.vf v4, ft1, v28\n"
                "vfmacc.vf v8, ft2, v28\n"
                "vfmacc.vf v12, ft3, v28\n"
                "beq %[rows], %[four], 4f\n"
                "vfmacc.vf v16, ft4, v28\n"
                "vfmacc.vf v20, ft5, v28\n"
                "vfmacc.vf v24, ft6, v28\n"
                "4:\naddi %[ap0], %[ap0], 4\naddi %[ap3], %[ap3], 4\naddi %[ap6], %[ap6], 4\n"
                "addi %[bp], %[bp], 128\naddi %[count], %[count], -1\nbnez %[count], 1b\n"
                "rdcycle %[end]\n"
                "vse32.v v0, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v4, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v8, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v12, (%[cp])\naddi %[cp], %[cp], 128\n"
                "beq %[rows], %[four], 5f\n"
                "vse32.v v16, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v20, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v24, (%[cp])\n"
                "5:\n"
                : [ap0] "+&r"(ap0), [ap3] "+&r"(ap3), [ap6] "+&r"(ap6),
                  [bp] "+&r"(bp), [cp] "+&r"(cp), [count] "+&r"(count),
                  [begin] "=&r"(begin), [end] "=&r"(end)
                : [vl] "r"(N), [kb] "r"(kb), [rows] "r"(rows), [four] "r"(4)
                : "memory", "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6",
                  "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
            body += end - begin;
        }
    return body;
}

/* Six rows leave two B register groups for successive K iterations. */
__attribute__((noinline))
static uint32_t six_row_pipelined_m4(void) {
    uint32_t body = 0;
    for (unsigned kb = 0; kb < K; kb += 32)
        for (unsigned i = 0; i < M; i += 6) {
            const unsigned rows = i + 6 <= M ? 6 : 2;
            const float *ap0 = a + i * K + kb;
            const float *ap3 = rows == 6 ? ap0 + 3 * K : ap0;
            const float *bp = b + kb * N;
            float *cp = c + i * N;
            unsigned count = 16;
            uint32_t begin, end;
            __asm__ volatile(
                "vsetvli zero, %[vl], e32, m4, ta, ma\n"
                "bnez %[kb], 2f\n"
                "vmv.v.i v0, 0\n"
                "vmv.v.i v4, 0\n"
                "vmv.v.i v8, 0\n"
                "vmv.v.i v12, 0\n"
                "vmv.v.i v16, 0\n"
                "vmv.v.i v20, 0\n"
                "j 3f\n2:\nmv t0, %[cp]\n"
                "vle32.v v0, (t0)\naddi t0, t0, 128\nvle32.v v4, (t0)\n"
                "beq %[rows], %[two], 3f\naddi t0, t0, 128\n"
                "vle32.v v8, (t0)\naddi t0, t0, 128\n"
                "vle32.v v12, (t0)\naddi t0, t0, 128\n"
                "vle32.v v16, (t0)\naddi t0, t0, 128\n"
                "vle32.v v20, (t0)\n"
                "3:\nrdcycle %[begin]\n1:\nvle32.v v24, (%[bp])\n"
                "flw ft0, 0(%[ap0])\nflw ft1, 1024(%[ap0])\n"
                "beq %[rows], %[two], 6f\n"
                "addi t0, %[ap0], 1024\nflw ft2, 1024(t0)\n"
                "flw ft3, 0(%[ap3])\nflw ft4, 1024(%[ap3])\n"
                "addi t0, %[ap3], 1024\nflw ft5, 1024(t0)\n"
                "6:\naddi t0, %[bp], 128\nvle32.v v28, (t0)\n"
                "vfmacc.vf v0, ft0, v24\nvfmacc.vf v4, ft1, v24\n"
                "beq %[rows], %[two], 4f\n"
                "vfmacc.vf v8, ft2, v24\n"
                "vfmacc.vf v12, ft3, v24\n"
                "vfmacc.vf v16, ft4, v24\n"
                "vfmacc.vf v20, ft5, v24\n"
                "4:\nflw ft0, 4(%[ap0])\nflw ft1, 1028(%[ap0])\n"
                "beq %[rows], %[two], 7f\n"
                "addi t0, %[ap0], 1024\nflw ft2, 1028(t0)\n"
                "flw ft3, 4(%[ap3])\nflw ft4, 1028(%[ap3])\n"
                "addi t0, %[ap3], 1024\nflw ft5, 1028(t0)\n"
                "7:\nvfmacc.vf v0, ft0, v28\nvfmacc.vf v4, ft1, v28\n"
                "beq %[rows], %[two], 8f\n"
                "vfmacc.vf v8, ft2, v28\n"
                "vfmacc.vf v12, ft3, v28\n"
                "vfmacc.vf v16, ft4, v28\n"
                "vfmacc.vf v20, ft5, v28\n"
                "8:\naddi %[ap0], %[ap0], 8\naddi %[ap3], %[ap3], 8\n"
                "addi %[bp], %[bp], 256\naddi %[count], %[count], -1\nbnez %[count], 1b\n"
                "rdcycle %[end]\n"
                "vse32.v v0, (%[cp])\naddi %[cp], %[cp], 128\nvse32.v v4, (%[cp])\n"
                "beq %[rows], %[two], 5f\naddi %[cp], %[cp], 128\n"
                "vse32.v v8, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v12, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v16, (%[cp])\naddi %[cp], %[cp], 128\n"
                "vse32.v v20, (%[cp])\n"
                "5:\n"
                : [ap0] "+&r"(ap0), [ap3] "+&r"(ap3), [bp] "+&r"(bp), [cp] "+&r"(cp),
                  [count] "+&r"(count), [begin] "=&r"(begin), [end] "=&r"(end)
                : [vl] "r"(N), [kb] "r"(kb), [rows] "r"(rows), [two] "r"(2)
                : "memory", "t0", "ft0", "ft1", "ft2", "ft3", "ft4", "ft5",
                  "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23", "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");
            body += end - begin;
        }
    return body;
}

#define COLS(OP, R) OP(R, 0) OP(R, 1) OP(R, 2) OP(R, 3)
#define ALL(OP) COLS(OP, 0) COLS(OP, 1) COLS(OP, 2) COLS(OP, 3)
#define DECL(R, J) vfloat32m1_t c##R##J = __riscv_vfmv_v_f_f32m1(0, 8);
#define FMA(R, J) c##R##J = __riscv_vfmacc_vv_f32m1(c##R##J, av, b##J, 8);
#define STORE(R, J) __riscv_vse32_v_f32m1(c + (i + R) * N + J * 8, c##R##J, 8);
#define KEEP(R, J) __asm__ volatile("" :: "vr"(c##R##J) : "memory");
__attribute__((noinline))
static uint32_t blocked_m1(void) {
    uint32_t body = 0;
    for (unsigned i = 0; i < M; i += 4) {
        ALL(DECL)
        ALL(KEEP)
        uint32_t begin = ticks();
        for (unsigned k = 0; k < K; ++k) {
            vfloat32m1_t b0 = __riscv_vle32_v_f32m1(b + k * N + 0, 8);
            vfloat32m1_t b1 = __riscv_vle32_v_f32m1(b + k * N + 8, 8);
            vfloat32m1_t b2 = __riscv_vle32_v_f32m1(b + k * N + 16, 8);
            vfloat32m1_t b3 = __riscv_vle32_v_f32m1(b + k * N + 24, 8);
            vfloat32m1_t av = __riscv_vfmv_v_f_f32m1(a[(i + 0) * K + k], 8);
            COLS(FMA, 0)
            av = __riscv_vfmv_v_f_f32m1(a[(i + 1) * K + k], 8);
            COLS(FMA, 1)
            av = __riscv_vfmv_v_f_f32m1(a[(i + 2) * K + k], 8);
            COLS(FMA, 2)
            av = __riscv_vfmv_v_f_f32m1(a[(i + 3) * K + k], 8);
            COLS(FMA, 3)
        }
        ALL(KEEP)
        body += ticks() - begin;
        ALL(STORE)
    }
    return body;
}

static int measure(const char *name, uint32_t (*kernel)(void), unsigned vl) {
    uint32_t bodies[5], totals[5];
    for (unsigned repeat = 0; repeat < 6; ++repeat) {
        uint32_t begin = simd_cycle();
        uint32_t body = kernel();
        uint32_t total = simd_cycle() - begin;
        if (memcmp(c, reference, sizeof(c))) {
            simd_puts("FAIL FMA_GEMM "); simd_puts(name); simd_puts("\n");
            return 1;
        }
        if (repeat) { bodies[repeat - 1] = body; totals[repeat - 1] = total; }
    }
    for (unsigned i = 1; i < 5; ++i)
        for (unsigned j = i; j && bodies[j] < bodies[j - 1]; --j) {
            uint32_t t = bodies[j]; bodies[j] = bodies[j - 1]; bodies[j - 1] = t;
            t = totals[j]; totals[j] = totals[j - 1]; totals[j - 1] = t;
        }
    simd_puts("FMA_GEMM "); simd_puts(name);
    simd_puts(" body_cycles="); simd_u32(bodies[2]);
    simd_puts(" total_cycles="); simd_u32(totals[2]);
    simd_puts(" useful_fma_elements="); simd_u32(FMA_ELEMENTS);
    simd_puts(" vector_fma_instructions="); simd_u32(FMA_ELEMENTS / vl);
    simd_puts(" utilization_basis_points=");
    simd_u32((FMA_ELEMENTS / 8u * 10000u) / bodies[2]);
    simd_puts("\n");
    return 0;
}

int main(void) {
    for (unsigned i = 0; i < M * K; ++i) a[i] = ((int)(i % 23) - 11) * 0.125f;
    for (unsigned i = 0; i < K * N; ++i) b[i] = ((int)(i % 17) - 8) * 0.0625f;
    uint32_t begin = simd_cycle();
    scalar_reference();
    simd_puts("FMA_GEMM scalar_cycles="); simd_u32(simd_cycle() - begin); simd_puts("\n");
#ifdef FMA_GEMM_FOCUS
    simd_puts("FMA_GEMM kernels=seven-row k_tile="); simd_u32(FMA_K_TILE); simd_puts("\n");
    int error = measure(SEVEN_ROW_NAME, seven_row_tiled_m4, 32);
#else
    simd_puts("FMA_GEMM kernels=all k_tile="); simd_u32(FMA_K_TILE); simd_puts("\n");
    int error = measure("simple-m1", simple_m1, 8);
    error |= measure("blocked-4x32-m4", blocked_m4, 32);
    error |= measure("blocked-4x32-m1", blocked_m1, 8);
    error |= measure("scheduled-4x32-m4", scheduled_m4, 32);
    error |= measure("cache-tiled-k32-m4", cache_tiled_m4, 32);
    error |= measure("pipelined-tiled-k32-m4", pipelined_tiled_m4, 32);
    error |= measure(SEVEN_ROW_NAME, seven_row_tiled_m4, 32);
    error |= measure("six-row-pipelined-k32-m4", six_row_pipelined_m4, 32);
#endif
    simd_puts(error ? "FAIL FMA_GEMM\n" : "PASS FMA_GEMM\n");
    return error;
}
