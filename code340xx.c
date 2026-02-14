/* code340xx.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* ASL                                                                       */
/*                                                                           */
/* Codegenerator TMS340xx                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmitree.h"
#include "asmlabel.h"
#include "asmcode.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "onoff_common.h"
#include "headids.h"

#include "code340xx.h"

#define REG_SP 15 /* also 31 in bank B */

/* ------------------------------------------------------------------------ */

static CPUVar cpu_34010, cpu_34020;

static Boolean coproc_set;
static Word coproc;
static const char coproc_name[] = "COPROC";

/* ------------------------------------------------------------------------ */
/* Register Handling */

static const char reg_name_sp[3] = "SP";
static const char reg_names_b[][8] =
{
  "SADDR",  /* B0 */
  "SPTCH",  /* B1 */
  "DADDR",  /* B2 */
  "DPTCH",  /* B3 */
  "OFFSET", /* B4 */
  "WSTART", /* B5 */
  "WEND",   /* B6 */
  "DYDX",   /* B7 */
  "COLOR0", /* B8 */
  "COLOR1", /* B9 */
  "COUNT",  /* B10 */
  "INC1",   /* B11 */
  "INC2",   /* B12 */
  "PATTRN"  /* B13 */
};

/*!------------------------------------------------------------------------
 * \fn     decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
 * \brief  parse CPU register name
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \return True if argument is CPU register
 * ------------------------------------------------------------------------ */

static Boolean decode_reg_core(const char *p_arg, Word *p_result, tSymbolSize *p_size)
{
  unsigned z;
  int l;
  char bank_ch;

  if (!as_strcasecmp(p_arg, reg_name_sp))
  {
    *p_result = REG_SP;
    if (p_size) *p_size = eSymbolSize32Bit;
    return True;
  }
  if (default_regsyms)
  {
    for (z = 0; z < as_array_size(reg_names_b); z++)
      if (!as_strcasecmp(p_arg, reg_names_b[z]))
      {
        *p_result = REGSYM_FLAG_ALIAS | 16 | z;
        if (p_size) *p_size = eSymbolSize32Bit;
        return True;
      }
  }

  l = strlen(p_arg);
  if ((l < 2) || (l > 3))
    return False;
  bank_ch = as_toupper(*p_arg);
  if ((bank_ch != 'A') && (bank_ch != 'B'))
    return False;
  *p_result = 0;
  for (p_arg++; *p_arg; p_arg++)
  {
    if (!as_isdigit(*p_arg))
      return False;
    *p_result = (*p_result * 10) + (*p_arg - '0');
  }
  if (*p_result > 14)
    return False;
  *p_result |= ((bank_ch - 'A') << 4);
  if (p_size) *p_size = eSymbolSize32Bit;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_340xx(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  translate register # back to textual form
 * \param  p_dest destination text buffer
 * \param  dest_size capacity of buffer
 * \param  value register #
 * \param  inp_size register's operand size
 * ------------------------------------------------------------------------ */

static void dissect_reg_340xx(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  if (inp_size == eSymbolSize32Bit)
  {
    unsigned num = value & 15, bank = value & 16;

    if ((value & REGSYM_FLAG_ALIAS) && bank && (num < as_array_size(reg_names_b)))
      strmaxcpy(p_dest, reg_names_b[num], dest_size);
    else if (num == REG_SP)
      strmaxcpy(p_dest, reg_name_sp, dest_size);
    else
      as_snprintf(p_dest, dest_size, "%c%u", !!bank + 'A', num);
  }
  else
    as_snprintf(p_dest, dest_size, "%d-%u", (unsigned)inp_size, (unsigned)value);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
 * \brief  decode register argument
 * \param  p_arg source argument
 * \param  p_result returns register #
 * \param  p_size actual operand size of register
 * \param  req_size requested register operand size
 * \param  must_be_reg argument must be  a register
 * \return OK/No/Abort
 * ------------------------------------------------------------------------ */

static tErrorNum chk_reg_size(tSymbolSize req_size, tSymbolSize act_size)
{
  /* 'any size' or exact match */
  if ((act_size == eSymbolSizeUnknown)
   || (req_size == eSymbolSizeUnknown)
   || (req_size == act_size))
    return ErrNum_None;
  /* integer requested, but got something else: */
  else if (req_size == eSymbolSize32Bit)
    return ErrNum_IntButFloat;
  /* float requested */
  else
    return ((act_size == eSymbolSizeFloat32Bit) || (act_size == eSymbolSizeFloat64Bit) || (act_size == eSymbolSizeFloat96Bit) || (act_size == eSymbolSize80Bit)) ? ErrNum_None : ErrNum_FloatButInt;
}

static tRegEvalResult decode_reg(const tStrComp *p_arg, Word *p_result, tSymbolSize *p_size, tSymbolSize req_size, Boolean must_be_reg)
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

  if (reg_eval_result == eIsReg)
  {
    tErrorNum error_num = chk_reg_size(req_size, eval_result.DataSize);

    if (error_num)
    {
      WrStrErrorPos(error_num, p_arg);
      reg_eval_result = must_be_reg ? eIsNoReg : eRegAbort;
    }
  }

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  if (p_size) *p_size = eval_result.DataSize;
  return reg_eval_result;
}

/* ------------------------------------------------------------------------ */
/* Code Helpers */

/*!------------------------------------------------------------------------
 * \fn     code_aligned(LongWord address)
 * \brief  is address properly aligned for code disposition?
 * \param  address address to check
 * \return True if aligned
 * ------------------------------------------------------------------------ */

static Boolean code_aligned(LongWord address)
{
  return !(address & 15);
}

/*!------------------------------------------------------------------------
 * \fn     decode_code_dest(const tStrComp *p_arg, LongWord *p_dest, tEvalResult *p_eval_result)
 * \brief  parse source argument containing branch/call dest address
 * \param  p_arg source argument
 * \param  p_dest result buffer
 * \return True if parsing OK
 * ------------------------------------------------------------------------ */

static Boolean decode_code_dest(const tStrComp *p_arg, LongWord *p_dest, tEvalResult *p_eval_result)
{
  *p_dest = EvalStrIntExpressionWithResult(p_arg, (MomCPU == cpu_34020) ? UInt32 : UInt30, p_eval_result);
  if (p_eval_result->OK)
  {
    if (!mFirstPassUnknownOrQuestionable(p_eval_result->Flags) && !code_aligned(*p_dest))
    {
      WrStrErrorPos(ErrNum_AddrMustBeAligned, p_arg);
      p_eval_result->OK = False;
    }
  }
  return p_eval_result->OK;
}

/*!------------------------------------------------------------------------
 * \fn     decode_field_index(int arg_index, Word *p_dest)
 * \brief  decode optional field index argument
 * \param  arg_index index of field argument if present
 * \param  p_dest dest buffer
 * \return True if decoding OK
 * ------------------------------------------------------------------------ */

static Boolean decode_field_index(int arg_index, Word *p_dest)
{
  if (arg_index > ArgCnt)
  {
    *p_dest = 0;
    return True;
  }
  else
  {
    Boolean ok;

    *p_dest = EvalStrIntExpression(&ArgStr[arg_index], UInt1, &ok);
    return ok;
  }
}

/*!------------------------------------------------------------------------
 * \fn     is_sext_16_32(LongWord value)
 * \brief  is value representable as 16 bit with sign extension?
 * \param  value value to test
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_sext_16_32(LongWord value)
{
  value &= 0xffff8000ul;
  return (value == 0xffff8000ul) || !value;
}

/*!------------------------------------------------------------------------
 * \fn     append_opcode(Word code)
 * \brief  append machine code word
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void append_opcode(Word code)
{
  set_basmcode_bit_field_le(CodeLen, code, 16);
  CodeLen += 16;
  ActListGran = 2;
  act_list_gran_bits_unused = 0;
}

/* ------------------------------------------------------------------------ */
/* Address Decoding */

typedef enum
{
  e_mode_none = -1,
  e_mode_reg = 0,
  e_mode_ireg = 1,
  e_mode_ireg_offs = 2,
  e_mode_abs = 3,
  e_mode_ireg_inc = 4,
  e_mode_ireg_dec = 5
} adr_mode_t;

typedef struct
{
  adr_mode_t mode;
  Word reg;
  int val_cnt;
  Word vals[2];
} adr_vals_t;

static adr_mode_t reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_mode_none;
  p_vals->reg = 0;
  p_vals->val_cnt = 0;
  return p_vals->mode;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  int z;

  for (z = 0; z < p_vals->val_cnt; z++)
    append_opcode(p_vals->vals[z]);
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals)
 * \brief  parse address expression
 * \param  p_arg source argument
 * \param  p_vals parse result
 * \return deduced address mode
 * ------------------------------------------------------------------------ */

static Boolean is_pre_decrement(const tStrComp *p_arg, Word *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (strncmp(p_arg->str.p_str, "*-", 2)
    && strncmp(p_arg->str.p_str, "-*", 2)))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 2, arg_len - 2);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, NULL, eSymbolSize32Bit, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static Boolean is_post_increment(const tStrComp *p_arg, Word *p_result, tRegEvalResult *p_reg_eval_result)
{
  String reg;
  tStrComp reg_comp;
  size_t arg_len = strlen(p_arg->str.p_str);

  if ((arg_len < 4)
   || (p_arg->str.p_str[0] != '*')
   || (p_arg->str.p_str[arg_len - 1] != '+'))
    return False;
  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 1, arg_len - 2);
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  *p_reg_eval_result = decode_reg(&reg_comp, p_result, NULL, eSymbolSize32Bit, False);
  return (*p_reg_eval_result != eIsNoReg);
}

