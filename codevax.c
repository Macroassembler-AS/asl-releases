/* codevax.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator VAX                                                        */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <assert.h>

#include "bpemu.h"
#include "strutil.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmcode.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "errmsg.h"
#include "intpseudo.h"
#include "decpseudo.h"
#include "decfloat.h"
#include "codevax.h"

typedef enum
{
  e_cpu_cap_none = 0,
  e_cpu_cap_float_g = 1 << 0,
  e_cpu_cap_float_h = 1 << 1,
  /* MOVC3 MOVC5 mandatory */
  /* CMPC3, LOCC, SCANC, SKPC, SPANC */
  e_cpu_cap_string_mvax = 1 << 2,
  e_cpu_cap_string_rest = 1 << 3,
  e_cpu_cap_crc = 1 << 4,
  e_cpu_cap_packed = 1 << 5,
  e_cpu_cap_edit = 1 << 6,
  e_cpu_cap_vector = 1 << 7
} cpu_cap_t;

#define e_cpu_cap_string (e_cpu_cap_string_mvax | e_cpu_cap_string_rest)
#define e_cpu_cap_all_no_vector (e_cpu_cap_float_g | e_cpu_cap_float_h | e_cpu_cap_string | e_cpu_cap_crc | e_cpu_cap_packed | e_cpu_cap_edit)
#define e_cpu_cap_all_vector (e_cpu_cap_all_no_vector | e_cpu_cap_vector)

#ifdef __cplusplus
# include "codevax.hpp"
#endif

typedef struct
{
  char name[15];
  cpu_cap_t caps;
} cpu_props_t;

typedef struct
{
  int gen_indices[4];
} var_arg_order_t;

typedef struct
{
  Word code;
  ShortInt bit_field_arg_start;
  Boolean bit_field_size_present;
  Byte max_access_mode;
  size_t arg_cnt;
  cpu_cap_t required_caps;
  unsigned adr_mode_mask[6];
  tSymbolSize op_size[6];
} gen_order_t;

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

typedef struct
{
  adr_mode_t mode;
  unsigned count;
  Byte vals[20];
  LargeInt imm_value;
  tSymbolFlags imm_flags;
} adr_vals_t;

static const cpu_props_t *p_curr_cpu_props;
static tSymbolSize op_size;
static gen_order_t *gen_orders;
static var_arg_order_t *var_arg_orders;
static unsigned var_arg_op_cnt;
static Boolean curr_access_mode_set;
static unsigned curr_access_mode;
static const char access_mode_name[] = "ACCMODE";

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

#define REG_AP 12
#define REG_FP 13
#define REG_SP 14
#define REG_PC 15

static const char xtra_reg_names[8] =
{
  'A','P',
  'F','P',
  'S','P',
  'P','C'
};

