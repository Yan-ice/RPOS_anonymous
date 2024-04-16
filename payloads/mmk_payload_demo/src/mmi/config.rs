#[allow(unused)]

pub const PAGE_SIZE: usize = 0x1000;//should not change
pub const PAGE_SIZE_BITS: usize = 0xc;
pub const NK_TRAMPOLINE: usize = usize::MAX - PAGE_SIZE + 1;