static adr_mode_t decode_adr(const tStrComp *p_arg, adr_vals_t *p_vals)
{
  tRegEvalResult reg_eval_result;

  reset_adr_vals(p_vals);

  if (*p_arg->str.p_str == '@')
  {
    Boolean ok;
    LongWord address = EvalStrIntExpressionOffs(p_arg, 1, UInt32, &ok);
    if (ok)
    {
      p_vals->vals[p_vals->val_cnt++] = address & 0xffffu;
      p_vals->vals[p_vals->val_cnt++] = (address >> 16) & 0xffffu;
      p_vals->mode = e_mode_abs;
    }
    return p_vals->mode;
  }

  if (is_pre_decrement(p_arg, &p_vals->reg, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return reset_adr_vals(p_vals);
    return (p_vals->mode = e_mode_ireg_dec);
  }

  if (is_post_increment(p_arg, &p_vals->reg, &reg_eval_result))
  {
    if (eRegAbort == reg_eval_result)
      return reset_adr_vals(p_vals);
    return (p_vals->mode = e_mode_ireg_inc);
  }

  if (*p_arg->str.p_str == '*')
  {
    tStrComp indir_arg;
    int split_pos, arg_len;

    StrCompRefRight(&indir_arg, p_arg, 1);
    KillPrefBlanksStrCompRef(&indir_arg);
    split_pos = FindDispBaseSplitWithQualifier(indir_arg.str.p_str, &arg_len, NULL, "()");
    if (split_pos >= 0)
    {
      Boolean ok;
      tStrComp disp_arg;

      StrCompSplitRef(&indir_arg, &disp_arg, &indir_arg, indir_arg.str.p_str + split_pos);
      StrCompShorten(&disp_arg, 1);
      KillPrefBlanksStrCompRef(&disp_arg);
      KillPostBlanksStrComp(&disp_arg);
      KillPostBlanksStrComp(&indir_arg);

      p_vals->vals[p_vals->val_cnt++] = EvalStrIntExpression(&disp_arg, SInt16, &ok);
      if (!ok)
        return reset_adr_vals(p_vals);
      p_vals->mode = e_mode_ireg_offs;
    }
    else
      p_vals->mode = e_mode_ireg;
 
    reg_eval_result = decode_reg(&indir_arg, &p_vals->reg, NULL, eSymbolSize32Bit, True);
    if (reg_eval_result != eIsReg)
      reset_adr_vals(p_vals);
    return p_vals->mode;
  }
  else
  {
    reg_eval_result = decode_reg(p_arg, &p_vals->reg, NULL, eSymbolSize32Bit, True);
    p_vals->mode = (reg_eval_result == eIsReg) ? e_mode_reg : e_mode_none;
    return p_vals->mode;
  }
}

/*!------------------------------------------------------------------------
 * \fn     set_coproc(unsigned new_coproc)
 * \brief  set default coprocessor id incl. readable symbol
 * \param  new_coproc id to set (0...7)
 * ------------------------------------------------------------------------ */

static void set_coproc(unsigned new_coproc)
{
  tStrComp tmp_comp;

  coproc_set = True;
  coproc = new_coproc;
  PushLocHandle(-1);
  StrCompMkTemp(&tmp_comp, (char*)coproc_name, 0);
  EnterIntSymbol(&tmp_comp, coproc, SegNone, True);
  PopLocHandle();
}

/* ------------------------------------------------------------------------ */
/* Instruction Parsers */

#define CODE_FLAG_020    (1 << 0)            /* All instructions */
#define CODE_FLAG_DUP_ARG (1 << 1)           /* one_reg */
#define CODE_FLAG_DEST_PAIR (1 << 1)         /* two_reg */
#define CODE_FLAG_INSERT_COMPLEMENT (1 << 1) /* reg_imm */
#define CODE_FLAG_HAS_SHORT_FORM (1 << 2)    /* reg_imm */
#define CODE_FLAG_ALL (7 << 0)

static Boolean check_020(Word code)
{
  return (code & CODE_FLAG_020)
       ? ChkMinCPU(cpu_34020)
       : True;
}

/*!------------------------------------------------------------------------
 * \fn     pc_word_dist(LongWord dest, Word instr_offs)
 * \brief  compute relative branch distance in words
 * \param  dest destination (bit) address
 * \param  instr_offs instruction (bit) length
 * \return distance in words
 * ------------------------------------------------------------------------ */

static LongInt pc_word_dist(LongWord dest, Word instr_offs)
{
  LongInt dist = (LongInt)dest - (EProgCounter() + instr_offs);
  return dist / 16;
}

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word code)
 * \brief  handle instructions without arguments
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    append_opcode(code);
}

