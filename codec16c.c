/* codec16c.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Code Generator National Semiconductor CR16C                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include "bpemu.h"
#include <string.h>
#include <assert.h>
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
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "onoff_common.h"
#include "assume.h"
#include "codecr16.h"
#include "codec16c.h"

/* ------------------------------------------------------------------------ */

static CPUVar cpu_cr16c;

typedef struct
{
  Word code_reg, code_imm4_16, code_imm20, code_imm32;
} alu32_order_t;

static alu32_order_t *alu32_orders;
static LongInt cfg_sr;

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

/*--------------------------------------------------------------------------*/
/* Register Handling */

#define REG_SP 15
#define REG_RA 14

/*!------------------------------------------------------------------------
 * \fn     decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
 * \brief  parse CPU register name
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \return True if argument is CPU register
 * ------------------------------------------------------------------------ */

static Boolean decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
{
  typedef enum { e_idle, e_sp, e_regnum, e_lh_none, e_lh, e_none } reg_parse_state_t;
  reg_parse_state_t parse_state = e_idle;
  Boolean low = False, high = False;
  char last = '\0';

  *p_result = 0;
  *p_size = eSymbolSizeUnknown;
  for (; ; last = *p_arg++)
    switch (as_toupper(*p_arg))
    {
      case 'R':
        if (parse_state != e_idle)
          return False;
        parse_state = e_regnum;
        break;
      case 'A':
        if ((parse_state != e_regnum) || (as_toupper(last) != 'R'))
          return False;
        *p_result = REG_RA;
        parse_state = e_lh_none;
        break;
      case 'S':
        if (parse_state != e_idle)
          return False;
        parse_state = e_sp;
        *p_result = REG_SP;
        break;
      case 'P':
        if (parse_state != e_sp)
          return False;
        *p_result = REG_SP;
        parse_state = e_lh_none;
        break;
      case '_':
        if ((parse_state != e_regnum) && (parse_state != e_lh_none))
          return False;
        parse_state = e_lh;
        break;
      case 'L':
        if (parse_state != e_lh)
          return False;
        low = True;
        parse_state = e_none;
        break;
      case 'H':
        if (parse_state != e_lh)
          return False;
        high = True;
        parse_state = e_none;
        break;
      case '\0':
        if ((parse_state != e_sp) && (parse_state != e_regnum) && (parse_state != e_lh_none) && (parse_state != e_none))
          return False;
        else if (low)
        {
          if (*p_result < 12)
            return False;
          *p_size = eSymbolSize16Bit;
          return True;
        }
        else if (high)
        {
          if (*p_result != REG_SP)
            return False;
          *p_result += 16;
          *p_size = eSymbolSize16Bit;
          return True;
        }
        else
        {
          *p_size = (*p_result < 12) ? eSymbolSize16Bit : eSymbolSize32Bit;
          return True;
        }
      default:
        if (as_isdigit(*p_arg))
        {
          if (parse_state != e_regnum)
            return False;
          *p_result = (*p_result * 10) + (*p_arg - '0');
          if (*p_result > 13)
            return False;
        }
        else
          return False;
    }
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_cr16c(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  translate register # back to textual form
 * \param  p_dest destination text buffer
 * \param  dest_size capacity of buffer
 * \param  value register #
 * \param  inp_size register's operand size
 * ------------------------------------------------------------------------ */

static void pr_reg(char *p_dest, size_t dest_size, tRegInt value, const char *p_suffix)
{
  switch (value)
  {
    case REG_SP:
      as_snprintf(p_dest, dest_size, "SP%s", p_suffix);
      break;
    case REG_RA:
      as_snprintf(p_dest, dest_size, "RA%s", p_suffix);
      break;
    default:
      as_snprintf(p_dest, dest_size, "R%u%s", (unsigned)value, p_suffix);
  }
}

static void dissect_reg_cr16c(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize16Bit:
      if (value <= 15)
        pr_reg(p_dest, dest_size, value, (value >= 12) ? "_L" : "");
      else if (value == REG_SP + 16)
        pr_reg(p_dest, dest_size, value & 15, "_H");
      else
        goto noneofall;
      break;
    case eSymbolSize32Bit:
      if ((value >= 12) && (value <= 15))
        pr_reg(p_dest, dest_size, value, "");
      else
        goto noneofall;
      break;
    default:
    noneofall:
      as_snprintf(p_dest, dest_size, "%d-%u", (unsigned)inp_size, (unsigned)value);
  }
}

/*--------------------------------------------------------------------------*/
/* Argument Decoding */

typedef enum
{
  e_mode_none = -1,
  e_mode_reg = 0,
  e_mode_imm = 1,
  e_mode_disp_reg = 2,
  e_mode_disp_rp = 3,
  e_mode_disp_rrp = 4,
  e_mode_abs = 5,
  e_mode_abs_rel = 6
} adr_mode_t;

typedef struct
{
  adr_mode_t mode;
  Word part, index_reg;
  int val_cnt;
  tSymbolFlags imm_disp_flags;
  LongWord imm_value;
  LongInt disp_value;
  Word vals[2];
} adr_vals_t;

static adr_mode_t reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_mode_none;
  p_vals->part = 0;
  p_vals->index_reg = 0;
  p_vals->imm_value = 0;
  p_vals->disp_value = 0;
  p_vals->val_cnt = 0;
  p_vals->imm_disp_flags = eSymbolFlag_None;
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_result_size, tSymbolSize req_size, Boolean must_be_reg)
 * \brief  decode register argument
 * \param  p_arg source argument
 * \param  p_result returns register #
 * \param  p_result_size returns register size (16/32 bit)
 * \param  req_size expected register size (16/24/32 bit, 24 bit to get registers in 'native full size')
 * \param  must_be_reg argument must be  a register
 * \return OK/No/Abort
 * ------------------------------------------------------------------------ */

static tRegEvalResult decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_result_size, tSymbolSize req_size, Boolean must_be_reg)
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

  if ((reg_eval_result == eIsReg) && (req_size != eSymbolSizeUnknown))
  {
    if (req_size == eSymbolSize24Bit)
      req_size = (reg_descr.Reg >= 12) ? eSymbolSize32Bit : eSymbolSize16Bit;

    if (eval_result.DataSize != req_size)
    {
      WrStrErrorPos(ErrNum_InvOpSize, p_arg);
      reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
    }
  }

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  if (p_result_size) *p_result_size = eval_result.DataSize;
  return reg_eval_result;
}

static IntType op_size_int_type(tSymbolSize op_size)
{
  switch (op_size)
  {
    case eSymbolSize8Bit:
      return Int8;
    case eSymbolSize16Bit:
      return Int16;
    case eSymbolSize32Bit:
      return Int32;
    default:
      return UInt1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, tSymbolSize op_size, Boolean mem_op)
 * \brief  parse address expression
 * \param  p_arg source argument
 * \param  p_vals parse result
 * \param  op_size operand size (8/16/32 bit)
 * \param  mem_op (r32) or (r16+1,r16) is memory operand and may have displacement,
           otherwise register pair resp. single 32 bit register
 * \return deduced address mode
 * ------------------------------------------------------------------------ */

static void check_rrp_rp(adr_vals_t *p_vals, const tStrComp *p_arg)
{
  if (!p_vals->index_reg)
    p_vals->mode = e_mode_disp_rp;
  else if (cfg_sr)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    reset_adr_vals(p_vals);
  }
  else
  {
    if ((p_vals->part <= 10) && !(p_vals->part & 1))
      p_vals->part = (p_vals->part >> 1);
    else if ((p_vals->part == 3) || (p_vals->part == 5))
      p_vals->part = (p_vals->part >> 1) + 5;
    else
    {
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      reset_adr_vals(p_vals);
      return;
    }
    p_vals->index_reg &= 1;
    p_vals->part |= (p_vals->index_reg << 3);
    p_vals->mode = e_mode_disp_rrp;
  }
}

