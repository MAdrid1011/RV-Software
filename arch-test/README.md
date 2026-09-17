# RISC-V 架构测试

这里保存 Zircon-2026 的官方 RISC-V Architecture Test v4（ACT4）配置。
当前范围是 `RV32IM_Zicsr_Zifencei` 的机器模式基础能力，首批生成并
运行六个 `Zicsr` 自校验程序。

上游测试固定在 `third_party/riscv-arch-test` 子模块。ACT4 使用 Clang
编译，并使用 Spike 生成期望签名。最终 ELF 由 ZirconSim 使用进程内
Spike 逐提交对拍。`tohost` 固定放在 `0xaffff000`，通过非缓存设备路径
结束测试。

需要 Clang、Spike、`mise` 和 ACT4 所需的 Python 工具。首次运行前，
按上游要求在子模块目录安装固定版本的工具环境：

```sh
cd third_party/riscv-arch-test
mise trust .mise.toml
mise install
cd ../..
```

构建、运行一个冒烟测试或运行全部六项测试：

```sh
make -C arch-test build
make -C arch-test run-smoke
make -C arch-test run
make -C arch-test clean
```

生成目录位于 `arch-test/build/`，运行报告位于 `arch-test/reports/`；两者
均不进入版本控制。
