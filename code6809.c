/* code6809.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator 6809/6309                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"

#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmallg.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code6809.h"

typedef struct
{
  char *Name;
  Word Code;
  CPUVar MinCPU;
} BaseOrder;

typedef struct
{
  char *Name;
  Word Code;
  Boolean Inv;
  CPUVar MinCPU;
} FlagOrder;

typedef struct
{
  char *Name;
  Word Code8;
  Word Code16;
  CPUVar MinCPU;
} RelOrder;

typedef struct
{
  char *Name;
  Word Code;
  tSymbolSize OpSize;
  Boolean MayImm;
  CPUVar MinCPU;
} ALUOrder;

typedef enum
{
  e_adr_mode_none = -1,
  e_adr_mode_imm = 1,
  e_adr_mode_dir = 2,
  e_adr_mode_ind = 3,
  e_adr_mode_ext = 4
} adr_mode_t;

#define adr_mode_mask_imm (1 << e_adr_mode_imm)
#define adr_mode_mask_dir (1 << e_adr_mode_dir)
#define adr_mode_mask_ind (1 << e_adr_mode_ind)
#define adr_mode_mask_ext (1 << e_adr_mode_ext)
#define adr_mode_mask_no_imm (adr_mode_mask_dir | adr_mode_mask_ind | adr_mode_mask_ext)
#define adr_mode_mask_all (adr_mode_mask_imm | adr_mode_mask_no_imm)

typedef struct
{
  adr_mode_t mode;
  int cnt;
  Byte vals[5];
} adr_vals_t;

#define StackRegCnt 12
static char StackRegNames[StackRegCnt][4] =
{
  "CCR",  "A",  "B","DPR",  "X",  "Y","S/U", "PC", "CC", "DP",  "S",  "D"
};
static Byte StackRegMasks[StackRegCnt] =
{
  0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x08, 0x40, 0x06
};

static const char FlagChars[] = "CVZNIHFE";

static LongInt DPRValue;

static BaseOrder  *FixedOrders;
static RelOrder   *RelOrders;
static ALUOrder   *ALUOrders;
static BaseOrder *RMWOrders;
static FlagOrder *FlagOrders;
static BaseOrder *LEAOrders;
static BaseOrder *ImmOrders;

static CPUVar CPU6809, CPU6309;

/*-------------------------------------------------------------------------*/

