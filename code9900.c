/* code9900.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TMS99xx(x)                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "be_le.h"
#include "bpemu.h"
#include "nls.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmcode.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "codevars.h"
#include "ibmfloat.h"
#include "chartrans.h"
#include "errmsg.h"

#include "code9900.h"

#define CKPT_NOTHING 16

enum
{
  eCoreNone = 0,
  eCore990_10  = 1 << 0,
  eCore990_12  = 1 << 1,
  eCore9900    = 1 << 2,
  eCore9940    = 1 << 3,
  eCore9995    = 1 << 4,
  eCore99105   = 1 << 5,
  eCore99110   = 1 << 6,
  eCoreAll     = eCore990_10 | eCore990_12 | eCore9900 | eCore9940 | eCore9995 | eCore99105 | eCore99110,

  eCoreFlagSupMode = 1 << 7
};

typedef struct
{
  const char *pName;
  Byte CoreFlags;
} tCPUProps;

typedef struct
{
  Word Code;
  Byte Flags;
} tOrder;

static const tCPUProps *pCurrCPUProps;
static tOrder *SingOrders, *ImmOrders, *FixedOrders, *RegOrders,
              *Type11Orders, *Type11aOrders, *Type12Orders, *Type15Orders,
              *Type16Orders, *Type17Orders, *Type20Orders;
static Boolean IsWord;
static Word AdrVal, AdrPart, DefCkpt;

/*-------------------------------------------------------------------------*/
/* Adressparser */

static Boolean DecodeReg(const tStrComp *pArg, Word *Erg)
{
  Boolean OK;
  *Erg = EvalStrIntExpression(pArg, UInt4, &OK);
  return OK;
}

static char *HasDisp(char *Asc)
{
  char *p;
  int Lev, Len = strlen(Asc);

  if ((Len >= 2) && (Asc[Len - 1] == ')'))
  {
    p = Asc + Len - 2; Lev = 0;
    while ((p >= Asc) && (Lev != -1))
    {
      switch (*p)
      {
        case '(': Lev--; break;
        case ')': Lev++; break;
      }
      if (Lev != -1) p--;
    }
    if (Lev != -1)
    {
      WrError(ErrNum_BrackErr);
      p = NULL;
    }
  }
  else
    p = NULL;

  return p;
}

static Boolean DecodeAdr(const tStrComp *pArg)
{
  Boolean IncFlag;
  Boolean OK;
  char *p;

  AdrCnt = 0;

  if (*pArg->str.p_str == '*')
  {
    tStrComp IArg;

    StrCompRefRight(&IArg, pArg, 1);
    if (IArg.str.p_str[strlen(IArg.str.p_str) - 1] == '+')
    {
      IncFlag = True;
      StrCompShorten(&IArg, 1);
    }
    else
      IncFlag = False;
    if (DecodeReg(&IArg, &AdrPart))
    {
      AdrPart += 0x10 + (Ord(IncFlag) << 5);
      return True;
    }
    return False;
  }

  if (*pArg->str.p_str == '@')
  {
    tStrComp IArg;

    StrCompRefRight(&IArg, pArg, 1);
    p = HasDisp(IArg.str.p_str);
    if (!p)
    {
      tSymbolFlags Flags;

      AdrVal = EvalStrIntExpressionWithFlags(&IArg, UInt16, &OK, &Flags);
      if (OK)
      {
        AdrPart = 0x20;
        AdrCnt = 1;
        if (!mFirstPassUnknown(Flags) && IsWord && Odd(AdrVal))
          WrError(ErrNum_AddrNotAligned);
        return True;
      }
    }
    else
    {
      tStrComp Disp, Reg;

      StrCompSplitRef(&Disp, &Reg, &IArg, p);
      StrCompShorten(&Reg, 1);
      if (DecodeReg(&Reg, &AdrPart))
      {
        if (AdrPart == 0) WrStrErrorPos(ErrNum_InvReg, &Reg);
        else
        {
          AdrVal = EvalStrIntExpression(&Disp, Int16, &OK);
          if (OK)
          {
            AdrPart += 0x20;
            AdrCnt = 1;
            return True;
          }
        }
      }
    }
    return False;
  }

  if (DecodeReg(pArg, &AdrPart))
    return True;
  else
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
}

static Boolean DecodeBitField(tStrComp *pArg, Word *pResult)
{
  tStrComp Arg, Start, Count;
  Boolean OK1, OK2;
  char *pSep;

  if (!IsIndirect(pArg->str.p_str))
  {
    WrStrErrorPos(ErrNum_InvBitField, pArg);
    return False;
  }
  StrCompRefRight(&Arg, pArg, 1);
  StrCompShorten(&Arg, 1);
  pSep = strchr(Arg.str.p_str, ',');
  if (!pSep)
  {
    WrStrErrorPos(ErrNum_InvBitField, pArg);
    return False;
  }
  StrCompSplitRef(&Start, &Count, &Arg, pSep);
  *pResult = EvalStrIntExpression(&Count, UInt4, &OK1)
           | (EvalStrIntExpression(&Start, UInt4, &OK2) << 12);
  return OK1 && OK2;
}

