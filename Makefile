export cwd := $(shell pwd)
export QEMU_SYSTEM := qemu-system-riscv64 # version 4.2.0 in our environment
export OUTPUT_PATH := $(cwd)/output

export TARGET ?= riscv64gc-unknown-none-elf
export MODE ?= release
export BOARD ?= qemu
export PAYLOAD ?= UltraOS_mmk


##########
# building

export SRC_PATH := $(cwd)/codes/mmk
export SBI_PATH := $(cwd)/opensbi_mmk
export K210_PATH := $(cwd)/k210

export PAYLOAD_PATH ?= $(cwd)/payloads/$(PAYLOAD)/

##########

#########
# running

export U_FAT32 := $(OUTPUT_PATH)/fat3.img
export SBI_BIN := $(OUTPUT_PATH)/opensbi_fw_jump_$(BOARD).bin
export MMK_BIN := $(OUTPUT_PATH)/MMK_$(BOARD).bin
export PAYLOAD_BIN := $(OUTPUT_PATH)/$(PAYLOAD)_$(BOARD).bin

#########


ifeq ($(BOARD), qemu)
	export MMK_ENTRY_PA := 0x80200000
	export PAYLOAD_ENTRY_PA := 0x80800000
	export FIRMWARE_ENTRY_PA := 0x80000000
	
else ifeq ($(BOARD), k210)
	export MMK_ENTRY_PA := 0x80020000
	export PAYLOAD_ENTRY_PA := 0x80200000
	export FIRMWARE_ENTRY_PA := 0x80000000
else ifeq ($(BOARD), nezha)
	export MMK_ENTRY_PA := 0x40200000
	export PAYLOAD_ENTRY_PA := 0x40800000
	export FIRMWARE_ENTRY_PA := 0x40000000
endif



all: run

run: build $(PAYLOAD_BIN)
ifeq ($(BOARD), qemu)
	@echo "qemu starting..."
	@$(QEMU_SYSTEM) \
                -machine virt \
                -nographic \
                -bios $(SBI_BIN) \
                -device loader,file=$(MMK_BIN),addr=$(MMK_ENTRY_PA) \
                -device loader,file=$(PAYLOAD_BIN),addr=$(PAYLOAD_ENTRY_PA) \
                -drive file=$(U_FAT32),if=none,format=raw,id=x0 \
        	-device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0\
                -smp threads=2
else ifeq ($(BOARD), k210)
	cd k210 && make run
endif

build: build_sbi build_mmk build_payload build_fs
	
build_mmk:
	cd $(SRC_PATH) && make build

build_sbi:
ifeq ($(BOARD), qemu)
	cd $(SBI_PATH) && make PLATFORM=generic
	cp $(SBI_PATH)/build/platform/generic/firmware/fw_jump.bin $(OUTPUT_PATH)/opensbi_fw_jump_$(BOARD).bin
else ifeq ($(BOARD), k210)
	cd $(SBI_PATH) && make PLATFORM=kendryte/k210
	cp $(SBI_PATH)/build/platform/kendryte/k210/firmware/fw_jump.bin $(OUTPUT_PATH)/opensbi_fw_jump_$(BOARD).bin
endif

build_payload:
	cd $(PAYLOAD_PATH) && make build
	
build_fs:
ifeq ($(BOARD), qemu)
	# rm -f c_linker/*.o
	# cd c_linker && make
	# cp c_linker/ttst fs_tool/content/
	cd fs_tool && make
else ifeq ($(BOARD), k210)
	cd k210 && make build
endif
	
concat_k210:
	cd k210 && make concat

env:
	mkdir -p output
	rustup update
	cargo install cargo-binutils
	cd $(SRC_PATH) && make env

rudra:
	cd $(SRC_PATH) && make rudra

clean:
	cd $(SRC_PATH) && make clean
	cd $(SBI_PATH) && make clean
	cd $(PAYLOAD_PATH) && make clean

