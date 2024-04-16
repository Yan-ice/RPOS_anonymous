
use core::convert::TryInto;

use aes_gcm::AeadCore;
use aes_gcm::KeyInit;
use tiny_keccak::Hasher;
use tiny_keccak::Sha3;
use aes_gcm::{Aes256Gcm, Nonce, Key, aead::{Aead, generic_array, OsRng}};
use alloc::sync::Arc;
use spin::Mutex;
use lazy_static::lazy_static;
use alloc::format;

use super::cryptoconfig::*;
use super::state;
use super::state::*;

pub fn c_initialize() -> usize{
    return 0;
}

pub fn c_finalize() -> usize{
    return 0;
}

pub fn c_get_slot_list(cb: CkBbool, cslot: CkSlotIdPtr, cup: CkUlongPtr) -> usize{
    return 0;
}

pub fn c_open_session(csid: CkSlotId, cflag: CkFlags, notify: fn(), cshp: CkSessionHandlePtr) -> usize{
    // let ans = SESSION_MANAGER.session_alloc();
    unsafe{
        *cshp = state::new_session();
    }
    return 0;
}

pub fn c_close_session(csh: CkSessionHandle) -> usize{
    state::del_session(csh);
    return 0;
}

pub fn c_login(h_session: CkSessionHandle, user_type: CkUserType, p_pin: CkCharPtr, ul_pinen: CkUlong) -> usize{
    return 0;
}

pub fn c_logout(csh: CkSessionHandle)-> usize{
    return 0;
}

pub fn c_encrypt_init(csh: CkSessionHandle, cmp: CkMechanismPtr, coh: CkObjectHandle) -> usize{
    unsafe{
        if let Some(mut session) = get_session(csh) {
            session.enc.valid = true;
            session.enc.mech = *cmp;
            session.enc.key = coh;
        }
    }
    return 0;
}

pub fn c_encrypt(csh: CkSessionHandle, p_data: CkBytePtr, 
    ul_data_len: CkUlong, p_encrypted_data: CkBytePtr, p_encrypted_data_len: CkUlongPtr) -> usize{
    
    c_encrypt_update(csh, p_data, ul_data_len, p_encrypted_data, p_encrypted_data_len);
    c_encrypt_final(csh, p_encrypted_data, p_encrypted_data_len);

    return 0;
}

pub fn c_encrypt_update(csh: CkSessionHandle, p_data: CkBytePtr, 
    ul_data_len: CkUlong, p_encrypted_data: CkBytePtr, p_encrypted_data_len: CkUlongPtr) -> usize{
        if let Some(session) = get_session(csh) {
            if ul_data_len > 32 {
                panic!("too long data!");
            }
    
            match session.enc.mech.mechanism {
                E_SHA3 => {
                    let mut hasher = Sha3::v256();
                    unsafe{
                        let start: usize= p_data as usize;
                        for i in 0..ul_data_len{
                            let temp = *((i as usize + start) as usize as *const usize);
                            hasher.update((&format!("{}", temp)).as_bytes());
                        }
                        let mut result = [0u8; 32];
                        hasher.finalize(&mut result);
                
                        let mut hash = [0u8; 8];
                        hash.copy_from_slice(&result[..8]);
                        for i in 0..32{
                            *p_encrypted_data.add(i) = result[i];
                        }
                        *p_encrypted_data_len = 32;
                    }
    
                }
                E_AES256 =>{
                    unsafe{
                    let key_data = get_object(session.enc.key).unwrap().content;
                    let key = Key::<Aes256Gcm>::from_slice(key_data.as_slice());
                    let cipher = Aes256Gcm::new(&key);
                    let nonce = Nonce::default();
                    let mut input = [0u8; 32];
                    for i in 0..32{
                        input[i] = *p_data.add(i);
                    }
                    let ans = cipher.encrypt(&nonce, input.as_ref());
                    match ans{
                        Ok(temp) => {
                            for i in 0..32{
                                *p_encrypted_data.add(i) = temp[i];
                            } 
                            *p_encrypted_data_len = temp.len() as u32;
                        }
                        _ => {
                            panic!("error");
                        }
                    }
                    }
                }
                _ => {
                    panic!("unexcepted mechanism");
                }
            }
        } 
        return 0;
}

