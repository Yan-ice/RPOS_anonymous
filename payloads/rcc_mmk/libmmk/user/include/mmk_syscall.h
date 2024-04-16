#ifndef _MMKSYSCALL_H_
#define _MMKSYSCALL_H_

#include <stdint.h>

#define MMK_SYSCALL_ECHO 0x400
#define MMK_SYSCALL_MEASURE 0x401
#define MMK_SYSCALL_PKCS 0x402

int64_t mmk_syscall_echo(uint64_t value);
int64_t mmk_syscall_measure();
int64_t mmk_syscall_pkcs(uint64_t func_id, uint64_t *params);

#endif // _SYSCALL_H_
