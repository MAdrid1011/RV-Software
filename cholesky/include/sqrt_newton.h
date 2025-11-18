/**
 * @brief Q31定点平方根函数（牛顿迭代法）
 * @param x Q31格式输入值
 * @return Q31格式平方根结果
 */
#include "num_type.h"

// 软件实现32位前导零计数
static inline int clz32(uint32_t x) {
    if (x == 0) return 32;
    int n = 0;
    if (x <= 0x0000FFFF) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFF) { n += 8;  x <<= 8;  }
    if (x <= 0x0FFFFFFF) { n += 4;  x <<= 4;  }
    if (x <= 0x3FFFFFFF) { n += 2;  x <<= 2;  }
    if (x <= 0x7FFFFFFF) { n += 1; }
    return n;
}

q31_t sqrt_q31_newton(q31_t x) {
    if (x <= 0) return 0;
    if (x == Q31_MAX) return 0x5A82799A;  // sqrt(1) in Q31
    
    // 将Q31转换为更大的精度进行计算
    uint64_t val = (uint32_t)x;
    uint64_t guess;
    
    // 初始猜测 - 使用位移优化
    if (val >= 0x40000000U) {  // >= 0.5
        guess = val >> 1;
    } else {
        // 对小数使用更好的初始猜测
        int leading_zeros = clz32(val);
        int shift = (32 - leading_zeros) >> 1;
        guess = val >> shift;
        if (guess == 0) guess = 1;
    }
    
    // 牛顿迭代: x_{n+1} = (x_n + a/x_n) / 2
    for (int i = 0; i < 10; i++) {
        uint64_t div_result = (val << 31) / guess;  // Q31除法
        uint64_t new_guess = (guess + div_result) >> 1;
        
        // 收敛检查
        if (new_guess == guess || 
            (new_guess > guess ? new_guess - guess : guess - new_guess) <= 1) {
            break;
        }
        guess = new_guess;
    }
    
    return (q31_t)guess;
}
