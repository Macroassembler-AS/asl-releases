/* codeimp16.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Codegenerator National IMP-16/PACE                                        */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "bpemu.h"
#include <ctype.h>
#include <string.h>

#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "headids.h"
#include "literals.h"

#include "codeimp16.h"

/*-------------------------------------------------------------------------*/
/* Types */

typedef struct
{
  const char *p_name;
  Word code;
} symbol_t;

typedef enum
{
  e_cpu_flag_none = 0,
  e_cpu_flag_core_pace = 1 << 0,
  e_cpu_flag_imp16_ext_instr = 1 << 1,
  e_cpu_flag_imp16_ien_status = 1 << 2
} cpu_flags_t;

/* NOTE: Only 4 bits are available for flags in the encoded machine code
   for decode_mem().  Do NOT add more flags here! */

typedef enum
{
  e_inst_flag_allow_indirect = 1 << 0,
  e_inst_flag_r01 = 1 << 1,
  e_inst_flag_r0 = 1 << 2,
  e_inst_flag_skip = 1 << 3,
  e_inst_flag_all = 0x0f
} inst_flag_t;

typedef struct
{
  const char *p_name;
  cpu_flags_t flags;
} cpu_props_t;

#ifdef __cplusplus
# include "codeimp16.hpp"
#endif

/*-------------------------------------------------------------------------*/
/* Locals */

static symbol_t *conditions, *status_flags;

static const cpu_props_t *p_curr_cpu_props;
static Boolean last_was_skip, this_was_skip;
static LongInt bps_val;

/*-------------------------------------------------------------------------*/
/* Register Symbols */

/*!------------------------------------------------------------------------
 * \fn     decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
 * \brief  check whether argument is a CPU register
 * \param  p_arg argument to check
 * \param  p_result numeric register value if yes
 * \param  p_size returns register size
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
{
  switch (strlen(p_arg))
  {
    case 3:
      if ((as_toupper(*p_arg) == 'A')
       && (as_toupper(p_arg[1]) == 'C')
       && isdigit(p_arg[2])
       && (p_arg[2] < '4'))
      {
        *p_result = p_arg[2] - '0';
        *p_size = eSymbolSize16Bit;
        return True;
      }
      break;
    default:
      break;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_imp16(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - IMP-16 variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void dissect_reg_imp16(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize16Bit:
      as_snprintf(p_dest, dest_size, "AC%u", (unsigned)(value & 3));
      break;
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result)
 * \brief  check whether argument is a CPU register or register alias
 * \param  p_arg argument to check
 * \param  p_result numeric register value if yes
 * \param  must_be_reg argument is expected to be a register
 * ------------------------------------------------------------------------ */

static tRegEvalResult decode_reg(const tStrComp *p_arg, Word *p_result)
{
  tRegDescr reg_descr;
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;

  /* built-in register */

  if (decode_reg_core(p_arg->str.p_str, p_result, &eval_result.DataSize))
  {
    reg_descr.Reg = *p_result;
    reg_eval_result = eIsReg;
    eval_result.DataSize = eSymbolSize16Bit;
  }

  /* (register) symbol */

  else
  {
    reg_eval_result = EvalStrRegExpressionAsOperand(p_arg, &reg_descr, &eval_result, eSymbolSizeUnknown, False);

    /* always try numeric value for register */

    if (eIsNoReg == reg_eval_result)
    {
      Boolean ok;

      reg_descr.Reg = EvalStrIntExpression(p_arg, UInt2, &ok);
      reg_eval_result = ok ? eIsReg : eRegAbort;
      if (ok) eval_result.DataSize = eSymbolSize16Bit;
    }
  }

  if (reg_eval_result == eIsReg)
  {
    if (eval_result.DataSize != eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, p_arg);
      reg_eval_result = eIsNoReg;
    }
  }

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  return reg_eval_result;
}

/*---------------------------------------------------------------------------*/
/* Address Parsing */

