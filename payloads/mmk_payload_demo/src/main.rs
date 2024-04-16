#![no_std]
#![no_main]

#![feature(panic_info_message)]
#![feature(alloc_error_handler)]

extern crate alloc;
extern crate bitflags;

#[macro_use]
mod mmi;

#[macro_use]
mod util;
mod tests;
mod timer;
mod heap_allocator;
mod config;
#[macro_use]
mod lang_items;

use lazy_static::lazy_static;
use riscv::register::satp;

use mmi::*;
use util::*;
use config::*;
use core::arch::global_asm;

use crate::heap_allocator::init_heap;
use crate::tests::*;
use spin::*;
use alloc::sync::Arc;

global_asm!(include_str!("entry.asm"));


struct Core2flag{
    is_in: bool,
}

impl Core2flag{
    pub fn is_in(&self)->bool{
        self.is_in
    }
    pub fn set_in(&mut self){
        self.is_in = true;
    }
}

lazy_static! {
    static ref CORE2_FLAG: Arc<Mutex<Core2flag>> = Arc::new(Mutex::new(
        Core2flag{
            is_in:false,
        }
    ));
}

extern "C"{
    fn eokernel();
}


fn trap_handler_delegate(){
    debug_os!("Trap handled.");
    return;
}
fn signal_handler_delegate(){
    debug_os!("Signal handled.");
    return;
}

// problem: it cannot work if no print between two nkapi.
#[no_mangle]
pub fn outer_kernel_init(){
    debug_os!("Demo payload init.");

    debug_os!("setting delegate");
    nkapi_set_delegate_handler(trap_handler_delegate as usize);
    debug_os!("setting signal");
    nkapi_set_signal_handler(signal_handler_delegate as usize);
    debug_os!("setting allocator");
    nkapi_set_allocator_range(eokernel as usize, OKSPACE_END);

    debug_os!("Config success.");
        
    init_heap();
    debug_os!("Heap init success.");
    
    // for vpn in (OKSPACE_START>>12)..(OKSPACE_END>>12) {
    //     debug_os!("mapping");
    //     nkapi_alloc(0, vpn.into(), MapType::Identical,
    //         MapPermission::R | MapPermission::W  | MapPermission::X );
    // }
        
    debug_os!("Memory init success.");

    nkapi_gatetest();

    // debug_os!("Trying illegal operation.");
    // satp::write(0);
    // nkapi_alloc(0, 0x80201.into(), MapType::Identical, MapPermission::W);
    // nkapi_alloc(0, 0x80600.into(), MapType::Specified(0x80202.into()), MapPermission::W);
    
    debug_os!("Test finished.");

    panic!("Demo payload exit.");
}