# Zircon CoreMark 移植

本目录包含 CoreMark v1.01 源码和 Zircon 裸机平台移植。默认构建运行一轮，适合功能验证：

```sh
make -C RV-Software/coremark
```

周期归一化测量可指定轮数；不同轮数使用独立目标文件目录和 ELF：

```sh
make -C RV-Software/coremark ITERATIONS=10
build/cmake/bin/zircon-sim \
    --elf RV-Software/coremark/build/coremark-i10-rv32imaf_zicsr_zifencei-ilp32f.elf \
    --seed 1 --max-cycles 10000000 --stall-cycles 10000 --no-progress
```

移植层在 `iterate()` 前后读取 `mcycle`，按下式计算与时钟频率无关的分数：

```text
CoreMark/MHz = 轮数 × 1,000,000 / 计时周期
```

此结果不预设 FPGA 或 ASIC 的工作频率。EEMBC 正式提交还需要目标实现上的运行时间
达到至少十秒。
