# RISC-V 架构测试

这里保存 Zircon-2026 使用的精简版官方 RISC-V Architecture Test。测试源码、
必要的环境头文件和许可证直接纳入 RV-Software，不依赖 ACT4、Python、mise、
Docker 或额外的 Git 子模块。

当前共有 149 项 RV32 测试，覆盖：

- `I`、`M`、`F`
- `Zaamo`、`Zalrsc`
- `Zicsr`、`Zifencei`
- `Zicntr`、`Zihpm`

每项测试由 Clang 直接构建。ZirconSim 使用进程内 Spike 逐条比较退休指令的
PC、机器码和整数/浮点目的寄存器，不生成或嵌入参考签名。测试结束时通过位于
`0xaffff000` 的非缓存 `tohost` 退出，不占用设备地址空间的起始位置。

需要支持 RV32 目标的 Clang、LLVM objdump、Spike 和已构建的 ZirconSim。macOS 默认查找
Homebrew LLVM；Linux 优先使用 Clang/LLVM 23，也可通过 `CLANG` 与 `OBJDUMP` 指定工具路径。
在 Zircon-2026 仓库根目录运行：

```sh
make -C RV-Software/arch-test build
make -C RV-Software/arch-test run-one TEST=Zaamo-amoadd.w-00
make -C RV-Software/arch-test run
make -C RV-Software/arch-test clean
```

`build` 默认使用主机全部逻辑处理器。生成文件位于 `RV-Software/arch-test/build/`，不会进入
版本控制。测试源码来自官方 RISC-V Architecture Test，许可证见 `LICENSES/`。
