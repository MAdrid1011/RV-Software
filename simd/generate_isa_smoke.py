"""Generate Zve32f arithmetic encoding cases from Spike's public encoding table."""
import argparse
from pathlib import Path
import re

ROOTS = set("""
vaadd vaaddu vadc vadd vand vasub vasubu vcompress vcpop vdiv vdivu
vfadd vfclass vfcvt vfdiv vfirst vfmacc vfmadd vfmax vfmerge vfmin vfmsac
vfmsub vfmul vfmv vfnmadd vfnmacc vfnmsub vfnmsac vfrdiv vfrec7 vfredmax
vfredmin vfredosum vfredusum vfrsqrt7 vfrsub vfsgnj vfsgnjn vfsgnjx
vfslide1down vfslide1up vfsqrt vfsub vid viota vmacc vmadc vmadd vmand
vmandn vmax vmaxu vmerge vmfeq vmfge vmfgt vmfle vmflt vmfne vmin vminu
vmnand vmnor vmor vmorn vmsbc vmsbf vmseq vmsgt vmsgtu vmsif vmsle
vmsleu vmslt vmsltu vmsne vmsof vmul vmulh vmulhsu vmulhu vmv vmxnor
vmxor vnclip vnclipu vnmsac vnmsub vnsra vnsrl vor vredand vredmax
vredmaxu vredmin vredminu vredor vredsum vredxor vrem vremu vrgather
vrgatherei16 vrsub vsadd vsaddu vsbc vsext vslide1down vslide1up
vslidedown vslideup vsll vsmul vsra vsrl vssra vssrl vssub vssubu vsub
vwadd vwaddu vwmacc vwmaccsu vwmaccu vwmaccus vwmul vwmulsu vwmulu
vwredsum vwredsumu vwsub vwsubu vxor vzext
""".split())
CROSS = {"vfwcvt_f_x_v", "vfwcvt_f_xu_v", "vfncvt_x_f_w", "vfncvt_xu_f_w",
         "vfncvt_rtz_x_f_w", "vfncvt_rtz_xu_f_w"}
WHOLE = {f"vmv{count}r_v" for count in (1, 2, 4, 8)}


def cases(header):
    pattern = r"#define MATCH_(V\w+) (0x[0-9a-f]+)\n#define MASK_\1 (0x[0-9a-f]+)"
    for name, match, mask in re.findall(pattern, header):
        name = name.lower()
        root = name.split("_")[0]
        if name in CROSS or name in WHOLE or (root in ROOTS and name != "vcpop_v" and not name.endswith("vf8")):
            yield name, int(match, 16), int(mask, 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--encoding", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--lmul", choices=("m1", "all"), default="m1",
                        help="Exercise m1 or every legal integral/fractional register group")
    args = parser.parse_args()
    instructions = sorted(cases(args.encoding.read_text()))
    output = ['#include "runtime.h"', 'static uint32_t input[64] __attribute__((aligned(64)));',
              'int main(void) {', '    for (unsigned i = 0; i < 64; ++i)',
              '        input[i] = (i * 0x013579bdu) ^ (i & 1 ? 0x80000000u : 0x3f800000u);']
    count = 0
    for name, match, mask in instructions:
        root = name.split("_")[0]
        floating = ((match >> 12) & 7) in (1, 5)
        sews = (16,) if name in CROSS else (32,) if floating else (16, 32) if root in ("vzext", "vsext") else (8, 16) if root.startswith(("vw", "vnclip", "vns")) else (8, 16, 32)
        if name.endswith("vf4"): sews = (32,)
        for sew in sews:
          for lmul in ((0,) if args.lmul == "m1" or name in WHOLE else (-3, -2, -1, 0, 1, 2, 3)):
            # Zve32f fractional configurations require SEW <= LMUL * ELEN.
            if lmul < 0 and sew > 32 * 2 ** lmul:
                continue
            widening = root.startswith("vw") or name.startswith("vfwcvt")
            narrowing = root.startswith(("vnclip", "vns")) or name.startswith("vfncvt")
            reduction = root in ("vwredsum", "vwredsumu")
            if (widening or narrowing) and not reduction and lmul == 3:
                continue
            extension = 2 if name.endswith("vf4") else 1 if name.endswith("vf2") else 0
            if extension and lmul - extension < -3:
                continue
            if root == "vrgatherei16" and lmul + (1 if sew == 8 else -1 if sew == 32 else 0) > 3:
                continue
            group = f"m{1 << lmul}" if lmul >= 0 else f"mf{1 << -lmul}"
            vlmax = int(256 // sew * 2 ** lmul)
            vl = max(1, vlmax - 3) if args.lmul == "all" else 7
            for masked in (False, True):
                if masked and (mask & (1 << 25) or name in WHOLE or root in ("vcompress", "viota")):
                    continue
                fields = (16 << 7) | (24 << 15) | (8 << 20) | (0 if masked else 1 << 25)
                instruction = match | (fields & ~mask)
                output += [f'    simd_puts("{name} e{sew} {group} {"masked" if masked else "unmasked"}\\n");',
                    '    __asm__ volatile(',
                    '        "vl8re32.v v8, (%0)\\n\\t" "vl8re32.v v16, (%0)\\n\\t"',
                    '        "vl8re32.v v24, (%0)\\n\\t" "vl1re32.v v0, (%0)\\n\\t" "li s8, 3\\n\\t"',
                    '        "li t0, 0x3fc00000\\n\\t" "fmv.w.x fs8, t0\\n\\t"',
                    f'        "li t0, {vl}\\n\\t" "vsetvli zero, t0, e{sew}, {group}, tu, mu\\n\\t"',
                    f'        ".word 0x{instruction:08x}"',
                    '        : : "r"(input) : "memory", "s8", "a6", "t0", "fs8", "fa6",',
                    '          "v0", "v8", "v9", "v10", "v11", "v12", "v13", "v14", "v15",',
                    '          "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",',
                    '          "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31");']
                count += 1
    output += [f'    simd_puts("Zve32f arithmetic cases: {count}\\n");', '    return 0;', '}']
    args.output.write_text("\n".join(output) + "\n")
    print(f"Generated {count} cases from {len(instructions)} Zve32f arithmetic encodings")


if __name__ == "__main__":
    main()
