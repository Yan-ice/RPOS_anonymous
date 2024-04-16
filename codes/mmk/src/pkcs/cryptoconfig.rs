#[allow(unused)]
use alloc::vec::Vec;

//pub struct OsRng;

pub type CkVoidPtr = u64;


pub type CkByte = u8;
pub type CkBytePtr = *mut CkByte;
pub type CkUlong = u32;
pub type CkBbool = CkByte;
pub type CckSlotId = CkUlong;
pub type CkSlotIdPtr = *mut CckSlotId;
pub type CkUlongPtr = *mut CkUlong;
pub type CkSlotId = CkUlong;
pub type CkFlags = CkUlong;
pub type CkSessionHandle = CkUlong;
pub type CkSessionHandlePtr = *mut CkSessionHandle;
pub type CkUserType = CkUlong;
pub type CkChar = CkByte;
pub type CkCharPtr = *mut CkChar;
pub type CkMechanismType = CkUlong;

#[derive(Clone, Copy)]
pub struct CkMechanism {
    pub mechanism: CkMechanismType,
    pub p_parameter: CkVoidPtr,
    pub ul_parameter_len: CkUlong,
}

pub type CkMechanismPtr = *const CkMechanism;
pub type CkObjectHandle = CkUlong;
pub type CkAttributeType = CkUlong;
pub type CkObjectHandlePtr = *mut CkObjectHandle;
#[derive(Clone, Copy)]
pub struct Copen{
    pub csid: CkSlotId,
    pub cflag: CkFlags,
    pub c: CkVoidPtr,
    pub notify: fn(),
    pub cshp: CkSessionHandlePtr
}

#[derive(Clone, Copy)]
pub struct CkAttribute {
    pub t: CkAttributeType,
    pub p_value: CkVoidPtr,
    pub ul_value_len: CkUlong,
}
#[derive(Clone, Copy)]
pub struct CSlotLIst{
    pub cb: CkBbool, 
    pub cslot: CkSlotIdPtr, 
    pub cup: CkUlongPtr
}

#[derive(Clone, Copy)]
pub struct Clogin{
    pub csh: CkSessionHandle, 
    pub cut: CkUserType, 
    pub ccp: CkCharPtr, 
    pub cu: CkUlong
}

#[derive(Clone, Copy)]
pub struct Einit{
    pub csh: CkSessionHandle, 
    pub cmp: CkMechanismPtr, 
    pub coh: CkObjectHandle
}

#[derive(Clone, Copy)]
pub struct Encrypt{
    pub csh: CkSessionHandle, 
    pub cbp1: CkBytePtr, 
    pub cu: CkUlong, 
    pub cbp2: CkBytePtr, 
    pub cup: CkUlongPtr
}

#[derive(Clone, Copy)]
pub struct Dinit{
    pub csh: CkSessionHandle,
    pub cmp: CkMechanismPtr,
    pub coh: CkObjectHandle
}

#[derive(Clone, Copy)]
pub struct Decrypt{
    pub csh: CkSessionHandle,
    pub cbp1: CkBytePtr,
    pub cu1: CkUlong,
    pub cbp2: CkBytePtr,
    pub cu2: CkUlongPtr,
}

#[derive(Clone, Copy)]
pub struct Dfinal{
    pub csh: CkSessionHandle,
    pub cbp: CkBytePtr,
    pub cup: CkUlongPtr
}

#[derive(Clone, Copy)]
pub struct Generate{
    pub csh: CkSessionHandle,
    pub cmp: CkMechanismPtr,
    pub cap1: CkAttributeType,
    pub cu1: CkUlong,
    pub cap2: CkAttributeType,
    pub cu2: CkUlong,
    pub cohp1: CkObjectHandlePtr,
    pub cohp2: CkObjectHandlePtr
}

pub struct SessionManager{
    pub base: u64,
    pub valid: Vec<u64>,
}

impl SessionManager{

    pub fn new() -> Self{
        Self{
            base: 1,
            valid: Vec::new()
        }
    }

    pub fn session_alloc(&mut self) -> u64{
        let ans = self.base;
        self.valid.push(ans);
        self.base = self.base + 1;
        return ans;
    }

    pub fn session_dealloc(&mut self, session: u64){
        // self.valid.remove(|x| {
        //     self.valid.get(x) == session
        // });
    }

    pub fn is_valid(&self, session: u64)->bool{
        let temp = &self.valid;
        for (index, item) in temp.into_iter().enumerate(){
            if *item == session{
                return true;
            }
        }
        return false;
    }

}

pub const E_SHA3: u32 = 1;
pub const E_AES256: u32 = 2;

// pub enum EEncryptMechanism{
//     SHA3 = 1,
//     AES256 = 2,
// }

// pub enum EDecryptMechanism{
//     AES256 = 2,
// }


type CkAttributePtr = *const CkAttribute;

