#include "stdio.h"
#include "mmk_syscall.h"
#include "pkcs11.h"

#define E_SHA3 1
#define E_AES256 2

int callback(){
  printf("this is callback\n");
}
int main() {

  printf("\n[mmk echo test]\n");

  mmk_syscall_echo(233);

  printf("\n[mmk measure test]\n");

  int64_t mval = mmk_syscall_measure();
  printf("measure result: %llx\n", mval);

  printf("\n[mmk CSP test]\n");

  C_Initialize(callback);

  CK_SESSION_HANDLE session;
  C_OpenSession(0,0,NULL,callback,&session);

  CK_MECHANISM mech = {E_AES256,0,0};
  CK_ATTRIBUTE attr = {0,0,0};
  CK_OBJECT_HANDLE pub_key, priv_key;
  C_GenerateKeyPair(session,&mech,&attr,0,&attr,0,&pub_key,&priv_key);

  uint8_t* str = "secret message";
  int data_len = strlen(str);
  uint8_t* out_buf[512];
  uint64_t output_len = 0;
  C_EncryptInit(session,&mech, pub_key);
  C_Encrypt(session,str,data_len,out_buf,&output_len);

  uint8_t* recover_buf[512];
  uint64_t recover_len = 0;
  C_DecryptInit(session,&mech, priv_key);
  C_Decrypt(session,out_buf,output_len,recover_buf,&recover_len);

  printf("Enc & Dec result: %s\n",recover_buf);
  return 0;
}