static adr_mode_t decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals, tSymbolSize op_size, Boolean mem_op)
{
  Boolean ok;
  int split_pos, arg_len;
  tStrComp index_remainder;

  reset_adr_vals(p_vals);

  if (*p_arg->str.p_str == '$')
  {
    LongWord imm_value =
    p_vals->imm_value = EvalStrIntExpressionOffsWithFlags(p_arg, 1, op_size_int_type(op_size),  &ok, &p_vals->imm_disp_flags);
    if (!ok)
      return reset_adr_vals(p_vals);

    /* for imm4, 9 is interpreted as -1 and 11 as escape code for extension word.  So
       these values cannot be encoded without extension words: */

    p_vals->val_cnt = (imm_value == 9) || (imm_value == 11);

    /* Otherwise, -1 may be encoded as imm4 value.  The encoding of
       -1 depends on operand size: */

    switch (op_size)
    {
      case eSymbolSize8Bit:
        p_vals->vals[0] = p_vals->imm_value & 0xff;
        if (p_vals->vals[0] == 0xff)
          imm_value = 9;
        else if (imm_value > 15)
          p_vals->val_cnt = 1;
        break;
      case eSymbolSize16Bit:
        p_vals->vals[0] = p_vals->imm_value & 0xffffu;
        if (p_vals->vals[0] == 0xffffu)
          imm_value = 9;
        else if (imm_value > 15)
          p_vals->val_cnt = 1;
        break;
      case eSymbolSize32Bit:
        p_vals->vals[0] = p_vals->imm_value & 0xffffu;
        p_vals->vals[1] = (p_vals->imm_value >> 16) & 0xffffu;
        if (imm_value == 0xfffffffful)
          imm_value = 9;
        else if (imm_value > 15)
          p_vals->val_cnt = 1;
        p_vals->val_cnt <<= 1;
        break;
      default:
        break;
    }
    p_vals->part = p_vals->val_cnt ? 11 : (imm_value & 0x001f);
    p_vals->mode = e_mode_imm;
    goto check;
  }

  /* split off index register */

  if (*p_arg->str.p_str == '[')
  {
    char *p_split = QuotPos(&p_arg->str.p_str[1], ']');
    if (p_split)
    {
      tStrComp index_arg;
      String index_str;

      StrCompMkTemp(&index_arg, index_str, sizeof index_str);
      StrCompCopySub(&index_arg, p_arg, 1, p_split - p_arg->str.p_str - 1);
      KillPostBlanksStrComp(&index_arg);
      KillPrefBlanksStrComp(&index_arg);
      switch (decode_reg(&index_arg, &p_vals->index_reg, NULL, eSymbolSize32Bit, False))
      {
        case eRegAbort:
          return reset_adr_vals(p_vals);
        case eIsNoReg:
          break;
        case eIsReg:
          if ((p_vals->index_reg < 12) || (p_vals->index_reg > 13))
          {
            WrStrErrorPos(ErrNum_InvIndexReg, &index_arg);
            return reset_adr_vals(p_vals);
          }
          else if (cfg_sr)
          {
            WrStrErrorPos(ErrNum_InvAddrMode, &index_arg);
            return reset_adr_vals(p_vals);
          }
          StrCompRefRight(&index_remainder, p_arg, p_split - p_arg->str.p_str + 1);
          p_arg = &index_remainder;
          break;
      }
    }
  }

  split_pos = FindDispBaseSplitWithQualifier(p_arg->str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp disp_arg, base_arg;
    Word regs[2];
    tSymbolSize reg_sizes[2];
    size_t reg_cnt = 0;
    char *p_split;
    Boolean ok;

    StrCompSplitRef(&disp_arg, &base_arg, p_arg, p_arg->str.p_str + split_pos);
    KillPostBlanksStrComp(&disp_arg);
    StrCompShorten(&base_arg, 1);
    KillPrefBlanksStrCompRef(&base_arg); KillPostBlanksStrComp(&base_arg);

    if (disp_arg.str.p_str[0])
    {
      if (!mem_op)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &disp_arg);
        return reset_adr_vals(p_vals);
      }
      p_vals->disp_value = EvalStrIntExpressionWithFlags(&disp_arg, SInt21, &ok, &p_vals->imm_disp_flags);
      if (!ok)
        return reset_adr_vals(p_vals);
    }
    else
    {
      p_vals->disp_value = 0;
      ok = True;
      p_vals->imm_disp_flags = eSymbolFlag_None;
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
      if (eIsReg != decode_reg(&base_arg, &regs[reg_cnt], &reg_sizes[reg_cnt], eSymbolSizeUnknown, True))
        return reset_adr_vals(p_vals);
      reg_cnt++;
      if (p_split)
        base_arg = base_arg_remainder;
    }
    while (p_split);

    switch (reg_cnt)
    {
      case 1:
        p_vals->part = regs[0];
        if (mem_op)
        {
          if (reg_sizes[0] == eSymbolSize32Bit)
          {
            /* no 32 bit registers if CFG.SR is set: */
            if (cfg_sr)
            {
              WrStrErrorPos(ErrNum_InvBaseReg, p_arg);
              return reset_adr_vals(p_vals);
            }
            check_rrp_rp(p_vals, p_arg);
          }
          /* [R12/13] not usable with single 16 bit register: */
          else if (p_vals->index_reg)
            goto bad;
          else
            p_vals->mode = e_mode_disp_reg;
        }
        else
        {
          if (reg_sizes[0] != eSymbolSize32Bit)
            goto bad;
          if (cfg_sr)
          {
            WrStrErrorPos(ErrNum_InvReg, p_arg);
            return reset_adr_vals(p_vals);
          }
          p_vals->mode = e_mode_reg;
        }
        break;
      case 2:
        if ((reg_sizes[0] != eSymbolSize16Bit) || (reg_sizes[1] != eSymbolSize16Bit))
          goto bad;
        /* (R1,R0)...(R12_L,R11) allowed independent of CFG.SR:
           (R13_L,R12_L)...(SP_L,RA_L) only if CFG.SR==1: */
        if (regs[1] <= (cfg_sr ? 14 : 11))
        {
          if (regs[0] != regs[1] + 1)
            goto bad;
        }
        /* (SP_H,SP_L) allowed if CFG.SR==1: */
        else if ((regs[1] == REG_SP) && (regs[0] == REG_SP + 16) && cfg_sr) {}
        else
          goto bad;
        /* index registers not allowed if CFG.SR==1: */
        if (cfg_sr && p_vals->index_reg)
          goto bad;
        p_vals->part = regs[1];
        if (mem_op)
          check_rrp_rp(p_vals, p_arg);
        else
        {
          p_vals->mode = e_mode_reg;
        }
        break;
      default:
      bad:
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_vals);
    }
    goto check;
  }

  switch (decode_reg(p_arg, &p_vals->part, NULL, (op_size == eSymbolSize8Bit) ? eSymbolSize16Bit : op_size, False))
  {
    case eRegAbort:
      return reset_adr_vals(p_vals);
    case eIsReg:
      /* SP_H only allowed in combination with SP_L if CFG.SR==1 */
      if ((p_vals->part > 15)
       || (cfg_sr && (op_size == eSymbolSize32Bit)))
      {
        WrStrErrorPos(ErrNum_InvReg, p_arg);
        return reset_adr_vals(p_vals);
      }
      p_vals->mode = e_mode_reg;
      goto check;
    default:
    {
      p_vals->disp_value = EvalStrIntExpressionWithFlags(p_arg, p_vals->index_reg ? UInt20 : UInt24, &ok, &p_vals->imm_disp_flags);
      if (ok)
      {
        if (p_vals->index_reg)
        {
          p_vals->index_reg -= 12;
          p_vals->mode = e_mode_abs_rel;
        }
        else
          p_vals->mode = e_mode_abs;
      }
      goto check;
    }
  }

  WrStrErrorPos(ErrNum_InvAddrMode, p_arg);

