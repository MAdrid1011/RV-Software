# Zircon CoreMark Port

This directory keeps the CoreMark v1.01 benchmark sources and the Zircon bare-metal
port. The default build uses one iteration for fast functional and Difftest runs:

```sh
make -C RV-Software/coremark
```

Use a separate iteration count for a cycle-normalized measurement. Each count has an
independent object directory and ELF name, so switching between functional and
measurement builds does not reuse incompatible objects:

```sh
make -C RV-Software/coremark ITERATIONS=10
build/cmake/bin/zircon-sim \
    --elf RV-Software/coremark/build/coremark-i10-rv32imaf_zicsr_zifencei-ilp32f.elf \
    --seed 1 --max-cycles 10000000 --stall-cycles 10000 --no-progress
```

The timed region reads `mcycle` immediately before and after `iterate()`. The port
reports the frequency-independent score as:

```text
CoreMark/MHz = iterations * 1,000,000 / timed_cycles
```

This cycle-normalized result does not assume a Vivado or ASIC clock frequency. An EEMBC
submission also requires a run of at least ten seconds on the implemented target; that
duration can only be checked after choosing the target implementation and its clock.
