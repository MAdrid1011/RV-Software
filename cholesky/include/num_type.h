#ifndef __NUM_TYPE_H__
#define __NUM_TYPE_H__

#include <stdint.h>
typedef uint32_t float32_t;
typedef uint32_t q31_t;
typedef unsigned long rv_csr_t;

#define Q31_MAX   ((q31_t)(0x7FFFFFFFL))
#define CSR_MCYCLE 0xb00
#define __ASM __asm

#define __RV_CSR_READ(csr)                                      \
    ({                                                          \
        rv_csr_t __v;                                           \
        __ASM volatile("csrr %0, " STRINGIFY(csr)               \
                     : "=r"(__v)                                \
                     :                                          \
                     : "memory");                               \
        __v;                                                    \
    })


typedef struct
{
  uint16_t numRows;     /**< number of rows of the matrix.     */
  uint16_t numCols;     /**< number of columns of the matrix.  */
  uint32_t *pData;     /**< points to the data of the matrix. */
} riscv_matrix_instance_f32;

typedef enum
{
  RISCV_MATH_SUCCESS                 =  0,        /**< No error */
  RISCV_MATH_ARGUMENT_ERROR          = -1,        /**< One or more arguments are incorrect */
  RISCV_MATH_LENGTH_ERROR            = -2,        /**< Length of data buffer is incorrect */
  RISCV_MATH_SIZE_MISMATCH           = -3,        /**< Size of matrices is not compatible with the operation */
  RISCV_MATH_NANINF                  = -4,        /**< Not-a-number (NaN) or infinity is generated */
  RISCV_MATH_SINGULAR                = -5,        /**< Input matrix is singular and cannot be inverted */
  RISCV_MATH_TEST_FAILURE            = -6,        /**< Test Failed */
  RISCV_MATH_DECOMPOSITION_FAILURE   = -7         /**< Decomposition Failed */
} riscv_status;

#endif
