use alloc::sync::Arc;
use mmi::nkapi_alloc;
use alloc::format;
use tiny_keccak::{Sha3, Hasher};

use crate::{config::*, debug_warn, debug_info, mm::frame_alloc, util::sbi::console_putchar, lang_items::Bytes};

pub static mut LOG_FRAME: usize = 0;
pub static mut CURRENT: usize = 0;

pub fn init(){
    unsafe {
        LOG_FRAME = usize::from(frame_alloc().unwrap()) << 12; 
        debug_info!("logging pageframe: {:x}", LOG_FRAME);
        *(LOG_FRAME as *mut usize) = 4;
    }
}

pub fn logging_handler(id: usize, para1: usize, para2: usize){
    match id {
        1 => {
            append(para1,para2);
        }
        2 => {
            gethash(para1);
        }
        3 => {
            printall();
        }
        _ => {
            debug_warn!("unsupported operation in logging!");
        }

    }
}

pub fn append(data_ptr: usize, data_len: usize){
    unsafe{
        if(data_ptr < NKSPACE_END){
            return;
        }
        let mut current: usize = *(LOG_FRAME as *mut usize);

        if current + data_len >= 4096 {
            debug_warn!("logging buffer is FULL!");
            return;
        }
        for a in 0..data_len {
            *((LOG_FRAME+current) as *mut u8) = *((data_ptr + a) as *mut u8);
            current = current + 1;
        } 
        *(LOG_FRAME as *mut usize) = current;
    } 
}

pub fn printall(){
    unsafe{
        let mut current: usize = *(LOG_FRAME as *mut usize);
        for a in 4..current {
            console_putchar(*((LOG_FRAME+a) as *mut u8) as usize);
        }
    }
}

pub fn gethash(out_ptr: usize){
    if(out_ptr < NKSPACE_END){
        return;
    }
    let mut hasher = Sha3::v256();
    unsafe{
        let mut current: usize = *(LOG_FRAME as *mut usize);
        for a in 4..current {
            hasher.update((&format!("{}",&*(out_ptr as *mut u8) )).as_bytes() );
        }
        let mut output: [u8; 32] = [0; 32];
        hasher.finalize(&mut output);
        (out_ptr as *mut u8).copy_from((&output[0]) as *const u8, 32);
    }
}
