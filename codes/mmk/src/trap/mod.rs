mod trap_handle;

use riscv::register::{
    mtvec::TrapMode,
    sie,
    stvec
};
use crate::config::*;
use crate::*;
use core::arch::global_asm;
pub use trap_handle::nk_trap_handler;

global_asm!(include_str!("trap.S"));
global_asm!(include_str!("trap_signal.S"));

extern "C"{
    fn __signal_trampoline();
    fn __alltraps();
    fn __restore();
    fn _ktrap();
    fn _kreturn();
}

pub fn init(){
    unsafe {
        PROXYCONTEXT().usr_trap_return = TRAMPOLINE + __restore as usize - __alltraps as usize;
        PROXYCONTEXT().usr_trampoline = TRAMPOLINE;
        PROXYCONTEXT().kernel_trampoline = TRAMPOLINE + _ktrap as usize - __alltraps as usize;
        stvec::write(PROXYCONTEXT().kernel_trampoline, TrapMode::Direct);
    }
}

pub fn enable_timer_interrupt() {
    unsafe { sie::set_stimer(); }
}
