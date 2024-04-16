#include <stdint.h>

#include "config.h"
#include "external.h"
#include "mm.h"
#include "string.h"
#include "log.h"

static uint8_t HEAP_SPACE[KERNEL_HEAP_SIZE] __attribute__((aligned(4096)));

void heap_allocator_init() {
  memset(HEAP_SPACE, 0, KERNEL_HEAP_SIZE);
  info("HEAP SPACE address is 0x%llx\n", &HEAP_SPACE);
  bd_init(HEAP_SPACE, HEAP_SPACE + KERNEL_HEAP_SIZE);
}
