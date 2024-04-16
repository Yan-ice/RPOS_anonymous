use super::{InnerPhysAddr, InnerPhysPageNum, PhysAddr, PhysPageNum};
use alloc::vec::{self, Vec};
use spin::Mutex;
use crate::*;
use lazy_static::*;
use alloc::collections::BTreeMap;
use crate::debug_info;


pub trait FrameAllocator {
    fn new() -> Self;
    fn alloc(&mut self, src: u8) -> Option<InnerPhysPageNum>;
    fn dealloc(&mut self, ppn: InnerPhysPageNum, src: u8);
    fn add_ref(&mut self, ppn: InnerPhysPageNum, src: u8);
    fn fork(&mut self, ppn: InnerPhysPageNum, src: u8, dst: u8);
    fn enquire_ref(&mut self, ppn: InnerPhysPageNum) -> Vec<u8>;
}

pub struct StackFrameAllocator {
    current: InnerPhysPageNum,
    end: InnerPhysPageNum,
    recycled: Vec<InnerPhysPageNum>,
    refcounter: BTreeMap<InnerPhysPageNum, Vec<u8>>,
}

impl StackFrameAllocator {
    pub fn init(&mut self, l: InnerPhysPageNum, r: InnerPhysPageNum) {
        self.current = l;
        self.end = r;
        debug_info!("last {} Physical Frames.", self.end.0 - self.current.0);
    }

    pub fn add_free(&mut self, ppn: InnerPhysPageNum){
        self.recycled.push(ppn);
    }

    pub fn print_free(&mut self){
        let size = self.recycled.len() + self.end.0 - self.current.0;
        debug_info!("Free memory: {} pages", size);
    }
}
impl FrameAllocator for StackFrameAllocator {
    fn new() -> Self {
        Self {
            current: InnerPhysPageNum(0),
            end: InnerPhysPageNum(0),
            recycled: Vec::new(),
            refcounter: BTreeMap::new(),
        }
    }
    fn alloc(&mut self, owner: u8) -> Option<InnerPhysPageNum> {
        
        if let Some(ppn) = self.recycled.pop() {
            //debug_info!{"alloced recycled ppn: {:X}", ppn}
            self.refcounter.insert(ppn, alloc::vec![owner]);

            Some(ppn.into())
        } else {
            if self.current == self.end {
                None
            } else {
                //debug_info!{"alloced ppn: {:X}", self.current}
                self.refcounter.insert(self.current, alloc::vec![owner]);

                self.current.step();
                
                Some(self.current.last())
            }
        }
    }

    fn dealloc(&mut self, ppn: InnerPhysPageNum, user: u8) {
        // if self.refcounter.contains_key(&ppn) {
        // let no_ref = false;
        if let Some(ref_times) = self.refcounter.get_mut(&ppn) {
            ref_times.retain(|x|{*x != user});

            //debug_info!{"dealloced ppn: {:X}", ppn}
                
            // debug_info!{"the refcount of {:X} decrease to {}", ppn, ref_times}
            if ref_times.is_empty() {
                self.refcounter.remove(&ppn);
                // validity check
                // if ppn >= self.current || self.recycled
                //     .iter()
                //     .find(|&v| {*v == ppn})
                //     .is_some() {
                //     // panic!("Frame ppn={:#x} has not been allocated!", ppn);
                // }
                // recycle
                self.recycled.push(ppn);
                return;
            }

            if ref_times[0] == 0 && ref_times.len() == 2 {
                ref_times.remove(0);
            }


        }      
    }

    fn fork(&mut self, ppn: InnerPhysPageNum, src: u8, dst: u8){
        if let Some(ref_times) = self.refcounter.get_mut(&ppn) {
            if ref_times[0] == 0 || ref_times[0] == src{
                if ref_times[0] != 0 {
                    ref_times.insert(0, 0);
                }
                ref_times.push(dst);
            }else{
                debug_info!{"only the owner can fork pages! {:X}", ppn.0}
            }
        }      
    }

