# RV-Software

The `zircon-2026` branch builds bare-metal software for
`RV32IMAF_Zicsr_Zifencei_Zaamo_Zalrsc` with the `ilp32f` ABI. The architecture and ABI can
be overridden for the Zircon-2024 comparison point:

```sh
make RISCV_ARCH=rv32im RISCV_ABI=ilp32
```

Programs terminate through the ELF `tohost` symbol. A value of `1` is pass;
another odd value is a failure code. Illegal instruction sentinels are not a
test protocol because they collide with architectural exception testing.

`functest/` contains focused bare-metal programs, `coremark/` contains the
cycle-normalized CoreMark port, and `arch-test/` contains the vendored subset
of the official RISC-V Architecture Test suite. The architecture tests build
with Clang and run through ZirconSim with in-process Spike differential testing.

`linux-system/` defines the fixed Buildroot, Linux, OpenSBI, initramfs, and
device-tree image used by Zircon-2026. The supported build and interactive boot
entry point is run from the parent Zircon-2026 repository:

```sh
make -C RV-Software/linux-system linux
```
