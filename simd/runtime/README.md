# SIMD8 裸机运行时

公共运行时供 SIMD 程序、性能核心 CoreMark 和 CoreMark-PRO 使用。

| 文件 | 职责 |
| --- | --- |
| `start.S` | 初始化全局指针、栈、BSS 与 `fcsr`，调用 `main` |
| `linker.ld` | 配置代码、数据、256 KiB 栈、堆及 `tohost/fromhost` |
| `runtime.h`、`runtime.c` | 周期计时、UART 输出、退出与基础内存函数 |
| `coremark_glue.c`、`coremark_flags.h` | CoreMark 的平台接口与编译信息 |

程序从 `0x80000000` 的 RAM 执行，通过 `0xa00003f8` 的 UART 输出；
`0xaffff000` 的 `tohost` 写入 1 表示成功，其他奇数值编码错误码。
`simd_cycle` 与 `simd_cycle64` 读取核心周期计数器，时基为核心时钟。

运行时使用非特权计数器与浮点 CSR，不依赖异常入口或操作系统。
从 [SIMD8 统一入口](../README.md) 构建时，运行时与程序使用一致的 ISA 和 ABI。
