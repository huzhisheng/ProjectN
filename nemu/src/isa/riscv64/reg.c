#include <isa.h>
#include "local-include/reg.h"

const char *regs[] = {
  "$0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
  "s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
  "a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
  "s8", "s9", "s10", "s11", "t3", "t4", "t5", "t6"
};

void isa_reg_display() {
  int i;
  for(i = 0; i < 32; i++){
    if(i % 8 == 0){
      printf("\n");
    }
    printf("%016lx ", gpr(i));
  }
  printf("\n");
}

//! 假设寄存器是x0-x31
word_t isa_reg_str2val(const char *s, bool *success) {
  if(*s != '$'){
    *success = false;
    return -1;
  }
  const char *temp_s = s+2;
  int reg_num = atoi(temp_s);
  if(reg_num < 0 || reg_num > 31){
    *success = false;
    return -1;
  }
  return gpr(reg_num);
}