static void decode_fixed_020(Word code)
{
  if (ChkArgCnt(0, 0) && ChkMinCPU(cpu_34020))
    append_opcode(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_one_reg(Word code)
 * \brief  handle instructions with one register as argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_one_reg(Word code)
{
  Word reg;

  if (!ChkArgCnt(1, 1)
   || !check_020(code))
    return;
  switch (decode_reg(&ArgStr[1], &reg, NULL, eSymbolSize32Bit, True))
  {
    case eIsReg:
    {
      /* CLR is actually XOR Rd,Rd */
      append_opcode((code & ~CODE_FLAG_ALL)
                  | reg
                  | ((code & CODE_FLAG_DUP_ARG) ? ((reg & 0x0f) << 5) : 0));
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_two_reg(Word code)
 * \brief  handle instructions with two registers as argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static Boolean decode_two_reg_core(Word code, Word Rs, Word Rd)
{
  if (((Rs ^ Rd) & 16) && (Rs != REG_SP) && (Rd != REG_SP))
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  else
  {
    append_opcode((code & ~CODE_FLAG_ALL) | ((Rs & 0x0f) << 5) | ((Rs | Rd) & 0x10 ) | (Rd & 0x0f));
    if ((code & CODE_FLAG_DEST_PAIR) && ((Rd & 15) == REG_SP - 1))
      WrStrErrorPos(ErrNum_WillOverwriteSP, &ArgStr[2]);
    return True;
  }
}

static void decode_two_reg(Word code)
{
  Word Rs, Rd;

  if (ChkArgCnt(2, 2)
   && check_020(code)
   && (eIsReg == decode_reg(&ArgStr[1], &Rs, NULL, eSymbolSize32Bit, True))
   && (eIsReg == decode_reg(&ArgStr[2], &Rd, NULL, eSymbolSize32Bit, True)))
    decode_two_reg_core(code, Rs, Rd);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_imm(Word code)
 * \brief  handle instructions with a 16/32 bit immediate and a register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_imm(Word code)
{
  tSymbolSize op_size;
  Word Rd, bare_code;
  LongWord imm_val;
  tEvalResult eval_result;

  if (!ChkArgCnt(2, 3)
   || !check_020(code)
   || (eIsReg != decode_reg(&ArgStr[2], &Rd, NULL, eSymbolSize32Bit, True)))
    return;
  if (3 == ArgCnt)
  {
    if (!as_strcasecmp(ArgStr[3].str.p_str, "L"))
      op_size = eSymbolSize32Bit;
    else if ((code & CODE_FLAG_HAS_SHORT_FORM) && !as_strcasecmp(ArgStr[3].str.p_str, "W"))
      op_size = eSymbolSize16Bit;
    else
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[3]);
      return;
    }
  }
  else
    op_size = (code & CODE_FLAG_HAS_SHORT_FORM) ? eSymbolSizeUnknown : eSymbolSize32Bit;

  imm_val = EvalStrIntExpressionWithResult(&ArgStr[1], Int32, &eval_result);
  if (!eval_result.OK)
    return;
  switch (op_size)
  {
    case eSymbolSizeUnknown:
      op_size = is_sext_16_32(imm_val) ? eSymbolSize16Bit : eSymbolSize32Bit;
      break;
    case eSymbolSize16Bit:
      if (!is_sext_16_32(imm_val))
        WrStrErrorPos(ErrNum_WOverRange, &ArgStr[1]);
      break;
    default:
      break;
  }
  if (code & CODE_FLAG_INSERT_COMPLEMENT)
    imm_val = ~imm_val;

  bare_code = code & ~CODE_FLAG_ALL;
  if (op_size == eSymbolSize32Bit)
  {
    if (bare_code == 0x0be0)
      append_opcode(0x0d00 | Rd); /* SUB */
    else
      append_opcode(bare_code | ((code & CODE_FLAG_HAS_SHORT_FORM) ? 0x20 : 0) | Rd);
  }
  else
    append_opcode(bare_code | Rd);
  append_opcode(imm_val & 0xffffu);
  if (op_size == eSymbolSize32Bit)
    append_opcode((imm_val >> 16) & 0xffffu);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_const(Word code)
 * \brief  handle instructions with a 5 bit constant and a register argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_const(Word code)
{
  Word Rd, K;
  tEvalResult eval_result;

  if (!ChkArgCnt(2, 2)
   || !check_020(code)
   || (eIsReg != decode_reg(&ArgStr[2], &Rd, NULL, eSymbolSize32Bit, True)))
    return;

  K = EvalStrIntExpressionWithResult(&ArgStr[1], UInt6, &eval_result);
  if (!eval_result.OK)
    return;
  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags))
  {
    if (!ChkRange(K, 1, 32))
      return;
  }
  append_opcode((code & ~CODE_FLAG_ALL) | ((K & 31) << 5) | Rd);
}

/*!------------------------------------------------------------------------
 * \fn     decode_shift(Word code)
 * \brief  handle shift & rotate instructions
 * \param  code machine codes
 * ------------------------------------------------------------------------ */

static void decode_shift(Word code)
{
  Word Rd, Rs;

  if (!ChkArgCnt(2, 2)
   || (eIsReg != decode_reg(&ArgStr[2], &Rd, NULL, eSymbolSize32Bit, True)))
    return;

  switch (decode_reg(&ArgStr[1], &Rs, NULL, eSymbolSize32Bit, False))
  {
    case eIsReg:
      decode_two_reg_core((code & 0xff) << 8, Rs, Rd);
      break;
    case eIsNoReg:
    {
      Boolean OK;
      Word K = EvalStrIntExpression(&ArgStr[1], UInt5, &OK);

      if (OK)
      {
        if (code & 0x0100)
          K = (32 - K) & 31;
        append_opcode((code & 0xfe00u) | (K << 5) | Rd);
      }
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jacc(Word code)
 * \brief  handle JAcc instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jacc(Word code)
{
  tEvalResult eval_result;
  LongWord dest;

  if (ChkArgCnt(1, 1) && decode_code_dest(&ArgStr[1], &dest, &eval_result))
  {
    append_opcode(code);
    append_opcode(dest & 0xffff);
    append_opcode((dest >> 16) & 0xffff);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_jrcc(Word code)
 * \brief  handle JRcc instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jrcc(Word code)
{
  tEvalResult eval_result;
  LongWord dest;

  if (ChkArgCnt(1, 1) && decode_code_dest(&ArgStr[1], &dest, &eval_result))
  {
    LongInt dist = pc_word_dist(dest, 16);

    if (!dist)
    {
      append_opcode(NOPCode);
      WrStrErrorPos(ErrNum_ReplacedByNOP, &ArgStr[1]);
    }
    else if ((dist >= -127) && (dist <= 127))
      append_opcode(code | (dist & 0xff));
    else
    {
      dist--;
      if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && ((dist < -32768) || (dist > 32767))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        append_opcode(code);
        append_opcode(dist & 0xffff);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_callr(Word code)
 * \brief  handle CALLR instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_callr(Word code)
{
  tEvalResult eval_result;
  LongWord dest;

  if (ChkArgCnt(1, 1) && decode_code_dest(&ArgStr[1], &dest, &eval_result))
  {
    LongInt dist = pc_word_dist(dest, 32);

    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && ((dist < -32768) || (dist > 32767))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
    else
    {
      append_opcode(code);
      append_opcode(dist & 0xffff);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_dsjs(Word code)
 * \brief  handle DSJS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void encode_dsjs_core(Word code, Word Rd, LongInt dist)
{
  if (dist < 0)
  {
    dist = -dist;
    code |= 0x0400;
  }
  append_opcode(code | (dist << 5) | Rd);
}

static void decode_dsjs(Word code)
{
  Word Rd;
  tEvalResult eval_result;
  LongWord dest;

  if (ChkArgCnt(2, 2)
   && (eIsReg == decode_reg(&ArgStr[1], &Rd, NULL, eSymbolSize32Bit, True))
   && decode_code_dest(&ArgStr[2], &dest, &eval_result))
  {
    LongInt dist = pc_word_dist(dest, 16);

    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && ((dist < -31) || (dist > 31))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
    else
      encode_dsjs_core(code, Rd, dist);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_dsj(Word code)
 * \brief  handle DSJxx instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_dsj(Word code)
{
  Word Rd;
  tEvalResult eval_result;
  LongWord dest;

  if (ChkArgCnt(2, 2)
   && (eIsReg == decode_reg(&ArgStr[1], &Rd, NULL, eSymbolSize32Bit, True))
   && decode_code_dest(&ArgStr[2], &dest, &eval_result))
  {
    LongInt dist = pc_word_dist(dest, 32);

    if ((dist >= -32) && (dist <= 30) && (code & 1))
      encode_dsjs_core(0x3800, Rd, dist + 1);
    else if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && ((dist < -32768) || (dist > 32767))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
    else
    {
      append_opcode((code & ~1) | Rd);
      append_opcode(dist & 0xffff);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_exgf(Word code)
 * \brief  handle EXGF instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_exgf(Word code)
{
  Word Rd, F;

  if (ChkArgCnt(1, 2)
   && (eIsReg == decode_reg(&ArgStr[1], &Rd, NULL, eSymbolSize32Bit, True))
   && decode_field_index(2, &F))
   append_opcode(code | (F << 9) | Rd);
}

/*!------------------------------------------------------------------------
 * \fn     decode_fill(Word code)
 * \brief  handle FILL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fill(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "L"))
      append_opcode(code | 0x0000);
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "XY"))
      append_opcode(code | 0x0020);
    else
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_pixblt(Word code)
 * \brief  handle PIXBLT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_pixblt(Word code)
{
  switch (ArgCnt)
  {
    case 2:
      if (!as_strcasecmp(ArgStr[1].str.p_str, "B"))
        code |= 0x80;
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "XY"))
        code |= 0x40;
      else if (as_strcasecmp(ArgStr[1].str.p_str, "L"))
      {
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
        return;
      }
      if (!as_strcasecmp(ArgStr[2].str.p_str, "XY"))
        code |= 0x20;
      else if (as_strcasecmp(ArgStr[2].str.p_str, "L"))
      {
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
        return;
      }
      append_opcode(code);
      break;
    case 3:
      if (!ChkMinCPU(cpu_34020));
      else if (as_strcasecmp(ArgStr[1].str.p_str, "L"))
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
      else if (as_strcasecmp(ArgStr[2].str.p_str, "M"))
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
      else if (as_strcasecmp(ArgStr[3].str.p_str, "L"))
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[3]);
      else
        append_opcode(0x0e17);
      break;
    default:
      (void)ChkArgCnt(2, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_pfill(Word code)
 * \brief  handle TFILL/PFILL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_pfill(Word code)
{
  if (ChkArgCnt(0, 1))
  {
    if (!ArgCnt || !as_strcasecmp(ArgStr[1].str.p_str, "XY"))
      append_opcode(code);
    else
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_line(Word code)
 * \brief  handle (F)LINE instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_line(Word code)
{
  if (ChkArgCnt(1, 1)
   && ((code & 0x100) || ChkMinCPU(cpu_34020)))
  {
    Boolean ok;
    Word Z = EvalStrIntExpression(&ArgStr[1], UInt1, &ok);

    if (ok)
      append_opcode(code | (Z << 7));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_blmove(Word code)
 * \brief  handle BLMOVE instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_blmove(Word code)
{
  if (ChkArgCnt(2, 2) && ChkMinCPU(cpu_34020))
  {
    Boolean ok;

    code |= EvalStrIntExpression(&ArgStr[1], UInt1, &ok) << 1;
    if (!ok) return;
    code |= EvalStrIntExpression(&ArgStr[2], UInt1, &ok) << 0;
    if (!ok) return;
    append_opcode(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_vblt(Word code)
 * \brief  handle VBLT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_vblt(Word code)
{
  if (ChkArgCnt(2, 2) && ChkMinCPU(cpu_34020))
  {
    if (as_strcasecmp(ArgStr[1].str.p_str, "B"))
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
    else if (as_strcasecmp(ArgStr[2].str.p_str, "L"))
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[2]);
    else
      append_opcode(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_vfill(Word code)
 * \brief  handle VFILL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_vfill(Word code)
{
  if (ChkArgCnt(1, 1) && ChkMinCPU(cpu_34020))
  {
    if (as_strcasecmp(ArgStr[1].str.p_str, "L"))
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
    else
      append_opcode(code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_mmxm(Word code)
 * \brief  handle MMTM/MMFM instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mmxm(Word code)
{
  Word Rp, reg_list = 0x0000;
  Boolean bank_fixed, is_mmtm = !(code & 0x0020);
  int z;

  if (!ChkArgCnt(2, ArgCntMax)
   || (eIsReg != decode_reg(&ArgStr[1], &Rp, NULL, eSymbolSize32Bit, True)))
    return;
  bank_fixed = Rp != REG_SP;

  for (z = 2; z <= ArgCnt; z++)
  {
    Word this_reg;

    if (eIsReg != decode_reg(&ArgStr[z], &this_reg, NULL, eSymbolSize32Bit, True))
      return;
    if (this_reg == Rp) WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[z]);
    if (bank_fixed && (this_reg != REG_SP) && ((Rp ^ this_reg) & 16))
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[z]);
      return;
    }
    if ((this_reg != REG_SP) && !bank_fixed)
    {
      Rp = (Rp & 15) | (this_reg & 16);
      bank_fixed = True;
    }
    this_reg &= 15;
    reg_list |= 1 << (is_mmtm ? (15 - this_reg) : this_reg);
  }
  append_opcode(code | Rp);
  append_opcode(reg_list);
}

/*!------------------------------------------------------------------------
 * \fn     decode_movb(Word code)
 * \brief  handle MOVB instruction
 * ------------------------------------------------------------------------ */

static void decode_movb(Word code)
{
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(code);

  if (!ChkArgCnt(2, 2))
    return;

  switch (decode_adr(&ArgStr[1], &src_adr_vals))
  {
    case e_mode_reg:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_ireg:
          if (!decode_two_reg_core(0x8c00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_offs:
          if (!decode_two_reg_core(0xac00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_abs:
          append_opcode(0x05e0 | src_adr_vals.reg);
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0x8e00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_offs: /* MOVB *Rs,*Rd(offs) -> MOVB *Rs(0),*Rd(offs) */
          src_adr_vals.mode = e_mode_ireg_offs;
          src_adr_vals.vals[src_adr_vals.val_cnt++] = 0;
          if (!decode_two_reg_core(0xbc00, src_adr_vals.reg, dest_adr_vals.reg))
            return;   
          break;
        case e_mode_ireg:
          if (!decode_two_reg_core(0x9c00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg_offs:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0xae00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg: /* MOVB *Rs(offs),*Rd -> MOVB *Rs(offs),*Rd(0) */
          dest_adr_vals.mode = e_mode_ireg_offs;
          dest_adr_vals.vals[dest_adr_vals.val_cnt++] = 0;
          /* FALL-THRU */
        case e_mode_ireg_offs:
          if (!decode_two_reg_core(0xbc00, src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_abs:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          append_opcode(0x07e0 | dest_adr_vals.reg);
          break;
        case e_mode_abs:
          append_opcode(0x0340);
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_none:
      return;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      return;
  }
  append_adr_vals(&src_adr_vals);
  append_adr_vals(&dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     decode_move(Word code)
 * \brief  handle MOVE instruction
 * ------------------------------------------------------------------------ */

static void decode_move(Word code)
{
  Word F;
  adr_vals_t src_adr_vals, dest_adr_vals;

  UNUSED(code);

  if (!ChkArgCnt(2, 3) || !decode_field_index(3, &F))
    return;

  switch (decode_adr(&ArgStr[1], &src_adr_vals))
  {
    case e_mode_reg:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (ArgCnt == 3) /* no F arg for MOVE Rs,Rd */
          {
            WrError(ErrNum_WrongArgCnt);
            return;
          }
          /* src & dest in different file -> M=1, R=file of Rs! */
          if ((src_adr_vals.reg & 16) != (dest_adr_vals.reg & 16)) 
          {
            dest_adr_vals.reg = (dest_adr_vals.reg & 15) | (src_adr_vals.reg & 16);
            src_adr_vals.reg |= 16;
          }
          /* src & dest in same file -> M=0 */
          else
            src_adr_vals.reg &= ~16;
          append_opcode(0x4c00 | (src_adr_vals.reg << 5) | dest_adr_vals.reg);
          break;
        case e_mode_ireg:
          if (!decode_two_reg_core(0x8000 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_inc:
          if (!decode_two_reg_core(0x9000 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_dec:
          if (!decode_two_reg_core(0xa000 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_offs:
          if (!decode_two_reg_core(0xb000 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_abs:
          append_opcode(0x0580 | (F << 9) | src_adr_vals.reg);
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0x8400 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg:
          if (!decode_two_reg_core(0x8800 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_offs: /* encode *Rs as *Rs(0) */
          src_adr_vals.vals[src_adr_vals.val_cnt++] = 0;
          src_adr_vals.mode = e_mode_ireg_offs;
          if (!decode_two_reg_core(0xb800 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg_inc:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0x9400 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_inc:
          if (!decode_two_reg_core(0x9800 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg_dec:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0xa400 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_dec:
          if (!decode_two_reg_core(0xa800 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_ireg_offs:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          if (!decode_two_reg_core(0xb400 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg_inc:
          if (!decode_two_reg_core(0xd000 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_ireg: /* encode *Rd as *Rd(0) */
          dest_adr_vals.vals[dest_adr_vals.val_cnt++] = 0;
          dest_adr_vals.mode = e_mode_ireg_offs;
          /* FALL-THRU */
        case e_mode_ireg_offs:
          if (!decode_two_reg_core(0xb800 | (F << 9), src_adr_vals.reg, dest_adr_vals.reg))
            return;
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_abs:
      switch (decode_adr(&ArgStr[2], &dest_adr_vals))
      {
        case e_mode_reg:
          append_opcode(0x05a0 | (F << 9) | dest_adr_vals.reg);
          break;
        case e_mode_ireg_inc:
          append_opcode(0xd400 | (F << 9) | dest_adr_vals.reg);
          break;
        case e_mode_abs:
          append_opcode(0x05c0 | (F << 9));
          break;
        case e_mode_none:
          return;
        default:
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          return;
      }
      break;
    case e_mode_none:
      return;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      return;
  }
  append_adr_vals(&src_adr_vals);
  append_adr_vals(&dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     decode_pixt(Word code)
 * \brief  handle PIXT instruction
 * ------------------------------------------------------------------------ */

static void decode_pixt(Word code)
{
  adr_vals_t adr_vals[2];
  Boolean is_xy[2];
  int z;

  if (!ChkArgCnt(2, 2))
    return;
  for (z = 0; z < 2; z++)
  {
    tStrComp *p_arg = &ArgStr[z + 1];
    char *p_str = ArgStr[z + 1].str.p_str;
    int l = strlen(p_str);

    is_xy[z] = ((l > 3) && !as_strcasecmp(p_str + l - 3, ".XY"));
    if (is_xy[z])
      StrCompShorten(p_arg, 3);
    switch (decode_adr(p_arg, &adr_vals[z]))
    {
      case e_mode_ireg:
        break;
      case e_mode_none:
        return;
      case e_mode_reg:
        if (!is_xy[z])
          break;        /* FALL-THRU */
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
        return;
    }
  }

  switch (adr_vals[0].mode)
  {
    case e_mode_reg:
      switch (adr_vals[1].mode)
      {
        case e_mode_reg:
          WrError(ErrNum_InvAddrMode);
          return;
        case e_mode_ireg:
          code |= is_xy[1] ? 0x0000 : 0x0800;
          break;
        default:
          return;
      }
      break;
    case e_mode_ireg:
      switch (adr_vals[1].mode)
      {
        case e_mode_reg:
          code |= is_xy[0] ? 0x0200 : 0x0a00;
          break;
        case e_mode_ireg:
          if (is_xy[0] && is_xy[1])
            code |= 0x0400;
          else if (!is_xy[0] && !is_xy[1])
            code |= 0x0c00;
          else
          {
            WrError(ErrNum_InvAddrMode);
            return;
          }
          break;
        default:
          return;
      }
      break;
    default:
      return;
  }

  decode_two_reg_core(code, adr_vals[0].reg, adr_vals[1].reg);
}

/*!------------------------------------------------------------------------
 * \fn     decode_setf(Word code)
 * \brief  handle SETF instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_setf(Word code)
{
  tEvalResult eval_result;
  Word F;
  
  if (!ChkArgCnt(2, 3) || !decode_field_index(3, &F))
    return;
  code |= F << 9;

  F = EvalStrIntExpressionWithResult(&ArgStr[2], UInt1, &eval_result);
  if (!eval_result.OK)
    return;
  code |= F << 5;

  F = EvalStrIntExpressionWithResult(&ArgStr[1], UInt6, &eval_result);
  if (!eval_result.OK)
    return;
  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags))
  {
    if (!ChkRange(F, 1, 32))
      return;
  }
  code |= F & 31;
  append_opcode(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_rets(Word code)
 * \brief  handle RETS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_rets(Word code)
{
  switch (ArgCnt)
  {
    case 0:
      break;
    case 1:
    {
      Boolean ok;
      code |= EvalStrIntExpression(&ArgStr[1], UInt5, &ok);
      if (!ok)
        return;
      break;
    }
    default:
      (void)ChkArgCnt(0,1);
      return;
  }
  append_opcode(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_trap(Word code)
 * \brief  handle TRAP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_trap(Word code)
{
  Boolean ok;

  if (!ChkArgCnt(1, 1))
    return;
  code |= EvalStrIntExpression(&ArgStr[1], UInt5, &ok);
  if (ok)
    append_opcode(code);  
}

/*!------------------------------------------------------------------------
 * \fn     decode_trapl(Word code)
 * \brief  handle TRAPL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_trapl(Word code)
{
  Boolean ok;
  Word arg;

  if (!ChkArgCnt(1, 1)
    || !ChkMinCPU(cpu_34020))
    return;
  arg = EvalStrIntExpression(&ArgStr[1], SInt16, &ok);
  if (ok)
  {
    append_opcode(code);
    append_opcode(arg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ext(Word code)
 * \brief  handle SEXT/ZEXT instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ext(Word code)
{
  Word Rd, F;
  
  if (ChkArgCnt(1, 2) 
   && decode_field_index(2, &F)
   && (eIsReg == decode_reg(&ArgStr[1], &Rd, NULL, eSymbolSize32Bit, True)))
    append_opcode(code | (F << 9) | Rd);  
}

/*!------------------------------------------------------------------------
 * \fn     decode_swapf(Word code)
 * \brief  handle SWAPF instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_swapf(Word code)
{
  Word Rd, F;
  adr_vals_t src_vals;

  if (!ChkArgCnt(2, 3)
   || (eIsReg != decode_reg(&ArgStr[2], &Rd, NULL, eSymbolSize32Bit, True))
   || !decode_field_index(3, &F))
    return;
  if (F)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[3]);
    return;
  }
  switch (decode_adr(&ArgStr[1], &src_vals))
  {
    case e_mode_ireg:
      decode_two_reg_core(code, src_vals.reg, Rd);
      break;
    case e_mode_none:
      break;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_cexec(Word code)
 * \brief  handle CEXEC instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static Boolean decode_coproc_id(const tStrComp *p_arg, Boolean *p_ok)
{
  if (p_arg->str.p_str[0])
    return EvalStrIntExpression(p_arg, UInt3, p_ok);
  else
  {
    *p_ok = True;
    return coproc;
  }
}

static void decode_cexec(Word code)
{
  Word size, id = coproc;
  LongWord command;
  Boolean ok, long_form;

  if (!ChkArgCnt(2, 4)
   || !ChkMinCPU(cpu_34020))
    return;

  size = EvalStrIntExpression(&ArgStr[1], UInt1, &ok);
  if (!ok)
    return;
  command = EvalStrIntExpression(&ArgStr[2], UInt21, &ok);
  if (!ok)
    return;
  if (ArgCnt >= 3)
  {
    id = decode_coproc_id(&ArgStr[3], &ok);
    if (!ok)
      return;
  }
  if ((ArgCnt >= 4) && ArgStr[4].str.p_str[0])
  {
    if (!as_strcasecmp(ArgStr[4].str.p_str, "L"))
      long_form = True;
    else
    {
      WrStrErrorPos(ErrNum_InvArg, &ArgStr[4]);
      return;
    }
  }
  else
    long_form = !!(code & 0xc0);

  if (long_form)
  {
    append_opcode(0x0600);
    append_opcode(((command << 8) & 0xff00u) | (size << 7));
  }
  else
    append_opcode(0xd800 | ((command & 0x3f) << 1) | size);
  append_opcode(((command >> 8) & 0x1fffu) | (id << 13));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmovcg(Word code)
 * \brief  handle CMOVCG instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

/* Variants:
   CMOVCG Rd1,command
   CMOVCG Rd1,command,id
   CMOVCG Rd1,Rd2,command
   CMOVCG Rd1,Rd2,command,id
   CMOVCG Rd1,Rd2,1,command
   CMOVCG Rd1,Rd2,1,command,id
*/

static void decode_cmovcg(Word code)
{
  Word Rd1, Rd2 = 0, size = 0, id = coproc;
  LongWord command;
  Boolean ok;

  UNUSED(code);

  /* Argument 1 must be Rd1: */

  if (ArgCnt < 2)
  {
    WrError(ErrNum_WrongArgCnt);
    return;
  }
  if (eIsReg != decode_reg(&ArgStr[1], &Rd1, NULL, eSymbolSize32Bit, True))
    return;

  /* Argument 2 may be Rd2 or command: */

  switch (decode_reg(&ArgStr[2], &Rd2, NULL, eSymbolSize32Bit, False))
  {
    case eIsReg:
      size = 1;
      switch (ArgCnt)
      {
        case 3: /* Rd1,Rd2,command */
          command = EvalStrIntExpression(&ArgStr[3], UInt21, &ok);
          if (!ok)
            return;
          break;
        case 4: /* Rd1,Rd2,command,id or Rd1,Rd2,size,command */
        {
          LongWord v1, v2;

          v1 = EvalStrIntExpression(&ArgStr[3], UInt21, &ok);
          if (!ok)
            return;
          v2 = EvalStrIntExpression(&ArgStr[4], UInt21, &ok);
          if (!ok)
            return;
          if (v1 <= 1)
          {
            size = v1; command = v2;
          }
          else if (v2 <= 7)
          {
            command = v1; id = v2;
          }
          else
          {
            WrStrErrorPos(ErrNum_OverRange, &ArgStr[4]);
            return;
          }
          break;
        }
        case 5:
          size = EvalStrIntExpression(&ArgStr[3], UInt21, &ok);
          if (!ok)
            return;
          command = EvalStrIntExpression(&ArgStr[4], UInt21, &ok);
          if (!ok)
            return;
          id = decode_coproc_id(&ArgStr[5], &ok);
          if (!ok)
            return;
          break;
        default:
          (void)ChkArgCnt(3, 5);
          return;
      }
      break;
    case eIsNoReg:
      Rd2 = 0;
      command = EvalStrIntExpression(&ArgStr[2], UInt21, &ok);
      if (!ok)
        return;
      switch (ArgCnt)
      {
        case 2:
          break;
        case 3:
          id = decode_coproc_id(&ArgStr[3], &ok);
          if (!ok)
            return;
          break;
        default:
          (void)ChkArgCnt(2, 3);
          return;
      }
      break;
    default:
      return;
  }

  append_opcode(0x0660 | Rd1);
  append_opcode(((command & 0xff) << 8) | (size << 7) | Rd2);
  append_opcode((id << 13) | ((command >> 8) & 0x1fff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmovcm(Word code)
 * \brief  handle CMOVCM instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static Word decode_coproc_mem_count(const tStrComp *p_arg, Word size, Boolean *p_ok)
{
  tEvalResult eval_result;
  Word count;

  count = EvalStrIntExpressionWithResult(p_arg, size ? UInt5 : UInt6, &eval_result);  
  *p_ok = eval_result.OK;
  if (!*p_ok)
    return count;
  if (size)
  {
    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && !ChkRange(count, 1, 16))
      *p_ok = False;
    else
      count = (count * 2) & 0x1f;
  }
  else
  {
    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && !ChkRange(count, 1, 32))
      *p_ok = False;
    else
      count &= 0x1f;
  }
  return count; 
}

static void decode_cmovcm(Word code)
{
  adr_vals_t rd_vals;
  Word count, size, id = coproc;
  LongWord command;
  Boolean ok;

  UNUSED(code);

  if (!ChkArgCnt(4, 5))
    return;

  switch (decode_adr(&ArgStr[1], &rd_vals))
  {
    case e_mode_ireg_inc:
      code = 0x06a0;
      break;
    case e_mode_ireg_dec:
      code = 0x06c0;
      break;
    case e_mode_none:
      return;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      return;
  }

  size = EvalStrIntExpression(&ArgStr[3], UInt1, &ok);
  if (!ok)
    return;

  count = decode_coproc_mem_count(&ArgStr[2], size, &ok);
  if (!ok)
    return;

  command = EvalStrIntExpression(&ArgStr[4], UInt21, &ok);
  if (!ok)
    return;

  if (ArgCnt == 5)
  {
    id = decode_coproc_id(&ArgStr[5], &ok);
    if (!ok)
      return;
  }

  append_opcode(code | rd_vals.reg);
  append_opcode(((command & 0xff) << 8) | (size << 7) | count);
  append_opcode((id << 13) | ((command >> 8) & 0x1fff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmovcs(Word code)
 * \brief  handle CMOVCS instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cmovcs(Word code)
{
  Word id = coproc;
  LongWord command;
  Boolean ok;

  UNUSED(code);

  if (!ChkArgCnt(1, 2))
    return;

  command = EvalStrIntExpression(&ArgStr[1], UInt21, &ok);
  if (!ok)
    return;

  if (ArgCnt == 2)
  {
    id = decode_coproc_id(&ArgStr[2], &ok);
    if (!ok)
      return;
  }

  append_opcode(0x0660);
  append_opcode(((command & 0xff) << 8) | 0x0001);
  append_opcode((id << 13) | ((command >> 8) & 0x1fff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmovgc(Word code)
 * \brief  handle CMOVCG instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

/* Variants:
   CMOVGC Rs,command
   CMOVGC Rs,command,id
   CMOVGC Rs1,Rs2,size,command
   CMOVGC Rs1,Rs2,size,command,id
*/

static void decode_cmovgc(Word code)
{
  Word Rs1, Rs2 = 0, size = 0, id = coproc;
  LongWord command;
  Boolean ok;
  int command_index;

  if (!ChkArgCnt(2, 5)
   || (eIsReg != decode_reg(&ArgStr[1], &Rs1, NULL, eSymbolSize32Bit, True)))
    return;

  if (ArgCnt >= 4)
  {
    if (eIsReg != decode_reg(&ArgStr[2], &Rs2, NULL, eSymbolSize32Bit, True))
      return;
    size = EvalStrIntExpression(&ArgStr[3], UInt1, &ok);
    if (!ok)
      return;
    command_index = 4;
    code = 0x0640;
  }
  else
  {
    command_index = 2;
    code = 0x0620;
  }

  command = EvalStrIntExpression(&ArgStr[command_index], UInt21, &ok);
  if (!ok)
    return;
  if (ArgCnt > command_index)
  {
    id = decode_coproc_id(&ArgStr[command_index + 1], &ok);
    if (!ok)
      return;
  }

  append_opcode(code | Rs1);
  append_opcode(((command & 0xff) << 8) | (size << 7) | Rs2);
  append_opcode((id << 13) | ((command >> 8) & 0x1fff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_cmovmc(Word code)
 * \brief  handle CMOVMC instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_cmovmc(Word code)
{
  adr_vals_t rs_vals;
  Word count, size, id = coproc;
  LongWord command;
  Boolean ok, imm_count;

  if (!ChkArgCnt(4, 5))
    return;

  switch (decode_adr(&ArgStr[1], &rs_vals))
  {
    case e_mode_ireg_inc:
      code = 0x0680;
      imm_count = False;
      break;
    case e_mode_ireg_dec:
      code = 0x0820;
      imm_count = True;
      break;
    case e_mode_none:
      return;
    default:
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      return;
  }

  size = EvalStrIntExpression(&ArgStr[3], UInt1, &ok);
  if (!ok)
    return;

  if (!imm_count)
  {
    switch (decode_reg(&ArgStr[2], &count, NULL, eSymbolSize32Bit, False))
    {
      case eRegAbort:
        return;
      case eIsReg:
        code = 0x06e0;
        break;
      default:
        imm_count = True;
    }
  }
  if (imm_count)
  {
    count = decode_coproc_mem_count(&ArgStr[2], size, &ok);
    if (!ok)
      return;
  }

  command = EvalStrIntExpression(&ArgStr[4], UInt21, &ok);
  if (!ok)
    return;

  if (ArgCnt >= 5)
  {
    id = decode_coproc_id(&ArgStr[5], &ok);
    if (!ok)
      return;
  }

  append_opcode(code | count);
  append_opcode(((command & 0xff) << 8) | (size << 7) | rs_vals.reg);
  append_opcode((id << 13) | ((command >> 8) & 0x1fff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_field(Word code)
 * \brief  handle FIELD instruction
 * ------------------------------------------------------------------------ */

static void decode_field(Word code)
{
  LargeInt field_value, field_length;
  Boolean ok;
  tSymbolFlags flags;

  UNUSED(code);

  if (!ChkArgCnt(2, 2))
    return;

  field_length = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt6, &ok, &flags);
  if (!ok)
    return;
  if (mFirstPassUnknownOrQuestionable(flags))
  {
    WrStrErrorPos(ErrNum_FirstPassCalc, &ArgStr[2]);
    return;
  }
  if (field_length > 32)
  {
    WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
    return;
  }
  field_value = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &ok, &flags);
  if (!ok)
    return;
#ifndef HAS64
  if (field_length < 32)
#endif
  {
    if (field_value >= 0)
    {
      LargeInt max_v = (1ul << field_length) - 1;
      if (mFirstPassUnknownOrQuestionable(flags))
        field_value &= max_v;
      else if (field_value > max_v)
      {
        WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
        return;
      }
    }
    else
    {
      LargeInt min_v = -(1ul << (field_length - 1)) - 1;
      if (mFirstPassUnknownOrQuestionable(flags))
        field_value &= min_v + 1;
      else if (field_value < min_v)
      {
        WrStrErrorPos(ErrNum_UnderRange, &ArgStr[1]);
        return;
      }
    }
  }
  set_basmcode_bit_field_le(CodeLen, field_value, field_length);
  CodeLen += field_length;
  ActListGran = (field_length > 8) ? 2 : 1;
  act_list_gran_bits_unused = (field_length > 16) ? 0 : (ActListGran * 8) - field_length;
}

/*!------------------------------------------------------------------------
 * \fn     decode_byte(Word code)
 * \brief  handle BYTE instruction
 * ------------------------------------------------------------------------ */

static void decode_byte(Word code)
{
  UNUSED(code);
  DecodeIntelDB(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt);
  if (CodeLen > 0)
  {
    ActListGran = 1;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_string(Word code)
 * \brief  handle STRING instruction
 * ------------------------------------------------------------------------ */

static void decode_string(Word code)
{
  UNUSED(code);
  DecodeIntelDB(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowString);
  if (CodeLen > 0)
  {
    ActListGran = 1;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_word(Word code)
 * \brief  handle WORD instruction
 * ------------------------------------------------------------------------ */

static void decode_word(Word code)
{
  UNUSED(code);
  DecodeIntelDW(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt);
  if (CodeLen > 0)
  {
    ActListGran = 2;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_long(Word code)
 * \brief  handle INT/LONG instruction
 * ------------------------------------------------------------------------ */

static void decode_long(Word code)
{
  UNUSED(code);
  DecodeIntelDD(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_AllowInt);
  if (CodeLen > 0)
  {
    ActListGran = 4;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_double(Word code)
 * \brief  handle DOUBLE instruction
 * ------------------------------------------------------------------------ */

static void decode_double(Word code)
{
  UNUSED(code);
  DecodeIntelDQ(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_TMS340Format | eIntPseudoFlag_AllowFloat);
  if (CodeLen > 0)
  {
    ActListGran = 4;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_float(Word code)
 * \brief  handle FLOAT instruction
 * ------------------------------------------------------------------------ */

static void decode_float(Word code)
{
  UNUSED(code);
  DecodeIntelDD(eIntPseudoFlag_LittleEndian | eIntPseudoFlag_TMS340Format | eIntPseudoFlag_AllowFloat);
  if (CodeLen > 0)
  {
    ActListGran = 4;
    act_list_gran_bits_unused = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_space(Word is_bes)
 * \brief  handle SPACE/BES instructions
 * \param  is_bes BES instruction (lable points to end of area)
 * ------------------------------------------------------------------------ */

static void decode_space(Word is_bes)
{
  DecodeIntelDS(1);
  if (is_bes)
    LabelHandle(&LabPart, EProgCounter() + CodeLen, False);
}

/*!------------------------------------------------------------------------
 * \fn     decode_bss(Word code)
 * \brief  handle BSS instruction
 * ------------------------------------------------------------------------ */

static void decode_bss(Word code)
{
  tSymbolFlags flags;
  Boolean ok;
  LongWord pad, size;

  UNUSED(code);

  if (!ChkArgCnt(2, 3))
    return;

  size = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt32, &ok, &flags);
  if (!ok)
    return;
  if (mFirstPassUnknown(flags))
  {
    WrStrErrorPos(ErrNum_FirstPassCalc, &ArgStr[2]);
    return;
  }
  if (ArgCnt == 3)
  {
    pad = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt32, &ok, &flags);
    if (!ok)
      return;
    if (mFirstPassUnknown(flags))
    {
      WrStrErrorPos(ErrNum_FirstPassCalc, &ArgStr[3]);
      return;
    }
  }
  else
    pad = 0;

  if (pad)
  {
    LongInt new_pc = (EProgCounter() + 15) & ~15;
    CodeLen = new_pc - EProgCounter();
  }  
  else
    CodeLen = 0;
  EnterIntSymbol(&ArgStr[1], EProgCounter() + CodeLen, ActPC, False);
  CodeLen += size;

  DontPrint = !!CodeLen;
  BookKeeping();
}

/*!------------------------------------------------------------------------
 * \fn     decode_even(Word code)
 * \brief  handle EVEN instruction
 * ------------------------------------------------------------------------ */

static void decode_even(Word code)
{
  LongInt new_pc;

  UNUSED(code);
  new_pc = (EProgCounter() + 15) & ~15;
  CodeLen = new_pc - EProgCounter();
  DontPrint = !!CodeLen;
  BookKeeping();
}

/*!------------------------------------------------------------------------
 * \fn     decode_coproc(Word code)
 * \brief  handle COPROC instruction
 * ------------------------------------------------------------------------ */

static void decode_coproc(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(1, 1) && ChkMinCPU(cpu_34020))
  {
    Word new_coproc;
    Boolean ok;

    new_coproc = EvalStrIntExpression(&ArgStr[1], UInt3, &ok);
    if (ok)
      set_coproc(new_coproc);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_instr(Word code)
 * \brief  handle REG instruction
 * ------------------------------------------------------------------------ */

static void decode_reg_instr(Word code)
{
  UNUSED(code);

  if (LabPart.str.p_str[0])
    CodeREG(0);
  else if (ChkArgCnt(1, 1))
  {
    Boolean is_on;

    if (CheckONOFFArg(&ArgStr[1], &is_on))
      SetFlag(&default_regsyms, default_regsyms_name, is_on);
  }
}

/*!------------------------------------------------------------------------
 * \fn     check_pc_aligned(Word code)
 * \brief  before handling machine instructions, check whether PC is on a
 *         word boundary
 * ------------------------------------------------------------------------ */

static void check_pc_aligned(Word code)
{
  UNUSED(code);
  if (!code_aligned(EProgCounter()))
    WrError(ErrNum_AddrNotAligned);
}

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  set up hash table
 * ------------------------------------------------------------------------ */

static void add_condition(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "JA%s", p_name);
  AddInstTable(InstTable, name, 0xc080 | (code << 8), decode_jacc);
  as_snprintf(name, sizeof(name), "JR%s", p_name);
  AddInstTable(InstTable, name, 0xc000 | (code << 8), decode_jrcc);
}

static void add_dot_pseudo(const char *p_name, Word code, InstProc decode_fnc)
{
  char name[20];
  AddInstTable(InstTable, p_name, code, decode_fnc);
  as_snprintf(name, sizeof(name), ".%s", p_name);
  AddInstTable(InstTable, name, code, decode_fnc);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(307);
  SetDynamicInstTable(InstTable);

  inst_table_set_prefix_proc(InstTable, check_pc_aligned, 0);

  AddInstTable(InstTable, "CLRC",   0x0320, decode_fixed);
  AddInstTable(InstTable, "DINT",   0x0360, decode_fixed);
  AddInstTable(InstTable, "EINT",   0x0d60, decode_fixed);
  AddInstTable(InstTable, "EMU",    0x0100, decode_fixed);
  AddInstTable(InstTable, "NOP",    NOPCode, decode_fixed);
  AddInstTable(InstTable, "POPST",  0x01c0, decode_fixed);
  AddInstTable(InstTable, "PUSHST", 0x01e0, decode_fixed);
  AddInstTable(InstTable, "RETI",   0x0940, decode_fixed);
  AddInstTable(InstTable, "SETC",   0x0de0, decode_fixed);
  AddInstTable(InstTable, "CLIP",   0x08f2, decode_fixed_020);
  AddInstTable(InstTable, "FPIXEQ", 0x0abb, decode_fixed_020);
  AddInstTable(InstTable, "FPIXNE", 0x0adb, decode_fixed_020);
  AddInstTable(InstTable, "IDLE"  , 0x0040, decode_fixed_020);
  AddInstTable(InstTable, "LINIT" , 0x0c57, decode_fixed_020);
  AddInstTable(InstTable, "MWAIT" , 0x0080, decode_fixed_020);
  AddInstTable(InstTable, "RETM",   0x0860, decode_fixed_020);
  AddInstTable(InstTable, "SETCDP", 0x0273, decode_fixed_020);
  AddInstTable(InstTable, "SETCMP", 0x02fb, decode_fixed_020);
  AddInstTable(InstTable, "SETCSP", 0x0251, decode_fixed_020);
  AddInstTable(InstTable, "VLCOL" , 0x0a00, decode_fixed_020);

  AddInstTable(InstTable, "ABS",   0x0380, decode_one_reg);
  AddInstTable(InstTable, "CLR",   0x5600 | CODE_FLAG_DUP_ARG, decode_one_reg);
  AddInstTable(InstTable, "DEC",   0x1420, decode_one_reg);
  AddInstTable(InstTable, "INC",   0x1020, decode_one_reg);
  AddInstTable(InstTable, "NEG",   0x03a0, decode_one_reg);
  AddInstTable(InstTable, "NEGB",  0x03c0, decode_one_reg);
  AddInstTable(InstTable, "NOT",   0x03e0, decode_one_reg);
  AddInstTable(InstTable, "CALL",  0x0920, decode_one_reg);
  AddInstTable(InstTable, "EXGPC", 0x0120, decode_one_reg);
  AddInstTable(InstTable, "GETPC", 0x0140, decode_one_reg);
  AddInstTable(InstTable, "GETST", 0x0180, decode_one_reg);
  AddInstTable(InstTable, "PUTST", 0x01a0, decode_one_reg);
  AddInstTable(InstTable, "REV",   0x0020, decode_one_reg);
  AddInstTable(InstTable, "JUMP",  0x0160, decode_one_reg);
  AddInstTable(InstTable, "CVDXYL",0x0a80 | CODE_FLAG_020, decode_one_reg);
  AddInstTable(InstTable, "CVMXYL",0x0a60 | CODE_FLAG_020, decode_one_reg);
  AddInstTable(InstTable, "EXGPS", 0x02a0 | CODE_FLAG_020, decode_one_reg);
  AddInstTable(InstTable, "GETPS", 0x02c0 | CODE_FLAG_020, decode_one_reg);
  AddInstTable(InstTable, "RPIX",  0x0280 | CODE_FLAG_020, decode_one_reg);

  AddInstTable(InstTable, "ADD",   0x4000, decode_two_reg);
  AddInstTable(InstTable, "ADDC",  0x4200, decode_two_reg);
  AddInstTable(InstTable, "ADDXY", 0xe000, decode_two_reg);
  AddInstTable(InstTable, "AND",   0x5000, decode_two_reg);
  AddInstTable(InstTable, "ANDN",  0x5200, decode_two_reg);
  AddInstTable(InstTable, "BTST",  0x4a00, decode_two_reg);
  AddInstTable(InstTable, "CMP",   0x4800, decode_two_reg);
  AddInstTable(InstTable, "CMPXY", 0xe400, decode_two_reg);
  AddInstTable(InstTable, "CPW",   0xe600, decode_two_reg);
  AddInstTable(InstTable, "CVXYL", 0xe800, decode_two_reg);
  AddInstTable(InstTable, "DIVS",  0x5800 | CODE_FLAG_DEST_PAIR, decode_two_reg);
  AddInstTable(InstTable, "DIVU",  0x5a00 | CODE_FLAG_DEST_PAIR, decode_two_reg);
  AddInstTable(InstTable, "DRAV",  0xf600, decode_two_reg);
  AddInstTable(InstTable, "LMO",   0x6a00, decode_two_reg);
  AddInstTable(InstTable, "MODS",  0x6c00, decode_two_reg);
  AddInstTable(InstTable, "MODU",  0x6e00, decode_two_reg);
  AddInstTable(InstTable, "MOVX",  0xec00, decode_two_reg);
  AddInstTable(InstTable, "MOVY",  0xee00, decode_two_reg);
  AddInstTable(InstTable, "MPYS",  0x5c00 | CODE_FLAG_DEST_PAIR, decode_two_reg);
  AddInstTable(InstTable, "MPYU",  0x5e00 | CODE_FLAG_DEST_PAIR, decode_two_reg);
  AddInstTable(InstTable, "OR",    0x5400, decode_two_reg);
  AddInstTable(InstTable, "SUB",   0x4400, decode_two_reg);
  AddInstTable(InstTable, "SUBB",  0x4600, decode_two_reg);
  AddInstTable(InstTable, "SUBXY", 0xe200, decode_two_reg);
  AddInstTable(InstTable, "XOR",   0x5600, decode_two_reg);
  AddInstTable(InstTable, "CVSXYL",0xea00 | CODE_FLAG_020, decode_two_reg);
  AddInstTable(InstTable, "RMO",   0x7a00 | CODE_FLAG_020, decode_two_reg);

  AddInstTable(InstTable, "ADDI",  0x0b00 | CODE_FLAG_HAS_SHORT_FORM, decode_reg_imm);
  AddInstTable(InstTable, "ADDXYI",0x0c00 | CODE_FLAG_020, decode_reg_imm);
  AddInstTable(InstTable, "ANDI",  0x0b80 | CODE_FLAG_INSERT_COMPLEMENT, decode_reg_imm);
  AddInstTable(InstTable, "ANDNI", 0x0b80, decode_reg_imm);
  AddInstTable(InstTable, "CMPI",  0x0b40 | CODE_FLAG_HAS_SHORT_FORM, decode_reg_imm);
  AddInstTable(InstTable, "MOVI",  0x09c0 | CODE_FLAG_HAS_SHORT_FORM, decode_reg_imm);
  AddInstTable(InstTable, "ORI",   0x0ba0, decode_reg_imm);
  AddInstTable(InstTable, "SUBI",  0x0be0 | CODE_FLAG_HAS_SHORT_FORM | CODE_FLAG_INSERT_COMPLEMENT, decode_reg_imm); /* 0x0d00 */
  AddInstTable(InstTable, "XORI",  0x0bc0, decode_reg_imm);

  AddInstTable(InstTable, "ADDK",  0x1000, decode_reg_const);
  AddInstTable(InstTable, "MOVK",  0x1800, decode_reg_const);
  AddInstTable(InstTable, "SUBK",  0x1400, decode_reg_const);
  AddInstTable(InstTable, "CMPK",  0x3400 | CODE_FLAG_020, decode_reg_const);

  AddInstTable(InstTable, "RL",    0x3068, decode_shift);
  AddInstTable(InstTable, "SLA",   0x2060, decode_shift);
  AddInstTable(InstTable, "SLL",   0x2462, decode_shift);
  AddInstTable(InstTable, "SRA",   0x2964, decode_shift);
  AddInstTable(InstTable, "SRL",   0x2d66, decode_shift);

  add_condition("UC",   0x0);
  add_condition("LO",   0x8);
  add_condition("C",    0x8);
  add_condition("LS",   0x2);
  add_condition("YLE",  0x2);
  add_condition("HI",   0x3);
  add_condition("YGT",  0x3);
  add_condition("HS",   0x9);
  add_condition("NC",   0x9);
  add_condition("EQ",   0xa);
  add_condition("Z",    0xa);
  add_condition("NE",   0xb);
  add_condition("NZ",   0xb);
  add_condition("LT",   0x4);
  add_condition("XLE",  0x4);
  add_condition("LE",   0x6);
  add_condition("GT",   0x7);
  add_condition("GE",   0x5);
  add_condition("XGT",  0x5);
  add_condition("YZ",   0xa);
  add_condition("YNZ",  0xb);
  add_condition("P",    0x1);
  add_condition("N",    0xe);
  add_condition("XZ",   0xe);
  add_condition("NN",   0xf);
  add_condition("XNZ",  0xf);
  add_condition("YNN",  0x9);
  add_condition("B",    0x8);
  add_condition("YN",   0x8);
  add_condition("NB",   0x9);
  add_condition("V",    0xc);
  add_condition("XN",   0xc);
  add_condition("NV",   0xd);
  add_condition("XNN",  0xd);

  AddInstTable(InstTable, "EXGF", 0xd500, decode_exgf);
  AddInstTable(InstTable, "FILL", 0x0fc0, decode_fill);
  AddInstTable(InstTable, "LINE", 0xdf1a, decode_line);
  AddInstTable(InstTable, "FLINE", 0xde1a, decode_line);
  AddInstTable(InstTable, "PIXBLT", 0x0f00, decode_pixblt);
  AddInstTable(InstTable, "PIXT", 0xf000, decode_pixt);
  AddInstTable(InstTable, "SETF", 0x0540, decode_setf);
  AddInstTable(InstTable, "SEXT",  0x0500, decode_ext);
  AddInstTable(InstTable, "ZEXT",  0x0520, decode_ext);
  AddInstTable(InstTable, "PFILL", 0x0a37, decode_pfill);
  AddInstTable(InstTable, "TFILL", 0x0efa, decode_pfill);
  AddInstTable(InstTable, "BLMOVE", 0x00f0, decode_blmove);
  AddInstTable(InstTable, "SWAPF", 0x7e00, decode_swapf);
  AddInstTable(InstTable, "VBLT", 0x0857, decode_vblt);
  AddInstTable(InstTable, "VFILL", 0x0a57, decode_vfill);

  AddInstTable(InstTable, "MMFM", 0x09a0, decode_mmxm);
  AddInstTable(InstTable, "MMTM", 0x0980, decode_mmxm);
  AddInstTable(InstTable, "MOVB", 0, decode_movb);
  AddInstTable(InstTable, "MOVE", 0, decode_move);

  AddInstTable(InstTable, "CALLA", 0x0d5f, decode_jacc);
  AddInstTable(InstTable, "CALLR", 0x0d3f, decode_callr);
  AddInstTable(InstTable, "DSJ",   0x0d81, decode_dsj);
  AddInstTable(InstTable, "DSJS",  0x3800, decode_dsjs);
  AddInstTable(InstTable, "DSJEQ", 0x0da0, decode_dsj);
  AddInstTable(InstTable, "DSJNE", 0x0dc0, decode_dsj);
  AddInstTable(InstTable, "RETS",  0x0960, decode_rets);
  AddInstTable(InstTable, "TRAP",  0x0900, decode_trap);
  AddInstTable(InstTable, "TRAPL", 0x080f, decode_trapl);
  AddInstTable(InstTable, "CEXEC", 0, decode_cexec);
  AddInstTable(InstTable, "CMOVCG",0, decode_cmovcg);
  AddInstTable(InstTable, "CMOVCM",0, decode_cmovcm);
  AddInstTable(InstTable, "CMOVCS",0, decode_cmovcs);
  AddInstTable(InstTable, "CMOVGC",0, decode_cmovgc);
  AddInstTable(InstTable, "CMOVMC",0, decode_cmovmc);

  inst_table_set_prefix_proc(InstTable, NULL, 0);

 add_dot_pseudo("BYTE", 0, decode_byte);
 add_dot_pseudo("STRING", 0, decode_string);
 add_dot_pseudo("WORD", 0, decode_word);
 add_dot_pseudo("INT", 0, decode_long);
 add_dot_pseudo("LONG", 0, decode_long);
 add_dot_pseudo("FLOAT", 0, decode_float);
 add_dot_pseudo("DOUBLE", 0, decode_double);
 add_dot_pseudo("SPACE", 0, decode_space);
 add_dot_pseudo("BES", 1, decode_space);
 add_dot_pseudo("BSS", 0, decode_bss);
 add_dot_pseudo("EVEN", 0, decode_even);
 add_dot_pseudo("FIELD", 0, decode_field);
 add_dot_pseudo("REG", 0, decode_reg_instr);
 add_dot_pseudo(coproc_name, 0, decode_coproc);
}

/*!------------------------------------------------------------------------
 * \fn     deinit_fields(void)
 * \brief  destroy hash table
 * ------------------------------------------------------------------------ */

static void deinit_fields(void)
{
  DestroyInstTable(InstTable);
}

/*!------------------------------------------------------------------------
 * \fn     intern_symbol_340xx(char *p_arg, TempResult *p_result)
 * \brief  handle built-in (register) symbols for TMS340xx
 * \param  p_arg source argument
 * \param  p_result result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_340xx(char *p_arg, TempResult *p_result)
{
  Word reg_num;
  int l;
  char *p_sep;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_340xx;
  }

  /* TMS340 special: [x,y] is a 32 bit x/y constant as used in
     the ...XY instructions: */

  l = strlen(p_arg);
  if ((l < 5) || (p_arg[0] != '[') || (p_arg[l - 1] != ']'))
    return;
  p_sep = QuotPos(p_arg + 1, ',');
  if (p_sep)
  {
    String str;
    tStrComp comp;
    LongWord res;
    Boolean ok;

    StrCompMkTemp(&comp, str, sizeof(str));
    strmemcpy(str, sizeof(str), p_arg + 1, p_sep - p_arg - 1);
    res = EvalStrIntExpression(&comp, Int16, &ok) & 0xffffu;
    if (!ok) return;
    strmemcpy(str, sizeof(str), p_sep + 1, l - (p_sep - p_arg) - 2);
    res = (res << 16) | (EvalStrIntExpression(&comp, Int16, &ok) & 0xffffu);
    if (!ok) return;
    as_tempres_set_int(p_result, res);
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_340xx(void)
 * \brief  actual code generation
 * ------------------------------------------------------------------------ */

static void make_code_340xx(void)
{
  /* to be ignored */

  if (*OpPart.str.p_str == '\0') return;

  /* search */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_340xx(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_340xx(void)
{
  return Memo("REG") || Memo("BES") || Memo(".BES");
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_340xx(void)
 * \brief  switch to TMS340 target
 * ------------------------------------------------------------------------ */

static void switch_to_340xx(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("TMS340xx");

  PCSymbol = "$";
  HeaderID = p_descr->Id;
  NOPCode = 0x0300;
  DivideChars = ",";
  HasAttrs = False;
  SetIntConstMode(eIntConstModeIntel);

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = ListGrans[SegCode] = 1;
  list_grans_bits_unused[SegCode] = grans_bits_unused[SegCode] = 7;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = (MomCPU == cpu_34020) ? 0xfffffffful : 0x3ffffffful;
  MakeCode = make_code_340xx;
  IsDef = is_def_340xx;
  InternSymbol = intern_symbol_340xx;
  DissectReg = dissect_reg_340xx;

  init_fields();
  SwitchFrom = deinit_fields;

  if ((MomCPU == cpu_34020) && !coproc_set)
    set_coproc(0);
  if (!onoff_ext_test_and_set(0x80))
    SetFlag(&default_regsyms, default_regsyms_name, True);
}

/*!------------------------------------------------------------------------
 * \fn     init_pass_vax(void)
 * \brief  initializations at beginning of pass
 * ------------------------------------------------------------------------ */

static void init_pass_340xx(void)
{
  /* Flag to introduce & initialize symbol at first switch to target */

  coproc_set = False;
}

/*!------------------------------------------------------------------------
 * \fn     code340xx_init(void)
 * \brief  register target(s)
 * ------------------------------------------------------------------------ */

void code340xx_init(void)
{
  cpu_34010 = AddCPU("34010", switch_to_340xx);
  cpu_34020 = AddCPU("34020", switch_to_340xx);
  AddInitPassProc(init_pass_340xx);
}
