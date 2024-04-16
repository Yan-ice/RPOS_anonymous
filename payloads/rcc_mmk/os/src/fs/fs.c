#include "fs.h"
#include "log.h"
#include "sbi.h"
#include "string.h"
#include "task.h"

int64_t stdin_read(char *buf, uint64_t len) {
  assert(len == 1,"stdin len not 1");

  // busy loop
  uint64_t c;
  while (1) {
    c = console_getchar();
    if (c == 0) {
      task_suspend_current_and_run_next();
      continue;

      //Yan_ice: to let shell work, temporarily suspend when read -1 (EOF).
      //This may cause error when read other things!
      //TODO: please let shell ignore input when read -1.
    
    //else{
    } else if (c != -1){
      break;
    }
  }

  uint8_t ch = (uint8_t)c;
  copy_byte_buffer(processor_current_user_id(), &ch, (uint8_t *)buf, 1,
                   TO_USER);
  return 1;
}

int64_t stdout_write(char *buf, uint64_t len) {
  static uint8_t stdout_write_buf[FS_BUFFER_SIZE];

  len = MIN(len, FS_BUFFER_SIZE);
  copy_byte_buffer(processor_current_user_id(), stdout_write_buf,
                   (uint8_t *)buf, len, FROM_USER);
  for (uint64_t i = 0; i < len; i++) {
    console_putchar((char)stdout_write_buf[i]);
  }
  return (int64_t)len;
}
