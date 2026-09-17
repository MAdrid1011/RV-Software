-include $(BASE_PORT)/script/riscv32.mk
ifeq ($(MAKECMDGOALS),)
  MAKECMDGOALS  = image
  .DEFAULT_GOAL = image
endif
# Compilers
LLVM_CONFIG ?= llvm-config
LLVM_BIN    ?= $(shell $(LLVM_CONFIG) --bindir 2>/dev/null)
LLD         ?= $(shell command -v ld.lld 2>/dev/null)
ifeq ($(LLVM_BIN),)
  $(error llvm-config was not found; install the LLVM version pinned by Zircon-2026)
endif
ifeq ($(LLD),)
  $(error ld.lld was not found; install the LLD version pinned by Zircon-2026)
endif
CC 			= $(LLVM_BIN)/clang
CXX 		= $(LLVM_BIN)/clang++
AS 			= $(LLVM_BIN)/clang
LD 			= $(LLD)
OBJ_DUMP 	= $(LLVM_BIN)/llvm-objdump
OBJ_COPY 	= $(LLVM_BIN)/llvm-objcopy

# include 
WORK_DIR  = $(abspath .)
INC_PATH = -I$(WORK_DIR)/include
INC_PATH += -I$(BASE_PORT)/base/include
INC_PATH += -I$(BASE_PORT)/tool/include

# FLAGS and LIBS
CFLAGS    += -O2 -Wall -Werror $(INC_PATH) -Wno-main -fno-asynchronous-unwind-tables -fno-builtin -fno-stack-protector -MMD
AFLAGS 	  += -MMD $(INC_PATH) 
LDFLAGS   += -T $(BASE_PORT)/script/linker.ld --defsym=_pmem_start=0x80000000 --defsym=_entry_offset=0x0 
CONFIG_TAG = $(RISCV_ARCH)-$(RISCV_ABI)
LIBS 	  += $(BASE_PORT)/base/build/$(CONFIG_TAG)/libbase.a $(BASE_PORT)/tool/build/$(CONFIG_TAG)/libtool.a
LINKAGE   = $(OBJS) $(LIBS)

# Build directory and OBJS
BUILD_DIR = $(abspath ./build)
IMAGE ?= $(BUILD_DIR)/$(NAMES)-$(CONFIG_TAG)
TAR_DIR ?= $(BUILD_DIR)/$(CONFIG_TAG)
OBJS = $(addprefix $(TAR_DIR)/, $(addsuffix .o, $(basename $(SRCS))))

# Colors
COLOR_RED   		= \033[1;31m
COLOR_GREEN 		= \033[1;32m
COLOR_YELLOW 		= \033[33m
COLOR_NONE  		= \033[0m

# Compile C to object file

$(TAR_DIR)/%.o: %.c
	@mkdir -p $(dir $@) && printf "$(COLOR_YELLOW)[CC]$(COLOR_NONE) $<\n"
	@$(CC) $(CFLAGS) -c -o $@ $(realpath $<)

# Compile assembly to object file
$(TAR_DIR)/%.o: %.S
	@mkdir -p $(dir $@) && printf "$(COLOR_YELLOW)[AS]$(COLOR_NONE) $<\n"
	@$(AS) $(AFLAGS) -c -o $@ $(realpath $<)

# make libbase
$(LIBS):
	@make -s -C $(BASE_PORT)/base libkernel
	@make -s -C $(BASE_PORT)/tool libkernel

# Linkage
$(IMAGE).elf: $(OBJS) $(LIBS)
	@printf "$(COLOR_YELLOW)[LD]$(COLOR_NONE) build/$(notdir $@)\n"
	@$(LD) $(LDFLAGS) -o $@ --start-group $(LINKAGE) --end-group
	@$(OBJ_DUMP) -d $@ > $(IMAGE).txt
	@$(OBJ_COPY) -O binary $@ $(IMAGE).bin

image: $(IMAGE).elf

run: image
	@make -s -C $(SIM_PATH) run IMG=$(IMAGE).bin ARGS=$(ARGS)

gdb: image
	@make -s -C $(SIM_PATH) gdb IMG=$(IMAGE).bin ARGS=$(ARGS)

clean: 
	rm -rf $(BUILD_DIR)
clean-all:
	rm -rf $(BUILD_DIR) $(BASE_PORT)/*/build
