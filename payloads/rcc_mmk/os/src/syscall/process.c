#include <stdint.h>

#include "efs.h"
#include "fcntl.h"
#include "fs.h"
#include "log.h"
#include "mm.h"
#include "sbi.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"
#include "task.h"
#include "timer.h"

int64_t sys_dup(uint64_t fd) {
  // info("[syscall] dup\n");
  TaskControlBlock *task = processor_current_task();
  File *file = task->fd_table[fd];

  if (fd <= 2 || fd > MAX_FILE_NUM || !file || file->file_ref < 1 ||
      file->type == FD_NONE) {
    return -1;
  }
  int64_t new_fd = task_control_block_alloc_fd(task);

  task->fd_table[new_fd]->pipe = task->fd_table[fd]->pipe;
  task->fd_table[new_fd]->inode = task->fd_table[fd]->inode;
  task->fd_table[new_fd]->type = task->fd_table[fd]->type;
  task->fd_table[new_fd]->readable = task->fd_table[fd]->readable;
  task->fd_table[new_fd]->writable = task->fd_table[fd]->writable;

  if (task->fd_table[fd]->type == FD_INODE) {
    task->fd_table[fd]->inode->ref++;
  }
  // info("[syscall] dup over\n");
  return new_fd;
}

int64_t sys_open(char *path, uint32_t flags) {
  // info("[syscall] open\n");
  TaskControlBlock *task = processor_current_task();

  char file_name[NAME_LENGTH_LIMIT + 1];
  copy_byte_buffer(processor_current_user_id(), (uint8_t *)file_name,
                   (uint8_t *)path, NAME_LENGTH_LIMIT + 1, FROM_USER);

  int64_t fd = -1;
  OSInode *inode = inode_open_file(file_name, flags);
  if (inode) {
    fd = task_control_block_alloc_fd(task);
    task->fd_table[fd]->pipe = NULL;
    task->fd_table[fd]->inode = inode;
    task->fd_table[fd]->type = FD_INODE;
    task->fd_table[fd]->readable = (flags & O_WRONLY) == 0;
    task->fd_table[fd]->writable = (flags & O_RDONLY) == 0;
  }
  return fd;
}

int64_t sys_close(uint64_t fd) {
  // info("[syscall] close\n");
  TaskControlBlock *task = processor_current_task();
  File *file = task->fd_table[fd];
  
  // info("fd %d need to be close\n", fd);

  if (fd <= 2 || fd > MAX_FILE_NUM || !file || file->file_ref < 1) {
    return -1;
  }
  if (--file->file_ref > 0) {
    return 0;
  }

  if (file->type == FD_PIPE) {
    // info("[syscall] close type is fd pipe\n");
    pipe_close(file->pipe, file->writable);
  } else if (file->type == FD_INODE) {
    inode_close_file(file->inode);
  }

  file->file_ref = 0;
  file->pipe = NULL;
  file->inode = NULL;
  file->type = FD_NONE;
  file->readable = false;
  file->writable = false;
  // info("[syscall] close over\n");
  return 0;
}

int64_t sys_pipe(uint64_t *pipe) {
  // info("[syscall] pipe\n");
  TaskControlBlock *task = processor_current_task();
  uint64_t token = processor_current_user_id();

  int64_t read_fd = task_control_block_alloc_fd(task);
  int64_t write_fd = task_control_block_alloc_fd(task);
  // info("[in kernel] read fd is %d, write fd is %d\n", read_fd, write_fd);
  if (read_fd < 0 || write_fd < 0) {
    return -1;
  }

  File *pipe_read = task->fd_table[read_fd];
  File *pipe_write = task->fd_table[write_fd];
  pipe_make(pipe_read, pipe_write);

  copy_byte_buffer(token, (uint8_t *)&read_fd, (uint8_t *)(pipe),
                   sizeof(uint8_t), TO_USER);
  copy_byte_buffer(token, (uint8_t *)&write_fd, (uint8_t *)((uint32_t *)pipe + 1),
                   sizeof(uint8_t), TO_USER);
  return 0;
}

int64_t sys_read(uint64_t fd, char *buf, uint64_t len) {
  // info("[syscall] read\n");
  TaskControlBlock *task = processor_current_task();
  File *file = task->fd_table[fd];

  if (fd == FD_STDIN) {
    return stdin_read(buf, len);
  }
  if (fd <= 2 || fd > MAX_FILE_NUM || !file) {
    return -1;
  }
  if (!file->readable) {
    return -1;
  }
  if (file->type == FD_PIPE) {
    return pipe_read(file->pipe, buf, len);
  } else if (file->type == FD_INODE) {
    return inode_read(file->inode, buf, len);
  }

  return -1;
}

