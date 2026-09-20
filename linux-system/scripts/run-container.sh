#!/bin/sh
set -eu

action=${1:?action is required}
image=${2:?container image is required}
system_dir=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
software_dir=$(CDPATH= cd -- "$system_dir/.." && pwd)
container="zircon-linux-build-$(id -u)"
container_root=/workspace/RV-Software
docker=${DOCKER:-docker}

container_image=$("$docker" image inspect --format '{{.Id}}' "$image")
existing_image=$("$docker" inspect --format '{{.Image}}' "$container" 2>/dev/null || true)
if [ -n "$existing_image" ] && [ "$existing_image" != "$container_image" ]; then
    "$docker" rm -f "$container" >/dev/null
    existing_image=
fi
if [ -z "$existing_image" ]; then
    "$docker" create --name "$container" \
        -u "$(id -u):$(id -g)" \
        -e HOME=/tmp \
        -e JOBS="${JOBS:-4}" \
        -w /workspace \
        "$image" sh -c 'while :; do sleep 3600; done' >/dev/null
fi
"$docker" start "$container" >/dev/null

"$docker" exec -u 0 "$container" sh -c \
    "mkdir -p '$container_root' && chown $(id -u):$(id -g) /workspace '$container_root'"
"$docker" exec -u 0 "$container" sh -c \
    "rm -rf '$container_root/linux' && find '$container_root/linux-system' -mindepth 1 -maxdepth 1 ! -name build -exec rm -rf {} + 2>/dev/null || true"

COPYFILE_DISABLE=1 tar -C "$software_dir/.." -cf - \
    --exclude='RV-Software/linux/.git' \
    --exclude='RV-Software/linux-system/build' \
    RV-Software/linux RV-Software/linux-system |
    "$docker" exec -i "$container" tar -xf - -C /workspace

case "$action" in
image)
    "$docker" exec -w "$container_root/linux-system" "$container" ./scripts/build-image.sh
    ;;
qemu-check)
    "$docker" exec -w "$container_root/linux-system" "$container" ./scripts/run-qemu.sh
    ;;
*)
    echo "unknown container action: $action" >&2
    exit 2
    ;;
esac

mkdir -p "$system_dir/build"
for artifact in fw_payload.elf Image zircon-2026.dtb; do
    "$docker" cp "$container:$container_root/linux-system/build/$artifact" "$system_dir/build/$artifact"
done
if [ "$action" = qemu-check ]; then
    "$docker" cp "$container:$container_root/linux-system/build/qemu-linux-check.log" \
        "$system_dir/build/qemu-linux-check.log"
fi
