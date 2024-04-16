# RPOS
## Description
**Memory Management Kernel** is an abstract of permission above the normal operating system. Considering the increasing number of malicious software and related attacks targeting operating system kernel, we design a new architecture to set up a secure memory area by kernel partition and privilege partition. We hope our architecture can enhance operating system securitysolve this problem.

This reporitory is the implementation of our architecture on RISC-V platform. We use a minor hardware feature to control the paging mechanism in operating system.
If the new platform has the similar hardware feature, our architecture can also be implemented on it. We hope our design can be widely applied to multiple platforms.

## Environment
- [Rust](https://www.rust-lang.org/tools/install): nightly-2023-06-25
- [qemu](https://github.com/qemu/qemu): qemu-system-riscv64 4.0
- target: riscv64gc-unknown-none-elf
- toolchain: [riscv64-unknown-linux-musl-gcc](https://github.com/riscv-collab/riscv-gnu-toolchain)

## Quick start
We provide the [mmk_payload_demo](https://github.com/MemoryManagementKernel/mmk_payload_demo) repository and hope it can help you run your kernel successfully.

You can adjust the arguments in Makefile to adapt your environment. Maybe you should change the value of PAYLOAD_PATH and PAYLOAD_BIN.

``` shell
# check the environment
$ make env

# run the kernel
$ make run 
```

## Ultilize RPOS for your own kernel
You can replace the memory management submodel with the provided API, and put your kernel below the payloads directory. For the kernel in Rust, please use the interface in [mmi_rust](https://github.com/MemoryManagementKernel/mmi_rust) repository. For the kernel in C, please use the static library in [mmi_rust](https://github.com/MemoryManagementKernel/libmmk) repository.

Run the following commands to test your own kernel with RPOS:
``` shell
# check the environment
$ make env

# run the kernel
$ make run PAYLOAD_PATH=<your payload>
```

## About us
If you have any comments, please send an email to us.

Yan_ice Email: [![](https://img.shields.io/badge/-jaddykwind@gmail.com-black?logo=gmail&style=flat)](mailto:jaddykwind@gmail.com)

JADDYK Email: [![](https://img.shields.io/badge/-jaddykwind@gmail.com-black?logo=gmail&style=flat)](mailto:jaddykwind@gmail.com)
