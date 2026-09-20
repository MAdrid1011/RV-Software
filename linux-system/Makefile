SHELL := /bin/bash

ROOT := $(abspath ../..)
BUILD := $(CURDIR)/build
CONTAINER_IMAGE := zircon-linux-build:2024.02.12
SIM_BUILD ?= $(ROOT)/build/linux-sim
SIMULATOR := $(SIM_BUILD)/bin/zircon-sim
JOBS ?= 4
SIM_JOBS ?= 4
SIM_THREADS ?= 5
CHECKPOINT_INTERVAL ?= 40000000
CHECKPOINT := $(BUILD)/linux-latest
LOG ?= $(BUILD)/linux-simulation.log
PGO_BUILD ?= $(ROOT)/build/linux-pgo-generate
PGO_PROFILE ?= $(PGO_BUILD)/zircon-linux.profdata
PGO_FINGERPRINT ?= $(PGO_BUILD)/zircon-linux.fingerprint
PGO_TRAINING_CYCLES ?= 40000000
DOCKER ?= docker

ifeq ($(shell uname -s),Darwin)
IMAGE_DEPENDENCY := container
IMAGE_RUN = env DOCKER="$(DOCKER)" JOBS="$(JOBS)" \
	./scripts/run-container.sh image $(CONTAINER_IMAGE)
QEMU_RUN = env DOCKER="$(DOCKER)" JOBS="$(JOBS)" \
	./scripts/run-container.sh qemu-check $(CONTAINER_IMAGE)
else
IMAGE_RUN = env JOBS="$(JOBS)" ./scripts/build-image.sh
QEMU_RUN = ./scripts/run-qemu.sh
endif

.PHONY: all container image qemu-check simulator linux-check linux clean

all: image

container:
	$(DOCKER) build -t $(CONTAINER_IMAGE) .

image: $(IMAGE_DEPENDENCY)
	$(IMAGE_RUN)

qemu-check: image
	$(QEMU_RUN)

simulator: image
	env SIM_BUILD="$(SIM_BUILD)" \
		PGO_BUILD="$(PGO_BUILD)" \
		PGO_PROFILE="$(PGO_PROFILE)" \
		PGO_FINGERPRINT="$(PGO_FINGERPRINT)" \
		PGO_TRAINING_CYCLES="$(PGO_TRAINING_CYCLES)" \
		SIM_JOBS="$(SIM_JOBS)" \
		SIM_THREADS="$(SIM_THREADS)" \
		./scripts/build-simulator.sh

linux-check: simulator
	@printf '\n============================================================\n'
	@printf ' DIFFERENTIAL LINUX BOOT CHECK START\n'
	@printf '============================================================\n\n'
	set -o pipefail; $(SIMULATOR) \
		--elf $(BUILD)/fw_payload.elf \
		--platform linux \
		--pass-marker ZIRCON_LINUX_BOOT_PASS \
		--max-cycles 2000000000 \
		--stall-cycles 5000000 \
		--checkpoint-save $(CHECKPOINT) \
		--checkpoint-interval $(CHECKPOINT_INTERVAL) \
		--progress-interval 30 \
		--no-color 2>&1 | tee -a $(LOG)

linux: simulator
	@printf '\n============================================================\n'
	@printf ' INTERACTIVE DIFFERENTIAL LINUX START\n'
	@printf ' UART input is connected to this terminal.\n'
	@printf '============================================================\n\n'
	set -o pipefail; $(SIMULATOR) \
		--elf $(BUILD)/fw_payload.elf \
		--platform linux \
		--uart-stdio \
		--max-cycles 0xffffffffffffffff \
		--stall-cycles 5000000 \
		--checkpoint-save $(CHECKPOINT) \
		--checkpoint-interval $(CHECKPOINT_INTERVAL) \
		--progress-interval 30 \
		--no-color 2>&1 | tee -a $(LOG)

clean:
	cmake -E remove_directory $(BUILD)
ifeq ($(shell uname -s),Darwin)
	-$(DOCKER) rm -f zircon-linux-build-$(shell id -u)
endif
