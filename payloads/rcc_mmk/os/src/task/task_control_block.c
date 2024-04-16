#include <stddef.h>

#include "config.h"
#include "string.h"
#include "task.h"
#include "log.h"

TrapContext *task_control_block_get_trap_cx(TaskControlBlock *s) {
  return (TrapContext *)pn2addr(s->trap_cx_ppn);
}

uint64_t task_control_block_get_user_id(TaskControlBlock *s) {
  return memory_set_id(&s->memory_set);
}

int64_t task_control_block_alloc_fd(TaskControlBlock *s) {
  // fd = 0,1,2 is reserved for stdio/stdout/stderr
  for (uint64_t i = 3; i < MAX_FILE_NUM; i++) {
    if (s->fd_table[i] == NULL) {
      s->fd_table[i] = bd_malloc(sizeof(File));
      memset(s->fd_table[i], 0, sizeof(File));
      s->fd_table[i]->proc_ref = 1;
      s->fd_table[i]->file_ref = 1;
      return i;
    } else if (s->fd_table[i]->file_ref == 0) {
      // don't touch proc_ref
      s->fd_table[i]->file_ref = 1;
      s->fd_table[i]->pipe = NULL;
      s->fd_table[i]->inode = NULL;
      s->fd_table[i]->type = FD_NONE;
      s->fd_table[i]->readable = false;
      s->fd_table[i]->writable = false;
      return i;
    }
  }
  return -1;
}

void task_control_block_dealloc_fd(TaskControlBlock *s) {
  File *file;
  // fd = 0,1,2 is reserved for stdio/stdout/stderr
  for (uint64_t i = 3; i < MAX_FILE_NUM; i++) {
    file = s->fd_table[i];
    if (file) {
      if (--file->proc_ref > 0) {
        continue;
      }
      if (file->type == FD_PIPE) {
        pipe_close(file->pipe, file->writable);
      }
      bd_free(file);
    }
  }
}

void task_control_block_new(TaskControlBlock *s, uint8_t *elf_data,
                            size_t elf_size) {
  // memory_set with elf program headers/trampoline/trap context/user stack
  uint64_t user_sp, user_heap;
  uint64_t entry_point;

  s->pid = pid_alloc();
  info("new pid is %d\n", s->pid);
  s->memory_set.page_table = s->pid;
  s->address.clear_child_tid = 0;
  s->address.set_child_tid = 0;
  memory_set_from_elf(&s->memory_set, elf_data, elf_size, &user_sp, &user_heap,
                      &entry_point, 0);

  // nkapi_activate(s->pid);

  // uint64_t* temp = 0;
  // info("data is %lx\n", *(temp));
  // nkapi_activate(0);

  s->trap_cx_ppn = memory_set_translate(
      &s->memory_set, (VirtPageNum)addr2pn((VirtAddr)TRAP_CONTEXT));
  info("trap context ppn of [%d] is 0x%llx\n", s->pid, s->trap_cx_ppn);
  // alloc a pid and a kernel stack in kernel space
  // panic("stop here\n");
  kernel_stack_new(&s->kernel_stack, s->pid);
  uint64_t kernel_stack_top = kernel_stack_get_top(&s->kernel_stack);
  // push a task context which goes to trap_return to the top of kernel stack
  s->heap_base = user_heap;
  s->heap_pt = user_heap;

  info("stack-%llx, heap-%llx ready trapret.\n", kernel_stack_top, user_heap);
  task_context_goto_trap_return(&s->task_cx, kernel_stack_top);
  s->task_status = TASK_STATUS_READY;
  s->parent = NULL;
  vector_new(&s->children, sizeof(TaskControlBlock *));
  s->exit_code = 0;

  // panic("stop here\n");

  // prepare TrapContext in user space
  // identical mapping, so directly set trap context in physical address
  TrapContext *trap_cx = task_control_block_get_trap_cx(s);
  //info("task control block start address is 0x%llx\n", &s);
  // info("task control block kernel stack address is 0x%llx\n", &(s->kernel_stack));
  // info("task control block end address is 0x%llx\n", &(s->stride));
  //info("trap context address is 0x%llx\n", trap_cx);
  // panic("stop here\n");

  app_init_context(entry_point, user_sp, user_heap, kernel_space_id(), kernel_stack_top,
                   (uint64_t)trap_handler, trap_cx);
  // panic("stop here\n");
  info("entry point: %lx\n",entry_point);
  memset(s->fd_table, 0, MAX_FILE_NUM * sizeof(File *));
  memset(&s->mailbox, 0, sizeof(Mailbox));

  // panic("stop here\n");

  s->priority = DEFAULT_PRIORITY;
  s->stride = 0;
}

void task_control_block_free(TaskControlBlock *s) {
  s->task_status = TASK_STATUS_EXITED;
  task_control_block_dealloc_fd(s);
  pid_dealloc(s->pid);
  bd_free(s);
}