static void PutByte(Byte Value)
{
  if ((CodeLen & 1) && !HostBigEndian)
  {
    BAsmCode[CodeLen] = BAsmCode[CodeLen - 1];
    BAsmCode[CodeLen - 1] = Value;
  }
  else
  {
    BAsmCode[CodeLen] = Value;
  }
  CodeLen++;
}

static Boolean EvalDist(const tStrComp *pArg, Word *pResult, Word InstrLen)
{
  Boolean OK;
  Integer AdrInt;
  tSymbolFlags Flags;

  AdrInt = EvalStrIntExpressionOffsWithFlags(pArg, !!(*pArg->str.p_str == '@'), UInt16, &OK, &Flags) - (EProgCounter() + InstrLen);
  if (OK && !mSymbolQuestionable(Flags) && Odd(AdrInt))
  {
    WrStrErrorPos(ErrNum_DistIsOdd, pArg);
    OK = False;
  }
  if (OK && !mSymbolQuestionable(Flags) && ((AdrInt < -256) || (AdrInt > 254)))
  {
    WrStrErrorPos(ErrNum_JmpDistTooBig, pArg);
    OK = False;
  }
  if (OK)
    *pResult = (AdrInt / 2) & 0xff;
  return OK;
}

static Boolean DecodeCond(const tStrComp *pArg, Word *pResult)
{
  const char *pConds[] =
    { "EQ", "NE", "HE", "L", "GE", "LT", "LE", "H", "LTE", "GT", NULL },
  **pRun;

  for (pRun = pConds; *pRun; pRun++)
    if (!as_strcasecmp(pArg->str.p_str, *pRun))
    {
      *pResult = pRun - pConds;
      return True;
    }
  WrStrErrorPos(ErrNum_UndefCond, pArg);
  return False;
}

/*-------------------------------------------------------------------------*/
/* Code Generators */

static void CheckSupMode(void)
{
  if (!SupAllowed && (pCurrCPUProps->CoreFlags & eCoreFlagSupMode))
    WrError(ErrNum_PrivOrder);
}

static Boolean CheckCore(Byte CoreReqFlags)
{
  if (!(CoreReqFlags & pCurrCPUProps->CoreFlags))
  {
    WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
    return False;
  }
  else
    return True;
}

static Boolean CheckNotMode3(const tStrComp *pArg)
{
  if ((AdrPart & 0x30) == 0x30)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    return False;
  }
  else
    return True;
}

static void DecodeTwo(Word Code)
{
  Word HPart;

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1]))
  {
    WAsmCode[0] = AdrPart;
    WAsmCode[1] = AdrVal;
    HPart = AdrCnt;
    if (DecodeAdr(&ArgStr[2]))
    {
      WAsmCode[0] += AdrPart << 6;
      WAsmCode[1 + HPart] = AdrVal;
      CodeLen = (1 + HPart + AdrCnt) << 1;
      WAsmCode[0] += Code;
    }
  }
}

static void DecodeType11(Word Index)
{
  const tOrder *pOrder = &Type11Orders[Index];
  Word AdrCnt1;

  if (ChkArgCnt(2, (pCurrCPUProps->CoreFlags & eCore990_12) ? 3 : 2)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeAdr(&ArgStr[1]))
  {
    Word Count;
    Boolean OK;

    if (ArgCnt == 3)
      Count = EvalStrIntExpression(&ArgStr[3], UInt4, &OK);
    else
    {
      Count = 4;
      OK = True;
    }
    if (OK)
    {
      WAsmCode[0] = pOrder->Code;
      WAsmCode[1] = (Count << 12) | AdrPart;
      WAsmCode[2] = AdrVal;
      AdrCnt1 = AdrCnt;
      if (DecodeAdr(&ArgStr[2]))
      {
        WAsmCode[1] |= AdrPart << 6;
        WAsmCode[2 + AdrCnt1] = AdrVal;
        CodeLen = (2 + AdrCnt1 + AdrCnt) << 1;
      }
    }
  }
}

static void DecodeType11a(Word Index)
{
  const tOrder *pOrder = &Type11aOrders[Index];
  Word AdrCnt1;

  if (ChkArgCnt(2, 2)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeAdr(&ArgStr[1]))
  {
    WAsmCode[0] = pOrder->Code;
    WAsmCode[1] = AdrPart;
    WAsmCode[2] = AdrVal;
    AdrCnt1 = AdrCnt;
    if (DecodeAdr(&ArgStr[2]))
    {
      WAsmCode[1] |= AdrPart << 6;
      WAsmCode[2 + AdrCnt1] = AdrVal;
      CodeLen = (2 + AdrCnt1 + AdrCnt) << 1;
    }
  }
}

