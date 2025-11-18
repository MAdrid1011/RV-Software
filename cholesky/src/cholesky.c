#include "matrix_functions.h"
#include "test_data.h"
#include "cholesky.h"
#include "riscv_mat_init_q31.h"
//#include "generate_posi_def_symme_f32.c"
#include "riscv_mat_trans_f32.h"
#include "riscv_mat_mult_f32.h"
#include "riscv_mat_add_f32.h"

//BENCH_DECLARE_VAR();
void generate_posi_def_symme_f32(const riscv_matrix_instance_f32 *pSrc, riscv_matrix_instance_f32 *pUnitMat,
    riscv_matrix_instance_f32 *pDot,  riscv_matrix_instance_f32 *pDst)
{
    //int dim = pSrc->numRows;
    riscv_mat_trans_f32(pSrc, pDst);
    riscv_mat_mult_f32(pDst, pSrc, pDot);
    riscv_mat_add_f32(pDot, pUnitMat, pDst);
}
int main(void)
{
    float32_t f32_output[M * M];

    float32_t tmp = 0x3A5F7821;
    riscv_matrix_instance_f32 f32_A;
    riscv_matrix_instance_f32 f32_des;
    riscv_matrix_instance_f32 f32_posi;
    riscv_matrix_instance_f32 f32_dot;
    riscv_matrix_instance_f32 f32_tmp;
    riscv_mat_init_f32(&f32_A, M, M, f32_input_array);
    riscv_mat_init_f32(&f32_des, M, M, f32_output);
    riscv_mat_init_f32(&f32_posi, M, M, f32_posi_array);
    riscv_mat_init_f32(&f32_dot, M, M, f32_dot_array);
    riscv_mat_init_f32(&f32_tmp, M, M, f32_tmp_array);
    //赋值
    for (int i = 0; i < M * M; i++) {
        f32_input_array[i] = predefined_input_data[i];
        f32_output[i] = predefined_output_data[i];
    }

    for (int i = 0; i < M; i++) {
        for (int j = 0; j < M; j++) {
            if (i == j) {
                f32_tmp_array[i * M + j] = tmp;
            }
        }
    }
    // int num =1000;
    // riscv_status result [num];
    // for (int i = 0; i < num; i++){
    // generate_posi_def_symme_f32(&f32_A, &f32_tmp, &f32_dot, &f32_posi);
    // //BENCH_START(riscv_mat_cholesky_f32);
    //  result [i]= riscv_mat_cholesky_f32(&f32_posi, &f32_des);
    // //BENCH_END(riscv_mat_cholesky_f32);
    // TEST_ASSERT_EQUAL(RISCV_MATH_SUCCESS, result[i]);
    // }

    generate_posi_def_symme_f32(&f32_A, &f32_tmp, &f32_dot, &f32_posi);
    riscv_status result = riscv_mat_cholesky_f32(&f32_posi, &f32_des);
    // TEST_ASSERT_EQUAL(RISCV_MATH_SUCCESS, result);

    return result;
}




