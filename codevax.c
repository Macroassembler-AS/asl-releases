/* codepdp11.c */
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
#include "decfloat.h"
#include "codepdp11.h"

typedef struct
{
  char name[15];
} cpu_props_t;

typedef struct
{
  Word code;
  tSymbolSize op_size;
} order_t;

typedef enum
{
  ModNone = -1,
  ModReg = 0,
  ModImm = 1,
  ModMem = 2
} adr_mode_t;

#define MModReg (1 << ModReg)
#define MModImm (1 << ModImm)
#define MModMem (1 << ModMem)

typedef struct
{
  adr_mode_t mode;
  unsigned count;
  Byte vals[20];
} adr_vals_t;

static const cpu_props_t *p_curr_cpu_props;
static tSymbolSize op_size;
static order_t *two_op_orders, *three_op_orders;

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
 * \brief  dissect register symbols - PDP-11 variant
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
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals_int(adr_vals_t *p_vals, LargeWord int_value, tSymbolSize size)
 * \brief  append integer value to encoded address value
 * \param  p_vals where to append
 * \param  int_value value to append
 * \param  size integer size
 * \return 
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

static int index_qualifier(const char *p_arg, int next_non_blank_pos, int split_pos)
{
  /* extra check for post increment: good enough? */

  int ret = ((next_non_blank_pos >= 4)
       && (p_arg[0] == '(')
       && (p_arg[next_non_blank_pos - 1] == ')')
       && (p_arg[next_non_blank_pos] == '+'))
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
    else
    {
      LargeInt value;
      IntType eval_int_type;
      Byte *p_specifier;

      if (index_reg)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return reset_adr_vals(p_result);
      }

      p_specifier = &p_result->vals[p_result->count]; p_result->count++;

      /* TODO: regard ranges for floating point */

      if (len_spec == 'S')
        eval_int_type = UInt6;
      else if (!len_spec || (len_spec == 'I'))
      {
        switch (op_size)
        {
          case eSymbolSize8Bit:
            eval_int_type = Int8;
            break;
          case eSymbolSize16Bit:
            eval_int_type = Int16;
            break;
          case eSymbolSize64Bit:
          case eSymbolSize128Bit:
#ifdef HAS64
            eval_int_type = Int64;
            break;
#endif
          case eSymbolSize32Bit:
            eval_int_type = Int32;
            break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &imm_arg);
            return reset_adr_vals(p_result);
        }
      }
      else 
      {
        WrStrErrorPos(ErrNum_UndefAttr, &len_spec_arg);
        return reset_adr_vals(p_result);
      }

      value = EvalStrIntExpressionWithResult(&imm_arg, eval_int_type, &eval_result);
      if (!eval_result.OK)
        return reset_adr_vals(p_result);

      if (!len_spec)
        len_spec = (RangeCheck(value, UInt6)) ? 'S' : 'I';

      if (len_spec == 'S')
        *p_specifier = value & 63;
      else
      {
        *p_specifier = 0x80 | REG_PC;
        append_adr_vals_int(p_result, (LargeWord)value, op_size);
      }
      return check_mode_mask(mode_mask, MModImm, p_arg, p_result);
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
      disp = EvalStrIntExpressionWithResult(&disp_arg, eval_int_type, &eval_result);
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

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    append_opcode(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_two_op(Word index)
 * \brief  handle instructions with two generic operands
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void decode_two_op(Word index)
{
  const order_t *p_order = &two_op_orders[index];
  adr_vals_t src_adr_vals, dest_adr_vals;

  op_size = p_order->op_size;
  if (ChkArgCnt(2, 2)
   && decode_adr(&ArgStr[1], &src_adr_vals, EProgCounter() + code_len(p_order->code), MModImm | MModMem | MModReg)
   && decode_adr(&ArgStr[2], &dest_adr_vals, EProgCounter() + code_len(p_order->code) + src_adr_vals.count, MModMem | MModReg))
  {
    append_opcode(p_order->code);
    append_adr_vals(&src_adr_vals);
    append_adr_vals(&dest_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_three_op(Word index)
 * \brief  handle instructions with three generic operands
 * \param  index index into instruction table
 * ------------------------------------------------------------------------ */

static void decode_three_op(Word index)
{
  const order_t *p_order = &three_op_orders[index];
  adr_vals_t src1_adr_vals, src2_adr_vals, dest_adr_vals;

  op_size = p_order->op_size;
  if (ChkArgCnt(3, 3)
   && decode_adr(&ArgStr[1], &src1_adr_vals, EProgCounter() + code_len(p_order->code), MModImm | MModMem | MModReg)
   && decode_adr(&ArgStr[2], &src2_adr_vals, EProgCounter() + code_len(p_order->code) + src1_adr_vals.count, MModImm | MModMem | MModReg)
   && decode_adr(&ArgStr[3], &dest_adr_vals, EProgCounter() + code_len(p_order->code) + src1_adr_vals.count + src2_adr_vals.count, MModMem | MModReg))
  {
    append_opcode(p_order->code);
    append_adr_vals(&src1_adr_vals);
    append_adr_vals(&src2_adr_vals);
    append_adr_vals(&dest_adr_vals);
  }
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     init_fields_wd16(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static void add_two_op(const char *p_name, tSymbolSize op_size, Word code)
{
  order_array_rsv_end(two_op_orders, order_t);
  two_op_orders[InstrZ].op_size = op_size;
  two_op_orders[InstrZ].code = code;
  AddInstTable(InstTable, p_name, InstrZ++, decode_two_op);
}

static void add_three_op(const char *p_name, tSymbolSize op_size, Word code)
{
  order_array_rsv_end(three_op_orders, order_t);
  three_op_orders[InstrZ].op_size = op_size;
  three_op_orders[InstrZ].code = code;
  AddInstTable(InstTable, p_name, InstrZ++, decode_three_op);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "HALT",  0x00   , decode_fixed);
  AddInstTable(InstTable, "NOP",   NOPCode, decode_fixed);

  InstrZ = 0;
  add_two_op("MOVB", eSymbolSize8Bit, 0x90);
  add_two_op("MOVW", eSymbolSize16Bit, 0xb0);
  add_two_op("MOVD", eSymbolSize32Bit, 0x70);
  add_two_op("MOVL", eSymbolSize64Bit, 0xd0);
  add_two_op("MOVO", eSymbolSize128Bit, 0x7dfd);
  add_two_op("ADDB2", eSymbolSize8Bit, 0x80);
  add_two_op("ADDW2", eSymbolSize16Bit, 0xa0);
  add_two_op("ADDL2", eSymbolSize32Bit, 0xc0);
  add_two_op("SUBB2", eSymbolSize8Bit, 0x82);
  add_two_op("SUBW2", eSymbolSize16Bit, 0xa2);
  add_two_op("SUBL2", eSymbolSize32Bit, 0xc2);
  add_two_op("MULB2", eSymbolSize8Bit, 0x84);
  add_two_op("MULW2", eSymbolSize16Bit, 0xa4);
  add_two_op("MULL2", eSymbolSize32Bit, 0xc4);
  add_two_op("DIVB2", eSymbolSize8Bit, 0x86);
  add_two_op("DIVW2", eSymbolSize16Bit, 0xa6);
  add_two_op("DIVL2", eSymbolSize32Bit, 0xc6);

  InstrZ = 0;
  add_three_op("ADDB3", eSymbolSize8Bit, 0x81);
  add_three_op("ADDW3", eSymbolSize16Bit, 0xa1);
  add_three_op("ADDL3", eSymbolSize32Bit, 0xc1);
  add_three_op("SUBB3", eSymbolSize8Bit, 0x83);
  add_three_op("SUBW3", eSymbolSize16Bit, 0xa3);
  add_three_op("SUBL3", eSymbolSize32Bit, 0xc3);
  add_three_op("MULB3", eSymbolSize8Bit, 0x85);
  add_three_op("MULW3", eSymbolSize16Bit, 0xa5);
  add_three_op("MULL3", eSymbolSize32Bit, 0xc5);
  add_three_op("DIVB3", eSymbolSize8Bit, 0x87);
  add_three_op("DIVW3", eSymbolSize16Bit, 0xa7);
  add_three_op("DIVL3", eSymbolSize32Bit, 0xc7);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  order_array_free(two_op_orders);
  order_array_free(three_op_orders);
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
  (void)p_arg; (void)p_result;

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
  CodeLen = 0;
  DontPrint = False;
  op_size = eSymbolSizeUnknown;

  /* to be ignored */

  if (Memo("")) return;

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
#if 0
  DissectReg = dissect_reg_vax;
#endif
  multi_char_le = True;

  init_fields();
}

/*!------------------------------------------------------------------------
 * \fn     codevax_init(void)
 * \brief  register VAX target
 * ------------------------------------------------------------------------ */

static const cpu_props_t cpu_props[] =
{
  {      "VAX-11/750"  },
  {      "VAX-11/780"  },
};

void codevax_init(void)
{
  const cpu_props_t *p_prop;

  for (p_prop = cpu_props; p_prop < cpu_props + as_array_size(cpu_props); p_prop++)
    (void)AddCPUUserWithArgs(p_prop->name, switch_to_vax, (void*)p_prop, NULL, NULL);
}
