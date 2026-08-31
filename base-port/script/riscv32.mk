CROSS_COMPILE := 
RISCV_ARCH    ?= rv32imaf_zicsr_zifencei
RISCV_ABI     ?= ilp32f
COMMON_FLAGS  := --target=riscv32 -march=$(RISCV_ARCH) -mabi=$(RISCV_ABI) -g -fno-pic 
CFLAGS        += $(COMMON_FLAGS) -static -fdata-sections -ffunction-sections
AFLAGS        += $(COMMON_FLAGS) 
LDFLAGS       += -melf32lriscv -static --gc-sections -e _start
