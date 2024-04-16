#include "mm.h"
#include "log.h"

void mm_init() {
  
  // mm init
  heap_allocator_init();
  memory_set_kernel_init();
  //frame_allocator_init();
}

int64_t copy_byte_buffer(uint64_t id, uint8_t *kernel, uint8_t *user,
                          uint64_t len, uint64_t direction)
{

  PhysAddr user_pa = user;
  nkapi_translate_va(id,(VirtAddr)user,&user_pa);

  if(direction == TO_USER) {
    memcpy((uint8_t*)user_pa, kernel, len);
    // nkapi_write(id, dst, kernel, len, offset);
  }else{
    // nkapi_print_pt(id, 0, 0x3);
    // info("[copy byte buffer to ker] user pa is %lx %lx\n", user, user_pa);
     memcpy(kernel, (uint8_t*)user_pa, len);
    //nkapi_write(0, dst, user, len, offset);
  }
  return len;
}


void mm_free() {
  // mm free
  //frame_allocator_free();
}
