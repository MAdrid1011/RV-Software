
#include "num_type.h"
#include "sqrt_newton.h"
#include "matrix_functions.h"
riscv_status riscv_mat_cholesky_f32(
  const riscv_matrix_instance_f32 * pSrc,
        riscv_matrix_instance_f32 * pDst)
{

  riscv_status status;                             /* status of matrix inverse */


#ifdef RISCV_MATH_MATRIX_CHECK

  /* Check for matrix mismatch condition */
  if ((pSrc->numRows != pSrc->numCols) ||
      (pDst->numRows != pDst->numCols) ||
      (pSrc->numRows != pDst->numRows)   )
  {
    /* Set status as RISCV_MATH_SIZE_MISMATCH */
    status = RISCV_MATH_SIZE_MISMATCH;
  }
  else

#endif /* #ifdef RISCV_MATH_MATRIX_CHECK */

  {
    int i,j,k;
    int n = pSrc->numRows;
    float32_t invSqrtVj;
    float32_t *pA,*pG;

    pA = pSrc->pData;
    pG = pDst->pData;


    for (i = 0 ; i < n; i++)
    {
       for (j = i ; j < n; j++)
       {
          pG[j * n + i] = pA[j * n + i];
          for(k=0; k < i ; k++)
          {
             pG[j * n + i] = pG[j * n + i] - pG[i * n + k] * pG[j * n + k];
          }
       }

       if (pG[i * n + i] <= 0.0f)
       {
         return(RISCV_MATH_DECOMPOSITION_FAILURE);
       }

       invSqrtVj = 1/sqrt_q31_newton(pG[i * n + i]);
       SCALE_COL_F32(pDst,i,invSqrtVj,i);

    }

    status = RISCV_MATH_SUCCESS;

  }


  /* Return to application */
  return (status);
  //return RISCV_MATH_SUCCESS;
}

/**
  @} end of MatrixChol group
 */

 