static void DecodeType12(Word Index)
{
  const tOrder *pOrder = &Type12Orders[Index];
  Word AdrCnt1, Count, Ckpt;
  Boolean OK;

  if (!ChkArgCnt(2, 4)
   || !CheckCore(pOrder->Flags & eCoreAll)
   || !DecodeAdr(&ArgStr[1]))
    return;
  WAsmCode[0] = pOrder->Code;
  WAsmCode[1] =  AdrPart;
  WAsmCode[2] = AdrVal;
  AdrCnt1 = AdrCnt;

  if (!DecodeAdr(&ArgStr[2]))
    return;
  WAsmCode[1] |= AdrPart << 6;
  WAsmCode[2 + AdrCnt1] = AdrVal;

  if ((ArgCnt < 3) || !*ArgStr[3].str.p_str)
  {
    Count = 0;
    OK = True;
  }
  else
    Count = EvalStrIntExpression(&ArgStr[3], UInt4, &OK);
  if (!OK)
    return;
  WAsmCode[1] |= Count << 12;

  if ((ArgCnt < 4) || !*ArgStr[4].str.p_str)
  {
    if (DefCkpt >= 16)
    {
      WrError(ErrNum_NoDefCkptReg);
      OK = False;
    }
    else
    {
      Ckpt = DefCkpt;
      OK = True;
    }
  }
  else
    OK = DecodeReg(&ArgStr[4], &Ckpt);
  if (!OK)
    return;
  WAsmCode[0] |= Ckpt;
  CodeLen = (2 + AdrCnt1 + AdrCnt) << 1;
}

static void DecodeOne(Word Code)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1]))
  {
    Word HPart;

    WAsmCode[0] = AdrPart;
    WAsmCode[1] = AdrVal;
    if (DecodeReg(&ArgStr[2], &HPart))
    {
      WAsmCode[0] += (HPart << 6) + (Code << 10);
      CodeLen = (1 + AdrCnt) << 1;
    }
  }
}

static void DecodeType15(Word Index)
{
  const tOrder *pOrder = &Type15Orders[Index];
  Word BitField;

  if (ChkArgCnt(2, 2)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeAdr(&ArgStr[1])
   && DecodeBitField(&ArgStr[2], &BitField))
  {
    WAsmCode[0] = pOrder->Code | (BitField & 0x000f);
    WAsmCode[1] = (BitField & 0xf000) | AdrPart;
    WAsmCode[2] = AdrVal;
    CodeLen = (2 + AdrCnt) << 1;
  }
}

static void DecodeType16(Word Index)
{
  const tOrder *pOrder = &Type16Orders[Index];
  Word BitField;

  if (ChkArgCnt(3, 3)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeAdr(&ArgStr[1])
   && DecodeBitField(&ArgStr[3], &BitField))
  {
    Word AdrCnt1 = AdrCnt;

    WAsmCode[0] = pOrder->Code | (BitField & 0x000f);
    WAsmCode[1] = (BitField & 0xf000) | AdrPart;
    WAsmCode[2] = AdrVal;
    if (DecodeAdr(&ArgStr[2]))
    {
      WAsmCode[1] |= (AdrPart << 6);
      WAsmCode[2 + AdrCnt1] = AdrVal;
      CodeLen = (2 + AdrCnt + AdrCnt1) << 1;
    }
  }
}

static void DecodeMOVA(Word Code)
{
  if (ChkArgCnt(2, 2)
   && CheckCore(eCore990_12)
   && DecodeAdr(&ArgStr[1]))
  {
    Word AdrCnt1 = AdrCnt;

    WAsmCode[0] = Code;
    WAsmCode[1] = AdrPart;
    WAsmCode[2] = AdrVal;
    if (DecodeAdr(&ArgStr[2]))
    {
      WAsmCode[1] |= (AdrPart << 6);
      WAsmCode[2 + AdrCnt1] = AdrVal;
      CodeLen = (2 + AdrCnt + AdrCnt1) << 1;
    }
  }
}

static void DecodeType20(Word Index)
{
  const tOrder *pOrder = &Type20Orders[Index];
  Word Condition;

  if (ChkArgCnt(3, 3)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeCond(&ArgStr[1], &Condition)
   && DecodeAdr(&ArgStr[2]))
  {
    Word AdrCnt1 = AdrCnt;

    WAsmCode[0] = pOrder->Code;
    WAsmCode[1] = AdrPart | (Condition << 12);
    WAsmCode[2] = AdrVal;
    if (DecodeAdr(&ArgStr[3]))
    {
      WAsmCode[1] |= (AdrPart << 6);
      WAsmCode[2 + AdrCnt1] = AdrVal;
      CodeLen = (2 + AdrCnt + AdrCnt1) << 1;
    }
  }
}

static void DecodeType17(Word Index)
{
  const tOrder *pOrder = &Type17Orders[Index];
  Word Reg;

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[ArgCnt], &Reg))
  {
    Word Delta, Dist;
    Boolean OK;

    if ((ArgCnt == 3) && *ArgStr[2].str.p_str)
      Delta = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
    else
    {
      Delta = 1;
      OK = True;
    }
    if (OK && EvalDist(&ArgStr[1], &Dist, 4))
    {
      WAsmCode[0] = pOrder->Code;
      WAsmCode[1] = (Dist & 0xff) | (Reg << 8) | (Delta << 12);
      CodeLen = 4;
    }
  }
}

