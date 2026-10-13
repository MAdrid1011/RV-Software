# CoreMark-PRO 单精度裸机负载

本目录将 EEMBC CoreMark-PRO 的两个 FP32 工作负载运行在 Zircon-2026 性能核心上：
`linear_alg-mid-100x100-sp` 使用 100×100 矩阵的线性代数计算，
`loops-all-mid-10k-sp` 包含 24 个 Livermore kernels。

上游代码固定为 `4832cc67b0926c7a80a4b7ce0ce00f4640ea6bec`，保留算法、数据集、
校验及许可证。裸机适配层提供堆、UART、核心周期计时和单上下文运行入口。
两个工作负载分别报告周期与校验结果。

## 构建与运行

需要 Clang 23、LLD、LLVM binutils、Meson、Ninja 和 RISC-V GCC 的
`rv32imf/ilp32f` 内建运算库。可通过 `GCC_BUILTINS` 指定该库路径。

```sh
git submodule update --init --recursive
make -C RV-Software/coremark-pro dependencies
make -C RV-Software/coremark-pro -j4
make -C RV-Software/coremark-pro run
```

依赖入口从固定提交构建 picolibc 1.8.11。程序及库使用 RV32IMF、`ilp32f`，
关闭自动向量化、fast-math 和隐式浮点收缩。最终 ELF 与 SIMD 程序使用相同的 ISA
检查入口，保证链接对象不引入未配置的指令。

默认使用 `build/performance/bin/zircon-sim`，开启 Spike 差分和动态进度。
`SIMULATOR`、`CLANG`、`PICOLIBC` 可由 Make 命令参数指定。框架运行一轮测量、
一个上下文及一次校验；周期计时按 1 MHz 归一化，输出同时包含端到端周期。
构建和运行结果位于本目录 `build/`，不纳入版本控制。