check:
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize op_size)
 * \brief  decode argument that is expected to be a register
 * \param  p_arg source argument
 * \param  p_result parse result
 * \param  op_size operand size (8/16/32 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

Boolean decode_adr_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize op_size)
{
  adr_vals_t adr_vals;
  switch (decode_adr(p_arg, &adr_vals, op_size, False))
  {
    case e_mode_reg:
      *p_result = adr_vals.part;
      return True;
    case e_mode_none:
      return False;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr_reg_ra(const tStrComp *p_arg)
 * \brief  decode argument that is expected to be register RA
 * \param  p_arg source argument
 * \return True if success
 * ------------------------------------------------------------------------ */

Boolean decode_adr_reg_ra(const tStrComp *p_arg)
{
  Word reg;

  if (!decode_adr_reg(p_arg, &reg, eSymbolSize32Bit))
    return False;
  if (reg != REG_RA)
  {
    WrStrErrorPos(ErrNum_InvReg, p_arg);
    return False;
  }
  return True;
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

  *p_dest = EvalStrIntExpressionWithFlags(p_arg, UInt24, &ok, p_flags);
  if (ok && (*p_dest & 1))
  {
    if (mFirstPassUnknownOrQuestionable(*p_flags))
      *p_dest &= ~1;
    else
    {
      WrStrErrorPos(ErrNum_AddrMustBeEven, p_arg);
      ok = False;
    }
  }
  return ok;
}

/*!------------------------------------------------------------------------
 * \fn     is_abs20(LongWord address)
 * \brief  check whether address can be reached via 20 bit absolute mode
 * \param  address memory address to check
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_abs20(LongWord address)
{
  return (address <= 0xefffful)
      || ((address >= 0xff0000ul) && (address <= 0xfffffful));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cpu_reg(const tStrComp *p_arg, Word *p_dest, tSymbolSize size)
 * \brief  parse CPU register name
 * \param  p_arg source argument
 * \param  p_dest dest buffer for vector #
 * \param  size 16/32 bits
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

typedef struct
{
  char name[9];
  Byte num;
} cpu_reg_t;

static Boolean decode_cpu_reg(const tStrComp *p_arg, Word *p_dest, tSymbolSize size)
{
  static const cpu_reg_t cpu_regs_16[] =
  {
    { "DBS"     , 0x40 },
    { "DSR"     , 0x41 },
    { "DCRL"    , 0x42 },
    { "DCRH"    , 0x43 },
    { "CAR0L"   , 0x44 },
    { "CAR0H"   , 0x45 },
    { "CAR1L"   , 0x46 },
    { "CAR1H"   , 0x47 },
    { "CFG"     , 0x48 },
    { "PSR"     , 0x49 },
    { "INTBASEL", 0x4a },
    { "INTBASEH", 0x4b },
    { "ISPL"    , 0x4c },
    { "ISPH"    , 0x4d },
    { "USPL"    , 0x4e },
    { "USPH"    , 0x4f },
  },
  cpu_regs_32[] =
  {
    { "DBS"     , 0x40 },
    { "DSR"     , 0x41 },
    { "DCR"     , 0x42 },
    { "CAR0"    , 0x44 },
    { "CAR1"    , 0x46 },
    { "CFG"     , 0x48 },
    { "PSR"     , 0x49 },
    { "INTBASE" , 0x4a },
    { "ISP"     , 0x4c },
    { "USP"     , 0x4e },
  };
  size_t z, count = (size == eSymbolSize32Bit) ? as_array_size(cpu_regs_32) : as_array_size(cpu_regs_16);
  const cpu_reg_t *cpu_regs = (size == eSymbolSize32Bit) ? cpu_regs_32 : cpu_regs_16;

  for (z = 0; z < count; z++)
    if (!as_strcasecmp(p_arg->str.p_str, cpu_regs[z].name))
    {
      *p_dest = cpu_regs[z].num & 0x0f;
      return True;
    }

  count = (size == eSymbolSize32Bit) ? as_array_size(cpu_regs_16) : as_array_size(cpu_regs_32);
  cpu_regs = (size == eSymbolSize32Bit) ? cpu_regs_16 : cpu_regs_32;
  for (z = 0; z < count; z++)
    if (!as_strcasecmp(p_arg->str.p_str, cpu_regs[z].name))
    {
      WrStrErrorPos(ErrNum_InvOpSize, p_arg);
      return False;
    }
  
  WrStrErrorPos(ErrNum_InvProcReg, p_arg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     eval_count_18(const tStrComp *p_arg, Boolean *p_ok, tSymbolFlags *p_flags)
 * \brief  handle immediate count 1...8
 * \param  p_arg source argument
 * \param  p_ok success flag
 * \param  p_flags evaluation flags
 * \return encoded count (0..7 -> 1..8)
 * ------------------------------------------------------------------------ */

static Word eval_count_18(const tStrComp *p_arg, Boolean *p_ok, tSymbolFlags *p_flags)
{
  Word count = EvalStrIntExpressionOffsWithFlags(p_arg, !!(*p_arg->str.p_str == '$'), UInt4, p_ok, p_flags);

  if (!*p_ok)
    return count;
  if (!mFirstPassUnknownOrQuestionable(*p_flags) && !ChkRangePos(count, 1, 8, p_arg))
    *p_ok = False;
  return count - 1;
}

/*!------------------------------------------------------------------------
 * \fn     encode_fmt_xx(...)
 * \brief  encode instruction format XX
 * ------------------------------------------------------------------------ */

static void encode_fmt_1(Word opcode_16, Word p1, Word p2, Word p3, Word p4)
{
  append_word(opcode_16);
  append_word((p4 << 12) | (p3 << 8) | (p2 << 4) | p1);
}

static void encode_fmt_2(Word opcode_16, Word p1, Word p2, tSymbolFlags p2_flags, LongWord p3, tSymbolFlags p3_flags, Word p4)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  assert(p3 <= 0xffffful);
  assert(p4 <= 15);
  append_word(opcode_16);
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0x0f00u);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0x00f0u);
  append_word(p1 | (p2 << 4) | ((p3 >> 8) & 0x0f00u) | (p4 << 12));
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p3 & 0xffffu);
}

static void encode_fmt_3(Word opcode_16, LongWord p1, tSymbolFlags p1_flags, Word p2, tSymbolFlags p2_flags, Word p4)
{
  assert(p1 <= 0xfffffful);
  assert(p2 <= 15);
  assert(p4 <= 15);
  append_word(opcode_16);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0x0f0f);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0x00f0);
  append_word((p4 << 12) | ((p1 >> 8) & 0x0f00) | (p2 << 4) | ((p1 >> 20) & 0xf));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p1 & 0xffffu);
}

static void encode_fmt_3a(Word opcode_16, LongInt p1, tSymbolFlags p1_flags, Word p2, Word p4)
{
  /*
   * bit 0 of p1 must be 0
   * bits 15:1 of p1 into bits 15:1 of word 3
   * bits 19:16 of p1 into bits 11:8 of word 2
   * bits 23:20 of p1 into bits 3:0 of word 2
   * bit 24 of p1 into bit 0 of word 3
   */
  assert(!(p1 & 1));
  append_word(opcode_16);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0x0f0f);
  append_word((p4 << 12) | (p2 << 4) | ((p1 >> 8) & 0x0f00) | ((p1 >> 20) & 0x000f));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffff);
  append_word((p1 & 0xfffeul) | ((p1 >> 24) & 1));
}

static void encode_fmt_4(Word opcode_16)
{
  append_word(opcode_16);
}

static void encode_fmt_5(Word opcode_8, LongInt p1, tSymbolFlags p1_flags)
{
  /*
   * bit 0 of p1 must be 0
   * bits 15:1 of p1 into bits 15:1 of word 2
   * bits 23:16 of p1 into bits 7:0 of word 1
   * bit 24 of p1 into bit 0 of word 2
   */
  assert(!(p1 & 1));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0x00ff);
  append_word((opcode_8 << 8) | ((p1 >> 16) & 0xff));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffff);
  append_word((p1 & 0xfffeul) | ((p1 >> 24) & 1));
}

static void encode_fmt_6(Word opcode_13, Word p1)
{
  assert(p1 <= 7);
  append_word((opcode_13 << 3) | p1);
}

static void encode_fmt_7(Word opcode_9, LongWord p1, tSymbolFlags p1_flags, Word p2, tSymbolFlags p2_flags)
{
  assert(p1 <= 0xffffful);
  assert(p2 <= 7);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xf);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0xf << 4);
  append_word((opcode_9 << 7) | (p2 << 4) | ((p1 >> 16) & 0xf));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p1 & 0xffffu);
}

static void encode_fmt_8(Word opcode_8, LongWord p1, tSymbolFlags p1_flags, Word p2, tSymbolFlags p2_flags, Word p3)
{
  assert(p1 <= 0xffffful);
  assert(p2 <= 7);
  assert(p3 <= 1);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xf);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0x7 << 4);
  append_word((opcode_8 << 8) | (p3 << 7) | (p2 << 4) | ((p1 >> 16) & 0xf));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p1 & 0xffffu);
}

static void encode_fmt_9(Word opcode_9, Word p1, Word p2)
{
  assert(p1 <= 15);
  assert(p2 <= 7);
  append_word((opcode_9 << 7) | (p2 << 4) | p1);
}

static void encode_fmt_11(Word opcode_12, Word p1)
{
  append_word((opcode_12 << 4) | p1);
}

static void encode_fmt_12(Word opcode_8, LongWord p1, tSymbolFlags p1_flags, Word p2, tSymbolFlags p2_flags)
{
  assert(p1 <= 0xffffful);
  assert(p2 <= 15);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xf);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0xf << 4);
  append_word((opcode_8 << 8) | (p2 << 4) | ((p1 >> 16) & 0xf));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p1 & 0xffffu);
}

static void encode_fmt_13(Word opcode_7, LongWord p1, tSymbolFlags p1_flags, Word p2, tSymbolFlags p2_flags, Word p3)
{
  assert(p1 <= 0xffffful);
  assert(p2 <= 15);
  assert(p3 <= 1);
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xf);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0xf << 4);
  append_word((opcode_7 << 9) | (p3 << 8) | (p2 << 4) | ((p1 >> 16) & 0xf));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p1 & 0xffffu);
}

static void encode_fmt_14(Word opcode_8, Word p1, Word p2, Word p3)
{
  assert(p1 <= 15);
  assert(p2 <= 7);
  assert(p3 <= 1);
  append_word((opcode_8 << 8) | (p3 << 7) | (p2 << 4) | p1);
}