static Boolean CodeReg(char *ChIn, Byte *erg)
{
  static char Regs[5] = "XYUS", *p;

  if (strlen(ChIn) != 1)
    return False;
  else
  {
    p = strchr(Regs, as_toupper(*ChIn));
    if (!p)
      return False;
    *erg = p - Regs;
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
    if (1[s] == '<')
    {
      *Erg = 3;
      return 2;
    }
    else
    {
      *Erg = 2;
      return 1;
    }
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

static void reset_adr_vals(adr_vals_t *p_vals)
{
  p_vals->mode = e_adr_mode_none;
  p_vals->cnt = 0;
}

static adr_mode_t DecodeAdr(int ArgStartIdx, int ArgEndIdx,
                            unsigned OpcodeLen, tSymbolSize op_size,
                            unsigned mode_mask, adr_vals_t *p_vals)
{
  tStrComp *pStartArg, *pEndArg, IndirComps[2];
  String temp;
  LongInt AdrLong;
  Word AdrWord;
  Boolean IndFlag, OK;
  Byte EReg, ZeroMode;
  char *p;
  unsigned Offset;
  Integer AdrInt;
  int AdrArgCnt = ArgEndIdx - ArgStartIdx + 1;
  const Boolean allow_6309 = (MomCPU >= CPU6309);

  reset_adr_vals(p_vals);
  pStartArg = &ArgStr[ArgStartIdx];
  pEndArg = &ArgStr[ArgEndIdx];

  /* immediate */

  if ((*pStartArg->str.p_str == '#') && (AdrArgCnt == 1))
  {
    switch (op_size)
    {
      case eSymbolSize32Bit:
        AdrLong = EvalStrIntExpressionOffs(pStartArg, 1, Int32, &OK);
        if (OK)
        {
          p_vals->vals[0] = Lo(AdrLong >> 24);
          p_vals->vals[1] = Lo(AdrLong >> 16);
          p_vals->vals[2] = Lo(AdrLong >>  8);
          p_vals->vals[3] = Lo(AdrLong);
          p_vals->cnt = 4;
        }
        break;
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

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (strlen(pEndArg->str.p_str) == 2) && (*pEndArg->str.p_str == '-') && (CodeReg(pEndArg->str.p_str + 1, &EReg)))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0x82 + (EReg << 5) + (Ord(IndFlag) << 4);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (strlen(pEndArg->str.p_str) == 3) && (!strncmp(pEndArg->str.p_str, "--", 2)) && (CodeReg(pEndArg->str.p_str + 2, &EReg)))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0x83 + (EReg << 5) + (Ord(IndFlag) << 4);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (!as_strcasecmp(pEndArg->str.p_str, "--W")))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else if (!allow_6309) WrError(ErrNum_AddrModeNotSupported);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0xef + Ord(IndFlag);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  /* Postinkrement ? */

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (strlen(pEndArg->str.p_str) == 2) && (pEndArg->str.p_str[1] == '+'))
  {
    temp[0] = *pEndArg->str.p_str;
    temp[1] = '\0';
    if (CodeReg(temp, &EReg))
    {
      if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
      else
      {
        p_vals->cnt = 1;
        p_vals->vals[0] = 0x80 + (EReg << 5) + (Ord(IndFlag) << 4);
        p_vals->mode = e_adr_mode_ind;
      }
      goto chk_mode;
    }
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (strlen(pEndArg->str.p_str) == 3) && (!strncmp(pEndArg->str.p_str + 1, "++", 2)))
  {
    temp[0] = *pEndArg->str.p_str;
    temp[1] = '\0';
    if (CodeReg(temp, &EReg))
    {
      if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
      else
      {
        p_vals->cnt = 1;
        p_vals->vals[0] = 0x81 + (EReg << 5) + (Ord(IndFlag) << 4);
        p_vals->mode = e_adr_mode_ind;
      }
      goto chk_mode;
    }
  }

  if ((AdrArgCnt >= 1) && (AdrArgCnt <= 2) && (!as_strcasecmp(pEndArg->str.p_str, "W++")))
  {
    if ((AdrArgCnt == 2) && !IsZeroOrEmpty(pStartArg)) WrError(ErrNum_InvAddrMode);
    else if (!allow_6309) WrError(ErrNum_AddrModeNotSupported);
    else
    {
      p_vals->cnt = 1;
      p_vals->vals[0] = 0xcf + Ord(IndFlag);
      p_vals->mode = e_adr_mode_ind;
    }
    goto chk_mode;
  }

  /* 16-Bit-Register (mit Index) ? */

  if ((AdrArgCnt <= 2) && (AdrArgCnt >= 1) && (CodeReg(pEndArg->str.p_str, &EReg)))
  {
    p_vals->vals[0] = (EReg << 5) + (Ord(IndFlag) << 4);

    /* nur 16-Bit-Register */

    if (AdrArgCnt == 1)
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x84;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* mit Index */

    if (!as_strcasecmp(pStartArg->str.p_str, "A"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x86;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if (!as_strcasecmp(pStartArg->str.p_str, "B"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x85;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if (!as_strcasecmp(pStartArg->str.p_str, "D"))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x8b;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if ((!as_strcasecmp(pStartArg->str.p_str, "E")) && allow_6309)
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x87;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if ((!as_strcasecmp(pStartArg->str.p_str, "F")) && allow_6309)
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x8a;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }
    if ((!as_strcasecmp(pStartArg->str.p_str, "W")) && allow_6309)
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x8e;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* Displacement auswerten */

    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    if (ZeroMode > 1)
    {
      tSymbolFlags Flags;

      AdrInt = EvalStrIntExpressionOffsWithFlags(pStartArg, Offset, Int8, &OK, &Flags);
      if (mFirstPassUnknown(Flags) && (ZeroMode == 3))
        AdrInt &= 0x0f;
    }
    else
      AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, Int16, &OK);
    if (!OK)
      goto chk_mode;

    /* Displacement 0 ? */

    if ((ZeroMode == 0) && (AdrInt == 0))
    {
      p_vals->cnt = 1;
      p_vals->vals[0] += 0x84;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* 5-Bit-Displacement */

    else if ((ZeroMode == 3) || ((ZeroMode == 0) && (!IndFlag) && (AdrInt >= -16) && (AdrInt <= 15)))
    {
      if ((AdrInt < -16) || (AdrInt > 15)) WrError(ErrNum_NoShortAddr);
      else if (IndFlag) WrError(ErrNum_InvAddrMode);
      else
      {
        p_vals->mode = e_adr_mode_ind;
        p_vals->cnt = 1;
        p_vals->vals[0] += AdrInt & 0x1f;
      }
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
        p_vals->vals[0] += 0x88;
        p_vals->vals[1] = Lo(AdrInt);
      }
      goto chk_mode;
    }

    /* 16-Bit-Displacement */

    else
    {
      p_vals->mode = e_adr_mode_ind;
      p_vals->cnt = 3;
      p_vals->vals[0] += 0x89;
      p_vals->vals[1] = Hi(AdrInt);
      p_vals->vals[2] = Lo(AdrInt);
      goto chk_mode;
    }
  }

  if ((AdrArgCnt <= 2) && (AdrArgCnt >= 1) && allow_6309 && (!as_strcasecmp(pEndArg->str.p_str, "W")))
  {
    p_vals->vals[0] = 0x8f + Ord(IndFlag);

    /* nur W-Register */

    if (AdrArgCnt == 1)
    {
      p_vals->cnt = 1;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* Displacement auswerten */

    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, Int16, &OK);

    /* Displacement 0 ? */

    if ((ZeroMode == 0) && (AdrInt == 0))
    {
      p_vals->cnt = 1;
      p_vals->mode = e_adr_mode_ind;
      goto chk_mode;
    }

    /* 16-Bit-Displacement */

    else
    {
      p_vals->mode = e_adr_mode_ind;
      p_vals->cnt = 3;
      p_vals->vals[0] += 0x20;
      p_vals->vals[1] = Hi(AdrInt);
      p_vals->vals[2] = Lo(AdrInt);
      goto chk_mode;
    }
  }

  /* PC-relativ ? */

  if ((AdrArgCnt == 2) && ((!as_strcasecmp(pEndArg->str.p_str, "PCR")) || (!as_strcasecmp(pEndArg->str.p_str, "PC"))))
  {
    p_vals->vals[0] = Ord(IndFlag) << 4;
    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    AdrInt = EvalStrIntExpressionOffs(pStartArg, Offset, Int16, &OK);
    if (OK)
    {
      AdrInt -= EProgCounter() + 2 + OpcodeLen;

      if (ZeroMode == 3) WrError(ErrNum_InvAddrMode);

      else if ((ZeroMode == 2) || ((ZeroMode == 0) && MayShort(AdrInt)))
      {
        if (!MayShort(AdrInt)) WrError(ErrNum_OverRange);
        else
        {
          p_vals->cnt = 2;
          p_vals->vals[0] += 0x8c;
          p_vals->vals[1] = Lo(AdrInt);
          p_vals->mode = e_adr_mode_ind;
        }
      }

      else
      {
        AdrInt--;
        p_vals->cnt = 3;
        p_vals->vals[0] += 0x8d;
        p_vals->vals[1] = Hi(AdrInt);
        p_vals->vals[2] = Lo(AdrInt);
        p_vals->mode = e_adr_mode_ind;
      }
    }
    goto chk_mode;
  }

  if (AdrArgCnt == 1)
  {
    tSymbolFlags Flags;

    Offset = ChkZero(pStartArg->str.p_str, &ZeroMode);
    AdrInt = EvalStrIntExpressionOffsWithFlags(pStartArg, Offset, Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags) && (ZeroMode == 2))
      AdrInt = (AdrInt & 0xff) | (DPRValue << 8);

    if (OK)
    {
      if (ZeroMode == 3) WrError(ErrNum_InvAddrMode);

      else if ((ZeroMode == 2) || ((ZeroMode == 0) && (Hi(AdrInt) == DPRValue) && (!IndFlag)))
      {
        if (IndFlag) WrError(ErrNum_NoIndir);
        else if (Hi(AdrInt) != DPRValue) WrError(ErrNum_NoShortAddr);
        else
        {
          p_vals->cnt = 1;
          p_vals->mode = e_adr_mode_dir;
          p_vals->vals[0] = Lo(AdrInt);
        }
      }

      else
      {
        if (IndFlag)
        {
          p_vals->mode = e_adr_mode_ind;
          p_vals->cnt = 3;
          p_vals->vals[0] = 0x9f;
          p_vals->vals[1] = Hi(AdrInt);
          p_vals->vals[2] = Lo(AdrInt);
        }
        else
        {
          p_vals->mode = e_adr_mode_ext;
          p_vals->cnt = 2;
          p_vals->vals[0] = Hi(AdrInt);
          p_vals->vals[1] = Lo(AdrInt);
        }
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

static Boolean CodeCPUReg(const char *Asc, Byte *Erg)
{
#define RegCnt (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][4] =
  {
    "D", "X", "Y", "U", "S", "SP", "PC", "W", "V", "A", "B", "CCR", "DPR", "CC", "DP", "Z", "E", "F"
  };
  static const Byte RegVals[RegCnt] =
  {
    0  , 1  , 2  , 3  , 4  , 4   , 5   , 6  , 7  , 8  , 9  , 10   , 11   , 10  , 11  , 13 , 14 , 15
   };

  unsigned z;
  String Asc_N;

  strmaxcpy(Asc_N, Asc, STRINGSIZE); NLS_UpString(Asc_N); Asc = Asc_N;

  for (z = 0; z < RegCnt; z++)
    if (!strcmp(Asc, RegNames[z]))
    {
      if (((RegVals[z] & 6) == 6) && !ChkMinCPUExt(CPU6309, ErrNum_AddrModeNotSupported));
      else
      {
        *Erg = RegVals[z];
        return True;
      }
    }
  return False;
}

static void SplitIncDec(char *s, int *Erg)
{
  int l = strlen(s);

  if (l == 0)
    *Erg = 0;
  else if (s[l - 1] == '+')
  {
    s[l - 1] = '\0';
    *Erg = 1;
  }
  else if (s[l - 1] == '-')
  {
    s[l - 1] = '\0';
    *Erg = -1;
  }
  else
    *Erg = 0;
}

static Boolean SplitBit(tStrComp *pArg, int *Erg)
{
  char *p;
  Boolean OK;
  tStrComp BitArg;

  p = QuotPos(pArg->str.p_str, '.');
  if (!p)
  {
    WrError(ErrNum_InvBitPos);
    return False;
  }
  StrCompSplitRef(pArg, &BitArg, pArg, p);
  *Erg = EvalStrIntExpression(&BitArg, UInt3, &OK);
  if (!OK)
    return False;
  *p = '\0';
  return True;
}

static void append_adr_vals(const adr_vals_t *p_vals)
{
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->cnt);
  CodeLen += p_vals->cnt;
}

/*-------------------------------------------------------------------------*/

/* Anweisungen ohne Argument */

static void DecodeFixed(Word Index)
{
  const BaseOrder *pOrder = FixedOrders + Index;

  if (!ChkArgCnt(0, 0));
  else if (!ChkMinCPU(pOrder->MinCPU));
  else if (Hi(pOrder->Code) == 0)
  {
    BAsmCode[0] = Lo(pOrder->Code);
    CodeLen = 1;
  }
  else
  {
    BAsmCode[0] = Hi(pOrder->Code);
    BAsmCode[1] = Lo(pOrder->Code);
    CodeLen = 2;
  }
}

/* Specials... */

static void DecodeSWI(Word Code)
{
  UNUSED(Code);

  if (ArgCnt == 0)
  {
    BAsmCode[0] = 0x3f;
    CodeLen = 1;
  }
  else if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Byte Num;

    Num = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt2, &OK, &Flags);
    if (OK && mFirstPassUnknown(Flags) && (Num < 2))
      Num = 2;
    if (OK && ChkRange(Num, 2, 3))
    {
      BAsmCode[0] = 0x10 | (Num & 1);
      BAsmCode[1] = 0x3f;
      CodeLen = 2;
    }
  }
}

/* relative Spruenge */

static void DecodeRel(Word Index)
{
  Boolean LongFlag = (Index & 0x8000) || False;
  const RelOrder *pOrder = RelOrders + (Index & 0x7fff);

  if (ChkArgCnt(1, 1))
  {
    Boolean ExtFlag = (LongFlag) && (Hi(pOrder->Code16) != 0), OK;
    tSymbolFlags Flags;
    Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);

    if (OK)
    {
      AdrInt -= EProgCounter() + 2 + Ord(LongFlag) + Ord(ExtFlag);
      if (!mSymbolQuestionable(Flags) && !LongFlag && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 1 + Ord(ExtFlag);
        if (LongFlag)
        {
          if (ExtFlag)
          {
            BAsmCode[0] = Hi(pOrder->Code16);
            BAsmCode[1] = Lo(pOrder->Code16);
          }
          else
            BAsmCode[0] = Lo(pOrder->Code16);
        }
        else
          BAsmCode[0] = Lo(pOrder->Code8);
        if (LongFlag)
        {
          BAsmCode[CodeLen] = Hi(AdrInt);
          BAsmCode[CodeLen + 1] = Lo(AdrInt);
          CodeLen += 2;
        }
        else
        {
          BAsmCode[CodeLen] = Lo(AdrInt);
          CodeLen++;
        }
      }
    }
  }
}

/* ALU-Operationen */

static void DecodeALU(Word Index)
{
  const ALUOrder *pOrder = ALUOrders + Index;

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(pOrder->MinCPU))
  {
    adr_vals_t vals;

    if (DecodeAdr(1, ArgCnt, 1 + !!Hi(pOrder->Code), pOrder->OpSize, pOrder->MayImm ? adr_mode_mask_all : adr_mode_mask_no_imm, &vals) != e_adr_mode_none)
    {
      if (Hi(pOrder->Code))
        BAsmCode[CodeLen++] = Hi(pOrder->Code);
      BAsmCode[CodeLen++] = Lo(pOrder->Code) + ((vals.mode - 1) << 4);
      append_adr_vals(&vals);
    }
  }
}

