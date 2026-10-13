"""从固定输入与周期中位数计算三种构建的加速比。"""
import math
from pathlib import Path
import re
import sys

root = Path(sys.argv[1])
results = {}
for variant in ("scalar", "auto", "rvv"):
    text = (root / variant / "run.txt").read_text()
    if "FAIL" in text:
        raise RuntimeError(f"{variant}: 校验失败")
    results[variant] = {name: int(cycles) for name, cycles in re.findall(r"SIMD_RESULT (\S+) (\d+)", text)}
names = ("gemm-i32", "fir-i32", "gemm-f32", "fir-f32", "fft-f32")
if any(set(result) != set(names) for result in results.values()):
    raise RuntimeError("五项程序的结果不完整")
speedups = []
for name in names:
    scalar, auto, rvv = (results[v][name] for v in ("scalar", "auto", "rvv"))
    speedup = scalar / rvv
    speedups.append(speedup)
    print(f"{name}: scalar={scalar} auto={auto} rvv={rvv} auto加速={scalar/auto:.3f} RVV加速={speedup:.3f}")
geomean = math.exp(sum(map(math.log, speedups)) / len(speedups))
print(f"RVV加速比几何平均={geomean:.3f}")
if geomean < 3:
    sys.exit(1)
