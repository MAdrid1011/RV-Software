#!/bin/sh
set -eu

objdump=${1:?objdump is required}
readelf=${2:?readelf is required}
shift 2

check_elf() {
    file=$1
    header=$($readelf -h "$file" 2>/dev/null || true)
    if printf '%s\n' "$header" | grep -Eq 'Flags:.*RVC'; then
        echo "$file: ELF flags advertise compressed instructions" >&2
        exit 1
    fi
    attributes=$($readelf -A "$file" 2>/dev/null || true)
    if printf '%s\n' "$attributes" | grep -Eq 'Tag_RISCV_arch:.*(_|rv32[^ ]*)[cd]([0-9]|_|$)'; then
        echo "$file: RISC-V attributes contain C or D" >&2
        exit 1
    fi
    disassembly=$($objdump -d "$file" 2>/dev/null || true)
    if printf '%s\n' "$disassembly" | grep -Eq '[[:space:]](fld|fsd|f(add|sub|mul|div|min|max|sqrt|sgnj|eq|lt|le)\.d|fcvt\.[^[:space:]]*\.d|fmv\.[^[:space:]]*\.d)'; then
        echo "$file: D-extension instruction found" >&2
        exit 1
    fi
}

for path in "$@"; do
    if [ -d "$path" ]; then
        find "$path" -type f -perm -0100 -print | while IFS= read -r candidate; do
            if file "$candidate" | grep -q 'ELF.*RISC-V'; then
                check_elf "$candidate"
            fi
        done
    else
        check_elf "$path"
    fi
done