static void DecodeLDQ(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(CPU6309))
  {
    adr_vals_t vals;

    if (DecodeAdr(1, ArgCnt, 2, eSymbolSize32Bit, adr_mode_mask_all, &vals) == e_adr_mode_imm)
    {
      BAsmCode[CodeLen++] = 0xcd;
      append_adr_vals(&vals);
    }
    else
    {
      BAsmCode[CodeLen++] = 0x10;
      BAsmCode[CodeLen++] = 0xcc + ((vals.mode - 1) << 4);
      append_adr_vals(&vals);
    }
  }
}

/* Read-Modify-Write-Operationen */

static void DecodeRMW(Word Index)
{
  const BaseOrder *pOrder = RMWOrders + Index;

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(pOrder->MinCPU))
  {
    adr_vals_t vals;

    switch (DecodeAdr(1, ArgCnt, 1, eSymbolSizeUnknown, adr_mode_mask_no_imm, &vals))
    {
      case e_adr_mode_dir:
        BAsmCode[CodeLen++] = pOrder->Code;
        goto append;
      case e_adr_mode_ind:
        BAsmCode[CodeLen++] = pOrder->Code + 0x60;
        goto append;
      case e_adr_mode_ext:
        BAsmCode[CodeLen++] = pOrder->Code + 0x70;
        goto append;
      append:
        append_adr_vals(&vals);
        break;
      default:
        return;
    }
  }
}