int64_t sys_write(uint64_t fd, char *buf, uint64_t len) {
  // info("[syscall] write\n");
  TaskControlBlock *task = processor_current_task();
  File *file = task->fd_table[fd];

  if (fd == FD_STDOUT || fd == FD_STDERR) {
    return stdout_write(buf, len);
  }
  if (fd <= 2 || fd > MAX_FILE_NUM || !file) {
    return -1;
  }
  if (!file->writable) {
    return -1;
  }
  if (file->type == FD_PIPE) {
    return pipe_write(file->pipe, buf, len);
  } else if (file->type == FD_INODE) {
    return inode_write(file->inode, buf, len);
  }

  return -1;
}

int64_t sys_exit(int exit_code) {
  // info("[syscall] exit\n");
  // info("Application (pid = %lld) exited with code %d\n",
      //  processor_current_task()->pid, exit_code);
  //info("Remaining physical pages %lld\n", frame_remaining_pages());
  task_exit_current_and_run_next(exit_code);
  panic("Unreachable in sys_exit!\n");
  return 0;
}

int64_t sys_yield() {
  // info("[syscall] yield\n");
  task_suspend_current_and_run_next();
  return 0;
}

int64_t sys_set_priority(int64_t prio) {
  // info("[syscall] priority\n");
  if (prio < 2) {
    return -1;
  }
  // todo: implement set_priority
  return prio;
}

int64_t sys_get_time(TimeVal *ts, int64_t tz) {
  // info("[syscall] get time\n");
  TimeVal sys_ts;
  int64_t time_us = timer_get_time_us();
  sys_ts.sec = time_us / USEC_PER_SEC;
  sys_ts.usec = time_us % USEC_PER_SEC;
  copy_byte_buffer(processor_current_user_id(), (uint8_t *)&sys_ts,
                   (uint8_t *)ts, sizeof(TimeVal), TO_USER);
  return 0;
}

int64_t sys_getpid() {
  // info("[syscall] getpid\n");
  TaskControlBlock *task = processor_current_task();
  return (int64_t)task->pid;
}

int64_t sys_munmap(uint64_t start, uint64_t len) {
  // info("[syscall] munmap\n");
  MemorySet *memory_set = task_current_memory_set();
  return memory_set_munmap(memory_set, start, len);
}

int64_t sys_fork() {
  // info("[syscall] fork\n");
  if (task_manager_almost_full()) {
    // info("task manager full\n");
    return -1;
  }

  TaskControlBlock *current_task = processor_current_task();
  TaskControlBlock *new_task = task_control_block_fork(current_task);
  PidHandle new_pid = new_task->pid;

  // modify trap context of new_task, because it returns immediately after
  // switching
  TrapContext *trap_cx = task_control_block_get_trap_cx(new_task);

  // we do not have to move to next instruction since we have done it before
  // for child process, fork returns 0
  trap_cx->x[10] = 0;

  // add new task to scheduler
  task_manager_add_task(new_task);
  return (int64_t)new_pid;
}

int64_t sys_exec(char *path) {
  // info("[syscall] exec \n");
  char app_name[NAME_LENGTH_LIMIT + 1];
  copy_byte_buffer(processor_current_user_id(), (uint8_t *)app_name,
                   (uint8_t *)path, NAME_LENGTH_LIMIT + 1, FROM_USER);

  static uint8_t data[MAX_APP_SIZE];
  size_t size;
  TaskControlBlock *task;


  if (fs_status()) {
    OSInode *inode = inode_open_file(app_name, O_RDONLY);
    if(inode){
      task = processor_current_task();
      task->elf_inode = inode;
      size = inode_read_all(task->elf_inode, data);
      
    }else{
      return -1;
    }
  } else {
    task = processor_current_task();
    //task->elf_inode = NULL;
    size = mem_load_pgms(app_name, data);
    // info("mem over\n");
  }
  task_control_block_exec(task, data, size);
  return 0;
}

int64_t sys_mmap(uint64_t start, uint64_t len, uint64_t prot) {
  // info("[syscall] mmap\n");
  MemorySet *memory_set = task_current_memory_set();
  return memory_set_mmap(memory_set, start, len, prot);
}

