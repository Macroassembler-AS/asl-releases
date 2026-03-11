/* codecr16.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator National Semiconductor CR16A/B                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmerr.h"
#include "asmallg.h"
#include "asmcode.h"
#include "errmsg.h"
#include "headids.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "codecr16.h"

/* ------------------------------------------------------------------------ */

static CPUVar cpu_cr16a, cpu_cr16b;
static IntType addr_int_type, code_int_type;
static LongInt memory_model;

/* ------------------------------------------------------------------------ */
/* Register Symbol Handling */

static const char reg_name_sp[] = "SP";
static const char reg_name_era[] = "ERA";
#define REG_SP 15
#define REG_RA 14
#define REG_ERA 13

/*!------------------------------------------------------------------------
 * \fn     decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
 * \brief  parse CPU register name
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \return True if argument is CPU register
 * ------------------------------------------------------------------------ */

static Boolean decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
{
  int l;

  if (!as_strcasecmp(p_arg, reg_name_sp))
  {
    *p_result = REG_SP;
    goto is_reg;
  }

  if (!as_strcasecmp(p_arg, reg_name_era) && (MomCPU >= cpu_cr16b))
  {
    *p_result = REG_ERA | REGSYM_FLAG_ALIAS;
    goto is_reg;
  }

  l = strlen(p_arg);
  if ((l < 2) || (l > 3))
    return False;
  if (as_toupper(*p_arg) != 'R')
    return False;
  p_arg++;
  if ((as_toupper(*p_arg) == 'A') && (l == 2))
  {
    *p_result = REG_RA;
    goto is_reg;
  }
  *p_result = 0;
  for (;*p_arg; p_arg++)
  {
    if (!as_isdigit(*p_arg))
      return False;
    *p_result = (*p_result * 10) + (*p_arg - '0');
  }
  if (*p_result > 13)
    return False;

is_reg:
  if (p_size) *p_size = eSymbolSize16Bit;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_cr16(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  translate register # back to textual form
 * \param  p_dest destination text buffer
 * \param  dest_size capacity of buffer
 * \param  value register #
 * \param  inp_size register's operand size
 * ------------------------------------------------------------------------ */

static void dissect_reg_cr16(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  if (inp_size == eSymbolSize16Bit)
    switch (value)
    {
      case REG_SP:
        strmaxcpy(p_dest, reg_name_sp, dest_size);
        return;
      case REG_RA:
        strmaxcpy(p_dest, "RA", dest_size);
        return;
      case REG_ERA | REGSYM_FLAG_ALIAS:
        strmaxcpy(p_dest, reg_name_era, dest_size);
        return;
      default:
        if (value <= 13)
        {
          as_snprintf(p_dest, dest_size, "R%u", value);
          return;
        }
    }
  as_snprintf(p_dest, dest_size, "%d-%u", (unsigned)inp_size, (unsigned)value);
}

/* ------------------------------------------------------------------------ */
/* Instruction Parser Helpers */

/*!------------------------------------------------------------------------
 * \fn     append_word(Word word)
 * \brief  append one more word of machine code
 * \param  word word to append
 * ------------------------------------------------------------------------ */

static void append_word(Word word)
{
  WAsmCode[CodeLen >> 1] = word;
  CodeLen += 2;
}

/* ------------------------------------------------------------------------ */
/* Argument Decoding */

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result, Boolean must_be_reg)
 * \brief  decode register argument
 * \param  p_arg source argument
 * \param  p_result returns register #
 * \param  must_be_reg argument must be  a register
 * \return OK/No/Abort
 * ------------------------------------------------------------------------ */

static tRegEvalResult decode_reg(const tStrComp *p_arg, Word *p_result, Boolean must_be_reg)
{
  tRegDescr reg_descr;
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;

  if (decode_reg_core(p_arg->str.p_str, p_result, &eval_result.DataSize))
  {
    reg_descr.Reg = *p_result;
    reg_eval_result = eIsReg;
  }
  else
    reg_eval_result = EvalStrRegExpressionAsOperand(p_arg, &reg_descr, &eval_result, eSymbolSizeUnknown, must_be_reg);

  if ((reg_eval_result == eIsReg) && (eval_result.DataSize != eSymbolSize16Bit))
  {
    WrStrErrorPos(ErrNum_InvOpSize, p_arg);
    reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
  }

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  return reg_eval_result;
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, IntType imm_int_type)
 * \brief  parse address expression
 * \param  p_arg source argument
 * \param  p_vals parse result
 * \param  imm_int_type data range for immediate arguments
 * \return deduced address mode
 * ------------------------------------------------------------------------ */

typedef enum
{
  e_mode_none = -1,
  e_mode_reg = 0,
  e_mode_imm = 1,
  e_mode_disp_base_short = 2,
  e_mode_disp_base_medium = 3,
  e_mode_disp_pair_medium = 4,
  e_mode_abs = 5
} adr_mode_t;

typedef struct
{
  adr_mode_t mode;
  Word part;
  int val_cnt;
  tSymbolFlags disp_flags;
  Word vals[2];
} adr_vals_t;

static adr_mode_t reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_mode_none;
  p_vals->part = 0;
  p_vals->val_cnt = 0;
  p_vals->disp_flags = eSymbolFlag_None;
  return p_vals->mode;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  int z;
  for (z = 0; z < p_vals->val_cnt; z++)
    append_word(p_vals->vals[z]);
}