/* Anweisungen mit Flag-Operand */

static void DecodeFlag(Word Index)
{
  const FlagOrder *pOrder = FlagOrders + Index;
  Boolean OK;
  const char *p;
  int z2, z3;

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    BAsmCode[1] = (pOrder->Inv) ? 0xff : 0x00;
    for (z2 = 1; z2 <= ArgCnt; z2++)
      if (OK)
      {
        p = (strlen(ArgStr[z2].str.p_str) == 1) ? strchr(FlagChars, as_toupper(*ArgStr[z2].str.p_str)) : NULL;
        if (p)
        {
          z3 = p - FlagChars;
          if (pOrder->Inv)
            BAsmCode[1] &= (0xff ^ (1 << z3));
          else
            BAsmCode[1] |= (1 << z3);
        }
        else if (*ArgStr[z2].str.p_str != '#')
        {
          WrError(ErrNum_OnlyImmAddr);
          OK = False;
        }
        else
        {
          BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[z2], 1, Int8, &OK);
          if (OK)
          {
            if (pOrder->Inv)
              BAsmCode[1] &= BAsmCode[2];
            else
              BAsmCode[1] |= BAsmCode[2];
          }
        }
      }
    if (OK)
    {
      CodeLen = 2;
      BAsmCode[0] = pOrder->Code;
    }
  }
}

/* Bit-Befehle */

static void DecodeImm(Word Index)
{
  const BaseOrder *pOrder = ImmOrders + Index;

  if (!ChkArgCnt(2, 3));
  else if (!ChkMinCPU(pOrder->MinCPU));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int8, &OK);
    if (OK)
    {
      adr_vals_t vals;

      switch (DecodeAdr(2, ArgCnt, 2, eSymbolSizeUnknown, adr_mode_mask_no_imm, &vals))
      {
        case e_adr_mode_dir:
          BAsmCode[0] = pOrder->Code;
          goto append;
        case e_adr_mode_ext:
          BAsmCode[0] = pOrder->Code + 0x70;
          goto append;
        case e_adr_mode_ind:
          BAsmCode[0] = pOrder->Code + 0x60;
          goto append;
        append:
          CodeLen = 2;
          append_adr_vals(&vals);
          break;
        default:
          return;
      }
    }
  }
}

static void DecodeBit(Word Code)
{
  int z2, z3;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU6309)
   && SplitBit(&ArgStr[1], &z2) && SplitBit(&ArgStr[2], &z3))
  {
    if (!CodeCPUReg(ArgStr[1].str.p_str, BAsmCode + 2)) WrError(ErrNum_InvRegName);
    else if ((BAsmCode[2] < 8) || (BAsmCode[2] > 11)) WrError(ErrNum_InvRegName);
    else
    {
      adr_vals_t vals;

      if (DecodeAdr(2, 2, 3, eSymbolSizeUnknown, adr_mode_mask_dir, &vals) != e_adr_mode_none)
      {
        BAsmCode[2] -= 7;
        if (BAsmCode[2] == 3)
          BAsmCode[2] = 0;
        BAsmCode[0] = 0x11;
        BAsmCode[1] = 0x30 + Code;
        BAsmCode[2] = (BAsmCode[2] << 6) + (z3 << 3) + z2;
        BAsmCode[3] = vals.vals[0];
        CodeLen = 4;
      }
    }
  }
}

/* Register-Register-Operationen */

