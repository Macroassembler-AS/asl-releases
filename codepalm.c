/* codepalm.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator PALM                                                       */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmallg.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmcode.h"
#include "headids.h"
#include "codevars.h"
#include "intpseudo.h"
#include "codepseudo.h"
#include "onoff_common.h"

#define REG_PC 0

#define MODIFIER_0 8

typedef enum
{
  ModReg = 0,  /* Rn */
  ModIReg = 1, /* (Rn), (Rn)-, (Rn)+ */
  ModDir = 2,  /* nn */
  ModImm = 3,  /* #nn, #nnnn */
  ModImmP1 = 4, /* #nn -> nn+1 */
  ModIO = 5,   /* n in I/O space */
  ModNone = 0xff
} adr_mode_t;

#define MModReg (1 << ModReg)
#define MModIReg (1 << ModIReg)
#define MModDir (1 << ModDir)
#define MModImm (1 << ModImm)
#define MModImmP1 (1 << ModImmP1)
#define MModIO (1 << ModIO)

typedef struct
{
  adr_mode_t mode;
  Word value;
} adr_vals_t;

static CPUVar cpu_5100, cpu_5110, cpu_5120;
static Boolean last_was_skip, this_was_skip;

/*--------------------------------------------------------------------------*/
/* Register Aliases */

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
  if (!as_strcasecmp(p_arg, "PC"))
  {
    *p_result = REG_PC | REGSYM_FLAG_ALIAS;
    *p_size = eSymbolSize16Bit;
    return True;
  }

  if (as_toupper(*p_arg) != 'R')
    return False;
  p_arg++;

  *p_result = 0;
  switch (strlen(p_arg))
  {
    case 2:
      if (*p_arg != '1')
        return False;
      *p_result += 10;
      p_arg++;
      /* FALL-THRU */
    case 1:
      if (!as_isdigit(*p_arg))
        return False;
      *p_result += *p_arg - '0';
      *p_size = eSymbolSize16Bit;
      return *p_result <= 15;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg(const tStrComp *p_arg, Word *p_result, Boolean must_be_reg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  p_arg argument to check
 * \param  p_result numeric register value if yes
 * \param  must_be_reg argument is expected to be a register
 * \return RegEvalResult
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

  *p_result = reg_descr.Reg & ~REGSYM_FLAG_ALIAS;
  return reg_eval_result;
}

/*!------------------------------------------------------------------------
 * \fn     dissect_reg_palm(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
 * \brief  dissect register symbols - PALM variant
 * \param  p_dest destination buffer
 * \param  dest_size destination buffer size
 * \param  value numeric register value
 * \param  inp_size register size
 * ------------------------------------------------------------------------ */

static void dissect_reg_palm(char *p_dest, size_t dest_size, tRegInt value, tSymbolSize inp_size)
{
  switch (inp_size)
  {
    case eSymbolSize16Bit:
      switch (value)
      {
        case REGSYM_FLAG_ALIAS | REG_PC:
          as_snprintf(p_dest, dest_size, "PC");
          break;
        default:
          as_snprintf(p_dest, dest_size, "R%u", (unsigned)(value & 15));
      }
      break;
    default:
      as_snprintf(p_dest, dest_size, "%d-%u", (int)inp_size, (unsigned)value);
  }
}

/*--------------------------------------------------------------------------*/
/* Address Expression Decoders */

/*!------------------------------------------------------------------------
 * \fn     decode_direct_word_address(const tStrComp *p_arg, Word *p_result)
 * \brief  evaluate direct address, which must be halfword-aligned (0,2,4...510)
 * \param  p_arg source argument
 * \param  p_result encoded result
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean decode_direct_word_address(const tStrComp *p_arg, Word *p_result)
{
  tEvalResult eval_result;

  *p_result = EvalStrIntExpressionWithResult(p_arg, UInt9, &eval_result);
  if (!eval_result.OK)
    return False;
  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags) && (*p_result & 1))
  {
    WrStrErrorPos(ErrNum_AddrMustBeEven, p_arg);
    return False;
  }
  *p_result = (*p_result >> 1) & 0xff;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_code_address(const tStrComp *p_arg, Word *p_result, tEvalResult *p_eval_result)
 * \brief  evaluate code (instruction) address, which must be halfword-aligned (0,2,4...510)
 * \param  p_arg source argument
 * \param  p_result encoded result
 * \param  p_eval_result additional evaluation flags
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean decode_code_address(const tStrComp *p_arg, Word *p_result, tEvalResult *p_eval_result)
{
  *p_result = EvalStrIntExpressionWithResult(p_arg, UInt16, p_eval_result);
  if (!p_eval_result->OK)
    return False;
  ChkSpace(SegCode, p_eval_result->AddrSpaceMask);
  if (!mFirstPassUnknownOrQuestionable(p_eval_result->Flags) && (*p_result & 1))
  {
    WrStrErrorPos(ErrNum_AddrMustBeEven, p_arg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_io_address(const tStrComp *p_arg, Word *p_result)
 * \brief  evaluate I/O address
 * \param  p_arg source argument
 * \param  p_result encoded result
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean decode_io_address(const tStrComp *p_arg, Word *p_result)
{
  tEvalResult eval_result;

  *p_result = EvalStrIntExpressionWithResult(p_arg, UInt4, &eval_result);
  if (eval_result.OK)
    ChkSpace(SegIO, eval_result.AddrSpaceMask);
  return eval_result.OK;
}

/*!------------------------------------------------------------------------
 * \fn     decode_indirect_modifier(const tStrComp *p_arg, Word *p_result)
 * \brief  decode auto-inc modifier (-4...+4)
 * \param  p_arg source argument
 * \param  p_result encoded result (0..8)
 * ------------------------------------------------------------------------ */

static Boolean decode_indirect_modifier(const tStrComp *p_arg, Word *p_result)
{
  tEvalResult eval_result;
  ShortInt dist;

  dist = EvalStrIntExpressionWithResult(p_arg, SInt4, &eval_result);
  if (!eval_result.OK)
    return False;

  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
   && !ChkRangePos(dist, -4, +4, p_arg))
    return False;

  if (!dist)
    *p_result = MODIFIER_0;
  else if (dist > 0)
    *p_result = (dist - 1) & 3;
  else
    *p_result = (-dist + 3) & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     decode_1_256(const tStrComp *p_arg, int offset, Word *p_result)
 * \brief  decode argument in range 1..256
 * \param  p_arg source argument
 * \param  p_result encoded result
 * ------------------------------------------------------------------------ */

static Boolean decode_1_256(const tStrComp *p_arg, int offset, Word *p_result)
{
  tEvalResult eval_result;

  *p_result = EvalStrIntExpressionOffsWithResult(p_arg, offset, UInt9, &eval_result);
  if (!eval_result.OK)
    return False;

  if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
   && !ChkRangePos(*p_result, 1, 256, p_arg))
    return False;

  *p_result = (*p_result - 1) & 0xff;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     reset_adr_vals(adr_vals_t *p_vals)
 * \brief  reset/clear decoded address expression
 * \param  p_vals buffer to reset
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = ModNone;
  p_vals->value = 0;
}

