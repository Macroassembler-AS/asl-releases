/* codewe32.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator WE32xxx                                                    */
/*                                                                           */
/*****************************************************************************/

/* TODO:
   The assembler inserts a NOP before instructions (other than branch)
   that read the PSW. This NOP allows the conditions bits to stabilize.
 */

#include "stdinc.h"
#include <string.h>
#include <assert.h>
#include "bpemu.h"
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
#include "symbolsize.h"
#include "asmcode.h"

#include "codewe32.h"

typedef enum
{
  ModNone = -1,
  ModReg = 0,
  ModImm = 1,
  ModMem = 2,
  ModLit = 3,
  ModBranch = 4
} adr_mode_t;

#define MModReg (1 << ModReg)
#define MModImm (1 << ModImm)
#define MModMem (1 << ModMem)
#define MModLit (1 << ModLit)
#define MModBranch (1 << ModBranch)
#define MModExpand (1 << 7)

typedef enum
{
  e_core_flag_none = 0,
  e_core_flag_we32200 = 1 << 0
} cpu_core_flags_t;

#ifdef __cplusplus
# include "codewe32.hpp"
#endif

typedef struct
{
  Word code;
  Byte max_exec_mode, core_flags;
  size_t arg_cnt;
  unsigned adr_mode_mask[4];
  tSymbolSize op_size[4];
} gen_order_t;

typedef struct
{
  int gen_indices[4];
} var_arg_order_t;

typedef struct
{
  adr_mode_t mode;
  unsigned count;
  Byte vals[20], val_guess_mask;
  unsigned val_offset;
  tSymbolFlags val_flags;
} adr_vals_t;

static CPUVar cpu_32100, cpu_32200;

static gen_order_t *gen_orders;
static var_arg_order_t *var_arg_orders;
static unsigned var_arg_op_cnt;
static tSymbolSize op_size;

static Boolean curr_exec_mode_set;
static unsigned curr_exec_mode;
static const char exec_mode_name[] = "EXECMODE";

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

#define REG_FP 9
#define REG_AP 10
#define REG_PSW 11
#define REG_SP 12
#define REG_PCBP 13
#define REG_ISP 14
#define REG_PC 15
#define NO_REG 0xff

static const char xtra_reg_names[7][5] =
{
  "FP",
  "AP",
  "PSW",
  "SP",
  "PCBP",
  "ISP",
  "PC"
};

