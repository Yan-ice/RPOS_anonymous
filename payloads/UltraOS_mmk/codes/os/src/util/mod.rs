pub mod mm_util;
pub mod memory_set;
pub mod vma;
pub mod sbi;

pub use mm_util::{
    translated_array_copy,
    translated_array_ref
};

pub use memory_set::{MemorySet, KERNEL_SPACE, KERNEL_MMAP_AREA};
pub use vma::*;
pub use sbi::*;

pub fn log2(num:usize) -> usize{
    let mut num = num;
    if num == 0{
        return 0;
    }
    for i in 0..64{
        if num > 0 {
            num = num >> 1;
        }
        else{
            return i-1;
        }
    }
    64
}