static LongInt get_displacement(const adr_vals_t *p_vals)
{
  switch (p_vals->mode)
  {
    case e_mode_disp_base_short:
    {
      LongInt ret = p_vals->vals[0] & 0x1f;
      return (ret & 0x10) ? (ret - 32) : ret;
    }
    case e_mode_disp_base_medium:
    case e_mode_disp_pair_medium:
    {
      LongInt ret = (LongWord)p_vals->vals[0] | (((LongWord)(p_vals->vals[1] & 3)) << 16);
      return (ret & 0x20000) ? (ret - 0x40000) : ret;
    }
    default:
      return 0;
  }
}

static adr_mode_t decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, IntType imm_int_type)
{
  Boolean ok;
  int split_pos, arg_len;

  reset_adr_vals(p_vals);

  if (*p_arg->str.p_str == '$')
  {
    LongInt value;

    value = EvalStrIntExpressionOffsWithFlags(p_arg, 1, imm_int_type, &ok, &p_vals->disp_flags);
    if (!ok)
      return reset_adr_vals(p_vals);

    switch (imm_int_type)
    {
      case Int16:
        p_vals->vals[0] = value & 0xffff;
        p_vals->val_cnt =
          (p_vals->vals[0] == 0xfff1) || ((p_vals->vals[0] > 0x000f) && (p_vals->vals[0] < 0xfff0));
        break;
      case SInt16:
        p_vals->vals[0] = value & 0xffff;
        p_vals->val_cnt = (value == -15) || (value < -16) || (value > 15);
        break;
      case Int8:
        p_vals->vals[0] = value & 0xff;
        p_vals->val_cnt =
          (p_vals->vals[0] == 0xf1) || ((p_vals->vals[0] > 0x0f) && (p_vals->vals[0] < 0xf0));
        break;
      case SInt8:
        p_vals->vals[0] = value & 0xff;
        p_vals->val_cnt = (value == -15) || (value < -16) || (value > 15);
        break;
      case Int4:
        p_vals->vals[0] = value & 0xf;
        p_vals->val_cnt = 1;
        break;
      default:
        break;
    }

    p_vals->part = p_vals->val_cnt ? 0x11 : (value & 0x001f);
    p_vals->mode = e_mode_imm;
    goto check;
  }

  split_pos = FindDispBaseSplitWithQualifier(p_arg->str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp disp_arg, base_arg;
    LongInt disp_acc;
    Word regs[2];
    size_t reg_cnt = 0;
    char *p_split;

    StrCompSplitRef(&disp_arg, &base_arg, p_arg, p_arg->str.p_str + split_pos);
    KillPostBlanksStrComp(&disp_arg);
    StrCompShorten(&base_arg, 1);
    KillPrefBlanksStrCompRef(&base_arg); KillPostBlanksStrComp(&base_arg);

    if (disp_arg.str.p_str[0])
    {
      disp_acc = EvalStrIntExpressionWithFlags(&disp_arg, Int18, &ok, &p_vals->disp_flags);
      if (!ok)
        return reset_adr_vals(p_vals);
    }
    else
    {
      disp_acc = 0;
      ok = True;
      p_vals->disp_flags = eSymbolFlag_None;
    }

    do
    {
      tStrComp base_arg_remainder;

      p_split = QuotPos(base_arg.str.p_str, ',');
      if (p_split)
      {
        StrCompSplitRef(&base_arg, &base_arg_remainder, &base_arg, p_split);
        KillPostBlanksStrComp(&base_arg);
        KillPrefBlanksStrCompRef(&base_arg_remainder);
      }
      if (reg_cnt >= as_array_size(regs))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &base_arg);
        return reset_adr_vals(p_vals);
      }
      if (eIsReg != decode_reg(&base_arg, &regs[reg_cnt++], True))
        return reset_adr_vals(p_vals);
      if (p_split)
        base_arg = base_arg_remainder;
    }
    while (p_split);

    switch (reg_cnt)
    {
      case 1:
        p_vals->part = regs[0];
        if ((disp_acc >= -16) && (disp_acc <= 15))
        {
          p_vals->vals[p_vals->val_cnt++] = disp_acc & 0x1f;
          p_vals->mode = e_mode_disp_base_short;
        }
        else
        {
          p_vals->vals[p_vals->val_cnt++] = disp_acc & 0xffff;
          p_vals->vals[p_vals->val_cnt++] = (disp_acc >> 16) & 0x0003;
          p_vals->mode = e_mode_disp_base_medium;
        }
        break;
      case 2:
        if (regs[0] != regs[1] + 1)
          goto bad;
        p_vals->part = regs[1];
        p_vals->vals[p_vals->val_cnt++] = disp_acc & 0xffff;
        p_vals->vals[p_vals->val_cnt++] = (disp_acc >> 16) & 0x0003;
        p_vals->mode = e_mode_disp_pair_medium;
        break;
      default:
      bad:
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_vals);
    }
    goto check;
  }

  switch (decode_reg(p_arg, &p_vals->part, False))
  {
    case eRegAbort:
      return reset_adr_vals(p_vals);
    case eIsReg:
      p_vals->mode = e_mode_reg;
      goto check;
    default:
    {
      Boolean ok;
      LongWord address = EvalStrIntExpressionWithFlags(p_arg, UInt18, &ok, &p_vals->disp_flags);
      if (ok)
      {
        p_vals->vals[p_vals->val_cnt++] = address & 0xffff;
        p_vals->vals[p_vals->val_cnt++] = (address >> 16) & 0x0003;
        p_vals->mode = e_mode_abs;
      }
      goto check;
    }
  }

