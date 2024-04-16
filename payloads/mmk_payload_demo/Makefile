# Building
PAYLOAD_ELF := target/$(TARGET)/$(MODE)/payload_demo
PAYLOAD_BIN := $(PAYLOAD_ELF)_$(BOARD).bin

# Binutils
OBJDUMP := rust-objdump --arch-name=riscv64
OBJCOPY := rust-objcopy --binary-architecture=riscv64

ifeq ($(MODE), release)
	FLAG := --release
endif

build: $(PAYLOAD_BIN)
	@cp $(PAYLOAD_BIN) $(OUTPUT_PATH)/

env:
	rustup override set nightly-2023-06-25
	(rustup target list | grep "riscv64gc-unknown-none-elf (installed)") || rustup target add $(TARGET)
	rustup component add rust-src
	rustup component add llvm-tools-preview

$(PAYLOAD_BIN): kernel
	@$(OBJCOPY) $(PAYLOAD_ELF) --strip-all -O binary $@

kernel: env
	@echo Platform: $(BOARD)
	@cp src/linker-$(BOARD).ld src/linker.ld
# @cargo build $(FLAG) -Z namespaced-features --features "board_$(BOARD)"
	@cargo build $(FLAG) --features "board_$(BOARD)"
	@echo kernel build over
	@rm src/linker.ld 

clean:
	@cargo clean
