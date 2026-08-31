# RV-Software

The `zircon-2026` branch builds bare-metal software for
`RV32IMAF_Zicsr_Zifencei` with the `ilp32f` ABI.  The architecture and ABI can
be overridden for the Zircon-2024 comparison point:

```sh
make RISCV_ARCH=rv32im RISCV_ABI=ilp32
```

Programs terminate through the ELF `tohost` symbol.  A value of `1` is pass;
another odd value is a failure code.  Illegal instruction sentinels are not a
test protocol because they collide with architectural exception testing.
RISC-V Software and Compiler Environment for CPU Test
