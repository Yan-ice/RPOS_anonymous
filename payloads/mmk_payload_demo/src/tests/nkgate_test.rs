use crate::{begin_test};
use crate::debug_info;
use crate::mmi::*; use crate::config::*;

const valA: usize = 66666;
const test_pt: usize = 2333;
const va_test: VirtAddr = VirtAddr{0: 0x10000000336};
const pa_test: PhysAddr = PhysAddr{0: 0x20000000336};

pub fn nkapi_gatetest(){
    begin_test!("nkapi gate test",
    {

        nkapi_pt_init(test_pt, true);
        debug_info!("p");
        nkapi_alloc(test_pt, va_test.into(), crate::mmi::MapType::Identical, MapPermission::R);
        debug_info!("p");
        if let Some(pa) = nkapi_translate_va(test_pt, va_test.into()) {
            assert_eq!(pa.0, va_test.0, "testing identical alloc.");
            debug_info!("nkapi: identical alloc test passed.");
        }else{
            panic!("nkapi: identical alloc test failed with None.")
        }
        
        nkapi_dealloc(test_pt, va_test.into());
        debug_info!("p");
        if let Some(_) = nkapi_translate_va(test_pt, va_test.into()) {
            panic!("nkapi: identical dealloc test failed.")
        }else{
            debug_info!("nkapi: dealloc test passed.");
        }

        nkapi_alloc(test_pt, va_test.into(), crate::mmi::MapType::Specified(pa_test.floor()), MapPermission::R);
        debug_info!("p");
        if let Some(pa) = nkapi_translate_va(test_pt, va_test.into()) {
            assert_eq!(pa.0, pa_test.0, "testing identical alloc.");
            debug_info!("nkapi: specified alloc test passed.");
        }else{
            panic!("nkapi: specified alloc test failed with None.")
        }
        debug_info!("p");
        nkapi_dealloc(test_pt, va_test.into());

    }
    );
}