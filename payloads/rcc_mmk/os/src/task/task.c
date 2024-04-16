#include "task.h"
#include "fcntl.h"
#include "fs.h"
#include "log.h"
#include "string.h"

static TaskControlBlock INITPROC;

void task_init() {
  pid_allocator_init();
  task_manager_init();

  static uint8_t initproc_elf[MAX_APP_SIZE];
  uint64_t initproc_elf_size = 0;
  
  if(fs_status() == 1){
    info("loading initproc from fs\n");
    //load elf from file system.
    INITPROC.elf_inode = inode_open_file("initproc", O_RDONLY);
    if (!INITPROC.elf_inode) {
      panic("Fail to create initproc\n");
    }
    initproc_elf_size = inode_read_all(INITPROC.elf_inode, initproc_elf);
  
  }else{
    info("loading initproc from memory\n");
    //TODO: load elf from memory here.
    initproc_elf_size = mem_load_pgms("initproc", initproc_elf);
    if(initproc_elf_size == 0){
      panic("Fail to load initproc from mem\n");
    }
  }
  
  // panic("unreachable task init\n");
  task_control_block_new(&INITPROC, initproc_elf, initproc_elf_size);
  task_manager_add_task(&INITPROC);
  info("initproc add finished.\n");
}

void task_suspend_current_and_run_next() {
  // There must be an application running.
  TaskControlBlock *task = processor_take_current_task();
  TaskContext *task_cx_ptr = &task->task_cx;

  // Change status to Ready
  task->task_status = TASK_STATUS_READY;

  // push back to ready queue
  task_manager_add_task(task);

  // jump to scheduling cycle
  processor_schedule(task_cx_ptr);
}

void task_exit_current_and_run_next(int exit_code) {
  // take from Processor
  TaskControlBlock *task = processor_take_current_task();

  // Change status to Zombie
  task->task_status = TASK_STATUS_ZOMBIE;

  // Record exit code
  task->exit_code = exit_code;
  // do not move to its parent but under initproc

  TaskControlBlock **x = (TaskControlBlock **)(task->children.buffer);
  for (uint64_t i = 0; i < task->children.size; i++) {
    x[i]->parent = &INITPROC;
    vector_push(&INITPROC.children, x[i]);
  }
  vector_free(&task->children);
  // deallocate user space
  memory_set_recycle_data_pages(&task->memory_set);
  // deallocate kernel stack

  //Yan_ice: temporarily not free kernel stack, it would cause mem leak
  //kernel_stack_free(&task->kernel_stack);
  
  // we do not have to save task context
  TaskContext _unused;
  task_context_zero_init(&_unused);
  processor_schedule(&_unused);
  
}

MemorySet *task_current_memory_set() {
  TaskControlBlock *task = processor_take_current_task();
  return &task->memory_set;
}

uint64_t grow_proc(TaskControlBlock* task, uint64_t lens){
  info("in grow proc, lens is %lx\n", lens);
    if (lens > 0) {
        uint64_t growed_addr = task->heap_pt + lens;
        uint64_t limit = task->heap_base + USER_HEAP_SIZE;
        if (growed_addr > limit) {
            info("process doesn't have enough memsize to grow %llx, %llx, %llx, %llx\n", limit, task->heap_base, growed_addr, task->pid);
            panic("process heap size boom\n");
        }
        task->heap_pt = growed_addr;
    }
    else {
        uint64_t shrinked_addr = task->heap_pt + lens;
            if (shrinked_addr < task->heap_base) {
                panic("Memory shrinked to the lowest boundary!\n");
            }
            task->heap_pt = shrinked_addr;
    }
    info("process heap arguement %llx, %llx\n", task->heap_base, task->heap_pt);
    return task->heap_base;
}