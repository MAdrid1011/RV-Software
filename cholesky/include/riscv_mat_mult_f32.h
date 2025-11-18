#ifndef RISCV_MAT_MULT_F32_H_
#define RISCV_MAT_MULT_F32_H_

#include "num_type.h"
#ifdef __cplusplus
extern "C" {
#endif

riscv_status riscv_mat_mult_f32(const riscv_matrix_instance_f32 *pSrcA,
                               const riscv_matrix_instance_f32 *pSrcB,
                               riscv_matrix_instance_f32 *pDst);

#ifdef __cplusplus
}
#endif
#endif /* RISCV_MAT_ADD_F32_H_ */