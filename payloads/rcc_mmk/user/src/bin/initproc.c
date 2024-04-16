#include <stdint.h>

#include "stdio.h"
#include "syscall.h"

int main() {
  printf("I am initproc\n");
  int64_t f = fork();
  int exit_code = 0;
  int64_t pid;
  printf("fork over, I am %d\n", f);
  if (f == 0) {
    printf("I am child\n");
    exec("user_shell\0");
  } else {
    printf("I am father\n");
    while (1) {
      pid = wait(&exit_code);
      if (pid == -1) {
        yield();
        continue;
      }
      printf("[initproc] Released a zombie process, pid=%lld, exit_code=%d\n",
             pid, exit_code);
    }
  }

  return 0;
}
