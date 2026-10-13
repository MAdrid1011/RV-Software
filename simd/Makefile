.DEFAULT_GOAL := all
CLANG ?= clang
LLVM_OBJDUMP ?= llvm-objdump
LLVM_READELF ?= llvm-readelf
SPIKE_ENCODING ?= $(HOME)/.local/opt/riscv-isa-sim/include/riscv/encoding.h
SIMULATOR ?= ../../build/performance/bin/zircon-sim
ISA := rv32imf_zicsr_zicntr_zve32f_zvl256b
TOOLCHAIN := $(shell $(CLANG) -dumpversion)
OUT := build/performance-vlen256-axi256-clang$(TOOLCHAIN)
TARGET_FLAGS := --target=riscv32-unknown-elf -march=$(ISA) -mabi=ilp32f
CFLAGS := $(TARGET_FLAGS) -O3 -g -ffp-contract=off -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -I.
SCALAR_FLAGS := -fno-vectorize -fno-slp-vectorize
SCALAR_CC := python3 compile_scalar.py $(CLANG)
LDFLAGS := $(TARGET_FLAGS) -fuse-ld=lld -nostdlib -Wl,-T,linker.ld -Wl,--gc-sections
VARIANTS := scalar auto rvv
ELFS := $(addprefix $(OUT)/,$(addsuffix /simd.elf,$(VARIANTS)))

.PHONY: all audit run coremark smoke memory-smoke serial-smoke scalar-wakeup-smoke state-smoke isa-smoke isa-groups isa-memory sgemm stream register-throughput
all: $(ELFS) audit
$(OUT)/twiddle.c: gen_twiddle.py
	@mkdir -p $(@D)
	python3 $< > $@
$(OUT)/runtime.o: runtime.c runtime.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/start.o: start.S
	@mkdir -p $(@D)
	$(CLANG) $(TARGET_FLAGS) -c $< -o $@
$(OUT)/reference.o: kernels.c kernels.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DKERNEL_PREFIX=ref_ -c $< -o $@
$(OUT)/twiddle.o: $(OUT)/twiddle.c compile_scalar.py
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/bench.o: bench.c runtime.h kernels.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/scalar/kernel.o: kernels.c kernels.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/auto/kernel.o: kernels.c kernels.h
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -c $< -o $@
$(OUT)/rvv/kernel.o: kernels_rvv.c kernels.h
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -c $< -o $@
$(OUT)/%/simd.elf: $(OUT)/%/kernel.o $(OUT)/bench.o $(OUT)/reference.o $(OUT)/twiddle.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
audit: $(ELFS)
	python3 audit_elf.py --readelf $(LLVM_READELF) --objdump $(LLVM_OBJDUMP) $(ELFS)
run: all
	@for variant in $(VARIANTS); do $(SIMULATOR) --elf $(OUT)/$$variant/simd.elf --max-cycles 1000000000 > $(OUT)/$$variant/run.txt || exit $$?; done
	python3 report.py $(OUT)
coremark: $(OUT)/coremark.elf
smoke: $(OUT)/arithmetic-smoke.elf
memory-smoke: $(OUT)/memory-smoke.elf
serial-smoke: $(OUT)/serial-smoke.elf
scalar-wakeup-smoke: $(OUT)/scalar-wakeup-smoke.elf
$(OUT)/scalar-wakeup-smoke.o: scalar_wakeup_smoke.c runtime.h compile_scalar.py
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/scalar-wakeup-smoke.elf: $(OUT)/scalar-wakeup-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
state-smoke: $(OUT)/state-smoke.elf
$(OUT)/state-smoke.c: generate_state_smoke.py
	@mkdir -p $(@D)
	python3 $< --output $@
$(OUT)/state-smoke.o: $(OUT)/state-smoke.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/state-smoke.elf: $(OUT)/state-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
isa-smoke: $(OUT)/isa-smoke.elf
isa-groups: $(OUT)/isa-groups.elf
isa-memory: $(OUT)/isa-memory.elf
$(OUT)/isa-memory.c: generate_memory_smoke.py $(SPIKE_ENCODING)
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@
$(OUT)/isa-memory.o: $(OUT)/isa-memory.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-memory.elf: $(OUT)/isa-memory.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/isa-groups.c: generate_isa_smoke.py $(SPIKE_ENCODING)
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@ --lmul all
$(OUT)/isa-groups.o: $(OUT)/isa-groups.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-groups.elf: $(OUT)/isa-groups.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/isa-smoke.c: generate_isa_smoke.py $(SPIKE_ENCODING)
	@mkdir -p $(@D)
	python3 $< --encoding $(SPIKE_ENCODING) --output $@
$(OUT)/isa-smoke.o: $(OUT)/isa-smoke.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/isa-smoke.elf: $(OUT)/isa-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
sgemm: $(OUT)/sgemm.elf
stream: $(OUT)/scalar/stream.elf $(OUT)/rvv/stream.elf
register-throughput: $(OUT)/register-throughput.elf
$(OUT)/stream-bench.o: stream_bench.c runtime.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/stream-reference.o: stream_kernels.c runtime.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DSTREAM_PREFIX=ref_ -c $< -o $@
$(OUT)/scalar/stream-kernels.o: stream_kernels.c runtime.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/rvv/stream-kernels.o: stream_kernels.c runtime.h
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) -DSTREAM_RVV -c $< -o $@
$(OUT)/%/stream.elf: $(OUT)/%/stream-kernels.o $(OUT)/stream-bench.o $(OUT)/stream-reference.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/register-throughput.o: register_throughput.c runtime.h
	@mkdir -p $(@D)
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/register-throughput.elf: $(OUT)/register-throughput.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/serial-smoke.o: serial_smoke.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/serial-smoke.elf: $(OUT)/serial-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/sgemm.o: sgemm.c runtime.h kernels.h
	$(CLANG) $(CFLAGS) -fno-vectorize -fno-slp-vectorize -c $< -o $@
$(OUT)/sgemm-reference.o: sgemm.c runtime.h kernels.h compile_scalar.py
	@mkdir -p $(@D)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -DSGEMM_SCALAR_ONLY -c $< -o $@
$(OUT)/sgemm.elf: $(OUT)/sgemm.o $(OUT)/sgemm-reference.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/memory-smoke.o: memory_smoke.c runtime.h
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/memory-smoke.elf: $(OUT)/memory-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
$(OUT)/arithmetic-smoke.o: arithmetic_smoke.c runtime.h compile_scalar.py
	$(CLANG) $(CFLAGS) $(SCALAR_FLAGS) -c $< -o $@
$(OUT)/arithmetic-smoke.elf: $(OUT)/arithmetic-smoke.o $(OUT)/runtime.o $(OUT)/start.o linker.ld
	$(CLANG) $(LDFLAGS) $(filter %.o,$^) -o $@
COREMARK_SOURCES := $(wildcard ../coremark/src/*.c)
$(OUT)/coremark.elf: $(OUT)/runtime.o $(OUT)/start.o coremark_glue.c coremark_flags.h compile_scalar.py linker.ld $(COREMARK_SOURCES)
	$(SCALAR_CC) $(CFLAGS) $(SCALAR_FLAGS) -include coremark_flags.h -I../coremark/include -I../base-port/tool/include -I../base-port/base/include -DITERATIONS=30 $(LDFLAGS) $(OUT)/runtime.o $(OUT)/start.o coremark_glue.c ../base-port/tool/src/stdio.c ../base-port/tool/src/int64.c $(COREMARK_SOURCES) -o $@