pub fn c_encrypt_final(csh: CkSessionHandle, cbp: CkBytePtr, clp: CkUlongPtr) -> usize{
    if let Some(mut session) = get_session(csh){
        session.enc.valid = false;
        session.enc.key = 0;
        update_session(session);
    }
    return 0;

}

pub fn c_decrypt_init(csh: CkSessionHandle, cmp: CkMechanismPtr, coh: CkObjectHandle) -> usize{
    
    unsafe{
        if let Some(mut session) = get_session(csh) {
            session.dec.valid = true;
            session.dec.mech = *cmp;
            session.dec.key = coh;
            update_session(session)
        }
    }
    
    return 0;
}

pub fn c_decrypt(csh: CkSessionHandle, p_data: CkBytePtr, 
    ul_data_len: CkUlong, p_decrypted_data: CkBytePtr, p_decrypted_data_len: CkUlongPtr) -> usize{
    
    c_decrypt_update(csh, p_data, ul_data_len, p_decrypted_data, p_decrypted_data_len);
    c_decrypt_final(csh, p_decrypted_data, p_decrypted_data_len);
    return 0;

}

pub fn c_decrypt_update(csh: CkSessionHandle, p_data: CkBytePtr, 
    ul_data_len: CkUlong, p_decrypted_data: CkBytePtr, p_decrypted_data_len: CkUlongPtr) -> usize{
    
    if ul_data_len > 32 {
        panic!("too long data!");
    }

    if let Some(session) = get_session(csh){
        match session.dec.mech.mechanism {
            E_SHA3 => {
                panic!("Cannot decrypt!"); 
            }
            E_AES256 => {
                 unsafe{
                    let key_data = get_object(session.dec.key).unwrap().content;
                    let key = Key::<Aes256Gcm>::from_slice(key_data.as_slice());
                    let cipher = Aes256Gcm::new(&key);

                    let nonce = Nonce::default();
                    let mut input = [0u8; 32];
                    for i in 0..32{
                        input[i] = *p_data.add(i);
                    }
                    let ans = cipher.encrypt(&nonce, input.as_ref());
                    match ans{
                        Ok(temp) => {
                            for i in 0..32{
                                *p_decrypted_data.add(i) = temp[i];
                            } 
                            *p_decrypted_data_len = temp.len() as u32;
                        }
                        _ => {
                            panic!("error");
                        }
                    }
                 }
            }
            _=>{
                panic!("Unsupported!");
            }

        }
    }
    return 0;
}

pub fn c_decrypt_final(csh: CkSessionHandle, cbp: CkBytePtr, cup: CkUlongPtr) -> usize{
    if let Some(mut session) = get_session(csh){
        session.dec.valid = false;
        session.dec.key = 0;
        update_session(session);
    }
    return 0;
}

pub fn c_generate_key_pair(h_session: CkSessionHandle, p_mechanism: CkMechanismPtr, 
            p_public_key_template: CkAttributeType, ul_public_key_attribute_count: CkUlong, 
            p_private_key_template: CkAttributeType, ul_private_key_attribute_count: CkUlong, 
            ph_public_key: CkObjectHandlePtr, ph_private_key: CkObjectHandlePtr) -> usize{
    let mut seed: [u8; 32] = [0; 32];
    for i in 0..32{
        let mut time:usize = 0;
        unsafe{
            core::arch::asm!(
                "rdtime a0",
                inout("a0") time
            );
        }
        seed[i] = (time % 255) as u8;
    }
    
    unsafe{
        let key = Aes256Gcm::generate_key(OsRng);
        let key_data: &[u8] = key.as_slice();
        *ph_public_key = new_object(key_data.to_vec());
        *ph_private_key = new_object(key_data.to_vec());
    }
    return 0;
}
