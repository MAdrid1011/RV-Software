NAMES = coremark
SRCS = $(shell find src/ -name "*.c")
BASE_PORT = $(abspath ../base-port)
SIM_PATH = $(abspath ../../)
ITERATIONS ?= 30
ifeq ($(ITERATIONS),1)
IMAGE = $(abspath ./build/coremark-$(CONFIG_TAG))
else
IMAGE = $(abspath ./build/coremark-i$(ITERATIONS)-$(CONFIG_TAG))
endif
TAR_DIR = $(abspath ./build/$(CONFIG_TAG)-i$(ITERATIONS))
include $(BASE_PORT)/Makefile
CFLAGS += -DITERATIONS=$(ITERATIONS)
