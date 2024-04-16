#include <stdint.h>

#include "drivers.h"
#include "fs.h"
#include "log.h"
#include "task.h"
#include "timer.h"
#include "trap.h"

extern uint8_t sbss, ebss, ekernel;

void clear_bss() {
  for (uint8_t *i = &sbss; i < &ebss; i++) {
    *i = 0;
  }
}

void main() {
  //clear_bss();
  nkapi_config_allocator_range((unsigned long)&ekernel, ((unsigned long)&ekernel) + 0x400000);

  info("rcc_mmk init 1.\n");

  mm_init();
  info("Memory init success.\n");

  trap_init();
  info("Trap init success.\n");

  plic_init();
  info("Plic init success.\n");

  inode_root_init();

  task_init();

  // trap_enable_timer_interrupt();

  // timer_set_next_trigger();

  processor_run_tasks();

  panic("Unreachable in main!\n");
}
