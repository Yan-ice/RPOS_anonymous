#include "pkcs11.h"
#include "mmk_syscall.h"


//general interface
static uint64_t C_INITIALIZE = 0x1;
static uint64_t C_FINALIZE = 0x2;
static uint64_t C_GET_FUNCTION_LIST = 0x4;

//slot management
static uint64_t C_GET_SLOT_LIST = 0x11;

//session management
static uint64_t C_OPEN_SESSION = 0x21;
static uint64_t C_CLOSE_SESSION = 0x22;
static uint64_t C_LOGIN = 0x27;
static uint64_t C_LOGOUT = 0x28;

//encrypt management
static uint64_t C_ENCRYPT_INIT = 0x31;
static uint64_t C_ENCRYPT = 0x32;
static uint64_t C_ENCRYPT_UPDATE = 0x33;
static uint64_t C_ENCRYPT_FINAL = 0x34;

//decrypt management
static uint64_t C_DECRYPT_INIT = 0x41;
static uint64_t C_DECRYPT = 0x42;
static uint64_t C_DECRYPT_UPDATE = 0x43;
static uint64_t C_DECRYPT_FINAL = 0x44;

//key management
static uint64_t C_GENERATE_KEY_PAIR = 0x51;


    CK_RV C_GetSlotList(CK_BBOOL only_token, CK_SLOT_ID_PTR slot_list, CK_ULONG_PTR count){
        uint64_t params[3] = {only_token, slot_list, count};
        return mmk_syscall_pkcs(C_GET_SLOT_LIST, params);
    }

    CK_RV C_Initialize(CK_VOID_PTR callback){
        void* p = callback;
        uint64_t params[1] = {callback};
        mmk_syscall_pkcs(C_INITIALIZE, params);
        ((void(*)(void))(callback))();
        return 0;
    }

    CK_RV C_Finalize(CK_VOID_PTR callback){
        uint64_t params[1] = {callback};
        mmk_syscall_pkcs(C_FINALIZE, params);
        ((void(*)(void))(callback))();
        return 0;
    }

    CK_RV C_GetFunctionList(CK_FUNCTION_LIST_PTR_PTR func_list){
        uint64_t params[1] = {func_list};
        return mmk_syscall_pkcs(C_GET_FUNCTION_LIST, params);
    }

    CK_RV C_OpenSession(CK_SLOT_ID slot, CK_FLAGS flags, CK_VOID_PTR ptr, CK_NOTIFY notify,
                        CK_SESSION_HANDLE_PTR session){
        uint64_t params[5] = {slot, flags, ptr, notify, session};
        return mmk_syscall_pkcs(C_OPEN_SESSION, params);
    }

    CK_RV C_CloseSession(CK_SESSION_HANDLE session){
        uint64_t params[1] = {session};
        return mmk_syscall_pkcs(C_CLOSE_SESSION, params);
    }


    CK_RV C_Login(CK_SESSION_HANDLE session, CK_USER_TYPE usr, CK_CHAR_PTR chars,  CK_ULONG val){
        uint64_t params[4] = {session, usr, chars, val};
        return mmk_syscall_pkcs(C_LOGIN, params);
    }

    CK_RV C_Logout(CK_SESSION_HANDLE session){
        uint64_t params[1] = {session};
        return mmk_syscall_pkcs(C_LOGOUT, params);
    }


    CK_RV C_Encrypt(CK_SESSION_HANDLE session, CK_BYTE_PTR in,  CK_ULONG in_len, CK_BYTE_PTR out,
                    CK_ULONG_PTR out_len){
        uint64_t params[5] = {session, in, in_len, out, out_len};
        return mmk_syscall_pkcs(C_ENCRYPT, params);
    }

    CK_RV C_EncryptFinal(CK_SESSION_HANDLE session, CK_BYTE_PTR last_out, CK_ULONG_PTR last_out_len){
        uint64_t params[3] = {session, last_out, last_out_len};
        return mmk_syscall_pkcs(C_ENCRYPT_FINAL, params);
    }

    CK_RV C_EncryptInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE key){
        uint64_t params[3] = {session, mech, key};
        return mmk_syscall_pkcs(C_ENCRYPT_INIT, params);
    }

    CK_RV C_EncryptUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR in,  CK_ULONG in_len, CK_BYTE_PTR out,
                          CK_ULONG_PTR out_len){
        uint64_t params[5] = {session, in, in_len, out, out_len};
        return mmk_syscall_pkcs(C_ENCRYPT_UPDATE, params);
    }

    CK_RV C_Decrypt(CK_SESSION_HANDLE session, CK_BYTE_PTR in, CK_ULONG in_len, CK_BYTE_PTR out,
                    CK_ULONG_PTR out_len){
        uint64_t params[5] = {session, in, in_len, out, out_len};
        return mmk_syscall_pkcs(C_DECRYPT, params);
    }

    CK_RV C_DecryptFinal(CK_SESSION_HANDLE session, CK_BYTE_PTR last_out, CK_ULONG_PTR last_out_len){
        uint64_t params[3] = {session, last_out, last_out_len};
        return mmk_syscall_pkcs(C_DECRYPT_FINAL, params);
    }

    CK_RV C_DecryptInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE key){
        uint64_t params[3] = {session, mech, key};
        return mmk_syscall_pkcs(C_DECRYPT_INIT, params);
    }

    CK_RV C_DecryptUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR in, CK_ULONG in_len, CK_BYTE_PTR out,
                    CK_ULONG_PTR out_len){
        uint64_t params[5] = {session, in, in_len, out, out_len};
        return mmk_syscall_pkcs(C_DECRYPT_UPDATE, params);
    }

    CK_RV C_GenerateKeyPair(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech,
                            CK_ATTRIBUTE_PTR pub_template,  CK_ULONG pub_attr_count, 
                            CK_ATTRIBUTE_PTR priv_template, CK_ULONG priv_attr_count,
                             CK_OBJECT_HANDLE_PTR pub_key, CK_OBJECT_HANDLE_PTR priv_key){
        uint64_t params[8] = {session, mech, pub_template, pub_attr_count, priv_template, priv_attr_count, pub_key, priv_key};
        return mmk_syscall_pkcs(C_GENERATE_KEY_PAIR, params);
        return 0;
    }




    CK_RV C_GetInfo(CK_INFO_PTR info_buf){
        //not implemented yet.
        return 0;
    }

    CK_RV C_CancelFunction(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_CloseAllSessions(CK_SLOT_ID slot_id){
        //not implemented yet.
        return 0;
    }


    CK_RV C_CopyObject(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj,
                       CK_ATTRIBUTE_PTR attr, CK_ULONG val, CK_OBJECT_HANDLE_PTR target){
        //not implemented yet.
        return 0;
    }

    CK_RV C_CreateObject(CK_SESSION_HANDLE session, CK_ATTRIBUTE_PTR attr, CK_ULONG val,
                         CK_OBJECT_HANDLE_PTR target){
        //not implemented yet.
        return 0;
    }


    CK_RV C_DecryptDigestUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR buf, CK_ULONG val,
                                CK_BYTE_PTR byte, CK_ULONG_PTR what){
        //not implemented yet.
        return 0;
    }

    CK_RV C_DecryptVerifyUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte, CK_ULONG val,
                                CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
                                    //not implemented yet.
        return 0;
                                }

    CK_RV C_DeriveKey(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE obj,
                      CK_ATTRIBUTE_PTR attr,  CK_ULONG val, CK_OBJECT_HANDLE_PTR obj_tar){
                        //not implemented yet.
        return 0;
                      }

    CK_RV C_DestroyObject(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj){
        //not implemented yet.
        return 0;
    }

    CK_RV C_Digest(CK_SESSION_HANDLE session, CK_BYTE_PTR byte,  CK_ULONG val, CK_BYTE_PTR byte_val,
                   CK_ULONG_PTR tar){
                    //not implemented yet.
        return 0;
                   }

    CK_RV C_DigestEncryptUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte,  CK_ULONG val,
                                CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
                                    //not implemented yet.
        return 0;
                                }

    CK_RV C_DigestFinal(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_DigestInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech){
        //not implemented yet.
        return 0;
    }

    CK_RV C_DigestKey(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj){
        //not implemented yet.
        return 0;
    }

    CK_RV C_DigestUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){

    }

    CK_RV C_FindObjects(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE_PTR obj_tar,  CK_ULONG val,
                        CK_ULONG_PTR tar){
                            //not implemented yet.
        return 0;
                        }

    CK_RV C_FindObjectsFinal(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_FindObjectsInit(CK_SESSION_HANDLE session, CK_ATTRIBUTE_PTR attr,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GenerateKey(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_ATTRIBUTE_PTR attr,
                         CK_ULONG val, CK_OBJECT_HANDLE_PTR obj_tar){
                            //not implemented yet.
        return 0;
                        }

    CK_RV C_GenerateRandom(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetAttributeValue(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj,
                              CK_ATTRIBUTE_PTR attr,  CK_ULONG val){
                                //not implemented yet.
        return 0;
                              }


    CK_RV C_GetFunctionStatus(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }


    CK_RV C_GetMechanismInfo(CK_SLOT_ID slot, CK_MECHANISM_TYPE mech,
                             CK_MECHANISM_INFO_PTR info){
                                //not implemented yet.
        return 0;
                             }

    CK_RV C_GetMechanismList(CK_SLOT_ID slot, CK_MECHANISM_TYPE_PTR mech, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetObjectSize(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetOperationState(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetSessionInfo(CK_SESSION_HANDLE session, CK_SESSION_INFO_PTR session_ptr){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetSlotInfo(CK_SLOT_ID slot, CK_SLOT_INFO_PTR slot_info){
        //not implemented yet.
        return 0;
    }


    CK_RV C_GetTokenInfo(CK_SLOT_ID slot, CK_TOKEN_INFO_PTR token){
        //not implemented yet.
        return 0;
    }

    CK_RV C_InitPIN(CK_SESSION_HANDLE session, CK_CHAR_PTR chars,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_InitToken(CK_SLOT_ID slot, CK_CHAR_PTR chars,  CK_ULONG val, CK_CHAR_PTR chars2){
        //not implemented yet.
        return 0;
    }

    CK_RV C_LoginUser(CK_SESSION_HANDLE session, CK_USER_TYPE usr,
                      CK_UTF8CHAR * u8char1,  CK_ULONG val1,
                      CK_UTF8CHAR * u8char2,  CK_ULONG val2){
                        //not implemented yet.
        return 0;
                      }


    CK_RV C_SeedRandom(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_SetAttributeValue(CK_SESSION_HANDLE session, CK_OBJECT_HANDLE obj,
                              CK_ATTRIBUTE_PTR attr,  CK_ULONG val){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_SetOperationState(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val,
                              CK_OBJECT_HANDLE obj, CK_OBJECT_HANDLE obj2){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_SetPIN(CK_SESSION_HANDLE session, CK_CHAR_PTR chars,  CK_ULONG val, CK_CHAR_PTR chars2,
                    CK_ULONG val2){
                    //not implemented yet.
        return 0;
                   }

    CK_RV C_Sign(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val, CK_BYTE_PTR byte_val2,
                 CK_ULONG_PTR tar){
                    //not implemented yet.
        return 0;
                 }

    CK_RV C_SignEncryptUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val,
                              CK_BYTE_PTR byte_val2, CK_ULONG_PTR tar){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_SignFinal(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_SignInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE obj){
        //not implemented yet.
        return 0;
    }

    CK_RV C_SignRecover(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val, CK_BYTE_PTR byte_val2,
                        CK_ULONG_PTR tar){
                            //not implemented yet.
        return 0;
                        }

    CK_RV C_SignRecoverInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech,
                            CK_OBJECT_HANDLE obj){
                                //not implemented yet.
        return 0;
                            }

    CK_RV C_SignUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_UnwrapKey(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE obj,
                      CK_BYTE_PTR byte_val,  CK_ULONG val, CK_ATTRIBUTE_PTR attr,  CK_ULONG val2,
                      CK_OBJECT_HANDLE_PTR obj_tar){
                        //not implemented yet.
        return 0;
                      }

    CK_RV C_Verify(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val, CK_BYTE_PTR byte_val2,
                    CK_ULONG val2){
                    //not implemented yet.
        return 0;
                   }

    CK_RV C_VerifyFinal(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_VerifyInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE obj){
        //not implemented yet.
        return 0;
    }

    CK_RV C_VerifyRecover(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val, CK_BYTE_PTR byte_val2,
                          CK_ULONG_PTR tar){
                            //not implemented yet.
        return 0;
                          }

    CK_RV C_VerifyRecoverInit(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech,
                              CK_OBJECT_HANDLE obj){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_VerifyUpdate(CK_SESSION_HANDLE session, CK_BYTE_PTR byte_val,  CK_ULONG val){
        //not implemented yet.
        return 0;
    }

    CK_RV C_WaitForSlotEvent(CK_FLAGS flags, CK_SLOT_ID_PTR slot, CK_VOID_PTR ptr){
        //not implemented yet.
        return 0;
    }

    CK_RV C_WrapKey(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech, CK_OBJECT_HANDLE obj,
                    CK_OBJECT_HANDLE obj2, CK_BYTE_PTR byte_val, CK_ULONG_PTR tar){
                        //not implemented yet.
        return 0;
                    }

    CK_RV C_GetInterfaceList(CK_INTERFACE_PTR interface, CK_ULONG_PTR tar){
        //not implemented yet.
        return 0;
    }

    CK_RV C_GetInterface(CK_UTF8CHAR_PTR charptr, CK_VERSION_PTR ver,
                         CK_INTERFACE_PTR_PTR interface, CK_FLAGS flags){
                            //not implemented yet.
        return 0;
                         }


    CK_RV C_SessionCancel(CK_SESSION_HANDLE session, CK_FLAGS flags){
        //not implemented yet.
        return 0;
    }

    CK_RV C_MessageEncryptInit(CK_SESSION_HANDLE session,
                               CK_MECHANISM * mech, CK_OBJECT_HANDLE obj){
                                //not implemented yet.
        return 0;
                               }

    CK_RV C_EncryptMessage(CK_SESSION_HANDLE session,
                           void * ptr,  CK_ULONG val,
                           CK_BYTE * mes1,  CK_ULONG val1,
                           CK_BYTE * mes2,  CK_ULONG val2,
                           CK_BYTE * mes3, CK_ULONG * val3){
                            //not implemented yet.
        return 0;
                           }

    CK_RV C_EncryptMessageBegin(CK_SESSION_HANDLE session,
                                void * ptr,  CK_ULONG val,
                                CK_BYTE *byte_v,
                                 CK_ULONG val2){
                                    //not implemented yet.
        return 0;
                                }

    CK_RV C_EncryptMessageNext(CK_SESSION_HANDLE session,
                               void* ptr,  CK_ULONG val,
                               CK_BYTE * byte_ptr,
                                CK_ULONG val2,
                               CK_BYTE * byte_ptr2,
                                CK_ULONG * val_ptr,
                                CK_ULONG val3){
                                //not implemented yet.
        return 0;
                               }

    CK_RV C_MessageEncryptFinal(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_MessageDecryptInit(CK_SESSION_HANDLE session,
                           CK_MECHANISM * mech, CK_OBJECT_HANDLE obj){
                            //not implemented yet.
        return 0;
                           }

    CK_RV C_DecryptMessage(CK_SESSION_HANDLE session,
                           void * ptr,  CK_ULONG val,
                           CK_BYTE * byte_ptr,  CK_ULONG val1,
                           CK_BYTE * byte_ptr2,  CK_ULONG val2,
                           CK_BYTE * byte_ptr3, CK_ULONG * valo){
                            //not implemented yet.
        return 0;
                           }

    CK_RV C_DecryptMessageBegin(CK_SESSION_HANDLE session,
                                void * ptr,  CK_ULONG val1,
                                CK_BYTE * byte_ptr,
                                 CK_ULONG val){
                                    //not implemented yet.
        return 0;
                                }

    CK_RV C_DecryptMessageNext(CK_SESSION_HANDLE session,
                               void * ptr,  CK_ULONG val1,
                               CK_BYTE * byte_ptr1,
                                CK_ULONG val,
                               CK_BYTE * byte_ptr,
                                CK_ULONG * val2,
                               CK_FLAGS flags){
                                //not implemented yet.
        return 0;
                               }

    CK_RV C_MessageDecryptFinal(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_MessageSignInit(CK_SESSION_HANDLE session,
                            CK_MECHANISM * mech, CK_OBJECT_HANDLE obj){
                                //not implemented yet.
        return 0;
                            }

    CK_RV C_SignMessage(CK_SESSION_HANDLE session,
                        void* ptr,  CK_ULONG val,
                        CK_BYTE * byte_ptr,  CK_ULONG val2,
                        CK_BYTE * byte_ptr2, CK_ULONG * uptr){
                            //not implemented yet.
        return 0;
                        }

    CK_RV C_SignMessageBegin(CK_SESSION_HANDLE session,
                             void * ptr,  CK_ULONG val){
                                //not implemented yet.
        return 0;
                             }

    CK_RV C_SignMessageNext(CK_SESSION_HANDLE session,
                            void* ptr,  CK_ULONG val,
                            CK_BYTE * byte_ptr1,  CK_ULONG val1,
                            CK_BYTE * byte_ptr2, CK_ULONG * out){
                                //not implemented yet.
        return 0;
                            }

    CK_RV C_MessageSignFinal(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_MessageVerifyInit(CK_SESSION_HANDLE session,
                              CK_MECHANISM * mech, CK_OBJECT_HANDLE obj){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_VerifyMessage(CK_SESSION_HANDLE session,
                          void * ptr,  CK_ULONG val,
                          CK_BYTE * byte_ptr1,  CK_ULONG val1,
                          CK_BYTE * byte_ptr2,  CK_ULONG val2){
                            //not implemented yet.
        return 0;
                          }

    CK_RV C_VerifyMessageBegin(CK_SESSION_HANDLE session,
                               void* ptr,  CK_ULONG val){
                                //not implemented yet.
        return 0;
                               }

    CK_RV C_VerifyMessageNext(CK_SESSION_HANDLE session,
                              void* ptr,  CK_ULONG val,
                              CK_BYTE * byte_ptr1,  CK_ULONG val1,
                              CK_BYTE * byte_ptr2,  CK_ULONG val2){
                                //not implemented yet.
        return 0;
                              }

    CK_RV C_MessageVerifyFinal(CK_SESSION_HANDLE session){
        //not implemented yet.
        return 0;
    }

    CK_RV C_IBM_ReencryptSingle(CK_SESSION_HANDLE session, CK_MECHANISM_PTR mech1,
                                CK_OBJECT_HANDLE obj1, CK_MECHANISM_PTR mech2,
                                CK_OBJECT_HANDLE obj2, CK_BYTE_PTR byte_val,
                                 CK_ULONG val, CK_BYTE_PTR byte_val2, CK_ULONG_PTR tar){
                                    //not implemented yet.
        return 0;
                                }