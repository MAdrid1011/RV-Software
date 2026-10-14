"""在相同机器 ISA 下将标量函数限制到标量子集，包含常量内存操作。"""
from pathlib import Path
import subprocess
import sys
import tempfile

arguments = sys.argv[1:]
with tempfile.TemporaryDirectory(prefix="zircon-scalar-") as temporary:
    wrapped = []
    for argument in arguments:
        if argument.endswith(".c"):
            source = Path(argument).resolve()
            wrapper = Path(temporary) / (str(len(wrapped)) + "-" + source.name)
            wrapper.write_text(
                '#pragma clang attribute push(__attribute__((target("arch=rv32imf_zicsr_zicntr"))), apply_to = function)\n'
                f'#include "{source}"\n'
                '#pragma clang attribute pop\n'
            )
            wrapped.append(str(wrapper))
        else:
            wrapped.append(argument)
    subprocess.run(wrapped, check=True)
