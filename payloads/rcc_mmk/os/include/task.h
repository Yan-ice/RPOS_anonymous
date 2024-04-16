#ifndef _TASK_H_
#define _TASK_H_

#include <stdbool.h>
#include <stdint.h>

#include "config.h"
#include "external.h"
#include "fs.h"
#include "mm.h"
#include "trap.h"

#define TASK_STATUS_READY 0
#define TASK_STATUS_RUNNING 1
#define TASK_STATUS_ZOMBIE 2
#define TASK_STATUS_EXITED 3

#define MAX_TASK_NUM 64
#define MAX_FILE_NUM 16

#define BIG_STRIDE 100000
#define MAX_PRIORITY 32
#define DEFAULT_PRIORITY 16

typedef uint64_t PidHandle;
typedef uint64_t TaskStatus;

typedef struct {
  PidHandle pid;
} KernelStack;

typedef struct {
  uint64_t ra;
  uint64_t sp;
  uint64_t s[12];
} TaskContext;


typedef struct {
    uint64_t set_child_tid;
    uint64_t clear_child_tid;
} ProcAddress;

typedef struct TaskControlBlock TaskControlBlock;

struct TaskControlBlock {
  PidHandle pid;
  KernelStack kernel_stack;
  PhysPageNum trap_cx_ppn;
  uint64_t heap_base;
  uint64_t heap_pt;
  TaskContext task_cx;
  TaskStatus task_status;
  MemorySet memory_set;
  TaskControlBlock *parent;
  struct vector children;
  int exit_code;
  File *fd_table[MAX_FILE_NUM];
  Mailbox mailbox;
  OSInode *elf_inode;

  // stride scheduling
  uint64_t priority;
  uint64_t stride;
  ProcAddress address;
};

typedef struct {
  TaskControlBlock *ready_queue[MAX_TASK_NUM];
  uint64_t head;
  uint64_t tail;
} TaskManager;

// task.c
void task_init();
void task_suspend_current_and_run_next();
void task_exit_current_and_run_next(int exit_code);
MemorySet *task_current_memory_set();

// task_control_block.c
TrapContext *task_control_block_get_trap_cx(TaskControlBlock *s);
uint64_t task_control_block_get_user_id(TaskControlBlock *s);
int64_t task_control_block_alloc_fd(TaskControlBlock *s);
void task_control_block_dealloc_fd(TaskControlBlock *s);
void task_control_block_new(TaskControlBlock *s, uint8_t *elf_data,
                            size_t elf_size);
void task_control_block_free(TaskControlBlock *s);
void task_control_block_exec(TaskControlBlock *s, uint8_t *elf_data,
                             size_t elf_size);
TaskControlBlock *task_control_block_fork(TaskControlBlock *parent);
TaskControlBlock *task_control_block_spawn(TaskControlBlock *parent,
                                           uint8_t *elf_data, size_t elf_size);

// task_manager.c
void task_manager_init();
bool task_manager_almost_full();
int64_t task_manager_add_task(TaskControlBlock *task);
TaskControlBlock *task_manager_fetch_task();
TaskControlBlock *task_manager_fetch_task_by_pid(uint64_t pid);

// task_context.c
void task_context_zero_init(TaskContext *cx);
void task_context_goto_trap_return(TaskContext *cx, uint64_t kstack_ptr);

// pid.c
void pid_allocator_init();
PidHandle pid_alloc();
void pid_dealloc(PidHandle pid);
void pid_allocator_print();
void kernel_stack_new(KernelStack *ks, PidHandle pid);
void kernel_stack_free(KernelStack *ks);
uint64_t kernel_stack_get_top(KernelStack *ks);

// processor.c
void processor_run_tasks();
TaskControlBlock *processor_take_current_task();
TaskControlBlock *processor_current_task();
uint64_t processor_current_user_id();
TrapContext *processor_current_trap_cx();
void processor_schedule(TaskContext *switched_task_cx_ptr);

uint64_t grow_proc(TaskControlBlock* task, uint64_t lens);

#endif // _TASK_H_
