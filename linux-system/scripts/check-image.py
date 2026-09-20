#!/usr/bin/env python3
import os
import pathlib
import re
import subprocess
import sys

RAM_START = 0x80000000
RAM_END = 0x84000000
PAYLOAD_START = 0x80400000
FDT_START = 0x82200000


def fail(message: str) -> None:
    raise SystemExit(message)


if len(sys.argv) != 4:
    fail("usage: check-image.py <fw_payload.elf> <Image> <dtb>")

firmware, image, dtb = map(pathlib.Path, sys.argv[1:])
for artifact in (firmware, image, dtb):
    if not artifact.is_file() or artifact.stat().st_size == 0:
        fail(f"missing or empty artifact: {artifact}")

if PAYLOAD_START + image.stat().st_size > FDT_START:
    fail("Linux Image overlaps the reserved DTB copy address")
if FDT_START + dtb.stat().st_size > RAM_END:
    fail("DTB copy exceeds Zircon RAM")

readelf = os.environ.get("READELF", "readelf")
program_headers = subprocess.check_output([readelf, "-lW", firmware], text=True)
load_pattern = re.compile(
    r"^\s*LOAD\s+0x[0-9a-f]+\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+"
    r"0x([0-9a-f]+)\s+0x([0-9a-f]+)",
    re.MULTILINE,
)
segments = load_pattern.findall(program_headers)
if not segments:
    fail("fw_payload.elf has no loadable segments")
for virtual, physical, file_size, memory_size in segments:
    start = int(physical, 16)
    end = start + int(memory_size, 16)
    if start < RAM_START or end > RAM_END:
        fail(f"firmware LOAD segment 0x{start:08x}-0x{end:08x} exceeds Zircon RAM")

print(
    f"image layout: firmware={firmware.stat().st_size} bytes, "
    f"Image={image.stat().st_size} bytes, dtb={dtb.stat().st_size} bytes"
)
