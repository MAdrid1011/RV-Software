#!/usr/bin/env python3
"""Build the pinned RV32IMF picolibc used by the bare-metal FP32 workloads."""
import argparse
from pathlib import Path
import shutil
import subprocess

REVISION = "f7edb0869e64b9d687f46961adeef7154921fe92"
ROOT = Path(__file__).resolve().parent


def run(*arguments):
    subprocess.run(list(map(str, arguments)), check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--prefix", type=Path, default=ROOT / "build" / "picolibc-install")
    parser.add_argument("--source", type=Path, default=ROOT / "build" / "picolibc")
    options = parser.parse_args()
    if options.jobs < 1:
        parser.error("jobs must be positive")
    for tool in ("git", "meson", "ninja", "clang", "llvm-ar", "llvm-strip"):
        if not shutil.which(tool):
            parser.error(f"Missing executable: {tool}")
    source = options.source.resolve()
    output = ROOT / "build" / "picolibc-rv32"
    source.parent.mkdir(parents=True, exist_ok=True)
    if not source.exists():
        run("git", "clone", "https://github.com/picolibc/picolibc.git", source)
        run("git", "-C", source, "checkout", "--detach", REVISION)
    revision = subprocess.check_output(["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if revision != REVISION:
        raise SystemExit(f"Expected picolibc {REVISION}; found {revision} in {source}")
    arguments = ["meson", "setup", output, source, "--cross-file", ROOT / "clang-rv32.ini",
                 f"--prefix={options.prefix.resolve()}", "-Dmultilib=false", "-Dtests=false",
                 "-Dpicocrt=false", "-Dthread-local-storage=false"]
    if output.exists():
        arguments.append("--reconfigure")
    run(*arguments)
    run("ninja", "-C", output, f"-j{options.jobs}")
    run("ninja", "-C", output, "install")


if __name__ == "__main__":
    main()
