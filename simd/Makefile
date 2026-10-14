.DEFAULT_GOAL := all
# 工具链与输出
CLANG ?= clang
LLVM_OBJDUMP ?= llvm-objdump
LLVM_READELF ?= llvm-readelf
SPIKE_ENCODING ?= $(HOME)/.local/opt/riscv-isa-sim/include/riscv/encoding.h
SIMULATOR ?= ../../build/performance/bin/zircon-sim
ISA := rv32imf_zicsr_zicntr_zve32f_zvl256b
TOOLCHAIN := $(shell $(CLANG) -dumpversion)
OUT := build/performance-vlen256-axi256-clang$(TOOLCHAIN)
TARGET_FLAGS := --target=riscv32-unknown-elf -march=$(ISA) -mabi=ilp32f
CFLAGS := $(TARGET_FLAGS) -O3 -g -ffp-contract=off -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -Iruntime -Ibenchmarks
SCALAR_FLAGS := -fno-vectorize -fno-slp-vectorize
SCALAR_CC := python3 tools/compile_scalar.py $(CLANG)
LDFLAGS := $(TARGET_FLAGS) -fuse-ld=lld -nostdlib -Wl,-T,runtime/linker.ld -Wl,--gc-sections
VARIANTS := scalar auto rvv
ELFS := $(addprefix $(OUT)/,$(addsuffix /simd.elf,$(VARIANTS)))
FMA_K_TILE ?= 32
FMA_KERNEL_SET ?= all
FMA_OUT := $(OUT)/fma-k$(FMA_K_TILE)-$(FMA_KERNEL_SET)

.PHONY: all audit run coremark print-coremark-elf smoke memory-smoke serial-smoke scalar-wakeup-smoke state-smoke isa-smoke isa-groups isa-memory sgemm stream register-throughput fma-gemm
all: $(ELFS) audit
# 五项计算程序与公共运行时
$(OUT)/twiddle.c: tools/gen_twiddle.py Makefile
	@mkdir -p $(@D)
	python3 $< > $@
$(OUT)/runtime.o: runtime/runtime.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/start.o: runtime/start.S Makefile
	@mkdir -p $(@D)
	$(CLANG) $(TARGET_FLAGS) -c $< -o $@
$(OUT)/reference.o: benchmarks/kernels.c benchmarks/kernels.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DKERNEL_PREFIX=ref_ -c $< -o $@
$(OUT)/twiddle.o: $(OUT)/twiddle.c benchmarks/kernels.h tools/compile_scalar.py Makefile
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/bench.o: benchmarks/bench.c runtime/runtime.h benchmarks/kernels.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/scalar/kernel.o: benchmarks/kernels.c benchmarks/kernels.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/auto/kernel.o: benchmarks/kernels.c benchmarks/kernels.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -c $< -o $@
$(OUT)/rvv/kernel.o: benchmarks/kernels_rvv.c benchmarks/kernels.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -c $< -o $@
$(OUT)/%/simd.elf: $(OUT)/%/kernel.o $(OUT)/bench.o $(OUT)/reference.o $(OUT)/twiddle.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
audit: $(ELFS)
	python3 tools/audit_elf.py --readelf $(LLVM_READELF) --objdump $(LLVM_OBJDUMP) $(ELFS)
run: all
	@for variant in $(VARIANTS); do $(SIMULATOR) --elf $(OUT)/$$variant/simd.elf --max-cycles 1000000000 > $(OUT)/$$variant/run.txt || exit $$?; done
	python3 tools/report.py $(OUT)
# 指令与状态测试
smoke: $(OUT)/arithmetic-smoke.elf
memory-smoke: $(OUT)/memory-smoke.elf
serial-smoke: $(OUT)/serial-smoke.elf
scalar-wakeup-smoke: $(OUT)/scalar-wakeup-smoke.elf
$(OUT)/arithmetic-smoke.o: tests/arithmetic_smoke.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/arithmetic-smoke.elf: $(OUT)/arithmetic-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/memory-smoke.o: tests/memory_smoke.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/memory-smoke.elf: $(OUT)/memory-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/serial-smoke.o: tests/serial_smoke.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/serial-smoke.elf: $(OUT)/serial-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/scalar-wakeup-smoke.o: tests/scalar_wakeup_smoke.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/scalar-wakeup-smoke.elf: $(OUT)/scalar-wakeup-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
state-smoke: $(OUT)/state-smoke.elf
$(OUT)/state-smoke.c: tools/generate_state_smoke.py Makefile
	@mkdir -p $(@D)
	python3 $< --output $@
