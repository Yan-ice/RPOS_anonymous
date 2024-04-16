mod crypto_impl;
mod state;
mod cryptoconfig;
use crate::debug_error;

use self::cryptoconfig::{CSlotLIst, CkSessionHandle, Clogin, Einit, Encrypt, Dinit, Decrypt, Dfinal, Generate, Copen, CkObjectHandle, CkMechanism};

//general interface
pub const C_INITIALIZE: usize = 0x1;
pub const C_FINALIZE: usize = 0x2;
pub const C_GET_FUNCTION_LIST: usize = 0x4;

//slot management
pub const C_GET_SLOT_LIST: usize = 0x11;

//session management
pub const C_OPEN_SESSION: usize = 0x21;
pub const C_CLOSE_SESSION: usize = 0x22;
pub const C_LOGIN: usize = 0x27;
pub const C_LOGOUT: usize = 0x28;

//encrypt management
pub const C_ENCRYPT_INIT: usize = 0x31;
pub const C_ENCRYPT: usize = 0x32;
pub const C_ENCRYPT_UPDATE: usize = 0x33;
pub const C_ENCRYPT_FINAL: usize = 0x34;

//decrypt management
pub const C_DECRYPT_INIT: usize = 0x41;
pub const C_DECRYPT: usize = 0x42;
pub const C_DECRYPT_UPDATE: usize = 0x43;
pub const C_DECRYPT_FINAL: usize = 0x44;

//key management
pub const C_GENERATE_KEY_PAIR: usize = 0x51;

pub struct Session {
    pub handle: CkSessionHandle,
    pub key: CkObjectHandle,
    pub mech: CkMechanism
}

pub fn cryptoki_handler(id: usize, params: [usize; 6]) -> usize {
    unsafe{
    match id {
        C_INITIALIZE => {
            return crypto_impl::c_initialize();
        }
        C_FINALIZE => {
            return crypto_impl::c_finalize();
        }
        C_GET_SLOT_LIST => {
            let temp: CSlotLIst = *(params[0] as *const CSlotLIst);
            return  crypto_impl::c_get_slot_list(temp.cb, temp.cslot, temp.cup);
        }
        C_OPEN_SESSION => {
            let temp: Copen = *(params[0] as *const Copen);
            return crypto_impl::c_open_session(temp.csid, temp.cflag, temp.notify, temp.cshp);
        }
        C_CLOSE_SESSION => {
            return crypto_impl::c_close_session(params[0] as CkSessionHandle);
        }
        C_LOGIN => {
            let temp:Clogin  = *(params[0] as *const Clogin);
            return crypto_impl::c_login(temp.csh, temp.cut, temp.ccp, temp.cu);
        }
        C_LOGOUT => {
            return crypto_impl::c_logout(params[0] as CkSessionHandle);
        }
        C_ENCRYPT_INIT => {
            let temp: Einit = *(params[0] as *const Einit);
            return crypto_impl::c_decrypt_init(temp.csh, temp.cmp, temp.coh);
        }
        C_ENCRYPT => {
            let temp: Encrypt = *(params[0] as *const Encrypt);
            return crypto_impl::c_encrypt(temp.csh, temp.cbp1, temp.cu, temp.cbp2, temp.cup);
        }
        C_ENCRYPT_FINAL => {
            let temp: Dfinal = *(params[0] as *const Dfinal);
            return crypto_impl::c_encrypt_final(temp.csh, temp.cbp, temp.cup);
        }
        C_ENCRYPT_UPDATE => {
            let temp: Encrypt = *(params[0] as *const Encrypt);
            return crypto_impl::c_encrypt_update(temp.csh, temp.cbp1, temp.cu, temp.cbp2, temp.cup);
        }
        C_DECRYPT_INIT => {
            let temp: Dinit = *(params[0] as *const Dinit);
            return crypto_impl::c_decrypt_init(temp.csh, temp.cmp, temp.coh);
        }
        C_DECRYPT => {
            let temp: Decrypt = *(params[0] as *const Decrypt);
            return crypto_impl::c_decrypt(temp.csh, temp.cbp1, temp.cu1, temp.cbp2, temp.cu2);
        }
        C_DECRYPT_UPDATE => {
            let temp: Decrypt = *(params[0] as *const Decrypt);
            return crypto_impl::c_decrypt_update(temp.csh, temp.cbp1, temp.cu1, temp.cbp2, temp.cu2);
        }
        C_DECRYPT_FINAL => {
            let temp: Dfinal = *(params[0] as *const Dfinal);
            return crypto_impl::c_decrypt_final(temp.csh, temp.cbp, temp.cup);
        }
        C_GENERATE_KEY_PAIR => {
            let temp: Generate = *(params[0] as *const Generate);
            return crypto_impl::c_generate_key_pair(temp.csh, temp.cmp, temp.cap1, temp.cu1, temp.cap2, temp.cu2, temp.cohp1, temp.cohp2);
        }
        _ => {
            debug_error!("Unsupported Cryptoki call!");
            return 1;
        }
    }
}
}