/*!------------------------------------------------------------------------
 * \fn     decode_adr(tStrComp *p_arg, tSymbolSize op_size, unsigned mode_mask)
 * \brief  decode address expression
 * \param  p_arg source argument
 * \param  op_size operand size (8/16 bit)
 * \param  mode_mask bit mask of allowed modes
 * \return register evaluation result: pattern match if eIsReg
 * ------------------------------------------------------------------------ */

static tRegEvalResult is_auto_increment(const tStrComp *p_arg, size_t arg_len, Word *p_result, const char *p_suffix, size_t suffix_len)
{
  String reg;
  tStrComp reg_comp;

  /* matches pattern at all? */

  if ((arg_len < 3 + suffix_len)
   || (p_arg->str.p_str[0] != '(')
   || (p_arg->str.p_str[arg_len - suffix_len - 1] != ')')
   || strcmp(&p_arg->str.p_str[arg_len - suffix_len], p_suffix))
    return eIsNoReg;

  /* if yes, see if part in () is a register */

  StrCompMkTemp(&reg_comp, reg, sizeof(reg));
  StrCompCopySub(&reg_comp, p_arg, 1, arg_len - (2 + suffix_len));
  KillPrefBlanksStrComp(&reg_comp);
  KillPostBlanksStrComp(&reg_comp);
  return decode_reg(&reg_comp, p_result, False);
}

/*!------------------------------------------------------------------------
 * \fn     chk_auto_increment(const tStrComp *p_arg, tSymbolSize op_size, adr_vals_t *p_result)
 * \brief  iterate over all auto-increment/decrement modes
 * \param  p_arg source argument
 * \param  op_size operand size used in instruction
 * \param  p_result returns decoded addressing mode if match
 * \return eIsReg: match, eIsNoReg: no match, eRegAbort -> error during decoding
 * ------------------------------------------------------------------------ */

static tRegEvalResult chk_auto_increment(const tStrComp *p_arg, tSymbolSize op_size, adr_vals_t *p_result)
{
  /* The addressing modes are sorted in the packed strings
     according to the modifier value (0..8): */

  static const char suffixes_8[] =
  {
    '+', '\0',
    '+', '+', '\0',
    '+', '+', '+', '\0',
    '+', '+', '+', '+', '\0',
    '-', '\0',
    '-', '-', '\0',
    '-', '-', '-', '\0',
    '-', '-', '-', '-', '\0',
    '\0'
  },
  suffixes_16[] =
  {
    '\'', '\0',
    '+', '\0',
    '+', '\'', '\0',
    '+', '+', '\0',
    '~', '\0',
    '-', '\0',
    '-', '~', '\0',
    '-', '-', '\0',
    '\0'
  };
  size_t arg_len = strlen(p_arg->str.p_str), suffix_len;
  const char *p_suffix = (op_size == eSymbolSize16Bit) ? suffixes_16 : suffixes_8;
  Word reg;

  p_result->value = 0;
  do
  {
    suffix_len = strlen(p_suffix);

    switch (is_auto_increment(p_arg, arg_len, &reg, p_suffix, suffix_len))
    {
      case eRegAbort:
        return eRegAbort;
      case eIsReg:
        p_result->value |= reg << 4;
        p_result->mode = ModIReg;
        return eIsReg;
      default:
        p_result->value++;
        p_suffix += suffix_len + 1;
    }
  }
  while (suffix_len > 0);
  return eIsNoReg;
}