void task_control_block_exec(TaskControlBlock *s, uint8_t *elf_data,
                             size_t elf_size) {
  // memory_set with elf program headers/trampoline/trap context/user stack
  uint64_t user_sp, user_heap;
  uint64_t entry_point;
  memory_set_free(&s->memory_set);
  s->address.clear_child_tid = 0;
  s->address.set_child_tid = 0;

  // substitute memory_set
  memory_set_from_elf(&s->memory_set, elf_data, elf_size, &user_sp, &user_heap,
                      &entry_point, 1);

  // update trap_cx ppn
  s->trap_cx_ppn = memory_set_translate(
      &s->memory_set, (VirtPageNum)addr2pn((VirtAddr)TRAP_CONTEXT));

  // initialize trap_cx
  TrapContext *trap_cx = task_control_block_get_trap_cx(s);
  uint64_t kernel_stack_top = kernel_stack_get_top(&s->kernel_stack);
  app_init_context(entry_point, user_sp, user_heap, kernel_space_id(), kernel_stack_top,
                   (uint64_t)trap_handler, trap_cx);
}

TaskControlBlock *task_control_block_fork(TaskControlBlock *parent) {
  TaskControlBlock *s = (TaskControlBlock *)bd_malloc(sizeof(TaskControlBlock));

  //Yan_ice: temporarily modify pid here (maybe cause error)
  s->pid = pid_alloc();
  s->memory_set.page_table = s->pid;
  s->address.clear_child_tid = 0;
  s->address.set_child_tid = 0;

  //printf("tcb fork pid: %d\n",s->pid);
  // copy user space (include trap context)
  memory_set_from_existed_user(&s->memory_set, &parent->memory_set);

  // info("leave from existed user\n");
  // info("after new vector: -------------------\n");

  // MapArea *test_temp = s->memory_set.areas.buffer;
  // for (uint64_t i = 0; i < s->memory_set.areas.size; i++) {
  //   info("buddy store map area: %llx - %llx\n",test_temp[i].vpn_range.l, test_temp[i].vpn_range.r);
  // }

  // info("---------------\n");


  s->trap_cx_ppn = memory_set_translate(
      &s->memory_set, (VirtPageNum)addr2pn((VirtAddr)TRAP_CONTEXT));

  // alloc a pid and a kernel stack in kernel space
  kernel_stack_new(&s->kernel_stack, s->pid);
  uint64_t kernel_stack_top = kernel_stack_get_top(&s->kernel_stack);
  s->heap_pt = parent->heap_pt;
  s->heap_base = parent->heap_base;
  task_context_goto_trap_return(&s->task_cx, kernel_stack_top);
  s->task_status = TASK_STATUS_READY;
  s->parent = parent;
  vector_new(&s->children, sizeof(TaskControlBlock *));
  s->exit_code = 0;

  // copy fd table
  memset(s->fd_table, 0, MAX_FILE_NUM * sizeof(File *));
  for (uint64_t i = 0; i < MAX_FILE_NUM; i++) {
    if (parent->fd_table[i] != NULL) {
      parent->fd_table[i]->proc_ref++;
      parent->fd_table[i]->file_ref++;
      s->fd_table[i] = parent->fd_table[i];
    }
  }

  // create mailbox
  memset(&s->mailbox, 0, sizeof(Mailbox));

  s->priority = parent->priority;
  s->stride = parent->stride;


  // add child
  vector_push(&parent->children, &s);

  // prepare TrapContext in user space
  TrapContext *trap_cx = task_control_block_get_trap_cx(s);
  trap_cx->kernel_sp = kernel_stack_top;

  s->address.clear_child_tid = parent->address.clear_child_tid;
  s->address.set_child_tid = parent->address.set_child_tid;
  return s;
}

TaskControlBlock *task_control_block_spawn(TaskControlBlock *parent,
                                           uint8_t *elf_data, size_t elf_size) {
  TaskControlBlock *s = (TaskControlBlock *)bd_malloc(sizeof(TaskControlBlock));

  // memory_set with elf program headers/trampoline/trap context/user stack
  uint64_t user_sp, user_heap;
  uint64_t entry_point;

  // new memory_set
  memory_set_from_elf(&s->memory_set, elf_data, elf_size, &user_sp, &user_heap,
                      &entry_point, 1);

  // alloc a pid and a kernel stack in kernel space
  s->pid = pid_alloc();
  kernel_stack_new(&s->kernel_stack, s->pid);

  // update trap_cx ppn
  s->trap_cx_ppn = memory_set_translate(
      &s->memory_set, (VirtPageNum)addr2pn((VirtAddr)TRAP_CONTEXT));

  // initialize trap_cx
  TrapContext *trap_cx = task_control_block_get_trap_cx(s);
  uint64_t kernel_stack_top = kernel_stack_get_top(&s->kernel_stack);
  app_init_context(entry_point, user_sp, user_heap, kernel_space_id(), kernel_stack_top,
                   (uint64_t)trap_handler, trap_cx);

  s->heap_pt = parent->heap_pt;
  s->heap_base = parent->heap_base;

  task_context_goto_trap_return(&s->task_cx, kernel_stack_top);
  s->task_status = TASK_STATUS_READY;
  s->parent = parent;
  vector_new(&s->children, sizeof(TaskControlBlock *));
  s->exit_code = 0;

  // create fd table
  memset(s->fd_table, 0, MAX_FILE_NUM * sizeof(File *));

  // create mailbox
  memset(&s->mailbox, 0, sizeof(Mailbox));

  s->priority = parent->priority;
  s->stride = parent->stride;

  // add child
  vector_push(&parent->children, &s);

  return s;
}
