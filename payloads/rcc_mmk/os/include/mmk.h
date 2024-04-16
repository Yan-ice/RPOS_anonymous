
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef _MMK_H_
#define _MMK_H_

///////////////////////////////////
/// type declaration

typedef uint64_t PtHandle;

typedef uint64_t PhysAddr;
typedef uint64_t VirtAddr;
typedef uint64_t PhysPageNum;
typedef uint64_t VirtPageNum;
typedef uint64_t PageTableEntry;

///////////////////////////////////
/// bitflags

typedef uint16_t PTEFlags;
typedef uint16_t MapPermission;

#define PTE_V (1L << 0)
#define PTE_R (1L << 1)
#define PTE_W (1L << 2)
#define PTE_X (1L << 3)
#define PTE_U (1L << 4)
#define PTE_G (1L << 5)
#define PTE_A (1L << 6)
#define PTE_D (1L << 7)

#define MAP_PERM_R PTE_R
#define MAP_PERM_W PTE_W
#define MAP_PERM_X PTE_X
#define MAP_PERM_U PTE_U

///////////////////////////////////
/// map type

typedef uint64_t MapType;

#define MAP_IDENTICAL 0xfffffffffffffffe
#define MAP_FRAMED 0xfffffffffffffffd
#define MAP_RAW 0xfffffffffffffffc
#define MAP_SPECIFIED(x) ((uint64_t)x)


///////////////////////////////////
/// 
/// the value below is NK call number.
/// 
#define NKAPI_TRAP_HANDLE 0
#define NKAPI_CONFIG 1

#define NKAPI_PT_INIT 2
#define NKAPI_PT_DESTROY 3
#define NKAPI_ALLOC 4
#define NKAPI_DEALLOC 5
#define NKAPI_ACTIVATE 6
#define NKAPI_TRANSLATE 7
#define NKAPI_SET_PERM 8
#define NKAPI_GET_PTE 9

#define NKAPI_WRITE 10
#define NKAPI_FORK_PTE 11
#define NKAPI_TIME 12
#define NKAPI_DEBUG 13
#define NKAPI_CURRENT_PT 14
///
///////////////////////////////////


///////////////////////////////////
/// 
/// the value below is NK_TRAP_HANDLE param.
/// 
#define NKCFG_S_DELEGATE 0
#define NKCFG_U_DELEGATE 1
#define NKCFG_SIGNAL 2
#define NKCFG_ALLOCATOR_START 3
#define NKCFG_ALLOCATOR_END 4
///
///////////////////////////////////

int nkapi_time(unsigned long *time);
int nkapi_current_pt(unsigned long *current_pt);

int nkapi_translate(unsigned long pt_handle, VirtPageNum vpn, unsigned char write, PhysPageNum *ppn);
int nkapi_translate_va(unsigned long pt_handle, VirtAddr va, PhysAddr *pa);
int nkapi_get_pte(unsigned long pt_handle, VirtPageNum vpn, unsigned long *pte);
int nkapi_fork_pte(unsigned long pt_handle, unsigned long pt_child, VirtPageNum vpn, unsigned char cow, PhysPageNum *ppn);
int nkapi_alloc(unsigned long pt_handle, VirtPageNum vpn, 
	MapType map_type, MapPermission map_perm, PhysPageNum *ppn);
int nkapi_dealloc(unsigned long pt_handle, VirtPageNum vpn);
int nkapi_pt_init(unsigned long pt_handle, unsigned char regenerate);
int nkapi_pt_destroy(unsigned long pt_handle);
int nkapi_activate(unsigned long pt_handle);
int nkapi_write(unsigned long pt_handle, VirtPageNum vpn,  uint8_t *data, unsigned long len, unsigned long offset);
int nkapi_set_permission(unsigned long pt_handle, VirtPageNum vpn, MapPermission map_perm);
int nkapi_print_pt(unsigned long pt_handle, unsigned long from, unsigned long to);

int nkapi_config_kernel_delegate_handler(unsigned long entry);
int nkapi_config_user_delegate_handler(unsigned long entry);
int nkapi_config_signal_handler(unsigned long entry);
int nkapi_config_allocator_range(unsigned long begin, unsigned long end);


#endif