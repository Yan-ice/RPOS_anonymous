
env:
	rustup update
	cargo install cargo-binutils
	cd codes/os && make env

run: build
	cd codes/user && make elf
	cd codes/os && make run

build:
	cd codes/user && make elf
	cd codes/os && make build
	cp codes/os/target/riscv64gc-unknown-none-elf/release/UltraOS.bin $(OUTPUT_PATH)/UltraOS_mmk_$(BOARD).bin