static void DecodeEP(Word Code)
{
  if (ChkArgCnt(4, 4)
   && CheckCore(eCore990_12)
   && DecodeAdr(&ArgStr[1]))
  {
    Word AdrCnt1 = AdrCnt, SrcCount, DestCount;
    Boolean DestOK, SrcOK;

    WAsmCode[0] = Code;
    WAsmCode[1] = AdrPart;
    WAsmCode[2] = AdrVal;

    SrcCount = EvalStrIntExpression(&ArgStr[3], UInt4, &SrcOK);
    DestCount = EvalStrIntExpression(&ArgStr[4], UInt4, &DestOK);

    if (SrcOK && DestOK && DecodeAdr(&ArgStr[2]))
    {
      WAsmCode[0] |= DestCount;
      WAsmCode[1] |= (AdrPart << 6) | (SrcCount << 12);
      WAsmCode[2 + AdrCnt1] = AdrVal;
      CodeLen = (2 + AdrCnt + AdrCnt1) << 1;
    }
  }
}

static void DecodeLDCR_STCR(Word Code)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1]))
  {
    Word HPart;
    Boolean OK;
    tSymbolFlags Flags;

    WAsmCode[0] = Code + AdrPart;
    WAsmCode[1] = AdrVal;
    HPart = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt5, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      HPart = 1;
    if (OK)
    {
      if (ChkRange(HPart, 1, 16))
      {
        WAsmCode[0] += (HPart & 15) << 6;
        CodeLen = (1 + AdrCnt) << 1;
      }
    }
  }
  return;
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(2, 2)
   && DecodeReg(&ArgStr[1], WAsmCode + 0))
  {
    Word HPart;

    if (DecodeReg(&ArgStr[2], &HPart))
    {
      WAsmCode[0] += (HPart << 4) + (Code << 8);
      CodeLen = 2;
    }
  }
}

static void DecodeSLAM_SRAM(Word Code)
{
  Word HPart;

  if (ChkArgCnt(2, 2)
   && CheckCore(eCore990_12 | eCore99105 | eCore99110)
   && DecodeReg(&ArgStr[2], &HPart)
   && DecodeAdr(&ArgStr[1]))
  {
    WAsmCode[0] = Code;
    WAsmCode[1] = 0x4000 | (HPart << 6) | AdrPart;
    WAsmCode[2] = AdrVal;
    CodeLen = (2 + AdrCnt) << 1;
  }
}

static void DecodeImm(Word Index)
{
  const tOrder *pOrder = &ImmOrders[Index];

  if (ChkArgCnt(2, 2)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeReg(&ArgStr[1], WAsmCode + 0))
  {
    Boolean OK;

    WAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      WAsmCode[0] |= pOrder->Code;
      CodeLen = 4;
    }
  }
}

static void DecodeLIIM(Word Code)
{
  if (ChkArgCnt(1, 1)
   && CheckCore(eCore9940))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
    {
      WAsmCode[0] |= Code;
      CodeLen = 2;
    }
  }
}

static void DecodeRegOrder(Word Index)
{
  const tOrder *pOrder = &RegOrders[Index];

  if (ChkArgCnt(1, 1)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeReg(&ArgStr[1], WAsmCode + 0))
  {
    WAsmCode[0] |= pOrder->Code;
    CodeLen = 2;
  }
}

static void DecodeLMF(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && CheckCore(eCore990_10 | eCore990_12 | eCore99105 | eCore99110)
   && DecodeReg(&ArgStr[1], WAsmCode + 0))
  {
    Boolean OK;

    WAsmCode[0] += 0x320 + (EvalStrIntExpression(&ArgStr[2], UInt1, &OK) << 4);
    if (OK)
      CodeLen = 2;
    CheckSupMode();
  }
}

static void DecodeMPYS_DIVS(Word Code)
{
  if (ChkArgCnt(1, 1)
   && CheckCore(eCore990_12 | eCore9995 | eCore99105 | eCore99110)
   && DecodeAdr(&ArgStr[1]))
  {
    WAsmCode[0] = Code + AdrPart;
    WAsmCode[1] = AdrVal;
    CodeLen = (1 + AdrCnt) << 1;
  }
}

static void DecodeSBit(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], SInt8, &OK);
    if (OK)
    {
      WAsmCode[0] = (WAsmCode[0] & 0xff) | Code;
      CodeLen = 2;
    }
  }
}

static void DecodeBit(Word Code)
{
  if (ChkArgCnt(2, 2)
   && CheckCore(eCore990_12 | eCore99105 | eCore99110)
   && DecodeAdr(&ArgStr[1])
   && CheckNotMode3(&ArgStr[1]))
  {
    Boolean OK;

    WAsmCode[1] = (EvalStrIntExpression(&ArgStr[2], UInt4, &OK) << 6) | AdrPart;
    if (OK)
    {
      WAsmCode[0] = Code;
      WAsmCode[2] = AdrVal;
      CodeLen = (2 + AdrCnt) << 1;
    }
  }
}

static void DecodeJmp(Word Code)
{
  Word Dist;

  if (ChkArgCnt(1, 1)
   && EvalDist(&ArgStr[1], &Dist, 2))
  {
    WAsmCode[0] = Code | Dist;
    CodeLen = 2;
  }
}

static void DecodeLWPI_LIMI(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt16, &OK);
    if (OK)
    {
      WAsmCode[0] = Code & 0x7fff;
      CodeLen = 4;
      if (Code & 0x8000)
        CheckSupMode();
    }
  }
}

