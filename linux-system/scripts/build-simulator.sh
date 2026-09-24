#!/usr/bin/env bash
set -euo pipefail

system_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
root=$(CDPATH= cd -- "$system_dir/../.." && pwd)
sim_build=${SIM_BUILD:-$root/build/linux-sim}
pgo_build=${PGO_BUILD:-$root/build/linux-pgo-generate}
pgo_profile=${PGO_PROFILE:-$pgo_build/zircon-linux.profdata}
pgo_fingerprint=${PGO_FINGERPRINT:-$pgo_build/zircon-linux.fingerprint}
pgo_pending_fingerprint=${PGO_PENDING_FINGERPRINT:-$pgo_build/zircon-linux.pending-fingerprint}
payload=${LINUX_PAYLOAD:-$system_dir/build/fw_payload.elf}
jobs=${SIM_JOBS:-4}
threads=${SIM_THREADS:-5}
training_cycles=${PGO_TRAINING_CYCLES:-20000000}
training_log=${PGO_TRAINING_LOG:-$system_dir/build/pgo-training.log}

if [[ ! -f $payload ]]; then
    echo "Linux payload is missing: $payload" >&2
    echo "Build the image before building the optimized simulator." >&2
    exit 1
fi

find_spike_pkg_config() {
    local prefix pkgconfig_dir

    if ! command -v pkg-config >/dev/null 2>&1; then
        echo "pkg-config is required to locate the Spike development library" >&2
        return 1
    fi
    if pkg-config --exists riscv-riscv; then
        return
    fi

    for prefix in \
        "${SPIKE_PREFIX:-}" \
        "${RISCV:-}" \
        "${HOME:-}/.local/opt/riscv-isa-sim" \
        "${HOME:-}/.local"; do
        [[ -n $prefix ]] || continue
        pkgconfig_dir="$prefix/lib/pkgconfig"
        [[ -f $pkgconfig_dir/riscv-riscv.pc ]] || continue
        export PKG_CONFIG_PATH="$pkgconfig_dir${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        if pkg-config --exists riscv-riscv; then
            return
        fi
    done

    echo "Spike development metadata riscv-riscv.pc was not found." >&2
    echo "Set SPIKE_PREFIX to the riscv-isa-sim installation prefix." >&2
    return 1
}

find_spike_pkg_config

find_clangxx() {
    local compiler version
    if [[ -n ${CXX:-} ]]; then
        compiler=$CXX
    elif command -v clang++ >/dev/null 2>&1; then
        compiler=$(command -v clang++)
    elif command -v c++ >/dev/null 2>&1; then
        compiler=$(command -v c++)
    else
        echo "Clang++ is required for the default PGO build" >&2
        return 1
    fi
    version=$("$compiler" --version | sed -n '1p')
    if [[ $version != *clang* && $version != *Clang* ]]; then
        echo "The default PGO build requires Clang++, but $compiler reports: $version" >&2
        return 1
    fi
    printf '%s\n' "$compiler"
}

find_llvm_profdata() {
    local bundled
    if [[ -n ${LLVM_PROFDATA:-} ]]; then
        printf '%s\n' "$LLVM_PROFDATA"
    elif bundled=$("$cxx" -print-prog-name=llvm-profdata 2>/dev/null) &&
        [[ -n $bundled ]] && command -v "$bundled" >/dev/null 2>&1; then
        command -v "$bundled"
    elif command -v llvm-profdata >/dev/null 2>&1; then
        command -v llvm-profdata
    elif command -v xcrun >/dev/null 2>&1; then
        xcrun --find llvm-profdata
    else
        echo "llvm-profdata is required for the default PGO build" >&2
        return 1
    fi
}

cxx=$(find_clangxx)
cxx_version=$("$cxx" --version | sed -n '1p')
manifest=$(mktemp "${TMPDIR:-/tmp}/zircon-pgo-manifest.XXXXXX")
trap 'rm -f "$manifest"' EXIT
{
    printf 'jobs=%s\nthreads=%s\ntraining_cycles=%s\n' "$jobs" "$threads" "$training_cycles"
    printf 'cxx=%s\n%s\n' "$cxx" "$cxx_version"
    cmake --version | sed -n '1p'
    verilator --version
    find "$root/src/main/scala" "$root/src/main/resources" "$root/ZirconSim/src" "$root/ZirconSim/include" \
        -type f -print | LC_ALL=C sort | while IFS= read -r file; do
            cksum "$file"
        done
    for file in \
        "$root/CMakeLists.txt" \
        "$root/build.sbt" \
        "$root/project/build.properties" \
        "$root/ZirconSim/CMakeLists.txt" \
        "$system_dir/Makefile" \
        "$system_dir/scripts/build-simulator.sh" \
        "$payload"; do
        cksum "$file"
    done
} >"$manifest"
fingerprint=$(cksum "$manifest" | awk '{ print $1 ":" $2 }')

profile_is_current=false
if [[ -f $pgo_profile && -f $pgo_fingerprint ]] &&
    [[ $(<"$pgo_fingerprint") == "$fingerprint" ]]; then
    profile_is_current=true
