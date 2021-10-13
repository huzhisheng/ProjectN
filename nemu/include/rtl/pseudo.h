#ifndef __RTL_PSEUDO_H__
#define __RTL_PSEUDO_H__

#ifndef __RTL_RTL_H__
#error "Should be only included by <rtl/rtl.h>"
#endif

/* RTL pseudo instructions */

static inline def_rtl(li, rtlreg_t* dest, const rtlreg_t imm) {
  rtl_addi(s, dest, rz, imm);
}

static inline def_rtl(mv, rtlreg_t* dest, const rtlreg_t *src1) {
  rtl_addi(s, dest, src1, 0);
}

static inline def_rtl(not, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- ~src1
  // TODO:
  rtl_addi(s, dest, rz, ~(*src1));
}

static inline def_rtl(neg, rtlreg_t *dest, const rtlreg_t* src1) {
  // dest <- -src1
  // TODO:
  rtl_addi(s, dest, rz, -(*src1));
}

static inline def_rtl(sext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- signext(src1[(width * 8 - 1) .. 0])
  // TODO:
  word_t val = *src1;
  switch (width) {
    case 4: *dest = (sword_t)(int32_t)val; return;
    case 1: *dest = (sword_t)( int8_t)val; return;
    case 2: *dest = (sword_t)(int16_t)val; return;
    IFDEF(CONFIG_ISA64, case 8: *dest = (sword_t)(int64_t)val; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

static inline def_rtl(zext, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- zeroext(src1[(width * 8 - 1) .. 0])
  // TODO:
  word_t val = *src1;
  switch (width) {
    case 4: *dest = (sword_t)(uint32_t)val; return;
    case 1: *dest = (sword_t)( uint8_t)val; return;
    case 2: *dest = (sword_t)(uint16_t)val; return;
    IFDEF(CONFIG_ISA64, case 8: *dest = (sword_t)(uint64_t)val; return);
    IFDEF(CONFIG_RT_CHECK, default: assert(0));
  }
}

static inline def_rtl(msb, rtlreg_t* dest, const rtlreg_t* src1, int width) {
  // dest <- src1[width * 8 - 1]
  // TODO:
  word_t mask = 1 << (width * 8 - 1);
  *dest = (*src1) & mask;
}
#endif
