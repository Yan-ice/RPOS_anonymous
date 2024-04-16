use riscv::register::{
    mtvec::TrapMode,
    stval,
    stvec, hpmcounter21::read
};

use crate::{mmi::*, mm::pt_current};
use crate::config::*;
use crate::*;
use tiny_keccak::Hasher;
use tiny_keccak::Sha3;
use alloc::format;

pub const MMKCALL_ECHO: usize = 0x400;
pub const MMKCALL_MEASURE: usize = 0x401;
pub const MMKCALL_PKCS: usize = 0x402;
pub const MMKCALL_LOGGING: usize = 0x403;


///
/// trap in NK would be handled here.
/// 
pub fn nk_trap_handler(mut ctx_addr: usize) -> usize {
    //ctx_addr = TRAP_CONTEXT;
    if let Some(pa) = nkapi_translate_va(pt_current(), ctx_addr.into()){
        unsafe{
            let ctx: &mut TrapContext = &mut *(pa.0 as *mut TrapContext); 
            ctx.sepc += 4;
            return nk_syscall_impl(ctx);
        }
    }else{
        panic!("invalid trap context!");
    }
}

pub fn nk_syscall_impl(ctx: &mut TrapContext) -> usize {
    debug_debug!("handle nk syscall {} for [{}]",ctx.x[17] ,pt_current());
    // let stval = stval::read();
    let call_id: usize = ctx.x[17];

    match call_id {
        MMKCALL_ECHO => {
            debug_info!("echo: {:x}", ctx.x[10]);
        }
        MMKCALL_MEASURE => {
            ctx.x[10] = get_measure();
        }
        MMKCALL_PKCS =>{
        //Yan_ice: temporarily remove pkcs.
            //ctx.x[10] = crate::pkcs::cryptoki_handler(ctx.x[10], 
            //[ctx.x[11],ctx.x[12],ctx.x[13],ctx.x[14],ctx.x[15],ctx.x[16]]);
        }
        MMKCALL_LOGGING => {
            crate::logging::logging_handler(ctx.x[10],ctx.x[11], ctx.x[12]);
        }
        _=>{debug_warn!("Unsupported syscall id [{}]", call_id);}
    }

    
    return 0;
}

fn get_measure() -> usize{
    let mut hasher = Sha3::v256();
    unsafe{
        let start: usize= OKSPACE_START;
        let end: usize = OKSPACE_END;
        for i in start..end{
            let temp = *(i as usize as *const usize);
            hasher.update((&format!("{}", temp)).as_bytes());
        }
        let mut result = [0u8; 32];
        hasher.finalize(&mut result);

        let mut hash = [0u8; 8];
        hash.copy_from_slice(&result[..8]);
        return u64::from_le_bytes(hash) as usize;
    }
}


