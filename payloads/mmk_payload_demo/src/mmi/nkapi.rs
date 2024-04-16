use crate::address::*;
use crate::flags::*;
use core::arch::asm;
use super::config::NK_TRAMPOLINE;
use crate::debug_info;
use crate::debug_os;

///////////////////////////////////
/// 
/// the value below is NK call number.
/// 

pub const NKAPI_CONFIG: usize = 0;
pub const NKAPI_TRAP_HANDLE: usize = 1;
pub const NKAPI_PT_INIT: usize = 2;
pub const NKAPI_ALLOC: usize = 3;
pub const NKAPI_DEALLOC: usize = 4;
pub const NKAPI_ACTIVATE: usize = 5;
pub const NKAPI_COPY_TO: usize = 6;
pub const NKAPI_TRANSLATE: usize = 7;
pub const NKAPI_GET_PTE: usize = 8;
pub const NKAPI_FORK_PTE: usize = 9;
pub const NKAPI_SET_PERM: usize = 10;
pub const NKAPI_TIME: usize = 11;
pub const NKAPI_DEBUG: usize = 12;
///
///////////////////////////////////

///////////////////////////////////
/// 
/// the value below is NK_TRAP_HANDLE param.
/// 

pub const NKCFG_DELEGATE: usize = 1; // config the trap delegation target address.
pub const NKCFG_SIGNAL: usize = 2;
pub const NKCFG_ALLOCATOR_START: usize = 3;
pub const NKCFG_ALLOCATOR_END: usize = 4;
///
///////////////////////////////////
/// 

