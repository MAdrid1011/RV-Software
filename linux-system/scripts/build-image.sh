#!/bin/sh
set -eu

system_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
software_dir=$(CDPATH= cd -- "$system_dir/.." && pwd)
build_dir="$system_dir/build"
external_dir="$system_dir/external"
linux_src="$software_dir/linux"
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN)}

"$system_dir/scripts/fetch-sources.sh" "$build_dir"

buildroot_src="$build_dir/buildroot-2024.02.12"
opensbi_src="$build_dir/opensbi-1.2"
if [ ! -f "$buildroot_src/Makefile" ]; then
    mkdir -p "$buildroot_src"
    tar -xf "$build_dir/sources/buildroot-2024.02.12.tar.xz" --strip-components=1 -C "$buildroot_src"
fi
buildroot_patch_stamp="$buildroot_src/.zircon-rv32-musl-patched"
if [ ! -f "$buildroot_patch_stamp" ]; then
    patch -d "$buildroot_src" -p1 < "$system_dir/patches/buildroot-2024.02.12-rv32-musl.patch"
    touch "$buildroot_patch_stamp"
fi
if [ ! -f "$opensbi_src/Makefile" ]; then
    mkdir -p "$opensbi_src"
    tar -xf "$build_dir/sources/opensbi-1.2.tar.gz" --strip-components=1 -C "$opensbi_src"
fi

buildroot_out="$build_dir/buildroot-out"
make -C "$buildroot_src" O="$buildroot_out" BR2_EXTERNAL="$external_dir" zircon_defconfig
for required in \
    BR2_TOOLCHAIN_BUILDROOT_MUSL=y \
    BR2_STATIC_LIBS=y \
    BR2_RISCV_ABI_ILP32F=y \
    BR2_RISCV_ISA_RVF=y; do
    if ! grep -qx "$required" "$buildroot_out/.config"; then
        echo "Buildroot configuration is missing $required" >&2
        exit 1
    fi
done
for forbidden in BR2_RISCV_ISA_RVC BR2_RISCV_ISA_RVD; do
    if ! grep -qx "# $forbidden is not set" "$buildroot_out/.config"; then
        echo "Buildroot configuration unexpectedly enables $forbidden" >&2
        exit 1
    fi
done
rm -f "$buildroot_out/target/usr/bin/zircon-smoke"
make -C "$buildroot_src" O="$buildroot_out" -j"$jobs"

cross="$buildroot_out/host/bin/riscv32-buildroot-linux-musl-"
rootfs="$buildroot_out/images/rootfs.cpio"
linux_out="$build_dir/linux-out"
make -C "$linux_src" O="$linux_out" ARCH=riscv CROSS_COMPILE="$cross" zircon_defconfig
"$linux_src/scripts/config" --file "$linux_out/.config" --set-str INITRAMFS_SOURCE "$rootfs"
make -C "$linux_src" O="$linux_out" ARCH=riscv CROSS_COMPILE="$cross" olddefconfig
make -C "$linux_src" O="$linux_out" ARCH=riscv CROSS_COMPILE="$cross" W=e -j"$jobs" \
    Image zircon/zircon-2026.dtb

kernel_image="$linux_out/arch/riscv/boot/Image"
dtb="$linux_out/arch/riscv/boot/dts/zircon/zircon-2026.dtb"
opensbi_out="$build_dir/opensbi-out"
make -C "$opensbi_src" O="$opensbi_out" -j"$jobs" \
    CROSS_COMPILE="$cross" \
    PLATFORM=generic \
    PLATFORM_RISCV_XLEN=32 \
    PLATFORM_RISCV_ABI=ilp32 \
    PLATFORM_RISCV_ISA=rv32ima_zicsr_zifencei \
    FW_PAYLOAD_PATH="$kernel_image" \
    FW_FDT_PATH="$dtb" \
    FW_PAYLOAD_FDT_ADDR=0x82200000

firmware="$opensbi_out/platform/generic/firmware/fw_payload.elf"
cp "$firmware" "$build_dir/fw_payload.elf"
cp "$kernel_image" "$build_dir/Image"
cp "$dtb" "$build_dir/zircon-2026.dtb"

"$system_dir/scripts/check-isa.sh" "${cross}objdump" "${cross}readelf" \
    "$linux_out/vmlinux" "$firmware" "$buildroot_out/target"
"$system_dir/scripts/check-image.py" "$build_dir/fw_payload.elf" "$kernel_image" "$dtb"

if "${cross}readelf" -A "$buildroot_out/target/usr/bin/zircon-validation" | grep -q 'd[0-9]'; then
    echo "zircon-validation advertises the D extension" >&2
    exit 1
fi

echo "built $build_dir/fw_payload.elf"
