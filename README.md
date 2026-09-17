# RV-Software

The `zircon-2026` branch builds bare-metal software for
`RV32IMAF_Zicsr_Zifencei_Zaamo_Zalrsc` with the `ilp32f` ABI. The architecture and ABI can
be overridden for the Zircon-2024 comparison point:

```sh
make RISCV_ARCH=rv32im RISCV_ABI=ilp32
```

Programs terminate through the ELF `tohost` symbol.  A value of `1` is pass;
another odd value is a failure code.  Illegal instruction sentinels are not a
test protocol because they collide with architectural exception testing.
RISC-V Software and Compiler Environment for CPU Test

官方架构测试的精简移植位于 `arch-test/`，可直接使用 Clang 构建并由 ZirconSim
与进程内 Spike 逐提交差分，无需额外测试框架或子模块。