static Boolean decode_reg_core(const char *p_arg, Byte *p_result, tSymbolSize *p_size)
{
  switch (strlen(p_arg))
  {
    case 2:
    {
      int z;

      for (z = 0; z < 8; z += 2)
        if ((as_toupper(p_arg[0]) == xtra_reg_names[z + 0])
         && (as_toupper(p_arg[1]) == xtra_reg_names[z + 1]))
      {
        *p_result = ((z / 2) + REG_AP) | REGSYM_FLAG_ALIAS;
        *p_size = eSymbolSize32Bit;
        return True;
      }
      if ((as_toupper(*p_arg) == 'R')
       && isdigit(p_arg[1]))
      {
        *p_result = p_arg[1] - '0';
        *p_size = eSymbolSize32Bit;
        return True;
      }
      break;
    }
    case 3:
      if ((as_toupper(*p_arg) == 'R')
       && (as_toupper(p_arg[1]) == '1')
       && isdigit(p_arg[2])
       && (p_arg[2] < '6'))
      {
        *p_result = 10 + p_arg[2] - '0';
        *p_size = eSymbolSize32Bit;
        return True;
      }
      break;
    default:
      break;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_vax(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - VAX variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void dissect_reg_vax(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize32Bit:
    {
      unsigned r_num = value & 15;

      if ((value & REGSYM_FLAG_ALIAS) && (r_num >= REG_AP))
        as_snprintf(p_dest, dest_size, "%2.2s", &xtra_reg_names[(r_num - REG_AP) * 2]);
      else
        as_snprintf(p_dest, dest_size, "R%u", r_num);
      break;
    }
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*--------------------------------------------------------------------------*/
/* Address Decoding */

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
 * \brief  reset encoded addressing
 * \param  p_vals buffer to reset
 * \return constant False for convenience
 * ------------------------------------------------------------------------ */

static Boolean reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = ModNone;
  p_vals->count = 0;
  p_vals->imm_value = 0;
  p_vals->imm_flags = eSymbolFlag_None;
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals_int(adr_vals_t *p_vals, LargeWord int_value, tSymbolSize size)
 * \brief  append integer value to encoded address value
 * \param  p_vals where to append
 * \param  int_value value to append
 * \param  size integer size
 * ------------------------------------------------------------------------ */

static void append_adr_vals_int(adr_vals_t *p_vals, LargeWord int_value, tSymbolSize size)
{
  unsigned num_iter = GetSymbolSizeBytes(size), z;

  for (z = 0; z < num_iter; z++)
  {
    p_vals->vals[p_vals->count++] = int_value & 0xff;
    int_value >>= 8;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(tStrComp *p_arg, adr_vals_t *p_result, Word pc_value, unsigned mode_mask)
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

static Boolean is_pre_decrement(const tStrComp *p_arg, Byte *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '-')
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

static Boolean is_post_increment(const tStrComp *p_arg, Byte *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '(')
   || (p_arg->str.p_str[arg_len - 2] != ')')
   || (p_arg->str.p_str[arg_len - 1] != '+'))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 1, arg_len - 3);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean decode_abs(const tStrComp *p_arg, adr_vals_t *p_result)
{
  Boolean ok;
  LongWord address = EvalStrIntExpression(p_arg, UInt32, &ok);

  if (ok)
    append_adr_vals_int(p_result, address, eSymbolSize32Bit);

  return ok;
}

static int index_qualifier(const char *p_arg, int last_non_blank_pos, int split_pos)
{
  /* extra check for post increment: good enough? */

  int ret = ((last_non_blank_pos >= 4)
       && (p_arg[0] == '(')
       && (p_arg[last_non_blank_pos - 1] == ')')
       && (p_arg[last_non_blank_pos] == '+'))
   ? split_pos : -1;
  return ret;
}

static Boolean decode_adr(tStrComp *p_arg, adr_vals_t *p_result, LongWord pc_value, unsigned mode_mask)
{
  tStrComp arg, len_spec_arg;
  Boolean deferred;
  Byte reg, index_reg;
  tEvalResult eval_result;
  tRegEvalResult reg_eval_result;
  int arg_len, split_pos;
  char len_spec, ch;

  reset_adr_vals(p_result);

  /* split off deferred flag? */

  deferred = (p_arg->str.p_str[0] == '@');
  StrCompRefRight(&arg, p_arg, deferred);
  if (deferred)
    KillPrefBlanksStrCompRef(&arg);

  /* Split off index register (which must not be PC): */

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, index_qualifier, "[]");
  if (split_pos > 0)
  {
    String reg_str;
    tStrComp reg_comp;

    StrCompMkTemp(&reg_comp, reg_str, sizeof(reg_str));
    StrCompCopySub(&reg_comp, &arg, split_pos + 1, arg_len - split_pos - 2);
    KillPostBlanksStrComp(&reg_comp);
    KillPrefBlanksStrComp(&reg_comp);
    switch (decode_reg(&reg_comp, &index_reg, False))
    {
      case eRegAbort:
        return False;
      case eIsReg:
        StrCompShorten(&arg, arg_len - split_pos);
        p_result->vals[p_result->count++] = (index_reg |= 0x40);
        pc_value++;
        break;
      default:
        break;
    }
  }
  else 
    index_reg = 0;

  /* Plain register? */

  switch (decode_reg(&arg, &reg, False))
  {
    case eIsReg:
      if (index_reg && !deferred)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_result);
      }
      p_result->vals[p_result->count++] = reg | (deferred ? 0x60 : 0x50);
      return check_mode_mask(mode_mask, deferred ? MModMem : MModReg, p_arg, p_result);
    case eRegAbort:
      return reset_adr_vals(p_result);
    default:
      break;
  }

  /* Split off length specifier (BWLIS)^... */

  if ((strlen(arg.str.p_str) > 2)
   && ('^' == arg.str.p_str[1])
   && strchr("BWLIS", (ch = as_toupper(arg.str.p_str[0]))))
  {
    StrCompSplitRef(&len_spec_arg, &arg, &arg, &arg.str.p_str[1]);
    len_spec = ch;
    KillPrefBlanksStrCompRef(&arg);
  }
  else
  {
    len_spec = '\0';
    LineCompReset(&len_spec_arg.Pos);
  }

  /* #imm, @#abs */

  if (*arg.str.p_str == '#')
  {
    tStrComp imm_arg;

    StrCompRefRight(&imm_arg, &arg, 1);

    /* @#abs */

    if (deferred)
    {
      p_result->vals[p_result->count++] = 0x90 | REG_PC;
      eval_result.OK = decode_abs(&imm_arg, p_result);
      return eval_result.OK
             ? check_mode_mask(mode_mask, MModMem, p_arg, p_result)
             : reset_adr_vals(p_result);
    }

    /* #imm */

    else
    {
      IntType eval_int_type;
      int (*float_convert)(as_float_t inp, Word *p_dest);
      Byte *p_specifier;

      if (index_reg)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_result);
      }

      p_specifier = &p_result->vals[p_result->count]; p_result->count++;

      if (len_spec && (len_spec != 'S') && (len_spec != 'I'))
      {
        WrStrErrorPos(ErrNum_UndefAttr, &len_spec_arg);
        return reset_adr_vals(p_result);
      } 

      switch (op_size)
      {
        case eSymbolSize8Bit:
          eval_int_type = Int8;
          goto int_common;
        case eSymbolSize16Bit:
          eval_int_type = Int16;
          goto int_common;
        case eSymbolSize32Bit:
          eval_int_type = Int32;
          goto int_common;
        case eSymbolSize64Bit:
          eval_int_type = Int64;
          goto int_common;
        case eSymbolSize128Bit:
          eval_int_type = Int128;
          goto int_common;

        case eSymbolSizeFloat32Bit:
          float_convert = as_float_2_dec_f;
          goto float_common;

        case eSymbolSizeFloat64Bit:
          float_convert = as_float_2_dec_d;
          goto float_common;

        case eSymbolSizeFloat64Bit_G:
          float_convert = as_float_2_dec_g;
          goto float_common;

        case eSymbolSizeFloat128Bit:
          float_convert = as_float_2_dec_h;
          goto float_common;

        int_common:
        {
          if (len_spec == 'S')
            eval_int_type = UInt6;
          p_result->imm_value = EvalStrIntExpressionWithFlags(&imm_arg, eval_int_type, &eval_result.OK, &p_result->imm_flags);
          if (!eval_result.OK)
            return reset_adr_vals(p_result);
          if (!len_spec)
            len_spec = (RangeCheck(p_result->imm_value, UInt6) && (mode_mask & MModLit)) ? 'S' : 'I';
          if (len_spec == 'S')
          {
            *p_specifier = p_result->imm_value & 63;
            return check_mode_mask(mode_mask, MModLit, p_arg, p_result);
          }
          else
          {
            *p_specifier = 0x80 | REG_PC;
            append_adr_vals_int(p_result, (LargeWord)p_result->imm_value, op_size);
            return check_mode_mask(mode_mask, MModImm, p_arg, p_result);
          }
        }

        float_common:
        {
          as_float_t value = EvalStrFloatExpressionWithResult(&imm_arg, &eval_result);

          if (!eval_result.OK)
            return reset_adr_vals(p_result);
          if (len_spec == 'S')
          {
            int ret = as_float_2_dec_lit(value, p_specifier);
            if (ret <= 0)
            {
              asmerr_check_fp_dispose_result(ret, &imm_arg);
              return reset_adr_vals(p_result);
            }
          }
          else if (!len_spec && (mode_mask & MModLit) && (as_float_2_dec_lit(value, p_specifier) > 0)) { }
          else
          {
            Word buf[8];
            int z, ret = float_convert(value, buf);

            if (ret < 0)
            {
              asmerr_check_fp_dispose_result(ret, &imm_arg);
              return reset_adr_vals(p_result);
            }
            *p_specifier = 0x80 | REG_PC;
            for (z = 0; z < ret; z++)
            {
              p_result->vals[p_result->count++] = Lo(buf[z]);
              p_result->vals[p_result->count++] = Hi(buf[z]);
            }
          }
          return check_mode_mask(mode_mask, (p_result->count > 1) ? MModImm : MModLit, p_arg, p_result);
        }

        default:
          WrStrErrorPos(ErrNum_InvOpSize, &imm_arg);
          return reset_adr_vals(p_result);
      }
    }
  }

  /* (Rn)+, @(Rn)+ */

  if (is_post_increment(&arg, &reg, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return reset_adr_vals(p_result);
    else if (len_spec)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return reset_adr_vals(p_result);
    }
    if (index_reg && (reg == (index_reg & 0xf)))
      WrStrErrorPos(ErrNum_Unpredictable,p_arg);
    p_result->vals[p_result->count++] = reg | (deferred ? 0x90 : 0x80);
    return check_mode_mask(mode_mask, MModMem, p_arg, p_result);
  }

  /* -(Rn), @-(Rn) (not supported on VAX) */

  if (is_pre_decrement(&arg, &reg, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return reset_adr_vals(p_result);
    else if (deferred || len_spec)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
      return reset_adr_vals(p_result);
    }
    if (index_reg && (reg == (index_reg & 0xf)))
      WrStrErrorPos(ErrNum_Unpredictable,p_arg);
    p_result->vals[p_result->count++] = reg | 0x70;
    return check_mode_mask(mode_mask, MModMem, p_arg, p_result);
  }

  /* (Rn), X(Rn) */

  split_pos = FindDispBaseSplitWithQualifier(arg.str.p_str, &arg_len, NULL, "()");
  if (split_pos >= 0)
  {
    tStrComp disp_arg, reg_arg;

    StrCompSplitRef(&disp_arg, &reg_arg, &arg, &arg.str.p_str[split_pos]);
    KillPostBlanksStrComp(&disp_arg);
    KillPrefBlanksStrCompRef(&reg_arg);
    StrCompShorten(&reg_arg, 1);
    KillPostBlanksStrComp(&reg_arg);

    if (decode_reg(&reg_arg, &reg, True) != eIsReg)
    {
      WrStrErrorPos(ErrNum_InvReg, &reg_arg);
      return reset_adr_vals(p_result);
    }

    /* (Rn) */

    if (!*disp_arg.str.p_str && !deferred)
    {
      if (len_spec)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_result);
      }
      p_result->vals[p_result->count++] = reg | 0x60;
    }

    /* X(Rn) */

    else
    {
      LongInt disp;
      tSymbolSize size = eSymbolSizeUnknown;
      IntType eval_int_type;

      switch (len_spec)
      {
        case 'B':
          eval_int_type = SInt8;
          size = eSymbolSize8Bit;
          break;
        case 'W':
          eval_int_type = SInt16;
          size = eSymbolSize16Bit;
          break;
        case 'L':
          size = eSymbolSize32Bit;
          /* FALL-THRU */
        case '\0':
          eval_int_type = SInt32;
          break;
        default: /* I,S not allowed here */
          WrStrErrorPos(ErrNum_UndefAttr, &len_spec_arg);
          return reset_adr_vals(p_result);
      }
      if (disp_arg.str.p_str[0])
        disp = EvalStrIntExpressionWithResult(&disp_arg, eval_int_type, &eval_result);
      else
      {
        disp = 0;
        eval_result.OK = True;
      }
      if (!eval_result.OK)
        return reset_adr_vals(p_result);

      /* deduce displacement size */

      if (eSymbolSizeUnknown == size)
      {
        if (RangeCheck(disp, SInt8))
          size = eSymbolSize8Bit;
        else if (RangeCheck(disp, SInt16))
          size = eSymbolSize16Bit;
        else
          size = eSymbolSize32Bit;
      }

      /* write out addressing mode */

      switch (size)
      {
        case eSymbolSize32Bit:
          p_result->vals[p_result->count++] = reg | (deferred ? 0xf0 : 0xe0);
          break;
        case eSymbolSize16Bit:
          p_result->vals[p_result->count++] = reg | (deferred ? 0xd0 : 0xc0);
          break;
        default:
          p_result->vals[p_result->count++] = reg | (deferred ? 0xb0 : 0xa0);
      }
      append_adr_vals_int(p_result, disp, size);
    }
    return check_mode_mask(mode_mask, MModMem, p_arg, p_result);
  }

  {
    LongInt dist;
    tSymbolSize dist_size;

    /* Remains: rel, @rel
       PC value is the PC value after displacement was loaded,
       which is the 'current PC' within the instruction we got as
       argument, plus the specifier and optional index byte,
       plus 1, 2, or 4 bytes for the displacement itself: */

    dist = EvalStrIntExpressionWithResult(&arg, UInt32, &eval_result) - pc_value;
    if (!eval_result.OK)
      return False;

    if (!len_spec)
    {
      if (RangeCheck(dist - 2, SInt8))
        len_spec = 'B';
      else if (RangeCheck(dist - 3, SInt16))
        len_spec = 'W';
      else
        len_spec = 'L';
    }
    switch (len_spec)
    {
      case 'B':
        p_result->vals[p_result->count++] = REG_PC | (deferred ? 0xb0 : 0xa0);
        dist -= 2;
        if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && !ChkRangeByType(dist, SInt8, &arg))
          return reset_adr_vals(p_result);
        dist_size = eSymbolSize8Bit;
        break;
      case 'W':
        p_result->vals[p_result->count++] = REG_PC | (deferred ? 0xd0 : 0xc0);
        dist -= 3;
        if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && !ChkRangeByType(dist, SInt16, &arg))
          return reset_adr_vals(p_result);
        dist_size = eSymbolSize16Bit;
        break;
      case 'L':
        p_result->vals[p_result->count++] = REG_PC | (deferred ? 0xf0 : 0xe0);
        dist -= 5;
        dist_size = eSymbolSize32Bit;
        break;
      default:
        WrStrErrorPos(ErrNum_UndefAttr, &len_spec_arg);
        return reset_adr_vals(p_result);
    }
    append_adr_vals_int(p_result, (LongWord)dist, dist_size);
    return check_mode_mask(mode_mask, MModMem, p_arg, p_result);
  }
}

