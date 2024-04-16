WORK_DIR = $(shell pwd)

CFLAGS = -fno-pic -mcmodel=medany -static -O2 -MMD -Wall -ggdb $(INCFLAGS) \
         -fno-builtin -fno-stack-protector -ffreestanding -Wno-main

CC = riscv64-linux-gnu-gcc

LD = riscv64-linux-gnu-ld

INCFLAGS = -I usr/include

APPS_LINKER_SCRIPT = $(WORK_DIR)/../user/src/linker.ld

LDFLAGS = -melf64lriscv -T $(APPS_LINKER_SCRIPT)

all: 
	$(CC) $(CFLAGS) -c -o syscall.o syscall.c
	$(LD) $(LDFLAGS) syscall.o -o syscall
	# cp syscall ../elf/syscall
	# rm -rf syscall

clean:
	rm -f *.d
	rm -f *.o
	cd ../elf && rm -f *