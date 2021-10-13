def_EHelper(auipc) {
  rtl_li(s, ddest, id_src1->imm + s->pc);
}

def_EHelper(lb){
  rtl_lm(s, s0, dsrc1, id_src2->simm, 1);
  rtl_sext(s, ddest, s0, 1);
}

def_EHelper(addi){
  rtl_addi(s, ddest, dsrc1, id_src2->simm);
}

def_EHelper(jal){
  rtl_addi(s, ddest, (&s->pc), 4);  //! 存pc+4到rd
  s->dnpc = s->pc + id_src1->simm;  //! 修改dnpc
}

def_EHelper(jalr){
  rtl_addi(s, s0, &(s->pc), 4);                     //! t=pc+4;
  sword_t simm = (*(dsrc1) + id_src2->simm) & (~1);
  rtl_mv(s, &(s->dnpc), (rtlreg_t*)(&simm));        //! pc=(x[rs1]+sext(offset))&~1
  rtl_mv(s, ddest, s0);                             //! x[rd]=t
}