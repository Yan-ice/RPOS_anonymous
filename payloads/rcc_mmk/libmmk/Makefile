# Toolchain
ARCH = riscv64-unknown-elf

export CC = $(ARCH)-gcc
export LD = $(ARCH)-ld
export AR = $(ARCH)-ar

# Directories
WORK_DIR = $(shell pwd)
export BUILD_DIR = $(WORK_DIR)/build

$(shell mkdir -p $(BUILD_DIR))

all:
	cd user && make
	cd os && make	

clean:
	rm -rf $(BUILD_DIR)
	cd user && make clean
	cd os && make clean

.PHONY: clean
