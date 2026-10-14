#include "runtime.h"

// Sixteen independent vector destinations hide arithmetic latency. rdcycle
// brackets only register arithmetic and loop control, excluding setup/Store.
#define SIXTEEN(OP) OP " v8, v0, v1\n" OP " v9, v0, v1\n" OP " v10, v0, v1\n" \
    OP " v11, v0, v1\n" OP " v12, v0, v1\n" OP " v13, v0, v1\n" \
    OP " v14, v0, v1\n" OP " v15, v0, v1\n" \
    OP " v16, v0, v1\n" OP " v17, v0, v1\n" OP " v18, v0, v1\n" \
    OP " v19, v0, v1\n" OP " v20, v0, v1\n" OP " v21, v0, v1\n" \
    OP " v22, v0, v1\n" OP " v23, v0, v1\n"
#define INIT "vmv.v.v v9, v8\nvmv.v.v v10, v8\nvmv.v.v v11, v8\n" \
    "vmv.v.v v12, v8\nvmv.v.v v13, v8\nvmv.v.v v14, v8\nvmv.v.v v15, v8\n" \
    "vmv.v.v v16, v8\nvmv.v.v v17, v8\nvmv.v.v v18, v8\nvmv.v.v v19, v8\n" \
    "vmv.v.v v20, v8\nvmv.v.v v21, v8\nvmv.v.v v22, v8\nvmv.v.v v23, v8\n"
#define CLOBBERS "memory", "v0", "v1", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15", \
    "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23"
#define FP_THROUGHPUT(NAME, OP, EXPECTED) \
static int NAME(void) { \
    unsigned count = 256; uint32_t begin, end; float result[8]; \
    const float x = 1.0f, y = 0.0009765625f, seed = 0.25f; \
    __asm__ volatile("vsetivli zero, 8, e32, m1, ta, ma\n" \
        "vfmv.v.f v0, %[x]\nvfmv.v.f v1, %[y]\nvfmv.v.f v8, %[seed]\n" INIT \
        "rdcycle %[begin]\n1:\n" SIXTEEN(OP) "addi %[count], %[count], -1\n" \
        "bnez %[count], 1b\nrdcycle %[end]\nvse32.v v8, (%[out])\n" \
        : [count] "+r"(count), [begin] "=&r"(begin), [end] "=&r"(end) \
        : [x] "f"(x), [y] "f"(y), [seed] "f"(seed), [out] "r"(result) : CLOBBERS); \
    for (unsigned i = 0; i < 8; ++i) if (result[i] != (EXPECTED)) return 1; \
    simd_puts("SIMD_REGISTER " #NAME " "); simd_u32(end - begin); \
    simd_puts(" elements=32768\n"); return 0; \
}
FP_THROUGHPUT(fp32_add, "vfadd.vv", 1.0009765625f)
FP_THROUGHPUT(fp32_mul, "vfmul.vv", 0.0009765625f)
FP_THROUGHPUT(fp32_fma, "vfmacc.vv", 0.5f)

static int int32_add(void) {
    unsigned count = 256; uint32_t begin, end, result[8];
    __asm__ volatile("vsetivli zero, 8, e32, m1, ta, ma\n"
        "vmv.v.i v0, 1\nvmv.v.i v1, 3\nvmv.v.i v8, 0\n" INIT
        "rdcycle %[begin]\n1:\n" SIXTEEN("vadd.vv") "addi %[count], %[count], -1\n"
        "bnez %[count], 1b\nrdcycle %[end]\nvse32.v v8, (%[out])\n"
        : [count] "+r"(count), [begin] "=&r"(begin), [end] "=&r"(end)
        : [out] "r"(result) : CLOBBERS);
    for (unsigned i = 0; i < 8; ++i) if (result[i] != 4) return 1;
    simd_puts("SIMD_REGISTER int32_add "); simd_u32(end - begin);
    simd_puts(" elements=32768\n"); return 0;
}

int main(void) {
    const int error = int32_add() | fp32_add() | fp32_mul() | fp32_fma();
    simd_puts(error ? "FAIL register arithmetic\n" : "PASS register arithmetic\n");
    return error;
}
