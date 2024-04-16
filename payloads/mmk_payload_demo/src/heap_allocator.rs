use buddy_system_allocator::LockedHeap;
use crate::debug_info;
static mut HEAP_SPACE: [u8; 32*0x1000] = [0; 32*0x1000];

#[global_allocator]
pub static HEAP_ALLOCATOR: LockedHeap = LockedHeap::empty();

#[alloc_error_handler]
pub fn handle_alloc_error(layout: core::alloc::Layout) -> ! {
    debug_info!("[handle_alloc_error]: May have no enough heap memory!");
    panic!("OS Heap allocation error, layout = {:?}", layout);
}

pub fn init_heap() {
    unsafe {
        HEAP_ALLOCATOR
            .lock()
            .init(HEAP_SPACE.as_ptr() as usize, 32*0x1000);
    }
}