/*!------------------------------------------------------------------------
 * \fn     is_register(const adr_vals_t *p_vals)
 * \brief  checks whether encoded argument is a register
 * \param  p_vals encoded argument
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_register(const adr_vals_t *p_vals)
{
  return (p_vals->count > 0)
      && ((p_vals->vals[0] & 0xf0) == 0x50);
}

/*!------------------------------------------------------------------------
 * \fn     is_immediate(const adr_vals_t *p_vals)
 * \brief  checks whether encoded argument is immediate
 * \param  p_vals encoded argument
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_immediate(const adr_vals_t *p_vals)
{
  return (p_vals->count > 0)
      && ((p_vals->vals[0] == (0x80 | REG_PC)) /* i^... */
       || (p_vals->vals[0] <= 0x3f)); /* s^... */
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch(tStrComp *p_arg, adr_vals_t *p_result, LongWord pc_value)
 * \brief  handle branch addressing mode
 * \param  p_arg source argument
 * \param  p_result dest buffer
 * \param  pc_value value of PC to use in calculation
 * \return True if argument OK
 * ------------------------------------------------------------------------ */

static Boolean decode_branch(tStrComp *p_arg, adr_vals_t *p_result, LongWord pc_value)
{
  tSymbolFlags flags;
  Boolean ok;
  LongWord dest;
  LongInt dist;
  IntType range_int;
  tSymbolSize act_op_size;

  reset_adr_vals(p_result);
  dest = EvalStrIntExpressionWithFlags(p_arg, UInt32, &ok, &flags);
  if (!ok)
    return False;
  if (op_size == eSymbolSizeUnknown)
  {
    dist = dest - (pc_value + 1);
    if (RangeCheck(dist, SInt8))
      act_op_size = eSymbolSize8Bit;
    else
    {
      act_op_size = eSymbolSize16Bit;
      BAsmCode[CodeLen - 1] += 0x20;
    }
  }
  else
    act_op_size = op_size;
  dist = dest - (pc_value + GetSymbolSizeBytes(act_op_size));
  if (!mFirstPassUnknownOrQuestionable(flags))
  {
    switch (act_op_size)
    {
      case eSymbolSize8Bit:
        range_int = SInt8;
        goto check;
      case eSymbolSize16Bit:
        range_int = SInt16;
        goto check;
      default:
        return False;
      check:
        if (!RangeCheck(dist, range_int))
        {
          WrStrErrorPos(ErrNum_JmpDistTooBig, p_arg);
          return False;
        }
    }
  }
  append_adr_vals_int(p_result, dist, act_op_size);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals(const adr_vals_t *p_vals)
 * \brief  append addressing mode values to instruction stream
 * \param  p_vals values to append
 * ------------------------------------------------------------------------ */

static void append_adr_vals(const adr_vals_t *p_vals)
{
  SetMaxCodeLen(CodeLen + p_vals->count);
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->count);
  CodeLen += p_vals->count;
}

/*!------------------------------------------------------------------------
 * \fn     set_access_mode(unsigned new_access_mode)
 * \brief  set assumed access mode incl. readable symbol
 * \param  new_access_mode mode to set (0...3)
 * ------------------------------------------------------------------------ */

static void set_access_mode(unsigned new_access_mode)
{
  tStrComp tmp_comp;

  curr_access_mode_set = True;
  curr_access_mode = new_access_mode;
  PushLocHandle(-1);
  StrCompMkTemp(&tmp_comp, (char*)access_mode_name, 0);
  EnterIntSymbol(&tmp_comp, curr_access_mode, SegNone, True);
  PopLocHandle();
}

/*--------------------------------------------------------------------------*/
/* Instruction Handler Helpers */

/*!------------------------------------------------------------------------
 * \fn     code_len(Word op_code)
 * \brief  check whether opcode is one or two byte opcode
 * \param  op_code opcode
 * \return 1 or 2
 * ------------------------------------------------------------------------ */

static int code_len(Word op_code)
{
  return 1 + !!Hi(op_code);
}

/*!------------------------------------------------------------------------
 * \fn     append_opcode(Word op_code)
 * \brief  append opcode to instruction stream
 * \param  op_code opcode to append
 * ------------------------------------------------------------------------ */

static void append_opcode(Word op_code)
{
  SetMaxCodeLen(CodeLen + code_len(op_code));

  BAsmCode[CodeLen++] = Lo(op_code);
  if (Hi(op_code))
    BAsmCode[CodeLen++] = Hi(op_code);
}

/*!------------------------------------------------------------------------
 * \fn     chk_required_caps(Word required_caps)
 * \brief  check whether current target supports all required capabilities
 * \param  required_caps bit mask of required capabilities
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean chk_required_caps(Word required_caps)
{
  if ((p_curr_cpu_props->caps & required_caps) != required_caps)
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
  else
    return True;
}

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     decode_gen(Word index)
 * \brief  handle generic instruction
 * \param  index index into instruction list
 * ------------------------------------------------------------------------ */