    fn add_ref(&mut self, ppn: InnerPhysPageNum, src: u8) {
        //debug_info!("adding ref: {:x}",ppn.0);
        let ref_user = self.refcounter.get_mut(&ppn).unwrap();
        ref_user.push(src);
    }


    fn enquire_ref(&mut self, ppn: InnerPhysPageNum) -> Vec<u8>{
        if let Some(ref_times) = self.refcounter.get_mut(&ppn) {
            if ref_times[0] == 0 && ref_times.len() == 2 {
                ref_times.remove(0);
            }
    
            return (*ref_times).to_vec().clone();
        }
        return Vec::new();
    }

}

type FrameAllocatorImpl = StackFrameAllocator;

lazy_static! {
    pub static ref FRAME_ALLOCATOR: Mutex<FrameAllocatorImpl> =
        Mutex::new(FrameAllocatorImpl::new());
    pub static ref OUTER_FRAME_ALLOCATOR: Mutex<FrameAllocatorImpl> =
        Mutex::new(FrameAllocatorImpl::new()); 
}

extern "C" {
    fn ekernel();
}

pub fn init_frame_allocator() {

    FRAME_ALLOCATOR
        .lock()
        .init(InnerPhysAddr::from(ekernel as usize).ceil(), InnerPhysAddr::from(NKSPACE_END).floor());
    
}


pub fn outer_frame_alloc(owner: u8) -> Option<InnerPhysPageNum> {
    
    let mut outer_allocator = OUTER_FRAME_ALLOCATOR.lock();
    
    if outer_allocator.current.0 == 0 {
        let st: PhysPageNum = PhysAddr::from(PROXYCONTEXT().allocator_start as usize).ceil();
        let ed: PhysPageNum = PhysAddr::from(PROXYCONTEXT().allocator_end as usize).floor();
        debug_warn!("Allocator config: {:?} - {:?}", st, ed);

        outer_allocator.init(st.into(), ed.into());
        
    }

    let pn = outer_allocator.alloc(owner);
    
    if let Some(ppn) = pn{
        let bytes_array = ppn.get_bytes_array();
        for i in bytes_array {
            *i = 0;
        }
    }
    pn
    
}

pub fn outer_frame_dealloc(ppn: InnerPhysPageNum, user: u8) {
    OUTER_FRAME_ALLOCATOR.lock().dealloc(ppn, user);
}

pub fn frame_alloc() -> Option<InnerPhysPageNum> {
    let pn = FRAME_ALLOCATOR
        .lock()
        .alloc(0);
    
    if let Some(ppn) = pn{
        let bytes_array = ppn.get_bytes_array();
        for i in bytes_array {
            *i = 0;
        }
    }else{
        debug_error!("No enough space for Page Table!");
    }
    pn
}

pub fn outer_fork(ppn: InnerPhysPageNum, user: u8, target: u8) {
    OUTER_FRAME_ALLOCATOR
        .lock()
        .fork(ppn, user, target);
}

pub fn outer_frame_add_ref(ppn: InnerPhysPageNum, user: u8) {
    OUTER_FRAME_ALLOCATOR
        .lock()
        .add_ref(ppn, user);
}

pub fn frame_dealloc(ppn: InnerPhysPageNum) {
    FRAME_ALLOCATOR
        .lock()
        .dealloc(ppn, 0);
}

pub fn enquire_ref(ppn: InnerPhysPageNum) -> Vec<u8> {
    OUTER_FRAME_ALLOCATOR
        .lock()
        .enquire_ref(ppn)
}

pub fn add_free(ppn: InnerPhysPageNum){
    FRAME_ALLOCATOR.lock().recycled.push(ppn);
}


// #[allow(unused)]
// pub fn frame_allocator_test() {
//     let mut v: Vec<FrameTracker> = Vec::new();
//     for i in 0..5 {
//         let frame = frame_alloc().unwrap();
//         debug_info!("{:?}", frame);
//         v.push(frame);
//     }
//     v.clear();
//     for i in 0..5 {
//         let frame = frame_alloc().unwrap();
//         debug_info!("{:?}", frame);
//         v.push(frame);
//     }
//     drop(v);
//     debug_info!("frame_allocator_test passed!");
// }
