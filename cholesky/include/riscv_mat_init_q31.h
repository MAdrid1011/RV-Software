#ifndef RISCV_MAT_INIT_F32_H_
#define RISCV_MAT_INIT_F32_H_ 

#include "num_type.h"
#ifdef __cplusplus
extern "C" {
#endif
void riscv_mat_init_f32(
    riscv_matrix_instance_f32 * S,
    uint16_t nRows,
    uint16_t nColumns,
    float32_t * pData);

#ifdef __cplusplus
}
#endif
#endif 