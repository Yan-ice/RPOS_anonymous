# Building

KERNEL_ELF := target/$(TARGET)/$(MODE)/MMK
KERNEL_BIN := $(KERNEL_ELF)_$(BOARD).bin

DISASM_TMP := target/$(TARGET)/$(MODE)/asm

ifeq ($(MODE), release)
	FLAG := --release
endif

SDCARD := /dev/sdb
APPS := ../user/src/bin/*
TOP := ../../k210.bin

# Binutils
OBJDUMP := rust-objdump --arch-name=riscv64
OBJCOPY := rust-objcopy --binary-architecture=riscv64

# Disassembly
DISASM ?= -x



build: $(KERNEL_BIN)
	@cp $(KERNEL_BIN) $(OUTPUT_PATH)/
env:
	rustup override set nightly-2023-06-25
	(rustup target list | grep "riscv64gc-unknown-none-elf (installed)") || rustup target add $(TARGET)
	rustup component add rust-src
	rustup component add llvm-tools-preview

# dev/zero永远输出0
sdcard: 
	@echo "Are you sure write to $(SDCARD) ? [y/N] " && read ans && [ $${ans:-N} = y ]
	@sudo dd if=/dev/zero of=$(SDCARD) bs=1048576 count=16
	@sudo dd if=$(FS_IMG) of=$(SDCARD)

$(KERNEL_BIN): kernel
	@$(OBJCOPY) $(KERNEL_ELF) --strip-all -O binary $@
#	@$(OBJDUMP) -S $(KERNEL_ELF) > $(KERNEL_ELF).S


kernel:
	@echo Platform: $(BOARD)
	@cp src/linker-$(BOARD).ld src/linker.ld
# @cargo build $(FLAG) -Z namespaced-features --features "board_$(BOARD)"
	@cargo build $(FLAG) --features "board_$(BOARD)"
	@echo kernel build over
	@rm src/linker.ld 

clean:
	@cargo clean
