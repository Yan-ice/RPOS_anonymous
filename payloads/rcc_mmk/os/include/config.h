#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>


#define USER_STACK_SIZE (4096 * 4)
#define USER_HEAP_SIZE (4096 * 16)
#define KERNEL_STACK_SIZE (4096 * 2)
#define KERNEL_HEAP_SIZE 0x110000
#define MEMORY_END 0x84000000
#define PAGE_SIZE 0x1000ul
#define PAGE_SIZE_BITS 0xc
#define PAGE_SHIFT 12

#define TRAMPOLINE (UINT64_MAX - 2 * PAGE_SIZE + 1)
#define TRAP_CONTEXT (0xffffe000ul)

#define APP_BASE_ADDRESS 0x80400000
#define APP_SIZE_LIMIT 0x20000

#define MAX_APP_SIZE (1024 * 1024)

// kernel stack in kernel space
#define kernel_stack_position_top(x)                                           \
   (TRAMPOLINE - (x) * (KERNEL_STACK_SIZE + PAGE_SIZE))
#define kernel_stack_position_bottom(x)                                        \
  (kernel_stack_position_top(x) - KERNEL_STACK_SIZE)

#define VIRTIO0_IRQ 1

#define CLOCK_FREQ 10000000

#define VIRTIO0 0x10001000L
#define PLIC 0x0c000000L

#define MMIO_NUM 1
const static uint64_t MMIO[MMIO_NUM][2] = {
  {VIRTIO0, 0x10000},   /* virtIO 0 */
  // {PLIC, 0x3000},   /* PLIC */
  // {PLIC + 0x200000, 0x5000},   /* PLIC */
};


// #ifdef BOARD_NEZHA
// #define VIRTIO0 0x10001000L
// #define PLIC 0x10000000L
// #define MMIO_NUM 4
// const static uint64_t MMIO[MMIO_NUM][2] = {
//   {PLIC, 0x3000},   /* PLIC     */
//   {PLIC+200000, 0x1000},   /* PLIC     */
//   {0x03002000, 0x1000},   /* DMAC     */
//   {0x04025000, 0x1000},   /* SPI 0    */
// };
// #endif 


// qemu puts platform-level interrupt controller (PLIC) here

#define PLIC_PRIORITY (PLIC + 0x0)
#define PLIC_PENDING (PLIC + 0x1000)
#define PLIC_MENABLE(hart) (PLIC + 0x2000 + (hart)*0x100)
#define PLIC_SENABLE(hart) (PLIC + 0x2080 + (hart)*0x100)
#define PLIC_MPRIORITY(hart) (PLIC + 0x200000 + (hart)*0x2000)
#define PLIC_SPRIORITY(hart) (PLIC + 0x201000 + (hart)*0x2000)
#define PLIC_MCLAIM(hart) (PLIC + 0x200004 + (hart)*0x2000)
#define PLIC_SCLAIM(hart) (PLIC + 0x201004 + (hart)*0x2000)

#endif // _CONFIG_H_
