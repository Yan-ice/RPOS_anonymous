#![no_std]
#![no_main]
// #![feature(global_asm)]
#![feature(panic_info_message)]
#![feature(alloc_error_handler)]


extern crate alloc;
extern crate bitflags;
extern crate mmi;

#[macro_use]
mod lang_items;

mod config;
mod mm;   
mod trap;
mod util;
mod context;
//mod pkcs; Yan_ice: temporarily remove pkcs.
mod logging;

use core::arch::global_asm;

global_asm!(include_str!("entry.asm"));
global_asm!(include_str!("nk_gate.S"));

use crate::mmi::*; 
use crate::config::*;
use crate::util::*;
use crate::context::*;

use mm::{
            KERNEL_SPACE as KERNEL_SPACE, 
            nkapi_vun_getpt as nkapi_vun_getpt
};
use core::panic::PanicInfo;

pub fn id() -> usize {
    let cpu_id;
    unsafe {
        core::arch::asm!("mv {0}, tp", 
                        out(reg) cpu_id);
    }
    cpu_id
}

fn clear_bss() {
    extern "C" {
        fn sbss();
        fn ebss();
    }
    (sbss as usize..ebss as usize).for_each(|a| {
        unsafe { (a as *mut u8).write_volatile(0) }
    });
}
extern "C" {
    pub fn nk_entry();
    pub fn nk_exit(hart: usize);
}

fn space(){

    extern "C" {
        fn stext();
        fn etext();
        fn srodata();
        fn erodata();
        fn sbss_with_stack();
        fn ebss();
        fn sdata();
        fn edata();
        fn ekernel();
    }
    debug_info!(".text [{:#x}, {:#x})", stext as usize, etext as usize);
    debug_info!(".rodata [{:#x}, {:#x})", srodata as usize, erodata as usize);
    debug_info!(".data [{:#x}, {:#x})", sdata as usize, edata as usize);
    debug_info!(".bss [{:#x}, {:#x})", sbss_with_stack as usize, ebss as usize);
    debug_info!("nkframe [{:#x}, {:#x})", ekernel as usize, crate::config::NKSPACE_END);
}

fn default_delegate(){
    panic!("trap occur in MMK!");
}
#[no_mangle]
pub fn nk_main(){
    let core = id();
    if core != 0 {
        loop{}
    }
    // unsafe{
    //     let mut temp = 0;
    //     core::arch::asm!("csrr {0}, sstatus", out(reg) temp);
    //     temp = temp | (1 << 18);
    //     core::arch::asm!("csrw sstatus, {0}", in(reg) temp);
    // }
    
    clear_bss();

    mm::init();
    space();
    debug_info!("mm init success.");
    
    trap::init();
    debug_info!("trap init success.");

    nkapi_pt_init(0, false);

    nkapi_alloc_mul(0, 
        VirtAddr(config::OKSPACE_START).into(), 
        VirtAddr(config::OKSPACE_START+0x300000).into(), 
        MapType::Identical, 
        MapPermission::R | MapPermission::W | MapPermission::X);

    //OUTER_KERNEL_SPACE().lock();
    debug_info!("payload pagetable init success.");

    let mut proxy = PROXYCONTEXT();
    proxy.nk_satp = KERNEL_SPACE.lock().token();
    proxy.outer_register[1] = config::OKSPACE_START as usize; //let ra be outer kernel init
    proxy.ktrap_delegate = default_delegate as usize;
    
    logging::init();
    
    debug_info!("Ready jump to payload.");
    
    unsafe{
        //nk_exit(0);
        core::arch::asm!("jr x31", 
        //in("x31") nk_exit as usize,
        in("x31") config::NK_TRAMPOLINE + nk_exit as usize - nk_entry as usize,
        in("x10") 0 );
        panic!("not reachable");
    }

}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    if let Some(location) = info.location() {
        debug_error!("[kernel] Panicked at {}: {} {}", location.file(), location.line(), info.message().unwrap());
    } else {
        debug_error!("[kernel] Panicked: {}", info.message().unwrap());
    }
    shutdown()
}