static void DecodeSing(Word Index)
{
  const tOrder *pOrder = &SingOrders[Index];

  if (ChkArgCnt(1, 1)
   && CheckCore(pOrder->Flags & eCoreAll)
   && DecodeAdr(&ArgStr[1]))
  {
    WAsmCode[0] = pOrder->Code | AdrPart;
    WAsmCode[1] = AdrVal;
    CodeLen = (1 + AdrCnt) << 1;
    if (pOrder->Flags & eCoreFlagSupMode)
      CheckSupMode();
  }
}

static void DecodeFixed(Word Index)
{
  const tOrder *pOrder = &FixedOrders[Index];

  if (ChkArgCnt(0, 0)
   && CheckCore(pOrder->Flags & eCoreAll))
  {
    WAsmCode[0] = pOrder->Code;
    CodeLen = 2;
    if (pOrder->Flags & eCoreFlagSupMode)
      CheckSupMode();
  }
}

static void DecodeRTWP(Word Code)
{
  Word Variant;
  Word MaxVariant = (((pCurrCPUProps->CoreFlags & eCoreAll) == eCore99105)
                  || ((pCurrCPUProps->CoreFlags & eCoreAll) == eCore99110)) ? 4 : 0;
  tEvalResult EvalResult;

  switch (ArgCnt)
  {
    case 0:
      Variant = 0;
      EvalResult.OK = True;
      break;
    case 1:
      Variant = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);
      if (!EvalResult.OK)
        return;
      if (mFirstPassUnknown(EvalResult.Flags))
        Variant = 0;
      else if (Variant > MaxVariant)
      {
        WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
        return;
      }
      else if (Variant == 3)
      {
        WrStrErrorPos(ErrNum_InvArg, &ArgStr[1]);
        return;
      }
      break;
    default:
      (void)ChkArgCnt(0,1);
      return;
  }
  if (EvalResult.OK)
  {
    WAsmCode[CodeLen >> 1] = Code + Variant;
    CodeLen += 2;
  }
}

static void DecodeBYTE(Word Code)
{
  TempResult t;

  UNUSED(Code);

  as_tempres_ini(&t);
  if (ChkArgCnt(1, ArgCntMax))
  {
    Boolean OK = True;
    tStrComp *pArg;

    forallargs(pArg, OK)
    {
      KillBlanks(pArg->str.p_str);
      EvalStrExpression(pArg, &t);
      switch (t.Typ)
      {
        case TempInt:
          if (mFirstPassUnknown(t.Flags))
            t.Contents.Int &= 0xff;
          if (!RangeCheck(t.Contents.Int, Int8)) WrStrErrorPos(ErrNum_OverRange, pArg);
          else if (SetMaxCodeLen(CodeLen + 1))
          {
            WrStrErrorPos(ErrNum_CodeOverflow, pArg);
            OK = False;
          }
          else
            PutByte(t.Contents.Int);
          break;
        case TempString:
          if (SetMaxCodeLen(t.Contents.str.len + CodeLen))
          {
            WrStrErrorPos(ErrNum_CodeOverflow, pArg);
            OK = False;
          }
          else
          {
            char *p, *pEnd;

            if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
              OK = False;
            else
            {
              pEnd = t.Contents.str.p_str + t.Contents.str.len;
              for (p = t.Contents.str.p_str; p < pEnd; PutByte(*(p++)));
            }
          }
          break;
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          /* fall-through */
        default:
          OK = False;
      }
    }
    if (!OK)
      CodeLen = 0;
  }
  as_tempres_free(&t);
}

static void DecodeWORD(Word Code)
{
  int z;
  Boolean OK;
  Word HVal16;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    z = 1;
    OK = True;
    do
    {
      HVal16 = EvalStrIntExpression(&ArgStr[z], Int16, &OK);
      if (OK)
      {
        WAsmCode[CodeLen >> 1] = HVal16;
        CodeLen += 2;
      }
      z++;
    }
    while ((z <= ArgCnt) && (OK));
    if (!OK)
      CodeLen = 0;
  }
}

static void DecodeFLOAT(Word DestLen)
{
  int z;
  Boolean OK;
  double FVal;

  if (ChkArgCnt(1, ArgCntMax))
  {
    z = 1;
    OK = True;
    do
    {
      FVal = EvalStrFloatExpression(&ArgStr[z], Float64, &OK);
      if (OK)
      {
        SetMaxCodeLen(CodeLen + DestLen);
        if (Double2IBMFloat(&WAsmCode[CodeLen >> 1], FVal, DestLen == 8))
          CodeLen += DestLen;
      }
      z++;
    }
    while ((z <= ArgCnt) && (OK));
    if (!OK)
      CodeLen = 0;
  }
}

static void DecodeBSS(Word Code)
{
  Boolean OK;
  Word HVal16;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    HVal16 = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    else if (OK)
    {
      if (!HVal16) WrError(ErrNum_NullResMem);
      DontPrint = True;
      CodeLen = HVal16;
      BookKeeping();
    }
  }
}