/*!------------------------------------------------------------------------
 * \fn     decode_mem_arg(const tStrComp *p_arg, Word *p_result, Boolean allow_indirect)
 * \brief  decode memory argument
 * \param  p_arg source argument
 * \param  p_result encoded addressing mode (one word mode)
 * \param  allow_indirect indirect mode (@) allowed?
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_mem_arg(const tStrComp *p_arg, Word *p_result, Boolean allow_indirect)
{
  tStrComp arg;
  int split_pos, arg_len;
  LongInt disp;
  LongWord addr;
  tEvalResult eval_result;
  Boolean force_pcrel = False, is_pcrel;
  Boolean base_ok, disp_ok;

  *p_result = 0x0000;

  StrCompRefRight(&arg, p_arg, 0);
  if (*arg.str.p_str == '@')
  {
    if (!allow_indirect)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return False;
    }
    StrCompIncRefLeft(&arg, 1);
    *p_result |= 0x1000;
  }

  /* The (emulated) immediate mode: We use the common literal
     mechanism, and as soon as we have the literal's name, branch
     to the common code handling absolute addresses: */

  if (*arg.str.p_str == '#')
  {
    tEvalResult eval_result;
    Word value;
    Boolean critical;
    String l_str;

    value = EvalStrIntExpressionOffsWithResult(&arg, 1, Int16, &eval_result);
    if (!eval_result.OK)
      return False;
    critical = mFirstPassUnknown(eval_result.Flags) || mUsesForwards(eval_result.Flags);

    StrCompMkTemp(&arg, l_str, sizeof(l_str));    
    literal_make(&arg, NULL, value, eSymbolSize16Bit, critical);
    force_pcrel = True;
    goto parse_abs;
  }

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp reg_arg;
    Word xreg;

    StrCompSplitRef(&arg, &reg_arg, &arg, &arg.str.p_str[split_pos]);
    KillPostBlanksStrComp(&arg);
    KillPrefBlanksStrCompRef(&reg_arg);
    StrCompShorten(&reg_arg, 1);
    KillPostBlanksStrComp(&reg_arg);

    /* Allow addr(pc) to explicitly force PC-relative addressing */

    if (!as_strcasecmp(reg_arg.str.p_str, "PC"))
    {
      force_pcrel = True;
      goto parse_abs;
    }
    if (!decode_reg(&reg_arg, &xreg)
      || (xreg < 2))
      return False;
    if (!*arg.str.p_str)
    {
      disp = 0;
      eval_result.OK = True;
    }
    else
      disp = EvalStrIntExpression(&arg, SInt8, &eval_result.OK);
    if (!eval_result.OK)
      return False;

    *p_result |= (xreg << 8) | (disp & 0xff);
    return True;
  }

parse_abs:
  addr = EvalStrIntExpressionWithResult(&arg, UInt16, &eval_result);
  if (!eval_result.OK)
    return False;
  disp = addr - (EProgCounter() + 1);
  disp_ok = (disp >= -128) && (disp < 127);
  base_ok = bps_val
          ? ((addr <= 127) || (addr >= 0xff80u))
          : (addr < 256);

  /* For addresses in the CODE segment, preferrably use PC-relative
     addressing.  For all other addresses/values, preferrably use
     absolute addressing: */

  if (force_pcrel)
    is_pcrel = True;
  else if (eval_result.AddrSpaceMask & (1 << SegCode))
    is_pcrel = disp_ok || !base_ok;
  else
    is_pcrel = !base_ok;

  if (is_pcrel)
  {
    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
     && !ChkRangePos(disp, -128, 127, &arg))
      return False;

    *p_result |= (1 << 8) | (disp & 0xff);
  }
  else
  {
    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
     && !ChkRangePos(addr,
                     bps_val ? ((addr & 0x8000ul) ? 0xff80ul : 0) : 0,
                     bps_val ? ((addr & 0x8000ul) ? 0xfffful : 127) : 255,
                     &arg))
          return False;

    *p_result |= (0 << 8) | (addr & 0xff);
  }

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_long_mem_arg(const tStrComp *p_arg, Word *p_result, Word *p_disp, IntType mem_type, Boolean allow_pcrel)
 * \brief  decode long (two word) memory argument
 * \param  p_arg source argument
 * \param  p_result encoded addressing mode (two word mode)
 * \param  p_disp displacement word
 * \param  mem_type address range for absolute addresses
 * \param  allow_pcrel allow PC-relative addressing?
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_long_mem_arg(const tStrComp *p_arg, Word *p_result, Word *p_disp, IntType mem_type, Boolean allow_pcrel)
{
  tStrComp arg;
  int split_pos, arg_len;
  LongWord addr;
  LongInt disp;
  tEvalResult eval_result;
  Boolean force_pcrel = False, is_pcrel;

  *p_result = *p_disp = 0x0000;

  StrCompRefRight(&arg, p_arg, 0);
  if (*arg.str.p_str == '@')
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }

  /* The (emulated) immediate mode: We use the common literal
     mechanism, and as soon as we have the literal's name, branch
     to the common code handling absolute addresses: */

  if (*arg.str.p_str == '#')
  {
    tEvalResult eval_result;
    Word value;
    Boolean critical;
    String l_str;

    if (!allow_pcrel)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &arg);
      return False;
    }

    value = EvalStrIntExpressionOffsWithResult(&arg, 1, Int16, &eval_result);
    if (!eval_result.OK)
      return False;
    critical = mFirstPassUnknown(eval_result.Flags) || mUsesForwards(eval_result.Flags);

    StrCompMkTemp(&arg, l_str, sizeof(l_str));    
    literal_make(&arg, NULL, value, eSymbolSize16Bit, critical);
    force_pcrel = True;
    goto parse_abs;
  }

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp reg_arg;
    Word xreg;

    StrCompSplitRef(&arg, &reg_arg, &arg, &arg.str.p_str[split_pos]);
    KillPostBlanksStrComp(&arg);
    KillPrefBlanksStrCompRef(&reg_arg);
    StrCompShorten(&reg_arg, 1);
    KillPostBlanksStrComp(&reg_arg);

    if (!as_strcasecmp(reg_arg.str.p_str, "PC"))
    {
      if (!allow_pcrel)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return False;
      }
      force_pcrel = True;
      goto parse_abs;
    }
    if (!decode_reg(&reg_arg, &xreg)
      || (xreg < 2))
      return False;
    if (!*arg.str.p_str)
    {
      *p_disp = 0;
      eval_result.OK = True;
    }
    else
      *p_disp = EvalStrIntExpression(&arg, (IntType)(mem_type + 1), &eval_result.OK);
    if (!eval_result.OK)
      return False;

    *p_result |= (xreg << 8);
    return True;
  }

