"""生成三种构建共用的 FP32 FFT 旋转因子。"""
import math
import struct
import sys

def f32(value):
    return struct.unpack("f", struct.pack("f", value))[0].hex() + "f"

print('#include "kernels.h"')
print("const float fft_twiddle[FFT_N] = {")
for index in range(512):
    angle = -2 * math.pi * index / 1024
    print(f"    {f32(math.cos(angle))}, {f32(math.sin(angle))},")
print("};")
