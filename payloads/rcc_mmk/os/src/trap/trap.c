#include "trap.h"
#include "drivers.h"
#include "log.h"
#include "riscv.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"

static void trap_from_kernel_interrupt(uint64_t cause) {
  int irq;
  switch (cause) {
  case SupervisorTimer:
    //info("timer interrupt\n");
    timer_set_next_trigger();
    break;
  case SupervisorExternal:
    //info("external interrupt\n");
    irq = plic_claim();
    if (irq == VIRTIO0_IRQ) {
      virtio_disk_intr();
    } else if (irq) {
      error("unexpected interrupt irq=%d\n", irq);
    }
    if (irq)
      plic_complete(irq);
    break;
  default:
    panic("device interrupt: scause = 0x%llx, stval = 0x%llx sepc = 0x%llx\n",
          r_scause(), r_stval(), r_sepc());
    break;
  }
}

void trap_from_kernel() {
  uint64_t scause = r_scause();
  uint64_t sstatus = r_sstatus();
  uint64_t stval = r_stval();
  uint64_t sepc = r_sepc();
  
  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kernel trap: not from supervisor mode\n");

  if (scause & (1ULL << 63)) {
    info("kernel trap handled: scause = 0x%llx stval = 0x%llx\n",
          scause, stval);
    trap_from_kernel_interrupt(scause & 0xff);
  } else {
    info("invalid kernel trap found. trying get instruction.\n");
    uint32_t inst = *(uint64_t*)(sepc);
    info("panic instruction: %x\n",inst);
    panic("invalid kernel trap: scause = 0x%llx stval = 0x%llx sepc = 0x%llx\n",
          scause, stval, sepc);

  }
}


void signal_handler(){
  //info("signal handler call.\n");
} 

void kernelvec();


void trap_init() {
  nkapi_config_kernel_delegate_handler((uint64_t)trap_from_kernel);
  nkapi_config_user_delegate_handler((uint64_t)trap_handler);
  nkapi_config_signal_handler((uint64_t)signal_handler);
  w_sie(r_sie() | SIE_SEIE | SIE_SSIE);
  info("trap handler: %x %x\n",(uint64_t)trap_from_kernel, (uint64_t)trap_handler);
  //info("test kernel interrupt:\n");
  //asm("csrr x10,mstatus");
}

void trap_enable_timer_interrupt() {
  // Trap enable timer interrupt
  w_sie(r_sie() | SIE_STIE);
}

void trap_handler() {
  //intr_off();
  TrapContext *cx = processor_current_trap_cx();
  uint64_t scause = r_scause();
  uint64_t stval = r_stval();

  if (scause & (1ULL << 63)) {
    scause &= ~(1ULL << 63);
    switch (scause) {
    case SupervisorTimer:
      timer_set_next_trigger();
      task_suspend_current_and_run_next();
      break;
    default:
      panic("Unsupported interrupt 0x%llx, stval = 0x%llx\n", scause, stval);
      break;
    }
  } else {
    switch (scause) {
    case UserEnvCall:
      cx->sepc += 4;
      cx->x[10] = syscall(cx->x[17], cx->x[10], cx->x[11], cx->x[12]);
      break;
    case StoreFault:
    case StorePageFault:
    case InstructionFault:
    case InstructionPageFault:
    case LoadFault:
    case LoadPageFault:
      info("Exception %lld in application. ", scause);
      info("bad addr = %llx, bad instruction addr = "
           "%llx, core dumped.\n",
            stval, cx->sepc);
      // page fault exit code
      task_exit_current_and_run_next(-2);
      break;
    case IllegalInstruction:
      info("bad addr = %llx, bad instruction addr = "
           "%llx, core dumped.\n",
            stval, cx->sepc);
      info("IllegalInstruction in application, core dumped.\n");
      // illegal instruction exit code
      task_exit_current_and_run_next(-3);
      break;
    default:
      panic("Unsupported exception 0x%llx, stval = 0x%llx\n", scause, stval);
      break;
    }
  }
}

extern void __alltraps();
extern void __restore();

void trap_return() {
  info("in trap return\n");
  uint64_t trap_cx_ptr = TRAP_CONTEXT;
  uint64_t user_satp = processor_current_user_id();
  uint64_t restore_va = (uint64_t)__restore - (uint64_t)__alltraps + TRAMPOLINE;
  asm volatile("fence.i");
  asm volatile("mv x10, %1\n"
               "mv x11, %2\n"
               "jr %0\n"
               :
               : "r"(restore_va), "r"(trap_cx_ptr), "r"(user_satp)
               : "memory", "x10", "x11");
  panic("Unreachable in back_to_user!\n");
}