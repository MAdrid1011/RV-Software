# SIMD8 裸机程序

本目录提供 INT32/FP32 的矩阵乘、FIR 和 FP32 FFT，供 `performance` 核心测量。
输入由固定种子的 xorshift32 生成，整数使用模 2³² 运算，FP32 保持各输出的运算顺序。
全部版本与独立编译的标量参考逐元素比较，浮点比较位模式。

## 构建

需要 Clang 23、LLD、LLVM binutils 和 Python 3：

```sh
make -C RV-Software/simd -j4
```

生成 `scalar`、`auto`、`rvv` 三个 ELF。默认编译目标为
`rv32imf_zicsr_zicntr_zve32f_zvl256b`，ABI 为 `ilp32f`，使用 `-O3 -ffp-contract=off`。
标量与参考关闭 loop/SLP 向量化，参考独立编译；函数限制到相同机器的标量 ISA 子集，
防止后端在标量版本中生成向量常量 Store。构建结束检查最终 ELF 的 ISA 与反汇编。

输出位于 `build/performance-vlen256-axi256-clang<版本>/`，不纳入 Git。
FFT 旋转因子统一舍入为 FP32，三种版本共用同一张表，不依赖运行时三角函数库。

`make -C RV-Software/simd smoke memory-smoke serial-smoke coremark` 生成向量算术、访存、多拍运算与
CoreMark 程序。访存程序覆盖非 32 B 对齐起始地址、跨行访问、分组、mask 和
标量读取已提交向量 Store 的结果。多拍程序覆盖除法、余数、平方根、转换、mask、
carry、merge、归约、slide 与整数宽度转换。所有入口默认使用 Spike 差分。

## 测量

```sh
make -C RV-Software/simd run SIMULATOR=../../build/performance/bin/zircon-sim
```

ZirconSim 使用 Spike 差分和动态进度。每项程序预热一次、计时五次，以 `cycle`
读取周期并取中位数；时间包含数组访存和循环控制。每次计时后校验输出，校验不计入
计算阶段。端到端周期包含输入准备、参考计算和校验。

`report.py` 汇总五项的标量、自动向量化和 RVV 专用周期与加速比，并检查 RVV 几何平均
加速比达到 3。所有比较使用同一核心配置。

## 指令与状态验证

```sh
make -C RV-Software/simd state-smoke isa-groups isa-memory
build/performance/bin/zircon-sim --elf \
  RV-Software/simd/build/performance-vlen256-axi256-clang23.1.2/state-smoke.elf
```

`state-smoke` 覆盖 SEW、LMUL、VL 边界、非零 `vstart`、mask/tail 策略、源目的重叠、
向量 CSR 别名和分支恢复。`isa-groups` 展开算术指令与有效寄存器组；`isa-memory`
覆盖连续、stride、indexed、segment、whole-register 与 mask 访存。生成程序保留测试名称，
运行时默认逐退休差分，并显示周期、IPC 和仿真速度。

算术和访存生成器同时输出同名 CSV 覆盖表，每行对应实际执行的指令编码、
SEW、LMUL、VL 和 mask；访存表另外列出 EEW 与 segment 字段数。
覆盖表与 C 程序由同一组用例产生，位于本目录的构建输出中。

| 验证入口 | 覆盖内容 | 用例数 |
| --- | --- | ---: |
| `isa-groups` | 整数、定点、mask、归约、排列及 FP32 编码与合法寄存器组 | 5709 |
| `isa-memory` | 连续、stride、indexed、segment、mask、whole-register、fault-only-first | 450 |
| `state-smoke` | 配置、VL 边界、策略、非零 vstart、重叠、CSR 别名与恢复 | 960 |

浮点特殊值、舍入、异常标志与连续八路 FMA 吞吐由 RTL 单元测试和 `serial-smoke`
共同验证；反压、取消与晚响应由计算、访存和 Cache 的 RTL 单元测试验证。

`scalar-wakeup-smoke` 在连续标量 Load 和分支之间交错执行 `vmv.x.s`、`vcpop.m`，
校验向量标量结果共用写回端口时的依赖与恢复。

## 吞吐与带宽程序

```sh
make -C RV-Software/simd sgemm stream register-throughput
```

`sgemm` 使用 Netlib BLAS SGEMM 接口，提供标量与 RVV 版本，覆盖矩阵转置组合并逐元素
校验结果。`stream` 比较数组加法与 SAXPY 在 Cache 内及大数组上的表现；
`register-throughput` 使用十六组独立目的寄存器，测量 INT32 加法及 FP32 加法、乘法和
FMA 的流水吞吐。各程序的 ELF 同样位于配置与工具链隔离的输出目录。