void DecodeTFR_TFM_EXG_6809(Word code, Boolean allow_inc_dec, reg_decoder_6809_t reg_decoder, Boolean reverse)
{
  if (ChkArgCnt(2, 2))
  {
    int Inc1, Inc2;

    SplitIncDec(ArgStr[1].str.p_str, &Inc1);
    SplitIncDec(ArgStr[2].str.p_str, &Inc2);
    if ((Inc1 != 0) || (Inc2 != 0))
    {
      if (!allow_inc_dec) WrError(ErrNum_InvAddrMode);
      else if (!reg_decoder(ArgStr[1].str.p_str, BAsmCode + 3) || (BAsmCode[3] > 4)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[1]);
      else if (!reg_decoder(ArgStr[2].str.p_str, BAsmCode + 2) || (BAsmCode[2] > 4)) WrStrErrorPos(ErrNum_InvRegName, &ArgStr[2]);
      else
      {
        BAsmCode[0] = 0x11;
        BAsmCode[1] = 0;
        if (reverse)
          BAsmCode[2] = (BAsmCode[2] << 4) | (BAsmCode[3] & 0x0f);
        else
          BAsmCode[2] |= BAsmCode[3] << 4;
        if ((Inc1 == 1) && (Inc2 == 1))
          BAsmCode[1] = 0x38;
        else if ((Inc1 == -1) && (Inc2 == -1))
          BAsmCode[1] = 0x39;
        else if ((Inc1 ==  1) && (Inc2 ==  0))
          BAsmCode[1] = 0x3a;
        else if ((Inc1 ==  0) && (Inc2 ==  1))
          BAsmCode[1] = 0x3b;
        if (BAsmCode[1] == 0) WrError(ErrNum_InvAddrMode);
        else
          CodeLen = 3;
      }
    }
    else if (Memo("TFM")) WrError(ErrNum_InvAddrMode);
    else if (!reg_decoder(ArgStr[1].str.p_str, BAsmCode + 2)) WrError(ErrNum_InvRegName);
    else if (!reg_decoder(ArgStr[2].str.p_str, BAsmCode + 1)) WrError(ErrNum_InvRegName);
    else if ((BAsmCode[1] != 13) && (BAsmCode[2] != 13) /* Z Register compatible to all other registers */
          && (((BAsmCode[1] ^ BAsmCode[2]) & 0x08) != 0)) WrError(ErrNum_ConfOpSizes);
    else
    {
      CodeLen = 2;
      BAsmCode[0] = code;
      if (reverse)
        BAsmCode[1] = (BAsmCode[1] << 4) | (BAsmCode[2] & 0x0f);
      else
        BAsmCode[1] |= BAsmCode[2] << 4;
    }
  }
}

static void DecodeTFR_TFM(Word code)
{
  DecodeTFR_TFM_EXG_6809(code, MomCPU == CPU6309, CodeCPUReg, False);
}

static void DecodeEXG(Word code)
{
  DecodeTFR_TFM_EXG_6809(code, False, CodeCPUReg, False);
}

static void DecodeALU2(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (!CodeCPUReg(ArgStr[1].str.p_str, &BAsmCode[3])) WrError(ErrNum_InvRegName);
  else if (!CodeCPUReg(ArgStr[2].str.p_str, &BAsmCode[2])) WrError(ErrNum_InvRegName);
  else if ((BAsmCode[2] != 13) && (BAsmCode[3] != 13) /* Z Register compatible to all other registers */
        && (((BAsmCode[2] ^ BAsmCode[3]) & 0x08) != 0)) WrError(ErrNum_ConfOpSizes);
  else
  {
    CodeLen = 3;
    BAsmCode[0] = 0x10;
    BAsmCode[1] = 0x30 + Code;
    BAsmCode[2] += BAsmCode[3] << 4;
  }
}

/* Berechnung effektiver Adressen */

static void DecodeLEA(Word Index)
{
  const BaseOrder *pOrder = LEAOrders + Index;

  if (ChkArgCnt(1, 2))
  {
    adr_vals_t vals;

    if (DecodeAdr(1, ArgCnt, 1, eSymbolSizeUnknown, adr_mode_mask_ind, &vals) != e_adr_mode_none)
    {
      BAsmCode[CodeLen++] = pOrder->Code;
      append_adr_vals(&vals);
    }
  }
}

/* Push/Pull */

void DecodeStack_6809(Word code)
{
  Boolean OK = True, Extent = False;
  int z2, z3;
  Boolean has_w = !!Hi(code);

  BAsmCode[1] = 0;

  /* S oder U einsetzen, entsprechend Opcode */

  *StackRegNames[StackRegCnt - 2] = OpPart.str.p_str[strlen(OpPart.str.p_str) - 1] ^ ('S' ^ 'U');
  for (z2 = 1; z2 <= ArgCnt; z2++)
    if (OK)
    {
      if (!as_strcasecmp(ArgStr[z2].str.p_str, "W"))
      {
        if (!has_w)
          OK = False;
        else if (ArgCnt != 1)
        {
          WrError(ErrNum_InAccReg);
          OK = False;
        }
        else
          Extent = True;
      }
      else
      {
        for (z3 = 0; z3 < StackRegCnt; z3++)
          if (!as_strcasecmp(ArgStr[z2].str.p_str, StackRegNames[z3]))
          {
            BAsmCode[1] |= StackRegMasks[z3];
            break;
          }
        if (z3 >= StackRegCnt)
        {
          if (!as_strcasecmp(ArgStr[z2].str.p_str, "ALL"))
            BAsmCode[1] = 0xff;
          else if (*ArgStr[z2].str.p_str != '#') OK = False;
          else
          {
            BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[z2], 1, Int8, &OK);
            if (OK)
              BAsmCode[1] |= BAsmCode[2];
          }
        }
      }
    }
  if (OK)
  {
    if (Extent)
    {
      CodeLen = 2;
      BAsmCode[0] = 0x10;
      BAsmCode[1] = Lo(code) + 4;
    }
    else
    {
      CodeLen = 2;
      BAsmCode[0] = code;
    }
  }
  else
    WrStrErrorPos(ErrNum_InvRegName, &ArgStr[z2 - 1]);
}

static void DecodeBITMD_LDMD(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU6309));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean OK;

    BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[1], 1,Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x11;
      BAsmCode[1] = Code;
      CodeLen = 3;
    }
  }
}

/*-------------------------------------------------------------------------*/
/* Erzeugung/Aufloesung Codetabellen */

static void AddFixed(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddRel(const char *NName, Word NCode8, Word NCode16)
{
  char LongName[30];

  order_array_rsv_end(RelOrders, RelOrder);
  RelOrders[InstrZ].Code8 = NCode8;
  RelOrders[InstrZ].Code16 = NCode16;
  AddInstTable(InstTable, NName, InstrZ, DecodeRel);
  as_snprintf(LongName, sizeof(LongName), "L%s", NName);
  AddInstTable(InstTable, LongName, InstrZ | 0x8000, DecodeRel);
  InstrZ++;
}

static void AddALU(const char *NName, Word NCode, tSymbolSize NSize, Boolean NImm, CPUVar NCPU)
{
  order_array_rsv_end(ALUOrders, ALUOrder);
  ALUOrders[InstrZ].Code = NCode;
  ALUOrders[InstrZ].OpSize = NSize;
  ALUOrders[InstrZ].MayImm = NImm;
  ALUOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU);
}

