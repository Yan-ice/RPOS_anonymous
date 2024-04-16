mod heap_allocator;
#[macro_use]
mod frame_allocator;
mod page_table;
mod memory_set;
mod inner_addr;

use crate::{debug_info, mm::frame_allocator::{OUTER_FRAME_ALLOCATOR, outer_fork}};
use riscv::register::{
    mtvec::TrapMode,
    scause::{
        self,
        Trap,
        Exception,
        Interrupt,
    },
    stval,
    stvec, sstatus,
};


use core::arch::asm;

use alloc::{boxed::Box};
use lazy_static::*;
use spin::Mutex;

use alloc::vec::Vec;

// use crate::task::{current_task, Signals};

use page_table::*;
use inner_addr::*;
use super::trap::nk_trap_handler;
use crate::mmi::*;
use crate::*;
pub use frame_allocator::{
    StackFrameAllocator, 
    FrameAllocator,
    add_free, 
    outer_frame_add_ref, 
    enquire_ref,

    frame_alloc,
    frame_dealloc,
    outer_frame_alloc,
    outer_frame_dealloc
};

use crate::config::*;

pub use page_table::{
    PageTable
};


pub use memory_set::{MemorySet, KERNEL_SPACE};
pub use heap_allocator::HEAP_ALLOCATOR;
pub use frame_allocator::FRAME_ALLOCATOR;

extern "C" {
    fn sproxy();
    fn strampoline();
    fn snktrampoline();
    fn ssignaltrampoline();
}


pub fn init() {

    //debug_info!("trampoline pa: {:x}", strampoline as usize);
    //debug_info!("nktrampoline pa: {:x}", snktrampoline as usize);
    heap_allocator::init_heap();  // 堆空间分配器

    frame_allocator::init_frame_allocator();  // 物理页帧分配器
    // KERNEL_SPACE是个lazy启动的，启动时将pagetable等数据写好
    
    KERNEL_SPACE.lock().activate();  // 切换页表
    //KERNEL_SPACE.lock().print_pagetable();
    // unsafe{
    //     PROXYCONTEXT().nk_satp = KERNEL_SPACE.lock().token();
    // }
    debug_info!("kernel table init success.");
    init_vec();
}

pub fn init_othercore(){

    KERNEL_SPACE.lock().activate();
}

lazy_static! {
    pub static ref PAGE_TABLE_LIST: Mutex<Vec<PageTableRecord>> = Mutex::new(
        Vec::<PageTableRecord>::new()
    );
    pub static ref CURRENT_PT: Mutex<Box<usize>> = Mutex::new(Box::new(0));
}

macro_rules! pt_operate {
    ($handle:expr, $target:ident, $oper:block) => {
        let mut _find = false;
        for tar in PAGE_TABLE_LIST.lock().iter_mut(){
            if tar.id() == $handle {
                _find = true;
                let $target: &mut PageTableRecord = tar;
                //debug_info!("find page table with pt handle [{}]", $handle);
                $oper
            }
        }
        if !_find {
            debug_error!("Cannot find pagetable with handle [{:x}]!",$handle);
            return Err(2);
        }
    };
}



