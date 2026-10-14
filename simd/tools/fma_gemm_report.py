"""按实际退休指令核对 FP32 矩阵乘的八路 FMA 利用率。"""
import argparse
import csv
import json
import re
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", required=True)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--nm", default="llvm-nm")
    args = parser.parse_args()
    symbols = {}
    for line in subprocess.check_output(
        [args.nm, "-S", "--defined-only", args.elf], text=True
    ).splitlines():
        fields = line.split()
        if len(fields) == 4:
            symbols[fields[3]] = (int(fields[0], 16), int(fields[1], 16))
    with open(args.profile, newline="") as source:
        profile = [(int(row["pc"], 0), int(row["instruction"], 0), int(row["retired"]))
                   for row in csv.DictReader(source)]
    log = Path(args.log).read_text()
    configuration = re.search(r"FMA_GEMM kernels=(all|seven-row) k_tile=(\d+)", log)
    if configuration is None:
        raise RuntimeError("缺少内核与分块配置")
    kernel_set, tile = configuration.groups()
    outcome = json.loads(log.splitlines()[-1])
    if outcome["status"] != "exit" or outcome["code"] != 0 or "PASS FMA_GEMM" not in log:
        raise RuntimeError("矩阵乘运行未通过")
    # The final summary snapshots instructions at tohost retirement; profiling
    # includes the few instructions retired while that Store reaches the bus.
    retired_total = sum(count for _, _, count in profile)
    if retired_total < outcome["instructions"]:
        raise RuntimeError("退休分布缺少已经计入仿真结果的指令")
    start, size = symbols["scalar_reference"]
    scalar_fma = sum(count for pc, inst, count in profile
                     if start <= pc < start + size and inst & 0x7f == 0x43
                     and (inst >> 25 & 3) == 0)
    if scalar_fma != 32 * 256 * 32:
        raise RuntimeError("标量参考 FMA 退休数量不一致")
    kernels = {
        "simple-m1": ("simple_m1", 8),
        "blocked-4x32-m4": ("blocked_m4", 32),
        "blocked-4x32-m1": ("blocked_m1", 8),
        "scheduled-4x32-m4": ("scheduled_m4", 32),
        "cache-tiled-k32-m4": ("cache_tiled_m4", 32),
        "pipelined-tiled-k32-m4": ("pipelined_tiled_m4", 32),
        f"seven-row-tiled-k{tile}-m4": ("seven_row_tiled_m4", 32),
        "six-row-pipelined-k32-m4": ("six_row_pipelined_m4", 32),
    }
    if kernel_set == "seven-row":
        kernels = {f"seven-row-tiled-k{tile}-m4": ("seven_row_tiled_m4", 32)}
    results = []
    for name, (symbol, vl) in kernels.items():
        match = re.search(r"FMA_GEMM " + re.escape(name) + r" body_cycles=(\d+) total_cycles=(\d+)", log)
        if match is None:
            raise RuntimeError(f"缺少 {name} 的周期结果")
        start, size = symbols[symbol]
        # vfmacc.vv/vf: funct6=0x2c, funct3=1/5, opcode=0x57.
        retired = sum(count for pc, inst, count in profile
                      if start <= pc < start + size and inst & 0x7f == 0x57
                      and inst >> 26 == 0x2c and (inst >> 12 & 7) in (1, 5))
        expected = 6 * 32 * 256 * 32 // vl
        if retired != expected:
            raise RuntimeError(f"{name}: 实际退休 FMA {retired} != {expected}")
        body, total = map(int, match.groups())
        results.append({
            "kernel": name, "vl": vl, "runs": 6,
            "retired_vector_fma_all_runs": retired,
            "retired_vector_fma_per_run": retired // 6,
            "useful_fma_elements_per_run": retired * vl // 6,
            "median_body_cycles": body, "total_cycles_at_body_median": total,
            "body_utilization_percent": 100 * retired * vl / (6 * 8 * body),
            "whole_kernel_utilization_percent": 100 * retired * vl / (6 * 8 * total),
        })
    print(json.dumps({"shape": {"m": 32, "k": 256, "n": 32},
                     "kernel_set": kernel_set, "seven_row_k_tile": int(tile),
                     "profile_retired_total": retired_total,
                     "scalar_reference_retired_fma": scalar_fma, "results": results},
                     ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