static void AddALU2(const char *NName)
{
  char RName[30];

  AddInstTable(InstTable, NName, InstrZ, DecodeALU2);
  as_snprintf(RName, sizeof(RName), "%sR", NName);
  AddInstTable(InstTable, RName, InstrZ, DecodeALU2);
  InstrZ++;
}

static void AddRMW(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(RMWOrders, BaseOrder);
  RMWOrders[InstrZ].Code = NCode;
  RMWOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRMW);
}

static void AddFlag(const char *NName, Word NCode, Boolean NInv, CPUVar NCPU)
{
  order_array_rsv_end(FlagOrders, FlagOrder);
  FlagOrders[InstrZ].Code = NCode;
  FlagOrders[InstrZ].Inv = NInv;
  FlagOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFlag);
}

static void AddLEA(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(LEAOrders, BaseOrder);
  LEAOrders[InstrZ].Code = NCode;
  LEAOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLEA);
}

static void AddImm(const char *NName, Word NCode, CPUVar NCPU)
{
  order_array_rsv_end(ImmOrders, BaseOrder);
  ImmOrders[InstrZ].Code = NCode;
  ImmOrders[InstrZ].MinCPU = NCPU;
  AddInstTable(InstTable, NName, InstrZ++, DecodeImm);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "SWI", 0, DecodeSWI);
  AddInstTable(InstTable, "LDQ", 0, DecodeLDQ);
  AddInstTable(InstTable, "TFR", 0x1f, DecodeTFR_TFM);
  AddInstTable(InstTable, "TFM", 0x1138, DecodeTFR_TFM);
  AddInstTable(InstTable, "EXG", 0x1e, DecodeEXG);
  AddInstTable(InstTable, "BITMD", 0x3c, DecodeBITMD_LDMD);
  AddInstTable(InstTable, "LDMD", 0x3d, DecodeBITMD_LDMD);

  InstrZ = 0;
  AddFixed("NOP"  , 0x0012, CPU6809); AddFixed("SYNC" , 0x0013, CPU6809);
  AddFixed("DAA"  , 0x0019, CPU6809); AddFixed("SEX"  , 0x001d, CPU6809);
  AddFixed("RTS"  , 0x0039, CPU6809); AddFixed("ABX"  , 0x003a, CPU6809);
  AddFixed("RTI"  , 0x003b, CPU6809); AddFixed("MUL"  , 0x003d, CPU6809);
  AddFixed("SWI2" , 0x103f, CPU6809); AddFixed("SWI3" , 0x113f, CPU6809);
  AddFixed("NEGA" , 0x0040, CPU6809); AddFixed("COMA" , 0x0043, CPU6809);
  AddFixed("LSRA" , 0x0044, CPU6809); AddFixed("RORA" , 0x0046, CPU6809);
  AddFixed("ASRA" , 0x0047, CPU6809); AddFixed("ASLA" , 0x0048, CPU6809);
  AddFixed("LSLA" , 0x0048, CPU6809); AddFixed("ROLA" , 0x0049, CPU6809);
  AddFixed("DECA" , 0x004a, CPU6809); AddFixed("INCA" , 0x004c, CPU6809);
  AddFixed("TSTA" , 0x004d, CPU6809); AddFixed("CLRA" , 0x004f, CPU6809);
  AddFixed("NEGB" , 0x0050, CPU6809); AddFixed("COMB" , 0x0053, CPU6809);
  AddFixed("LSRB" , 0x0054, CPU6809); AddFixed("RORB" , 0x0056, CPU6809);
  AddFixed("ASRB" , 0x0057, CPU6809); AddFixed("ASLB" , 0x0058, CPU6809);
  AddFixed("LSLB" , 0x0058, CPU6809); AddFixed("ROLB" , 0x0059, CPU6809);
  AddFixed("DECB" , 0x005a, CPU6809); AddFixed("INCB" , 0x005c, CPU6809);
  AddFixed("TSTB" , 0x005d, CPU6809); AddFixed("CLRB" , 0x005f, CPU6809);
  AddFixed("PSHSW", 0x1038, CPU6309); AddFixed("PULSW", 0x1039, CPU6309);
  AddFixed("PSHUW", 0x103a, CPU6309); AddFixed("PULUW", 0x103b, CPU6309);
  AddFixed("SEXW" , 0x0014, CPU6309); AddFixed("NEGD" , 0x1040, CPU6309);
  AddFixed("COMD" , 0x1043, CPU6309); AddFixed("LSRD" , 0x1044, CPU6309);
  AddFixed("RORD" , 0x1046, CPU6309); AddFixed("ASRD" , 0x1047, CPU6309);
  AddFixed("ASLD" , 0x1048, CPU6309); AddFixed("LSLD" , 0x1048, CPU6309);
  AddFixed("ROLD" , 0x1049, CPU6309); AddFixed("DECD" , 0x104a, CPU6309);
  AddFixed("INCD" , 0x104c, CPU6309); AddFixed("TSTD" , 0x104d, CPU6309);
  AddFixed("CLRD" , 0x104f, CPU6309); AddFixed("COMW" , 0x1053, CPU6309);
  AddFixed("LSRW" , 0x1054, CPU6309); AddFixed("RORW" , 0x1056, CPU6309);
  AddFixed("ROLW" , 0x1059, CPU6309); AddFixed("DECW" , 0x105a, CPU6309);
  AddFixed("INCW" , 0x105c, CPU6309); AddFixed("TSTW" , 0x105d, CPU6309);
  AddFixed("CLRW" , 0x105f, CPU6309); AddFixed("COME" , 0x1143, CPU6309);
  AddFixed("DECE" , 0x114a, CPU6309); AddFixed("INCE" , 0x114c, CPU6309);
  AddFixed("TSTE" , 0x114d, CPU6309); AddFixed("CLRE" , 0x114f, CPU6309);
  AddFixed("COMF" , 0x1153, CPU6309); AddFixed("DECF" , 0x115a, CPU6309);
  AddFixed("INCF" , 0x115c, CPU6309); AddFixed("TSTF" , 0x115d, CPU6309);
  AddFixed("CLRF" , 0x115f, CPU6309); AddFixed("CLRS" , 0x1fd4, CPU6309);
  AddFixed("CLRV" , 0x1fd7, CPU6309); AddFixed("CLRX" , 0x1fd1, CPU6309);
  AddFixed("CLRY" , 0x1fd2, CPU6309);

  InstrZ = 0;
  AddRel("BRA", 0x0020, 0x0016); AddRel("BRN", 0x0021, 0x1021);
  AddRel("BHI", 0x0022, 0x1022); AddRel("BLS", 0x0023, 0x1023);
  AddRel("BHS", 0x0024, 0x1024); AddRel("BCC", 0x0024, 0x1024);
  AddRel("BLO", 0x0025, 0x1025); AddRel("BCS", 0x0025, 0x1025);
  AddRel("BNE", 0x0026, 0x1026); AddRel("BEQ", 0x0027, 0x1027);
  AddRel("BVC", 0x0028, 0x1028); AddRel("BVS", 0x0029, 0x1029);
  AddRel("BPL", 0x002a, 0x102a); AddRel("BMI", 0x002b, 0x102b);
  AddRel("BGE", 0x002c, 0x102c); AddRel("BLT", 0x002d, 0x102d);
  AddRel("BGT", 0x002e, 0x102e); AddRel("BLE", 0x002f, 0x102f);
  AddRel("BSR", 0x008d, 0x0017);

  InstrZ = 0;
  AddALU("LDA" , 0x0086, eSymbolSize8Bit , True , CPU6809);
  AddALU("STA" , 0x0087, eSymbolSize8Bit , False, CPU6809);
  AddALU("CMPA", 0x0081, eSymbolSize8Bit , True , CPU6809);
  AddALU("ADDA", 0x008b, eSymbolSize8Bit , True , CPU6809);
  AddALU("ADCA", 0x0089, eSymbolSize8Bit , True , CPU6809);
  AddALU("SUBA", 0x0080, eSymbolSize8Bit , True , CPU6809);
  AddALU("SBCA", 0x0082, eSymbolSize8Bit , True , CPU6809);
  AddALU("ANDA", 0x0084, eSymbolSize8Bit , True , CPU6809);
  AddALU("ORA" , 0x008a, eSymbolSize8Bit , True , CPU6809);
  AddALU("EORA", 0x0088, eSymbolSize8Bit , True , CPU6809);
  AddALU("BITA", 0x0085, eSymbolSize8Bit , True , CPU6809);

  AddALU("LDB" , 0x00c6, eSymbolSize8Bit , True , CPU6809);
  AddALU("STB" , 0x00c7, eSymbolSize8Bit , False, CPU6809);
  AddALU("CMPB", 0x00c1, eSymbolSize8Bit , True , CPU6809);
  AddALU("ADDB", 0x00cb, eSymbolSize8Bit , True , CPU6809);
  AddALU("ADCB", 0x00c9, eSymbolSize8Bit , True , CPU6809);
  AddALU("SUBB", 0x00c0, eSymbolSize8Bit , True , CPU6809);
  AddALU("SBCB", 0x00c2, eSymbolSize8Bit , True , CPU6809);
  AddALU("ANDB", 0x00c4, eSymbolSize8Bit , True , CPU6809);
  AddALU("ORB" , 0x00ca, eSymbolSize8Bit , True , CPU6809);
  AddALU("EORB", 0x00c8, eSymbolSize8Bit , True , CPU6809);
  AddALU("BITB", 0x00c5, eSymbolSize8Bit , True , CPU6809);

  AddALU("LDD" , 0x00cc, eSymbolSize16Bit, True , CPU6809);
  AddALU("STD" , 0x00cd, eSymbolSize16Bit, False, CPU6809);
  AddALU("CMPD", 0x1083, eSymbolSize16Bit, True , CPU6809);
  AddALU("ADDD", 0x00c3, eSymbolSize16Bit, True , CPU6809);
  AddALU("ADCD", 0x1089, eSymbolSize16Bit, True , CPU6309);
  AddALU("SUBD", 0x0083, eSymbolSize16Bit, True , CPU6809);
  AddALU("SBCD", 0x1082, eSymbolSize16Bit, True , CPU6309);
  AddALU("MULD", 0x118f, eSymbolSize16Bit, True , CPU6309);
  AddALU("DIVD", 0x118d, eSymbolSize16Bit, True , CPU6309);
  AddALU("ANDD", 0x1084, eSymbolSize16Bit, True , CPU6309);
  AddALU("ORD" , 0x108a, eSymbolSize16Bit, True , CPU6309);
  AddALU("EORD", 0x1088, eSymbolSize16Bit, True , CPU6309);
  AddALU("BITD", 0x1085, eSymbolSize16Bit, True , CPU6309);

  AddALU("LDW" , 0x1086, eSymbolSize16Bit, True , CPU6309);
  AddALU("STW" , 0x1087, eSymbolSize16Bit, False, CPU6309);
  AddALU("CMPW", 0x1081, eSymbolSize16Bit, True , CPU6309);
  AddALU("ADDW", 0x108b, eSymbolSize16Bit, True , CPU6309);
  AddALU("SUBW", 0x1080, eSymbolSize16Bit, True , CPU6309);

  AddALU("STQ" , 0x10cd, eSymbolSize16Bit, True , CPU6309);
  AddALU("DIVQ", 0x118e, eSymbolSize16Bit, True , CPU6309);

  AddALU("LDE" , 0x1186, eSymbolSize8Bit , True , CPU6309);
  AddALU("STE" , 0x1187, eSymbolSize8Bit , False, CPU6309);
  AddALU("CMPE", 0x1181, eSymbolSize8Bit , True , CPU6309);
  AddALU("ADDE", 0x118b, eSymbolSize8Bit , True , CPU6309);
  AddALU("SUBE", 0x1180, eSymbolSize8Bit , True , CPU6309);

  AddALU("LDF" , 0x11c6, eSymbolSize8Bit , True , CPU6309);
  AddALU("STF" , 0x11c7, eSymbolSize8Bit , False, CPU6309);
  AddALU("CMPF", 0x11c1, eSymbolSize8Bit , True , CPU6309);
  AddALU("ADDF", 0x11cb, eSymbolSize8Bit , True , CPU6309);
  AddALU("SUBF", 0x11c0, eSymbolSize8Bit , True , CPU6309);

  AddALU("LDX" , 0x008e, eSymbolSize16Bit, True , CPU6809);
  AddALU("STX" , 0x008f, eSymbolSize16Bit, False, CPU6809);
  AddALU("CMPX", 0x008c, eSymbolSize16Bit, True , CPU6809);

  AddALU("LDY" , 0x108e, eSymbolSize16Bit, True , CPU6809);
  AddALU("STY" , 0x108f, eSymbolSize16Bit, False, CPU6809);
  AddALU("CMPY", 0x108c, eSymbolSize16Bit, True , CPU6809);

  AddALU("LDU" , 0x00ce, eSymbolSize16Bit, True , CPU6809);
  AddALU("STU" , 0x00cf, eSymbolSize16Bit, False, CPU6809);
  AddALU("CMPU", 0x1183, eSymbolSize16Bit, True , CPU6809);

  AddALU("LDS" , 0x10ce, eSymbolSize16Bit, True , CPU6809);
  AddALU("STS" , 0x10cf, eSymbolSize16Bit, False, CPU6809);
  AddALU("CMPS", 0x118c, eSymbolSize16Bit, True , CPU6809);

  AddALU("JSR" , 0x008d, eSymbolSize16Bit, False, CPU6809);

  InstrZ = 0;
  AddALU2("ADD"); AddALU2("ADC");
  AddALU2("SUB"); AddALU2("SBC");
  AddALU2("AND"); AddALU2("OR" );
  AddALU2("EOR"); AddALU2("CMP");

  InstrZ = 0;
  AddRMW("NEG", 0x00, CPU6809);
  AddRMW("COM", 0x03, CPU6809);
  AddRMW("LSR", 0x04, CPU6809);
  AddRMW("ROR", 0x06, CPU6809);
  AddRMW("ASR", 0x07, CPU6809);
  AddRMW("ASL", 0x08, CPU6809);
  AddRMW("LSL", 0x08, CPU6809);
  AddRMW("ROL", 0x09, CPU6809);
  AddRMW("DEC", 0x0a, CPU6809);
  AddRMW("INC", 0x0c, CPU6809);
  AddRMW("TST", 0x0d, CPU6809);
  AddRMW("JMP", 0x0e, CPU6809);
  AddRMW("CLR", 0x0f, CPU6809);

  InstrZ = 0;
  AddFlag("CWAI" , 0x3c, True , CPU6809);
  AddFlag("ANDCC", 0x1c, True , CPU6809);
  AddFlag("ORCC" , 0x1a, False, CPU6809);

  InstrZ = 0;
  AddLEA("LEAX", 0x30, CPU6809);
  AddLEA("LEAY", 0x31, CPU6809);
  AddLEA("LEAS", 0x32, CPU6809);
  AddLEA("LEAU", 0x33, CPU6809);

  InstrZ = 0;
  AddImm("AIM", 0x02, CPU6309);
  AddImm("OIM", 0x01, CPU6309);
  AddImm("EIM", 0x05, CPU6309);
  AddImm("TIM", 0x0b, CPU6309);

  AddInstTable(InstTable, "PSHS", 0x34 | ((MomCPU == CPU6309) ? 0x100 : 0), DecodeStack_6809);
  AddInstTable(InstTable, "PULS", 0x35 | ((MomCPU == CPU6309) ? 0x100 : 0), DecodeStack_6809);
  AddInstTable(InstTable, "PSHU", 0x36 | ((MomCPU == CPU6309) ? 0x100 : 0), DecodeStack_6809);
  AddInstTable(InstTable, "PULU", 0x37 | ((MomCPU == CPU6309) ? 0x100 : 0), DecodeStack_6809);

  InstrZ = 0;
  AddInstTable(InstTable, "BAND" , InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BIAND", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BOR"  , InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BIOR" , InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BEOR" , InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BIEOR", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "LDBT" , InstrZ++, DecodeBit);
  AddInstTable(InstTable, "STBT" , InstrZ++, DecodeBit);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(RelOrders);
  order_array_free(ALUOrders);
  order_array_free(RMWOrders);
  order_array_free(FlagOrders);
  order_array_free(LEAOrders);
  order_array_free(ImmOrders);
}

