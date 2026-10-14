# SIMD8 构建与测量工具

这些 Python 3 工具由 [统一 Makefile](../README.md) 调用，负责源程序生成、ISA 检查和结果统计。

| 工具 | 输入与输出 |
| --- | --- |
| `compile_scalar.py` | 包装 Clang 命令，将 C 函数限制到标量 ISA 子集 |
| `gen_twiddle.py` | 向标准输出生成 1024 点 FFT 的 FP32 旋转因子 C 表 |
| `generate_state_smoke.py` | `--output` 指定状态测试 C 文件与同名 CSV |
| `generate_isa_smoke.py` | 从 `--encoding` 生成算术测试；`--lmul all` 展开合法分组 |
| `generate_memory_smoke.py` | 从 `--encoding` 生成访存测试 C 文件与同名 CSV |
| `audit_elf.py` | 检查一个或多个 ELF 的 ISA 属性与反汇编 |
| `report.py` | 从构建目录三个版本的 `run.txt` 汇总五项程序的周期与加速比 |
| `fma_gemm_report.py` | 结合 `--elf`、`--profile` 和 `--log` 核对实际退休 FMA 数并输出 JSON |

ISA 检查与退休报告支持显式指定 LLVM 工具路径。
FMA 报告的完整使用方法见 [矩阵乘利用率](../benchmarks/README.md#fp32-矩阵乘-fma-利用率)。
生成数据、CSV、运行日志与报告均保存在构建目录。