static void decode_gen(Word index)
{
  const gen_order_t *p_order = &gen_orders[index];
  adr_vals_t adr_vals;
  unsigned tot_count;
  size_t z;
  Boolean bit_field_size_imm = False,
          bit_field_pos_imm = False;
  tSymbolFlags bit_field_pos_flags = eSymbolFlag_None,
               bit_field_size_flags = eSymbolFlag_None;
  LongWord bit_field_pos = 0;
  Byte bit_field_size = 0;

  if (!ChkArgCnt(p_order->arg_cnt, p_order->arg_cnt)
   || !chk_required_caps(p_order->required_caps))
    return;

  if (curr_access_mode > p_order->max_access_mode)
  {
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
    return;
  }

  append_opcode(p_order->code);
  tot_count = code_len(p_order->code);
  for (z = 0; z < p_order->arg_cnt; z++)
  {
    Boolean ret;

    /* Decode n-th argument */

    op_size = p_order->op_size[z];
    ret = (p_order->adr_mode_mask[z] == MModBranch)
        ? decode_branch(&ArgStr[z + 1], &adr_vals, EProgCounter() + tot_count)
        : decode_adr(&ArgStr[z + 1], &adr_vals, EProgCounter() + tot_count, p_order->adr_mode_mask[z]);
    if (!ret)
    {
      CodeLen = 0;
      return;
    }

    /* Consistency checks for bit field size/position arguments.
       Of course, we can only warn if the size/pos values are immediate.
       The field size may be missing if it is implicitly one: */

    if (p_order->bit_field_arg_start >= 0)
    {
      int bfield_index = z - p_order->bit_field_arg_start;

      switch (bfield_index)
      {
        case 0: /* position argument, independent of presence of size argument: */
          if (is_immediate(&adr_vals))
          {
            bit_field_pos_imm = True;
            bit_field_pos = adr_vals.imm_value;
            bit_field_pos_flags = adr_vals.imm_flags;  
          }
          break;
        case 1: /* size argument or already location argument: */
          if (!p_order->bit_field_size_present)
          {
            bit_field_size_imm = True;
            bit_field_size = 1;
            bit_field_size_flags = eSymbolFlag_None;
            goto check_location;
          }
          else if (is_immediate(&adr_vals))
          {
            bit_field_size_imm = True;
            bit_field_size = adr_vals.imm_value;
            bit_field_size_flags = adr_vals.imm_flags;
            if (!mFirstPassUnknownOrQuestionable(bit_field_size_flags)
             && (bit_field_size > 32))
            {
              WrStrErrorPos(ErrNum_InvBitField, &ArgStr[z + 1]);
              CodeLen = 0;
              return;
            }
          }
          break;
        case 2: /* location argument if size argument was present */
          if (p_order->bit_field_size_present)
            goto check_location;
          break;
        check_location:
          if (is_register(&adr_vals))
          {
            if (bit_field_size_imm
             && bit_field_pos_imm
             && !mFirstPassUnknownOrQuestionable(bit_field_size_flags)
             && !mFirstPassUnknownOrQuestionable(bit_field_pos_flags)
             && (bit_field_size != 0)
             && (bit_field_pos > 31))
            {
              WrStrErrorPos(ErrNum_InvBitField, &ArgStr[z + 1]);
              CodeLen = 0;
              return;
            }
          }
          break;
      }
    }

    /* Append to instruction stream: */

    append_adr_vals(&adr_vals);
    tot_count += adr_vals.count;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_case(Word code)
 * \brief  Handle CASEx instructions
 * \param  code machine code & operand size
 * ------------------------------------------------------------------------ */

static void decode_case(Word code)
{
  int z;
  adr_vals_t adr_vals;
  unsigned tot_count;

  /* Selector, base & limit are mandatory: */

  if (!ChkArgCnt(3, ArgCntMax))
    return;

  /* Opcode first: */

  op_size = (tSymbolSize)Hi(code);
  append_opcode(Lo(code));
  tot_count = 1;

  /* Decode the fixed arguments: */

  for (z = 1; z <= 3; z++)
  {
    if (!decode_adr(&ArgStr[z], &adr_vals, EProgCounter() + tot_count, MModReg | MModMem | MModLit | MModImm))
    {
      CodeLen = 0;
      return;
    }

    /* Consistency check: */

    if ((3 == z)
     && is_immediate(&adr_vals)
     && (adr_vals.imm_value + 4 != ArgCnt))
      WrStrErrorPos(ErrNum_CaseWrongArgCnt, &ArgStr[z]);

    append_adr_vals(&adr_vals);
    tot_count += adr_vals.count;
  }

  /* Do not increment tot_count for branch arguments, since all displacements
     are relative to disp[0].  Further, we have to compensate for decode_branch()
     adding the displacement's size: */

  op_size = eSymbolSize16Bit;
  for (; z <= ArgCnt; z++)
  {
    if (!decode_branch(&ArgStr[z], &adr_vals, EProgCounter() + tot_count - 2))
    {
      CodeLen = 0;
      return;
    }
    append_adr_vals(&adr_vals);
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
 * \fn     decode_accmode(Word index)
 * \brief  handle ACCMODE instruction
 * ------------------------------------------------------------------------ */

static void decode_accmode(Word index)
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
      set_access_mode(mode);
      return;
    }

  mode = EvalStrIntExpression(&ArgStr[1], UInt2, &ok);
  if (ok)
    set_access_mode(mode);
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
    case 'A':
      return MModMem | MModImm;
    case 'V':
      return MModMem | MModImm | MModReg;
    case 'B':
      return MModBranch;
    default:
      return 0;
  }
}

static gen_order_t *rsv_gen_order(Word code)
{
  gen_order_t *p_order;

  order_array_rsv_end(gen_orders, gen_order_t);
  p_order = &gen_orders[InstrZ];
  p_order->code = code;
  p_order->bit_field_arg_start = -1;
  p_order->bit_field_size_present = False;
  p_order->max_access_mode = 3;
  p_order->required_caps = e_cpu_cap_none;
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

static void add_fixed(const char *p_name, Word code)
{
  (void)rsv_gen_order(code);

  AddInstTable(InstTable, p_name, InstrZ++, decode_gen);
}

static void add_one_op(const char *p_name, tSymbolSize op_size, Boolean allow_dest_immediate, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  p_order->arg_cnt = 1;
  p_order->adr_mode_mask[0] = get_adr_mode_mask(allow_dest_immediate ? 'r' : 'w');
  p_order->op_size[0] = op_size;
  AddInstTable(InstTable, p_name, InstrZ++, decode_gen);
}

static int add_two_op_size(const char *p_name, tSymbolSize src_op_size, tSymbolSize dest_op_size, Boolean allow_dest_immediate, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  p_order->arg_cnt = 2;
  p_order->adr_mode_mask[0] = get_adr_mode_mask('r');
  p_order->adr_mode_mask[1] = get_adr_mode_mask(allow_dest_immediate ? 'r' : 'w');
  p_order->op_size[0] = src_op_size;
  p_order->op_size[1] = dest_op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static int add_two_op(const char *p_name, tSymbolSize op_size, Boolean allow_dest_immediate, Word code)
{
  return add_two_op_size(p_name, op_size, op_size, allow_dest_immediate, code);
}

static int add_three_op(const char *p_name, tSymbolSize op_size, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  p_order->arg_cnt = 3;
  p_order->adr_mode_mask[0] =
  p_order->adr_mode_mask[1] = get_adr_mode_mask('r');
  p_order->adr_mode_mask[2] = get_adr_mode_mask('w');
  p_order->op_size[0] =
  p_order->op_size[1] =
  p_order->op_size[2] = op_size;
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

static void add_two_three_op(const char *p_name, tSymbolSize op_size, Word code)
{
  char name[20];
  var_arg_order_t *p_order = rsv_var_arg_order();

  as_snprintf(name, sizeof(name), "%s2", p_name);
  p_order->gen_indices[0] = add_two_op(name, op_size, False, code);
  as_snprintf(name, sizeof(name), "%s3", p_name);
  p_order->gen_indices[1] = add_three_op(name, op_size, Hi(code) ? (code + 0x100) : (code + 1));
  AddInstTable(InstTable, p_name, var_arg_op_cnt++, decode_var_arg);
}

static void add_byte_two_op(const char *p_name, tSymbolSize op_size, Word code)
{
  gen_order_t *p_order = rsv_gen_order(code);

  p_order->arg_cnt = 3;
  p_order->adr_mode_mask[0] =
  p_order->adr_mode_mask[1] = get_adr_mode_mask('r');
  p_order->adr_mode_mask[2] = get_adr_mode_mask('w');
  p_order->op_size[0] = eSymbolSize8Bit;
  p_order->op_size[1] =
  p_order->op_size[2] = op_size;
  AddInstTable(InstTable, p_name, InstrZ++, decode_gen);
}

static int add_gen_caps(const char *p_name, const char *p_format, Word code, cpu_cap_t required_caps)
{
  gen_order_t *p_order = rsv_gen_order(code);

  p_order->required_caps = required_caps;
  if (isdigit(*p_format))
  {
    p_order->max_access_mode = *p_format - '0';
    p_format++;
  }
  for (; p_format[0] && p_format[1]; p_format += 2)
  {
    assert(p_order->arg_cnt < as_array_size(p_order->adr_mode_mask));
    p_order->adr_mode_mask[p_order->arg_cnt] = get_adr_mode_mask(p_format[0]);
    if ('V' == as_toupper(p_format[0]))
    {
      if (p_order->arg_cnt >= 2)
      {
        p_order->bit_field_arg_start = p_order->arg_cnt - 2;
        p_order->bit_field_size_present = True;
      }
      else /* BBx operate on bit fields of implicit size one */
        p_order->bit_field_arg_start = p_order->arg_cnt - 1;
    }
    switch (as_toupper(p_format[1]))
    {
      case 'X':
        p_order->op_size[p_order->arg_cnt] = eSymbolSizeUnknown;
        break;
      case 'B':
        p_order->op_size[p_order->arg_cnt] = eSymbolSize8Bit;
        break;
      case 'W':
        p_order->op_size[p_order->arg_cnt] = eSymbolSize16Bit;
        break;
      case 'L':
        p_order->op_size[p_order->arg_cnt] = eSymbolSize32Bit;
        break;
      case 'F':
        p_order->op_size[p_order->arg_cnt] = eSymbolSizeFloat32Bit;
        break;
      case 'Q':
        p_order->op_size[p_order->arg_cnt] = eSymbolSize64Bit;
        break;
      case 'D':
        p_order->op_size[p_order->arg_cnt] = eSymbolSizeFloat64Bit;
        break;
      case 'G':
        p_order->op_size[p_order->arg_cnt] = eSymbolSizeFloat64Bit_G;
        p_order->required_caps |= e_cpu_cap_float_g;
        break;
      case 'O':
        p_order->op_size[p_order->arg_cnt] = eSymbolSize128Bit;
        break;
      case 'H':
        p_order->op_size[p_order->arg_cnt] = eSymbolSizeFloat128Bit;
        p_order->required_caps |= e_cpu_cap_float_h;
        break;
      default: assert(0);
    }
    p_order->arg_cnt++;
  }
  AddInstTable(InstTable, p_name, InstrZ, decode_gen);
  return InstrZ++;
}

#define add_gen(p_name, p_format, code) \
        add_gen_caps(p_name, p_format, code, e_cpu_cap_none)

static void init_fields(void)
{
  var_arg_order_t *p_var_arg_order;
  InstTable = CreateInstTable(403);
  SetDynamicInstTable(InstTable);

  add_null_pseudo(InstTable);

  var_arg_op_cnt =
  InstrZ = 0;

  add_gen("HALT", "0", 0x00);
  add_fixed("NOP",  NOPCode);
  add_fixed("RSB", 0x05);
  add_fixed("RET", 0x04);
  add_fixed("BPT", 0x03);
  add_fixed("BUGW", 0xfeff);
  add_fixed("BUGL", 0xfdff);
  add_fixed("XFC", 0xfc);
  add_fixed("REI", 0x02);
  add_gen("LDPCTX", "0", 0x06);
  add_gen("SVPCTX", "0", 0x07);

  add_one_op("CLRB" , eSymbolSize8Bit  , False, 0x94);
  add_one_op("CLRW" , eSymbolSize16Bit , False, 0xb4);
  add_one_op("CLRL" , eSymbolSize32Bit , False, 0xd4);
  add_one_op("CLRQ" , eSymbolSize64Bit , False, 0x7c);
  add_one_op("CLRO" , eSymbolSize128Bit, False, 0x7cfd);
  add_one_op("CLRF" , eSymbolSizeFloat32Bit, False, 0xd4);
  add_one_op("CLRD" , eSymbolSizeFloat64Bit, False, 0x7c);
  add_one_op("CLRG" , eSymbolSizeFloat64Bit_G, False, 0x7c);
  add_one_op("CLRH" , eSymbolSizeFloat128Bit, False, 0x7cfd);
  add_one_op("DECB" , eSymbolSize8Bit  , False, 0x97);
  add_one_op("DECW" , eSymbolSize16Bit , False, 0xb7);
  add_one_op("DECL" , eSymbolSize32Bit , False, 0xd7);
  add_one_op("INCB" , eSymbolSize8Bit  , False, 0x96);
  add_one_op("INCW" , eSymbolSize16Bit , False, 0xb6);
  add_one_op("INCL" , eSymbolSize32Bit , False, 0xd6);
  add_one_op("PUSHL", eSymbolSize32Bit , True , 0xdd);
  add_one_op("TSTB" , eSymbolSize8Bit  , True , 0x95);
  add_one_op("TSTW" , eSymbolSize16Bit , True , 0xb5);
  add_one_op("TSTL" , eSymbolSize32Bit , True , 0xd5);
  add_one_op("TSTF" , eSymbolSizeFloat32Bit, True , 0x53);
  add_one_op("TSTD" , eSymbolSizeFloat64Bit, True , 0x73);
  add_one_op("TSTG" , eSymbolSizeFloat64Bit_G, True , 0x53fd);
  add_one_op("TSTH" , eSymbolSizeFloat128Bit, True , 0x73fd);
  add_one_op("BICPSW",eSymbolSize16Bit , True , 0xb9);
  add_one_op("BISPSW",eSymbolSize16Bit , True , 0xb8);
  add_one_op("MOVPSL",eSymbolSize32Bit , False, 0xdc);
  add_one_op("POPR" , eSymbolSize16Bit , True , 0xba);
  add_one_op("PUSHR", eSymbolSize16Bit , True , 0xbb);

  add_two_op("ADAWI", eSymbolSize16Bit , False, 0x58);
  add_two_op("ADWC" , eSymbolSize32Bit , False, 0xd8);
  add_two_op("BITB" , eSymbolSize8Bit  , True , 0x93);
  add_two_op("BITW" , eSymbolSize16Bit , True , 0xb3);
  add_two_op("BITL" , eSymbolSize32Bit , True , 0xd3);
  add_two_op("CMPB" , eSymbolSize8Bit  , True , 0x91);
  add_two_op("CMPW" , eSymbolSize16Bit , True , 0xb1);
  add_two_op("CMPL" , eSymbolSize32Bit , True , 0xd1);
  add_two_op("CMPF" , eSymbolSizeFloat32Bit, True , 0x51);
  add_two_op("CMPD" , eSymbolSizeFloat64Bit, True , 0x71);
  add_two_op("CMPG" , eSymbolSizeFloat64Bit_G, True , 0x51fd);
  add_two_op("CMPH" , eSymbolSizeFloat128Bit, True , 0x71fd);
  add_two_op("MOVB" , eSymbolSize8Bit  , False, 0x90);
  add_two_op("MOVW" , eSymbolSize16Bit , False, 0xb0);
  add_two_op("MOVL" , eSymbolSize32Bit , False, 0xd0);
  add_two_op("MOVQ" , eSymbolSize64Bit , False, 0x7d);
  add_two_op("MOVO" , eSymbolSize128Bit, False, 0x7dfd);
  add_two_op("MOVF" , eSymbolSizeFloat32Bit, False, 0x50);
  add_two_op("MOVD" , eSymbolSizeFloat64Bit, False, 0x70);
  add_two_op("MOVG" , eSymbolSizeFloat64Bit_G, False, 0x50fd);
  add_two_op("MOVH" , eSymbolSizeFloat128Bit, False, 0x70fd);
  add_two_op("MCOMB", eSymbolSize8Bit  , False, 0x92);
  add_two_op("MCOMW", eSymbolSize16Bit , False, 0xb2);
  add_two_op("MCOML", eSymbolSize32Bit , False, 0xd2);
  add_two_op("MNEGB", eSymbolSize8Bit  , False, 0x8e);
  add_two_op("MNEGW", eSymbolSize16Bit , False, 0xae);
  add_two_op("MNEGL", eSymbolSize32Bit , False, 0xce);
  add_two_op("MNEGF", eSymbolSizeFloat32Bit, False, 0x52);
  add_two_op("MNEGD", eSymbolSizeFloat64Bit, False, 0x72);
  add_two_op("MNEGG", eSymbolSizeFloat64Bit_G, False, 0x52fd);
  add_two_op("MNEGH", eSymbolSizeFloat128Bit, False, 0x72fd);
  add_two_op("SBWC" , eSymbolSize32Bit , False, 0xd9);

  add_two_op_size("CVTBW" , eSymbolSize8Bit,  eSymbolSize16Bit, False, 0x99);
  add_two_op_size("CVTBL" , eSymbolSize8Bit,  eSymbolSize32Bit, False, 0x98);
  add_two_op_size("CVTWB" , eSymbolSize16Bit, eSymbolSize8Bit,  False, 0x33);
  add_two_op_size("CVTWL" , eSymbolSize16Bit, eSymbolSize32Bit, False, 0x32);
  add_two_op_size("CVTLB" , eSymbolSize32Bit, eSymbolSize8Bit,  False, 0xf6);
  add_two_op_size("CVTLW" , eSymbolSize32Bit, eSymbolSize16Bit, False, 0xf7);
  add_two_op_size("CVTFB" , eSymbolSizeFloat32Bit, eSymbolSize8Bit, False, 0x48);
  add_two_op_size("CVTFW" , eSymbolSizeFloat32Bit, eSymbolSize16Bit, False, 0x49);
  add_two_op_size("CVTFL" , eSymbolSizeFloat32Bit, eSymbolSize32Bit, False, 0x4a);
  add_two_op_size("CVTRFL", eSymbolSizeFloat32Bit, eSymbolSize32Bit, False, 0x4b);
  add_two_op_size("CVTBF" , eSymbolSize8Bit, eSymbolSizeFloat32Bit, False, 0x4c);
  add_two_op_size("CVTWF" , eSymbolSize16Bit, eSymbolSizeFloat32Bit, False, 0x4d);
  add_two_op_size("CVTLF" , eSymbolSize32Bit, eSymbolSizeFloat32Bit, False, 0x4e);
  add_two_op_size("CVTFD" , eSymbolSizeFloat32Bit, eSymbolSizeFloat64Bit, False, 0x56);
  add_two_op_size("CVTDB" , eSymbolSizeFloat64Bit, eSymbolSize8Bit, False, 0x68);
  add_two_op_size("CVTDW" , eSymbolSizeFloat64Bit, eSymbolSize16Bit, False, 0x69);
  add_two_op_size("CVTDL" , eSymbolSizeFloat64Bit, eSymbolSize32Bit, False, 0x6a);
  add_two_op_size("CVTRDL", eSymbolSizeFloat64Bit, eSymbolSize32Bit, False, 0x6b);
  add_two_op_size("CVTBD" , eSymbolSize8Bit, eSymbolSizeFloat64Bit, False, 0x6c);
  add_two_op_size("CVTWD" , eSymbolSize16Bit, eSymbolSizeFloat64Bit, False, 0x6d);
  add_two_op_size("CVTLD" , eSymbolSize32Bit, eSymbolSizeFloat64Bit, False, 0x6e);
  add_two_op_size("CVTDF" , eSymbolSizeFloat64Bit, eSymbolSize32Bit, False, 0x76);
  add_two_op_size("CVTDH" , eSymbolSizeFloat64Bit, eSymbolSizeFloat128Bit, False, 0x32fd);
  add_two_op_size("CVTGF" , eSymbolSizeFloat64Bit_G, eSymbolSizeFloat32Bit, False, 0x33fd);
  add_two_op_size("CVTGB" , eSymbolSizeFloat64Bit_G, eSymbolSize8Bit, False, 0x48fd);
  add_two_op_size("CVTGW" , eSymbolSizeFloat64Bit_G, eSymbolSize16Bit, False, 0x49fd);
  add_two_op_size("CVTGL" , eSymbolSizeFloat64Bit_G, eSymbolSize32Bit, False, 0x4afd);
  add_two_op_size("CVTRGL", eSymbolSizeFloat64Bit_G, eSymbolSize32Bit, False, 0x4bfd);
  add_two_op_size("CVTBG" , eSymbolSize8Bit, eSymbolSizeFloat64Bit_G, False, 0x4cfd);
  add_two_op_size("CVTWG" , eSymbolSize16Bit, eSymbolSizeFloat64Bit_G, False, 0x4dfd);
  add_two_op_size("CVTLG" , eSymbolSize32Bit, eSymbolSizeFloat64Bit_G, False, 0x4efd);
  add_two_op_size("CVTGH" , eSymbolSizeFloat64Bit_G, eSymbolSizeFloat128Bit, False, 0x56fd);
  add_two_op_size("CVTHB" , eSymbolSizeFloat128Bit, eSymbolSize8Bit, False, 0x68fd);
  add_two_op_size("CVTHW" , eSymbolSizeFloat128Bit, eSymbolSize16Bit, False, 0x69fd);
  add_two_op_size("CVTHL" , eSymbolSizeFloat128Bit, eSymbolSize32Bit, False, 0x6afd);
  add_two_op_size("CVTRHL", eSymbolSizeFloat128Bit, eSymbolSize32Bit, False, 0x6bfd);
  add_two_op_size("CVTBH" , eSymbolSize8Bit, eSymbolSizeFloat128Bit, False, 0x6cfd);
  add_two_op_size("CVTWH" , eSymbolSize16Bit, eSymbolSizeFloat128Bit, False, 0x6dfd);
  add_two_op_size("CVTLH" , eSymbolSize32Bit, eSymbolSizeFloat128Bit, False, 0x6efd);
  add_two_op_size("CVTHG" , eSymbolSizeFloat128Bit, eSymbolSizeFloat64Bit_G, False, 0x76fd);
  add_two_op_size("CVTFH" , eSymbolSizeFloat32Bit, eSymbolSizeFloat128Bit, False, 0x98fd);
  add_two_op_size("CVTFG" , eSymbolSizeFloat32Bit, eSymbolSizeFloat64Bit_G, False, 0x99fd);
  add_two_op_size("CVTHF" , eSymbolSizeFloat128Bit, eSymbolSizeFloat32Bit, False, 0xf6fd);
  add_two_op_size("CVTHD" , eSymbolSizeFloat128Bit, eSymbolSizeFloat64Bit, False, 0xf7fd);
  add_two_op_size("MOVZBW", eSymbolSize8Bit,  eSymbolSize16Bit, False, 0x9b);
  add_two_op_size("MOVZBL", eSymbolSize8Bit,  eSymbolSize32Bit, False, 0x9a);
  add_two_op_size("MOVZWL", eSymbolSize16Bit, eSymbolSize32Bit, False, 0x3c);

  add_two_three_op("ADDB", eSymbolSize8Bit,  0x80);
  add_two_three_op("ADDW", eSymbolSize16Bit, 0xa0);
  add_two_three_op("ADDL", eSymbolSize32Bit, 0xc0);
  add_two_three_op("ADDF", eSymbolSizeFloat32Bit, 0x40);
  add_two_three_op("ADDD", eSymbolSizeFloat64Bit, 0x60);
  add_two_three_op("ADDG", eSymbolSizeFloat64Bit_G, 0x40fd);
  add_two_three_op("ADDH", eSymbolSizeFloat128Bit, 0x60fd);
  add_two_three_op("BICB", eSymbolSize8Bit,  0x8a);
  add_two_three_op("BICW", eSymbolSize16Bit, 0xaa);
  add_two_three_op("BICL", eSymbolSize32Bit, 0xca);
  add_two_three_op("BISB", eSymbolSize8Bit,  0x88);
  add_two_three_op("BISW", eSymbolSize16Bit, 0xa8);
  add_two_three_op("BISL", eSymbolSize32Bit, 0xc8);
  add_two_three_op("DIVB", eSymbolSize8Bit,  0x86);
  add_two_three_op("DIVW", eSymbolSize16Bit, 0xa6);
  add_two_three_op("DIVL", eSymbolSize32Bit, 0xc6);
  add_two_three_op("DIVF", eSymbolSizeFloat32Bit, 0x46);
  add_two_three_op("DIVD", eSymbolSizeFloat64Bit, 0x66);
  add_two_three_op("DIVG", eSymbolSizeFloat64Bit_G, 0x46fd);
  add_two_three_op("DIVH", eSymbolSizeFloat128Bit, 0x66fd);
  add_two_three_op("MULB", eSymbolSize8Bit,  0x84);
  add_two_three_op("MULW", eSymbolSize16Bit, 0xa4);
  add_two_three_op("MULL", eSymbolSize32Bit, 0xc4);
  add_two_three_op("MULF", eSymbolSizeFloat32Bit, 0x44);
  add_two_three_op("MULD", eSymbolSizeFloat64Bit, 0x64);
  add_two_three_op("MULG", eSymbolSizeFloat64Bit_G, 0x44fd);
  add_two_three_op("MULH", eSymbolSizeFloat128Bit, 0x64fd);
  add_two_three_op("SUBB", eSymbolSize8Bit,  0x82);
  add_two_three_op("SUBW", eSymbolSize16Bit, 0xa2);
  add_two_three_op("SUBL", eSymbolSize32Bit, 0xc2);
  add_two_three_op("SUBF", eSymbolSizeFloat32Bit, 0x42);
  add_two_three_op("SUBD", eSymbolSizeFloat64Bit, 0x62);
  add_two_three_op("SUBG", eSymbolSizeFloat64Bit_G, 0x42fd);
  add_two_three_op("SUBH", eSymbolSizeFloat128Bit, 0x62fd);
  add_two_three_op("XORB", eSymbolSize8Bit,  0x8c);
  add_two_three_op("XORW", eSymbolSize16Bit, 0xac);
  add_two_three_op("XORL", eSymbolSize32Bit, 0xcc);

  add_byte_two_op("ASHL", eSymbolSize32Bit, 0x78);
  add_byte_two_op("ASHQ", eSymbolSize64Bit, 0x79);
  add_byte_two_op("ROTL", eSymbolSize32Bit, 0x9c);

  add_gen("EDIV"  , "rlrqwlwl", 0x7b);
  add_gen("EMUL"  , "rlrlrlwq", 0x7a);
  add_gen("MOVAB" , "abwl", 0x9e);
  add_gen("MOVAW" , "awwl", 0x3e);
  add_gen("MOVAL" , "alwl", 0xde);
  add_gen("MOVAF" , "afwl", 0xde);
  add_gen("MOVAQ" , "aqwl", 0x7e);
  add_gen("MOVAD" , "adwl", 0x7e);
  add_gen("MOVAG" , "agwl", 0x7e);
  add_gen("MOVAO" , "aowl", 0x7efd);
  add_gen("MOVAH" , "ahwl", 0x7efd);
  add_gen("PUSHAB", "ab", 0x9f);
  add_gen("PUSHAW", "aw", 0x3f);
  add_gen("PUSHAL", "al", 0xdf);
  add_gen("PUSHAF", "af", 0xdf);
  add_gen("PUSHAQ", "aq", 0x7f);
  add_gen("PUSHAD", "ad", 0x7f);
  add_gen("PUSHAG", "ag", 0x7f);
  add_gen("PUSHAO", "ao", 0x7ffd);
  add_gen("PUSHAH", "ah", 0x7ffd);
  add_gen("CMPV"  , "rlrbvbrl", 0xec);
  add_gen("CMPZV" , "rlrbvbrl", 0xed);
  add_gen("EXTV"  , "rlrbvbwl", 0xee);
  add_gen("EXTZV" , "rlrbvbwl", 0xef);
  add_gen("FFC"   , "rlrbvbwl", 0xeb);
  add_gen("FFS"   , "rlrbvbwl", 0xea);
  add_gen("INSV"  , "rlrlrbvb", 0xf0);

  add_gen("ACBB"  , "rbrbmbbw", 0x9d);
  add_gen("ACBW"  , "rwrwmwbw", 0x3d);
  add_gen("ACBL"  , "rlrlmlbw", 0xf1);
  add_gen("ACBF"  , "rfrfmfbw", 0x4f);
  add_gen("ACBD"  , "rdrdmdbw", 0x6f);
  add_gen("ACBG"  , "rgrgmgbw", 0x4ffd);
  add_gen("ACBH"  , "rhrhmhbw", 0x6ffd);
  add_gen("AOBLEQ", "rlmlbb", 0xf3);
  add_gen("AOBLSS", "rlmlbb", 0xf2);
  add_gen("BGTR"  , "bb", 0x14);
  add_gen("BLEQ"  , "bb", 0x15);
  add_gen("BNEQ"  , "bb", 0x12);
  add_gen("BNEQU" , "bb", 0x12);
  add_gen("BEQL"  , "bb", 0x13);
  add_gen("BEQLU" , "bb", 0x13);
  add_gen("BGEQ"  , "bb", 0x18);
  add_gen("BLSS"  , "bb", 0x19);
  add_gen("BGTRU" , "bb", 0x1a);
  add_gen("BLEQU" , "bb", 0x1b);
  add_gen("BVC"   , "bb", 0x1c);
  add_gen("BVS"   , "bb", 0x1d);
  add_gen("BGEQU" , "bb", 0x1e);
  add_gen("BCC"   , "bb", 0x1e);
  add_gen("BLSSU" , "bb", 0x1f);
  add_gen("BCS"   , "bb", 0x1f);
  add_gen("BBS"   , "rlvbbb", 0xe0);
  add_gen("BBC"   , "rlvbbb", 0xe1);
  add_gen("BBSS"  , "rlvbbb", 0xe2);
  add_gen("BBCS"  , "rlvbbb", 0xe3);
  add_gen("BBSC"  , "rlvbbb", 0xe4);
  add_gen("BBCC"  , "rlvbbb", 0xe5);
  add_gen("BBSSI" , "rlvbbb", 0xe6);
  add_gen("BBCCI" , "rlvbbb", 0xe7);
  add_gen("BLBS"  , "rlbb", 0xe8);
  add_gen("BLBC"  , "rlbb", 0xe9);
  add_gen("BRB"   , "bb", 0x11);
  add_gen("BRW"   , "bw", 0x31);
  add_gen("BR"    , "bx", 0x11);
  add_gen("BSBB"  , "bb", 0x10);
  add_gen("BSBW"  , "bw", 0x30);
  add_gen("BSB"   , "bx", 0x10);
  AddInstTable(InstTable, "CASEB", 0x8f | (eSymbolSize8Bit << 8), decode_case);
  AddInstTable(InstTable, "CASEW", 0xaf | (eSymbolSize16Bit << 8), decode_case);
  AddInstTable(InstTable, "CASEL", 0xcf | (eSymbolSize32Bit << 8), decode_case);
  add_gen("JMP"   , "ab", 0x17);
  add_gen("JSB"   , "ab", 0x16);
  add_gen("SOBGEQ", "mlbb", 0xf4);
  add_gen("SOBGTR", "mlbb", 0xf5);
  add_gen("CALLG" , "abab", 0xfa);
  add_gen("CALLS" , "rlab", 0xfb);
  add_gen("INDEX" , "rlrlrlrlrlwl", 0x0a);

  add_gen("INSQHI", "abaq", 0x5c);
  add_gen("INSQTI", "abaq", 0x5d);
  add_gen("INSQUE", "abab", 0x0e);
  add_gen("REMQHI", "aqwl", 0x5e);
  add_gen("REMQTI", "aqwl", 0x5f);
  add_gen("REMQUE", "abwl", 0x0f);

  add_gen("EMODF", "rfrbrfwlwf", 0x54);
  add_gen("EMODD", "rdrbrdwlwd", 0x74);
  add_gen("EMODG", "rgrwrgwlwg", 0x54fd);
  add_gen("EMODH", "rhrwrhwlwh", 0x74fd);
  add_gen("POLYF", "rfrwab", 0x55);
  add_gen("POLYD", "rdrwab", 0x75);
  add_gen("POLYG", "rgrwab", 0x55fd);
  add_gen("POLYH", "rhrwab", 0x75fd);

  p_var_arg_order = rsv_var_arg_order();
  p_var_arg_order->gen_indices[0] = add_gen_caps("CMPC3" , "rwabab"      , 0x29, e_cpu_cap_string_mvax);
  p_var_arg_order->gen_indices[1] = add_gen_caps("CMPC5" , "rwabrbrwab"  , 0x2d, e_cpu_cap_string_rest);
  AddInstTable(InstTable, "CMPC", var_arg_op_cnt++, decode_var_arg);
  add_gen_caps("LOCC"  , "rbrwab"      , 0x3a, e_cpu_cap_string_mvax);
  add_gen_caps("MATCHC", "rwabrwab"    , 0x39, e_cpu_cap_string_rest);
  p_var_arg_order = rsv_var_arg_order();
  p_var_arg_order->gen_indices[0] = add_gen("MOVC3" , "rwabab"      , 0x28);
  p_var_arg_order->gen_indices[1] = add_gen("MOVC5" , "rwabrbrwab"  , 0x2c);
  AddInstTable(InstTable, "MOVC", var_arg_op_cnt++, decode_var_arg);
  add_gen_caps("MOVTC" , "rwabrbabrwab", 0x2e, e_cpu_cap_string_rest);
  add_gen_caps("MOVTUC", "rwabrbabrwab", 0x2f, e_cpu_cap_string_rest);
  add_gen_caps("SCANC" , "rwababrb"    , 0x2a, e_cpu_cap_string_mvax);
  add_gen_caps("SKPC"  , "rbrwab"      , 0x3b, e_cpu_cap_string_mvax);
  add_gen_caps("SPANC" , "rwababrb"    , 0x2b, e_cpu_cap_string_mvax);

  add_gen_caps("CRC"   , "abrlrwab"    , 0x0b, e_cpu_cap_crc);

  p_var_arg_order = rsv_var_arg_order();
  p_var_arg_order->gen_indices[0] = add_gen_caps("ADDP4" , "rwabrwab"    , 0x20, e_cpu_cap_packed);
  p_var_arg_order->gen_indices[1] = add_gen_caps("ADDP6" , "rwabrwabrwab", 0x21, e_cpu_cap_packed);
  AddInstTable(InstTable, "ADDP", var_arg_op_cnt++, decode_var_arg);
  add_gen_caps("ASHP"  , "rbrwabrbrwab", 0xf8, e_cpu_cap_packed);
  p_var_arg_order = rsv_var_arg_order();
  p_var_arg_order->gen_indices[0] = add_gen_caps("CMPP3" , "rwabab"      , 0x35, e_cpu_cap_packed);
  p_var_arg_order->gen_indices[1] = add_gen_caps("CMPP4" , "rwabrwab"    , 0x37, e_cpu_cap_packed);
  AddInstTable(InstTable, "CMPP", var_arg_op_cnt++, decode_var_arg);
  add_gen_caps("CVTLP" , "rlrwab"      , 0xf9, e_cpu_cap_packed);
  add_gen_caps("CVTPL" , "rwabwl"      , 0x36, e_cpu_cap_packed);
  add_gen_caps("CVTPS" , "rwabrwab"    , 0x08, e_cpu_cap_packed);
  add_gen_caps("CVTPT" , "rwababrwab"  , 0x24, e_cpu_cap_packed);
  add_gen_caps("CVTSP" , "rwabrwab"    , 0x09, e_cpu_cap_packed);
  add_gen_caps("CVTTP" , "rwababrwab"  , 0x26, e_cpu_cap_packed);
  add_gen_caps("DIVP"  , "rwabrwabrwab", 0x27, e_cpu_cap_packed);
  add_gen_caps("MOVP"  , "rwabab"      , 0x34, e_cpu_cap_packed);
  add_gen_caps("MULP"  , "rwabrwabrwab", 0x25, e_cpu_cap_packed);
  p_var_arg_order = rsv_var_arg_order();
  p_var_arg_order->gen_indices[0] = add_gen_caps("SUBP4" , "rwabrwab"    , 0x22, e_cpu_cap_packed);
  p_var_arg_order->gen_indices[1] = add_gen_caps("SUBP6" , "rwabrwabrwab", 0x23, e_cpu_cap_packed);
  AddInstTable(InstTable, "SUBP", var_arg_op_cnt++, decode_var_arg);
  add_gen_caps("EDITPC", "rwababab"    , 0x38, e_cpu_cap_edit);

  add_gen("PROBER", "rbrwab"      , 0x0c);
  add_gen("PROBEW", "rbrwab"      , 0x0d);
  add_gen("CHMK"  , "rw"          , 0xbc);
  add_gen("CHME"  , "rw"          , 0xbd);
  add_gen("CHMS"  , "rw"          , 0xbe);
  add_gen("CHMU"  , "rw"          , 0xbf);
  add_gen("MTPR"  , "0rlrl"       , 0xda);
  add_gen("MFPR"  , "0rlwl"       , 0xdb);

  AddInstTable(InstTable, "BLKB",  1, DecodeIntelDS);
  AddInstTable(InstTable, "BLKW",  2, DecodeIntelDS);
  AddInstTable(InstTable, "BLKL",  4, DecodeIntelDS);
  AddInstTable(InstTable, "BLKQ",  8, DecodeIntelDS);
  AddInstTable(InstTable, "BLKO", 16, DecodeIntelDS);
  AddInstTable(InstTable, "BLKF",  4, DecodeIntelDS);
  AddInstTable(InstTable, "BLKD",  8, DecodeIntelDS);
  AddInstTable(InstTable, "BLKG",  8, DecodeIntelDS);
  AddInstTable(InstTable, "BLKH", 16, DecodeIntelDS);

  AddInstTable(InstTable, access_mode_name, 0, decode_accmode);

  /* TODO: ASCID */

  AddInstTable(InstTable, "ASCII"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowString | eIntPseudoFlag_DECFormat                       , DecodeIntelDB);
  AddInstTable(InstTable, "ASCIZ"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowString | eIntPseudoFlag_DECFormat | eIntPseudoFlag_ASCIZ, DecodeIntelDB);
  AddInstTable(InstTable, "ASCIC"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowString | eIntPseudoFlag_DECFormat | eIntPseudoFlag_ASCIC, DecodeIntelDB);
/*AddInstTable(InstTable, "ASCID"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowString | eIntPseudoFlag_DECFormat | eIntPseudoFlag_ASCID, DecodeIntelDB);*/
  AddInstTable(InstTable, "PACKED"    , 0, decode_dec_packed);

  AddInstTable(InstTable, "BYTE"      , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt                              , DecodeIntelDB);
  AddInstTable(InstTable, "WORD"      , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt                              , DecodeIntelDW);
  AddInstTable(InstTable, "LWORD"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt                              , DecodeIntelDD);
  AddInstTable(InstTable, "QUAD"      , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt                              , DecodeIntelDQ);
  AddInstTable(InstTable, "OCTA"      , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt                              , DecodeIntelDO);
  AddInstTable(InstTable, "FLOAT"     , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat , DecodeIntelDD);
  AddInstTable(InstTable, "F_FLOATING", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat , DecodeIntelDD);
  AddInstTable(InstTable, "DOUBLE"    , eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat , DecodeIntelDQ);
  AddInstTable(InstTable, "D_FLOATING", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat , DecodeIntelDQ);
  AddInstTable(InstTable, "G_FLOATING", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECGFormat, DecodeIntelDQ);
  AddInstTable(InstTable, "H_FLOATING", eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowFloat | eIntPseudoFlag_DECFormat , DecodeIntelDO);

  AddInstTable(InstTable, "REG", 0, CodeREG);
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
 * \fn     intern_symbol_vax(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for VAX
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_vax(char *p_arg, TempResult *p_result)
{
  Byte reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_vax;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_vax(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void make_code_vax(void)
{
  op_size = eSymbolSizeUnknown;

  /* Pseudo Instructions */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_vax(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_vax(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_vax(void)
 * \brief  deinitialize as target
 * ------------------------------------------------------------------------ */

static void switch_from_vax(void)
{
  deinit_fields();
  p_curr_cpu_props = NULL;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_vax(void *p_user)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_vax(void *p_user)
{
  const TFamilyDescr *p_descr;

  p_curr_cpu_props = (const cpu_props_t*)p_user;
  p_descr = FindFamilyByName("VAX");
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0x01;
  DivideChars = ",";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[UInt32].Max;

  MakeCode = make_code_vax;
  IsDef = is_def_vax;
  SwitchFrom = switch_from_vax;
  InternSymbol = intern_symbol_vax;
  DissectReg = dissect_reg_vax;
  multi_char_le = True;

  init_fields();

  if (!curr_access_mode_set)
    set_access_mode(3);
}

/*!------------------------------------------------------------------------
 * \fn     initpass_vax(void)
 * \brief  initializations at beginning of pass
 * ------------------------------------------------------------------------ */

static void initpass_vax(void)
{
  /* Flag to introduce & initialize symbol at first switch to target */

  curr_access_mode_set = False;
}

/*!------------------------------------------------------------------------
 * \fn     codevax_init(void)
 * \brief  register VAX target
 * ------------------------------------------------------------------------ */

static const cpu_props_t cpu_props[] =
{
  { "MicroVAX-I"  , e_cpu_cap_float_g | e_cpu_cap_string_mvax },
  { "MicroVAX-II" , e_cpu_cap_float_g },
  { "VAX-11/725"  , e_cpu_cap_all_no_vector },
  { "VAX-11/730"  , e_cpu_cap_all_no_vector },
  { "VAX-11/750"  , e_cpu_cap_all_no_vector }, /* g/h optional */
  { "VAX-11/780"  , e_cpu_cap_all_no_vector }, /* g/h optional */
  { "VAX-11/782"  , e_cpu_cap_all_no_vector }, /* g/h optional */
  { "VAX-11/785"  , e_cpu_cap_all_no_vector }, /* g/h optional */
  { "VAX-8200"    , e_cpu_cap_all_no_vector },
  { "VAX-8300"    , e_cpu_cap_all_no_vector },
  { "VAX-8500"    , e_cpu_cap_all_no_vector },
  { "VAX-8600"    , e_cpu_cap_all_no_vector },
  { "VAX-8650"    , e_cpu_cap_all_no_vector },
  { "VAX-8800"    , e_cpu_cap_all_no_vector },
};

void codevax_init(void)
{
  const cpu_props_t *p_prop;

  for (p_prop = cpu_props; p_prop < cpu_props + as_array_size(cpu_props); p_prop++)
    (void)AddCPUUserWithArgs(p_prop->name, switch_to_vax, (void*)p_prop, NULL, NULL);
  AddInitPassProc(initpass_vax);
}
