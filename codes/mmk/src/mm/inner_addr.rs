use crate::config::*;

#[repr(C)]
#[derive(Copy, Clone, Ord, PartialOrd, Eq, PartialEq)]
pub struct InnerPhysPageNum(pub usize);

#[repr(C)]
#[derive(Copy, Clone, Ord, PartialOrd, Eq, PartialEq)]
pub struct InnerPhysAddr(pub usize);


/// 
/// The four functions below is used to translate InnerPhysAddr and PhysAddr.
/// It is VERY important.
/// 

impl From<PhysPageNum> for InnerPhysPageNum {
    fn from(v: PhysPageNum) -> Self {
        return v.0.into();

        //assert_eq!(v.page_offset(), 0);
        let size = (NKSPACE_END - NKSPACE_START) >> 12;
        let target_pa = v.0 << 12;

        if target_pa <= OKSPACE_END && target_pa >= NKSPACE_START {
            return InnerPhysPageNum(v.0 + size);
        }
        return InnerPhysPageNum(v.0);
    }
}

impl From<InnerPhysPageNum> for PhysPageNum {
    fn from(v: InnerPhysPageNum) -> Self {
        return v.0.into();

        //assert_eq!(v.page_offset(), 0);
        let size = (NKSPACE_END - NKSPACE_START) >> 12;
        let target_pa = (v.0 << 12) - size;
        if target_pa <= OKSPACE_END && target_pa >= NKSPACE_START {
            return PhysPageNum(v.0 - size);
        }
        return PhysPageNum(v.0);
    }
}
impl From<PhysAddr> for InnerPhysAddr {
    fn from(v: PhysAddr) -> Self {
        return v.0.into();

        //assert_eq!(v.page_offset(), 0);
        let size = NKSPACE_END - NKSPACE_START;
        if v.0 <= OKSPACE_END && v.0 >= NKSPACE_START {
            return InnerPhysAddr(v.0 + size);
        }
        return InnerPhysAddr(v.0);
    }
}

impl From<InnerPhysAddr> for PhysAddr {
    fn from(v: InnerPhysAddr) -> Self {
        return v.0.into();

        //assert_eq!(v.page_offset(), 0);
        let size = NKSPACE_END - NKSPACE_START;
        let tar = v.0 - size;
        if tar <= OKSPACE_END && tar >= NKSPACE_START {
            return PhysAddr(tar);
        }
        return PhysAddr(v.0);
    }
}

///
/// Function above is VERY important.
/// 


impl From<usize> for InnerPhysAddr {
    fn from(v: usize) -> Self { Self(v) }
}
impl From<usize> for InnerPhysPageNum {
    fn from(v: usize) -> Self { Self(v) }
}

impl From<InnerPhysAddr> for usize {
    fn from(v: InnerPhysAddr) -> Self { v.0 }
}
impl From<InnerPhysPageNum> for usize {
    fn from(v: InnerPhysPageNum) -> Self { v.0 }
}

impl InnerPhysAddr {
    pub fn floor(&self) -> InnerPhysPageNum { InnerPhysPageNum(self.0 / PAGE_SIZE) }
    pub fn ceil(&self) -> InnerPhysPageNum { InnerPhysPageNum((self.0 - 1 + PAGE_SIZE) / PAGE_SIZE) }
    pub fn page_offset(&self) -> usize { self.0 & (PAGE_SIZE - 1) }
    pub fn aligned(&self) -> bool { self.page_offset() == 0 }
}

impl From<InnerPhysAddr> for InnerPhysPageNum {
    fn from(v: InnerPhysAddr) -> Self {
        //assert_eq!(v.page_offset(), 0);
        v.floor()
    }
}
impl From<InnerPhysPageNum> for InnerPhysAddr {
    fn from(v: InnerPhysPageNum) -> Self { Self(v.0 << PAGE_SIZE_BITS) }
}

