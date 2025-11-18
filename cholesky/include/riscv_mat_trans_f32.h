#ifndef RISCV_MAT_TRANS_F32_H_
#define RISCV_MAT_TRANS_F32_H_

#include "num_type.h"
#ifdef __cplusplus
extern "C" {
#endif

riscv_status riscv_mat_trans_f32(const riscv_matrix_instance_f32 *pSrc,
                                 riscv_matrix_instance_f32 *pDst);

#ifdef __cplusplus
}
#endif
#endif /* RISCV_MAT_TRANS_F32_H_ */