static Boolean decode_reg_core(const char *p_arg, Byte *p_result, tSymbolSize *p_size)
{
  int z;

  if (*p_arg == '%')
    p_arg++;

  for (z = 0; z < 7; z++)
    if (!as_strcasecmp(p_arg, xtra_reg_names[z]))
    {
      *p_result = (REG_FP + z) | REGSYM_FLAG_ALIAS;
      *p_size = eSymbolSize32Bit;
      return True;
    }

  if (as_toupper(*p_arg) != 'R')
    return False;

  switch (strlen(p_arg))
  {
    case 2:
    {
      if (as_isdigit(p_arg[1]))
      {
        *p_result = p_arg[1] - '0';
        *p_size = eSymbolSize32Bit;
        return True;
      }
      break;
    }
    case 3:
      if (as_isdigit(p_arg[2]) && ((p_arg[1] >= '0') && (p_arg[1] <= '3')))
      {
        *p_result = (10 * (p_arg[1] - '0')) + (p_arg[2] - '0');
        *p_size = eSymbolSize32Bit;
        return *p_result <= ((MomCPU >= cpu_32200) ? 31 : 15);
      }
      break;
    default:
      break;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_we32(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - WE32xxx variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void dissect_reg_we32(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize32Bit:
    {
      unsigned r_num = value & 15;

      if ((value & REGSYM_FLAG_ALIAS) && (r_num >= REG_FP))
        strmaxcpy(p_dest, xtra_reg_names[r_num - REG_FP], dest_size);
      else
        as_snprintf(p_dest, dest_size, "R%u", r_num);
      break;
    }
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*-------------------------------------------------------------------------*/
/* Address Expression Decoder */

static tRegEvalResult decode_reg(const tStrComp *p_arg, Byte *p_result, Boolean must_be_reg)
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

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  return reg_eval_result;
}

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  clear encoded address expression
 * \param  p_vals expression to clear
 * ------------------------------------------------------------------------ */

static Boolean reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = ModNone;
  p_vals->count = 0;
  p_vals->val_offset = 0;
  p_vals->val_flags = eSymbolFlag_None;
  p_vals->val_guess_mask = 0xff;
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(tStrComp *p_arg, adr_vals_t *p_result, LongWord pc_value, unsigned mode_mask)
 * \brief  parse address expression
 * \param  p_arg source argument
 * \param  p_result parsed result
 * \param  pc_value value of PC to be used in PC-relative calculation
 * \param  mode_mask bit mask of allowed addressing modes
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean check_mode_mask(unsigned mode_mask, unsigned act_mask, tStrComp *p_arg, adr_vals_t *p_result)
{
  if (!(mode_mask & act_mask))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return reset_adr_vals(p_result);
  }
  else
    return True;
}

static void append_adr_vals_int(adr_vals_t *p_result, LongWord value, tSymbolSize op_size)
{
  unsigned z;

  p_result->val_offset = p_result->count;
  for (z = 0; z < GetSymbolSizeBytes(op_size); z++)
  {
    p_result->vals[p_result->count++] = value & 0xff;
    value >>= 8;
  }
}

static Boolean is_literal(LongWord value, tSymbolSize op_size, Byte *p_mode_field)
{
  if (value <= 63)
  {
    *p_mode_field = value & 63;
    return True;
  }
  switch (op_size)
  {
    case eSymbolSize8Bit:
      if ((value & 0xff) >= 0xf0)
        goto is_lit;
      return False;
    case eSymbolSize16Bit:
      if ((value & 0xffff) >= 0xfff0u)
        goto is_lit;
      return False;
    case eSymbolSize32Bit:
      if (value >= 0xfffffff0ul)
        goto is_lit;
      return False;
    default:
      return False;
    is_lit:
      *p_mode_field = value & 0xff;
      return True;
  }
}

static Boolean byte_2_word_sign_extendable(LongWord value)
{
  return ((value < 0x80) || (value >= 0xffffff80ul));
}

static Boolean byte_2_halfword_extendable(Word value)
{
  return ((value < 0x80) || (value >= 0xff80ul));
}

static Boolean halfword_2_word_sign_extendable(LongWord value)
{
  return ((value < 0x8000) || (value >= 0xffff8000ul));
}

static void append_reg_disp(adr_vals_t *p_result, LongInt disp, Byte reg, Boolean deferred)
{
  if (reg >= 16)
    p_result->vals[p_result->count++] = 0xcb;
  if ((disp >= -128) && (disp <= 127))
  {
    p_result->vals[p_result->count++] = (deferred ? 0xd0 : 0xc0) | (reg & 0x0f);
    append_adr_vals_int(p_result, disp & 0xff, eSymbolSize8Bit);
  }
  else if ((disp >= -32768) && (disp <= 32767))
  {
    p_result->vals[p_result->count++] = (deferred ? 0xb0 : 0xa0) | (reg & 0x0f);
    append_adr_vals_int(p_result, disp & 0xffffu, eSymbolSize16Bit);
  }
  else
  {
    p_result->vals[p_result->count++] = (deferred ? 0x90 : 0x80) | (reg & 0x0f);
    append_adr_vals_int(p_result, (LongWord)disp, eSymbolSize32Bit);
  }
}

static Boolean check_ext_regs(Byte reg, const tStrComp *p_arg)
{
  if (reg < 16)
    return True;
  else if (MomCPU < cpu_32200)
  {
    WrStrErrorPos(ErrNum_AddrModeNotSupported, p_arg);
    return False;
  }
  else if ((reg >= 24) && (curr_exec_mode > 0))
  {
    WrStrErrorPos(ErrNum_RegAccessibleOnlyInExecMode, p_arg);
    return False;
  }
  else
    return True;
}

static Boolean is_plusminus(char src, Byte *p_pre_post)
{
  static const char may[3] = "-+";
  const char *p_pos = strchr(may, src);

  if (!p_pos)
    return False;
  *p_pre_post = (p_pos - may) << 7;
  return True;
}

static Boolean is_pre_excrement(const tStrComp *p_arg, Byte *p_result, tRegEvalResult *p_reg_eval_result, Byte *p_pre_post)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || !is_plusminus(p_arg->str.p_str[0], p_pre_post)
   || (p_arg->str.p_str[1] != '(')
   || (p_arg->str.p_str[arg_len - 1] != ')'))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 2, arg_len - 3);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean is_post_excrement(const tStrComp *p_arg, Byte *p_result, tRegEvalResult *p_reg_eval_result, Byte *p_pre_post)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '(')
   || (p_arg->str.p_str[arg_len - 2] != ')')
   || !is_plusminus(p_arg->str.p_str[arg_len - 1], p_pre_post))
    return False;
  *p_pre_post |= 0x40;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 1, arg_len - 3);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean decode_adr(tStrComp *p_arg, adr_vals_t *p_result, LongWord pc_value, unsigned mode_mask)
{
  Byte reg, pre_post;
  Boolean deferred, ok;
  tStrComp arg;
  int arg_len, split_pos;
  LongWord address;
  tRegEvalResult reg_eval_result;

  reset_adr_vals(p_result);
  StrCompRefRight(&arg, p_arg, 0);

  /* Expanded operand type: */

  if (arg.str.p_str[0] == '{')
  {
    const char *p_start, *p_end;

    for (p_start = &arg.str.p_str[1]; as_isspace(*p_start); p_start++);
    p_end = strchr(p_start, '}');
    if (p_end)
    {
      static const char names[][8] = { "05uword", "25uhalf", "34byte", "35ubyte", "44word", "45sword", "64half", "65shalf", "75sbyte" };
      unsigned z;

      for (; p_end > p_start; p_end--)
        if (!as_isspace(*p_end))
          break;
      for (z = 0; z < as_array_size(names); z++)
      {
        int l = names[z][1] - '0';
        if ((p_end - p_start == l) && !as_strncasecmp(p_start, names[z] + 2, l))
        {
          if (mode_mask & MModExpand)
          {
            StrCompIncRefLeft(&arg, p_end + 1 - arg.str.p_str);
            KillPrefBlanksStrCompRef(&arg);
            p_result->vals[p_result->count++] = 0xe0 | (names[z][0] - '0');
            break;
          }
          else
          {
            WrStrErrorPos(ErrNum_InvAddrMode, &arg);
            return False;
          }
        }
      }
    }
  }

  /* Deferred modifier? */

  deferred = (arg.str.p_str[0] == '*');
  if (deferred)
  {
    StrCompIncRefLeft(&arg, 1);
    KillPrefBlanksStrCompRef(&arg);
  }

  /* Plain register? */

  switch (decode_reg(&arg, &reg, False))
  {
    case eIsReg:
    {
      Boolean is_write = (mode_mask & ~MModExpand) == (MModReg | MModMem);

      if (!check_ext_regs(reg, p_arg))
        return False;
      else if (deferred || (reg == REG_PC))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return False;
      }
      if (is_write && (curr_exec_mode > 0) && ((reg == REG_PSW) || (reg == REG_PCBP) || (reg == REG_ISP)))
      {
        WrStrErrorPos(ErrNum_RegReadOnlyInExecMode, p_arg);
        return False;
      }
      p_result->mode = ModReg;
      if (reg >= 16)
        p_result->vals[p_result->count++] = 0xcb;
      p_result->vals[p_result->count++] = (reg & 0x0f) | 0x40;
      return check_mode_mask(mode_mask, MModReg, p_arg, p_result);
    }
    case eRegAbort:
      return reset_adr_vals(p_result);
    default:
      break;
  }

  /* Immediate: */

  if (*arg.str.p_str == '&')
  {
    LargeInt imm_value;
    tSymbolSize imm_op_size = op_size;

    if (deferred)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return False;
    }

    switch (imm_op_size)
    {
      case eSymbolSize8Bit:
        imm_value = EvalStrIntExpressionOffsWithFlags(&arg, 1, Int8, &ok, &p_result->val_flags);
        goto int_common;
      case eSymbolSize16Bit:
        imm_value = EvalStrIntExpressionOffsWithFlags(&arg, 1, Int16, &ok, &p_result->val_flags);
        goto int_common;
      case eSymbolSize32Bit:
        imm_value = EvalStrIntExpressionOffsWithFlags(&arg, 1, Int32, &ok, &p_result->val_flags);
        goto int_common;
      default:
        WrStrErrorPos(ErrNum_InvOpSize, &arg);
        return reset_adr_vals(p_result);
      int_common:
      {
        Byte lit_value;

        if (!ok)
          return reset_adr_vals(p_result);
        if (is_literal(imm_value, op_size, &lit_value))
        {
          append_adr_vals_int(p_result, lit_value, eSymbolSize8Bit);
          return check_mode_mask(mode_mask, MModLit, p_arg, p_result);
        }
        else
        {
          switch (imm_op_size)
          {
            case eSymbolSize32Bit:
              if (byte_2_word_sign_extendable(imm_value))
                goto imm8;
              else if (halfword_2_word_sign_extendable(imm_value))
                goto imm16;
              p_result->vals[p_result->count++] = 0x4f;
              break;
            imm16:
              imm_op_size = eSymbolSize16Bit;
              /* FALL-THRU */
            case eSymbolSize16Bit:
              if (byte_2_halfword_extendable(imm_value))
                goto imm8;
              p_result->vals[p_result->count++] = 0x5f;
              break;
            imm8:
              imm_op_size = eSymbolSize8Bit;
              /* FALL-THRU */
            case eSymbolSize8Bit:
              p_result->vals[p_result->count++] = 0x6f;
              break;
            default:
              break;
          }
          append_adr_vals_int(p_result, imm_value, imm_op_size);
          return check_mode_mask(mode_mask, MModImm, p_arg, p_result);
        }
      }
    }
  }

  /* absolute */

  if (*arg.str.p_str == '$')
  {
    LongWord address = EvalStrIntExpressionOffsWithFlags(&arg, 1, UInt32, &ok, &p_result->val_flags);

    if (!ok)
      return False;
    p_result->vals[p_result->count++] = deferred ? 0xef : 0x7f;
    append_adr_vals_int(p_result, address, eSymbolSize32Bit);
    return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
  }

  /* (Rn)+, -(Rn), (Rn)-, +(Rn) */

  if (is_post_excrement(&arg, &reg, &reg_eval_result, &pre_post)
   || is_pre_excrement(&arg, &reg, &reg_eval_result, &pre_post))
  {
    if (eRegAbort == reg_eval_result)
      return reset_adr_vals(p_result);
    else if (deferred)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &arg);
      return reset_adr_vals(p_result);
    }
    else if (MomCPU < cpu_32200)
    {
      WrStrErrorPos(ErrNum_AddrModeNotSupported, &arg);
      return reset_adr_vals(p_result);
    }
    p_result->vals[p_result->count++] = 0x5b;
    p_result->vals[p_result->count++] = reg | pre_post;
    return check_mode_mask(mode_mask, MModMem, &arg, p_result);
  }

  /* [disp](%rn) */

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp disp_arg, reg_arg, reg_arg_remainder;
    char *p_reg_sep;
    Byte lo_reg = NO_REG, hi_reg = NO_REG, *p_dest_reg;

    StrCompSplitRef(&disp_arg, &reg_arg, &arg, &arg.str.p_str[split_pos]);
    KillPostBlanksStrComp(&disp_arg);
    KillPrefBlanksStrCompRef(&reg_arg);
    StrCompShorten(&reg_arg, 1);
    KillPostBlanksStrComp(&reg_arg);

    do
    {
      p_reg_sep = QuotPos(reg_arg.str.p_str, ',');
      if (p_reg_sep)
        StrCompSplitRef(&reg_arg, &reg_arg_remainder, &reg_arg, p_reg_sep);
      if ((decode_reg(&reg_arg, &reg, True) != eIsReg)
       || (reg == REG_PSW)
       || !check_ext_regs(reg, p_arg))
        return reset_adr_vals(p_result);

      /* WE32200 allows up to two registers in argument: one from low, one from high half.
         WE32100 only allows one register from low half: */

      p_dest_reg = (reg < 16) ? &lo_reg : &hi_reg;
      if (*p_dest_reg == NO_REG)
        *p_dest_reg = reg;
      else
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &reg_arg);
        return reset_adr_vals(p_result);
      }
      
      if (p_reg_sep)
        reg_arg = reg_arg_remainder;
    }
    while (p_reg_sep);

    /* (%rn) */

    if (!*disp_arg.str.p_str && ((hi_reg != NO_REG) != (lo_reg != NO_REG)) && !deferred)
    {
      reg = (lo_reg != NO_REG) ? lo_reg : hi_reg;
      /* 0x5f is immediate, use d8(pc) instead: */
      if (reg != REG_PC)
      {
        if (reg >= 16)
          p_result->vals[p_result->count++] = 0xcb;
        p_result->vals[p_result->count++] = 0x50 | (reg & 0x0f);
      }
      else
      {
        p_result->vals[p_result->count++] = 0xcf;
        append_adr_vals_int(p_result, 0, eSymbolSize8Bit);
      }
      return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
    }

    else
    {
      /* disp(rm,rn) allows at most 16 bit displacement */
      
      LongInt disp = EvalStrIntExpressionWithFlags(&disp_arg, ((lo_reg != NO_REG) && (hi_reg != NO_REG)) ? SInt16 : SInt32, &ok, &p_result->val_flags);

      /* AP/FP short offset */

      if ((disp >= 0) && (disp <= 14) && !deferred && (hi_reg == NO_REG) && ((lo_reg == REG_AP) || (lo_reg == REG_FP)))
      {
        p_result->vals[p_result->count++] = ((reg == REG_AP) ? 0x70 : 0x60) | (disp & 0xf);
        p_result->val_offset = 0;
        p_result->val_guess_mask = 0x0f;
        return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
      }
      
      /* Displacement with single register */
      
      else if ((lo_reg != NO_REG) != (hi_reg != NO_REG))
        append_reg_disp(p_result, disp, (lo_reg != NO_REG) ? lo_reg : hi_reg, deferred);
        
      /* Displacement with base and index register */
        
      else if ((lo_reg != NO_REG) && (hi_reg != NO_REG))
      {
        Boolean byte_disp = ((disp >= -128) && (disp <= 127));

        if (MomCPU < cpu_32200)
        {
          WrStrErrorPos(ErrNum_AddrModeNotSupported, &arg);
          return reset_adr_vals(p_result);
        }
        p_result->vals[p_result->count++] = byte_disp ? 0xab : 0xbb;
        p_result->vals[p_result->count++] = (lo_reg << 4) | (hi_reg & 0xf);
        p_result->vals[p_result->count++] = disp & 0xff;
        if (!byte_disp)
          p_result->vals[p_result->count++] = (disp >> 8) & 0xff;
      }
        
      else
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &arg);
        return reset_adr_vals(p_result);
      }

      return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
    }
  }

  /* %rm[%rn]:
     This may also be sym[section], so operate non-destructively: */

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "[]");
  if (split_pos >= 0)
  {
    String copy;
    tStrComp copy_comp, index_arg, base_arg;
    Byte index_reg, base_reg;

    StrCompMkTemp(&copy_comp, copy, sizeof(copy));
    StrCompCopy(&copy_comp, &arg);

    StrCompSplitRef(&base_arg, &index_arg, &copy_comp, &copy[split_pos]);
    KillPostBlanksStrComp(&base_arg);
    KillPrefBlanksStrCompRef(&index_arg);
    StrCompShorten(&index_arg, 1);
    KillPostBlanksStrComp(&index_arg);

    switch (decode_reg(&base_arg, &base_reg, False))
    {
      case eRegAbort:
        return reset_adr_vals(p_result);
      case eIsReg:
        switch (decode_reg(&index_arg, &index_reg, False))
        {
          case eRegAbort:
            return reset_adr_vals(p_result);
          case eIsReg:
            if (MomCPU < cpu_32200)
            {
              WrStrErrorPos(ErrNum_AddrModeNotSupported, &arg);
              return reset_adr_vals(p_result);
            }
            else if (base_reg < 16)
            {
              WrStrErrorPos(ErrNum_InvAddrMode, &base_arg);
              return reset_adr_vals(p_result);
            }
            else if (!check_ext_regs(base_reg, &base_arg))
              return reset_adr_vals(p_result);
            else if ((index_reg >= 16) || (index_reg == REG_PSW) || (index_reg == REG_PC))
            {
              WrStrErrorPos(ErrNum_InvAddrMode, &index_arg);
              return reset_adr_vals(p_result);
            }
            p_result->vals[p_result->count++] = 0xdb;
            p_result->vals[p_result->count++] = (index_reg << 4) | (base_reg & 0xf);
            return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
          default:
            break;
        }
        break;
      default:
        break;
    }
  }

  /* Default PC-relative */

  address = EvalStrIntExpressionWithFlags(&arg, UInt32, &ok, &p_result->val_flags);
  if (ok)
  {
    LongInt disp = address - (pc_value + 1);

    append_reg_disp(p_result, disp, REG_PC, deferred);
    return check_mode_mask(mode_mask, MModMem,  p_arg, p_result);
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals_int(const adr_vals_t *p_vals)
 * \brief  append encoded address expression
 * \param  p_vals values to append
 * ------------------------------------------------------------------------ */

static void append_adr_vals(const adr_vals_t *p_vals)
{
  SetMaxCodeLen(CodeLen + p_vals->count);
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->count);
  if (p_vals->val_offset < p_vals->count)
  set_b_guessed(p_vals->val_flags,
                CodeLen + p_vals->val_offset,
                p_vals->count - p_vals->val_offset,
                p_vals->val_guess_mask);
  CodeLen += p_vals->count;
}

