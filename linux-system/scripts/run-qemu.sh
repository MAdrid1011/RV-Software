#!/bin/sh
set -eu

system_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
build_dir="$system_dir/build"
opensbi_src="$build_dir/opensbi-1.2"
buildroot_out="$build_dir/buildroot-out"
linux_out="$build_dir/linux-out"
cross="$buildroot_out/host/bin/riscv32-buildroot-linux-musl-"
qemu_out="$build_dir/opensbi-qemu-out"

make -C "$opensbi_src" O="$qemu_out" \
    CROSS_COMPILE="$cross" \
    PLATFORM=generic \
    PLATFORM_RISCV_XLEN=32 \
    PLATFORM_RISCV_ABI=ilp32 \
    PLATFORM_RISCV_ISA=rv32ima_zicsr_zifencei

log="$build_dir/qemu-linux-check.log"
qemu-system-riscv32 \
    -machine virt \
    -cpu rv32,c=false,d=false \
    -m 64M \
    -nographic \
    -bios "$qemu_out/platform/generic/firmware/fw_dynamic.bin" \
    -kernel "$linux_out/arch/riscv/boot/Image" \
    -append "console=hvc0 earlycon=sbi rdinit=/init panic=-1" \
    >"$log" 2>&1 &
qemu_pid=$!
elapsed=0
status=0
while kill -0 "$qemu_pid" 2>/dev/null; do
    if grep -q ZIRCON_LINUX_BOOT_PASS "$log"; then
        break
    fi
    if [ "$elapsed" -ge 120 ]; then
        status=124
        break
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done
if grep -q ZIRCON_LINUX_BOOT_PASS "$log"; then
    kill "$qemu_pid" 2>/dev/null || true
    wait "$qemu_pid" 2>/dev/null || true
elif kill -0 "$qemu_pid" 2>/dev/null; then
    kill "$qemu_pid" 2>/dev/null || true
    wait "$qemu_pid" 2>/dev/null || true
else
    wait "$qemu_pid" || status=$?
fi
if ! grep -q ZIRCON_LINUX_BOOT_PASS "$log"; then
    cat "$log"
    exit "$status"
fi
grep -E 'OpenSBI|Linux version|Zircon Linux validation|ZIRCON_LINUX_BOOT_PASS' "$log"