riscv_status riscv_mat_trans_f32(
    const riscv_matrix_instance_f32 * pSrc,
          riscv_matrix_instance_f32 * pDst)
  {
    float32_t *pIn = pSrc->pData;                  /* input data matrix pointer */
    float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
    float32_t *px;                                 /* Temporary output data matrix pointer */
    uint16_t nRows = pSrc->numRows;                /* number of rows */
    uint16_t nCols = pSrc->numCols;                /* number of columns */
    uint32_t col, row = nRows, i = 0U;             /* Loop counters */
    riscv_status status;                             /* status of matrix transpose */
  
  #ifdef RISCV_MATH_MATRIX_CHECK
  
    /* Check for matrix mismatch condition */
    if ((pSrc->numRows != pDst->numCols) ||
        (pSrc->numCols != pDst->numRows)   )
    {
      /* Set status as RISCV_MATH_SIZE_MISMATCH */
      status = RISCV_MATH_SIZE_MISMATCH;
    }
    else
  
  #endif /* #ifdef RISCV_MATH_MATRIX_CHECK */
    {
  #if defined(RISCV_MATH_VECTOR)
      uint32_t blkCnt = nRows;
      size_t l;
      ptrdiff_t bstride = 4;  //  32bit/8bit = 4
      ptrdiff_t col_diff = bstride * nCols;
      uint16_t colnum;
      vfloat32m8_t v_in;
      float32_t *pIn1;
  
      for(colnum = 0; colnum < nCols; colnum++)
      {
        blkCnt = nRows;
        pIn1 = pIn;
        for (; (l = __riscv_vsetvl_e32m8(blkCnt)) > 0; blkCnt -= l)
        {
          v_in = __riscv_vlse32_v_f32m8(pIn, col_diff, l);
          pIn += l * nCols;
          __riscv_vse32_v_f32m8(pOut, v_in, l);
          pOut += l;
        }
        pIn = pIn1 + 1;
      }
      /* Set status as RISCV_MATH_SUCCESS */
      status = RISCV_MATH_SUCCESS;
  #else
      /* Matrix transpose by exchanging the rows with columns */
      /* row loop */
      do
      {
        /* Pointer px is set to starting address of column being processed */
        px = pOut + i;
  
  #if defined (RISCV_MATH_LOOPUNROLL)
  
        /* Loop unrolling: Compute 4 outputs at a time */
        col = nCols >> 2U;
  
        while (col > 0U)        /* column loop */
        {
          /* Read and store input element in destination */
          *px = *pIn++;
          /* Update pointer px to point to next row of transposed matrix */
          px += nRows;
  
          *px = *pIn++;
          px += nRows;
  
          *px = *pIn++;
          px += nRows;
  
          *px = *pIn++;
          px += nRows;
  
          /* Decrement column loop counter */
          col--;
        }
  
        /* Loop unrolling: Compute remaining outputs */
        col = nCols & 0x3U;
  
  #else
  
        /* Initialize col with number of samples */
        col = nCols;
  
  #endif /* #if defined (RISCV_MATH_LOOPUNROLL) */
  
        while (col > 0U)
        {
          /* Read and store input element in destination */
          *px = *pIn++;
  
          /* Update pointer px to point to next row of transposed matrix */
          px += nRows;
  
          /* Decrement column loop counter */
          col--;
        }
  
        i++;
  
        /* Decrement row loop counter */
        row--;
  
      } while (row > 0U);          /* row loop end */
  
      /* Set status as RISCV_MATH_SUCCESS */
      status = RISCV_MATH_SUCCESS;
  #endif /*defined(RISCV_MATH_VECTOR)*/
    }
  
    /* Return to application */
    return (status);
  
  }




  riscv_status riscv_mat_mult_f32(
    const riscv_matrix_instance_f32 * pSrcA,
    const riscv_matrix_instance_f32 * pSrcB,
    riscv_matrix_instance_f32 * pDst)
  {
    float32_t *pIn1 = pSrcA->pData;                /* Input data matrix pointer A */
    float32_t *pIn2 = pSrcB->pData;                /* Input data matrix pointer B */
    float32_t *pInA = pSrcA->pData;                /* Input data matrix pointer A */
    float32_t *pInB = pSrcB->pData;                /* Input data matrix pointer B */
    float32_t *pOut = pDst->pData;                 /* Output data matrix pointer */
    float32_t *px;                                 /* Temporary output data matrix pointer */
    float32_t sum;                                 /* Accumulator */
    uint16_t numRowsA = pSrcA->numRows;            /* Number of rows of input matrix A */
    uint16_t numColsB = pSrcB->numCols;            /* Number of columns of input matrix B */
    uint16_t numColsA = pSrcA->numCols;            /* Number of columns of input matrix A */
    uint32_t col, i = 0U, row = numRowsA, colCnt;  /* Loop counters */
    riscv_status status;                             /* Status of matrix multiplication */
  
  #ifdef RISCV_MATH_MATRIX_CHECK
  
    /* Check for matrix mismatch condition */
    if ((pSrcA->numCols != pSrcB->numRows) ||
        (pSrcA->numRows != pDst->numRows)  ||
        (pSrcB->numCols != pDst->numCols)    )
    {
      /* Set status as RISCV_MATH_SIZE_MISMATCH */
      status = RISCV_MATH_SIZE_MISMATCH;
    }
    else
  
  #endif /* #ifdef RISCV_MATH_MATRIX_CHECK */
  
    {
  #if defined(RISCV_MATH_VECTOR)
      size_t ii, jj, kk;
      size_t l;
      vfloat32m4_t va0m4, vres0m4, vres1m4, vres2m4, vres3m4;
      vfloat32m8_t va0m8, vres0m8, vres1m8;
      colCnt = numRowsA;
  
      /* ch = 4, mul = 4 */
      for (jj = colCnt / 4; jj > 0; jj--) {
        px = pOut;
        pInB = pIn2;
        for (ii = numColsB; ii > 0; ii -= l) {
          l = __riscv_vsetvl_e32m4(ii);
          pInA = pIn1;
          vres0m4 = __riscv_vfmv_v_f_f32m4(0.0, l);
          vres1m4 = __riscv_vmv_v_v_f32m4(vres0m4, l);
          vres2m4 = __riscv_vmv_v_v_f32m4(vres0m4, l);
          vres3m4 = __riscv_vmv_v_v_f32m4(vres0m4, l);
          for (kk = 0; kk < numColsA; kk++) {
            va0m4 = __riscv_vle32_v_f32m4(pInB + kk * numColsB, l);
            vres0m4 = __riscv_vfmacc_vf_f32m4(vres0m4, *(pInA), va0m4, l);
            vres1m4 = __riscv_vfmacc_vf_f32m4(vres1m4, *(pInA + numColsA), va0m4, l);
            vres2m4 = __riscv_vfmacc_vf_f32m4(vres2m4, *(pInA + 2 * numColsA), va0m4, l);
            vres3m4 = __riscv_vfmacc_vf_f32m4(vres3m4, *(pInA + 3 * numColsA), va0m4, l);
            pInA++;
          }
          __riscv_vse32_v_f32m4(px, vres0m4, l);
          __riscv_vse32_v_f32m4(px + numColsB, vres1m4, l);
          __riscv_vse32_v_f32m4(px + 2 * numColsB, vres2m4, l);
          __riscv_vse32_v_f32m4(px + 3 * numColsB, vres3m4, l);
          px += l;
          pInB += l;
        }
        pIn1 +=  4 * numColsA;
        pOut += 4 * numColsB;
      }
  
      /* ch = 2, mul = 8 */
      colCnt = colCnt & 0x3;
      for (jj = colCnt / 2; jj > 0; jj--) {
        px = pOut;
        pInB = pIn2;
        for (ii = numColsB; ii > 0; ii -= l) {
          l = __riscv_vsetvl_e32m8(ii);
          pInA = pIn1;
          vres0m8 = __riscv_vfmv_v_f_f32m8(0.0, l);
          vres1m8 = __riscv_vmv_v_v_f32m8(vres0m8, l);
          for (kk = 0; kk < numColsA; kk++) {
            va0m8 = __riscv_vle32_v_f32m8(pInB + kk * numColsB, l);
            vres0m8 = __riscv_vfmacc_vf_f32m8(vres0m8, *(pInA), va0m8, l);
            vres1m8 = __riscv_vfmacc_vf_f32m8(vres1m8, *(pInA + numColsA), va0m8, l);
            pInA++;
          }
          __riscv_vse32_v_f32m8(px, vres0m8, l);
          __riscv_vse32_v_f32m8(px + numColsB, vres1m8, l);
          px += l;
          pInB += l;
        }
        pIn1 += 2 * numColsA;
        pOut += 2 * numColsB;
      }
      /* ch = 1, mul = 8 */
      colCnt = colCnt & 0x1;
      for (jj = colCnt; jj > 0; jj--) {
        px = pOut;
        pInB = pIn2;
        for (ii = numColsB; ii > 0; ii -= l) {
          l = __riscv_vsetvl_e32m8(ii);
          pInA = pIn1;
          vres0m8 = __riscv_vfmv_v_f_f32m8(0.0, l);
          for (kk = 0; kk < numColsA; kk++) {
            va0m8 = __riscv_vle32_v_f32m8(pInB + kk * numColsB, l);
            vres0m8 = __riscv_vfmacc_vf_f32m8(vres0m8, *(pInA++), va0m8, l);
          }
          __riscv_vse32_v_f32m8(px, vres0m8, l);
          px += l;
          pInB += l;
        }
        pIn1 += numColsA;
        pOut += numColsB;
      }
  
      /* Set status as RISCV_MATH_SUCCESS */
      status = RISCV_MATH_SUCCESS;
  #else
      /* The following loop performs the dot-product of each row in pSrcA with each column in pSrcB */
      /* row loop */
      do
      {
        /* Output pointer is set to starting address of row being processed */
        px = pOut + i;
  
        /* For every row wise process, column loop counter is to be initiated */
        col = numColsB;
  
        /* For every row wise process, pIn2 pointer is set to starting address of pSrcB data */
        pIn2 = pSrcB->pData;
  
        /* column loop */
        do
        {
          /* Set the variable sum, that acts as accumulator, to zero */
          sum = 0.0f;
  
          /* Initialize pointer pIn1 to point to starting address of column being processed */
          pIn1 = pInA;
  
  #if defined (RISCV_MATH_LOOPUNROLL)
  
          /* Loop unrolling: Compute 4 MACs at a time. */
          colCnt = numColsA >> 2U;
  
          /* matrix multiplication */
          while (colCnt > 0U)
          {
            /* c(m,p) = a(m,1) * b(1,p) + a(m,2) * b(2,p) + .... + a(m,n) * b(n,p) */
  
            /* Perform the multiply-accumulates */
            sum += *pIn1++ * *pIn2;
            pIn2 += numColsB;
  
            sum += *pIn1++ * *pIn2;
            pIn2 += numColsB;
  
            sum += *pIn1++ * *pIn2;
            pIn2 += numColsB;
  
            sum += *pIn1++ * *pIn2;
            pIn2 += numColsB;
  
            /* Decrement loop counter */
            colCnt--;
          }
  
          /* Loop unrolling: Compute remaining MACs */
          colCnt = numColsA & 0x3U;
  
  #else
  
          /* Initialize cntCnt with number of columns */
          colCnt = numColsA;
  
  #endif /* #if defined (RISCV_MATH_LOOPUNROLL) */
  
          while (colCnt > 0U)
          {
            /* c(m,p) = a(m,1) * b(1,p) + a(m,2) * b(2,p) + .... + a(m,n) * b(n,p) */
  
            /* Perform the multiply-accumulates */
            sum += *pIn1++ * *pIn2;
            pIn2 += numColsB;
  
            /* Decrement loop counter */
            colCnt--;
          }
  
          /* Store result in destination buffer */
          *px++ = sum;
  
          /* Decrement column loop counter */
          col--;
  
          /* Update pointer pIn2 to point to starting address of next column */
          pIn2 = pInB + (numColsB - col);
  
        } while (col > 0U);
  
        /* Update pointer pInA to point to starting address of next row */
        i = i + numColsB;
        pInA = pInA + numColsA;
  
        /* Decrement row loop counter */
        row--;
  
      } while (row > 0U);
  
      /* Set status as RISCV_MATH_SUCCESS */
      status = RISCV_MATH_SUCCESS;
  #endif /* defined(RISCV_MATH_VECTOR) */
    }
    /* Return to application */
    return (status);
  }

  void riscv_mat_init_f32(
    riscv_matrix_instance_f32 * S,
    uint16_t nRows,
    uint16_t nColumns,
    float32_t * pData)
    {
        /* Assign Number of Rows */
        S->numRows = nRows;
    
        /* Assign Number of Columns */
        S->numCols = nColumns;
    
        /* Assign Data pointer */
        S->pData = pData;
    }

    riscv_status riscv_mat_add_f32(
        const riscv_matrix_instance_f32 * pSrcA,
        const riscv_matrix_instance_f32 * pSrcB,
              riscv_matrix_instance_f32 * pDst)
      {
        float32_t *pInA = pSrcA->pData;                /* input data matrix pointer A */
        float32_t *pInB = pSrcB->pData;                /* input data matrix pointer B */
        float32_t *pOut = pDst->pData;                 /* output data matrix pointer */
      
        uint32_t numSamples;                           /* total number of elements in the matrix */
        uint32_t blkCnt;                               /* loop counters */
        riscv_status status;                             /* status of matrix addition */
      
      #ifdef RISCV_MATH_MATRIX_CHECK
      
        /* Check for matrix mismatch condition */
        if ((pSrcA->numRows != pSrcB->numRows) ||
            (pSrcA->numCols != pSrcB->numCols) ||
            (pSrcA->numRows != pDst->numRows)  ||
            (pSrcA->numCols != pDst->numCols)    )
        {
          /* Set status as RISCV_MATH_SIZE_MISMATCH */
          status = RISCV_MATH_SIZE_MISMATCH;
        }
        else
      
      #endif /* #ifdef RISCV_MATH_MATRIX_CHECK */
      
        {
          /* Total number of samples in input matrix */
          numSamples = (uint32_t) pSrcA->numRows * pSrcA->numCols;
      #if defined(RISCV_MATH_VECTOR)
          blkCnt = numSamples;
          size_t l;
          vfloat32m8_t vx, vy;
          for (; (l = __riscv_vsetvl_e32m8(blkCnt)) > 0; blkCnt -= l) {
            vx = __riscv_vle32_v_f32m8(pInA, l);
            pInA += l;
            vy = __riscv_vle32_v_f32m8(pInB, l);
            pInB += l;
            __riscv_vse32_v_f32m8(pOut, __riscv_vfadd_vv_f32m8(vy, vx, l), l);
            pOut += l;
          }
      #else
      #if defined (RISCV_MATH_LOOPUNROLL)
      
          /* Loop unrolling: Compute 4 outputs at a time */
          blkCnt = numSamples >> 2U;
      
          while (blkCnt > 0U)
          {
            /* C(m,n) = A(m,n) + B(m,n) */
      
            /* Add and store result in destination buffer. */
            *pOut++ = *pInA++ + *pInB++;
      
            *pOut++ = *pInA++ + *pInB++;
      
            *pOut++ = *pInA++ + *pInB++;
      
            *pOut++ = *pInA++ + *pInB++;
      
            /* Decrement loop counter */
            blkCnt--;
          }
      
          /* Loop unrolling: Compute remaining outputs */
          blkCnt = numSamples & 0x3U;
      
      #else
      
          /* Initialize blkCnt with number of samples */
          blkCnt = numSamples;
      
      #endif /* #if defined (RISCV_MATH_LOOPUNROLL) */
      
          while (blkCnt > 0U)
          {
            /* C(m,n) = A(m,n) + B(m,n) */
      
            /* Add and store result in destination buffer. */
            *pOut++ = *pInA++ + *pInB++;
      
            /* Decrement loop counter */
            blkCnt--;
          }
      #endif /* defined(RISCV_MATH_VECTOR) */
          /* Set status as RISCV_MATH_SUCCESS */
          status = RISCV_MATH_SUCCESS;
        }
      
        /* Return to application */
        return (status);
      }
      
      /**
        @} end of MatrixAdd group
       */
      



