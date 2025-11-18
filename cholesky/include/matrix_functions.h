#ifndef __MATRIX_FUNCTIONS_H__
#define __MATRIX_FUNCTIONS_H__


#include "num_type.h"
//#include <tool.h>


/**
@brief         Floating-point matrix initialization.
@param[in,out] S         points to an instance of the floating-point matrix structure
@param[in]     nRows     number of rows in the matrix
@param[in]     nColumns  number of columns in the matrix
@param[in]     pData     points to the matrix data array
*/

// static inline void do_srand(void)
// {
//     unsigned long randvar = __RV_CSR_READ(CSR_MCYCLE);
//     srand(randvar);
//     // printf("srandvar is %d\n", randvar);
// }
// static void generate_rand_f32(float32_t *src, int length)
// {
//     do_srand();
//     for (int i = 0; i < length; i++)
//     {
//         src[i] = (float32_t)((rand() % Q31_MAX - Q31_MAX / 2) * 1.0 / Q31_MAX);
//     }
// }

//断言判断
#define TEST_ASSERT_EQUAL(expected, actual) \
do { \
    if ((expected) != (actual)) { \
        printf("Assertion failed: %s != %s, file %s, line %d\n", #expected, #actual, __FILE__, __LINE__); \
    } \
} while(0)


#define SCALE_COL_F32(A,ROW,v,i)        \
    SCALE_COL_T(float32_t,,A,ROW,v,i)

#define SCALE_COL_T(T,CAST,A,ROW,v,i)        \
{                                       \
  int32_t _w;                            \
  T *data = (A)->pData;                 \
  const int32_t _numCols = (A)->numCols; \
  const int32_t nb = (A)->numRows - ROW;\
                                        \
  data += i + _numCols * (ROW);          \
                                        \
  for(_w=0;_w < nb; _w++)                  \
  {                                     \
     *data *= CAST v;                   \
     data += _numCols;                   \
  }                                     \
}

#endif