static void encode_fmt_15(Word opcode_8, Word p1, Word p2, tSymbolFlags p2_flags)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  set_w_guessed(p2_flags, CodeLen >> 1, 1, 0x00f0);
  append_word((opcode_8 << 8) | (p2 << 4) | p1);
}

static void encode_fmt_16(Word opcode_8, Word p1, Word p2, tSymbolFlags p2_flags, LongWord p3, tSymbolFlags p3_flags)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  assert(p3 <= 0xffffu);
  set_w_guessed(p2_flags, CodeLen >> 1, 1, 0x00f0);
  append_word((opcode_8 << 8) | (p2 << 4) | p1);
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0xffffu);
  append_word(p3);
}

static void encode_fmt_17(Word opcode_10, Word p1, Word p2, tSymbolFlags p2_flags, Word p3, tSymbolFlags p3_flags)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  assert(p3 <= 0x3fff);
  /*
   * bits 13:8 of p3 into bits 13:8 of word 2
   * bits 7:6 of p3 into bits 15:14 of word 2
   * bits 5:4 of p3 into bits 5:4 of word 1
   * bits 3:0 of p3 into bits 3:0 of word 2
   */
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0x0030);
  append_word((opcode_10 << 6) | (p3 & 0x30) | p1);
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0xff0fu);
  or_w_guessed(p2_flags, CodeLen >> 1, 1, 0x00f0u);
  append_word(((p3 << 8) & 0xc000u) | (p3 & 0x03f00) | (p2 << 4) | (p3 & 0xf));
}

static void encode_fmt_18(Word opcode_4, Word p1, Word p2, Word p3, tSymbolFlags p3_flags)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  assert(p3 <= 15);
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0xf << 8);
  append_word((opcode_4 << 12) | (p3 << 8) | (p2 << 4) | p1);
}

static void encode_fmt_19(Word opcode_4, Word p1, Word p2, Word p3, Word p4, tSymbolFlags p4_flags)
{
  assert(p1 <= 15);
  assert(p2 <= 15);
  assert(p3 <= 15);
  append_word((opcode_4 << 12) | (p3 << 8) | (p2 << 4) | p1);
  set_w_guessed(p4_flags, CodeLen >> 1, 1, 0xffff);
  append_word(p4);
}

static void encode_fmt_20(Word opcode_7, Word p1, Word p2)
{
  assert(p1 <= 15);
  assert(p2 <= 31);
  append_word((opcode_7 << 9) | (p2 << 4) | p1);
}

static void encode_fmt_21(Word opcode_4, LongInt p1, tSymbolFlags p1_flags, Word p2)
{
  /*
   * bit 0 of p1 must be 0
   * bits 8:5 of p1 into bits 11:8 of word 1
   * bits 4:1 of p1 into bits 3:0 of word 1
   */
  assert(!(p1 & 1));
  set_w_guessed(p1_flags, CodeLen >> 1, 1, 0x0f0f);
  append_word((opcode_4 << 12) | (p2 << 4) | ((p1 << 3) & 0x0f00) | ((p1 >> 1) & 0x000f));
}

static void encode_fmt_22(Word opcode_4, Word p1, Word p2, LongInt p3, tSymbolFlags p3_flags)
{
  /*
   * bit 0 of p3 must be 0
   * bits 15:1 of p3 into bits 15:1 of word 2
   * bit 16 of p3 into bit 0 of word 2
   */
  assert(!(p1 & 1));
  assert(!(p3 & 1));
  append_word((opcode_4 << 12) | ((p1 << 3) & 0x0f00) | ((p1 >> 1) & 0x000f) | (p2 << 4));
  set_w_guessed(p3_flags, CodeLen >> 1, 1, 0xffffu);
  append_word((p3 & 0xfffeul) | ((p3 >> 16) & 1));
}

