
use super::{
    frame_alloc, frame_dealloc, nkapi_translate, nkapi_translate_va, InnerPhysAddr, InnerPhysPageNum, PageTableEntry
};
use crate::{debug_warn, debug_info};
use crate::mmi::*;
use crate::config::*;

use alloc::{vec::Vec, boxed::Box};
use bitflags::*;
use spin::Mutex;
use crate::*;

#[derive(Copy, Clone)]
pub struct PageTable {
    pt_id: usize,
    root_ppn: InnerPhysPageNum,
}

pub struct PageTableRecord {
    pub pt_id: usize,
    pub root_ppn: InnerPhysPageNum,
    frames: Vec<InnerPhysPageNum>
}

impl From<&PageTableRecord> for PageTable{
    fn from(pt: &PageTableRecord) -> Self {
        PageTable {
            pt_id: pt.pt_id,
            root_ppn: pt.root_ppn
        }
    }
}
impl From<&mut PageTableRecord> for PageTable{
    fn from(pt: &mut PageTableRecord) -> Self {
        PageTable {
            pt_id: pt.pt_id,
            root_ppn: pt.root_ppn
        }
    }
}

impl PageTable {
    pub fn id(&self) -> usize{
        return self.pt_id;
    }
    pub fn token(&self) -> usize {
        8usize << 60 | self.root_ppn.0
    }
}

/// Assume that it won't oom when creating/mapping.
impl PageTableRecord {
    pub fn id(&self) -> usize{
        return self.pt_id;
    }
    
    pub fn new(id: usize) -> Self {
        let ppn = frame_alloc().unwrap();
        PageTableRecord {
            pt_id: id,
            root_ppn: ppn,
            frames: Vec::new(),
        }
    }
    pub fn destroy(mut self){
        for mapped_frame in self.frames.into_iter(){
            frame_dealloc(mapped_frame);
        }
        self.pt_id = usize::MAX;
        self.root_ppn = 0.into();
    }

    fn find_pte_create(&mut self, vpn: VirtPageNum) -> Option<&mut PageTableEntry> {
        let idxs = vpn.indexes();
        let mut ppn = self.root_ppn;
        let mut result: Option<&mut PageTableEntry> = None;
        
        for i in 0..3 {
            let pte = &mut ppn.get_pte_array()[idxs[i]];
            if i == 2 {
                result = Some(pte);
                break;
            }
            if !pte.is_valid() {
                let ppn = frame_alloc().unwrap();
                if self.pt_id != 0{
                    // debug_info!("index i {}, idex {}", i, idxs[i]);
                    // debug_info!{"invalid!!!!!!!!"}
                    // debug_info!("i {}", i);
                    // debug_info!("root ppn {:x}", self.root_ppn.0);
                    // debug_info!("vpn is {:x}", vpn.0);
                    // debug_info!("ppn is {:x}", ppn.0);
                }

                *pte = PageTableEntry::new(ppn, PTEFlags::V);
                self.frames.push(ppn);
                
            }
            ppn = pte.ppn();
        }

        // if self.pt_id != 0{
        //     debug_info!("create page table for pt: {}, vpn: 0x{:x}, ppn: 0x{:x}", self.pt_id, vpn.0, ppn.0);
        // }
        result

    }
    pub fn find_pte(&self, vpn: VirtPageNum) -> Option<&PageTableEntry> {
        let idxs = vpn.indexes();
        let mut ppn = self.root_ppn;
        let mut result: Option<&PageTableEntry> = None;
        for i in 0..3 {
            let pte = &ppn.get_pte_array()[idxs[i]];
            if !pte.is_valid() {
                return None;
            }
            if i == 2 {
                result = Some(pte);
                break;
            }
            ppn = pte.ppn();
        }
        result
    }

    // level = {1,2,3}
    pub fn find_pte_level(&self, vpn:VirtPageNum, level:usize) -> Option<&PageTableEntry> {
        let idxs = vpn.indexes();
        let mut ppn = self.root_ppn;
        let mut result: Option<&PageTableEntry> = None;
        for i in 0..(level) {
            let pte = &ppn.get_pte_array()[idxs[i]];
            if !pte.is_valid() {
                return None;
            }
            if i == (level -1) {
                result = Some(pte);
                break;
            }
            ppn = pte.ppn();
        }
        result
    }
    
    // only X+W+R can be set
    // return -1 if find no such pte
    pub fn set_pte_flags(&mut self, vpn: VirtPageNum, flags: usize) -> isize{
        let idxs = vpn.indexes();
        let mut ppn = self.root_ppn;
        for i in 0..3 {
            let pte = &mut ppn.get_pte_array()[idxs[i]];
            if i == 2 {
                // if pte == None{
                //     panic!("set_pte_flags: no such pte");
                // }
                // else{
                    pte.set_pte_flags(flags);
                // }
                break;
            }
            if !pte.is_valid() {
                return -1;
            }
            ppn = pte.ppn();
        }
        0
    }

    pub fn trace_address(&mut self, va: VirtAddr){
        let vpn = va.floor();

        let idxs = vpn.indexes();
        let mut ppn = self.root_ppn;
        let mut result: Option<&PageTableEntry> = None;
        // print!("Tracing translation for {:?}:",va);

        for i in 0..3 {
            // print!("{:x}[{}] -> ", ppn.0, i);
            let pte = &ppn.get_pte_array()[idxs[i]];
            
            if !pte.is_valid() {
                print!("INVALID\n");
                // debug_info!("Trace failed. {:?} -> X", va);
                return;
            }
            if i == 2 {
                print!("{:x}\n", pte.ppn().0);
                // debug_info!("Trace finished. {:?} -> {:?}", va, self.translate_va(va));
                return;
            }
            ppn = pte.ppn();
        }
    }