static adr_mode_t decode_adr(tStrComp *p_arg, tSymbolSize op_size, unsigned mode_mask, adr_vals_t *p_result)
{
  reset_adr_vals(p_result);

  /* Rn */

  switch (decode_reg(p_arg, &p_result->value, False))
  {
    case eIsReg:
      p_result->mode = ModReg;
      goto check_exit;
    case eRegAbort:
      return p_result->mode;
    default:
      break;
  }

  /* #imm */

  if (*p_arg->str.p_str == '#')
  {
    tStrComp imm_arg;
    Boolean ok;

    StrCompRefRight(&imm_arg, p_arg, 1);
    switch (op_size)
    {
      case eSymbolSize16Bit:
        p_result->value = EvalStrIntExpression(&imm_arg, Int16, &ok);
        break;
      case eSymbolSize8Bit:
        if (mode_mask & MModImmP1)
          ok = decode_1_256(&imm_arg, 0, &p_result->value);
        else
          p_result->value = EvalStrIntExpression(&imm_arg, Int8, &ok) & 0xff;
        break;
      default:
        WrStrErrorPos(ErrNum_InvOpSize, p_arg);
        ok = False;
    }
    if (ok)
      p_result->mode = ModImm;
    goto check_exit;
  }

  /* Indirect with modifier: */

  switch (chk_auto_increment(p_arg, op_size, p_result))
  {
    case eRegAbort:
      return p_result->mode;
    case eIsReg:
      goto check_exit;
    default:
      break;
  }

  /* -> direct address, either 0,2,4 in memory or I/O address */

  if (mode_mask & MModIO)
  {
    if (decode_io_address(p_arg, &p_result->value))
      p_result->mode = ModIO;
  }
  else
  {
    if (decode_direct_word_address(p_arg, &p_result->value))
      p_result->mode = ModDir;
  }

check_exit:
  if ((p_result->mode != ModNone) && !((mode_mask >> p_result->mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    reset_adr_vals(p_result);
  }
  return p_result->mode;
}

/*!------------------------------------------------------------------------
 * \fn     strip_indirect(tStrComp *p_dest, tStrComp *p_src)
 * \brief  check whether argument is indirect ( '(...)' ) and strip parentheses if yes
 * \param  p_dest where to put stripped argument if source was indirect
 * \param  p_src argument to check
 * \return True if argument was indirect
 * ------------------------------------------------------------------------ */

static Boolean strip_indirect(tStrComp *p_dest, tStrComp *p_src)
{
  if (IsIndirect(p_src->str.p_str))
  {
    StrCompRefRight(p_dest, p_src, 1);
    StrCompShorten(p_dest, 1);
    KillPrefBlanksStrCompRef(p_dest);
    KillPostBlanksStrComp(p_dest);
    return True;
  }
  else
    return False;
}

/*--------------------------------------------------------------------------*/
/* Decoder Helpers */

static void put_code(Word code)
{
  /* Skip instructions only skip the next halfword of code.  If the
     previous instruction was a skip, and we are about to create an
     instruction consisting of more than one halfword, warn about this.
     Note this only happens for 'macro instructions' like JMP,
     LWI or CALL, which are composed of more than one micro instruction: */

  if ((2 == CodeLen) && last_was_skip)
    WrStrErrorPos(ErrNum_TrySkipMultiwordInstruction, &OpPart);

  WAsmCode[CodeLen / 2] = code;
  CodeLen += 2;
}

/*!------------------------------------------------------------------------
 * \fn     check_bra_dist(LongInt dist, Boolean silent)
 * \brief  check whether branch distance is valid for BRA instruction
 * \param  dist distance to check
 * \param  silent issue error messages if not?
 * \return True if distance is OK
 * ------------------------------------------------------------------------ */

static Boolean check_bra_dist(LongInt dist, Boolean silent)
{
  if ((dist > 256) || (dist < -256))
  {
    if (!silent) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
    return False;
  }
  else if (!dist)
  {
    if (!silent) WrStrErrorPos(ErrNum_JmpDistIsZero, &ArgStr[1]);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     append_bra(LongInt dist)
 * \brief  append BRA machine instruction with given distance
 * \param  dist branch distance (must have been checked before)
 * ------------------------------------------------------------------------ */

static void encode_bra(LongInt dist)
{
  if (dist > 0)
    put_code(0xa000 | ((dist - 1) & 0xff));
  else
    put_code(0xf000 | ((-dist - 1) & 0xff));
}

/*!------------------------------------------------------------------------
 * \fn     put_bra(const tStrComp *p_arg, int pc_offset)
 * \brief  append a relative branch to generated code
 * \param  p_arg source argument of branch target address
 * \param  pc_offset assumed PC offset to beginning of this instruction
 * ------------------------------------------------------------------------ */

static Boolean put_bra(const tStrComp *p_arg, int pc_offset)
{
  Word address;
  tEvalResult eval_result;

  if (decode_code_address(p_arg, &address, &eval_result))
  {
    LongInt dist = address - (EProgCounter() + pc_offset);

    if (!mFirstPassUnknownOrQuestionable(eval_result.Flags)
     && !check_bra_dist(dist, False))
        return False;

    encode_bra(dist);
    return True;
  }
  else
    return False;
}


/*--------------------------------------------------------------------------*/
/* Instruction handlers/decoders */

/*!------------------------------------------------------------------------
 * \fn     decode_fixed(Word Code)
 * \brief  handle instructions without argument
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_fixed(Word code)
{
  if (ChkArgCnt(0, 0))
    put_code(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_basic_jump(Word Code)
 * \brief  handle fundamental jumps (opcode group C)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_basic_jump(Word code)
{
  unsigned arg_cnt = Hi(code) & 0x0f;
  Word data_reg, mask_reg = 0;

  if (ChkArgCnt(arg_cnt, arg_cnt)
   && decode_reg(&ArgStr[1], &data_reg, True)
   && ((arg_cnt < 2) || decode_reg(&ArgStr[2], &mask_reg, True)))
  {
    put_code((data_reg << 8) | (mask_reg << 4) | (code & 0xf00f));
    this_was_skip = True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_basic_arith(Word Code)
 * \brief  handle fundamental arithmetic ops (opcode group 0)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_basic_arith(Word code)
{
  unsigned min_arg_cnt = Hi(code) & 0x0f;
  Word reg2;

  if (ChkArgCnt(min_arg_cnt, 2)
   && decode_reg(&ArgStr[1], &reg2, True))
  {
    Word reg1 = reg2;

    if ((ArgCnt == 1) || decode_reg(&ArgStr[2], &reg1, True))

    put_code((reg2 << 8) | (reg1 << 4) | (code & 0xf00f));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_add_sub(Word Code)
 * \brief  handle ADD/SUB instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_add_sub(Word code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&ArgStr[1], eSymbolSize8Bit, MModReg, &dest_adr_vals))
    {
      case ModReg:
        switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModReg | MModImm | MModImmP1, &src_adr_vals))
        {
          case ModReg:
            put_code(0x0000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 4) | (code & 0x000f));
            break;
          case ModImm:
            put_code((code & 0xf000) | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_and_or(Word Code)
 * \brief  handle AND/OR instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_and_or(Word code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&ArgStr[1], eSymbolSize8Bit, MModReg, &dest_adr_vals))
    {
      case ModReg:
        switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModReg | MModImm, &src_adr_vals))
        {
          case ModReg:
            put_code(0x0000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 4) | (code & 0x000f));
            break;
          case ModImm:
            /* AND Rn, #nn is CLRI Rn with complement of argument */
            if ((code &= 0xf000) == 0x9000) src_adr_vals.value^= 0xff;
            put_code(code | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_basic_io(Word Code)
 * \brief  handle fundamental arithmetic ops with I/O address (opcode group 0)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_basic_io(Word code)
{
  Word reg1, address;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[2], &reg1, True)
   && decode_io_address(&ArgStr[1], &address))
    put_code(0x0000 | (address << 8) | (reg1 << 4) | (code & 0x0f));
}

/*!------------------------------------------------------------------------
 * \fn     decode_shift_rotate(Word code)
 * \brief  handle basic shift & rotate instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_shift_rotate(Word code)
{
  Word reg1;

  if (ChkArgCnt(1, 1)
   && decode_reg(&ArgStr[1], &reg1, True))
    put_code(code | (reg1 << 4));
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem_direct(Word code)
 * \brief  handle direct memory addressing instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mem_direct(Word code)
{
  Word reg1, da;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg1, True)
   && decode_direct_word_address(&ArgStr[2], &da))
    put_code(code | (reg1 << 8) | (da & 0xff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_mem_indirect(Word code)
 * \brief  handle indirect memory addressing instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_mem_indirect(Word code)
{
  Word reg1, reg2, modifier = MODIFIER_0;

  if (ChkArgCnt(2, 3)
   && decode_reg(&ArgStr[1], &reg1, True)
   && decode_reg(&ArgStr[2], &reg2, True)
   && ((ArgCnt < 3) || decode_indirect_modifier(&ArgStr[3], &modifier)))
    put_code(code | (reg1 << 8) | (reg2 << 4) | modifier);
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_imm8(Word code)
 * \brief  handle memory-to-immediate instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_imm8(Word code)
{
  Word reg1;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg1, True))
  {
    Boolean ok;
    Word imm_val = EvalStrIntExpressionOffs(&ArgStr[2], !!(ArgStr[2].str.p_str[0] == '#'), Int8, &ok);

    if (ok)
      put_code(code | (reg1 << 8) | (imm_val & 0xff));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_reg_imm8p1(Word code)
 * \brief  handle memory-to-immediate-plus-one instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_reg_imm8p1(Word code)
{
  Word reg1, imm_val;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg1, True)
   && decode_1_256(&ArgStr[2], !!(ArgStr[2].str.p_str[0] == '#'), &imm_val))
    put_code(code | (reg1 << 8) | (imm_val & 0xff));
}

/*!------------------------------------------------------------------------
 * \fn     decode_lwi(Word code)
 * \brief  handle LWI instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_lwi(Word code)
{
  Word reg1;

  UNUSED(code);

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg1, True))
  {
    Boolean ok;
    Word imm_val = EvalStrIntExpressionOffs(&ArgStr[2], !!(ArgStr[2].str.p_str[0] == '#'), Int16, &ok);

    if (ok)
    {
      put_code(0xd001 | (reg1 << 8));
      put_code(imm_val);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ctl(Word code)
 * \brief  handle CTL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ctl(Word code)
{
  Word address;

  if (ChkArgCnt(2, 2)
    && decode_io_address(&ArgStr[1], &address))
  {
    Boolean ok;
    Word command = EvalStrIntExpressionOffs(&ArgStr[2], !!(ArgStr[2].str.p_str[0] == '#'), Int8, &ok);

    if (ok)
      put_code(code | (address << 8) | (command & 0xff));
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_io_transfer(Word code)
 * \brief  handle I/O transfer instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_io_transfer(Word code)
{
  Word address, reg1, modifier = MODIFIER_0;

  if (ChkArgCnt(2, 3)
   && decode_io_address(&ArgStr[1], &address)
   && decode_reg(&ArgStr[2], &reg1, True)
   && ((ArgCnt < 3) || decode_indirect_modifier(&ArgStr[3], &modifier)))
    put_code(code | (address << 8) | (reg1 << 4) | modifier);
}

/*!------------------------------------------------------------------------
 * \fn     decode_getrb(Word code)
 * \brief  handle GETRB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_getrb(Word code)
{
  Word address, reg1;

  if (ChkArgCnt(2, 2)
   && decode_io_address(&ArgStr[1], &address)
   && decode_reg(&ArgStr[2], &reg1, True))
    put_code(code | (address << 8) | (reg1 << 4));
}

/*!------------------------------------------------------------------------
 * \fn     decode_stat(Word code)
 * \brief  handle STAT instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_stat(Word code)
{
  Word address, reg1;

  if (ChkArgCnt(2, 2)
   && decode_io_address(&ArgStr[2], &address)
   && decode_reg(&ArgStr[1], &reg1, True))
    put_code(code | (address << 8) | (reg1 << 4));
}

/*!------------------------------------------------------------------------
 * \fn     decode_getb(Word code)
 * \brief  handle GETB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_getb(Word code)
{
  UNUSED(code);

  /* The original IBM syntax is GETB ioaddr,Rn[,mod]
     Corti unified all instructions to have the destination be the first argument: */

  switch (ArgCnt)
  {
    case 3: /* only IBM syntax with three args: GETB ioaddr,Rn,mod */
      decode_io_transfer(0xe000);
      break;
    case 2:
    {
      adr_vals_t dest_adr_vals, src_adr_vals;

      switch (decode_adr(&ArgStr[1], eSymbolSize8Bit, MModIO | MModReg | MModIReg, &dest_adr_vals))
      {
        case ModIO: /* GETB ioaddr,... */
          switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModReg, &src_adr_vals))
          {
            case ModReg:
              put_code(0xe000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 4) | MODIFIER_0);
              break;
            default:
              break;
          }
          break;
        case ModReg: /* GETB Rn,... */
          switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModIO, &src_adr_vals))
          {
            case ModIO:
              put_code(0x000e | (src_adr_vals.value << 8) | (dest_adr_vals.value << 4));
              break;
            default:
              break;
          }
          break;
        case ModIReg: /* GETB (Rn)...,... */
          switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModIO, &src_adr_vals))
          {
            case ModIO:
              put_code(0xe000 | (src_adr_vals.value << 8) | (dest_adr_vals.value << 0));
              break;
            default:
              break;
          }
          break;
        default:
          break;
      }
      break;
    }
    default:
      (void)ChkArgCnt(2, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_putb(Word code)
 * \brief  handle PUTB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_putb(Word code)
{
  UNUSED(code);

  /* The original IBM syntax is PUTB ioaddr,Rn[,mod] with 2 or 3 arguments
     'Corti' syntax puts modifier into second argument: */

  switch (ArgCnt)
  {
    case 3: /* only IBM syntax with three args: PUTB ioaddr,Rn[,mod] */
      decode_io_transfer(0x4000);
      break;
    case 2:
    {
      Word address;
      adr_vals_t src_adr_vals;

      if (decode_io_address(&ArgStr[1], &address))
        switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModReg | MModIReg, &src_adr_vals))
        {
          case ModReg:
            put_code(0x4000 | (address << 8) | (src_adr_vals.value << 4) | MODIFIER_0);
            break;
          case ModIReg:
            put_code(0x4000 | (address << 8) | (src_adr_vals.value << 0));
            break;
          default:
            break;
        }
      break;
    }
    default:
      (void)ChkArgCnt(2, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_getadd(Word Code)
 * \brief  handle GETADD instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_getadd(Word code)
{
  Word reg, address;

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[1], &reg, True)
   && decode_io_address(&ArgStr[2], &address))
    put_code(0x0000 | (address << 8) | (reg << 4) | (code & 0x0f));
}