static void DecodeCKPT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && CheckCore(eCore990_12))
  {
    Word NewDefCkpt;

    if (!as_strcasecmp(ArgStr[1].str.p_str, "NOTHING"))
      DefCkpt = CKPT_NOTHING;
    else
      if (DecodeReg(&ArgStr[1], &NewDefCkpt))
        DefCkpt = NewDefCkpt;
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Belegung/Freigabe Codetabellen */

static void AddTwo(const char *NName16, const char *NName8, Word NCode)
{
  AddInstTable(InstTable, NName16, (NCode << 13)         , DecodeTwo);
  AddInstTable(InstTable, NName8,  (NCode << 13) + 0x1000, DecodeTwo);
}

static void AddOne(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeOne);
}

static void AddSing(const char *NName, Word NCode, Byte Flags)
{
  order_array_rsv_end(SingOrders, tOrder);
  SingOrders[InstrZ].Code = NCode;
  SingOrders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeSing);
}

static void AddSBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode << 8, DecodeSBit);
}

static void AddBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void AddJmp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode << 8, DecodeJmp);
}

static void AddShift(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeShift);
}

static void AddImm(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(ImmOrders, tOrder);
  ImmOrders[InstrZ].Code = NCode << 4;
  ImmOrders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeImm);
}

static void AddReg(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(RegOrders, tOrder);
  RegOrders[InstrZ].Code = NCode << 4;
  RegOrders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRegOrder);
}

static void AddFixed(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(FixedOrders, tOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddType11(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type11Orders, tOrder);
  Type11Orders[InstrZ].Code = NCode;
  Type11Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType11);
}

static void AddType11a(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type11aOrders, tOrder);
  Type11aOrders[InstrZ].Code = NCode;
  Type11aOrders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType11a);
}

static void AddType12(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type12Orders, tOrder);
  Type12Orders[InstrZ].Code = NCode;
  Type12Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType12);
}

static void AddType15(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type15Orders, tOrder);
  Type15Orders[InstrZ].Code = NCode;
  Type15Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType15);
}

static void AddType16(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type16Orders, tOrder);
  Type16Orders[InstrZ].Code = NCode;
  Type16Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType16);
}

static void AddType17(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type17Orders, tOrder);
  Type17Orders[InstrZ].Code = NCode;
  Type17Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType17);
}