macro_rules! nkapi{
    //no return value
    ( $(#[$attr:meta])* fn $NAME:ident ( $($arg:ident : $tt:ty),+ ) $blk:block ) => {  

        #[warn(unreachable_code)]
        pub fn $NAME( $($arg:usize),+ ) -> (usize, usize){
            $( let $arg: $tt = <$tt>::from($arg); )+
            let __ret = ||-> Result<usize, usize> {$blk; return Ok(0);}();
            match __ret {
                Ok(retval) => {
                    return (retval, 0);
                },
                Err(errcode) => {
                    if errcode == 0 {return (0,1);}
                    return (errcode, errcode);
                }
            }
        }
    };

    //no params, has return value
    ( $(#[$attr:meta])* fn $NAME:ident () -> $ret:ty $blk:block ) => {
        pub fn $NAME() -> (usize, usize){

            let __ret = ||-> Result<$ret, usize> {$blk}();
            match __ret {
                Ok(retval) => {
                    return (retval.into(), 0);
                },
                Err(errcode) => {
                    if errcode == 0 {return ((0 as usize).into(),1);}
                    return (errcode,errcode);
                }
            }
            
        }
    };

    //has params, has return value
    ( $(#[$attr:meta])* fn $NAME:ident ( $($arg:ident : $tt:ty),+ ) -> $ret:ty $blk:block ) => {
        pub fn $NAME( $($arg:usize),+ ) -> (usize, usize){
            $( let $arg: $tt = <$tt>::from($arg); )+

            let __ret = ||-> Result<$ret, usize> {$blk}();
            match __ret {
                Ok(retval) => {
                    return (retval.into(), 0);
                },
                Err(errcode) => {
                    if errcode == 0 {return ((0 as usize).into(),1);}
                    return (errcode,errcode);
                }
            }
            
        }
    };
}


pub fn init_vec(){
    let proxy = PROXYCONTEXT();

    proxy.nkapi_enable = 1;
    proxy.nkapi_vec[NKAPI_TRAP_HANDLE] = nk_trap_handler as usize;
    proxy.nkapi_vec[NKAPI_CONFIG] = nkapi_config as usize;
    proxy.nkapi_vec[NKAPI_PT_INIT] = nkapi_pt_init as usize;
    proxy.nkapi_vec[NKAPI_PT_DESTROY] = nkapi_pt_destroy as usize;
    proxy.nkapi_vec[NKAPI_ALLOC] = nkapi_alloc as usize;
    proxy.nkapi_vec[NKAPI_DEALLOC] = nkapi_dealloc as usize;
    proxy.nkapi_vec[NKAPI_ACTIVATE] = nkapi_activate as usize;
    proxy.nkapi_vec[NKAPI_WRITE] = nkapi_write as usize;
    proxy.nkapi_vec[NKAPI_TRANSLATE] = nkapi_translate as usize;
    proxy.nkapi_vec[NKAPI_GET_PTE] = nkapi_get_pte as usize;
    proxy.nkapi_vec[NKAPI_FORK_PTE] = nkapi_fork_pte as usize;
    proxy.nkapi_vec[NKAPI_SET_PERM] = nkapi_set_permission as usize;
    proxy.nkapi_vec[NKAPI_TIME] = nkapi_time as usize;
    proxy.nkapi_vec[NKAPI_DEBUG] = nkapi_print_pt as usize;

    proxy.nkapi_vec[NKAPI_CURRENT_PT] = nkapi_current_pt as usize;

    proxy.delegate = 0;

}

fn check_valid(owner: u8, ppn: InnerPhysPageNum, perm: MapPermission) -> bool{
    //NK SPACE can never be access.
    if ppn.0 >= NKSPACE_START<<12 && ppn.0 < NKSPACE_END<<12{
        debug_error!("No permission to access nk space {:x}", ppn.0);
        return false;

    }
    if enquire_ref(ppn.into()).len() == 0 {
        return true;
    }
    //only owner can access with write perm.
    if perm.contains(MapPermission::W){
        if enquire_ref(ppn.into())[0] != owner {
            debug_error!("Only owner can have write permission {:x}", ppn.0);
            return false;
        }
    }

    //only user can operate it.
    for usr in enquire_ref(ppn.into()){
        if usr == owner {
            return true;
        }
    }
    debug_error!("Only page user can operate this {:x}", ppn.0);
    return false;
}

pub fn pt_current() -> usize {
    CURRENT_PT.lock().as_ref().clone()
}


//the function below would expose to outer kernel

nkapi!{
    fn nkapi_time() -> usize {
        let mut time:usize = 0;
        unsafe{
            core::arch::asm!(
                "rdtime a0",
                inout("a0") time
            );
        }
        Ok(time)
    }
}

nkapi!{
    fn nkapi_config(t: usize, val: usize){
        let proxy = PROXYCONTEXT();
        match t{
            NKCFG_S_DELEGATE =>{
                proxy.ktrap_delegate = val;
            }
            NKCFG_U_DELEGATE =>{
                proxy.delegate = val;
            }
            NKCFG_SIGNAL => {
                proxy.signal_handler = val;
            }
            NKCFG_ALLOCATOR_START => {
                proxy.allocator_start = val;
            }
            NKCFG_ALLOCATOR_END => {
                proxy.allocator_end = val;
            }
            _ => {
                debug_info!("Unknown config ID: {}", t);
                return Err(1);
            }
        }
    }
    
}

nkapi!{
    fn nkapi_print_pt(pt_handle: usize, from: usize, to: usize){
        pt_operate! (pt_handle, target_pt, {
            // debug_info!("=========[print pt {}]==========", pt_handle);
            target_pt.trace_address((from<<12).into());
            // debug_info!("printing {} ~ {}", from, to);
            target_pt.print_pagetable(from<<12, to<<12);
        });
    }
}

nkapi!{
    fn nkapi_fork_pte(pt_handle: usize, pt_child: usize, vpn: VirtPageNum, cow: usize) -> PhysPageNum{
        let cow = cow!=0;
        let mut flag: PTEFlags = PTEFlags::V;
        pt_operate! (pt_handle, target_pt, {
            let src_pte = target_pt.find_pte(vpn);
            if src_pte.is_none() {
                debug_error!("fork_pte: source pte is invalid!");
                return Err(1);
            }
            flag = src_pte.unwrap().flags();
        });
        pt_operate! (pt_child, target_pt, {
            let dst_pte = target_pt.find_pte(vpn);
            if dst_pte.is_some() {
                debug_error!("fork_pte: target pte already exists: {:?}", vpn);
                return Err(1);
            }
        }); 

        if cow{
            flag = flag & !PTEFlags::W | PTEFlags::O;

            let mut src_pte = None;
            pt_operate! (pt_handle, target_pt, {
                target_pt.set_flags(vpn, flag);
                src_pte = Some(target_pt.find_pte(vpn).unwrap().clone());
            });
            //debug_info!("forking pte: {:?} -> {:?}",vpn, src_pte.unwrap().ppn());
            pt_operate! (pt_child, target_pt, {
                target_pt.map(vpn, src_pte.unwrap().ppn(), src_pte.unwrap().flags());
            });
            outer_fork(src_pte.unwrap().ppn(), pt_handle as u8, pt_child as u8);
            return Ok(src_pte.unwrap().ppn().into());
            
        }else{
            let (dst_ppn, _) = nkapi_alloc(pt_child, vpn.0, 1, 
                MapType::Framed.into(), (flag.bits() as usize).into());
            let (src_ppn, _) = nkapi_translate(pt_handle, vpn.into(), 0);
            nkapi_write(pt_child, vpn.into(), PhysPageNum(src_ppn).get_bytes_array().as_ptr() as *const u8 as usize, 0);
            return Ok(PhysPageNum(dst_ppn));
        }
    }  
}

nkapi!{
    fn nkapi_pt_init(pt_handle: usize, re_generate: usize) -> usize {
        
        let __ret = |status_code: &mut usize|-> usize { {return *status_code;} }(&mut 0);

        let mut re_activate = pt_current() == pt_handle;

        if re_generate != 0 && pt_handle != 0{
            debug_debug!("Pagetable [{}] already exists, destroying.",pt_handle);
            let mut ptlist = PAGE_TABLE_LIST.lock();
            for tar in 0..ptlist.len(){
                if ptlist[tar].id() == pt_handle {
                    ptlist.remove(tar).destroy();
                }
            }
            unsafe{
                asm!("sfence.vma zero, t0",in("t0") pt_handle);
            }
            
        }

        for i in PAGE_TABLE_LIST.lock().iter(){
            if i.id() == pt_handle {
                debug_warn!("Pagetable [{}] already exists.",pt_handle);
                //pagetable with this handle already exist
                return Err(1);
            }
        }
        
        //Yan_ice: here we create a new pagetable,
        let mut pt = PageTableRecord::new(pt_handle);
        debug_debug!("Creating user PageTable [{}] with token {:x}.",pt_handle, pt.token());

            pt.map(VirtAddr::from(SIGNAL_TRAMPOLINE).into(),
                InnerPhysAddr::from(ssignaltrampoline as usize).into(),
                PTEFlags::R | PTEFlags::X | PTEFlags::U,
            );
        
            // mapping trampoline
            pt.map(VirtAddr::from(TRAMPOLINE).into(), 
                InnerPhysAddr::from(strampoline as usize).into(),
                PTEFlags::R | PTEFlags::X | PTEFlags::W);
            pt.map(VirtAddr::from(NK_TRAMPOLINE).into(), 
                InnerPhysAddr::from(snktrampoline as usize).into(),
                PTEFlags::R | PTEFlags::X);
            pt.map(VirtAddr::from(PROXY_CONTEXT).into(),
                InnerPhysAddr::from(sproxy as usize).into(),
                PTEFlags::R | PTEFlags::W,
            );

        if pt_handle != 0{
            pt_operate!(0,pt_kernel,{
                pt.map_kernel_shared(pt_kernel);
            });
        }

        PAGE_TABLE_LIST.lock().push(pt);
    
        if re_activate {
            nkapi_activate(pt_handle);
        }

        return Ok(pt_handle);
    }
}


nkapi!{
    fn nkapi_set_permission(pt_handle: usize, vpn: VirtPageNum, flags: usize){
        // find target pagetable
        pt_operate! (pt_handle, target_pt, {
            if target_pt.translate(vpn).is_none() {
                debug_warn!("PTE with {:?} not valid while setting permission.", vpn);
            }
            target_pt.set_pte_flags(vpn, flags);
        });
    }
}

extern "C"{
    fn _ktrap();
}

nkapi!{
    fn nkapi_pt_destroy(pt_handle: usize) {
        if pt_handle == 0 {
            debug_error!("Cannot destroy pt [0]");
            return Err(1);
        }
        if pt_current() == pt_handle {
            debug_info!("Destroying current_pt [{}], auto activate [0].",pt_handle);
            nkapi_activate(0);
        }
        let mut ptlist = PAGE_TABLE_LIST.lock();
        for tar in 0..ptlist.len(){
            if ptlist[tar].id() == pt_handle {
                ptlist.remove(tar).destroy();
            }
        }
        unsafe{
            asm!("sfence.vma t0, zero",in("t0") pt_handle);
        }
        
    }
}

nkapi!{
    fn nkapi_alloc(pt_handle: usize, root_vpn: VirtPageNum, size: usize, 
        map_type_u: usize, perm: MapPermission) -> PhysPageNum{

        let map_type = MapType::from(map_type_u);
        let mut pte_flags = PTEFlags::from_bits(perm.bits()).unwrap();
        
        //Yan_ice: pte of pt_handle 0 is shared.
        if pt_handle == 0 {
            pte_flags = pte_flags | PTEFlags::G;
        }
        
        pt_operate! (pt_handle, target_pt, {
            
            let mut first_ppn: PhysPageNum = PhysPageNum(0);
            for i in 0..size {
                let vpn = VirtPageNum{0: root_vpn.0 + i};
                let target_ppn: InnerPhysPageNum;
                match map_type{
                    MapType::Framed => {
                        if let Some(ppn) = outer_frame_alloc(pt_handle as u8){
                            //debug_info!("outer allocating pt: {:x} ppn: {:?}", pt_handle, ppn);
                            target_ppn = ppn;
                        }else{
                            panic!("No more memory in Outer Kernel!");
                        }
                    }
                    MapType::Raw => {
                        if let Some(ppn) = outer_frame_alloc(pt_handle as u8){
                            target_ppn = ppn;
                            // debug_info!("outer allocating: {:?}", target_ppn);
                            // debug_info!("vpn is 0x{:x}", vpn.0);
                        }else{
                            panic!("No more memory in Outer Kernel!");
                        }

                        // debug_info!("in pt {} map ppn 0x{:x} to vpn 0x{:x}", pt_handle, target_ppn.0, VirtPageNum(target_ppn.0).0);
                        target_pt.map(VirtPageNum(target_ppn.0), target_ppn, pte_flags);

                        // debug_info!("in pt {} map ppn 0x{:x} to vpn 0x{:x}", pt_handle, target_ppn.0, vpn.0);
                        // target_pt.map(vpn, target_ppn, pte_flags);
                        return Ok(target_ppn.into());
                    }
                    MapType::Identical => {
                        target_ppn = PhysPageNum::from(vpn.0).into();
                    }
                    MapType::Specified(ppn) => {
                        target_ppn = ppn.into();
                    }
                }
                
                if i == 0{
                    first_ppn = target_ppn.into();
                }

                // debug_info!("in pt {} map ppn 0x{:x} to vpn 0x{:x}", pt_handle, target_ppn.0, vpn.0);

                if !check_valid(pt_handle as u8, target_ppn, perm) {
                    debug_error!("Invalid allocation!");
                    return Err(1);
                }

                //clean the page frame
                if map_type == MapType::Framed{
                    let bytes_array = target_ppn.get_bytes_array();
                    for i in bytes_array {
                        *i = 0;
                    }
                }

                // modify pagetable entry
                if pt_handle != 0{
                    // debug_info!("in pt {} map ppn 0x{:x} to vpn 0x{:x}", pt_handle, target_ppn.0, vpn.0);
                }

                target_pt.map(vpn, target_ppn, pte_flags);
            }
            return Ok(first_ppn);

        });
        debug_info!("nkapi_alloc: cannot find pagetable!");
        Err(1)
    }
}

nkapi!{
    fn nkapi_dealloc(pt_handle: usize, vpn: VirtPageNum){
        pt_operate! (pt_handle, target_pt, {
            if let Some(pte) = target_pt.translate(vpn){
    
                if !check_valid(pt_handle as u8, pte.ppn(), MapPermission::R) {
                    debug_info!("deallocate failed: invalid");
                    return Err(1);
                }
                target_pt.unmap(vpn);
                outer_frame_dealloc(pte.ppn(),pt_handle as u8);

            }else{
                debug_info!("deallocate failed: not found");
                return Err(1);
            }
            return Ok(0);
        });
    }
}


nkapi!{
    // while translating COW with write==True, it would start alloc and copy.

    fn nkapi_current_pt() -> usize {
        return Ok(pt_current());
    }
}

nkapi!{
    // while translating COW with write==True, it would start alloc and copy.

    fn nkapi_translate(pt_handle: usize, vpn: VirtPageNum, write: usize) -> PhysPageNum {
        let write = write!=0;
        pt_operate! (pt_handle, target_pt, {
            if let Some(pte) = target_pt.translate(vpn){
                if write && pte.is_valid() && pte.is_cow(){
                    let former_ppn = pte.ppn();
                    let usrs = enquire_ref(former_ppn);
                    if usrs.len() == 1 && usrs[0] == pt_handle as u8{
                        // change the flags of the src_pte
                        target_pt.set_flags(
                            vpn, pte.flags() & !PTEFlags::O | PTEFlags::W
                        );
                    }else{
                        let ppn = outer_frame_alloc(pt_handle as u8).unwrap();
                        target_pt.remap_cow(vpn, ppn, former_ppn);
                    }
                }
                return Ok(pte.ppn().into());
            }
            debug_info!("WARN: cannot translate {:?}", vpn);
        });
        return Err(1);
    }
}

nkapi!{
    fn nkapi_translate_va(pt_handle: usize, va: VirtAddr) -> PhysAddr{
        pt_operate! (pt_handle, target_pt, {
            if let Some(pa) = target_pt.translate_va(va){
                return Ok(pa.into());
            }
        });
        return Err(1);
    }
}

nkapi!{
    fn nkapi_get_pte(pt_handle: usize, vpn: VirtPageNum) -> PageTableEntry{
    
        pt_operate! (pt_handle, target_pt, {
            if let Some(pte) = target_pt.find_pte(vpn) {
                return Ok(pte.clone());
            }
        });
        Err(1)
    }
    
}


nkapi!{
    fn nkapi_write(pt_handle: usize, vpn: VirtPageNum, data_ptr: usize, offset:usize){

        unsafe{
            let (former_pa,_) = nkapi_translate_va(pt_current(), data_ptr.into());

            pt_operate! (pt_handle, target_pt, {

                //debug_info!("nk_copy: copying data from {:x}", former_pa.0);
                let data = &*(former_pa as *const usize as *mut [u8; PAGE_SIZE]);
                // debug_info!("write pt {} vpn is 0x{:x}", pt_handle, vpn.0);
                let pte = &mut target_pt.translate(vpn).unwrap();
                //debug_info!("nkapi_write: writing datas to [{}] {:?}({:?})",pt_handle, vpn, pte.ppn());
                if pte.is_valid() && pte.is_cow(){
                    //debug_info!("copy on write");

                    let former_ppn = pte.ppn();
                    let usrs = enquire_ref(former_ppn);
                    if usrs.len() == 1 && usrs[0] == pt_handle as u8{
                        // change the flags of the src_pte
                        target_pt.set_flags(
                            vpn, pte.flags() & !PTEFlags::O | PTEFlags::W
                        );
                    }else{
                        let ppn = outer_frame_alloc(pt_handle as u8).unwrap();
                        target_pt.remap_cow(vpn, ppn, former_ppn);
                    }
                }

                let src = &data[0..(PAGE_SIZE - offset)];
                let dst = &mut pte.ppn().get_bytes_array()[offset..PAGE_SIZE];
                dst.copy_from_slice(src);

            });

        }
    }
}

nkapi!{
    fn nkapi_activate(pt_handle: usize){
        pt_operate! (pt_handle, target_pt, {
            let satp = target_pt.token();
            // nk_entry_gate();
            // unsafe {
            //     satp::write(satp);
            //     llvm_asm!("sfence.vma" :::: "volatile");
            // }
    
            // debug_info!("outer kernel's table switch.");
            debug_debug!("nkapi: pagetable [{}] activated.", pt_handle);
            *CURRENT_PT.lock().as_mut() = pt_handle;
            PROXYCONTEXT().outer_satp = satp;
            
        });
    }
}
//use crate::task::__switch;


// this function is temporaily used. it is vulunerable!
pub fn nkapi_vun_getpt(pt_handle: usize) -> Result<PageTable, usize>{
    pt_operate! (pt_handle, target_pt, {
        return Ok(target_pt.into());
    });
    panic!("not found!");
}
