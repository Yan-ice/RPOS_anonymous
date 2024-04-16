use crate::config::*;

#[derive(Debug)]
#[repr(C)]
pub struct TaskContext {
    pub ra: usize,
    s: [usize; 12],
}


impl TaskContext {
    pub fn goto_trap_return() -> Self {
        unsafe{
            Self {
                ra: *((usize::MAX-0x3000+1 + 93*8) as *const usize),
                //ra: user_trap_return, the 93th in proxy context.
                // Yan_ice: temorarily constant here.
                s: [0; 12],
            }
        }
        
    }
}