/*!------------------------------------------------------------------------
 * \fn     extract_reg_num(const adr_vals_t *p_vals)
 * \brief  retrieve the register involved in the encoded addressing mode
 * \param  p_vals encoded addressing mode
 * \return register # or 32 if no register involved
 * ------------------------------------------------------------------------ */

static Byte extract_reg_num(const adr_vals_t *p_vals)
{
  if (p_vals->count < 1)
    return 32;
  switch (p_vals->vals[0] >> 4)
  {
    case 0x4:
    case 0x5:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xc:
    case 0xd:
      return p_vals->vals[0] & 0xf;
    case 0x7:
      return REG_AP;
    case 0x6:
      return REG_FP;
    default:
      return 32;
  }
}

/*!------------------------------------------------------------------------
 * \fn     set_exec_mode(unsigned new_exec_mode)
 * \brief  set assumed execution mode incl. readable symbol
 * \param  new_exec_mode mode to set (0...3)
 * ------------------------------------------------------------------------ */

static void set_exec_mode(unsigned new_exec_mode)
{
  tStrComp tmp_comp;

  curr_exec_mode_set = True;
  curr_exec_mode = new_exec_mode;
  PushLocHandle(-1);
  StrCompMkTemp(&tmp_comp, (char*)exec_mode_name, 0);
  EnterIntSymbol(&tmp_comp, curr_exec_mode, SegNone, True);
  PopLocHandle();
}

