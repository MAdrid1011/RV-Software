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
| [`coremark/`](coremark/ZIRCON.md) | CoreMark 裸机移植与周期归一化测量 |
| [`arch-test/`](arch-test/README.md) | RISC-V 架构测试子集与 Spike 差分 |
| `linux-system/` | Buildroot、Linux、OpenSBI、设备树与 initramfs 镜像 |

从 Zircon-2026 主仓库启动 Linux：

```sh
make -C RV-Software/linux-system linux
```