/*!------------------------------------------------------------------------
 * \fn     decode_move(Word Code)
 * \brief  handle MOVE instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_move(Word code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&ArgStr[1], eSymbolSize16Bit, MModDir | MModReg | MModIReg, &dest_adr_vals))
    {
      case ModReg: /* MOVE Rn,... */
        switch (decode_adr(&ArgStr[2], eSymbolSize16Bit, MModDir | MModReg | MModIReg | MModImm, &src_adr_vals))
        {
          case ModReg:
            put_code(0x0004 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 4));
            break;
          case ModDir:
            put_code(0x2000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          case ModIReg:
            put_code(0xd000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          case ModImm:
            put_code(0xd001 | (dest_adr_vals.value << 8));
            put_code(src_adr_vals.value);
            break;
          default:
            break;
        }
        break;
      case ModIReg: /* MOVE (Rn),... */
        switch (decode_adr(&ArgStr[2], eSymbolSize16Bit, MModReg, &src_adr_vals))
        {
          case ModReg:
            put_code(0x5000 | (src_adr_vals.value << 8) | (dest_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      case ModDir: /* MOVE addr,... */
        switch (decode_adr(&ArgStr[2], eSymbolSize16Bit, MModReg, &src_adr_vals))
        {
          case ModReg:
            put_code(0x3000 | (src_adr_vals.value << 8) | (dest_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_movb(Word Code)
 * \brief  handle MOVB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_movb(Word code)
{
  adr_vals_t dest_adr_vals, src_adr_vals;

  UNUSED(code);

  if (ChkArgCnt(2, 2))
    switch (decode_adr(&ArgStr[1], eSymbolSize8Bit, MModReg | MModIReg, &dest_adr_vals))
    {
      case ModReg: /* MOVB Rn,... */
        switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModIReg | MModImm, &src_adr_vals))
        {
          case ModIReg:
            put_code(0x6000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          case ModImm:
            put_code(0x8000 | (dest_adr_vals.value << 8) | (src_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      case ModIReg: /* MOVB (Rn)...,... */
        switch (decode_adr(&ArgStr[2], eSymbolSize8Bit, MModReg, &src_adr_vals))
        {
          case ModReg:
            put_code(0x7000 | (src_adr_vals.value << 8) | (dest_adr_vals.value << 0));
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     decode_bra(Word Code)
 * \brief  handle BRA instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_bra(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(1, 1))
    put_bra(&ArgStr[1], 2);
}

/*!------------------------------------------------------------------------
 * \fn     decode_jmp(Word Code)
 * \brief  handle JMP instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_jmp(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(1, 1))
  {
    tStrComp ind_comp;
    Word addr_or_reg;
    adr_vals_t adr_vals;

    /* Extra handling for auto-increment/deceement since it did not fit
       in here easily otherwise: */

    switch (chk_auto_increment(&ArgStr[1], eSymbolSize16Bit, &adr_vals))
    {
      case eRegAbort:
        return;
      case eIsReg: /* JMP (Rn)*** -> LDHI PC,Rn,*** */
        put_code(0xd000 | adr_vals.value);
        return;
      default:
        break;
    }

    if (strip_indirect(&ind_comp, &ArgStr[1]))
    {
      switch (decode_adr(&ind_comp, eSymbolSize16Bit, MModDir | MModReg, &adr_vals))
      {
        case ModDir: /* JMP (addr) -> LDHD PC, addr */
          put_code(0x2000 | adr_vals.value);
          break;
        case ModReg: /* JMP (Rn) -> LDHI PC, Rn */
          put_code(0xd008 | (adr_vals.value << 4));
          break;
        default:
          break;
      }
    }

    else switch (decode_reg(&ArgStr[1], &addr_or_reg, False))
    {
      case eIsNoReg:
      {
        tEvalResult eval_result;
        Boolean force_long = !!(*ArgStr[1].str.p_str == '>');
        tStrComp addr_comp;

        StrCompRefRight(&addr_comp, &ArgStr[1], force_long);
        if (decode_code_address(&addr_comp, &addr_or_reg, &eval_result))
        {
          LongInt dist = addr_or_reg - (EProgCounter() + 2);

          if (check_bra_dist(dist, True) && !force_long)
            /* JMP addr -> BRA addr */
            encode_bra(dist);

          else
          {
            /* JMP addr -> LDHI PC,PC,2 ; DW addr */
            put_code(0xd001);
            put_code(addr_or_reg);
          }
        }
        break;
      }
      case eIsReg:
        /* JMP Rn -> MOVE PC, Rn */
        put_code(0x0004 | (addr_or_reg << 4));
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_call(Word Code)
 * \brief  handle CALL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_call(Word code)
{
  Word link_reg;

  UNUSED(code);

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[2], &link_reg, True))
  {
    tStrComp ind_comp;
    Word addr_or_reg;

    if (strip_indirect(&ind_comp, &ArgStr[1]))
    {
      adr_vals_t adr_vals;

      switch (decode_adr(&ind_comp, eSymbolSize16Bit, MModReg, &adr_vals))
      {
        case ModReg: /* CALL (Rn),Rl -> MVP2 Rl,PC ; LDHI PC, Rn */
          put_code(0x0003 | (link_reg << 8));
          put_code(0xd008 | (adr_vals.value << 4));
          break;
        default:
          break;
      }
    }

    else switch (decode_reg(&ArgStr[1], &addr_or_reg, False))
    {
      case eIsNoReg:
      {
        tEvalResult eval_result;

        if (decode_code_address(&ArgStr[1], &addr_or_reg, &eval_result))
        {
          /* CALL <addr>,Rl -> MVP2 Rl,PC ; LDHI PC,Rl,2 ; DW <addr> */
          put_code(0x0003 | (link_reg << 8));
          put_code(0xd001 | (link_reg << 4));
          put_code(addr_or_reg);
        }
        break;
      }
      case eIsReg:
        /* CALL Rn,Rl -> MVP2 Rl,PC ; MOVE PC,Rn */
        put_code(0x0003 | (link_reg << 8));
        put_code(0x0004 | (addr_or_reg << 4));
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_rcall(Word Code)
 * \brief  handle RCALL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_rcall(Word code)
{
  Word link_reg;

  UNUSED(code);

  if (ChkArgCnt(2, 2)
   && decode_reg(&ArgStr[2], &link_reg, True))
  {
    /* RCALL <addr>,Rl -> MVP2 Rl,PC ; BRA <addr> */
    put_code(0x0003 | (link_reg << 8));
    if (!put_bra(&ArgStr[1], 4))
      CodeLen = 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ret(Word Code)
 * \brief  handle RET instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_ret(Word code)
{
  Word link_reg;

  UNUSED(code);

  if (ChkArgCnt(1, 1)
   && decode_reg(&ArgStr[1], &link_reg, True))
    put_code(0x0004 | (link_reg << 4));
}

/*!------------------------------------------------------------------------
 * \fn     decode_pseudo(void)
 * \brief  handle pseudo instructions
 * \return True if handled
 * ------------------------------------------------------------------------ */

static Boolean decode_pseudo(void)
{
  if (Memo("REG"))
  {
    CodeREG(0);
    return True;
  }

  if (Memo("PORT"))
  {
    CodeEquate(SegIO, 0, SegLimits[SegIO]);
    return True;
  }

  return False;
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static void init_fields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "NOP"  , NOPCode, decode_fixed); /* C */
  AddInstTable(InstTable, "HALT" , 0x0000 , decode_fixed); /* C */

  AddInstTable(InstTable, "JLE"  , 0xc200, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JLO"  , 0xc201, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JEQ"  , 0xc202, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JNO"  , 0xc103, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JALL" , 0xc204, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JALLM", 0xc205, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JNOM" , 0xc206, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JHAM" , 0xc207, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JHI"  , 0xc208, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JHE"  , 0xc209, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JHL"  , 0xc20a, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JSB"  , 0xc10b, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JSN"  , 0xc20c, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JSNM" , 0xc20d, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JSM"  , 0xc20e, decode_basic_jump); /* I */
  AddInstTable(InstTable, "JHSNM", 0xc20f, decode_basic_jump); /* I */

  AddInstTable(InstTable, "SLE"  , 0xc200, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SLT"  , 0xc201, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SE"   , 0xc202, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SZ"   , 0xc103, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SS"   , 0xc104, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SBS"  , 0xc205, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SBC"  , 0xc206, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SBSH" , 0xc207, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SGT"  , 0xc208, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SGE"  , 0xc209, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNE"  , 0xc20a, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNZ"  , 0xc10b, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNS"  , 0xc10c, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNBS" , 0xc20d, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNBC" , 0xc20e, decode_basic_jump); /* C */
  AddInstTable(InstTable, "SNBSH", 0xc20f, decode_basic_jump); /* C */

  AddInstTable(InstTable, "MVM2" , 0x0200, decode_basic_arith); /* I */
  AddInstTable(InstTable, "MVM1" , 0x0201, decode_basic_arith); /* I */
  AddInstTable(InstTable, "MVP1" , 0x0202, decode_basic_arith); /* I */
  AddInstTable(InstTable, "MVP2" , 0x0203, decode_basic_arith); /* I */
  AddInstTable(InstTable, "MOVE" , 0x0000, decode_move); /* I/C */
  AddInstTable(InstTable, "AND"  , 0x9005, decode_and_or); /* I/A */
  AddInstTable(InstTable, "OR"   , 0xb006, decode_and_or); /* I/A */
  AddInstTable(InstTable, "ORB"  , 0xb006, decode_and_or); /* I/A */
  AddInstTable(InstTable, "XOR"  , 0x0207, decode_basic_arith); /* I */
  AddInstTable(InstTable, "ADD"  , 0xa008, decode_add_sub); /* I/C */
  AddInstTable(InstTable, "SUB"  , 0xf009, decode_add_sub); /* I/C */
  AddInstTable(InstTable, "ADDS1", 0x020a, decode_basic_arith); /* I */
  AddInstTable(InstTable, "ADDS2", 0x020b, decode_basic_arith); /* I */
  AddInstTable(InstTable, "HTL"  , 0x020c, decode_basic_arith); /* I */
  AddInstTable(InstTable, "LTH"  , 0x020d, decode_basic_arith); /* I */

  AddInstTable(InstTable, "DEC2" , 0x0100, decode_basic_arith); /* C */
  AddInstTable(InstTable, "DEC"  , 0x0101, decode_basic_arith); /* C */
  AddInstTable(InstTable, "INC"  , 0x0102, decode_basic_arith); /* C */
  AddInstTable(InstTable, "INC2" , 0x0103, decode_basic_arith); /* C */
  AddInstTable(InstTable, "ADDH" , 0x020a, decode_basic_arith); /* C */
  AddInstTable(InstTable, "ADDH2", 0x020b, decode_basic_arith); /* C */
  AddInstTable(InstTable, "MHL"  , 0x020c, decode_basic_arith); /* C */
  AddInstTable(InstTable, "MLH"  , 0x020d, decode_basic_arith); /* C */

  AddInstTable(InstTable, "GETR" , 0x000e, decode_basic_io); /* I */
  AddInstTable(InstTable, "GETA" , 0x000f, decode_basic_io); /* I */
  AddInstTable(InstTable, "GETADD", 0x000f, decode_getadd); /* C */

  AddInstTable(InstTable, "SHFTR", 0xe00c, decode_shift_rotate); /* I */
  AddInstTable(InstTable, "ROTR" , 0xe00d, decode_shift_rotate); /* I */
  AddInstTable(InstTable, "SRR3" , 0xe00e, decode_shift_rotate); /* I */
  AddInstTable(InstTable, "SRR4" , 0xe00f, decode_shift_rotate); /* I */
  AddInstTable(InstTable, "SHR"  , 0xe00c, decode_shift_rotate); /* C */
  AddInstTable(InstTable, "ROR"  , 0xe00d, decode_shift_rotate); /* C */
  AddInstTable(InstTable, "ROR3" , 0xe00e, decode_shift_rotate); /* C */
  AddInstTable(InstTable, "SWAP" , 0xe00f, decode_shift_rotate); /* C */

  AddInstTable(InstTable, "LDHD" , 0x2000, decode_mem_direct); /* I */
  AddInstTable(InstTable, "STHD" , 0x3000, decode_mem_direct); /* I */
  AddInstTable(InstTable, "LDHI" , 0xd000, decode_mem_indirect); /* I */
  AddInstTable(InstTable, "STHI" , 0x5000, decode_mem_indirect); /* I */
  AddInstTable(InstTable, "LDBI" , 0x6000, decode_mem_indirect); /* I */
  AddInstTable(InstTable, "STBI" , 0x7000, decode_mem_indirect); /* I */

  AddInstTable(InstTable, "MOVB" , 0, decode_movb); /* C */
  AddInstTable(InstTable, "LWI"  , 0, decode_lwi); /* C */

  AddInstTable(InstTable, "EMIT" , 0x8000, decode_reg_imm8); /* I */
  AddInstTable(InstTable, "CLRI" , 0x9000, decode_reg_imm8); /* I */
  AddInstTable(InstTable, "CLR"  , 0x9000, decode_reg_imm8); /* C */
  AddInstTable(InstTable, "SETI" , 0xb000, decode_reg_imm8); /* I */
  AddInstTable(InstTable, "SET"  , 0xb000, decode_reg_imm8); /* C */
  AddInstTable(InstTable, "LBI"  , 0x8000, decode_reg_imm8); /* C */
  AddInstTable(InstTable, "ADDI" , 0xa000, decode_reg_imm8p1); /* I */
  AddInstTable(InstTable, "SUBI" , 0xf000, decode_reg_imm8p1); /* I */

  AddInstTable(InstTable, "CTL"  , 0x1000, decode_ctl); /* I */
  AddInstTable(InstTable, "CTRL" , 0x1000, decode_ctl); /* C */

  AddInstTable(InstTable, "PUTB" , 0, decode_putb); /* I/C */
  AddInstTable(InstTable, "GETB" , 0, decode_getb); /* I/C */
  AddInstTable(InstTable, "GETRB", 0xe00f, decode_getrb); /* I */
  AddInstTable(InstTable, "STAT" , 0xe00f, decode_stat); /* C */
  AddInstTable(InstTable, "BRA"  , 0, decode_bra); /* C */
  AddInstTable(InstTable, "JMP"  , 0, decode_jmp); /* C */
  AddInstTable(InstTable, "CALL" , 0, decode_call); /* C */
  AddInstTable(InstTable, "RCALL" , 0, decode_rcall); /* C */
  AddInstTable(InstTable, "RET"  , 0, decode_ret); /* C */
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
 * \fn     intern_symbol_palm(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for PDP-11
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void intern_symbol_palm(char *p_arg, TempResult *p_result)
{
  Word reg_num;

  if (decode_reg_core(p_arg, &reg_num, &p_result->DataSize))
  {
    p_result->Typ = TempReg;
    p_result->Contents.RegDescr.Reg = reg_num;
    p_result->Contents.RegDescr.Dissect = dissect_reg_palm;
    p_result->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     make_code_palm(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void make_code_palm(void)
{
  CodeLen = 0; DontPrint = False;
  this_was_skip = False;

  /* to be ignored */

  if (Memo(""))
  {
    this_was_skip = last_was_skip;
    goto func_exit;
  }

  /* Pseudo Instructions */

  if (decode_pseudo())
  {
    this_was_skip = last_was_skip;
    goto func_exit;
  }
  if (DecodeIntelPseudo(True))
    return;

  /* machine instructions may not begin on odd addresses */

  if (Odd(EProgCounter()))
  {
    if (DoPadding)
      InsertPadding(1, False);
    else
      WrError(ErrNum_AddrNotAligned);
  }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
func_exit:
  last_was_skip = this_was_skip;
}

/*!------------------------------------------------------------------------
 * \fn     is_def_palm(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean is_def_palm(void)
{
  return Memo("REG")
      || Memo("PORT");
}

/*!------------------------------------------------------------------------
 * \fn     qualify_quote_palm(const char *p_start, const char *p_quote_pos)
 * \brief  Treat special case of (Rn)' and (Rn)+' which is no quoting
 * \param  p_start start of string
 * \param  p_quote_pos position of quote
 * \return False if it's no quoting
 * ------------------------------------------------------------------------ */

static Boolean qualify_quote_palm(const char *p_start, const char *p_quote_pos)
{
  if (*p_quote_pos == '\'')
  {
    if ((p_quote_pos >= p_start + 1)
     && (*(p_quote_pos - 1) == ')'))
      return False;
    if ((p_quote_pos >= p_start + 2)
     && (*(p_quote_pos - 1) == '+')
     && (*(p_quote_pos - 2) == ')'))
      return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     switch_from_palm(void)
 * \brief  deinitialize as target
 * ------------------------------------------------------------------------ */

static void switch_from_palm(void)
{
  deinit_fields();
}

static Boolean true_fnc(void)
{
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_palm(void)
 * \brief  prepare to assemble code for this target
 * ------------------------------------------------------------------------ */

static void switch_to_palm(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("PALM");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIBM);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0x0004; /* = MOVE R0,R0 */
  DivideChars = ",";

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 2;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  Grans[SegIO] = 1;
  ListGrans[SegIO] = 1;
  SegInits[SegIO] = 0;
  SegLimits[SegIO] = 0xf;

  MakeCode = make_code_palm;
  IsDef = is_def_palm;
  SwitchFrom = switch_from_palm;
  QualifyQuote = qualify_quote_palm;
  InternSymbol = intern_symbol_palm;
  DissectReg = dissect_reg_palm;
  SetIsOccupiedFnc = true_fnc;

  AddONOFF(DoPaddingName, &DoPadding, DoPaddingName, False);
  init_fields();
  last_was_skip = False;
}

/*!------------------------------------------------------------------------
 * \fn     codepalm_init(void)
 * \brief  register PALM target
 * ------------------------------------------------------------------------ */

void codepalm_init(void)
{
  cpu_5100 = AddCPU("IBM5100", switch_to_palm);
  cpu_5110 = AddCPU("IBM5110", switch_to_palm);
  cpu_5120 = AddCPU("IBM5120", switch_to_palm);
}