/*-------------------------------------------------------------------------*/

static Boolean DecodeAttrPart_6809(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  /* deduce operand size No size is zero-length string -> '\0' */

  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_6809(void)
{
  tSymbolSize OpSize;

  CodeLen = 0;
  DontPrint = False;
  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize8Bit;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize, True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_6809(void)
{
  DPRValue = 0;
}

static Boolean IsDef_6809(void)
{
  return False;
}

static void SwitchTo_6809(void)
{
#define ASSUME09Count (sizeof(ASSUME09s) / sizeof(*ASSUME09s))
  static const ASSUMERec ASSUME09s[] =
  {
    { "DPR", &DPRValue, 0, 0xff, 0x100, NULL }
  };

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x63;
  NOPCode = 0x12;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  DecodeAttrPart = DecodeAttrPart_6809;
  MakeCode = MakeCode_6809;
  IsDef = IsDef_6809;

  SwitchFrom = DeinitFields;
  InitFields();
  AddMoto16PseudoONOFF(False);

  pASSUMERecs = ASSUME09s;
  ASSUMERecCnt = ASSUME09Count;
}

void code6809_init(void)
{
  CPU6809 = AddCPU("6809", SwitchTo_6809);
  CPU6309 = AddCPU("6309", SwitchTo_6809);

  AddInitPassProc(InitCode_6809);
}