int64_t sys_waitpid(int64_t pid, int *exit_code_ptr) {
  // info("[syscall] wait pid\n");
  TaskControlBlock *task = processor_current_task();
  // info("I want to wait pid %d\n", pid);
  // find a child process
  bool found = false;
  uint64_t found_idx;
  PidHandle found_pid;
  int exit_code;
  TaskControlBlock **x = (TaskControlBlock **)(task->children.buffer);
  for (int64_t i = task->children.size - 1; i >= 0; i--) {
    if (pid == x[i]->pid || pid == -1) {
      found = true;
      found_idx = i;
      found_pid = x[i]->pid;
      exit_code = x[i]->exit_code;
      break;
    }
  }
  if (!found) {
    return -1;
  }

  TaskControlBlock *task_child = x[found_idx];

  if (task_child->task_status == TASK_STATUS_ZOMBIE) {
    task_control_block_free(task_child);
    vector_remove(&task->children, found_idx);
    copy_byte_buffer(memory_set_id(&task->memory_set), (uint8_t *)&exit_code,
                     (uint8_t *)exit_code_ptr, sizeof(int), TO_USER);
    return (int64_t)found_pid;
  } else {
    // info("here\n");
    return -2;
  }
}

int64_t sys_spawn(char *path) {
  // info("[syscall] spawn\n");
  if (task_manager_almost_full()) {
    return -1;
  }

  TaskControlBlock *current_task = processor_current_task();

  char app_name[NAME_LENGTH_LIMIT + 1];
  copy_byte_buffer(processor_current_user_id(), (uint8_t *)app_name,
                   (uint8_t *)path, NAME_LENGTH_LIMIT + 1, FROM_USER);

  static uint8_t data[MAX_APP_SIZE];
  size_t size;
  TaskControlBlock *new_task;
  OSInode *inode = inode_open_file(path, O_RDONLY);

  if (inode) {
    size = inode_read_all(inode, data);
    new_task = task_control_block_spawn(current_task, data, size);
    new_task->elf_inode = inode;
    task_manager_add_task(new_task);
    return (int64_t)new_task->pid;
  } else {
    return -1;
  }
}

int64_t sys_mailread(char *buf, uint64_t len) {
  // info("[syscall] mailread\n");
  TaskControlBlock *task = processor_current_task();

  if (task->mailbox.write_mails == task->mailbox.read_mails) {
    return -1;
  }
  if (len == 0) {
    return 0;
  }
  if (len > MAIL_SIZE) {
    len = MAIL_SIZE;
  }

  int64_t ret = copy_byte_buffer(
      processor_current_user_id(),
      (uint8_t *)task->mailbox.buffer[task->mailbox.read_mails % MAX_MAIL_NUM],
      (uint8_t *)buf, len, TO_USER);
  if (ret < 0) {
    return -1;
  }
  // info("read mail from %lld, len = %lld\n", task->mailbox.read_mails, len);
  task->mailbox.read_mails++;
  return len;
}

int64_t sys_mailwrite(int64_t pid, char *buf, uint64_t len) {
  // info("[syscall] mailwrite\n");
  TaskControlBlock *task = task_manager_fetch_task_by_pid((uint64_t)pid);

  if (task->mailbox.write_mails - task->mailbox.read_mails == MAX_MAIL_NUM) {
    return -1;
  }
  if (len == 0) {
    return 0;
  }
  if (len > MAIL_SIZE) {
    len = MAIL_SIZE;
  }

  int64_t ret = copy_byte_buffer(
      processor_current_user_id(),
      (uint8_t *)task->mailbox.buffer[task->mailbox.write_mails % MAX_MAIL_NUM],
      (uint8_t *)buf, len, FROM_USER);
  if (ret < 0) {
    return -1;
  }
  // info("write mail to %lld, len = %lld\n", task->mailbox.write_mails, len);
  task->mailbox.write_mails++;
  return len;
}

int64_t sys_sbrk(uint64_t grow_size, uint64_t is_shrink){
  // info("[syscall] sbrk\n");
  TaskControlBlock *temp = processor_current_task();
  return grow_proc(temp, grow_size);
}

int64_t sys_brk(uint64_t brk_address) {
  // info("[syscall] brk, input arguement is %llx\n", brk_address);
    uint64_t addr_new = 0;
    if (brk_address == 0) {
        addr_new = sys_sbrk(0, 0);
    }
    else{
        TaskControlBlock *temp = processor_current_task();
        uint64_t former_addr = grow_proc(temp, 0);
        uint64_t grow_size = (brk_address - former_addr);
        addr_new = grow_proc(temp, grow_size);
    }

  // PhysAddr* pa;
  // nkapi_translate_va(processor_current_task()->pid, addr_new, pa);
  // info("pa is %lx\n", pa);    
  return addr_new;
}

int64_t sys_gettid() {
  // info("[syscall] get tid\n");
  return processor_current_task()->pid;
}

int64_t sys_set_tid_address(uint64_t tid){
  // info("[syscall] set tid address\n");
  processor_current_task()->address.clear_child_tid = tid;
  return sys_gettid();
}

int64_t umask(int64_t num){
  return 0;
}