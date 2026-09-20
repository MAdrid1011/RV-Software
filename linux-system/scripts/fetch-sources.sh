#!/bin/sh
set -eu

build_dir=${1:?build directory is required}
source_dir="$build_dir/sources"
mkdir -p "$source_dir"

fetch() {
    name=$1
    url=$2
    expected=$3
    destination="$source_dir/$name"
    if [ ! -f "$destination" ]; then
        curl -L --fail --retry 3 -o "$destination.tmp" "$url"
        mv "$destination.tmp" "$destination"
    fi
    actual=$(sha256sum "$destination" | awk '{print $1}')
    if [ "$actual" != "$expected" ]; then
        echo "$name: expected SHA256 $expected, got $actual" >&2
        exit 1
    fi
}

fetch buildroot-2024.02.12.tar.xz \
    https://buildroot.org/downloads/buildroot-2024.02.12.tar.xz \
    0b3aee04a6ca267343c821c6f1784d3254d649961970b13602fd19328455b4cd
fetch opensbi-1.2.tar.gz \
    https://github.com/riscv-software-src/opensbi/archive/refs/tags/v1.2.tar.gz \
    8fcbce598a73acc2c7f7d5607d46b9d5107d3ecbede8f68f42631dcfc25ef2b2
