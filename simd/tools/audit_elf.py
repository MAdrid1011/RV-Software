"""检查裸机计算程序的 ISA、ABI 与指令边界。"""
import argparse
import re
import subprocess

def output(*args):
    return subprocess.check_output(args, text=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--readelf", default="llvm-readelf")
    parser.add_argument("--objdump", default="llvm-objdump")
    parser.add_argument("elf", nargs="+")
    args = parser.parse_args()
    for elf in args.elf:
        attributes = output(args.readelf, "-A", elf)
        arch = re.search(r"Value:\s*(rv32\S+)", attributes)
        if arch is None:
            raise RuntimeError(f"{elf}: 缺少 RISC-V ISA 属性")
        extensions = arch[1].split("_")
        if any(re.match(r"(?:a|c|d|za\w*)\d", ext) for ext in extensions):
            raise RuntimeError(f"{elf}: 不允许的 ISA {arch[1]}")
        if not all(any(ext.startswith(required) for ext in extensions) for required in ("zicsr", "zicntr", "zve32f", "zvl256b")):
            raise RuntimeError(f"{elf}: ISA 属性不完整 {arch[1]}")
        instructions = output(args.objdump, "-d", "--no-show-raw-insn", elf)
        if re.search(r"\s(?:ecall|ebreak|mret|sret|wfi|sfence\.vma|amo\w*|lr\.w|sc\.w)\b", instructions):
            raise RuntimeError(f"{elf}: 含特权或原子指令")
        if re.search(r"\s(?:csrr\w*|csrw|csrs|csrc)\s.*\b(?:m\w+|sstatus|satp|stvec|sepc|scause|stval|sie|sip)\b", instructions):
            raise RuntimeError(f"{elf}: 含特权 CSR 访问")
        if "/scalar/" in elf and re.search(r"\s(?:vset\w*|v[a-z0-9]+\.[a-z0-9]+)\b", instructions):
            raise RuntimeError(f"{elf}: 标量版本含向量指令")
        print(f"ISA_CHECK {elf}: {arch[1]}")

if __name__ == "__main__":
    main()
