use spin::{Mutex, RwLock};
use alloc::collections::VecDeque;
use lazy_static::lazy_static;
use alloc::sync::Arc;
use super::{ioctl::Termios};
use crate::fs::File;
use crate::util::mm_util::{UserBuffer,copy_object};
use super::ioctl::*;

lazy_static! {
    pub static ref TTY: Arc<TtyINode> = Arc::new(TtyINode::default());
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Winsize {
     ws_row: u16,
     ws_col: u16,
     xpixel: u16,
     ypixel: u16,
}

impl Default for Winsize{
    fn default() -> Self {
        Self{
            ws_row:24,
            ws_col:80,
            xpixel:0,
            ypixel:0,
        }
    }
}


#[derive(Default)]
pub struct TtyINode {
    /// foreground process group
    foreground_pgid: RwLock<i32>,
    buf: Mutex<VecDeque<u8>>,
    //eventbus: Mutex<EventBus>,
    winsize: RwLock<Winsize>,
    termios: RwLock<Termios>,
}

// TODO: independ of rust sbi
impl File for TtyINode {
    fn readable(&self) -> bool {
        false
    }
    
    fn writable(&self) -> bool {
        false
    }
    
    fn read(&self, buf: UserBuffer) -> usize {
        0
    }
    
    fn write(&self, buf: UserBuffer) -> usize {
        0
    }

    fn ioctl(&self, cmd: u32, arg: usize)->isize{
        //debug_os!("ioctl: cmd={}; arg={:X}", cmd, arg);
        let cmd = cmd as usize;
        match cmd {
            TIOCGPGRP => {
                //let argp = arg as *mut i32; // pid_t
                let argp =  *self.foreground_pgid.read();
                copy_object(&argp as *const i32, arg as *mut i32);
                //copy_to_user(arg, &argp as *const i32);
                0
            }
            TIOCSPGRP => {
                //let fpgid = unsafe { *(arg as *const i32) };
                let mut argp:i32 = 0;
                copy_object(arg as *const i32, &mut argp as *mut i32);
                //copy_from_user(&mut argp as *mut i32, arg);
                *self.foreground_pgid.write() = argp;
                0
            }
            TIOCGWINSZ => {
                let winsize = Winsize::default();
                //debug_os!("size = {}", size);
                copy_object(&winsize as *const Winsize,arg as *mut Winsize);
                //copy_to_user(arg, &winsize as *const Winsize);
                0
            }
            TCGETS => {
                let termois  = *self.termios.read();
                copy_object(&termois as *const Termios,arg as *mut Termios);
                //copy_to_user(arg, &termois as *const Termios);
                0
            }
            TCSETS => {
                let mut termios = Termios::default();
                copy_object(arg as *const Termios, &mut termios as *mut Termios);
                //copy_from_user(&mut termios as *mut Termios, arg);
                *self.termios.write() = termios;
                0
            }
            /* WARNING: 仅临时handle */
            RTC_RD_TIME=> {
                0
            }
            _ => -1,// not support
        }
    }

}