#include <stdint.h>
#include "fs.h"
#include "log.h"

typedef struct  {
  char name[32];
} app_name;

extern uint64_t _app_num;
extern uint64_t _app_names;
extern uint64_t _app_datas;


uint64_t mem_load_pgms(char* name, uint8_t* load_data){
  // info("load %s app from %d apps\n", name, _app_num);
  uint8_t* temp = (uint8_t *)&_app_names;
  for(int a = 0;a<_app_num;a++){
    if(strcmp(name, (char*) temp) == 0 ){
      uint64_t* si = &_app_datas;
      uint64_t siz = si[a+1] - si[a];
      memcpy(load_data, (uint8_t*)si[a], siz);
      // info("find app %s in memory\n", name);
      return siz;
    }
    temp += (strlen((char *)temp) + 1);
  }
  // error("app %s not found in memory.\n", name);
  return 0;
}