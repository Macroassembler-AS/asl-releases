/* codeko09.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator Konami 052001                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codevars.h"
#include "headids.h"

#include "motpseudo.h"
#include "code6809.h"
#include "codeko09.h"

typedef enum
{
  e_adr_mode_none = -1,
  e_adr_mode_imm = 1,
  e_adr_mode_ind = 2
} adr_mode_t;

#define adr_mode_mask_imm (1 << e_adr_mode_imm)
#define adr_mode_mask_ind (1 << e_adr_mode_ind)
#define adr_mode_mask_no_imm (adr_mode_mask_ind)
#define adr_mode_mask_all (adr_mode_mask_imm | adr_mode_mask_no_imm)

typedef struct
{
  adr_mode_t mode;
  int cnt;
  Byte vals[5];
} adr_vals_t;

static const char reg_16_names[4] = { 'X','Y','U','S' };
static LongInt DPRValue;

/*!------------------------------------------------------------------------
 * \fn     decode_cpu_reg(const char *p_asc, Byte *p_ret)
 * \brief  decode CPU register names for TFR/EXG
 * \param  p_asc source argument
 * \param  p_ret resulting encoded value
 * \return True if known register
 * ------------------------------------------------------------------------ */

static Boolean decode_cpu_reg(const char *p_asc, Byte *p_ret)
{
#define reg_cnt as_array_size(reg_names)
  static const char reg_names[][2] =
  {
    "X", "Y", "U", "S", "A", "B"
  };
  static const Byte reg_vals[reg_cnt] =
  {
    2  , 3  , 5  , 4   , 0  , 1
  };
  size_t z;

  for (z = 0; z < reg_cnt; z++)
    if (!as_strcasecmp(p_asc, reg_names[z]))
    {
      *p_ret = reg_vals[z];
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(int ArgStartIdx, int ArgEndIdx,
                     tSymbolSize op_size, unsigned mode_mask, adr_vals_t *p_vals)
 * \brief  decode/evaluate address expression
 * \param  ArgStartIdx 1st argument of expression
 * \param  ArgEndIdx last argument of expression
 * \param  op_size operand size (8/16)
 * \param  mode_mask bit mask of allowed modes
 * \param  p_vals dest buffer
 * \return encoded addressing mode
 * ------------------------------------------------------------------------ */

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_adr_mode_none;
  p_vals->cnt = 0;
}

#define IDX_PCREG 7

static Boolean code_reg(const char *p_arg, Byte *p_ret)
{
  if (!as_strcasecmp(p_arg, "PCR") || !as_strcasecmp(p_arg, "PC"))
  {
    *p_ret = IDX_PCREG;
    return True;
  }

  if (strlen(p_arg) != 1)
    return False;
  else
  {
    static const char Regs[6] = "XY\aUS";
    const char *p = strchr(Regs, as_toupper(*p_arg));

    if (!p)
      return False;
    *p_ret = p - Regs + 2;
    return True;
  }
}

static unsigned ChkZero(const char *s, Byte *Erg)
{
  if (*s == '>')
  {
    *Erg = 1;
    return 1;
  }
  else if (*s == '<')
  {
    *Erg = 2;
    return 1;
  }
  else
  {
    *Erg = 0;
    return 0;
  }
}

static Boolean MayShort(Integer Arg)
{
  return ((Arg >= -128) && (Arg < 127));
}

static Boolean IsZeroOrEmpty(const tStrComp *pArg)
{
  Boolean OK;
  LongInt Value;

  if (!*pArg->str.p_str)
    return True;
  Value = EvalStrIntExpression(pArg, Int32, &OK);
  return OK && !Value;
}

static adr_mode_t DecodeAdr(int ArgStartIdx, int ArgEndIdx,
                            tSymbolSize op_size, unsigned mode_mask, adr_vals_t *p_vals)
{
  tStrComp *pStartArg, *pEndArg, IndirComps[2];
  String temp;
  Word AdrWord;
  Boolean IndFlag, OK;
  Byte EReg, ZeroMode;
  char *p;
  unsigned Offset;
  Integer AdrInt;
  int AdrArgCnt = ArgEndIdx - ArgStartIdx + 1, end_arg_len;
  const unsigned OpcodeLen = 1;

  reset_adr_vals(p_vals);
  pStartArg = &ArgStr[ArgStartIdx];
  pEndArg = &ArgStr[ArgEndIdx];

  /* immediate */

  if ((*pStartArg->str.p_str == '#') && (AdrArgCnt == 1))
  {
    switch (op_size)
    {
      case eSymbolSize16Bit:
        AdrWord = EvalStrIntExpressionOffs(pStartArg, 1, Int16, &OK);
        if (OK)
        {
          p_vals->vals[0] = Hi(AdrWord);
          p_vals->vals[1] = Lo(AdrWord);
          p_vals->cnt = 2;
        }
        break;
      case eSymbolSize8Bit:
        p_vals->vals[0] = EvalStrIntExpressionOffs(pStartArg, 1, Int8, &OK);
        if (OK)
          p_vals->cnt = 1;
        break;
      default:
        OK = False;
        break;
    }
    if (OK)
      p_vals->mode = e_adr_mode_imm;
    goto chk_mode;
  }

  /* indirekter Ausdruck ? */

  if ((*pStartArg->str.p_str == '[') && (pStartArg->str.p_str[strlen(pStartArg->str.p_str) - 1] == ']'))
  {
    tStrComp Arg, Remainder;

    IndFlag = True;
    StrCompRefRight(&Arg, pStartArg, 1);
    StrCompShorten(&Arg, 1);
    AdrArgCnt = 0;
    do
    {
      p = QuotPos(Arg.str.p_str, ',');
      if (p)
        StrCompSplitRef(&IndirComps[AdrArgCnt], &Remainder, &Arg, p);
      else
        IndirComps[AdrArgCnt] = Arg;
      KillPrefBlanksStrCompRef(&IndirComps[AdrArgCnt]);
      KillPostBlanksStrComp(&IndirComps[AdrArgCnt]);
      AdrArgCnt++;
      if (p)
        Arg = Remainder;
    }
    while (p && (AdrArgCnt < 2));
    pStartArg = &IndirComps[0];
    pEndArg = &IndirComps[AdrArgCnt - 1];
  }
  else
    IndFlag = False;

  /* Predekrement ? */

  end_arg_len = strlen(pEndArg->str.p_str);
  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (end_arg_len >= 2) && (*pEndArg->str.p_str == '-') && (code_reg(pEndArg->str.p_str + 1, &EReg)))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0x02 + (EReg << 4) + (Ord(IndFlag) << 3);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (end_arg_len >= 3) && (!strncmp(pEndArg->str.p_str, "--", 2)) && (code_reg(pEndArg->str.p_str + 2, &EReg)))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0x03 + (EReg << 4) + (Ord(IndFlag) << 3);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  /* Postinkrement ? */

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (end_arg_len >= 2) && (pEndArg->str.p_str[end_arg_len - 1] == '+'))
  {
    memcpy(temp, pEndArg->str.p_str, end_arg_len - 1);
    temp[end_arg_len - 1] = '\0';
    if (code_reg(temp, &EReg))
    {
      if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
      else
      {
        p_vals->cnt = 1;
        p_vals->vals[0] = 0x00 + (EReg << 4) + (Ord(IndFlag) << 3);
        p_vals->mode = e_adr_mode_ind;
      }
      goto chk_mode;
    }
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (end_arg_len >= 3) && (!strncmp(pEndArg->str.p_str + end_arg_len - 2, "++", 2)))
  {
    memcpy(temp, pEndArg->str.p_str, end_arg_len - 2);
    temp[end_arg_len - 2] = '\0';
    if (code_reg(temp, &EReg))
    {
      if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
      else
      {
        p_vals->cnt = 1;
        p_vals->vals[0] = 0x01 + (EReg << 4) + (Ord(IndFlag) << 3);
        p_vals->mode = e_adr_mode_ind;
      }
      goto chk_mode;
    }
  }

  /* 16-Bit-Register (mit Index) ? */

  if ((AdrArgCnt <= 2) && (AdrArgCnt >= 1) && (code_reg(pEndArg->str.p_str, &EReg)))
  {
    p_vals->vals[0] = (EReg << 4) + (Ord(IndFlag) << 3);

    /* nur 16-Bit-Register */

    if (AdrArgCnt == 1)
    {
      p_vals->cnt = 1;
      p_vals->vals[0] |= 0x06;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* mit Index */

    if (!as_strcasecmp(pStartArg->str.p_str, "A"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] |= 0x80;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if (!as_strcasecmp(pStartArg->str.p_str, "B"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] |= 0x81;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if (!as_strcasecmp(pStartArg->str.p_str, "D"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x87;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* Displacement auswerten */

    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    if (EReg == IDX_PCREG)
      AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, UInt16, &OK)
             - (EProgCounter() + 2 + OpcodeLen);
    else if (ZeroMode > 1)
      AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, SInt8, &OK);
    else
      AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, SInt16, &OK);
    if (!OK)
      goto chk_mode;

    /* Displacement 0 ? */

    if ((ZeroMode == 0) && (AdrInt == 0))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x06;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* 8-Bit-Displacement */

    else if ((ZeroMode == 2) || ((ZeroMode == 0) && (MayShort(AdrInt))))
    {
      if (!MayShort(AdrInt)) WrError(ErrNum_NoShortAddr);
      else
      {
        p_vals->mode = e_adr_mode_ind;
        p_vals->cnt = 2;
        p_vals->vals[0] += 0x04;
        p_vals->vals[1] = Lo(AdrInt);
      }
      goto chk_mode;
    }

    /* 16-Bit-Displacement */

    else
    {
      p_vals->mode = e_adr_mode_ind;
      p_vals->cnt = 3;
      p_vals->vals[0] += 0x05;
      if (EReg == IDX_PCREG)
        AdrInt--;
      p_vals->vals[1] = Hi(AdrInt);
      p_vals->vals[2] = Lo(AdrInt);
      goto chk_mode;
    }
  }

  /* absolute/direct */

  if (AdrArgCnt == 1)
  {
    tSymbolFlags Flags;

    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    AdrInt = EvalStrIntExpressionOffsWithFlags(pStartArg, Offset, UInt16, &OK, &Flags);
    if (mFirstPassUnknown(Flags) && (ZeroMode == 2))
      AdrInt = (AdrInt & 0xff) | (DPRValue << 8);

    if (OK)
    {
      if ((ZeroMode == 2) || ((ZeroMode == 0) && (Hi(AdrInt) == DPRValue)))
      {
        if (Hi(AdrInt) != DPRValue) WrError(ErrNum_NoShortAddr);
        else
        {
          p_vals->mode = e_adr_mode_ind;
          p_vals->cnt = 2;
          p_vals->vals[0] = 0xc4 | (Ord(IndFlag) << 3);
          p_vals->vals[1] = Lo(AdrInt);
        }
      }

      else
      {
        p_vals->mode = e_adr_mode_ind;
        p_vals->cnt = 3;
        p_vals->vals[0] = 0x07 | (Ord(IndFlag) << 3);
        p_vals->vals[1] = Hi(AdrInt);
        p_vals->vals[2] = Lo(AdrInt);
      }
    }
    goto chk_mode;
  }

  if (p_vals->mode == e_adr_mode_none)
    WrError(ErrNum_InvAddrMode);

chk_mode:
  if ((p_vals->mode != e_adr_mode_none) && !((mode_mask >> p_vals->mode) & 1))
  {
    WrError(ErrNum_InvAddrMode);
    reset_adr_vals(p_vals);
  }
  return p_vals->mode;
}

/*!------------------------------------------------------------------------
 * \fn     append_adr_vals(const adr_vals_t *p_vals)
 * \brief  append encoded addressing mode to instruction
 * \param  p_vals what to append
 * ------------------------------------------------------------------------ */

static void append_adr_vals(const adr_vals_t *p_vals)
{
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->cnt);
  CodeLen += p_vals->cnt;
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     decode_inh(Word code)
 * \brief  decode instructions without argument
 * ------------------------------------------------------------------------ */

static void decode_inh(Word code)
{
  if (ChkArgCnt(0, 0))
    BAsmCode[CodeLen++] = Lo(code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_imm_8(Word code)
 * \brief  decode instructions with 8 bit immediate argument
 * ------------------------------------------------------------------------ */

static void decode_imm_8(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(1, ArgCnt, eSymbolSize8Bit, adr_mode_mask_imm, &adr_vals))
    {
      case e_adr_mode_imm:
        BAsmCode[CodeLen++] = code;
        BAsmCode[CodeLen++] = adr_vals.vals[0];
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_idx(Word code)
 * \brief  decode instructions with indexed argument
 * ------------------------------------------------------------------------ */

static void decode_idx(Word code)
{
  if (ChkArgCnt(1, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(1, ArgCnt, eSymbolSizeUnknown, adr_mode_mask_ind, &adr_vals))
    {
      case e_adr_mode_ind:
        BAsmCode[CodeLen++] = code;
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_ari(Word code)
 * \brief  decode instructions with indexed or immediate argument
 * ------------------------------------------------------------------------ */

static void decode_ari(Word code)
{
  if (ChkArgCnt(1, 2))
  {
    adr_vals_t adr_vals;

    switch (DecodeAdr(1, ArgCnt, (code & 0x8000) ? eSymbolSize16Bit : eSymbolSize8Bit, adr_mode_mask_all, &adr_vals))
    {
      case e_adr_mode_imm:
        BAsmCode[CodeLen++] = Lo(code) + 0;
        goto append;
      case e_adr_mode_ind:
        BAsmCode[CodeLen++] = Lo(code) + (Hi(code) & 0x7f);
        goto append;
      append:
        append_adr_vals(&adr_vals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch_8_core(Word code)
 * \brief  decode instructions with 8 bit displacement
 * ------------------------------------------------------------------------ */

static void decode_branch_8_core(unsigned arg_index, Word code)
{
  tEvalResult eval_result;
  LongInt dist = EvalStrIntExpressionWithResult(&ArgStr[arg_index], UInt16, &eval_result)
               - (EProgCounter() + 2);
  if (eval_result.OK)
  {
    if (!RangeCheck(dist, SInt8) && !mSymbolQuestionable(eval_result.Flags)) WrError(ErrNum_JmpDistTooBig);
    else
    {
      BAsmCode[CodeLen++] = code;
      BAsmCode[CodeLen++] = dist & 0xff;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch_8(Word code)
 * \brief  decode instructions with 8 bit displacement
 * ------------------------------------------------------------------------ */

static void decode_branch_8(Word code)
{
  if (ChkArgCnt(1, 1))
    decode_branch_8_core(1, code);
}

/*!------------------------------------------------------------------------
 * \fn     decode_branch_16(Word code)
 * \brief  decode instructions with 16 bit displacement
 * ------------------------------------------------------------------------ */

static void decode_branch_16(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult eval_result;
    LongInt dist = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &eval_result)
                 - (EProgCounter() + 3);
    if (eval_result.OK)
    {
      if (!RangeCheck(dist, SInt16) && !mSymbolQuestionable(eval_result.Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[CodeLen++] = code;
        BAsmCode[CodeLen++] = (dist >> 8) & 0xff;
        BAsmCode[CodeLen++] = dist & 0xff;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     decode_div(Word code)
 * \brief  decode DIV instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_div(Word code)
{
  if (!ChkArgCnt(2, 2)); 
  else if (as_strcasecmp(ArgStr[1].str.p_str, "X")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (as_strcasecmp(ArgStr[2].str.p_str, "B")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_move(Word code)
 * \brief  decode L(MOVE) instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_move(Word code)
{
  if (!ChkArgCnt(3, 3)); 
  else if (as_strcasecmp(ArgStr[1].str.p_str, "Y")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (as_strcasecmp(ArgStr[2].str.p_str, "X")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (as_strcasecmp(ArgStr[3].str.p_str, "U")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
  else
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_bset(Word code)
 * \brief  decode BSETx instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_bset(Word code)
{
  if (!ChkArgCnt(2, 2)); 
  else if (as_strcasecmp(ArgStr[1].str.p_str, "X")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (as_strcasecmp(ArgStr[2].str.p_str, "U")) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else
    BAsmCode[CodeLen++] = code;
}

/*!------------------------------------------------------------------------
 * \fn     decode_exg_tfr(Word code)
 * \brief  decode EXG/TFR instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_exg_tfr(Word code)
{
  DecodeTFR_TFM_EXG_6809(code, False, decode_cpu_reg, True);
}

/*!------------------------------------------------------------------------
 * \fn     decode_dec(Word code)
 * \brief  handle DEC instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void decode_dec(Word code)
{
  switch (ArgCnt)
  {
    case 3:
      if (as_strcasecmp(ArgStr[2].str.p_str, "JNZ")) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "B"))
        decode_branch_8_core(3, code);
      else if (!as_strcasecmp(ArgStr[1].str.p_str, "X"))
        decode_branch_8_core(3, code + 1);
      else
        WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      break;
    case 2:
      decode_idx(0x8e);
      break;
    default:
      (void)ChkArgCnt(2, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     init_fields(void)
 * \brief  create hash table
 * ------------------------------------------------------------------------ */

static void add_acc_idx(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sA", p_name);
  AddInstTable(InstTable, name , code + 0, decode_inh);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name , code + 1, decode_inh);
  AddInstTable(InstTable, p_name, code + 2, decode_idx);
}

static void add_acc_idx_16(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sD", p_name);
  AddInstTable(InstTable, name, code + 0, decode_inh);
  as_snprintf(name, sizeof(name), "%sW", p_name);
  AddInstTable(InstTable, name, code + 1, decode_idx);
}

static void add_reg_16(const char *p_name, Word code, Word code_incr, InstProc proc)
{
  char name[10];
  size_t z;

  for (z = 0; z < as_array_size(reg_16_names); z++)
  {
    as_snprintf(name, sizeof(name), "%s%c", p_name, reg_16_names[z]);
    AddInstTable(InstTable, name, code + (z * code_incr), proc);
  }
}

static void add_reg_stack(const char *p_name, Word code, InstProc proc)
{
  char name[10];
  size_t z;

  for (z = 2; z < as_array_size(reg_16_names); z++)
  {
    as_snprintf(name, sizeof(name), "%s%c", p_name, reg_16_names[z]);
    AddInstTable(InstTable, name, code + (3 - z), proc);
  }
}

static void add_ari_8(const char *p_name, Word code)
{
  char name[10];

  as_snprintf(name, sizeof(name), "%sA", p_name);
  AddInstTable(InstTable, name, 0x0200 | code, decode_ari);
  as_snprintf(name, sizeof(name), "%sB", p_name);
  AddInstTable(InstTable, name, 0x0200 | (code + 1), decode_ari);
}

static void add_branch(const char *p_name, Word code)
{
  char name[10];

  AddInstTable(InstTable, p_name, code, decode_branch_8);
  as_snprintf(name, sizeof(name), "L%s", p_name);
  AddInstTable(InstTable, name, code + 8, decode_branch_16);
}

static void init_fields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);

  add_reg_16("LEA", 0x08, 1, decode_idx);
  add_reg_stack("PSH", 0x0c, DecodeStack_6809);
  add_reg_stack("PUL", 0x0e, DecodeStack_6809);
  add_ari_8("LD"  , 0x10);
  add_ari_8("ADD" , 0x14);
  add_ari_8("ADC" , 0x18);
  add_ari_8("SUB" , 0x1c);
  add_ari_8("SBC" , 0x20);
  add_ari_8("AND" , 0x24);
  add_ari_8("BIT" , 0x28);
  add_ari_8("EOR" , 0x2c);
  add_ari_8("OR"  , 0x30);
  add_ari_8("CMP" , 0x34);
  AddInstTable(InstTable, "SETLINES", 0x0138, decode_ari);
  AddInstTable(InstTable, "STA", 0x3a, decode_idx);
  AddInstTable(InstTable, "STB", 0x3b, decode_idx);
  AddInstTable(InstTable, "ANDCC", 0x3c, decode_imm_8);
  AddInstTable(InstTable, "ORCC", 0x3d, decode_imm_8);
  AddInstTable(InstTable, "EXG", 0x3e, decode_exg_tfr);
  AddInstTable(InstTable, "TFR", 0x3f, decode_exg_tfr);
  AddInstTable(InstTable, "LDD", 0x8140, decode_ari);
  add_reg_16("LD", 0x8142, 2, decode_ari);
  AddInstTable(InstTable, "CMPD", 0x814a, decode_ari);
  add_reg_16("CMP", 0x814c, 2, decode_ari);
  AddInstTable(InstTable, "ADDD", 0x8154, decode_ari);
  AddInstTable(InstTable, "SUBD", 0x8156, decode_ari);
  AddInstTable(InstTable, "STD", 0x58, decode_ari);
	add_branch("BRA", 0x60);
  add_branch("BHI", 0x61);
  add_branch("BCC", 0x62);
  add_branch("BNE", 0x63);
  add_branch("BVC", 0x64);
  add_branch("BPL", 0x65);
  add_branch("BGE", 0x66);
  add_branch("BGT", 0x67);
  add_branch("BRN", 0x70);
  add_branch("BLS", 0x71);
  add_branch("BCS", 0x72);
  add_branch("BEQ", 0x73);
  add_branch("BVS", 0x74);
  add_branch("BMI", 0x75);
  add_branch("BLT", 0x76);
  add_branch("BLE", 0x77);
  add_reg_16("ST", 0x59, 1, decode_ari);
  add_acc_idx("CLR" , 0x80);
  add_acc_idx("COM" , 0x83);
  add_acc_idx("NEG" , 0x86);
  add_acc_idx("INC" , 0x89);
  AddInstTable(InstTable, "DECA" , 0x8c + 0, decode_inh);
  AddInstTable(InstTable, "DECB" , 0x8c + 1, decode_inh);
  AddInstTable(InstTable, "RTS", 0x8f, decode_inh);
  add_acc_idx("TST" , 0x90);
  add_acc_idx("LSR" , 0x93);
  add_acc_idx("ROR" , 0x96);
  add_acc_idx("ASR" , 0x99);
  add_acc_idx("ASL" , 0x9c);
  AddInstTable(InstTable, "RTI", 0x9f, decode_inh);
  add_acc_idx("ROL" , 0xa0);
  AddInstTable(InstTable, "LSRW", 0xa3, decode_idx);
  AddInstTable(InstTable, "RORW", 0xa4, decode_idx);
  AddInstTable(InstTable, "ASRW", 0xa5, decode_idx);
  AddInstTable(InstTable, "ASLW", 0xa6, decode_idx);
  AddInstTable(InstTable, "ROLW", 0xa7, decode_idx);
  AddInstTable(InstTable, "JMP", 0xa8, decode_idx);
  AddInstTable(InstTable, "JSR", 0xa9, decode_idx);
  AddInstTable(InstTable, "BSR" , 0xaa, decode_branch_8);
  AddInstTable(InstTable, "LBSR" , 0xab, decode_branch_16);
  AddInstTable(InstTable, "DEC" , 0xac, decode_dec);
  AddInstTable(InstTable, "NOP" , NOPCode, decode_inh);
  AddInstTable(InstTable, "ABX" , 0xb0, decode_inh);
  AddInstTable(InstTable, "DAA" , 0xb1, decode_inh);
  AddInstTable(InstTable, "SEX" , 0xb2, decode_inh);
  AddInstTable(InstTable, "MUL" , 0xb3, decode_inh);
  AddInstTable(InstTable, "LMUL", 0xb4, decode_inh);
  AddInstTable(InstTable, "DIV" , 0xb5, decode_div);
  AddInstTable(InstTable, "BMOVE", 0xb6, decode_move);
  AddInstTable(InstTable, "MOVE", 0xb7, decode_move);
  AddInstTable(InstTable, "LSRD", 0x01b8, decode_ari);
  AddInstTable(InstTable, "RORD", 0x01ba, decode_ari);
  AddInstTable(InstTable, "ASRD", 0x01bc, decode_ari);
  AddInstTable(InstTable, "ASLD", 0x01be, decode_ari);
  AddInstTable(InstTable, "ROLD", 0x01c0, decode_ari);
  add_acc_idx_16("CLR", 0xc2);
  add_acc_idx_16("NEG", 0xc4);
  add_acc_idx_16("INC", 0xc6);
  add_acc_idx_16("DEC", 0xc8);
  add_acc_idx_16("TST", 0xca);
  AddInstTable(InstTable, "ABSA" , 0xcc, decode_inh);
  AddInstTable(InstTable, "ABSB" , 0xcd, decode_inh);
  AddInstTable(InstTable, "ABSD" , 0xce, decode_inh);
  AddInstTable(InstTable, "BSETA", 0xcf, decode_bset);
  AddInstTable(InstTable, "BSETD", 0xd0, decode_bset);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
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
 * \fn     decode_attr_part_ko09(void)
 * \brief  parse attribute part
 * ------------------------------------------------------------------------ */

static Boolean decode_attr_part_ko09(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  /* Deduce operand size.  No size is zero-length string -> '\0' */

  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

/*!------------------------------------------------------------------------
 * \fn     make_code_ko09(void)
 * \brief  handle machine instructions
 * ------------------------------------------------------------------------ */

static void make_code_ko09(void)
{
  tSymbolSize op_size;

  CodeLen = 0;
  DontPrint = False;
  op_size = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize8Bit;

  /* to be ignored */

  if (Memo(""))
    return;

  /* pseudo instructions */

  if (DecodeMoto16Pseudo(op_size, True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     is_def_ko09(void)
 * \brief  label part consumed by instruction?
 * ------------------------------------------------------------------------ */

static Boolean is_def_ko09(void)
{
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     switch_to_ko09(void)
 * \brief  switch to target
 * ------------------------------------------------------------------------ */

static void switch_to_ko09(void)
{
  const TFamilyDescr *p_descr = FindFamilyByName("052001");
  static const ASSUMERec ASSUME09s[] =
  {
    { "DPR", &DPRValue, 0, 0xff, 0x100, NULL }
  };

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = p_descr->Id;
  NOPCode = 0xae;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  DecodeAttrPart = decode_attr_part_ko09;
  MakeCode = make_code_ko09;
  IsDef = is_def_ko09;

  SwitchFrom = deinit_fields;
  init_fields();
  AddMoto16PseudoONOFF(False);

  pASSUMERecs = ASSUME09s;
  ASSUMERecCnt = as_array_size(ASSUME09s);
}

/*!------------------------------------------------------------------------
 * \fn     codeko09_init(void)
 * \brief  attach target
 * ------------------------------------------------------------------------ */

void codeko09_init(void)
{
  (void)AddCPU("052001", switch_to_ko09);
}
