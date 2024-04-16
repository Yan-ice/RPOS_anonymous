use riscv::register::{
    scause::{
        self,
        Trap,
        Exception,
        Interrupt,
    },
    stval,
    sstatus,
    sstatus::SPP, sstatus::Sstatus
};

use crate::{mmi::*, task::current_user_id, debug_info, debug_os, debug_error};
use crate::config::*;
use crate::timer::set_next_trigger;

use crate::syscall::sys_musl::syscall;

use crate::task::{
    exit_current_and_run_next,
    suspend_current_and_run_next,
    current_user_token,
    current_trap_cx,
    current_task,
    Signals,
    perform_signal_handler,
};

use core::arch::asm;

use crate::config::{TRAP_CONTEXT, TRAMPOLINE};


#[repr(C)]
#[derive(Debug, Clone)]
pub struct TrapContext {
    pub x: [usize; 32],
    pub sstatus: Sstatus,
    pub sepc: usize,
    //pub kernel_satp: usize,
    pub kernel_sp: usize,
    pub trap_handler: usize,
    
}

impl TrapContext {
    pub fn set_sp(&mut self, sp: usize) { self.x[2] = sp; }
    pub fn get_sp(& self)->usize { self.x[2] }

    pub fn app_init_context(
        // 只有三个函数调用过这个方法，在初始化的时候
        //，也就是从elf得到pcb的时候，trap context要一起初始化，这里初始化为elf的header
        entry: usize, // trap之前的上一条指令
        sp: usize, // 当前用户栈的栈顶
        //kernel_satp: usize,  // 未理解的内核页表，这东西在干啥
        kernel_sp: usize  // 内核栈栈顶
    ) -> Self {
        // set CPU privilege to User after trapping back
        //Yan_ice: If it is supervisor mode, it can use sret successfully.
        unsafe{
            sstatus::set_spp(SPP::User);
            let sstatus = sstatus::read();
            let mut cx = Self {
                x: [0; 32],
                sstatus,
                sepc: entry,
                kernel_sp,
                trap_handler: TRAMPOLINE
            };
            cx.set_sp(sp);
            cx
        }
        
    }

}

///
/// Main trap delegate handler in Outer Kernel.
/// 
pub fn trap_handler_delegate(ctx: *mut TrapContext){
    let scause: scause::Scause = scause::read();
    let stval = stval::read();
    unsafe{
        handle_outer_trap(&mut *ctx,scause,stval);
    }
    return;
}

fn handle_outer_trap(cx: &mut TrapContext, scause: scause::Scause, stval: usize){
    
    //TODO: entry gate
    //trap到outer kernel时，切换为kernel trap。
    match scause.cause() {
        Trap::Exception(Exception::UserEnvCall) =>{
            // jump to next instruction anyway
            // let mut cx = current_trap_cx();
            cx.sepc += 4;

            //G_SATP.lock().set_syscall(cx.x[17]);
            let syscall_id = cx.x[17];
            if syscall_id > 62 && syscall_id != 113 {
                unsafe {
                    //llvm_asm!("sfence.vma zero, zero" :::: "volatile");
                }
            }
            
            //get system call return value
            let result = syscall(syscall_id, [cx.x[10], cx.x[11], cx.x[12], cx.x[13], cx.x[14], cx.x[15]]);
            // cx is changed during sys_exec, so we have to call it again
            //if syscall_id != 64 && syscall_id != 63{
            //    debug_os!("[{}]syscall-({}) = 0x{:X}  ", current_task().unwrap().pid.0, syscall_id, result);
            //} 
            //cx = current_trap_cx();
            cx.x[10] = result as usize;
        }
        Trap::Exception(Exception::InstructionFault) |
        Trap::Exception(Exception::InstructionPageFault) => {
            let task = current_task().unwrap();
            debug_info!{"pinInstructionFault"}
            //debug_os!("prev syscall = {}", G_SATP.lock().get_syscall());
            
            debug_info!(
                "[kernel] {:?} in application-{}, bad addr = {:#x}, bad instruction = {:#x}, core dumped.",
                scause.cause(),
                task.pid.0,
                stval,
                current_trap_cx().sepc,
            );
            drop(task);
            // page fault exit code
            let current_task = current_task().unwrap();
            if current_task.is_signal_execute() || !current_task.check_signal_handler(Signals::SIGSEGV){
                drop(current_task);
                exit_current_and_run_next(-2);
            }
        }
        
        Trap::Exception(Exception::IllegalInstruction) => {
            debug_os!("[kernel] IllegalInstruction in application, continue.");
            //let mut cx = current_trap_cx();
            //cx.sepc += 4;
            debug_info!(
                "{:?} in application, bad addr = {:#x}, bad instruction = {:#x}, core dumped.",
                scause.cause(),
                cx.sepc,
                stval
            );
            // illegal instruction exit code
            exit_current_and_run_next(-3);
        }

        Trap::Exception(Exception::LoadFault) |
        Trap::Exception(Exception::StoreFault) |
        Trap::Exception(Exception::StorePageFault) |
        Trap::Exception(Exception::LoadPageFault) =>{
            
            let is_load: bool;
            if scause.cause() == Trap::Exception(Exception::LoadFault) || scause.cause() == Trap::Exception(Exception::LoadPageFault) {
                is_load = true;
            } else {
                is_load = false;
            }
            let va: VirtAddr = (stval as usize).into();
            // The boundary decision
            if va > TRAMPOLINE.into() {
                panic!("VirtAddr out of range: {:?}",va);
            }
            
            //println!("check_lazy 1");
            let lazy = current_task().unwrap().check_lazy(va, is_load);
            if lazy != 0 {
                // page fault exit code
                let current_task = current_task().unwrap();
                if current_task.is_signal_execute() || !current_task.check_signal_handler(Signals::SIGSEGV){
                    
                    debug_error!(
                        "[kernel] {:?} in application, bad addr = {:#x}, bad instruction = {:#x}, core dumped.",
                        scause.cause(),
                        stval,
                        current_trap_cx().sepc,
                    );
                    drop(current_task);
                    exit_current_and_run_next(-2);
                }
            }
            unsafe {
                //llvm_asm!("sfence.vma" :::: "volatile");
                asm!("fence.i");
            }
        }
        Trap::Interrupt(Interrupt::SupervisorTimer) => {
            debug_os!{"pinTimer"}
            set_next_trigger();
            suspend_current_and_run_next();
            //is_schedule = true;
        }
        
        _ => {
            panic!("Unsupported trap {:?}, stval = {:#x}!", scause.cause(), stval);
        }
    }
}