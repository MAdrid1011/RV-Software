ARCH_TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
RV_SOFTWARE_DIR := $(abspath $(ARCH_TEST_DIR)/..)
ZIRCON_ROOT ?= $(abspath $(RV_SOFTWARE_DIR)/..)

BUILD_DIR := $(ARCH_TEST_DIR)/build
ELF_DIR := $(BUILD_DIR)/elf
TEST_DIR := $(ARCH_TEST_DIR)/tests
SIM ?= $(ZIRCON_ROOT)/build/cmake/bin/zircon-sim
CLANG ?= $(shell brew --prefix llvm 2>/dev/null)/bin/clang
OBJDUMP ?= $(shell brew --prefix llvm 2>/dev/null)/bin/llvm-objdump
JOBS ?= $(shell sysctl -n hw.logicalcpu 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)

ISA := rv32imaf_zicsr_zifencei_zaamo_zalrsc_zicntr_zihpm
ABI := ilp32f
COMMON_FLAGS := --target=riscv32 -fuse-ld=lld -march=$(ISA) -mabi=$(ABI) \
    -O0 -g -mcmodel=medany -nostdlib -Wl,--no-relax \
    -T $(ARCH_TEST_DIR)/config/link.ld -I $(ARCH_TEST_DIR)/env \
    -DXLEN=32 -DTEST_FLEN=32
SIM_FLAGS ?= --seed 1 --max-cycles 10000000 --stall-cycles 10000 --no-progress

SOURCES := $(shell find $(TEST_DIR) -type f -name '*.S' | sort)
TEST_NAMES := $(basename $(notdir $(SOURCES)))
TEST_DIRS := $(sort $(dir $(SOURCES)))
ELFS := $(addprefix $(ELF_DIR)/,$(addsuffix .elf,$(TEST_NAMES)))

vpath %.S $(TEST_DIRS)

.PHONY: build build-all list run run-one clean

build:
	+$(MAKE) -j$(JOBS) build-all

build-all: $(ELFS)

list:
	@printf '%s\n' $(TEST_NAMES)

$(ELF_DIR)/%.elf: %.S $(ARCH_TEST_DIR)/config/link.ld $(wildcard $(ARCH_TEST_DIR)/env/*.h)
	@mkdir -p $(@D)
	$(CLANG) $(COMMON_FLAGS) -DTEST_FILE=\"$*\" $< -o $@
	$(OBJDUMP) -d $@ > $@.objdump

run: build
	@set -e; for elf in $(ELFS); do \
	    printf '\n[%s]\n' "$$(basename "$$elf" .elf)"; \
	    $(SIM) --elf "$$elf" $(SIM_FLAGS); \
	done

run-one:
	@test -n "$(TEST)" || { echo 'usage: make run-one TEST=I-add-00'; exit 2; }
	+$(MAKE) $(ELF_DIR)/$(TEST).elf
	$(SIM) --elf $(ELF_DIR)/$(TEST).elf $(SIM_FLAGS)

clean:
	cmake -E rm -rf $(BUILD_DIR)
