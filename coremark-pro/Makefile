.DEFAULT_GOAL := all
CLANG ?= clang
ISA := rv32imf_zicsr_zicntr_zve32f_zvl256b
TOOLCHAIN := $(shell $(CLANG) -dumpversion)
OUT := build/performance-vlen256-axi256-clang$(TOOLCHAIN)
PICOLIBC ?= build/picolibc-install
GCC_BUILTINS ?= /usr/lib/gcc/riscv64-unknown-elf/13.2.0/rv32imf/ilp32f/libgcc.a
SIMULATOR ?= ../../build/performance/bin/zircon-sim
MAX_CYCLES ?= 5000000000
UP := upstream
INCLUDES := -I$(PICOLIBC)/include -I$(UP)/mith/include -I$(UP)/mith/al/include
TARGET := --target=riscv32-unknown-elf -march=$(ISA) -mabi=ilp32f
FLAGS := $(TARGET) -std=gnu11 -O3 -g -ffp-contract=off -fno-vectorize -fno-slp-vectorize -fno-pic -fno-stack-protector -ffunction-sections -fdata-sections $(INCLUDES)
DEFINES := -DUSE_FP32=1 -DUSE_SINGLE_CONTEXT=1 -DUSE_CLOCK=1 -DFAKE_FILEIO=1 -DHAVE_PTHREAD=0 -DNO_ALIGNED_ALLOC=1
CC := python3 ../simd/tools/compile_scalar.py $(CLANG)
COMMON := $(wildcard $(UP)/mith/src/*.c) $(UP)/mith/al/src/al_single.c $(UP)/mith/al/src/al_smp.c platform.c adaptation.c
COMMON_OBJ := $(addprefix $(OUT)/,$(COMMON:.c=.o)) $(OUT)/runtime.o
LINPACK := $(UP)/benchmarks/fp/linpack/linpack.c $(UP)/benchmarks/fp/linpack/ref/inputs_f32.c
LOOPS := $(UP)/benchmarks/fp/loops/loops.c $(wildcard $(UP)/benchmarks/fp/loops/ref-sp/*.c)
WORKLOADS := linear_alg-mid-100x100-sp loops-all-mid-10k-sp
ELFS := $(addprefix $(OUT)/,$(addsuffix .elf,$(WORKLOADS)))

.PHONY: all audit run dependencies
dependencies:
	python3 bootstrap.py --prefix $(PICOLIBC)
all: $(ELFS) audit
$(OUT)/%.o: %.c ../simd/tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(CC) $(FLAGS) $(DEFINES) -c $< -o $@
$(OUT)/%/workload.o: $(UP)/workloads/% Makefile
	@mkdir -p $(@D)
	$(CC) $(FLAGS) $(DEFINES) -Dmain=workload_main -c $</$*.c -o $@
$(OUT)/runtime.o: ../simd/runtime/runtime.c ../simd/runtime/runtime.h ../simd/tools/compile_scalar.py Makefile
	@mkdir -p $(@D)
	$(CC) $(FLAGS) $(DEFINES) -c $< -o $@
$(OUT)/start.o: ../simd/runtime/start.S Makefile
	@mkdir -p $(@D)
	$(CLANG) $(TARGET) -c $< -o $@
$(OUT)/linear_alg-mid-100x100-sp.elf: $(addprefix $(OUT)/,$(LINPACK:.c=.o)) $(OUT)/linear_alg-mid-100x100-sp/workload.o
$(OUT)/loops-all-mid-10k-sp.elf: $(addprefix $(OUT)/,$(LOOPS:.c=.o)) $(OUT)/loops-all-mid-10k-sp/workload.o
$(ELFS): $(COMMON_OBJ) $(OUT)/start.o ../simd/runtime/linker.ld Makefile $(PICOLIBC)/lib/libc.a $(PICOLIBC)/lib/libm.a $(GCC_BUILTINS)
	$(CLANG) $(TARGET) -fuse-ld=lld -nostdlib -Wl,-T,../simd/runtime/linker.ld -Wl,--gc-sections $(filter %.o,$^) -Wl,--start-group $(PICOLIBC)/lib/libc.a $(PICOLIBC)/lib/libm.a $(GCC_BUILTINS) -Wl,--end-group -o $@
audit: $(ELFS)
	python3 ../simd/tools/audit_elf.py $(ELFS)
run: all
	@for name in $(WORKLOADS); do $(SIMULATOR) --elf $(OUT)/$$name.elf --max-cycles $(MAX_CYCLES) > $(OUT)/$$name.txt 2>&1 || exit $$?; done
