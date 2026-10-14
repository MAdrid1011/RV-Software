"""Generate all Zve32f vector-memory encodings and legal segment widths."""
import argparse
import csv
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--encoding", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    rows = re.findall(r"#define MATCH_(V\w+) (0x[0-9a-f]+)\n#define MASK_\1 (0x[0-9a-f]+)",
                      args.encoding.read_text())
    output = ['#include "runtime.h"',
              'static uint32_t source[256] __attribute__((aligned(64)));',
              'static uint32_t target[256] __attribute__((aligned(64)));',
              'static uint32_t indices[8] __attribute__((aligned(64)));',
              'int main(void) {',
              '    for (unsigned i = 0; i < 256; ++i) source[i] = i * 0x013579bdu ^ 0x80000003u;']
    count = 0
    coverage = []
    encodings = set()
    for raw_name, raw_match, raw_mask in sorted(rows):
        name, match, mask = raw_name.lower(), int(raw_match, 16), int(raw_mask, 16)
        if match & 0x7f not in (7, 0x27) or (match >> 12) & 7 not in (0, 5, 6):
            continue
        encodings.add(name)
        store = match & 0x7f == 0x27
        mop = (match >> 26) & 3
        lumop = (match >> 20) & 31
        whole = mop == 0 and lumop == 8
        mask_access = mop == 0 and lumop == 11
        eew = {0: 8, 5: 16, 6: 32}[(match >> 12) & 7]
        for fields in ((1,) if whole or mask_access else range(1, 9)):
            for masked in ((False,) if whole or mask_access else (False, True)):
                bits = (8 << 7) | (12 << 15) | ((13 if mop == 2 else 24) << 20)
                bits |= (fields - 1) << 29
                bits |= 0 if masked else 1 << 25
                instruction = match | (bits & ~mask)
                width = 32 if mask_access else eew
                coverage.append((name, f"0x{instruction:08x}", width, eew, "m1", 7,
                                 fields, int(masked)))
                if mop & 1:
                    output += [f'    for (unsigned i = 0; i < 8; ++i) indices[i] = (i / 2) * {fields * width // 8};']
                output += [f'    simd_puts("{name} nf{fields} {"masked" if masked else "unmasked"}\\n");',
                    '    __asm__ volatile(',
                    '        "vl8re32.v v8, (%0)\\n\\t" "vl1re32.v v0, (%0)\\n\\t"',
                    '        "vl1re32.v v24, (%2)\\n\\t" "mv a2, %1\\n\\t"',
                    f'        "li a3, {fields * width // 8}\\n\\t"',
                    f'        "vsetivli zero, 7, e{width}, m1, tu, mu\\n\\t"',
                    f'        ".word 0x{instruction:08x}\\n\\t" "fence rw, rw"',
                    f'        : : "r"(source), "r"({"target" if store else "source"} + 1), "r"(indices)',
                    '        : "memory", "a2", "a3", "v0", "v8", "v9", "v10", "v11",',
                    '          "v12", "v13", "v14", "v15", "v24");']
                count += 1
    output += [f'    simd_puts("Zve32f memory cases: {count}\\n");', '    return 0;', '}']
    args.output.write_text("\n".join(output) + "\n")
    with args.output.with_suffix(".csv").open("w", newline="") as stream:
        writer = csv.writer(stream)
        writer.writerow(("instruction", "encoding", "sew", "eew", "lmul", "vl", "fields", "masked"))
        writer.writerows(coverage)
    print(f"Generated {count} cases from {len(encodings)} Zve32f memory encodings")


if __name__ == "__main__":
    main()