static void encode_fmt23(Word opcode_12, Word p1, LongWord p2, tSymbolFlags p2_flags)
{
  assert(p1 <= 15);
  append_word((opcode_12 << 4) | p1);
  set_w_guessed(p2_flags, CodeLen >> 1, 2, 0xffffu);
  append_word(p2 & 0xffff);
  append_word((p2 >> 16) & 0xffff);
}

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    encode_fmt_4(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_br(Word condition)
 * \brief  handle branch instructions
 * \param  condition condition in machine code
 * ------------------------------------------------------------------------ */

static void decode_br(Word condition)
{
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(1, 1)
   && decode_code_addr(&ArgStr[1], &dest, &flags))
  {
    LongInt dist = dest - EProgCounter();

    if ((dist >= -254) && (dist <= 254))
      encode_fmt_21(1, dist, flags, condition);
    else if ((dist >= -65536l) && (dist <= 65534l))
      encode_fmt_22(1, 0x100, condition, dist, flags);
    else
      encode_fmt_3a(0x0010, dist, flags, condition, 0);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bal(Word code)
 * \brief  handle BAL instruction
 * \param  code machine code (RA version)
 * ------------------------------------------------------------------------ */

static void decode_bal(Word code)
{
  LongWord dest;
  Word link_reg;
  tSymbolFlags flags;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &link_reg, eSymbolSize32Bit)
   && decode_code_addr(&ArgStr[2], &dest, &flags))
  {
    LongInt dist = dest - EProgCounter();

    if (link_reg == REG_RA)
      encode_fmt_5(code, dist, flags);
    else
      encode_fmt_3a(0x0010, dist, flags, link_reg, 2);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jal(Word code)
 * \brief  handle JAL instruction
 * \param  code machine code (RA version)
 * ------------------------------------------------------------------------ */

static void decode_jal(Word code)
{
  Word dest_reg, link_reg;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &link_reg, eSymbolSize32Bit)
   && decode_adr_reg(&ArgStr[2], &dest_reg, eSymbolSize32Bit))
  {
    if (link_reg == REG_RA)
      encode_fmt_11(code, dest_reg);
    else
      encode_fmt_1(0x0014, link_reg, dest_reg, 0, 8);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bxx0(Word code)
 * \brief  handle BEQ0/BNE0 instructions
 * \param  code machine code (RA version)
 * ------------------------------------------------------------------------ */

static void decode_bxx0(Word code)
{
  Word reg;
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &reg, (code & 0x01) ? eSymbolSize16Bit : eSymbolSize8Bit)
   && decode_code_addr(&ArgStr[2], &dest, &flags))
  {
    LongInt dist = dest - EProgCounter();

    if (!mFirstPassUnknownOrQuestionable(flags) && ((dist < 2) || (dist > 32))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
    else
      encode_fmt_15(code,
                    reg,
                    (dist / 2) - 1, flags);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bxx1(Word code)
 * \brief  handle BEQ1/BNE1 instructions
 * \param  code machine codes (CMP opcode + brach condition)
 * ------------------------------------------------------------------------ */

static void decode_bxx1(Word code)
{
  Word reg;
  LongWord dest;
  tSymbolFlags flags;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &reg, (code & 0x0200) ? eSymbolSize16Bit : eSymbolSize8Bit)
   && decode_code_addr(&ArgStr[2], &dest, &flags))
  {
    LongInt dist = dest - (EProgCounter() + 2);

    if (!mFirstPassUnknownOrQuestionable(flags) && ((dist < 2) || (dist > 254))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
    else
    {
      encode_fmt_15(Hi(code), reg, 1, eSymbolFlag_None);
      encode_fmt_21(1, dist, flags, Lo(code));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_alu_8_16(Word code)
 * \brief  handle 8/16 bit instructions reg,reg or imm,rp
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_alu_8_16(Word code)
{
  Word dest_reg;
  tSymbolSize op_size = (code & 0x02) ? eSymbolSize16Bit : eSymbolSize8Bit;

  if (!ChkArgCnt(2, 2))
    return;
  if (decode_adr_reg(&ArgStr[2], &dest_reg, op_size))
  {
    adr_vals_t src_adr_vals;

    switch (decode_adr(&ArgStr[1], &src_adr_vals, op_size, False))
    {
      case e_mode_reg:
        encode_fmt_15(code | 1,
                      dest_reg,
                      src_adr_vals.part, eSymbolFlag_None);
        break;
      case e_mode_imm:
        if (src_adr_vals.val_cnt)
          encode_fmt_16(code,
                        dest_reg,
                        src_adr_vals.part, src_adr_vals.imm_disp_flags,
                        src_adr_vals.vals[0], src_adr_vals.imm_disp_flags);
        else
          encode_fmt_15(code,
                        dest_reg,
                        src_adr_vals.part, src_adr_vals.imm_disp_flags);
        break;
      case e_mode_none:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_alu_32(Word code)
 * \brief  handle 32 bit instructions rp,rp or imm,rp
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_alu_32(Word index)
{
  Word dest_reg;
  const alu32_order_t *p_order = &alu32_orders[index];

  if (!ChkArgCnt(2, 2))
    return;
  if (decode_adr_reg(&ArgStr[2], &dest_reg, eSymbolSize32Bit))
  {
    adr_vals_t src_adr_vals;

    switch (decode_adr(&ArgStr[1], &src_adr_vals, eSymbolSize32Bit, False))
    {
      case e_mode_reg:
        if (!p_order->code_reg)
          goto not_supp;
        else if (Lo(p_order->code_reg) == 0x14)
          encode_fmt_1(0x0014,
                       dest_reg, src_adr_vals.part, 0, p_order->code_reg >> 12);
        else
          encode_fmt_15(p_order->code_reg >> 8,
                        dest_reg,
                        src_adr_vals.part, eSymbolFlag_None);
        break;
      case e_mode_imm:
        if (!src_adr_vals.val_cnt && p_order->code_imm4_16)
          encode_fmt_15(p_order->code_imm4_16,
                        dest_reg,
                        src_adr_vals.part, src_adr_vals.imm_disp_flags); 
        else
        {
          if (p_order->code_imm4_16
           && ((src_adr_vals.imm_value <= 0x00007ffful)
            || (src_adr_vals.imm_value >= 0xffff8000ul)))
            encode_fmt_16(p_order->code_imm4_16,
                          dest_reg,
                          11, src_adr_vals.imm_disp_flags,
                          src_adr_vals.imm_value & 0xffffu, src_adr_vals.imm_disp_flags);
          else if (p_order->code_imm20
           && ((src_adr_vals.imm_value <= 0x0007fffful)
            || (src_adr_vals.imm_value >= 0xfff80000ul)))
            encode_fmt_12(p_order->code_imm20,
                          src_adr_vals.imm_value & 0xfffffu, src_adr_vals.imm_disp_flags,
                          dest_reg, eSymbolFlag_None);
          else if (p_order->code_imm32)
            encode_fmt23(p_order->code_imm32,
                         dest_reg,
                         src_adr_vals.imm_value, src_adr_vals.imm_disp_flags);
          else
            goto not_supp;
        }
        break;
      case e_mode_none:
        break;
      default:
      not_supp:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mov_ext(Word code)
 * \brief  handle sign/zero extension instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mov_ext(Word code)
{
  Word src_reg, dest_reg;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &src_reg, (code & 0x200) ? eSymbolSize16Bit : eSymbolSize8Bit)
   && decode_adr_reg(&ArgStr[2], &dest_reg, (code & 0x200) ? eSymbolSize32Bit : eSymbolSize16Bit))
    encode_fmt_15(code >> 8, dest_reg, src_reg, eSymbolFlag_None);
}

/*!------------------------------------------------------------------------
 * \fn     decode_mac(Word code)
 * \brief  handle MACxW instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mac(Word code)
{
  Word src1_reg, src2_reg, dest_reg;

  if (ChkArgCnt(3, 3)
   && decode_adr_reg(&ArgStr[1], &src1_reg, eSymbolSize16Bit)
   && decode_adr_reg(&ArgStr[2], &src2_reg, eSymbolSize16Bit)
   && decode_adr_reg(&ArgStr[3], &dest_reg, eSymbolSize32Bit))
    encode_fmt_1(0x0014, src2_reg, src1_reg, dest_reg, code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_mul(Word code)
 * \brief  handle MULS/MULU instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mul(Word code)
{
  Word src_reg, dest_reg;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[1], &src_reg, (code & 0xf0) ? eSymbolSize16Bit : eSymbolSize8Bit)
   && decode_adr_reg(&ArgStr[2], &dest_reg, (code & 0xf0) ? eSymbolSize32Bit : eSymbolSize16Bit))
    encode_fmt_15(code,  dest_reg, src_reg, eSymbolFlag_None);
}

/*!------------------------------------------------------------------------
 * \fn     decode_scond(Word code)
 * \brief  handle Scond instructions
 * \param  condition encoded condition
 * ------------------------------------------------------------------------ */

static void decode_scond(Word condition)
{
  Word reg;

  if (ChkArgCnt(1, 1)
   && decode_adr_reg(&ArgStr[1], &reg, eSymbolSize16Bit))
    encode_fmt_15(0x08, reg, condition, eSymbolFlag_None);
}

/*!------------------------------------------------------------------------
 * \fn     decode_jump(Word code)
 * \brief  handle jump instructions
 * \param  condition jump condition
 * ------------------------------------------------------------------------ */

static void decode_jump(Word condition)
{
  Word reg;

  if (ChkArgCnt(1, 1)
   && decode_adr_reg(&ArgStr[1], &reg, eSymbolSize32Bit))
    encode_fmt_15(0x0a, reg, condition, eSymbolFlag_None);
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
   && decode_cr16_vector(&ArgStr[1], &vector, e_cr16_gen_c))
    encode_fmt_11(code, vector);
}

/*!------------------------------------------------------------------------
 * \fn     decode_lpr_spr(Word code)
 * \brief  handle LPR(D)/SPR(D) instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_lpr_spr(Word code)
{
  Word reg_src_dest, reg_pr;
  const Boolean is_spr = !!(code & 2);
  tSymbolSize op_size = (code & 1) ? eSymbolSize32Bit : eSymbolSize16Bit;

  if (!SupAllowed)
  {
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
    return;
  }

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[is_spr + 1], &reg_src_dest, op_size)
   && decode_cpu_reg(&ArgStr[2 - is_spr], &reg_pr, op_size))
    encode_fmt_1(0x0014, reg_src_dest, reg_pr, 0, code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_push_pop(Word code)
 * \brief  handle PUSH/POP(RET) instructions
 * \param  code opcode(8)
 * ------------------------------------------------------------------------ */

static void decode_push_pop(Word code)
{
  Word count_m1 = 0, start_reg = 0, ra = 0;
  tSymbolFlags count_flags = eSymbolFlag_None;

  switch (ArgCnt)
  {
    /* count==0 is not encodable, so if only RA is to be handled, do not encode as ra: */
    case 1:
      if (!decode_adr_reg_ra(&ArgStr[1]))
        return;
      count_m1 = 0;
      start_reg = REG_RA;
      goto encode;
    case 3:
      if (!decode_adr_reg_ra(&ArgStr[3]))
        return;
      ra = 1;
      /* fall-thru */
    case 2:
    {
      Boolean ok;
      if (!decode_adr_reg(&ArgStr[2], &start_reg, eSymbolSize24Bit))
        return;
      count_m1 = eval_count_18(&ArgStr[1], &ok, &count_flags);
      if (!ok)
        return;
      goto encode;
    }
    default:
      (void)ChkArgCnt(1, 3);
      return;
    encode:
      if (!mFirstPassUnknownOrQuestionable(count_flags))
      {
        /*
         * For POPRET, do not allow start + count > 15 (i.e. restore SP)
         * For PUSH/POP, do not allow start + count > 16 (i.e. register file wrap around) (?)
         */
        if (start_reg + count_m1 >= (16 - (code == 0x03)))
        {
          WrError(ErrNum_InvRegList);
          return;
        }
        /* Do not allow specifying RA twice (?) */
        if ((start_reg <= REG_RA) && (start_reg + count_m1 >= REG_RA) && ra)
        {
          WrError(ErrNum_InvRegList);
          return;
        }
      }
      encode_fmt_14(code, start_reg, count_m1, ra);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_shift(Word index)
 * \brief  handle shift orders
 * \param  index index into insn table
 * ------------------------------------------------------------------------ */

typedef struct
{
  Word code_reg, code_left_imm, code_right_imm;
  tSymbolSize op_size;
  IntType imm_int_type;
} shift_order_t;

static shift_order_t *shift_orders;

static void decode_shift(Word index)
{
  const shift_order_t *p_order = &shift_orders[index];
  Word dest;

  if (ChkArgCnt(2, 2) && decode_adr_reg(&ArgStr[2], &dest, p_order->op_size))
  {
    adr_vals_t count;

    switch (decode_adr(&ArgStr[1], &count, eSymbolSize8Bit, False))
    {
      case e_mode_reg:
        encode_fmt_15(p_order->code_reg,
                      dest,
                      count.part, eSymbolFlag_None);
        break;
      case e_mode_imm:
      {
        LongInt imm_count = (LongInt)count.imm_value,
                max_mask = IntTypeDefs[p_order->imm_int_type].Max;

        if (!mFirstPassUnknownOrQuestionable(count.imm_disp_flags)
         && !ChkRangePos(imm_count, -max_mask, max_mask, &ArgStr[1]))
          return;
        switch (p_order->op_size)
        {
          case eSymbolSize32Bit:
            encode_fmt_20((imm_count < 0) ? p_order->code_right_imm : p_order->code_left_imm, dest, count.imm_value & 31);
            break;
          case eSymbolSize16Bit:
            encode_fmt_15((imm_count < 0) ? p_order->code_right_imm : p_order->code_left_imm,
                          dest,
                          count.imm_value & 15, count.imm_disp_flags);
            break;
          default:
            encode_fmt_9((imm_count < 0) ? p_order->code_right_imm : p_order->code_left_imm, dest, count.imm_value & 7);
            break;
        }
        break;
      }
      case e_mode_none:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_load_store(Word index)
 * \brief  handle load_store orders
 * \param  index index into insn table
 * ------------------------------------------------------------------------ */

typedef struct
{
  Word code4, code7, code8, code8_abs, code10;
  tSymbolSize op_size;
  Boolean is_store;
} load_store_order_t;

static load_store_order_t *load_store_orders;

static void decode_load_store(Word index)
{
  const load_store_order_t *p_order = &load_store_orders[index];
  adr_vals_t mem_adr_vals, nomem_adr_vals;
  const tStrComp *p_nomem_arg = &ArgStr[2 - p_order->is_store],
                 *p_mem_arg = &ArgStr[p_order->is_store + 1];

  if (!ChkArgCnt(2, 2))
    return;

  switch (decode_adr(p_nomem_arg, &nomem_adr_vals, p_order->op_size, False))
  {
    case e_mode_reg:
      switch (decode_adr(p_mem_arg, &mem_adr_vals, p_order->op_size, True))
      {
        case e_mode_none:
          break;
        case e_mode_disp_reg:
          if (!mem_adr_vals.disp_value && cfg_sr)
            encode_fmt_18(p_order->code8 >> 4, mem_adr_vals.part, nomem_adr_vals.part, 0x0e, eSymbolFlag_None);
          else if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && cfg_sr)
            encode_fmt_17(p_order->code10,
                          mem_adr_vals.part,
                          nomem_adr_vals.part, eSymbolFlag_None,
                          mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
          else if (mem_adr_vals.disp_value >= 0)
            encode_fmt_2(0x0012 | (p_order->code4 & 1),
                         mem_adr_vals.part,
                         nomem_adr_vals.part, eSymbolFlag_None,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         p_order->code4 & 0x0e);
          else
            encode_fmt_2(0x0018 | (p_order->code4 & 1),
                         mem_adr_vals.part,
                         nomem_adr_vals.part, eSymbolFlag_None,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         p_order->code4 & 0x0e);
          break;
        case e_mode_disp_rp:
          if (mem_adr_vals.disp_value >= 0)
          {
            Boolean is4 = (p_order->op_size == eSymbolSize8Bit)
                        ? (mem_adr_vals.disp_value <= 13)
                        : (!(mem_adr_vals.disp_value & 1) && (mem_adr_vals.disp_value <= 26));
            if (is4)
              encode_fmt_18(p_order->code8 >> 4, mem_adr_vals.part, nomem_adr_vals.part, (mem_adr_vals.disp_value >> (p_order->op_size != eSymbolSize8Bit)) & 0xf, mem_adr_vals.imm_disp_flags);
            else if (RangeCheck(mem_adr_vals.disp_value, UInt16))
              encode_fmt_19(p_order->code8 >> 4, mem_adr_vals.part, nomem_adr_vals.part, 0xf, mem_adr_vals.disp_value & 0xffffu, mem_adr_vals.imm_disp_flags);
            else
              encode_fmt_2(0x0012 | (p_order->code4 & 1),
                           mem_adr_vals.part,
                           nomem_adr_vals.part, eSymbolFlag_None,
                           mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                           (p_order->code4 & 0x0e) + 1);
          }
          else
            encode_fmt_2(0x0018 | (p_order->code4 & 1),
                         mem_adr_vals.part,
                         nomem_adr_vals.part, eSymbolFlag_None,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         (p_order->code4 & 0x0e) + 1);
          break;
        case e_mode_disp_rrp:
          if (!mem_adr_vals.disp_value && !cfg_sr)
            encode_fmt_18(p_order->code8 >> 4, mem_adr_vals.part, nomem_adr_vals.part, 0x0e, eSymbolFlag_None);
          else if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && !cfg_sr)
            encode_fmt_17(p_order->code10,
                          mem_adr_vals.part,
                          nomem_adr_vals.part, eSymbolFlag_None,
                          mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
          else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
            encode_fmt_2(0x0012 | (p_order->code4 & 1),
                         mem_adr_vals.part,
                         nomem_adr_vals.part, eSymbolFlag_None,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         (p_order->code4 & 0x0e) + 2);
          break;
        case e_mode_abs:
          if (is_abs20(mem_adr_vals.disp_value))
            encode_fmt_12(p_order->code8_abs,
                          mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                          nomem_adr_vals.part, eSymbolFlag_None);
          else
            encode_fmt_3(0x0012 | (p_order->code4 & 1),
                         mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                         nomem_adr_vals.part, eSymbolFlag_None,
                         (p_order->code4 & 0x0e) + 3);
          break;
        case e_mode_abs_rel:
          encode_fmt_13(p_order->code7,
                        mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                        nomem_adr_vals.part, eSymbolFlag_None,
                        mem_adr_vals.index_reg);
          break;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, p_mem_arg);
      }
      break;
    case e_mode_imm:
    {
      Word is_word;

      if (!mFirstPassUnknownOrQuestionable(nomem_adr_vals.imm_disp_flags) && !ChkRangePos(nomem_adr_vals.imm_value, 0, 15, p_nomem_arg))
        return;
      if (!p_order->is_store)
        goto inv_addr_mode;
      switch (p_order->op_size)
      {
        case eSymbolSize16Bit:
          is_word = 1; break;
        case eSymbolSize8Bit:
          is_word = 0; break;
        default:
          goto inv_addr_mode;
      }
      switch (decode_adr(p_mem_arg, &mem_adr_vals, p_order->op_size, True))
      {
        case e_mode_none:
          break;
        case e_mode_disp_reg:
          if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && cfg_sr)
            encode_fmt_17(0x218 + (is_word << 8),
                          mem_adr_vals.part,
                          nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                          mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
          else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
            encode_fmt_2(0x0012 + is_word,
                         mem_adr_vals.part,
                         nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         0);
          break;
        case e_mode_disp_rp:
          if (!mem_adr_vals.disp_value)
            encode_fmt_15(0x82 + (is_word << 6),
                          mem_adr_vals.part,
                          nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags);
          else if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0xffffl))
            encode_fmt_16(0x83 + (is_word << 6),
                          mem_adr_vals.part,
                          nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                          mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags);
          else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
            encode_fmt_2(0x0012 + is_word,
                         mem_adr_vals.part,
                         nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         1);
          break;
        case e_mode_disp_rrp:
          if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && !cfg_sr)
            encode_fmt_17(0x218 + (is_word << 8),
                          mem_adr_vals.part,
                          nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                          mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
          else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
            encode_fmt_2(0x0012 + is_word,
                         mem_adr_vals.part,
                         nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                         mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                         2);
          break;
        case e_mode_abs:
          if (is_abs20(mem_adr_vals.disp_value))
            encode_fmt_12(0x81 + (is_word << 6),
                          mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                          nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags);
          else
            encode_fmt_3(0x0012 + is_word,
                         mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                         nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                         3);
          break;
        case e_mode_abs_rel:
          encode_fmt_13(0x42 + (is_word << 5),
                        mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                        nomem_adr_vals.imm_value, nomem_adr_vals.imm_disp_flags,
                        mem_adr_vals.index_reg);
          break;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, p_mem_arg);
      }
      break;
    }
    case e_mode_none:
      break;
    default:
    inv_addr_mode:
      WrStrErrorPos(ErrNum_InvAddrMode, p_nomem_arg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_loadm_storm(Word code)
 * \brief  handle LOADM(P)/STORM(P) instructions
 * \param  code opcode
 * ------------------------------------------------------------------------ */

static void decode_loadm_storm(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags flags;
    Boolean ok;
    Word count = eval_count_18(&ArgStr[1], &ok, &flags);
    if (ok)
      encode_fmt_6(code, count);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bit(Word code)
 * \brief  handle bit operations
 * \param  code machine code bits differentiating op
 * ------------------------------------------------------------------------ */

static void decode_bit(Word code)
{
  Boolean is_word = code & 1;
  Word bit_pos;
  tSymbolFlags bit_pos_flags;
  Boolean ok;
  adr_vals_t mem_adr_vals;

  code &= ~1;

  if (!ChkArgCnt(2, 2))
    return;
  bit_pos = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], ArgStr[1].str.p_str[0] == '$', is_word ? UInt4 : UInt3, &ok, &bit_pos_flags);
  if (!ok)
    return;
  switch (decode_adr(&ArgStr[2], &mem_adr_vals, is_word ? eSymbolSize16Bit : eSymbolSize8Bit, True))
  {
    case e_mode_disp_reg:
      if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && cfg_sr)
        encode_fmt_17(0x1aa + (code << 3) + is_word,
                      mem_adr_vals.part,
                      bit_pos, bit_pos_flags,
                      mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
      else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
        encode_fmt_2(0x0010 + is_word,
                     mem_adr_vals.part,
                     bit_pos, bit_pos_flags,
                     mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                     code + 4);
      break;
    case e_mode_disp_rp:
      if (!mem_adr_vals.disp_value)
        encode_fmt_15(0x6a + (code << 1) + (is_word << 2),
                      mem_adr_vals.part,
                      bit_pos, bit_pos_flags);
      else if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0xffffl))
        encode_fmt_16(0x6b + (code << 1) - (is_word << 1),
                      mem_adr_vals.part,
                      bit_pos, bit_pos_flags,
                      mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags);
      else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
        encode_fmt_2(0x0010 + is_word,
                     mem_adr_vals.part,
                     bit_pos, bit_pos_flags,
                     mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                     5 + code);
      break;
    case e_mode_disp_rrp:
      if ((mem_adr_vals.disp_value >= 0) && (mem_adr_vals.disp_value <= 0x3fff) && !cfg_sr)
        encode_fmt_17(0x1aa + (code << 3) + is_word,
                      mem_adr_vals.part,
                      bit_pos, bit_pos_flags,
                      mem_adr_vals.disp_value & 0x3fff, mem_adr_vals.imm_disp_flags);
      else if (mFirstPassUnknownOrQuestionable(mem_adr_vals.imm_disp_flags) || ChkRange(mem_adr_vals.disp_value, 0, 0xffffful))
        encode_fmt_2(0x0010 + is_word,
                     mem_adr_vals.part,
                     bit_pos, bit_pos_flags,
                     mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                     6 + code);
      break;
    case e_mode_abs:
      if (!is_abs20(mem_adr_vals.disp_value))
        encode_fmt_3(0x0010 + is_word,
                     mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                     bit_pos, bit_pos_flags,
                     code + 7);
      else if (is_word)
        encode_fmt_12(0x6f + (code << 1),
                      mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                      bit_pos, bit_pos_flags);
      else
        encode_fmt_7(0xd7 + (code << 2),
                      mem_adr_vals.disp_value & 0xffffful, mem_adr_vals.imm_disp_flags,
                      bit_pos, bit_pos_flags);
      break;
    case e_mode_abs_rel:
      if (is_word)
        encode_fmt_13(0x36 + code,
                      mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                      bit_pos, bit_pos_flags,
                      mem_adr_vals.index_reg);
      else
        encode_fmt_8(0x68 + (code << 1),
                     mem_adr_vals.disp_value, mem_adr_vals.imm_disp_flags,
                     bit_pos, bit_pos_flags,
                      mem_adr_vals.index_reg);
      break;
    case e_mode_none:
      break;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_tbit(Word code)
 * \brief  handle TBIT instruction
 * \param  code machine opcode
 * ------------------------------------------------------------------------ */

static void decode_tbit(Word code)
{
  Word src;
  adr_vals_t pos_adr_vals;

  if (ChkArgCnt(2, 2)
   && decode_adr_reg(&ArgStr[2], &src, eSymbolSize16Bit))
    switch (decode_adr(&ArgStr[1], &pos_adr_vals, eSymbolSize8Bit, False))
    {
      case e_mode_reg:
        encode_fmt_15(code | 0x01,
                      src,
                      pos_adr_vals.part, eSymbolFlag_None);
        break;
      case e_mode_imm:
        if (mFirstPassUnknownOrQuestionable(pos_adr_vals.imm_disp_flags) || ChkRange(pos_adr_vals.imm_value, 0,15))
          encode_fmt_15(code,
                        src,
                        pos_adr_vals.imm_value, pos_adr_vals.imm_disp_flags);
        break;
      case e_mode_none:
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    }
}

static void decode_cinv(Word code)
{
  char *p_split;
  const char *p_idx, reg_list[] = "UID";
  int arg_len;
  tStrComp remainder, part, *p_arg;
  unsigned mask = 0, idx;

  if (!ChkArgCnt(1, 1))
    return;
  p_arg =&ArgStr[1];
  
  arg_len = strlen(p_arg->str.p_str);
  if ((arg_len < 2) || (p_arg->str.p_str[0] != '[') || (p_arg->str.p_str[arg_len - 1] != ']'))
  {
    WrStrErrorPos(ErrNum_WrongOptionList, p_arg);
    return;
  }
  StrCompRefRight(&remainder, p_arg, 1);
  StrCompShorten(&remainder, 1);
  KillPrefBlanksStrCompRef(&remainder);
  KillPostBlanksStrComp(&remainder);
  while (remainder.str.p_str[0])
  {
    p_split = QuotPos(remainder.str.p_str, ',');
    if (p_split)
    {
      StrCompSplitRef(&part, &remainder, &remainder, p_split);
      KillPostBlanksStrComp(&part);
      KillPrefBlanksStrCompRef(&remainder);
    }
    else
      part = remainder;
    p_idx = (strlen(part.str.p_str) == 1) ? strchr(reg_list, as_toupper(*part.str.p_str)) : NULL;
    if (!p_idx)
    {
      WrStrErrorPos(ErrNum_UnknownOption, &part);
      return;
    }
    idx = 1 << (p_idx - reg_list);
    if (mask & idx)
    {
      WrStrErrorPos(ErrNum_DuplicateOption, &part);
      return;
    }
    mask |= idx;
    if (!p_split)
      break;
  }
  if (mask < 2)
    WrStrErrorPos(ErrNum_WrongOptionList, p_arg);
  else
    encode_fmt_4(code | mask);
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

static void add_dot_pseudo(const char *p_name, Word code, InstProc decode_fnc)
{
  AddInstTable(InstTable, p_name, code, decode_fnc);
  if (*p_name == '.')
    AddInstTable(InstTable, p_name + 1, code, decode_fnc);
}

static void add_condition(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "S%s", p_name);
  AddInstTable(InstTable, name, code, decode_scond);
  as_snprintf(name, sizeof(name), "J%s", p_name);
  AddInstTable(InstTable, name, code, decode_jump);
  as_snprintf(name, sizeof(name), "B%s", p_name);
  AddInstTable(InstTable, name, code, decode_br);
}

static void add_alu(const char *p_name, Word code8_16, Word code32_reg, Word code32_imm4_16, Word code32_imm20, Word code32_imm32)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, code8_16 | 0x00, decode_alu_8_16);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  AddInstTable(InstTable, name, code8_16 | 0x02, decode_alu_8_16);
  if (code32_reg || code32_imm4_16 || code32_imm20 || code32_imm32)
  {
    as_snprintf(name, sizeof(name), "%sD", p_name);
    order_array_rsv_end(alu32_orders, alu32_order_t);
    alu32_orders[InstrZ].code_reg = code32_reg;
    alu32_orders[InstrZ].code_imm4_16 = code32_imm4_16;
    alu32_orders[InstrZ].code_imm20 = code32_imm20;
    alu32_orders[InstrZ].code_imm32 = code32_imm32;
    AddInstTable(InstTable, name, InstrZ++, decode_alu_32);
  }
}

static void add_shift(const char *p_name, Word code_reg, Word code_left_imm, Word code_right_imm, tSymbolSize op_size)
{
  order_array_rsv_end(shift_orders, shift_order_t);
  shift_orders[InstrZ].code_reg = code_reg;
  shift_orders[InstrZ].code_left_imm = code_left_imm;
  shift_orders[InstrZ].code_right_imm = code_right_imm;
  shift_orders[InstrZ].op_size = op_size;
  switch (op_size)
  {
    case eSymbolSize32Bit:
      shift_orders[InstrZ].imm_int_type = SInt6;
      break;
    case eSymbolSize16Bit:
      shift_orders[InstrZ].imm_int_type = SInt5;
      break;
    default:
      shift_orders[InstrZ].imm_int_type = SInt4;
  }
  AddInstTable(InstTable, p_name, InstrZ++, decode_shift);
}

static void add_load_store(const char *p_name, tSymbolSize op_size, Boolean is_store)
{
  order_array_rsv_end(load_store_orders, load_store_order_t);
  load_store_orders[InstrZ].op_size = op_size;
  load_store_orders[InstrZ].is_store = is_store;
  load_store_orders[InstrZ].code4 = is_store ? 1 : 0;
  load_store_orders[InstrZ].code7 = is_store ? 0x60 : 0x40;
  load_store_orders[InstrZ].code8 =
  load_store_orders[InstrZ].code8_abs = is_store ? 0xc0 : 0x80;
  load_store_orders[InstrZ].code10 = is_store ? 0x318 : 0x218;
  switch (op_size)
  {
    case eSymbolSize32Bit:
      load_store_orders[InstrZ].code4 += 8;
      load_store_orders[InstrZ].code7 += 6;
      load_store_orders[InstrZ].code8 += 0x20;
      load_store_orders[InstrZ].code8_abs += 0x07;
      load_store_orders[InstrZ].code10 += 2;
      break;
    case eSymbolSize16Bit:
      load_store_orders[InstrZ].code4 += 12;
      load_store_orders[InstrZ].code7 += 7;
      load_store_orders[InstrZ].code8 += 0x10;
      load_store_orders[InstrZ].code8_abs += 0x09;
      load_store_orders[InstrZ].code10 += 3;
      break;
    default:
      load_store_orders[InstrZ].code4 += 4;
      load_store_orders[InstrZ].code7 += 5;
      load_store_orders[InstrZ].code8 += 0x30;
      load_store_orders[InstrZ].code8_abs += 0x08;
      load_store_orders[InstrZ].code10 += 1;
      break;
  }
  AddInstTable(InstTable, p_name, InstrZ++, decode_load_store);
}

static void add_load_store_all_sizes(const char *p_name, Boolean is_store)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sB", p_name);
  add_load_store(name, eSymbolSize8Bit, is_store);
  as_snprintf(name, sizeof(name), "%sD", p_name);
  add_load_store(name, eSymbolSize32Bit, is_store);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  add_load_store(name, eSymbolSize16Bit, is_store);
}

static void add_bit(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, code + 0, decode_bit);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  AddInstTable(InstTable, name, code + 1, decode_bit);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);

  add_null_pseudo(InstTable);

  inst_table_set_prefix_proc(InstTable, check_pc_even, 0);
  AddInstTable(InstTable, "NOP"   , NOPCode, decode_fixed);
  AddInstTable(InstTable, "RETX"  , 0x0003, decode_fixed);
  AddInstTable(InstTable, "DI"    , 0x0004, decode_fixed);
  AddInstTable(InstTable, "EI"    , 0x0005, decode_fixed);
  AddInstTable(InstTable, "WAIT"  , 0x0006, decode_fixed);
  AddInstTable(InstTable, "EIWAIT", 0x0007, decode_fixed);

  codecr16_iter_conditions(add_condition);
  AddInstTable(InstTable, "BR", 0x0e, decode_br);
  AddInstTable(InstTable, "BAL", 0xc0, decode_bal);
  AddInstTable(InstTable, "JUMP", 0xe, decode_jump);
  AddInstTable(InstTable, "JUSR", 0xf, decode_jump);
  AddInstTable(InstTable, "JAL", 0x00d, decode_jal);
  AddInstTable(InstTable, "EXCP", 0x00c, decode_excp);
  AddInstTable(InstTable, "BEQ0B", 0x0c, decode_bxx0);
  AddInstTable(InstTable, "BEQ0W", 0x0e, decode_bxx0);
  AddInstTable(InstTable, "BNE0B", 0x0d, decode_bxx0);
  AddInstTable(InstTable, "BNE0W", 0x0f, decode_bxx0);
  AddInstTable(InstTable, "BEQ1B", 0x5000, decode_bxx1);
  AddInstTable(InstTable, "BEQ1W", 0x5200, decode_bxx1);
  AddInstTable(InstTable, "BNE1B", 0x5001, decode_bxx1);
  AddInstTable(InstTable, "BNE1W", 0x5201, decode_bxx1);

  add_alu("ADD" , 0x30, 0x6100, 0x60, 0x04, 0x2);
  add_alu("ADDC", 0x34, 0x0000, 0x00, 0x00, 0x0);
  add_alu("ADDU", 0x2c, 0x0000, 0x00, 0x00, 0x0);
  add_alu("SUB" , 0x38, 0xc014, 0x00, 0x00, 0x3);
  add_alu("SUBC", 0x3c, 0x0000, 0x00, 0x00, 0x0);
  add_alu("MUL" , 0x64, 0x0000, 0x00, 0x00, 0x0);
  add_alu("CMP" , 0x50, 0x5700, 0x56, 0x00, 0x9);
  add_alu("AND" , 0x20, 0xb014, 0x00, 0x00, 0x4);
  add_alu("OR"  , 0x24, 0x9014, 0x00, 0x00, 0x5);
  add_alu("XOR" , 0x28, 0xa014, 0x00, 0x00, 0x6);
  add_alu("MOV" , 0x58, 0x5500, 0x54, 0x05, 0x7);

  InstrZ = 0;
  add_shift("ASHUB", 0x41 , 0x080 , 0x081, eSymbolSize8Bit );
  add_shift("ASHUW", 0x45 ,  0x42 ,  0x43, eSymbolSize16Bit);
  add_shift("ASHUD", 0x48 ,  0x26 ,  0x27, eSymbolSize32Bit);
  add_shift("LSHB" , 0x44 , 0x080 , 0x013, eSymbolSize8Bit );
  add_shift("LSHW" , 0x46 ,  0x42 ,  0x49, eSymbolSize16Bit);
  add_shift("LSHD" , 0x47 ,  0x26 ,  0x25, eSymbolSize32Bit);

  AddInstTable(InstTable, "MOVXB", 0x5c00, decode_mov_ext);
  AddInstTable(InstTable, "MOVXW", 0x5e00, decode_mov_ext);
  AddInstTable(InstTable, "MOVZB", 0x5d00, decode_mov_ext);
  AddInstTable(InstTable, "MOVZW", 0x5f00, decode_mov_ext);

  AddInstTable(InstTable, "MACQW", 0xd, decode_mac);
  AddInstTable(InstTable, "MACUW", 0xe, decode_mac);
  AddInstTable(InstTable, "MACSW", 0xf, decode_mac);

  AddInstTable(InstTable, "MULSB", 0x0b, decode_mul);
  AddInstTable(InstTable, "MULSW", 0x62, decode_mul);
  AddInstTable(InstTable, "MULUW", 0x63, decode_mul);

  AddInstTable(InstTable, "LPR" , 0, decode_lpr_spr);
  AddInstTable(InstTable, "LPRD", 1, decode_lpr_spr);
  AddInstTable(InstTable, "SPR" , 2, decode_lpr_spr);
  AddInstTable(InstTable, "SPRD", 3, decode_lpr_spr);
  AddInstTable(InstTable, "POP"   , 0x02, decode_push_pop);
  AddInstTable(InstTable, "POPRET", 0x03, decode_push_pop);
  AddInstTable(InstTable, "PUSH"  , 0x01, decode_push_pop);
  AddInstTable(InstTable, "LOADM" , 0x14, decode_loadm_storm);
  AddInstTable(InstTable, "LOADMP", 0x15, decode_loadm_storm);
  AddInstTable(InstTable, "STORM" , 0x16, decode_loadm_storm);
  AddInstTable(InstTable, "STORMP", 0x17, decode_loadm_storm);
  AddInstTable(InstTable, "CINV", 0x0008, decode_cinv);

  InstrZ = 0;
  add_load_store_all_sizes("LOAD", False);
  add_load_store_all_sizes("STOR", True);

  add_bit("CBIT", 0);
  add_bit("SBIT", 4);
  add_bit("TBIT", 8);
  AddInstTable(InstTable, "TBIT", 0x06, decode_tbit);

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
  order_array_free(shift_orders);
  order_array_free(load_store_orders);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     make_code_cr16c(void)
 * \brief  encode target-specific instruction
 * ------------------------------------------------------------------------ */

static void make_code_cr16c(void)
{
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_cr16c(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_cr16c(void)
{
  return Memo("REG") || Memo(".REG");
}

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_cr16c(char *p_arg, TempResult *p_result)
 * \brief  handle built-in (register) symbols for CR16
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_cr16c(char *p_arg, TempResult *p_result)
{
  Word reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_cr16c;
  }
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_cr16c(void)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_cr16c(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("CR16C");
  static as_assume_rec_t assume_cr16c[] =
  {
    { "SR", &cfg_sr, 0, 1, 0, NULL }
  };

  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0x2c00; /* = addub $0x0,r0 */
  DivideChars = ",";
  HasAttrs = False;
  AttrChars = "";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0; SegLimits[SegCode] = 0xfffffful;

  MakeCode = make_code_cr16c;
  IsDef = is_def_cr16c;
  DissectReg = dissect_reg_cr16c;
  InternSymbol = intern_symbol_cr16c;
  SwitchFrom = deinit_fields;
  init_fields();
  onoff_supmode_add();
  assume_set(assume_cr16c, as_array_size(assume_cr16c));
}

/*!------------------------------------------------------------------------
 * \fn     init_code_cr16c(void)
 * \brief  executed at beginning of pass
 * ------------------------------------------------------------------------ */

static void init_code_cr16c(void)
{
  /* assume no small registers */
  cfg_sr = 0;
}

/*!------------------------------------------------------------------------
 * \fn     codecr16c_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

void codecr16c_init(void)
{
  cpu_cr16c = AddCPU("CR16C", switch_to_cr16c);
  AddInitPassProc(init_code_cr16c);
}