check:
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     isreg_0189(Word reg)
 * \brief  is register # 0, 1, 8, or 9 (needed for some CR16B insns)?
 * \param  reg register #
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean isreg_0189(Word reg)
{
  return !(reg & 0x6);
}

/*!------------------------------------------------------------------------
 * \fn     decode_code_addr(const tStrComp *p_arg, LongWord *p_dest, tSymbolFlags *p_flags)
 * \brief  parse code address for branches/jumps
 * \param  p_arg source argument
 * \param  p_dest dest buffer for address
 * \param  p_flags dest buffer for symbol flags
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean decode_code_addr(const tStrComp *p_arg, LongWord *p_dest, tSymbolFlags *p_flags)
{
  Boolean ok;

  *p_dest = EvalStrIntExpressionWithFlags(p_arg, code_int_type, &ok, p_flags);
  if (ok && (*p_dest & 1))
  {
    if (mFirstPassUnknownOrQuestionable(*p_flags))
      *p_dest &= 1;
    else
    {
      WrStrErrorPos(ErrNum_AddrMustBeEven, p_arg);
      ok = False;
    }
  }
  return ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_ireg_pair(const tStrComp *p_arg, Word *p_reg)
 * \brief  decode register pair ('(Rn+1,Rn)')
 * \param  p_arg source argument
 * \param  p_reg dest buffer for register (pair)
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean decode_ireg_pair(const tStrComp *p_arg, Word *p_reg)
{
  adr_vals_t adr_vals;

  switch (decode_adr(p_arg, &adr_vals, Int16))
  {
    case e_mode_disp_pair_medium:
      if (get_displacement(&adr_vals))
        goto wrong;
      *p_reg = adr_vals.part;
      return True;
    case e_mode_none:
      return False;
    default:
    wrong:
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ireg_dest(const tStrComp *p_arg, Word *p_r_target)
 * \brief  decode indirect jump dest (Rtarget or (Rtarget+1,Rtarget))
 * \param  p_arg source argument
 * \param  p_r_target dest buffer for register (pair)
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean decode_ireg_dest(const tStrComp *p_arg, Word *p_r_target)
{
  return (MomCPU == cpu_cr16b)
       ? decode_ireg_pair(p_arg, p_r_target)
       : (decode_reg(p_arg, p_r_target, True) == eIsReg);
}

/*!------------------------------------------------------------------------
 * \fn     decode_14(const tStrComp *p_arg, Word *p_result, tSymbolFlags *p_flags)
 * \brief  decode immediate value from 1..4 as 0..3
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \param  p_flags associated symbol flags
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean decode_14(const tStrComp *p_arg, Word *p_result, tSymbolFlags *p_flags)
{
  Boolean ok;

  *p_result = EvalStrIntExpressionOffsWithFlags(p_arg, !!(p_arg->str.p_str[0] == '$'), UInt3, &ok, p_flags);
  if (ok && !mFirstPassUnknownOrQuestionable(*p_flags) && !ChkRangePos(*p_result, 1, 4, p_arg))
    ok = False;
  if (ok)
    (*p_result)--;
  return ok;
}

/*!------------------------------------------------------------------------
 * \fn     decode_vector(const tStrComp *p_arg, Word *p_dest)
 * \brief  parse exception vector name
 * \param  p_arg source argument
 * \param  p_dest dest buffer for vector #
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

typedef struct
{
  char name[4];
  Byte num;
} vector_t;

static Boolean decode_vector(const tStrComp *p_arg, Word *p_dest)
{
  static const vector_t vectors[] =
  {
    { "SVC", 0x45 },
    { "DVZ", 0x46 },
    { "FLG", 0x47 },
    { "BPT", 0x48 },
    { "UND", 0x4a },
    { "DBG", 0x4e },
  };
  size_t z;

  for (z = 0; z < as_array_size(vectors); z++)
    if (!as_strcasecmp(p_arg->str.p_str, vectors[z].name))
    {
      if (!(vectors[z].num & ((MomCPU == cpu_cr16b) ? 0x80 : 0x40)))
        continue;
      *p_dest = vectors[z].num;
      return True;
    }
  WrStrErrorPos(ErrNum_UnknownVector, p_arg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     decode_cpu_reg(const tStrComp *p_arg, Word *p_dest)
 * \brief  parse CPU register name
 * \param  p_arg source argument
 * \param  p_dest dest buffer for vector #
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

typedef struct
{
  char name[9];
  Byte num;
} cpu_reg_t;

static Boolean decode_cpu_reg(const tStrComp *p_arg, Word *p_dest)
{
  static const cpu_reg_t cpu_regs[] =
  {
    { "PSR"     , 0xc1 },
    { "INTBASE" , 0x43 },
    { "INTBASEL", 0x83 },
    { "INTBASEH", 0x84 },
    { "CFG"     , 0x85 },
    { "DSR"     , 0x87 },
    { "DCR"     , 0x89 },
    { "ISP"     , 0xcb },
    { "CARL"    , 0x8d },
    { "CARH"    , 0x8e }
  };
  size_t z;

  for (z = 0; z < as_array_size(cpu_regs); z++)
    if (!as_strcasecmp(p_arg->str.p_str, cpu_regs[z].name))
    {
      if (!(cpu_regs[z].num & ((MomCPU == cpu_cr16b) ? 0x80 : 0x40)))
        continue;
      *p_dest = cpu_regs[z].num & 0x0f;
      return True;
    }
  WrStrErrorPos(ErrNum_InvCtrlReg, p_arg);
  return False;
}

/* ------------------------------------------------------------------------ */
/* Instruction Parsers */

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    append_word(code);
}

