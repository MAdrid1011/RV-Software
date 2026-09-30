# TACLeBench

本目录提供 TACLeBench 在 Zircon-2026 上的裸机移植。TACLeBench 面向最坏执行时间与嵌入式处理器研究，负载覆盖控制、信号处理、编解码、密码、排序、搜索和数值计算。

构建使用 Clang/LLVM 23。RV32 核心只实现单精度浮点扩展，使用 `double` 的基准由 RISC-V GCC 工具链提供标准软双精度算术运行时。

`upstream/` 完整保留 [tacle/tacle-bench](https://github.com/tacle/tacle-bench) 提交 `c6a0d73e47bbd2bc86e34637156fb26dd4d5cf08` 的 README、基准源码和各项目许可证。不同基准采用不同授权条款，使用或再分发前应阅读对应目录中的许可证与说明，尤其是 `parallel/DEBIE/terms_of_use-2014-05.pdf`。

## 基准范围

统一构建入口覆盖 54 个具有独立 `main()` 的单核正式基准：

| 类别 | 数量 | 内容 |
| --- | ---: | --- |
| `app` | 2 | 工业升降机与汽车电动车窗控制 |
| `kernel` | 29 | 算法、数学、排序、哈希和信号处理内核 |
| `sequential` | 23 | 编解码、密码、音频、图像和控制程序 |

上游的 `parallel` 与 `test` 目录也完整保留。`parallel` 中的 DEBIE、PapaBench 和 ROSACE 具有任务调度、多处理器或目标外设语义，需要为 Zircon 的执行模型单独适配，因此不计入单核批量构建；`test` 是编译器行为测试，不属于正式性能集合。

## 构建与运行

列出可直接构建的基准：

```sh
make -C RV-Software/taclebench list
```

构建一个基准或全部 54 个单核基准：

```sh
make -C RV-Software/taclebench BENCH=kernel/sha image
make -C RV-Software/taclebench suite
make -C RV-Software/taclebench check
```

`check` 默认启用 Spike 提交级差分，并显示各项仿真的动态周期、IPC 和运行速度。可通过 `CHECK_BENCHMARKS` 只运行指定集合。

运行一个基准：

```sh
make -C RV-Software/taclebench BENCH=app/powerwindow sim
```

程序返回值由 Zircon 裸机运行时通过 `tohost` 报告。返回 `0` 表示该基准的上游校验通过。

## 可合并写控制负载

`ports/lift-write-combine.c` 保持 `lift` 的控制算法、1001 次控制步和标准 checksum，将电机、指示灯与控制状态组成 8-word 设备描述符，对比顺序外设写入与可合并写入：

```sh
make -C RV-Software/taclebench sim-write-combine
```

串口输出位于计时区间之外，结果分别给出纯控制计算、设备提交开销和完整控制循环周期。
