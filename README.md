# RV-Software

本仓库提供 Zircon-2026 使用的裸机程序、CoreMark、RISC-V 架构测试和 Linux 软件镜像。
默认 `zircon-2026` 分支面向 `RV32IMAF_Zicsr_Zifencei_Zaamo_Zalrsc` 与 `ilp32f` ABI。
比较早期 Zircon-2024 配置时，可指定：

```sh
make RISCV_ARCH=rv32im RISCV_ABI=ilp32
```

裸机程序通过 ELF 中的 `tohost` 符号报告结果：`1` 表示通过，其他奇数值表示错误码。
异常测试不会把非法指令当作程序结束标记。

| 目录 | 内容 |
| --- | --- |
| `functest/` | 指令和子系统的定向裸机测试 |
| `device-test/` | 外设接口与设备功能测试 |
| [`taclebench/`](taclebench/README.md) | TACLeBench 嵌入式与控制类工作负载 |
| [`coremark/`](coremark/ZIRCON.md) | CoreMark 裸机移植与周期归一化测量 |
| [`arch-test/`](arch-test/README.md) | RISC-V 架构测试子集与 Spike 差分 |
| `linux-system/` | Buildroot、Linux、OpenSBI、设备树与 initramfs 镜像 |

从 Zircon-2026 主仓库启动 Linux：

```sh
make -C RV-Software/linux-system linux
```

## TACLeBench

TACLeBench 移植提供 54 个单核嵌入式基准的统一构建入口，并保留完整上游基准树。`lift` 扩展将工业升降机控制状态组成设备描述符，用于对比顺序外设写入和可合并写入。

```sh
make -C RV-Software/taclebench suite
make -C RV-Software/taclebench sim-write-combine
```

完整基准列表、运行方法、来源和许可证说明见 [`taclebench/README.md`](taclebench/README.md)。