static void decode_fixed_b(Word code)
{
  if (ChkMinCPU(cpu_cr16b))
    decode_fixed(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_one_reg(Word code)
 * \brief  handle instructions with one register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_one_reg(Word code)
{
  Word reg;

  if (ChkArgCnt(1, 1)
   && (eIsReg == decode_reg(&ArgStr[1], &reg, True)))
    append_word(code | (reg << 1));
}

/*!------------------------------------------------------------------------
 * \fn     decode_two_reg(Word code)
 * \brief  handle instructions with two register arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_two_reg(Word code)
{
  Word reg[2];

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg[code & 1], True)
   && decode_reg(&ArgStr[2], &reg[(code & 1) ^ 1], True))
    append_word(code | (reg[1] << 5) | (reg[0] << 1));
}

static void decode_two_reg_b(Word code)
{
  if (ChkMinCPU(cpu_cr16b))
    decode_two_reg(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_regimm_reg(Word code)
 * \brief  handle instructions with one register/immediate and one register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_regimm_reg(Word code)
{
  Word Rdest;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[2], &Rdest, True))
  {
    adr_vals_t adr_vals;
    IntType imm_int_type = (IntType)(code & 0x1ff);
    
    code ^= imm_int_type;
    switch (decode_adr(&ArgStr[1], &adr_vals, imm_int_type))
    {
      case e_mode_reg:
        append_word(code | (Rdest << 5) | (adr_vals.part << 1) | 0x4001);
        break;
      case e_mode_imm:
        append_word(code | (Rdest << 5) | (adr_vals.part & 0x1f));
        append_adr_vals(&adr_vals);
        break;
      case e_mode_none:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_regpair_b(Word code)
 * \brief  handle MULUW/MULSW instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_regpair_b(Word code)
{
  Word Rsrc, Rdest;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(cpu_cr16b)
   && decode_reg(&ArgStr[1], &Rsrc, True)
   && decode_ireg_pair(&ArgStr[2], &Rdest))
  {
    if ((code & 0x1000) && !isreg_0189(Rsrc)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
      append_word(code | (Rdest << 5) | (Rsrc << 1));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jump(Word code)
 * \brief  handle JUMP/Jcond instructions
 * \param  code machine code/conditions
 * ------------------------------------------------------------------------ */

static void decode_jump(Word code)
{
  Word Rtarget;

  if (ChkArgCnt(1, 1) && decode_ireg_dest(&ArgStr[1], &Rtarget))
    append_word(code | (Rtarget << 1));
}

/*!------------------------------------------------------------------------
 * \fn     decode_br(Word code)
 * \brief  handle branch instructions
 * \param  code machine code/conditions
 * ------------------------------------------------------------------------ */

static void decode_br(Word code)
{
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(1, 1)
   && decode_code_addr(&ArgStr[1], &dest, &flags))
  {
    LongInt dist = dest - (EProgCounter() + 2);

    if ((dist >= -256) && (dist <= 254))
    {
      set_w_guessed(flags, CodeLen >> 1, 1, 0x1e1e);
      append_word(0x4000 | code | ((dist & 0x1e0) << 4) | (dist & 0x1f));
    }
    else
    {
      dist -= 2;
      if ((MomCPU == cpu_cr16b) && memory_model)
      {
        set_w_guessed(flags, CodeLen >> 1, 1, 0x001e);
        append_word(0x7400 | code | ((dist >> 12) & 0x10) | ((dist >> 16) & 0x0e));
        set_w_guessed(flags, CodeLen >> 1, 1, 0xffff);
        append_word((dist & 0xfffe) | ((dist >> 20) & 1));
      }
      else
      {
        set_w_guessed(flags, CodeLen >> 1, 1, 0x0010);
        append_word(0x140e | code | ((dist & 0x10000ul) >> 12));
        set_w_guessed(flags, CodeLen >> 1, 1, 0xffff);
        append_word(dist & 0xfffful);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bal(Word code)
 * \brief  handle BAL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_bal(Word code)
{
  Word Rlink;
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(2, 2)
   && decode_ireg_dest(&ArgStr[1], &Rlink)
   && decode_code_addr(&ArgStr[2], &dest, &flags))
  {
    LongInt dist = dest - (EProgCounter() + 4);

    if ((MomCPU == cpu_cr16b) && memory_model)
    {
      set_w_guessed(flags, CodeLen >> 1, 1, 0x001e);
      append_word(code | (Rlink << 5) | ((dist >> 12) & 0x10) | ((dist >> 16) & 0x0e));
      set_w_guessed(flags, CodeLen >> 1, 1, 0xffff);
      append_word((dist & 0xfffe) | ((dist >> 20) & 1));
    }
    else
    {
      set_w_guessed(flags, CodeLen >> 1, 1, 0x0010);
      append_word(code | (Rlink << 5) | ((dist & 0x10000ul) >> 12));
      set_w_guessed(flags, CodeLen >> 1, 1, 0xffff);
      append_word(dist & 0xfffful);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jal(Word code)
 * \brief  handle JAL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jal(Word code)
{
  Word Rlink, Rtarget;

  if (ChkArgCnt(2, 2)
   && decode_ireg_dest(&ArgStr[1], &Rlink)
   && decode_ireg_dest(&ArgStr[2], &Rtarget))
    append_word(code | (Rlink << 5) | (Rtarget << 1));
}

/*!------------------------------------------------------------------------
 * \fn     decode_excp(Word code)
 * \brief  handle EXCP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_excp(Word code)
{
  Word vector;

  if (ChkArgCnt(1, 1)
   && decode_vector(&ArgStr[1], &vector))
    append_word(code | (vector << 1));
}

/*!------------------------------------------------------------------------
 * \fn     decode_lpr_spr(Word code)
 * \brief  handle LPR/SPR instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_lpr_spr(Word code)
{
  Word Rsrc_dest, Rproc;
  const Boolean is_spr = !!(code & 0x0200);

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[is_spr + 1], &Rsrc_dest, True)
   && decode_cpu_reg(&ArgStr[2 - is_spr], &Rproc))
    append_word(code | (Rproc << 5) | (Rsrc_dest << 1));
}

/*!------------------------------------------------------------------------
 * \fn     decode_load_store(Word code)
 * \brief  handle LOADi/STORi instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_load_store(Word code)
{
  adr_vals_t reg_adr_vals, mem_adr_vals;
  const Boolean is_stor = !!(code & 0x4000);
  const tStrComp *p_mem_arg = &ArgStr[is_stor + 1],
                 *p_reg_arg = &ArgStr[2 - is_stor];

  if (!ChkArgCnt(2, 2))
    return;

  switch (decode_adr(p_reg_arg, &reg_adr_vals, Int4))
  {
    case e_mode_reg:
      code |= reg_adr_vals.part << 5;
      break;
    case e_mode_imm:
      if (!is_stor || (MomCPU < cpu_cr16b))
        goto wrong;
      break;
    case e_mode_none:
      return;
    default:
    wrong:
      WrStrErrorPos(ErrNum_InvAddrMode, p_reg_arg);
  }

  switch (decode_adr(p_mem_arg, &mem_adr_vals, Int16))
  {
    case e_mode_disp_base_short:
      if (reg_adr_vals.mode == e_mode_imm)
        goto stor_imm_disp;
      set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0x1e01);
      append_word(code | (mem_adr_vals.part << 1) | (mem_adr_vals.vals[0] & 1) | ((mem_adr_vals.vals[0] & 0x1e) << 8));
      break;
    case e_mode_disp_base_medium:
      if (reg_adr_vals.mode == e_mode_imm)
        goto stor_imm_disp;
      set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0x0600);
      append_word(code | 0x1001 | (mem_adr_vals.part << 1) | ((mem_adr_vals.vals[1] & 3) << 9));
      set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
      append_word(mem_adr_vals.vals[0]);
      break;
    case e_mode_disp_pair_medium:
      if (reg_adr_vals.mode == e_mode_imm)
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[is_stor + 1]);
      else
      {
        set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0x0600);
        append_word(code | 0x1801 | (mem_adr_vals.part << 1) | ((mem_adr_vals.vals[1] & 3) << 9));
        set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
        append_word(mem_adr_vals.vals[0]);
      }
      break;
    case e_mode_abs:
      if (reg_adr_vals.mode == e_mode_imm)
      {
        set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0x0120);
        or_w_guessed(reg_adr_vals.disp_flags, CodeLen >> 1, 1, 0x001e);
        append_word(0x04c0 | (code & 0x2000) | ((mem_adr_vals.vals[1] & 1) << 5) | ((mem_adr_vals.vals[1] & 2) << 7) | (reg_adr_vals.vals[0] << 1));
      }
      else
      {
        set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0x0600);
        append_word(code | 0x181f |  ((mem_adr_vals.vals[1] & 3) << 9));
      }
        set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
        append_word(mem_adr_vals.vals[0]);
      break;
    case e_mode_none:
      return;
    stor_imm_disp:
    {
      LongInt disp = get_displacement(&mem_adr_vals);

      if (!isreg_0189(mem_adr_vals.part)) WrStrErrorPos(ErrNum_InvAddrMode, p_mem_arg);
      else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.disp_flags) || ChkRangePos(disp, -32768, 32767, p_mem_arg))
      {
        set_w_guessed(reg_adr_vals.disp_flags, CodeLen >> 1, 1, 0x001e);
        append_word((disp ? 0x04c1 : 0x44c1) | (code & 0x2000) | (mem_adr_vals.part << 5) | (reg_adr_vals.vals[0] << 1));
        if (disp)
        {
          set_w_guessed(mem_adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
          append_word(disp & 0xffff);
        }
      }
      break;
    }
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, p_mem_arg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_push_pop(Word code)
 * \brief  handle PUSH/POP(RET) instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_push_pop(Word code)
{
  Word Rsrc_dest;
  Word count;
  tSymbolFlags count_flags;

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(cpu_cr16b)
   && decode_reg(&ArgStr[ArgCnt], &Rsrc_dest, True))
  {
    if (ArgCnt == 1)
    {
      count = 0;
      count_flags = eSymbolFlag_None;
    }
    else if (!decode_14(&ArgStr[1], &count, &count_flags))
      return;
    set_w_guessed(count_flags, CodeLen >> 1, 1, 3 << 5);
    append_word(code | (Rsrc_dest << 1) | (count << 5));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_movd(Word code)
 * \brief  handle MOVD instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_movd(Word code)
{
  Word Rdest;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(cpu_cr16b)
   && decode_ireg_pair(&ArgStr[2], &Rdest))
  {
    Boolean ok;
    tSymbolFlags flags;
    LongWord imm = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(ArgStr[1].str.p_str[0] == '$'), Int21, &ok, &flags);
    if (ok)
    {
      set_w_guessed(flags, CodeLen >> 1, 1, 0x021e);
      append_word(code | (Rdest << 5) | ((imm >> 11) & 0x0200) | ((imm >> 16) & 0x000e) | ((imm >> 12) & 0x0010));
      set_w_guessed(flags, CodeLen >> 1, 1, 0xffff);
      append_word(imm & 0xffff);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmp_br(Word code)
 * \brief  handle compare-and-branch instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cmp_br(Word code)
{
  Word Rdest;
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(cpu_cr16b)
   && decode_reg(&ArgStr[1], &Rdest, True)
   && decode_code_addr(&ArgStr[2], &dest, &flags))
  {
    LongInt dist = dest - (EProgCounter() + 2);

    if (!isreg_0189(Rdest)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else if (!mFirstPassUnknownOrQuestionable(flags) && ((dist < 0) || (dist > 31))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
    else
    {
      set_w_guessed(flags, CodeLen >> 1, 1, 0x001e);
      append_word(code | (Rdest << 5) | (dist & 0x1e));
    }
  } 
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem_bit(Word code)
 * \brief  handle xBITi instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mem_bit(Word code)
{
  const Boolean is_word = !!(code & 0x2000);

  if (ChkMinCPU(cpu_cr16b)
   && ChkArgCnt(2, 2))
  {
    Boolean ok;
    tSymbolFlags position_flags;
    Word position = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(ArgStr[1].str.p_str[0] == '$'), is_word ? UInt4 : UInt3, &ok, &position_flags);
    adr_vals_t adr_vals;

    if (ok)
      switch (decode_adr(&ArgStr[2], &adr_vals, is_word ? Int16 : Int8))
      {
        case e_mode_disp_base_short:
        case e_mode_disp_base_medium:
        {
          LongInt disp = get_displacement(&adr_vals);
          if (!mFirstPassUnknownOrQuestionable(adr_vals.disp_flags))
          {
            if (!ChkRangePos(disp, -32768, 32767, &ArgStr[2]))
              return;
          }
          if (!isreg_0189(adr_vals.part))
            goto wrong;
          set_w_guessed(position_flags, CodeLen >> 1, 1, (is_word ? 0xf : 0x7) << 1);
          append_word(code | (disp ? 0x0001 : 0x4001) | (position << 1) | (adr_vals.part << 5));
          if (disp)
          {
            set_w_guessed(adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
            append_word(disp & 0xffff);
          }
          break;
        }
        case e_mode_abs:
        {
          set_w_guessed(position_flags, CodeLen >> 1, 1, (is_word ? 0xf : 0x7) << 1);
          or_w_guessed(adr_vals.disp_flags, CodeLen >> 1, 1, 0x9 << 5);
          append_word(code | 0x0000 | (position << 1) | ((adr_vals.vals[1] & 1) << 5) | ((adr_vals.vals[1] & 2) << 7));
          set_w_guessed(adr_vals.disp_flags, CodeLen >> 1, 1, 0xffff);
          append_word(adr_vals.vals[0]);
          break;
        }
        case e_mode_none:
          break;
        default:
        wrong:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      }
  }
}

/*!------------------------------------------------------------------------
 * \fn     void decode_loadm_storm(Word code)
 * \brief  handle LOADM_STORM instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_loadm_storm(Word code)
{
  Word count;
  tSymbolFlags count_flags;

  if (ChkMinCPU(cpu_cr16b)
   && ChkArgCnt(1, 1)
   && decode_14(&ArgStr[1], &count, &count_flags))
  {
    set_w_guessed(count_flags, CodeLen >> 1, 1, 3 << 5);
    append_word(code | (count << 5));
  }
}

/*!------------------------------------------------------------------------
 * \fn     check_pc_even(Word code)
 * \brief  before handling machine instructions, check whether PC is on an
 *         even address
 * ------------------------------------------------------------------------ */

static void check_pc_even(Word code)
{
  UNUSED(code);
  if (EProgCounter() & 1)
    WrError(ErrNum_AddrNotAligned);
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static void add_regimm_reg_8_16(const char *p_name, Word code, IntType int_type_b, IntType int_type_w)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, code | int_type_b, decode_regimm_reg);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  AddInstTable(InstTable, name, code | int_type_w | 0x2000, decode_regimm_reg);
}

static void add_condition(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "S%s", p_name);
  AddInstTable(InstTable, name, 0x6e00 | (code << 5), decode_one_reg);
  as_snprintf(name, sizeof(name), "J%s", p_name);
  AddInstTable(InstTable, name, (((MomCPU == cpu_cr16b) && memory_model) ? 0x1601 : 0x5401) | (code << 5), decode_jump);
  as_snprintf(name, sizeof(name), "B%s", p_name);
  AddInstTable(InstTable, name, code << 5, decode_br);
}

static void add_cmp_br(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%s0B", p_name);
  AddInstTable(InstTable, name, code | 0x0000, decode_cmp_br);
  as_snprintf(name, sizeof(name), "%s1B", p_name);
  AddInstTable(InstTable, name, code | 0x0040, decode_cmp_br);
  as_snprintf(name, sizeof(name), "%s0W", p_name);
  AddInstTable(InstTable, name, code | 0x2000, decode_cmp_br);
  as_snprintf(name, sizeof(name), "%s1W", p_name);
  AddInstTable(InstTable, name, code | 0x2040, decode_cmp_br);
}

static void add_mem_bit(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, code, decode_mem_bit);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  AddInstTable(InstTable, name, code | 0x2000, decode_mem_bit);
}

static void add_dot_pseudo(const char *p_name, Word code, InstProc decode_fnc)
{
  AddInstTable(InstTable, p_name, code, decode_fnc);
  if (*p_name == '.')
    AddInstTable(InstTable, p_name + 1, code, decode_fnc);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);

  add_null_pseudo(InstTable);

  inst_table_set_prefix_proc(InstTable, check_pc_even, 0);
  AddInstTable(InstTable, "NOP"   , NOPCode, decode_fixed);
  AddInstTable(InstTable, "DI"    , 0x7dde , decode_fixed);
  AddInstTable(InstTable, "EI"    , 0x7dfe , decode_fixed);
  AddInstTable(InstTable, "WAIT"  , 0x7ffe , decode_fixed);
  AddInstTable(InstTable, "RETX"  , 0x79fe , decode_fixed);
  AddInstTable(InstTable, "EIWAIT", 0x7fe6 , decode_fixed_b);

  AddInstTable(InstTable, "JUMP", ((MomCPU == cpu_cr16b) && memory_model) ? 0x17c1 : 0x55c1 , decode_jump);

  AddInstTable(InstTable, "MOVXB", 0x6800, decode_two_reg);
  AddInstTable(InstTable, "MOVZB", 0x6a00, decode_two_reg);
  AddInstTable(InstTable, "JAL"  , ((MomCPU == cpu_cr16b) && memory_model) ? 0x1600 : 0x7401, decode_jal);

  add_regimm_reg_8_16("MOV" , 0x1800, Int8, Int16);
  add_regimm_reg_8_16("ADD" , 0x0000, Int8, Int16);
  add_regimm_reg_8_16("ADDU", 0x0200, Int8, Int16);
  add_regimm_reg_8_16("ADDC", 0x1200, Int8, Int16);
  add_regimm_reg_8_16("MUL" , 0x0600, SInt8, SInt16);
  add_regimm_reg_8_16("SUB" , 0x1e00, Int8, Int16);
  add_regimm_reg_8_16("SUBC", 0x1a00, Int8, Int16);
  add_regimm_reg_8_16("CMP" , 0x0e00, Int8, Int16);
  add_regimm_reg_8_16("AND" , 0x1000, Int8, Int16);
  add_regimm_reg_8_16("OR"  , 0x1c00, Int8, Int16);
  add_regimm_reg_8_16("XOR" , 0x0c00, Int8, Int16);
  add_regimm_reg_8_16("ASHU", 0x0800, SInt4, SInt5);
  add_regimm_reg_8_16("LSH" , 0x0a00, SInt4, SInt5);
  AddInstTable(InstTable, "TBIT", 0x3600 | UInt4, decode_regimm_reg);

  add_condition("EQ", 0);
  add_condition("NE", 1);
  add_condition("GE", 13);
  add_condition("CS", 2);
  add_condition("CC", 3);
  add_condition("HI", 4);
  add_condition("LS", 5);
  add_condition("LO", 10);
  add_condition("HS", 11);
  add_condition("GT", 6);
  add_condition("LE", 7);
  add_condition("FS", 8);
  add_condition("FC", 9);
  add_condition("LT", 12);
  AddInstTable(InstTable, "BR", 0x0e << 5, decode_br);
  AddInstTable(InstTable, "BAL", ((MomCPU == cpu_cr16b) && memory_model) ? 0x7600 : 0x340e, decode_bal);
  AddInstTable(InstTable, "EXCP", 0x7be0, decode_excp);
  AddInstTable(InstTable, "LPR", 0x7000, decode_lpr_spr);
  AddInstTable(InstTable, "SPR", 0x7200, decode_lpr_spr);
  AddInstTable(InstTable, "LOADB", 0x8000, decode_load_store);
  AddInstTable(InstTable, "LOADW", 0xa000, decode_load_store);
  AddInstTable(InstTable, "STORB", 0xc000, decode_load_store);
  AddInstTable(InstTable, "STORW", 0xe000, decode_load_store);

  AddInstTable(InstTable, "PUSH"  , 0x6c00, decode_push_pop);
  AddInstTable(InstTable, "POP"   , 0x6c80, decode_push_pop);
  AddInstTable(InstTable, "POPRET", memory_model ? 0x6d80 : 0x6d00, decode_push_pop);
  AddInstTable(InstTable, "MOVD"  , 0x6400, decode_movd);
  AddInstTable(InstTable, "MULSB" , 0x6000, decode_two_reg_b);
  AddInstTable(InstTable, "MULSW" , 0x6200, decode_reg_regpair_b);
  AddInstTable(InstTable, "MULUW" , 0x7e00, decode_reg_regpair_b);
  add_cmp_br("BEQ", 0x1401);
  add_cmp_br("BNE", 0x1481);
  add_mem_bit("TBIT", 0x0480);
  add_mem_bit("CBIT", 0x0400);
  add_mem_bit("SBIT", 0x0440);
  AddInstTable(InstTable, "LOADM", 0x7e04, decode_loadm_storm);
  AddInstTable(InstTable, "STORM", 0x7e84, decode_loadm_storm);

  inst_table_set_prefix_proc(InstTable, NULL, 0);
  add_dot_pseudo(".REG", 0, CodeREG);
  add_dot_pseudo(".BYTE"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
  add_dot_pseudo(".DC8"   , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
  add_dot_pseudo(".WORD"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDW);
  add_dot_pseudo(".DC16"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDW);
  add_dot_pseudo(".LONG"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDD);
  add_dot_pseudo(".DC24"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDP);
  add_dot_pseudo(".DC32"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDD);
  add_dot_pseudo(".DC64"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDQ);
  add_dot_pseudo(".FLOAT" , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat, DecodeIntelDD);
  add_dot_pseudo(".DF32"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat, DecodeIntelDD);
  add_dot_pseudo(".DOUBLE", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat, DecodeIntelDQ);
  add_dot_pseudo(".DF64"  , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat, DecodeIntelDQ);
  add_dot_pseudo(".DS8"   , 1, DecodeIntelDS);
  add_dot_pseudo(".BLKB"  , 1, DecodeIntelDS);
  add_dot_pseudo(".SPACE" , 1, DecodeIntelDS);
  add_dot_pseudo(".DS16"  , 2, DecodeIntelDS);
  add_dot_pseudo(".BLKW"  , 2, DecodeIntelDS);
  add_dot_pseudo(".DS32"  , 4, DecodeIntelDS);
  add_dot_pseudo(".BLKF"  , 4, DecodeIntelDS);
  add_dot_pseudo(".BLKL"  , 4, DecodeIntelDS);
  add_dot_pseudo(".DS64"  , 8, DecodeIntelDS);
  add_dot_pseudo(".BLKD"  , 8, DecodeIntelDS);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     make_code_cr16(void)
 * \brief  encode target-specific instruction
 * ------------------------------------------------------------------------ */

static void make_code_cr16(void)
{
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_cr16(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_cr16(void)
{
  return Memo("REG") || Memo(".REG");
}

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_cr16(char *p_arg, TempResult *p_result)
 * \brief  handle built-in (register) symbols for CR16
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_cr16(char *p_arg, TempResult *p_result)
{
  Word reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_cr16;
  }
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_cr16(void *p_user)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_cr16(void *p_user)
{
  const TFamilyDescr *p_descr = FindFamilyByName("CR16A/B");
  (void)p_user;

  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0x0200;
  DivideChars = ",";
  HasAttrs = False;
  AttrChars = "";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  if (MomCPU == cpu_cr16a)
  {
    addr_int_type = UInt18;
    code_int_type = UInt17;
  }
  else
  {
    addr_int_type = UInt21;
    code_int_type = memory_model ? UInt21 : UInt17;
  }
  SegLimits[SegCode] = IntTypeDefs[addr_int_type].Max;

  MakeCode = make_code_cr16;
  IsDef = is_def_cr16;
  DissectReg = dissect_reg_cr16;
  InternSymbol = intern_symbol_cr16;
  SwitchFrom = deinit_fields;
  init_fields();
}

/*!------------------------------------------------------------------------
 * \fn     codecr16_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

void codecr16_init(void)
{
  static const tCPUArg cr16_args[] =
  {
    { "model", 0, 1, 1, &memory_model },
    { NULL   , 0, 0, 0, NULL          }
  };

  cpu_cr16a = AddCPUUser("CR16A", switch_to_cr16, NULL, NULL);
  cpu_cr16b = AddCPUUserWithArgs("CR16B", switch_to_cr16, NULL, NULL, cr16_args);
}