fi

collect_raw_profiles() {
    raw_profiles=()
    while IFS= read -r -d '' profile; do
        raw_profiles+=("$profile")
    done < <(find "$pgo_build" -name '*.profraw' -type f -size +0c -print0 2>/dev/null)
}

training_completed() {
    [[ -f $training_log ]] &&
        grep -Eq '"status":"timeout","cycles":'"$training_cycles"',' "$training_log"
}

merge_raw_profiles() {
    collect_raw_profiles
    if [[ ${#raw_profiles[@]} -eq 0 ]]; then
        echo "PGO training did not produce a non-empty raw profile" >&2
        return 1
    fi
    mkdir -p "$(dirname "$pgo_profile")"
    "$llvm_profdata" merge -output="$pgo_profile.tmp" "${raw_profiles[@]}"
    mv "$pgo_profile.tmp" "$pgo_profile"
    printf '%s\n' "$fingerprint" >"$pgo_fingerprint"
    rm -f "$pgo_pending_fingerprint"
}

common_flags=(
    -DCMAKE_CXX_COMPILER="$cxx"
    -DCMAKE_BUILD_TYPE=Release
    -DZIRCON_SIM_ENABLE_CHECKPOINTS=ON
    -DZIRCON_SIM_ENABLE_THIN_LTO=ON
    -DZIRCON_SIM_NATIVE_OPTIMIZATION=ON
    -DZIRCON_VERILATOR_THREADS="$threads"
    -DZIRCON_VERILATOR_BUILD_JOBS="$jobs"
)

if [[ $profile_is_current != true ]]; then
    llvm_profdata=$(find_llvm_profdata)
    collect_raw_profiles
    pending_build_is_current=false
    if [[ -f $pgo_pending_fingerprint ]] &&
        [[ $(<"$pgo_pending_fingerprint") == "$fingerprint" ]] &&
        [[ -x $pgo_build/bin/zircon-sim ]]; then
        pending_build_is_current=true
    fi
    if [[ $pending_build_is_current == true ]] &&
        [[ ${#raw_profiles[@]} -gt 0 ]] && training_completed; then
        printf '\n============================================================\n'
        printf ' PGO PROFILE RECOVERY\n'
        printf ' Reusing a completed %s-cycle training run.\n' "$training_cycles"
        printf '============================================================\n\n'
        merge_raw_profiles
    else
        if [[ $pending_build_is_current == true ]]; then
            printf '\n[PGO TRAINING BUILD REUSE] %s\n\n' "$pgo_build/bin/zircon-sim"
        else
            rm -rf "$pgo_build" "$sim_build"
            printf '\n============================================================\n'
            printf ' PGO INSTRUMENTED SIMULATOR BUILD\n'
            printf '============================================================\n\n'
            cmake -S "$root" -B "$pgo_build" \
                "${common_flags[@]}" \
                -DZIRCON_SIM_PGO_MODE=GENERATE
            cmake --build "$pgo_build" --target zircon-sim --parallel "$jobs"
            printf '%s\n' "$fingerprint" >"$pgo_pending_fingerprint"
        fi

        mkdir -p "$(dirname "$training_log")"
        find "$pgo_build" -name '*.profraw' -type f -delete
        printf '\n============================================================\n'
        printf ' PGO PROFILE TRAINING START\n'
        printf ' Running %s Linux cycles for profile collection.\n' "$training_cycles"
        printf ' This is training, not the interactive Linux session.\n'
        printf '============================================================\n\n'
        set +e
        LLVM_PROFILE_FILE="$pgo_build/zircon-%p.profraw" \
            "$pgo_build/bin/zircon-sim" \
            --elf "$payload" \
            --platform linux \
            --max-cycles "$training_cycles" \
            --stall-cycles 5000000 \
            --allow-timeout \
            --progress-interval 30 \
            --no-color 2>&1 | tee "$training_log"
        training_status=${PIPESTATUS[0]}
        set -e
        if [[ $training_status -ne 0 ]]; then
            echo "PGO training failed with status $training_status" >&2
            exit "$training_status"
        fi
        if ! training_completed; then
            echo "PGO training exited without reaching $training_cycles cycles" >&2
            exit 1
        fi
        merge_raw_profiles
    fi
    printf '\n============================================================\n'
    printf ' PGO PROFILE TRAINING COMPLETE\n'
    printf ' Profile: %s\n' "$pgo_profile"
    printf '============================================================\n\n'
else
    printf '\n[PGO PROFILE REUSE] %s\n\n' "$pgo_profile"
fi

printf '============================================================\n'
printf ' OPTIMIZED SIMULATOR BUILD\n'
printf '============================================================\n\n'
cmake -S "$root" -B "$sim_build" \
    "${common_flags[@]}" \
    -DZIRCON_SIM_PGO_MODE=USE \
    -DZIRCON_SIM_PGO_PROFILE="$pgo_profile"
cmake --build "$sim_build" --target zircon-sim --parallel "$jobs"
