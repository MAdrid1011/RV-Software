# 向量指令与状态测试

本目录保存手写的裸机定向测试。配置、算术编码与复杂访存用例由
[tools/](../tools/README.md) 中的生成器构建，生成的 C 和 CSV 覆盖表保存在 `build/`。

| Make 目标 | 验证内容 |
| --- | --- |
| `smoke` | 常用整数与 FP32 向量算术 |
| `memory-smoke` | 非 32 B 对齐、跨行、分组、mask 与标量读取向量 Store |
| `serial-smoke` | 除法、平方根、转换、mask、归约、排列及宽度转换 |
| `scalar-wakeup-smoke` | 标量 Load、分支与向量标量结果的依赖和恢复 |
| `state-smoke` | SEW、LMUL、VL、vstart、mask/tail、重叠与 CSR 状态 |
| `isa-smoke`、`isa-groups` | Zve32f 算术编码与合法寄存器分组 |
| `isa-memory` | 连续、stride、indexed、segment、whole-register 与 mask 访存 |

在主仓库根目录构建并运行：

```sh
make -C RV-Software/simd smoke memory-smoke serial-smoke scalar-wakeup-smoke \
  state-smoke isa-smoke isa-groups isa-memory
build/performance/bin/zircon-sim --elf \
  RV-Software/simd/build/performance-vlen256-axi256-clang23.1.2/state-smoke.elf
```

ZirconSim 默认开启 Spike 差分与动态进度显示。生成器使用 Spike 的 `encoding.h`，
可通过 `SPIKE_ENCODING` 指定路径。CSV 记录实际用例编码、SEW、LMUL、VL 和 mask；
访存用例还包含 EEW 与 segment 数量。

裸机测试与主仓库 `src/test/scala/Backend/Vector/` 的 RTL 单元测试配合使用。
RTL 测试覆盖反压、恢复、晚响应、浮点特殊值、舍入、异常标志与连续八路 FMA 吞吐。
