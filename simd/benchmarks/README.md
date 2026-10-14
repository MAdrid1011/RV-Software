# SIMD8 计算程序

本目录提供整数与 FP32 的数组计算，以及寄存器吞吐和访存带宽程序。
构建与运行通过 [上级 Makefile](../README.md) 完成。

| 文件 | 内容 |
| --- | --- |
| `bench.c` | 五项程序的输入准备、计时与逐元素校验 |
| `kernels.h`、`kernels.c` | 公共接口、标量参考与可自动向量化的实现 |
| `kernels_rvv.c` | 矩阵乘、FIR 和 FFT 的 RVV intrinsics 实现 |
| `sgemm.c` | Netlib BLAS SGEMM 接口，支持输入矩阵转置 |
| `stream_bench.c`、`stream_kernels.c` | 数组加法与 SAXPY 的标量/RVV 比较 |
| `register_throughput.c` | 独立寄存器 INT32 加法及 FP32 加法、乘法和 FMA |
| `fma_gemm.c` | 32×256×32 融合矩阵乘与八路 FMA 利用率 |

默认使用 `-O3`，关闭隐式浮点收缩，不启用 fast-math。FMA 程序显式使用融合运算；
每个输出保持递增 K 的累加顺序。FFT 的 FP32 旋转因子在构建时生成，三个版本共用同一张表。

## FP32 矩阵乘 FMA 利用率

`fma-gemm` 计算行优先矩阵 `A[32][256] × B[256][32]`。
标量参考使用 `fmadd.s`，向量内核使用 `vfmacc.vv/vf`，结果逐元素比较位模式。
内核覆盖 LMUL=1/4、寄存器分块、Cache 分块与输入组交替供数。
七行微内核使用七个独立累加器复用同一 B 组，尾部处理剩余四行。

在主仓库根目录运行：

```sh
make -C RV-Software/simd fma-gemm
build/performance/bin/zircon-sim --elf \
  RV-Software/simd/build/performance-vlen256-axi256-clang23.1.2/fma-k32-all/fma-gemm.elf \
  --max-cycles 100000000 --instruction-profile build/fma-gemm-retirement.csv \
  > build/fma-gemm.log
python3 RV-Software/simd/tools/fma_gemm_report.py \
  --elf RV-Software/simd/build/performance-vlen256-axi256-clang23.1.2/fma-k32-all/fma-gemm.elf \
  --profile build/fma-gemm-retirement.csv --log build/fma-gemm.log
```

各内核预热一次、计时五次。八路有效利用率定义为
`有效 FP32 FMA 元素数 / (8 × 周期数)`：VL=8 时，一条向量 FMA 处理八个元素；
LMUL=4、VL=32 时，一条指令分四个八路任务处理 32 个元素。
报告结合 ELF 符号与实际退休统计核对六次执行的 FMA 指令数。

`body_cycles` 汇总 K 循环周期，包含输入 Load、地址计算和循环控制；
`total_cycles` 还包含累加器准备、中间结果与最终输出 Store、外层循环和计时。
两种口径分别报告，以区分计算主体与完整矩阵乘的开销。

`FMA_K_TILE` 为七行内核的 K 分块大小，默认 32，取值为 256 的偶数因子；
`FMA_KERNEL_SET=seven-row` 仅构建七行内核。例如：

```sh
make -C RV-Software/simd fma-gemm FMA_K_TILE=16 FMA_KERNEL_SET=seven-row
```

输出目录包含分块大小和内核集合，例如 `fma-k16-seven-row/`。
