#ifndef _MM_H_
#define _MM_H_

#include "config.h"
#include "external.h"
#include "mmk.h"

#define MMAP_MAX_SIZE (1 << 30) // 1 GB

#define page_floor(x) ((x) / PAGE_SIZE)
#define page_ceil(x) (((x)-1ul + PAGE_SIZE) / PAGE_SIZE)
#define page_offset(x) ((x) & (PAGE_SIZE - 1))
#define page_aligned(x) (page_offset(x) == 0)
#define addr2pn(x) page_floor(x)
#define pn2addr(x) ((x) << PAGE_SIZE_BITS)

#define pte_new(ppn, flags) ((ppn) << 10 | (PageTableEntry)flags)
#define pte_empty() 0
#define pte_ppn(pte) (((pte) >> 10) & ((1L << 44) - 1))
#define pte_flags(pte) ((PTEFlags)((pte)&0xff))
#define pte_is_valid(pte) (((pte)&PTE_V) != pte_empty())
#define pte_readable(pte) (((pte)&PTE_R) != pte_empty())
#define pte_writable(pte) (((pte)&PTE_W) != pte_empty())
#define pte_executable(pte) (((pte)&PTE_X) != pte_empty())

#define FROM_USER 0
#define TO_USER 1

typedef struct {
  PhysPageNum root_ppn;
  struct vector frames;
} PageTable;

typedef struct {
  VirtPageNum l;
  VirtPageNum r;
} VPNRange;

typedef struct {
  VPNRange vpn_range;
  MapType map_type;
  MapPermission map_perm;
} MapArea;

//Yan_ice: need to be modified: PageTable -> PtHandle
typedef struct {
  uint64_t page_table;
  struct vector areas;
} MemorySet;

/*  Data structure in rcc memory management
 *
 *  MemorySet
 *    ├── PageTable
 *    │     ├── Root PPN
 *    │     └── Vector of PPN frames
 *    └── Vector of MapArea
 *                    ├── VPN Range
 *                    │     ├── VPN l
 *                    │     └── VPN r
 *                    ├── MapType (Identical / Framed)
 *                    └── MapPermission (R / W / X / U)
 */

// mm.c
void mm_init();
void mm_free();

// address.c
void vpn_indexes(VirtPageNum vpn, uint64_t *idx);
PageTableEntry *ppn_get_pte_array(PhysPageNum ppn);
uint8_t *ppn_get_bytes_array(PhysPageNum ppn);

// heap_allocator.c
void heap_allocator_init();

// frame_allocator.c
// void frame_allocator_init();
// void frame_allocator_free();
// PhysPageNum frame_alloc();
// void frame_dealloc(PhysPageNum ppn);
// uint64_t frame_remaining_pages();
// void frame_allocator_print();


// page_table.c
// void page_table_new(PtHandle pt);
// void page_table_free(PtHandle pt);
// void page_table_from_token(PtHandle pt, uint64_t satp);
// PageTableEntry *page_table_find_pte_create(PtHandle pt, VirtPageNum vpn);
// PageTableEntry *page_table_find_pte(PtHandle pt, VirtPageNum vpn);
// void page_table_map(PtHandle pt, VirtPageNum vpn, PhysPageNum ppn,
//                     PTEFlags flags);
// void page_table_unmap(PtHandle pt, VirtPageNum vpn);
// PageTableEntry *page_table_translate(PtHandle pt, VirtPageNum vpn);
// uint64_t page_table_token(PtHandle pt);
int64_t copy_byte_buffer(uint64_t token, uint8_t *kernel, uint8_t *user,
                          uint64_t len, uint64_t direction);

// memory_set.c
uint64_t memory_set_token(MemorySet *memory_set);
void memory_set_free(MemorySet *memory_set);
void memory_set_from_elf(MemorySet *memory_set, uint8_t *elf_data,
                         size_t elf_size, uint64_t *user_sp, uint64_t *user_heap,
                         uint64_t *entry_point, uint8_t clear);
void memory_set_from_existed_user(MemorySet *memory_set, MemorySet *user_space);
void memory_set_kernel_init();
PhysPageNum memory_set_translate(MemorySet *memory_set, VirtPageNum vpn);
void memory_set_recycle_data_pages(MemorySet *memory_set);
void kernel_space_insert_framed_area(VirtAddr start_va, VirtAddr end_va,
                                     MapPermission permission);
void kernel_space_remove_area_with_start_vpn(VirtPageNum start_vpn);
uint64_t kernel_space_id();
int64_t memory_set_mmap(MemorySet *memory_set, uint64_t start, uint64_t len,
                        uint64_t prot);
int64_t memory_set_munmap(MemorySet *memory_set, uint64_t start, uint64_t len);

#endif // _MM_H_