static void AddType20(const char *NName, Word NCode, Word Flags)
{
  order_array_rsv_end(Type20Orders, tOrder);
  Type20Orders[InstrZ].Code = NCode;
  Type20Orders[InstrZ].Flags = Flags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeType20);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "LDCR", 0x3000, DecodeLDCR_STCR);
  AddInstTable(InstTable, "STCR", 0x3400, DecodeLDCR_STCR);
  AddInstTable(InstTable, "LMF", 0, DecodeLMF);
  AddInstTable(InstTable, "MPYS", 0x01c0, DecodeMPYS_DIVS);
  AddInstTable(InstTable, "DIVS", 0x0180, DecodeMPYS_DIVS);
  AddInstTable(InstTable, "LWPI", 0x02e0, DecodeLWPI_LIMI);
  AddInstTable(InstTable, "LIMI", 0x8300, DecodeLWPI_LIMI);
  AddInstTable(InstTable, "WORD", 0, DecodeWORD);
  AddInstTable(InstTable, "SINGLE", 4, DecodeFLOAT);
  AddInstTable(InstTable, "DOUBLE", 8, DecodeFLOAT);
  AddInstTable(InstTable, "CKPT", 0, DecodeCKPT);

  AddTwo("A"   , "AB"   , 5); AddTwo("C"   , "CB"   , 4); AddTwo("S"   , "SB"   , 3);
  AddTwo("SOC" , "SOCB" , 7); AddTwo("SZC" , "SZCB" , 2); AddTwo("MOV" , "MOVB" , 6);

  AddOne("COC" , 0x08); AddOne("CZC" , 0x09); AddOne("XOR" , 0x0a);
  AddOne("MPY" , 0x0e); AddOne("DIV" , 0x0f); AddOne("XOP" , 0x0b);

  InstrZ = 0;
  AddSing("B"   , 0x0440, eCoreAll);
  AddSing("BL"  , 0x0680, eCoreAll);
  AddSing("BLWP", 0x0400, eCoreAll);
  AddSing("CLR" , 0x04c0, eCoreAll);
  AddSing("SETO", 0x0700, eCoreAll);
  AddSing("INV" , 0x0540, eCoreAll);
  AddSing("NEG" , 0x0500, eCoreAll);
  AddSing("ABS" , 0x0740, eCoreAll);
  AddSing("SWPB", 0x06c0, eCoreAll);
  AddSing("INC" , 0x0580, eCoreAll);
  AddSing("INCT", 0x05c0, eCoreAll);
  AddSing("DEC" , 0x0600, eCoreAll);
  AddSing("DECT", 0x0640, eCoreAll);
  AddSing("X"   , 0x0480, eCoreAll);
  AddSing("LDS" , 0x0780, eCoreFlagSupMode | eCore990_10 | eCore990_12 | eCore99110);
  AddSing("LDD" , 0x07c0, eCoreFlagSupMode | eCore990_10 | eCore990_12 | eCore99110);
  AddSing("DCA" , 0x2c00, eCore9940);
  AddSing("DCS" , 0x2c40, eCore9940);
  AddSing("BIND", 0x0140, eCore990_12 | eCore99105 | eCore99110);
  AddSing("AR"  , 0x0c40, eCore990_12 | eCore99110);
  AddSing("SR"  , 0x0cc0, eCore990_12 | eCore99110);
  AddSing("MR"  , 0x0d00, eCore990_12 | eCore99110);
  AddSing("DR"  , 0x0d40, eCore990_12 | eCore99110);
  AddSing("LR"  , 0x0d80, eCore990_12 | eCore99110);
  AddSing("STR" , 0x0dc0, eCore990_12 | eCore99110);
  AddSing("CIR" , 0x0c80, eCore990_12 | eCore99110);
  AddSing("EVAD", 0x0100, eCore99105 | eCore99110);
  AddSing("AD"  , 0x0e40, eCore990_12);
  AddSing("CID" , 0x0e80, eCore990_12);
  AddSing("SD"  , 0x0ec0, eCore990_12);
  AddSing("MD"  , 0x0f00, eCore990_12);
  AddSing("DD"  , 0x0f40, eCore990_12);
  AddSing("LD"  , 0x0f80, eCore990_12);
  AddSing("STD" , 0x0fc0, eCore990_12);

  AddSBit("SBO" , 0x1d); AddSBit("SBZ", 0x1e); AddSBit("TB" , 0x1f);

  AddJmp("JEQ", 0x13); AddJmp("JGT", 0x15); AddJmp("JH" , 0x1b);
  AddJmp("JHE", 0x14); AddJmp("JL" , 0x1a); AddJmp("JLE", 0x12);
  AddJmp("JLT", 0x11); AddJmp("JMP", 0x10); AddJmp("JNC", 0x17);
  AddJmp("JNE", 0x16); AddJmp("JNO", 0x19); AddJmp("JOC", 0x18);
  AddJmp("JOP", 0x1c);

  AddShift("SLA", 0x0a); AddShift("SRA", 0x08);
  AddShift("SRC", 0x0b); AddShift("SRL", 0x09);

  InstrZ = 0;
  AddImm("AI"  , 0x022, eCoreAll);
  AddImm("ANDI", 0x024, eCoreAll);
  AddImm("CI"  , 0x028, eCoreAll);
  AddImm("LI"  , 0x020, eCoreAll);
  AddImm("ORI" , 0x026, eCoreAll);
  AddImm("BLSK", 0x00b, eCore990_12 | eCore99105 | eCore99110);

  InstrZ = 0;
  AddType11("AM"  , 0x002a, eCore990_12 | eCore99105 | eCore99110);
  AddType11("SM"  , 0x0029, eCore990_12 | eCore99105 | eCore99110);
  AddType11("NRM" , 0x0c08, eCore990_12);
  AddType11("RTO" , 0x001e, eCore990_12);
  AddType11("LTO" , 0x001f, eCore990_12);
  AddType11("CNTO", 0x0020, eCore990_12);
  AddType11("BDC" , 0x0023, eCore990_12);
  AddType11("DBC" , 0x0024, eCore990_12);
  AddType11("SWPM", 0x0025, eCore990_12);
  AddType11("XORM", 0x0026, eCore990_12);
  AddType11("ORM" , 0x0027, eCore990_12);
  AddType11("ANDM", 0x0028, eCore990_12);

  InstrZ = 0;
  AddType11a("CR"  , 0x0301, eCore99110);
  AddType11a("MM"  , 0x0302, eCore99110);

  AddInstTable(InstTable, "SLAM", 0x001d, DecodeSLAM_SRAM);
  AddInstTable(InstTable, "SRAM", 0x001c, DecodeSLAM_SRAM);

  InstrZ = 0;
  AddReg("STST", 0x02c, eCoreAll);
  AddReg("LST" , 0x008, eCore9995 | eCore99105 | eCore99110 | eCore990_12);
  AddReg("STWP", 0x02a, eCoreAll);
  AddReg("LWP" , 0x009, eCore9995 | eCore99105 | eCore99110 | eCore990_12);
  AddReg("STPC", 0x003, eCore990_12);
  AddReg("LIM" , 0x007, eCore990_12);
  AddReg("LCS" , 0x00a, eCore990_12);

  AddBit("TMB" , 0x0c09);
  AddBit("TCMB", 0x0c0a);
  AddBit("TSMB", 0x0c0b);

  InstrZ = 0;
  AddInstTable(InstTable, "RTWP", 0x0380, DecodeRTWP);
  AddFixed("IDLE", 0x0340, eCoreFlagSupMode | eCoreAll);
  AddFixed("RSET", 0x0360, eCoreFlagSupMode | (eCoreAll & ~eCore9940));
  AddFixed("CKOF", 0x03c0, eCoreFlagSupMode | (eCoreAll & ~eCore9940));
  AddFixed("CKON", 0x03a0, eCoreFlagSupMode | (eCoreAll & ~eCore9940));
  AddFixed("LREX", 0x03e0, eCoreFlagSupMode | (eCoreAll & ~eCore9940));
  AddFixed("CER" , 0x0c06, eCore990_12 | eCore99110);
  AddFixed("CRE" , 0x0c04, eCore990_12 | eCore99110);
  AddFixed("NEGR", 0x0c02, eCore990_12 | eCore99110);
  AddFixed("CRI" , 0x0c00, eCore990_12 | eCore99110);
  AddFixed("EMD" , 0x002d, eCore990_12);
  AddFixed("EINT", 0x002e, eCore990_12);
  AddFixed("DINT", 0x002f, eCore990_12);
  AddFixed("CDI" , 0x0c01, eCore990_12);
  AddFixed("NEGD", 0x0c03, eCore990_12);
  AddFixed("CDE" , 0x0c05, eCore990_12);
  AddFixed("CED" , 0x0c07, eCore990_12);
  AddFixed("XIT" , 0x0c0e, eCore990_12);

  InstrZ = 0;
  AddType12("SNEB", 0x0e10, eCore990_12);
  AddType12("CRC" , 0x0e20, eCore990_12);
  AddType12("TS"  , 0x0e30, eCore990_12);
  AddType12("CS"  , 0x0040, eCore990_12);
  AddType12("SEQB", 0x0050, eCore990_12);
  AddType12("MOVS", 0x0060, eCore990_12);
  AddType12("MVSR", 0x00c0, eCore990_12);
  AddType12("MVSK", 0x00d0, eCore990_12);
  AddType12("POPS", 0x00e0, eCore990_12);
  AddType12("PSHS", 0x00f0, eCore990_12);

  InstrZ = 0;
  AddType15("IOF" , 0x0e00, eCore990_12);

  InstrZ = 0;
  AddType16("INSF", 0x0c10, eCore990_12);
  AddType16("XV"  , 0x0c30, eCore990_12);
  AddType16("XF"  , 0x0c20, eCore990_12);

  InstrZ = 0;
  AddType17("SRJ" , 0x0c0c, eCore990_12);
  AddType17("ARJ" , 0x0c0d, eCore990_12);

  AddInstTable(InstTable, "MOVA", 0x002b, DecodeMOVA);

  InstrZ = 0;
  AddType20("SLSL", 0x0021, eCore990_12);
  AddType20("SLSP", 0x0020, eCore990_12);

  AddInstTable(InstTable, "EP" , 0x03f0, DecodeEP);

  AddInstTable(InstTable, "LIIM", 0x2c80, DecodeLIIM);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(SingOrders);
  order_array_free(ImmOrders);
  order_array_free(FixedOrders);
  order_array_free(RegOrders);
  order_array_free(Type11Orders);
  order_array_free(Type11aOrders);
  order_array_free(Type12Orders);
  order_array_free(Type15Orders);
  order_array_free(Type16Orders);
  order_array_free(Type17Orders);
  order_array_free(Type20Orders);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_9900(void)
{
  CodeLen = 0;
  DontPrint = False;
  IsWord = False;

  /* to be ignored */

  if (Memo("")) return;

  /* may be aligned arbitrarily */

  if (Memo("BYTE"))
  {
    DecodeBYTE(0);
    return;
  }
  if (Memo("BSS"))
  {
    DecodeBSS(0);
    return;
  }

  /* For all other (pseudo) instructions, optionally pad to even */

  if (Odd(EProgCounter()))
  {
    if (DoPadding)
      InsertPadding(1, False);
    else
      WrError(ErrNum_AddrNotAligned);
  }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_9900(void)
{
  return False;
}

static void InternSymbol_9900(char *Asc, TempResult*Erg)
{
  Boolean OK;
  char *h = Asc;
  LargeInt Num;

  as_tempres_set_none(Erg);
  if ((strlen(Asc) >= 2) && (as_toupper(*Asc) == 'R'))
    h = Asc + 1;
  else if ((strlen(Asc) >= 3) && (as_toupper(*Asc) == 'W') && (as_toupper(Asc[1]) == 'R'))
    h = Asc + 2;

  Num = ConstLongInt(h, &OK, 10);
  if (!OK || (Num < 0) || (Num > 15))
    return;

  as_tempres_set_int(Erg, Num);
}

static void SwitchTo_9900(void *pUser)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x48;
  NOPCode = 0x0000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  pCurrCPUProps = (const tCPUProps*)pUser;

  MakeCode = MakeCode_9900;
  IsDef = IsDef_9900;
  SwitchFrom = DeinitFields;
  InternSymbol = InternSymbol_9900;
  AddONOFF(DoPaddingName, &DoPadding , DoPaddingName , False);
  onoff_supmode_add();

  InitFields();
}

static void InitCode_TI990(void)
{
  DefCkpt = CKPT_NOTHING; /* CKPT NOTHING */
}

static const tCPUProps CPUProps[] =
{
  { "TI990/4"  , eCore9900                      },
  { "TI990/10" , eCore990_10 | eCoreFlagSupMode },
  { "TI990/12" , eCore990_12 | eCoreFlagSupMode },
  { "TMS9900"  , eCore9900                      },
  { "TMS9940"  , eCore9940                      },
  { "TMS9995"  , eCore9995                      },
  { "TMS99105" , eCore99105  | eCoreFlagSupMode },
  { "TMS99110" , eCore99110  | eCoreFlagSupMode },
  { NULL       , eCoreNone                      },
};

void code9900_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->pName; pProp++)
    (void)AddCPUUser(pProp->pName, SwitchTo_9900, (void*)pProp, NULL);

  AddInitPassProc(InitCode_TI990);
}