    pub fn print_pagetable(&mut self, from: usize, to:usize){
        // debug_info!("[pt] printing pagetable with token {:x}",self.token());

        let idxs = [0 as usize;3];
        let mut ppns = [InnerPhysPageNum(0);3];
        ppns[0] = self.root_ppn;
        for i in 0..512{
            // debug_info!("[pt] printing progress ({}/512)",i);
            let pte = &mut ppns[0].get_pte_array()[i];
            if !pte.is_valid(){
                continue;
            }
            ppns[1] = pte.ppn();
            for j in 0..512{
                let pte = &mut ppns[1].get_pte_array()[j];
                if !pte.is_valid(){
                    continue;
                }
                ppns[2] = pte.ppn();
                for k in 0..512{
                    let pte = &mut ppns[2].get_pte_array()[k];
                    if !pte.is_valid(){
                        continue;
                    }
                    let va = ((((i<<9)+j)<<9)+k)<<12 ;
                    let pa = pte.ppn().0 << 12 ;
                    let flags = pte.flags();
                    if va < from || va > to {
                        continue;
                    }
                    // debug_info!("va:0x{:x}  pa:0x{:x} flags:{:?}",va,pa,flags);
                }
            }
        }
    }
    
    #[allow(unused)]
    pub fn map(&mut self, vpn: VirtPageNum, ppn: InnerPhysPageNum, flags: PTEFlags) {
    
        let pte = self.find_pte_create(vpn).unwrap();

        if pte.is_valid() {
            // debug_warn!("vpn 0x{:x} is mapped before mapping.", vpn.0);
            pte.set_flags(flags | PTEFlags::V | PTEFlags::A | PTEFlags::D);
            return;
        }

        //Yan_ice: add A bit and D bit to support nezha-D1
        *pte = PageTableEntry::new(ppn, 
            flags | PTEFlags::V | PTEFlags::A | PTEFlags::D);
    }
    #[allow(unused)]
    pub fn remap_cow(&mut self, vpn: VirtPageNum, ppn: InnerPhysPageNum, former_ppn: InnerPhysPageNum) {
        let pte = self.find_pte_create(vpn).unwrap(); // former ppn
        // debug_info!{"remapping {:?}", 
        *pte = PageTableEntry::new(ppn, pte.flags() & !PTEFlags::O | PTEFlags::W );
        ppn.get_bytes_array().copy_from_slice(former_ppn.get_bytes_array());
    }
    #[allow(unused)]
    pub fn unmap(&mut self, vpn: VirtPageNum) {
        let pte = self.find_pte_create(vpn).unwrap();
        assert!(pte.is_valid(), "vpn {:?} is invalid before unmapping", vpn);
        *pte = PageTableEntry::empty();
    }
    pub fn translate(&self, vpn: VirtPageNum) -> Option<PageTableEntry> {
        let p = self.find_pte(vpn)
            .map(|pte| {pte.clone()});
        p
    }
    pub fn translate_va(&self, va: VirtAddr) -> Option<InnerPhysAddr> {
        if let Some(pte) = self.find_pte(va.clone().floor()) {
            if pte.is_valid() {
                let pa: InnerPhysAddr = InnerPhysAddr{0: pte.ppn().0*crate::config::PAGE_SIZE + va.page_offset()};
                return Some(pa);
            }
        }
        None
        
    }
    pub fn set_cow(&mut self, vpn: VirtPageNum) {
        self.find_pte_create(vpn).unwrap().set_cow();
    }
    pub fn reset_cow(&mut self, vpn: VirtPageNum) {
        self.find_pte_create(vpn).unwrap().reset_cow();
    }
    pub fn set_flags(&mut self, vpn: VirtPageNum, flags: PTEFlags) {
        self.find_pte_create(vpn).unwrap().set_flags(flags);
    }

   pub fn map_kernel_shared(&mut self, kernel_pagetable: &mut PageTableRecord){

        // insert shared pte of os
        let kernel_vpn:VirtPageNum = (NKSPACE_END / PAGE_SIZE).into();
        let pte_kernel = kernel_pagetable.find_pte_level(kernel_vpn, 1);
        let idxs = kernel_vpn.indexes();
        let mut ppn: InnerPhysPageNum = self.root_ppn;
        let pte = &mut ppn.get_pte_array()[idxs[0]];
        *pte = *pte_kernel.unwrap();


        //insert top va(kernel stack + trampoline)
        let kernel_vpn:VirtPageNum = (TRAMPOLINE / PAGE_SIZE).into();
        let pte_kernel = kernel_pagetable.find_pte_level(kernel_vpn, 1);
        let idxs = kernel_vpn.indexes();
        let mut ppn: InnerPhysPageNum = self.root_ppn;
        let pte = &mut ppn.get_pte_array()[idxs[0]];
        *pte = *pte_kernel.unwrap();

        // Yan_ice: MMIO would be mapped by the Payload instead of MMK (os kernel)
        // for pair in MMIO {
        //     let page_num = pair.0 / PAGE_SIZE;
        //     self.map(page_num.into(), page_num.into(), PTEFlags::R | PTEFlags::W);
        // }
        
    }

    //Yan_ice： 这个是satp！
    pub fn token(&self) -> usize {
        (8usize << 60) | (self.pt_id << 44) | (self.root_ppn.0)
        //MODE activate | ASID | root_ppn
    }


}

