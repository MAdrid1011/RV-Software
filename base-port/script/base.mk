
# make ar
LLVM_CONFIG ?= llvm-config
LLVM_BIN    ?= $(shell $(LLVM_CONFIG) --bindir 2>/dev/null)
ifeq ($(LLVM_BIN),)
  $(error llvm-config was not found; install the LLVM version pinned by Zircon-2026)
endif
AR = $(LLVM_BIN)/llvm-ar
CC = $(LLVM_BIN)/clang
AS = $(LLVM_BIN)/clang

RISCV_ARCH ?= rv32imaf_zicsr_zifencei
RISCV_ABI  ?= ilp32f
COMMON_FLAGS = -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -Os --target=riscv32 -g

CFLAGS = -MMD $(COMMON_FLAGS) $(INC_PATH)
CFLAGS += -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector 
AFLAGS = $(COMMON_FLAGS)
ARFLAGS = rcs

CONFIG_TAG = $(RISCV_ARCH)-$(RISCV_ABI)
BUILD_DIR = $(abspath ./build)/$(CONFIG_TAG)
TAR_DIR = $(BUILD_DIR)/obj
OBJS = $(addprefix $(TAR_DIR)/, $(addsuffix .o, $(basename $(KER_SRCS))))
LIBKER = $(BUILD_DIR)/lib$(LIBNAME).a

# Compile C to object file
$(TAR_DIR)/%.o: %.c
	@mkdir -p $(dir $@) && printf "\033[33m[CC]\033[0m $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $(realpath $<)

# Compile assembly to object file
$(TAR_DIR)/%.o: %.S
	@mkdir -p $(dir $@) && printf "\033[33m[AS]\033[0m $<\n"
	@$(AS) $(AFLAGS) -c -o $@ $(realpath $<)

libkernel: $(LIBKER)
$(LIBKER): $(OBJS)
	@printf "\033[33m[AR]\033[0m build/$(notdir $@)\n"
	@$(AR) $(ARFLAGS) $@ $(realpath $^)

clean:
	rm -rf $(BUILD_DIR)
