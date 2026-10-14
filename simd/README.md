# SIMD8 裸机程序

本目录为 Zircon-2026 的 `performance` 核心提供整数与 FP32 计算程序、指令测试和公共
裸机运行时。计算程序包含标量、自动向量化和 RVV 专用版本，使用相同输入与运算顺序，
并与独立编译的标量参考逐元素比较。整数使用模 2³² 运算，浮点比较位模式。

## 目录

| 目录 | 内容 |
| --- | --- |
| [runtime/](runtime/README.md) | 启动代码、链接脚本、计时、输出与 CoreMark 适配 |
| [benchmarks/](benchmarks/README.md) | 矩阵乘、FIR、FFT、SGEMM、吞吐与带宽程序 |
| [tests/](tests/README.md) | 算术、访存、多拍运算与状态验证 |
| [tools/](tools/README.md) | 标量编译、数据生成、ISA 检查与统计工具 |

根目录 `Makefile` 提供全部构建入口，生成物位于
`build/performance-vlen256-axi256-clang<版本>/`，不纳入 Git。

## 构建与运行

需要 Clang 23、LLD、LLVM binutils 和 Python 3。在主仓库根目录运行：

```sh
make -C RV-Software/simd -j4
make -C RV-Software/simd run SIMULATOR=../../build/performance/bin/zircon-sim
```

默认目标为 `rv32imf_zicsr_zicntr_zve32f_zvl256b`，ABI 为 `ilp32f`，
使用 `-O3 -ffp-contract=off`。标量和参考程序关闭 loop/SLP 向量化，
函数使用标量 ISA 子集；构建结束检查最终 ELF 的 ISA 与反汇编。
ZirconSim 默认开启 Spike 差分与动态进度显示。

五项计算程序分别为 INT32/FP32 的 64×64 矩阵乘、4096 输出且 32 taps 的 FIR，
以及 1024 点 FP32 FFT。各程序预热一次、计时五次，以 `cycle` 读取周期并取中位数。
计算阶段包含数组访存和循环控制；输入准备、参考计算和校验计入端到端周期。

## 其他入口

| Make 目标 | 程序 |
| --- | --- |
| `coremark` | 性能核心的标量 CoreMark |
| `sgemm` | Netlib BLAS SGEMM 接口与转置组合校验 |
| `stream` | Cache 内及大数组上的数组加法与 SAXPY |
| `register-throughput` | 独立寄存器 INT32 / FP32 运算吞吐 |
| `fma-gemm` | 32×256×32 FP32 融合矩阵乘 |
| `smoke memory-smoke serial-smoke scalar-wakeup-smoke` | 手写指令与子系统测试 |
| `state-smoke isa-smoke isa-groups isa-memory` | 生成的状态、算术与访存测试 |

例如：

```sh
make -C RV-Software/simd coremark sgemm stream register-throughput fma-gemm
make -C RV-Software/simd smoke memory-smoke serial-smoke state-smoke isa-groups isa-memory
```

正式单精度工作负载的移植位于 [CoreMark-PRO](../coremark-pro/README.md)。
核心配置与执行结构见主仓库的
[SIMD8 性能核心](https://github.com/MAdrid1011/Zircon-2026/blob/main/docs/PerformanceCore.md)。