impl InnerPhysAddr {
    pub fn get_ref<T>(&self) -> &'static T {
        unsafe {
            (self.0 as *const T).as_ref().unwrap()
        }
    }
    pub fn get_mut<T>(&self) -> &'static mut T {
        unsafe {
            (self.0 as *mut T).as_mut().unwrap()
        }
    }
}
impl InnerPhysPageNum {
    
    pub fn get_bytes_array(&self) -> &'static mut [u8] {
        let pa: InnerPhysAddr = self.clone().into();
        unsafe {
            core::slice::from_raw_parts_mut(pa.0 as *mut u8, 4096)
        }
    }
    pub fn get_mut<T>(&self) -> &'static mut T {
        let pa: InnerPhysAddr = self.clone().into();
        pa.get_mut()
    }

    pub fn next(&self) -> InnerPhysPageNum{
        return InnerPhysPageNum(self.0 + 1);
    }
    pub fn last(&self) -> InnerPhysPageNum{
        return InnerPhysPageNum(self.0 - 1);
    }
    pub fn step(&mut self){
        self.0 += 1;
    }
}


impl InnerPhysPageNum{
    pub fn get_pte_array(&self) -> &'static mut [PageTableEntry] {
        let pa: InnerPhysAddr = self.clone().into();
        unsafe {
            core::slice::from_raw_parts_mut(pa.0 as *mut PageTableEntry, 512)
        }
    }
}


#[derive(Copy, Clone)]
#[repr(C)]
pub struct PageTableEntry {
    pub bits: usize,
}

impl From<usize> for PageTableEntry{
    fn from(a: usize) -> Self{
        PageTableEntry { bits: a }
    }
}
impl From<PageTableEntry> for usize{
    fn from(a: PageTableEntry) -> Self{
        a.bits
    }
}

impl PageTableEntry {
    pub fn new(ppn: InnerPhysPageNum, flags: PTEFlags) -> Self {
        PageTableEntry {
            bits: ppn.0 << 10 | flags.bits() as usize,
        }
    }
    pub fn empty() -> Self {
        PageTableEntry {
            bits: 0,
        }
    }
    pub fn ppn(&self) -> InnerPhysPageNum {
        (self.bits >> 10 & ((1usize << 44) - 1)).into()
    }
    pub fn flags(&self) -> PTEFlags {
        PTEFlags::from_bits((self.bits & 0x3FF) as u16).unwrap()
    }
    pub fn is_valid(&self) -> bool {
        (self.flags() & PTEFlags::V) != PTEFlags::empty()
    }
    pub fn readable(&self) -> bool {
        (self.flags() & PTEFlags::R) != PTEFlags::empty()
    }
    pub fn writable(&self) -> bool {
        (self.flags() & PTEFlags::W) != PTEFlags::empty()
    }
    pub fn executable(&self) -> bool {
        (self.flags() & PTEFlags::X) != PTEFlags::empty()
    }
    pub fn set_flags(&mut self, flags: PTEFlags) {
        let new_flags: u16 = flags.bits().clone();
        self.bits = (self.bits & 0xFFFF_FFFF_FFFF_FF00) | (new_flags as usize);
    }

    // the 9th flag is used as COW flag.
    pub fn set_cow(&mut self) {
        (*self).bits = self.bits | (1 << 9);
    }
    pub fn reset_cow(&mut self) {
        (*self).bits = self.bits & !(1 << 9);
    }
    pub fn is_cow(&self) -> bool {
        self.bits & (1 << 9) != 0
    }
    pub fn set_bits(&mut self, ppn: InnerPhysPageNum, flags: PTEFlags) {
        self.bits = ppn.0 << 10 | flags.bits() as usize;
    }
    // only X+W+R can be set
    pub fn set_pte_flags(&mut self, flags: usize) {
        self.bits = (self.bits & !(0b1110 as usize)) | ( flags & (0b1110 as usize));
    }

}