parse_abs:
  addr = EvalStrIntExpressionWithResult(&arg, mem_type, &eval_result);
  if (!eval_result.OK)
    return False;
  is_pcrel = force_pcrel || (eval_result.AddrSpaceMask & (1 << SegCode));

  if (is_pcrel)
  {
    disp = addr - (EProgCounter() + 1);

    *p_result |= (1 << 8);
    *p_disp = disp & 0xffff;
    return True;
  }
  else
  {
    *p_result |= (0 << 8);
    *p_disp = addr & 0xffff;
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_symbol(const tStrComp *p_arg, Word *p_code, const symbol_t *p_symbols)
 * \brief  handle condition or status flag
 * \param  p_arg source argument
 * \param  p_code resulting code
 * \param  p_symbols array of available symbols
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean decode_symbol(const tStrComp *p_arg, Word *p_code, const symbol_t *p_symbols)
{
  for (*p_code = 0; p_symbols[*p_code].p_name; (*p_code)++)
  {
    if (!as_strcasecmp(p_symbols[*p_code].p_name, p_arg->str.p_str))
    {
      *p_code = p_symbols[*p_code].code;
      return True;
    }
  }
  return False;
}

#define decode_condition(p_arg, p_code) decode_symbol(p_arg, p_code, conditions)
#define decode_status_flag(p_arg, p_code) decode_symbol(p_arg, p_code, status_flags)

/*---------------------------------------------------------------------------*/
/* Coding Helpers */

/*!------------------------------------------------------------------------
 * \fn     put_code(Word code)
 * \brief  append one more word of machine code
 * \param  code machine code word to append
 * ------------------------------------------------------------------------ */

static void put_code(Word code)
{
  /* Skip instructions only skip the next word of code.  If the
     previous instruction was a skip, and we are about to create an
     instruction consisting of more than one word, warn about this: */

  if ((1 == CodeLen) && last_was_skip)
    WrStrErrorPos(ErrNum_TrySkipMultiwordInstruction, &OpPart);

  WAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     check_imp16_ext_instr(void)
 * \brief  check whether extended IMP-16 instructions are allowed and complain if not
 * \return True if nothing to complain
 * ------------------------------------------------------------------------ */

static Boolean check_imp16_ext_instr(void)
{
  if (!(p_curr_cpu_props->flags & e_cpu_flag_imp16_ext_instr))
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
  return True;
}

/*---------------------------------------------------------------------------*/
/* Instruction Decoders */

/*!------------------------------------------------------------------------
 * \fn     decode_ld_st(Word code)
 * \brief  handle load/store instructions on PACE
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_ld_st(Word code)
{
  Word reg, mem, reg_max;

  if (!ChkArgCnt(2, 2))
    return;

  if (!decode_mem_arg(&ArgStr[2], &mem, True))
    return;
  
  if (mem & 0x1000)
  {
    reg_max = 0;
    code -= 0x2000;
  }
  else
    reg_max = 3;

  if ((decode_reg(&ArgStr[1], &reg) != eIsReg)
   || (reg > reg_max))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  put_code(code | (reg << 10) | (mem & 0x3ff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem_reg(Word code)
 * \brief  handle instructions with register and memory argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_mem_reg(Word code)
{
  Word reg, mem;

  if (!ChkArgCnt(2, 2))
    return;

  if ((decode_reg(&ArgStr[1], &reg) != eIsReg)
   || ((code & e_inst_flag_r01) && (reg >= 2))
   || ((code & e_inst_flag_r0) && (reg != 0)))
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  if (decode_mem_arg(&ArgStr[2], &mem, !!(code & e_inst_flag_allow_indirect)))
    put_code((code & ~e_inst_flag_all) | (reg << 10) | (mem & 0x13ff));
  this_was_skip = !!(code & e_inst_flag_skip);
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem(Word code)
 * \brief  handle instructions with one memory reference
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_mem(Word code)
{
  Word mem,
       opcode = (code >> 10) & 0x3f,
       i_opcode_xor = (code >> 4) & 0x3f;

  if (!ChkArgCnt(1, 1))
    return;

  if (decode_mem_arg(&ArgStr[1], &mem, !!i_opcode_xor))
    put_code(((opcode ^ ((mem & 0x1000) ? i_opcode_xor : 0x00)) << 10) | (mem & 0x3ff));
  this_was_skip = !!(code & e_inst_flag_skip);
}

/*!------------------------------------------------------------------------
 * \fn     decode_long_mem(Word code)
 * \brief  handle instructions with one long memory reference
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_long_mem(Word code)
{
  Word mem, ext;

  if (!ChkArgCnt(1, 1)
   || !check_imp16_ext_instr())
    return;

  if (decode_long_mem_arg(&ArgStr[1], &mem, &ext, UInt16, code < 0x04c0))
  {
    put_code(code | mem);
    put_code(ext);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_long_byte_mem(Word code)
 * \brief  handle instructions with one long byte memory reference
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_long_byte_mem(Word code)
{
  Word mem, ext;

  if (!ChkArgCnt(1, 1)
   || !check_imp16_ext_instr())
    return;

  if (decode_long_mem_arg(&ArgStr[1], &mem, &ext, UInt15, False))
  {
    put_code((code & ~1) | mem);
    put_code((ext << 1) | (code & 1));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_one_reg(Word code)
 * \brief  handle instructions having one register as argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_one_reg(Word code)
{
  Word reg;

  if (!ChkArgCnt(1, 1))
    return;

  if (decode_reg(&ArgStr[1], &reg) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  put_code(code | (reg << 8));
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_imm(Word code)
 * \brief  handle instructions having one register and one immediate argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_imm(Word code)
{
  Word reg, imm_value;
  Boolean ok;

  if (!ChkArgCnt(2, 2))
    return;

  if (decode_reg(&ArgStr[1], &reg) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  imm_value = EvalStrIntExpression(&ArgStr[2], Int8, &ok);
  if (ok)
    put_code((code & ~e_inst_flag_all) | (reg << 8) | (imm_value & 0xff));
  this_was_skip = !!(code & e_inst_flag_skip);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_reg(Word code)
 * \brief  handle instructions having two register arguments
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_reg(Word code)
{
  Word reg1, reg2;

  if (!ChkArgCnt(2, 2))
    return;

  if (decode_reg(&ArgStr[1], &reg1) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }
  if (decode_reg(&ArgStr[2], &reg2) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
    return;
  }

  if (p_curr_cpu_props->flags & e_cpu_flag_core_pace)
    put_code(code | (reg1 << 6) | (reg2 << 8));
  else
    put_code(code | (reg1 << 10) | (reg2 << 8));
}

/*!------------------------------------------------------------------------
 * \fn     decode_none(Word code)
 * \brief  handle instructions having no argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_none(Word code)
{
  if (ChkArgCnt(0, 0))
    put_code(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_none_ext(Word code)
 * \brief  handle extended instructions having no argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_none_ext(Word code)
{
  if (ChkArgCnt(0, 0)
   || check_imp16_ext_instr())
    put_code(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm7(Word code)
 * \brief  handle instructions having one 7 bit immediate argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_imm7(Word code)
{
  Boolean ok;
  Word imm_value;

  if (!ChkArgCnt(1, 1))
    return;

  imm_value = EvalStrIntExpression(&ArgStr[1], UInt7, &ok);
  if (ok)
    put_code(code | (imm_value & 0x7f));
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm8(Word code)
 * \brief  handle instructions having one 8 bit immediate argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_imm8(Word code)
{
  Boolean ok;
  Word imm_value;

  if (!ChkArgCnt(1, 1))
    return;

  imm_value = EvalStrIntExpression(&ArgStr[1], UInt8, &ok);
  if (ok)
    put_code(code | (imm_value & 0xff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_sflg_pflg_imp16(Word code)
 * \brief  handle PFLG/SFLG instructions - IMP-16 version
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_sflg_pflg_imp16(Word code)
{
  tEvalResult eval_result;
  Word bit_num, imm_value;

  if (!ChkArgCnt(2, 2))
    return;

  bit_num = EvalStrIntExpressionWithResult(&ArgStr[1], UInt4, &eval_result);
  if (!eval_result.OK)
    return;
  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
   && !ChkRangePos(bit_num, 8, 15, &ArgStr[1]))
    return;

  imm_value = EvalStrIntExpressionWithResult(&ArgStr[2], UInt7, &eval_result);
  if (eval_result.OK)
    put_code(code | ((bit_num & 7) << 8) | (imm_value & 0x7f));
}

/*!------------------------------------------------------------------------
 * \fn     decode_sflg_pflg_pace(Word code)
 * \brief  handle PFLG/SFLG instructions - PACE version
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_sflg_pflg_pace(Word code)
{
  Word bit_num;

  if (ChkArgCnt(1, 1)
   && decode_status_flag(&ArgStr[1], &bit_num))
    put_code(code | ((bit_num & 15) << 8));
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm7_io(Word code)
 * \brief  handle instructions having one 7 bit immediate argument as I/O address
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_imm7_io(Word code)
{
  tEvalResult eval_result;
  Word imm_value;

  if (!ChkArgCnt(1, 1))
    return;

  imm_value = EvalStrIntExpressionWithResult(&ArgStr[1], UInt7, &eval_result);
  if (eval_result.OK)
  {
    ChkSpace(SegIO, eval_result.AddrSpaceMask);
    put_code(code | (imm_value & 0x7f));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jsri(Word code)
 * \brief  handle JSRI instruction
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_jsri(Word code)
{
  tEvalResult eval_result;
  Word address;

  if (!ChkArgCnt(1, 1))
    return;

  address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &eval_result);
  if (eval_result.OK)
  {
    if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
      address &= 0x7f;
    if ((address >= 0x80) && (address < 0xff80))
      WrStrErrorPos(ErrNum_UnderRange, &ArgStr[1]);
    else
      put_code(code | (address & 0x7f));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jsrp(Word code)
 * \brief  handle JSRP instruction
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_jsrp(Word code)
{
  tEvalResult eval_result;
  Word address;

  if (!ChkArgCnt(1, 1)
   || !check_imp16_ext_instr())
    return;

  address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt9, &eval_result);
  if (eval_result.OK)
  {
    if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
      address &= ~0x80;
    if (address & 0x80)
      WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
    else
      put_code(code | (address & 0x7f));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_shift_imp(Word code)
 * \brief  handle shift instructions - IMP-16 variant
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_shift_imp16(Word code)
{
  Word reg, count;
  Boolean ok;

  if (!ChkArgCnt(2, 2))
    return;

  if (decode_reg(&ArgStr[1], &reg) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  count = EvalStrIntExpression(&ArgStr[2], UInt7, &ok);
  if (ok)
  {
    if (code & 0x0080)
      count = ~count + 1;
    put_code((code & 0xfc00) | (reg << 8) | (count & 0xff));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_shift_pace(Word code)
 * \brief  handle shift instructions - PACE variant
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_shift_pace(Word code)
{
  Word reg, count, link = 0;
  Boolean ok;

  if (!ChkArgCnt(2, 3))
    return;

  if (decode_reg(&ArgStr[1], &reg) != eIsReg)
  {
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    return;
  }

  count = EvalStrIntExpression(&ArgStr[2], UInt7, &ok);
  if (!ok)
    return;

  if (3 == ArgCnt)
  {
    link = EvalStrIntExpression(&ArgStr[3], UInt1, &ok);
    if (!ok)
      return;
  }

  put_code(code | (reg << 8) | (count << 1) | link);
}

/*!------------------------------------------------------------------------
 * \fn     decode_boc(Word code)
 * \brief  handle BOC instruction
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void decode_boc(Word code)
{
  LongInt dist;
  tEvalResult eval_result;
  Word cond;

  if (!ChkArgCnt(2, 2)
   || !decode_condition(&ArgStr[1], &cond))
    return;

  dist = EvalStrIntExpressionWithResult(&ArgStr[2], UInt16, &eval_result) - (EProgCounter() + 1);
  if (eval_result.OK)
  {
    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && !RangeCheck(dist, SInt8))
      WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
    else
      put_code(code | (cond << 8) | (dist & 0xff));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_bit(Word code)
 * \brief  handle instructions with bit position as argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_bit(Word code)
{
  Word bit_pos;

  if (!ChkArgCnt(1, 1)
   || !check_imp16_ext_instr())
    return;

  if (!(code & 0x0001)
   || !decode_status_flag(&ArgStr[1], &bit_pos))
  {
    Boolean ok;

    bit_pos = EvalStrIntExpression(&ArgStr[1], UInt4, &ok);
    if (!ok)
      return;
  }

  put_code((code & 0xfffe) | (bit_pos & 0x000f));
}

/*!------------------------------------------------------------------------
 * \fn     decode_jmpp_jint(Word code)
 * \brief  handle JMPP and JINT instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jmpp_jint(Word code)
{
  Word address;
  tEvalResult eval_result;

  if (!ChkArgCnt(1, 1)
   || !check_imp16_ext_instr())
    return;

  address = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &eval_result);
  if (!eval_result.OK)
    return;

  if (mFirstPassUnknownOrQuestionable(eval_result.Flags))
    address &= 15;
  if (address < 16);
  else if (!ChkRangePos(address, code & 0x1f0, (code & 0x1f0) + 15, &ArgStr[1]))
    return;

  put_code(code | (address & 15));
}

/*!------------------------------------------------------------------------
 * \fn     decode_port(Word code)
 * \brief  handle PORT instruction
 * ------------------------------------------------------------------------ */

static void decode_port(Word code)
{
  UNUSED(code);
  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*!------------------------------------------------------------------------
 * \fn     decode_ltorg(Word code)
 * \brief  handle LTORG instruction
 * ------------------------------------------------------------------------ */

static LargeInt ltorg_16(const as_literal_t *p_lit, struct sStrComp *p_name)
{
  LargeInt ret;

  SetMaxCodeLen((CodeLen + 1) * 2);
  ret = EProgCounter() + CodeLen;
  EnterIntSymbol(p_name, ret, ActPC, False);
  put_code(p_lit->value & 0xffff);
  return ret;
}

static void decode_ltorg(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(0, 0))
    literals_dump(ltorg_16, eSymbolSize16Bit, MomSectionHandle, False);
}

/*---------------------------------------------------------------------------*/
/* Code Table Handling */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  construct instruction hash table
 * ------------------------------------------------------------------------ */

static void add_condition(const char *p_name, Word code)
{
  order_array_rsv_end(conditions, symbol_t);
  conditions[InstrZ].p_name = p_name;
  conditions[InstrZ++].code = code;
}

static void add_status_flag(const char *p_name, Word code)
{
  order_array_rsv_end(status_flags, symbol_t);
  status_flags[InstrZ].p_name = p_name;
  status_flags[InstrZ++].code = code;
}

static void init_fields(Boolean is_pace)
{
  InstTable = CreateInstTable(103);

  AddInstTable(InstTable, "HALT" , 0x0000, decode_none);
  AddInstTable(InstTable, "PUSHF", is_pace ? 0x0c00 : 0x0080, decode_none);
  AddInstTable(InstTable, "PULLF", is_pace ? 0x1000 : 0x0280, decode_none);
  AddInstTable(InstTable, "NOP"  , NOPCode, decode_none);
  AddInstTable(InstTable, "BOC"  , is_pace ? 0x4000 : 0x1000, decode_boc);
  AddInstTable(InstTable, "JMP"  , is_pace ? 0x1a00 : 0x2010, decode_mem);
  AddInstTable(InstTable, "JSR"  , is_pace ? 0x1600 : 0x2810, decode_mem);
  AddInstTable(InstTable, "RADD" , is_pace ? 0x6800 : 0x3000, decode_reg_reg);
  AddInstTable(InstTable, "RAND" , is_pace ? 0x5400 : 0x3083, decode_reg_reg);
  AddInstTable(InstTable, "RXOR" , is_pace ? 0x5800 : 0x3082, decode_reg_reg);
  AddInstTable(InstTable, "RXCH" , is_pace ? 0x6c00 : 0x3080, decode_reg_reg);
  AddInstTable(InstTable, "RCPY" , is_pace ? 0x5c00 : 0x3081, decode_reg_reg);
  AddInstTable(InstTable, "PUSH" , is_pace ? 0x6000 : 0x4000, decode_one_reg);
  AddInstTable(InstTable, "PULL" , is_pace ? 0x6400 : 0x4400, decode_one_reg);
  AddInstTable(InstTable, "AISZ" , (is_pace ? 0x7800 : 0x4800) | e_inst_flag_skip, decode_reg_imm);
  AddInstTable(InstTable, "LI"   , is_pace ? 0x5000 : 0x4c00, decode_reg_imm);
  AddInstTable(InstTable, "CAI"  , is_pace ? 0x7000 : 0x5000, decode_reg_imm);
  AddInstTable(InstTable, "XCHRS", is_pace ? 0x1c00 : 0x5400, decode_one_reg);
  AddInstTable(InstTable, "ISZ"  , (is_pace ? 0x8c00 : 0x7800) | e_inst_flag_skip, decode_mem);
  AddInstTable(InstTable, "DSZ"  , (is_pace ? 0xac00 : 0x7c00) | e_inst_flag_skip, decode_mem);
  AddInstTable(InstTable, "ADD"  , is_pace ? 0xe000 : 0xc000, decode_mem_reg);
  AddInstTable(InstTable, "AND"  , is_pace ? (0xa800 | e_inst_flag_r0) : (0x6000 | e_inst_flag_r01), decode_mem_reg);
  AddInstTable(InstTable, "OR"   , is_pace ? (0xa400 | e_inst_flag_r0) : (0x6800 | e_inst_flag_r01), decode_mem_reg);
  AddInstTable(InstTable, "SKG"  , (is_pace ? (0x9c00 | e_inst_flag_r0) : 0xe000) | e_inst_flag_skip, decode_mem_reg);
  AddInstTable(InstTable, "SKNE" , 0xf000 | e_inst_flag_skip, decode_mem_reg);
  AddInstTable(InstTable, "SKAZ" , (is_pace ? (0xb800 | e_inst_flag_r0) : (0x7000 | e_inst_flag_r01)) | e_inst_flag_skip, decode_mem_reg);

  if (is_pace)
  {
    AddInstTable(InstTable, "LD"   , 0xc000, decode_ld_st);
    AddInstTable(InstTable, "ST"   , 0xd000, decode_ld_st);
    AddInstTable(InstTable, "RTI"  , 0x7c00, decode_imm8);
    AddInstTable(InstTable, "RTS"  , 0x8000, decode_imm8);
    AddInstTable(InstTable, "RADC" , 0x7400, decode_reg_reg);
    AddInstTable(InstTable, "SUBB" , (0x9000 | e_inst_flag_r0), decode_mem_reg);
    AddInstTable(InstTable, "DECA" , (0x8800 | e_inst_flag_r0), decode_mem_reg);
    AddInstTable(InstTable, "LSEX" , (0xbc00 | e_inst_flag_r0), decode_mem_reg);
    AddInstTable(InstTable, "CFR"  , 0x0400, decode_one_reg);
    AddInstTable(InstTable, "CRF"  , 0x0800, decode_one_reg);
    AddInstTable(InstTable, "ROL"  , 0x2000, decode_shift_pace);
    AddInstTable(InstTable, "ROR"  , 0x2400, decode_shift_pace);
    AddInstTable(InstTable, "SHL"  , 0x2800, decode_shift_pace);
    AddInstTable(InstTable, "SHR"  , 0x2c00, decode_shift_pace);
    AddInstTable(InstTable, "SFLG" , 0x3080, decode_sflg_pflg_pace);
    AddInstTable(InstTable, "PFLG" , 0x3000, decode_sflg_pflg_pace);
  }
  else /* IMP-16 */
  {
    AddInstTable(InstTable, "LD"   , (0x8000 | e_inst_flag_allow_indirect), decode_mem_reg);
    AddInstTable(InstTable, "ST"   , (0xa000 | e_inst_flag_allow_indirect), decode_mem_reg);
    AddInstTable(InstTable, "RTI"  , 0x0100, decode_imm7);
    AddInstTable(InstTable, "RTS"  , 0x0200, decode_imm7);
    AddInstTable(InstTable, "JSRP" , 0x0300, decode_jsrp);
    AddInstTable(InstTable, "JSRI" , 0x0380, decode_jsri);
    AddInstTable(InstTable, "RIN"  , 0x0400, decode_imm7_io);
    AddInstTable(InstTable, "ROUT" , 0x0600, decode_imm7_io);
    AddInstTable(InstTable, "MPY"  , 0x0480, decode_long_mem);
    AddInstTable(InstTable, "DIV"  , 0x0490, decode_long_mem);
    AddInstTable(InstTable, "DADD" , 0x04a0, decode_long_mem);
    AddInstTable(InstTable, "DSUB" , 0x04b0, decode_long_mem);
    AddInstTable(InstTable, "LDB"  , 0x04c0, decode_long_mem);
    AddInstTable(InstTable, "STB"  , 0x04d0, decode_long_mem);
    AddInstTable(InstTable, "LLB"  , 0x04c0, decode_long_byte_mem);
    AddInstTable(InstTable, "SLB"  , 0x04d0, decode_long_byte_mem);
    AddInstTable(InstTable, "LRB"  , 0x04c1, decode_long_byte_mem);
    AddInstTable(InstTable, "SRB"  , 0x04d1, decode_long_byte_mem);
    AddInstTable(InstTable, "JMPP" , 0x0500, decode_jmpp_jint);
    AddInstTable(InstTable, "ISCAN", 0x0510, decode_none_ext);
    AddInstTable(InstTable, "JINT" , 0x0520, decode_jmpp_jint);
    AddInstTable(InstTable, "SETST", 0x0701, decode_reg_bit);
    AddInstTable(InstTable, "CLRST", 0x0711, decode_reg_bit);
    AddInstTable(InstTable, "SETBIT",0x0720, decode_reg_bit);
    AddInstTable(InstTable, "CLRBIT",0x0730, decode_reg_bit);
    AddInstTable(InstTable, "SKSTF", 0x0741, decode_reg_bit);
    AddInstTable(InstTable, "SKBIT", 0x0750, decode_reg_bit);
    AddInstTable(InstTable, "CMPBIT",0x0760, decode_reg_bit);
    AddInstTable(InstTable, "SUB"  , 0xd000, decode_mem_reg);
    AddInstTable(InstTable, "ROL"  , 0x5800, decode_shift_imp16);
    AddInstTable(InstTable, "ROR"  , 0x5880, decode_shift_imp16);
    AddInstTable(InstTable, "SHL"  , 0x5c00, decode_shift_imp16);
    AddInstTable(InstTable, "SHR"  , 0x5c80, decode_shift_imp16);
    AddInstTable(InstTable, "SFLG" , 0x0800, decode_sflg_pflg_imp16);
    AddInstTable(InstTable, "PFLG" , 0x0880, decode_sflg_pflg_imp16);
  }


  if (ValidSegs & (1 << SegCode))
    AddInstTable(InstTable, "PORT" , 0, decode_port);
  AddInstTable(InstTable, "ASCII" , eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString | eIntPseudoFlag_BigEndian, DecodeIntelDB);
  AddInstTable(InstTable, "WORD" , eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDW);
  AddInstTable(InstTable, "LTORG", 0, decode_ltorg);

  InstrZ = 0;
  add_condition("REQ0", 1);
  add_condition("PSIGN", 2);
  add_condition("BIT0", 3);
  add_condition("BIT1", 4);
  add_condition("NREQ0", 5);
  add_condition("NSIGN", 11);
  if (is_pace)
  {
    add_condition("STFL", 0);
    add_condition("BIT2", 6);
    add_condition("CONTIN", 7);
    add_condition("LINK", 8);
    add_condition("IEN", 9);
    add_condition("CARRY", 10);
    add_condition("OVF", 12);
    add_condition("JC13", 13);
    add_condition("JC14", 14);
    add_condition("JC15", 15);
  }
  else
  {
    add_condition("INT", 0);
    add_condition("CPINT", 6);
    add_condition("START", 7);
    add_condition("STFL", 8);
    add_condition("INEN", 9);
    add_condition("CY/OV", 10);
    if (p_curr_cpu_props->flags & e_cpu_flag_imp16_ien_status)
    {
      add_condition("POA", 12);
      add_condition("SEL", 13);
    }
  }
  add_condition(NULL, 0);

  InstrZ = 0;
  if (is_pace)
  {
    add_status_flag("IE1", 1);
    add_status_flag("IE2", 2);
    add_status_flag("IE3", 3);
    add_status_flag("IE4", 4);
    add_status_flag("IE5", 5);
    add_status_flag("OV", 6);
    add_status_flag("CY", 7);
    add_status_flag("LINK", 8);
    add_status_flag("IEN", 9);
    add_status_flag("BYTE", 10);
    add_status_flag("F11", 11);
    add_status_flag("F12", 12);
    add_status_flag("F13", 13);
    add_status_flag("F14", 14);
  }
  else
  {
    add_status_flag("L", 15);
    add_status_flag("OV", 14);
    add_status_flag("CY", 13);
    if (p_curr_cpu_props->flags & e_cpu_flag_imp16_ien_status)
    {
      add_status_flag("IEN3", 12);
      add_status_flag("IEN2", 8);
      add_status_flag("IEN1", 4);
      add_status_flag("IEN0", 0);
    }
  }
  add_status_flag(NULL, 0);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  clean up hash table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  order_array_free(conditions);
  order_array_free(status_flags);
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/
/* Semiglobal Functions */

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_imp16(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for IMP-16
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_imp16(char *p_arg, TempResult *p_result)
{
  Word reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_imp16;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_imp16(void)
 * \brief  handle machine instuctions
 * ------------------------------------------------------------------------ */

static void make_code_imp16(void)
{
  CodeLen = 0; DontPrint = False;
  this_was_skip = False;

  /* to be ignored */

  if (Memo(""))
    goto func_exit;

  /* pseudo instructions */

  if (DecodeIntelPseudo(False))
    goto func_exit;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);

func_exit:
  last_was_skip = this_was_skip;
}

/*!------------------------------------------------------------------------
 * \fn     is_def_imp16(void)
 * \brief  does instruction consume label?
 * ------------------------------------------------------------------------ */

static Boolean is_def_imp16(void)
{
  return (ValidSegs & (1 << SegCode)) ? Memo("PORT") : False;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_imp16(void *p_user)
 * \brief  switch to target
 * \param  properties descriptor
 * ------------------------------------------------------------------------ */

static void switch_to_imp16(void *p_user)
{
  const TFamilyDescr *p_descr;
  Boolean is_pace;

  p_curr_cpu_props = (cpu_props_t*)p_user;
  is_pace = !!(p_curr_cpu_props->flags & e_cpu_flag_core_pace);
  p_descr = FindFamilyByName(is_pace ? "IPC-16" : "IMP-16");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIBM);
  IntConstModeIBMNoTerm = True;
  QualifyQuote = QualifyQuote_SingleQuoteConstant;

  PCSymbol = ".";
  HeaderID = p_descr->Id;
  NOPCode = is_pace ? 0x5c00 : 0x3081; /* = RCPY AC0, AC0 */
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  if (is_pace)
  {
    static ASSUMERec assume_pace = { "BPS", &bps_val, 0, 1, 0, NULL };

    pASSUMERecs = &assume_pace;
    ASSUMERecCnt = 1;
  }
  else
  {
    ValidSegs |= 1 << SegIO;
    Grans[SegIO] = 2; ListGrans[SegIO] = 2; SegInits[SegCode] = 0;
    SegLimits[SegIO] = 0x7f;
  }

  MakeCode = make_code_imp16;
  IsDef = is_def_imp16;
  InternSymbol = intern_symbol_imp16;
  SwitchFrom = deinit_fields;
  init_fields(is_pace);
}

/*!------------------------------------------------------------------------
 * \fn     init_pass_pace(void)
 * \brief  set internal variables to default upon start of pass
 * ------------------------------------------------------------------------ */

static void init_pass_pace(void)
{
  bps_val = 0;
}

/*!------------------------------------------------------------------------
 * \fn     codeimp16_init(void)
 * \brief  attach target
 * ------------------------------------------------------------------------ */

static const cpu_props_t cpu_props[] =
{
  { "IMP-16C/200", e_cpu_flag_none },
  { "IMP-16C/300", e_cpu_flag_imp16_ext_instr },
  { "IMP-16P/200", e_cpu_flag_none },
  { "IMP-16P/300", e_cpu_flag_imp16_ext_instr },
  { "IMP-16L"    , e_cpu_flag_imp16_ext_instr | e_cpu_flag_imp16_ien_status },
  { "IPC-16"     , e_cpu_flag_core_pace },
  { "INS8900"    , e_cpu_flag_core_pace }
};

void codeimp16_init(void)
{
  const cpu_props_t *p_prop;

  for (p_prop = cpu_props; p_prop < cpu_props + as_array_size(cpu_props); p_prop++)
    (void)AddCPUUser(p_prop->p_name, switch_to_imp16, (void*)p_prop, NULL);
  AddInitPassProc(init_pass_pace);
}