$(OUT)/state-smoke.o: $(OUT)/state-smoke.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/state-smoke.elf: $(OUT)/state-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
isa-smoke: $(OUT)/isa-smoke.elf
isa-groups: $(OUT)/isa-groups.elf
isa-memory: $(OUT)/isa-memory.elf
$(OUT)/isa-memory.c: tools/generate_memory_smoke.py $(SPIKE_ENCODING) Makefile
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@
$(OUT)/isa-memory.o: $(OUT)/isa-memory.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-memory.elf: $(OUT)/isa-memory.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/isa-groups.c: tools/generate_isa_smoke.py $(SPIKE_ENCODING) Makefile
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@ --lmul all
$(OUT)/isa-groups.o: $(OUT)/isa-groups.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-groups.elf: $(OUT)/isa-groups.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/isa-smoke.c: tools/generate_isa_smoke.py $(SPIKE_ENCODING) Makefile
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@
$(OUT)/isa-smoke.o: $(OUT)/isa-smoke.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-smoke.elf: $(OUT)/isa-smoke.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
# 矩阵、寄存器吞吐与带宽程序
sgemm: $(OUT)/sgemm.elf
stream: $(OUT)/scalar/stream.elf $(OUT)/rvv/stream.elf
register-throughput: $(OUT)/register-throughput.elf
fma-gemm: $(FMA_OUT)/fma-gemm.elf
$(FMA_OUT)/fma-gemm.o: benchmarks/fma_gemm.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	@test '$(FMA_KERNEL_SET)' = all || test '$(FMA_KERNEL_SET)' = seven-row
	$(CLANG) $(CFLAGS) -DFMA_K_TILE=$(FMA_K_TILE) $(if $(filter seven-row,$(FMA_KERNEL_SET)),-DFMA_GEMM_FOCUS) -c $< -o $@
$(FMA_OUT)/fma-gemm.elf: $(FMA_OUT)/fma-gemm.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/stream-bench.o: benchmarks/stream_bench.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/stream-reference.o: benchmarks/stream_kernels.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DSTREAM_PREFIX=ref_ -c $< -o $@
$(OUT)/scalar/stream-kernels.o: benchmarks/stream_kernels.c runtime/runtime.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/rvv/stream-kernels.o: benchmarks/stream_kernels.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -DSTREAM_RVV -c $< -o $@
$(OUT)/%/stream.elf: $(OUT)/%/stream-kernels.o $(OUT)/stream-bench.o $(OUT)/stream-reference.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/register-throughput.o: benchmarks/register_throughput.c runtime/runtime.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/register-throughput.elf: $(OUT)/register-throughput.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/sgemm.o: benchmarks/sgemm.c runtime/runtime.h benchmarks/kernels.h Makefile
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -fno-vectorize -fno-slp-vectorize -c $< -o $@
$(OUT)/sgemm-reference.o: benchmarks/sgemm.c runtime/runtime.h benchmarks/kernels.h tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DSGEMM_SCALAR_ONLY -c $< -o $@
$(OUT)/sgemm.elf: $(OUT)/sgemm.o $(OUT)/sgemm-reference.o $(OUT)/runtime.o $(OUT)/start.o runtime/linker.ld Makefile
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
# 标量 CoreMark 裸机适配
coremark: $(OUT)/coremark.elf
print-coremark-elf:
	@printf '%s\n' '$(abspath $(OUT)/coremark.elf)'
COREMARK_SOURCES := $(wildcard ../coremark/src/*.c)
$(OUT)/coremark.elf: $(OUT)/runtime.o $(OUT)/start.o runtime/coremark_glue.c runtime/coremark_flags.h tools/compile_scalar.py runtime/linker.ld $(COREMARK_SOURCES) Makefile
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -include runtime/coremark_flags.h -I../coremark/include -I../base-port/tool/include -I../base-port/base/include -DITERATIONS=30 $(LDFLAGS) $(OUT)/runtime.o $(OUT)/start.o runtime/coremark_glue.c ../base-port/tool/src/stdio.c ../base-port/tool/src/int64.c $(COREMARK_SOURCES) -o $@
