ARCH_TEST_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
RV_SOFTWARE_DIR := $(abspath $(ARCH_TEST_DIR)/..)
ZIRCON_ROOT ?= $(abspath $(RV_SOFTWARE_DIR)/..)
UPSTREAM := $(RV_SOFTWARE_DIR)/third_party/riscv-arch-test
CONFIG := $(ARCH_TEST_DIR)/config/test_config.yaml
WORK := $(ARCH_TEST_DIR)/build
SIM ?= $(ZIRCON_ROOT)/build/cmake/bin/zircon-sim
TOOLCHAIN_BIN ?= $(shell brew --prefix llvm 2>/dev/null | sed 's|$$|/bin|')
ARCH_TEST_TOOLS := $(ARCH_TEST_DIR)/toolchain
ELF_DIR := $(WORK)/zircon-rv32im-zicsr-clang/elfs/rv32i/Zicsr
TESTS := csrrw csrrs csrrc csrrwi csrrsi csrrci
ELFS := $(addprefix $(ELF_DIR)/Zicsr-,$(addsuffix -00.elf,$(TESTS)))

.PHONY: build run-smoke run clean

build: ; cd $(UPSTREAM) && PATH="$(ARCH_TEST_TOOLS):$(TOOLCHAIN_BIN):$$PATH"; export PATH; mise exec -- uv run act $(CONFIG) --workdir $(WORK) --extensions Zicsr --jobs 0

run-smoke: build ; $(SIM) --elf $(ELF_DIR)/Zicsr-csrrw-00.elf --seed 1 --max-cycles 1000000 --stall-cycles 10000

run: build ; @for elf in $(ELFS); do $(SIM) --elf $$elf --seed 1 --max-cycles 1000000 --stall-cycles 10000 --no-progress || exit $$?; done

clean: ; $(RM) -r $(WORK)