/*-------------------------------------------------------------------------*/
/* Instruction Encoders */

static size_t code_len(Word op_code)
{
  return Hi(op_code) ? 2 : 1;
}

/*!------------------------------------------------------------------------
 * \fn     append_opcode(Word op_code)
 * \brief  append opcode to instruction stream
 * \param  op_code opcode to append
 * ------------------------------------------------------------------------ */

static void append_opcode(Word op_code)
{
  SetMaxCodeLen(CodeLen + code_len(op_code));

  if (Hi(op_code))
    BAsmCode[CodeLen++] = Hi(op_code);
  BAsmCode[CodeLen++] = Lo(op_code);
}

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     decode_gen(Word index)
 * \brief  handle generic orders
 * \param  index index int instruction table
 * ------------------------------------------------------------------------ */

static void decode_gen(Word index)
{
  const gen_order_t *p_order = &gen_orders[index];
  unsigned tot_count;
  size_t z;

  if (!ChkArgCnt(p_order->arg_cnt, p_order->arg_cnt))
    return;

  if ((p_order->core_flags & e_core_flag_we32200) && (MomCPU != cpu_32200))
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return;
  }

  if (curr_exec_mode > p_order->max_exec_mode)
  {
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
    return;
  }

  append_opcode(p_order->code);
  tot_count = code_len(p_order->code);
  for (z = 0; z < p_order->arg_cnt; z++)
  {
    Boolean ret;
    adr_vals_t adr_vals;

    /* Decode n-th argument */

    op_size = p_order->op_size[z];
    ret = decode_adr(&ArgStr[z + 1], &adr_vals, EProgCounter() + tot_count, p_order->adr_mode_mask[z]);
    if (!ret)
    {
      CodeLen = 0;
      return;
    }
    append_adr_vals(&adr_vals);
    tot_count += adr_vals.count;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_var_arg(Word index)
 * \brief  handle instructions with a variable number of generic operands
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void decode_var_arg(Word index)
{
  const var_arg_order_t *p_var_arg_order = &var_arg_orders[index];
  size_t z;

  for (z = 0; z < as_array_size(p_var_arg_order->gen_indices); z++)
    if (p_var_arg_order->gen_indices[z] < 0)
      break;
    else if (ArgCnt == (int)gen_orders[p_var_arg_order->gen_indices[z]].arg_cnt)
    {
      decode_gen(p_var_arg_order->gen_indices[z]);
      return;
    }
  WrError(ErrNum_WrongArgCnt);
}

/*!------------------------------------------------------------------------
 * \fn     append_branch(tSymbolSize disp_size, Byte disp8_inc, const tStrComp *p_arg)
 * \brief  handle append branch displacement to code
 * \param  disp_size displacement size to be used (8/16/auto)
 * \param  disp8_inc opcode increment for 8 bit displacement
 * \param  p_arg source argument containing target address
 * ------------------------------------------------------------------------ */

static Boolean append_branch(tSymbolSize disp_size, Byte disp8_inc, const tStrComp *p_arg)
{
  tSymbolFlags flags;
  Boolean ok;
  LongWord dest;
  LongInt dist;

  dest = EvalStrIntExpressionWithFlags(p_arg, UInt32, &ok, &flags);
  if (!ok)
    return False;

  switch (disp_size)
  {
    case eSymbolSize8Bit:
      dist = dest - (EProgCounter() + CodeLen + 1);
    is_8:
      if (!mFirstPassUnknownOrQuestionable(flags) && !RangeCheck(dist, SInt8))
      {
        WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        return False;
      }
      else
      {
        set_b_guessed(flags, CodeLen, 1, 0xff);
        BAsmCode[CodeLen++] = dist & 0xff;
      }
      break;
    case eSymbolSize16Bit:
      dist = dest - (EProgCounter() + CodeLen + 2);
    is_16:
      if (!mFirstPassUnknownOrQuestionable(flags) && !RangeCheck(dist, SInt16))
      {
        WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
        return False;
      }
      else
      {
        set_b_guessed(flags, CodeLen, 2, 0xff);
        BAsmCode[CodeLen++] = dist & 0xff;
        BAsmCode[CodeLen++] = (dist >> 8) & 0xff;
      }
      break;
    default:
      dist = dest - (EProgCounter() + CodeLen + 1);
      if (RangeCheck(dist, SInt8))
      {
        BAsmCode[0] += disp8_inc;
        goto is_8;
      }
      else
      {
        dist--;
        goto is_16;
      }
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch(Word index)
 * \brief  handle branch instructions
 * \param  index opcode & displacement size
 * ------------------------------------------------------------------------ */

static void decode_branch(Word index)
{
  if (!ChkArgCnt(1, 1))
    return;

  append_opcode(Lo(index));
  if (!append_branch((tSymbolSize)((index >> 8) & 0xff), 0x01, &ArgStr[1]))
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     decode_decrement_branch(Word index)
 * \brief  handle [Tcc]DT<B|H> instructions
 * \param  index opcode & displacement size
 * ------------------------------------------------------------------------ */

static void decode_decrement_branch(Word code)
{
  adr_vals_t adr_vals;
  Byte opcode = Lo(code);

  if (!ChkArgCnt(2, 2))
    return;

  if (MomCPU < cpu_32200)
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return;
  }
  append_opcode(opcode);

  if (decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + CodeLen, MModReg | MModMem))
  {
    append_adr_vals(&adr_vals);
    if (!append_branch((tSymbolSize)((code >> 8) & 0xff), ((opcode & 0x0f) == 0x09) ? 0x10 : 0x40, &ArgStr[2]))
      CodeLen = 0;
  }
  else
    CodeLen = 0;
}

/*!------------------------------------------------------------------------
 * \fn     void decode_save_restore(Word opcode)
 * \brief  handle SAVE/RESTORE instructions
 * \param  opcode machine opcode
 * ------------------------------------------------------------------------ */

static void decode_save_restore(Word opcode)
{
  adr_vals_t adr_vals;

  if (ChkArgCnt(1, 1) && decode_adr(&ArgStr[1], &adr_vals, EProgCounter() + code_len(opcode), MModReg))
  {
    if (extract_reg_num(&adr_vals) > 9) WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[1]);
    append_opcode(opcode);
    append_adr_vals(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_byte_op(Word opcode)
 * \brief  handle instruction having byte(s) as argument
 * \param  opcode machine opcode
 * ------------------------------------------------------------------------ */

static void decode_byte_op(Word opcode)
{
  unsigned num_bytes = Hi(opcode);

  if (ChkArgCnt(num_bytes, num_bytes))
  {
    unsigned z;

    append_opcode(Lo(opcode));
    for (z = 1; z <= num_bytes; z++)
    {
      Boolean ok;
      tSymbolFlags flags;

      BAsmCode[CodeLen] = EvalStrIntExpressionWithFlags(&ArgStr[z], Int8, &ok, &flags);
      if (ok)
      {
        set_b_guessed(flags, CodeLen, 1, 0xff);
        CodeLen++;
      }
      else
      {
        CodeLen = 0;
        return;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_coprocessor_op(Word opcode)
 * \brief  handle coprocessor instructions
 * \param  opcode machine opcode
 * ------------------------------------------------------------------------ */

static void decode_coprocessor_op(Word opcode)
{
  unsigned num_operands = Hi(opcode);
  Boolean ok;
  tSymbolFlags flags;
  LongWord value;
  unsigned z;

  if (!ChkArgCnt(1 + num_operands, 1 + num_operands))
    return;

  value = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ok, &flags);
  if (!ok)
    return;
  append_opcode(Lo(opcode));
  for (z = 0; z < 4; z++, value >>= 8)
  {
    set_b_guessed(flags, CodeLen, 1, 0xff);
    BAsmCode[CodeLen++] = value & 0xff;
  }

  for (z = 0; z < num_operands; z++)
  {
    adr_vals_t adr_vals;

    if (!decode_adr(&ArgStr[z + 2], &adr_vals, EProgCounter() + CodeLen, MModMem))
    {
      CodeLen = 0;
      return;
    }
    append_adr_vals(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_exec_mode(Word index)
 * \brief  handle EXECMODE instruction
 * ------------------------------------------------------------------------ */

static void decode_exec_mode(Word index)
{
  static const char mode_names[][11] = { "kernel", "executive", "supervisor", "user" };
  unsigned mode;
  Boolean ok;

  UNUSED(index);

  if (!ChkArgCnt(1, 1))
    return;
  for (mode = 0; mode < as_array_size(mode_names); mode++)
    if (!as_strcasecmp(ArgStr[1].str.p_str, mode_names[mode]))
    {
      set_exec_mode(mode);
      return;
    }

  mode = EvalStrIntExpression(&ArgStr[1], UInt2, &ok);
  if (ok)
    set_exec_mode(mode);
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static unsigned get_adr_mode_mask(char specifier)
{
  switch (as_toupper(specifier))
  {
    case 'R':
      return MModImm | MModLit | MModMem | MModReg;
    case 'W':
    case 'M':
      return MModMem | MModReg;
    case 'V':
      return MModMem | MModImm | MModReg;
    case 'B':
      return MModBranch;
    case 'A':
      return MModMem;
    default:
      abort();
      return 0;
  }
}

static gen_order_t *rsv_gen_order(Word code)
{
  gen_order_t *p_order;

  order_array_rsv_end(gen_orders, gen_order_t);
  p_order = &gen_orders[InstrZ];
  p_order->code = code;
  p_order->max_exec_mode = 3;
  p_order->core_flags = e_core_flag_none;
  p_order->arg_cnt = 0;
  return p_order;
}

static var_arg_order_t *rsv_var_arg_order(void)
{
  var_arg_order_t *p_order;
  size_t z;

  dyn_array_rsv_end(var_arg_orders, var_arg_order_t, var_arg_op_cnt);
  p_order = &var_arg_orders[var_arg_op_cnt];
  for (z = 0; z < as_array_size(p_order->gen_indices); z++)
    p_order->gen_indices[z] = -1;
  return p_order;
}

static const char *handle_order_format(gen_order_t *p_order, const char *p_format)
{
  unsigned mask = MModExpand;

  p_order->arg_cnt = 0;
  if (*p_format == '+')
  {
    p_order->core_flags |= e_core_flag_we32200;
    p_format++;
  }
  if (as_isdigit(*p_format))
    p_order->max_exec_mode = *p_format++ - '0';
  for (; *p_format; p_format++)
    switch (as_toupper(*p_format))
    {
      case '-':
        mask &= ~MModExpand;
        break;
      default:
        p_order->adr_mode_mask[p_order->arg_cnt++] = mask | get_adr_mode_mask(*p_format);
        mask = MModExpand;
        break;
    }
  return p_format;
}

static void add_fixed(const char *p_name, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  handle_order_format(p_order, p_format);
  AddInstTable(InstTable, p_name, InstrZ++, decode_gen);
}

static int add_one_op_size(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);
   
  handle_order_format(p_order, p_format);
  assert(p_order->arg_cnt == 1);
  p_order->op_size[0] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_two_op(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  handle_order_format(p_order, p_format);
  assert(p_order->arg_cnt == 2);
  p_order->op_size[0] =
  p_order->op_size[1] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_three_op(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  handle_order_format(p_order, p_format);
  assert(p_order->arg_cnt == 3);
  p_order->arg_cnt = 3;
  p_order->op_size[0] =
  p_order->op_size[1] =
  p_order->op_size[2] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_four_op(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  handle_order_format(p_order, p_format);
  assert(p_order->arg_cnt == 4);
  p_order->op_size[0] =
  p_order->op_size[1] =
  p_order->op_size[2] = eSymbolSize32Bit;
  p_order->op_size[3] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_three_op_shift(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  handle_order_format(p_order, p_format);
  assert(p_order->arg_cnt == 3);
  p_order->op_size[0] = eSymbolSize32Bit;
  p_order->op_size[1] =
  p_order->op_size[2] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_two_three_op(const char *p_name, tSymbolSize op_size, const char *p_format, Word code)
{
  char name[20];
  var_arg_order_t *p_order = rsv_var_arg_order();

  as_snprintf(name, sizeof(name), "%s2", p_name);
  p_order->gen_indices[0] = add_two_op(name, op_size, "rw", code);
  as_snprintf(name, sizeof(name), "%s3", p_name);
  p_order->gen_indices[1] = add_three_op(name, op_size, p_format, code + 0x40);
  AddInstTable(InstTable, p_name, var_arg_op_cnt++, decode_var_arg);
  return 0;
}

typedef int (*add_op_one_size_t)(const char *p_name, tSymbolSize op_size, const char *p_format, Word code);

static void add_allsize(const char *p_name, add_op_one_size_t add_fnc, const char *p_format, Word code)
{
  char name[20];
   
  as_snprintf(name, sizeof(name), p_name, 'B');
  add_fnc(name, eSymbolSize8Bit, p_format, code);
  as_snprintf(name, sizeof(name), p_name, 'H');
  add_fnc(name, eSymbolSize16Bit, p_format, code - 1);
  as_snprintf(name, sizeof(name), p_name, 'W');
  add_fnc(name, eSymbolSize32Bit, p_format, code - 3);
}

static void add_branch_core(const char *p_name, tSymbolSize op_size, Word code)
{
  AddInstTable(InstTable, p_name, (((Word)(op_size & 0xff)) << 8) | code, decode_branch);
}

static void add_branch(const char *p_name, Word code)
{
  char name[20];

  add_branch_core(p_name, eSymbolSizeUnknown, code);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  add_branch_core(name, eSymbolSize8Bit, code + 0x01);
  as_snprintf(name, sizeof(name), "%sH", p_name);
  add_branch_core(name, eSymbolSize16Bit, code);
}

static void add_decrement_branch_core(const char *p_name, tSymbolSize op_size, Word code)
{
  AddInstTable(InstTable, p_name, (((Word)(op_size & 0xff)) << 8) | code, decode_decrement_branch);
}

static void add_decrement_branch(const char *p_name, Word code)
{
  char name[20];

  add_decrement_branch_core(p_name, eSymbolSizeUnknown, code);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  add_decrement_branch_core(name, eSymbolSize8Bit, code + (((code & 0x0f) == 0x09) ? 0x10 : 0x40));
  as_snprintf(name, sizeof(name), "%sH", p_name);
  add_decrement_branch_core(name, eSymbolSize16Bit, code);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(403);
  SetDynamicInstTable(InstTable);
  var_arg_op_cnt = InstrZ = 0;

  add_null_pseudo(InstTable);

  add_fixed("MVERNO", "", 0x3009);
  add_fixed("NOP",  "", NOPCode);
  add_fixed("RCC", "", 0x50);
  add_fixed("RCS", "", 0x58);
  add_fixed("REQL", "", 0x7c);
  add_fixed("REQLU", "", 0x6c);
  add_fixed("RET", "", 0x18);
  add_fixed("RGEQ", "", 0x40);
  add_fixed("RGEQU", "", 0x50);
  add_fixed("RGTR", "", 0x44);
  add_fixed("RGTRU", "", 0x54);
  add_fixed("RLEQ", "", 0x4c);
  add_fixed("RLEQU", "", 0x5c);
  add_fixed("RLSS", "", 0x48);
  add_fixed("RLSSU", "", 0x58);
  add_fixed("RNEQ", "", 0x74);
  add_fixed("RNEQU", "", 0x64);
  add_fixed("RSB", "", 0x78);
  add_fixed("RVC", "", 0x60);
  add_fixed("RVS", "", 0x68);
  add_fixed("STRCPY", "", 0x3035);
  add_fixed("STREND", "", 0x301f);
  add_fixed("BPT", "", 0x2e);
  add_fixed("CFLUSH", "", 0x27);
  add_fixed("MOVBLW", "", 0x3019);
  add_fixed("CALLPS", "0", 0x30ac);
  add_fixed("DISVJMP", "0", 0x3013);
  add_fixed("ENBVJMP", "0", 0x300d);
  add_fixed("INTACK", "0", 0x302f);
  add_fixed("RETPS", "0", 0x30c8);
  add_fixed("WAIT", "0", 0x2f);
  add_fixed("GATE", "", 0x3061);
  add_fixed("RETG", "", 0x3045);
  add_fixed("CLRX", "+", 0x0b);
  add_fixed("SETX", "+", 0x0a);
  add_fixed("RETQINT", "+0", 0x98);
  add_fixed("UCALLPS", "+0", 0x30c0);

  add_allsize("MOV%c",  add_two_op, "rw", 0x87);
  add_allsize("MCOM%c", add_two_op, "rw", 0x8b);
  add_allsize("MNEG%c", add_two_op, "rw", 0x8f);
  add_allsize("CMP%c",  add_two_op, "rr", 0x3f);
  add_allsize("BIT%c",  add_two_op, "rr", 0x3b);

  add_allsize("DEC%c", add_one_op_size, "m", 0x97);
  add_allsize("INC%c", add_one_op_size, "m", 0x93);
  add_allsize("TST%c", add_one_op_size, "r", 0x2b);
  add_allsize("CLR%c", add_one_op_size, "m", 0x83);

  add_one_op_size("POPW", eSymbolSize32Bit, "-m", 0x20);
  add_one_op_size("PUSHW", eSymbolSize32Bit, "-r", 0xa0);

  add_allsize("SWAP%cI", add_one_op_size, "-a", 0x1f);
  add_one_op_size("PUSHA", eSymbolSize32Bit, "-a", 0xe0);
  add_one_op_size("JMP", eSymbolSize32Bit, "a", 0x24);
  add_one_op_size("JSB", eSymbolSize32Bit, "-a", 0x34);

  add_allsize("ADD%c", add_two_three_op, "rrw", 0x9f);
  add_allsize("SUB%c", add_two_three_op, "rrw", 0xbf);
  add_allsize("MUL%c", add_two_three_op, "rrw", 0xab);
  add_allsize("DIV%c", add_two_three_op, "rrw", 0xaf);
  add_allsize("MOD%c", add_two_three_op, "rrw", 0xa7);
  add_allsize("AND%c", add_two_three_op, "rrw", 0xbb);
  add_allsize("XOR%c", add_two_three_op, "rrw", 0xb7);
  add_allsize("OR%c" , add_two_three_op, "rrw", 0xb3);
  add_two_three_op("ADDPB", eSymbolSize8Bit, "+rrw", 0xa3);
  add_two_three_op("SUBPB", eSymbolSize8Bit, "+rrw", 0x9b);

  add_three_op_shift("ALSW3", eSymbolSize32Bit, "rrw", 0xc0);
  add_allsize("ARS%c3", add_three_op_shift, "rrw", 0xc7);
  add_allsize("LLS%c3", add_three_op_shift, "rrw", 0xd3);
  add_three_op_shift("LRSW3", eSymbolSize32Bit, "rrw", 0xd4);
  add_three_op_shift("ROTW" , eSymbolSize32Bit, "rrw", 0xd8);

  add_branch("BCC" , 0x52);
  add_branch("BCS" , 0x5a);
  add_branch("BVC" , 0x62);
  add_branch("BVS" , 0x6a);
  add_branch("BEU" , 0x6e);
  add_branch("BE"  , 0x7e);
  add_branch("BNE" , 0x76);
  add_branch("BNEU", 0x66);
  add_branch("BL"  , 0x4a);
  add_branch("BLU" , 0x5a);
  add_branch("BLE" , 0x4e);
  add_branch("BLEU", 0x5e);
  add_branch("BG"  , 0x46);
  add_branch("BGU" , 0x56);
  add_branch("BGE" , 0x42);
  add_branch("BGEU", 0x52);
  add_branch("BR"  , 0x7a);
  add_branch("BSB" , 0x36);

  /* Encoding of [Tcc]DTx instructions is a bit guessed: */

  add_decrement_branch("DT", 0x19);
  add_decrement_branch("TEDT", 0x0d);
  add_decrement_branch("TGDT", 0x2d);
  add_decrement_branch("TGEDT", 0x1d);
  add_decrement_branch("TNEDT", 0x3d);

  AddInstTable(InstTable, "RESTORE", 0x18, decode_save_restore);
  AddInstTable(InstTable, "SAVE", 0x10, decode_save_restore);
  AddInstTable(InstTable, "EXTOP", 0x0114, decode_byte_op);
  AddInstTable(InstTable, "NOP2", 0x0173, decode_byte_op);
  AddInstTable(InstTable, "NOP3", 0x0272, decode_byte_op);

  AddInstTable(InstTable, "SPOP"  , 0x0032, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPRS", 0x0122, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPRD", 0x0102, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPRT", 0x0106, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPS2", 0x0223, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPD2", 0x0203, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPT2", 0x0207, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPWS", 0x0133, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPWD", 0x0113, decode_coprocessor_op);
  AddInstTable(InstTable, "SPOPWT", 0x0117, decode_coprocessor_op);

  add_two_op("CALL", eSymbolSize32Bit, "-a-a", 0x2c);
  add_two_op("MOVAW", eSymbolSize32Bit, "aw", 0x04);
  add_two_op("MOVTRW", eSymbolSize32Bit, "aw", 0x0c);

  add_allsize("EXTF%c", add_four_op, "rrrw", 0xcf);
  add_allsize("INSF%c", add_four_op, "rrrw", 0xcb);

#if 0
  /* need detail info about these WE32200 instructions: */

  add_("CASWI", "+", 0x09);
  add_("PACKB", "+", 0x0e);
  add_("UNPACKB", "+", 0x0f);
#endif

  AddInstTable(InstTable, exec_mode_name, 0, decode_exec_mode);
  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "WORD", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDD);
  AddInstTable(InstTable, "HALF", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt, DecodeIntelDW);
  AddInstTable(InstTable, "BYTE", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDB);
  AddInstTable(InstTable, "DS", 0, DecodeIntelDS);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  order_array_free(var_arg_orders);
  order_array_free(gen_orders);
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_we32(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for WE32xxx
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_we32(char *p_arg, TempResult *p_result)
{
  Byte reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_we32;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_we32(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void make_code_we32(void)
{
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_we32(void)
 * \brief  deinitialize as target
 * ------------------------------------------------------------------------ */

static void switch_from_we32(void)
{
  deinit_fields();
}

/*!------------------------------------------------------------------------
 * \fn     is_def_we32(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */
     
static Boolean is_def_we32(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_we32(void *p_user)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static Boolean true_func(void)
{
  return True;
}

static void switch_to_we32(void)
{
  const TFamilyDescr *p_descr;

  p_descr = FindFamilyByName("WE32xxx");
  SetIntConstMode(eIntConstModeC);

  PCSymbol = ".";
  HeaderID = p_descr->Id;
  NOPCode = 0x70;
  DivideChars = ",";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[UInt32].Max;

  MakeCode = make_code_we32;
  IsDef = is_def_we32;
  SwitchFrom = switch_from_we32;
  InternSymbol = intern_symbol_we32;
  DissectReg = dissect_reg_we32;

  SaveIsOccupiedFnc =
  RestoreIsOccupiedFnc = true_func;

  init_fields();

  if (!curr_exec_mode_set)
    set_exec_mode(3);
}

/*!------------------------------------------------------------------------
 * \fn     initpass_we32(void)
 * \brief  initializations at beginning of pass
 * ------------------------------------------------------------------------ */

static void initpass_we32(void)
{
  /* Flag to introduce & initialize symbol at first switch to target */

  curr_exec_mode_set = False;
}

/*!------------------------------------------------------------------------
 * \fn     codewe32_init(void)
 * \brief  register WE32xxx target
 * ------------------------------------------------------------------------ */
    
void codewe32_init(void)
{
  cpu_32100 = AddCPU("WE32100", switch_to_we32);
  cpu_32200 = AddCPU("WE32200", switch_to_we32);
  AddInitPassProc(initpass_we32);
}
