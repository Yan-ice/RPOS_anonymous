use super::cryptoconfig::*;
use aes_gcm::aead::generic_array::GenericArray;
use lazy_static::lazy_static;
use alloc::{sync::Arc, vec::Vec};
use spin::Mutex;
use aes_gcm::AeadCore;
use aes_gcm::KeyInit;
use tiny_keccak::Hasher;
use tiny_keccak::Sha3;
use aes_gcm::{Aes256Gcm, Nonce, Key, aead::{Aead, generic_array, OsRng}};

lazy_static!{
    static ref COUNTER: Arc<Mutex<Counter>> = Arc::new(Mutex::new(Counter { cnt: 0}));
    pub static ref SESSION_LIST: Arc<Mutex<Vec<Session>>> = Arc::new(Mutex::new(Vec::new()));
    pub static ref OBJECT_LIST: Arc<Mutex<Vec<CkObject>>> = Arc::new(Mutex::new(Vec::new()));
}

struct Counter {
    cnt: u32
}

impl Counter {
    pub fn get_next(&mut self) -> u32 {
        self.cnt = self.cnt + 1;
        self.cnt
    }
}

pub fn new_session() -> CkSessionHandle{
    let ses = Session::new();
    SESSION_LIST.lock().push(ses);
    return ses.handle;
}
pub fn get_session(handle: CkSessionHandle) -> Option<Session> {
    for i in SESSION_LIST.lock().clone().into_iter() {
        if i.handle == handle {
            return Some(i);
        }
    }
    return None;
}
pub fn update_session(session: Session){
    del_session(session.handle);
    SESSION_LIST.lock().push(session);
}

pub fn del_session(handle: CkSessionHandle){
    SESSION_LIST.lock().retain(|x| x.handle != handle); 
}


pub fn new_object(data: Vec<u8>) -> CkObjectHandle {
    let obj = CkObject::new(data);
    OBJECT_LIST.lock().push(obj);
    return obj.handle;
}
pub fn get_object(handle: CkObjectHandle) -> Option<CkObject> {
    for i in OBJECT_LIST.lock().clone().into_iter() {
        if i.handle == handle {
            return Some(i);
        }
    }
    return None;
}
pub fn del_object(handle: CkObjectHandle) {
    OBJECT_LIST.lock().retain(|x| x.handle != handle); 
}

#[derive(Clone, Copy)]
pub struct CkObject {
    pub handle: CkObjectHandle,
    pub content: [u8; 256],
    pub content_size: u8
}

impl CkObject {
    pub fn new(data: Vec<u8>) -> Self {
        let mut cko: CkObject = CkObject { 
            handle: COUNTER.lock().get_next(), 
            content: [0; 256], 
            content_size: data.len() as u8 };
        for i in 0..data.len(){
            cko.content[i] = data[i];
        }
        return cko;
    }
}



#[derive(Clone, Copy)]
pub struct Session {
    pub handle: CkSessionHandle,
    pub enc: CkContext,
    pub dec: CkContext,
}

impl Session {
    pub fn new() -> Self {
        return Session { 
            handle: COUNTER.lock().get_next(), 
            enc: CkContext::new(), 
            dec: CkContext::new()
        };
    }
}

#[derive(Clone, Copy)]
pub struct CkContext {
    pub valid: bool,
    pub key: CkObjectHandle,
    pub mech: CkMechanism
}

impl CkContext {
    pub fn new() -> Self{
        return CkContext { valid: false, key: 0, mech: CkMechanism { mechanism: 0, p_parameter: 0, ul_parameter_len: 0 } };
    }
}
