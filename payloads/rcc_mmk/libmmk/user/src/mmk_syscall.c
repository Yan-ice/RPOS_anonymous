#include "mmk_syscall.h"
#include <stdint.h>

static inline int64_t syscall(uint64_t id, uint64_t a0, uint64_t a1,
                              uint64_t a2) {
  int64_t ret;
  asm volatile("mv x10, %1\n"
               "mv x11, %2\n"
               "mv x12, %3\n"
               "mv x17, %4\n"
               "ecall\n"
               "mv %0, x10\n"
               : "=r"(ret)
               : "r"(a0), "r"(a1), "r"(a2), "r"(id)
               : "memory", "x10", "x11", "x12", "x17");
  return ret;
}

int64_t mmk_syscall_echo(uint64_t val) { 
    return syscall(MMK_SYSCALL_ECHO, val, 0, 0); 
}
int64_t mmk_syscall_measure() { 
    return syscall(MMK_SYSCALL_MEASURE, 0, 0, 0); 
}
int64_t mmk_syscall_pkcs(uint64_t call_id, uint64_t *params_ptr) { 
    return syscall(MMK_SYSCALL_PKCS, call_id, params_ptr, 0); 
}
