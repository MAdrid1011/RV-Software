"""Generate bare-metal RVV configuration, restart and overlap cases."""
import argparse
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    source = [
        '#include "runtime.h"',
        'static uint32_t input[64] __attribute__((aligned(64)));',
        'static volatile uint32_t branch_value;',
        'int main(void) {',
        '    for (unsigned i = 0; i < 64; ++i) input[i] = i * 0x013579bdu + 0x8642ace0u;',
        '    unsigned failures = 0;',
        '    uint32_t vl, type, start, alias;',
        '    __asm__ volatile("csrwi vcsr, 7\\n\\t" "csrr %0, vxrm\\n\\t"',
        '                     "csrr %1, vxsat" : "=r"(type), "=r"(alias) : : "memory");',
        '    failures += type != 3 || alias != 1;',
        '    __asm__ volatile("csrwi vxrm, 1\\n\\t" "csrwi vxsat, 0\\n\\t"',
        '                     "csrr %0, vcsr" : "=r"(alias) : : "memory");',
        '    failures += alias != 2;',
        '    __asm__ volatile("csrr %0, vlenb" : "=r"(alias));',
        '    failures += alias != 32;',
    ]
    count = 0
    clobbers = ', '.join(f'"v{i}"' for i in (0, *range(8, 32)))
    for sew in (8, 16, 32):
        for lmul in (-2, -1, 0, 1, 2, 3):
            if lmul < 0 and sew > 32 * 2 ** lmul:
                continue
            group = f"m{1 << lmul}" if lmul >= 0 else f"mf{1 << -lmul}"
            maximum = int(256 // sew * 2 ** lmul)
            for policy in range(4):
                tail = "ta" if policy & 1 else "tu"
                mask = "ma" if policy & 2 else "mu"
                expected_type = (policy << 6) | ((sew.bit_length() - 4) << 3) | (lmul & 7)
                for avl in sorted({0, 1, maximum - 1, maximum, maximum + 1}):
                    length = min(avl, maximum)
                    for restart in sorted({0, 1, length // 2, length}):
                        source += [
                            f'    simd_puts("state e{sew} {group} {tail}/{mask} vl={length} start={restart}\\n");',
                            '    __asm__ volatile(',
                            '        "vl8re32.v v8, (%4)\\n\\t" "vl8re32.v v16, (%4)\\n\\t"',
                            '        "vl8re32.v v24, (%4)\\n\\t" "vl1re32.v v0, (%4)\\n\\t"',
                            f'        "li t0, {avl}\\n\\t" "vsetvli %0, t0, e{sew}, {group}, {tail}, {mask}\\n\\t"',
                            '        "csrr %1, vtype\\n\\t"',
                            f'        "li t0, {restart}\\n\\t" "csrw vstart, t0\\n\\t"',
                            '        "vadd.vv v16, v16, v8, v0.t\\n\\t" "csrr %2, vstart\\n\\t"',
                            '        "vadd.vv v8, v16, v8\\n\\t" "csrr %3, vl"',
                            f'        : "=&r"(vl), "=&r"(type), "=&r"(start), "=&r"(alias)',
                            f'        : "r"(input) : "memory", "t0", {clobbers});',
                            f'    failures += vl != {length} || alias != {length} || type != {expected_type} || start != 0;',
                        ]
                        count += 1
    source += [
        '    for (unsigned iteration = 0; iteration < 64; ++iteration) {',
        '        branch_value = iteration & 1;',
        '        if (branch_value) {',
        '            __asm__ volatile("vsetivli zero, 7, e32, m1, tu, mu\\n\\t"',
        '                             "vadd.vi v16, v8, 3" : : : "memory", "v16");',
        '        } else {',
        '            __asm__ volatile("vsetivli zero, 13, e16, m2, ta, ma\\n\\t"',
        '                             "vadd.vi v16, v8, 5" : : : "memory", "v16", "v17");',
        '        }',
        '        __asm__ volatile("csrr %0, vl" : "=r"(vl));',
        '        failures += vl != ((iteration & 1) ? 7u : 13u);',
        '    }',
        f'    simd_puts("SIMD state cases: {count}, failures: ");',
        '    simd_u32(failures); simd_puts("\\n");',
        '    return failures != 0;',
        '}',
    ]
    args.output.write_text("\n".join(source) + "\n")
    print(f"Generated {count} VL, policy, vstart and overlap cases")


if __name__ == "__main__":
    main()