macro_rules! entry_gate {
    ($tar:expr,$retval0: expr, $retval1: expr) => {
        unsafe{
            asm!(
                "fence.i
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
    ($tar:expr,$t1:expr,$retval0: expr, $retval1: expr) => {
        unsafe{
            asm!(
                "fence.i 
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                in("a0") usize::from($t1),
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
    ($tar:expr,$t1:expr,$t2:expr,$retval0: expr, $retval1: expr) => {
        unsafe{
            asm!(
                "fence.i 
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                in("a0") usize::from($t1),
                in("a1") usize::from($t2),
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
    ($tar:expr,$t1:expr,$t2:expr,$t3:expr,$retval0: expr, $retval1: expr) => {
        unsafe{
            asm!(
                "fence.i 
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                in("a0") usize::from($t1),
                in("a1") usize::from($t2),
                in("a2") usize::from($t3),
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
    ($tar:expr,$t1:expr,$t2:expr,$t3:expr,$t4:expr,$retval0: expr, $retval1: expr) => {
        unsafe{

            asm!(
                "fence.i 
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                in("a0") usize::from($t1),
                in("a1") usize::from($t2),
                in("a2") usize::from($t3),
                in("a3") usize::from($t4),
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
    ($tar:expr,$t1:expr,$t2:expr,$t3:expr,$t4:expr,$t5:expr,$retval0: expr, $retval1: expr) => {
        unsafe{
            let para_vec = [usize::from($t1),usize::from($t2),usize::from($t3),usize::from($t4),usize::from($t5)];
            //println!("params_in: {:x} {:x} {:x} {:x} {:x}", 
            //para_vec[0], para_vec[1], para_vec[2], para_vec[3], para_vec[4]);
            asm!(
                "fence.i
                jalr x1, t3, 0",
                in("t3") NK_TRAMPOLINE,
                in("x17") $tar as usize*8,
                in("a0") para_vec[0],
                in("a1") para_vec[1],
                in("a2") para_vec[2],
                in("a3") para_vec[3],
                in("a4") para_vec[4],
                lateout("a0") $retval0,
                lateout("a1") $retval1,
            );
        }
    };
}



pub fn nkapi_time() -> usize{
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_TIME, retval0, retval1);
    if retval1 != 0 {
        panic!("Error handling!");
    }
    return retval0;
}

pub fn nkapi_translate(pt_handle: usize, vpn:VirtPageNum, write: bool) -> Option<PhysPageNum>{
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_TRANSLATE,pt_handle,vpn,write, retval0, retval1);
    if retval1 == 0{
        return Some(retval0.into());
    }
    return None;

}

pub fn nkapi_translate_va(pt_handle: usize, va:VirtAddr) -> Option<PhysAddr>{
    if let Some(ppn) = nkapi_translate(pt_handle, va.floor(), true) {
        return Some(PhysAddr((ppn.0<<12) + va.page_offset()));
    }
    None
}

pub fn nkapi_get_pte(pt_handle: usize, vpn: VirtPageNum) -> Option<PageTableEntry>{
    // if let Some(ppn) = nkapi_translate(pt_handle,va.clone().floor(),false) {
    //     let pa: PhysAddr = PhysAddr{0: ppn.0*crate::config::PAGE_SIZE + va.page_offset()};
    //     return Some(pa);
    // }
    // None
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_GET_PTE,pt_handle,vpn, retval0, retval1);
    if retval1 == 0{
        return Some(retval0.into());
    }
    return None;

}

pub fn nkapi_fork_pte(pt_handle: usize, pt_child: usize, vpn: VirtPageNum, cow: bool) -> Option<PhysPageNum> {
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_FORK_PTE, pt_handle, pt_child, vpn, cow, retval0, retval1);
    if retval1 == 0{
        return Some(retval0.into());
    }
    return None;

}

pub fn nkapi_alloc(pt_handle: usize, vpn: VirtPageNum, map_type: MapType, perm: MapPermission)-> PhysPageNum{
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_ALLOC, pt_handle, vpn, 1 as usize, usize::from(map_type), perm, 
    retval0, retval1);
    if retval1 != 0 {
        panic!("Error handling!");
    }
    return retval0.into();
}

pub fn nkapi_alloc_mul(pt_handle: usize, vpn_start: VirtPageNum, vpn_end: VirtPageNum, map_type: MapType, perm: MapPermission)-> PhysPageNum{
    let retval0: usize;
    let retval1: usize;
    let size = vpn_end.0 - vpn_start.0 + 1;
    entry_gate!(NKAPI_ALLOC, pt_handle, vpn_start, size, usize::from(map_type), perm, 
    retval0, retval1);
    
    if retval1 != 0 {
        panic!("Error handling: {}", retval1);
    }
    return retval0.into();
}

pub fn nkapi_pt_init(pt_handle: usize, regenerate: bool){
    let retval0: usize;
    let retval1: usize;

    entry_gate!(NKAPI_PT_INIT,pt_handle, regenerate, retval0, retval1);
}

pub fn nkapi_dealloc(pt_handle: usize, vpn: VirtPageNum){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_DEALLOC, pt_handle, vpn,retval0, retval1);
}

pub fn nkapi_activate(pt_handle: usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_ACTIVATE, pt_handle ,retval0, retval1);
}

pub fn nkapi_copyTo(pt_handle: usize, mut current_vpn: VirtPageNum, data: &[u8], offset:usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_COPY_TO,pt_handle, current_vpn, 
        data as *const [u8] as *const usize as usize, offset, retval0, retval1);
}

pub fn nkapi_set_delegate_handler(entry: usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_CONFIG, NKCFG_DELEGATE, entry,
        retval0, retval1);
}

pub fn nkapi_set_signal_handler(entry: usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_CONFIG, NKCFG_SIGNAL, entry,
        retval0, retval1);
}

pub fn nkapi_set_allocator_range(begin: usize, end: usize){
    let mut retval0: usize;
    let mut retval1: usize;
    debug_os!("setting allocator 1");
    entry_gate!(NKAPI_CONFIG, NKCFG_ALLOCATOR_START, begin,
        retval0, retval1);
    debug_os!("setting allocator 2");
    entry_gate!(NKAPI_CONFIG, NKCFG_ALLOCATOR_END, end,
        retval0, retval1);
}

pub fn nkapi_set_permission(pt_handle: usize, vpn:VirtPageNum, flags: usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_SET_PERM, pt_handle, vpn, flags,retval0, retval1);
}

pub fn nkapi_print_pt(pt_handle: usize, from: usize, to: usize){
    let retval0: usize;
    let retval1: usize;
    entry_gate!(NKAPI_DEBUG, pt_handle, from, to ,retval0, retval1);
}


