BENCH ?= app/lift
VARIANT ?= upstream
LLVM_CONFIG ?= llvm-config-23
RISCV_GCC ?= riscv64-unknown-elf-gcc

SINGLE_CORE_CATEGORIES := app kernel sequential
BENCHMARKS := $(foreach category,$(SINGLE_CORE_CATEGORIES),\
	$(patsubst upstream/bench/%,%,$(wildcard upstream/bench/$(category)/*)))
CHECK_BENCHMARKS ?= $(BENCHMARKS)
BENCH_DIR := upstream/bench/$(BENCH)
BENCH_NAME := $(subst /,-,$(BENCH))

ifeq ($(wildcard $(BENCH_DIR)),)
$(error Unknown BENCH '$(BENCH)'; run 'make list' to show supported benchmarks)
endif

ifeq ($(VARIANT),write-combine)
ifneq ($(BENCH),app/lift)
$(error VARIANT=write-combine is only defined for BENCH=app/lift)
endif
NAMES = tacle-lift-write-combine
SRCS = ports/lift-write-combine \
	ports/lift \
	ports/start \
	upstream/bench/app/lift/liftlibcontrol \
	upstream/bench/app/lift/liftlibio
CFLAGS += -DLIFT_LIBRARY_ONLY
else
NAMES = tacle-$(BENCH_NAME)
SRCS = ports/start $(patsubst %.c,%,$(wildcard $(BENCH_DIR)/*.c))
endif

BASE_PORT = $(abspath ../base-port)
SIM = $(abspath ../../build/cmake/bin/zircon-sim)
MAX_CYCLES ?= 500000000
SIM_FLAGS ?= --max-cycles $(MAX_CYCLES) --seed 1
RISCV_BUILTINS := $(shell $(RISCV_GCC) -march=rv32imf -mabi=ilp32f -print-libgcc-file-name 2>/dev/null)

ifeq ($(RISCV_BUILTINS),)
$(error $(RISCV_GCC) could not locate the RV32 soft-float runtime)
endif

LIBS += $(RISCV_BUILTINS)

-include $(BASE_PORT)/Makefile

CFLAGS += -I$(abspath $(BENCH_DIR)) \
	-Wno-unknown-pragmas \
	-Wno-unused-but-set-variable

.PHONY: list suite check sim sim-write-combine

list:
	@printf '%s\n' $(BENCHMARKS)

suite:
	@set -e; \
	for benchmark in $(BENCHMARKS); do \
		printf '[TACLeBench] %s\n' "$$benchmark"; \
		$(MAKE) --no-print-directory BENCH="$$benchmark" image; \
	done

check: suite
	@set -e; \
	for benchmark in $(CHECK_BENCHMARKS); do \
		printf '[TACLeBench] run %s\n' "$$benchmark"; \
		$(MAKE) --no-print-directory BENCH="$$benchmark" sim; \
	done

sim: image
	$(SIM) --elf $(IMAGE).elf $(SIM_FLAGS)

sim-write-combine:
	$(MAKE) BENCH=app/lift VARIANT=write-combine sim
