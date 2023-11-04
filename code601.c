/* code601.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator PowerPC-Familie                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "be_le.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "code601.h"

typedef struct
{
  const char *Name;
  LongWord Code;
  Byte CPUMask;
} BaseOrder;

static BaseOrder *FixedOrders;
static BaseOrder *Reg1Orders;
static BaseOrder *CReg1Orders;
static BaseOrder *CBit1Orders;
static BaseOrder *FReg1Orders;
static BaseOrder *Reg2Orders;
static BaseOrder *CReg2Orders;
static BaseOrder *FReg2Orders;
static BaseOrder *Reg2BOrders;
static BaseOrder *Reg2SwapOrders;
static BaseOrder *NoDestOrders;
static BaseOrder *Reg3Orders;
static BaseOrder *CReg3Orders;
static BaseOrder *FReg3Orders;
static BaseOrder *Reg3SwapOrders;
static BaseOrder *MixedOrders;
static BaseOrder *FReg4Orders;
static BaseOrder *RegDispOrders;
static BaseOrder *FRegDispOrders;
static BaseOrder *Reg2ImmOrders;
static BaseOrder *Imm16Orders;
static BaseOrder *Imm16SwapOrders;

static CPUVar CPU403, CPU403C, CPU505, CPU601, CPU821, CPU6000;

#define M_403 0x01
#define M_403C 0x02
#define M_505 0x04
#define M_601 0x08
#define M_821 0x10
#define M_6000 0x20
#define M_SUP 0x80

#ifdef __STDC__
#define T1  1lu
#define T3  3lu
#define T4  4lu
#define T7  7lu
#define T8  8lu
#define T9  9lu
#define T10 10lu
#define T11 11lu
#define T12 12lu
#define T13 13lu
#define T14 14lu
#define T15 15lu
#define T16 16lu
#define T17 17lu
#define T18 18lu
#define T19 19lu
#define T20 20lu
#define T21 21lu
#define T22 22lu
#define T23 23lu
#define T24 24lu
#define T25 25lu
#define T26 26lu
#define T27 27lu
#define T28 28lu
#define T29 29lu
#define T31 31lu
#define T32 32lu
#define T33 33lu
#define T34 34lu
#define T35 35lu
#define T36 36lu
#define T37 37lu
#define T38 38lu
#define T39 39lu
#define T40 40lu
#define T41 41lu
#define T42 42lu
#define T43 43lu
#define T44 44lu
#define T45 45lu
#define T46 46lu
#define T47 47lu
#define T48 48lu
#define T49 49lu
#define T50 50lu
#define T51 51lu
#define T52 52lu
#define T53 53lu
#define T54 54lu
#define T55 55lu
#define T59 59lu
#define T63 63lu
#else
#define T1  1l
#define T3  3l
#define T4  4l
#define T7  7l
#define T8  8l
#define T9  9l
#define T10 10l
#define T11 11l
#define T12 12l
#define T13 13l
#define T14 14l
#define T15 15l
#define T16 16l
#define T17 17l
#define T18 18l
#define T19 19l
#define T20 20l
#define T21 21l
#define T22 22l
#define T23 23l
#define T24 24l
#define T25 25l
#define T26 26l
#define T27 27l
#define T28 28l
#define T29 29l
#define T31 31l
#define T32 32l
#define T33 33l
#define T34 34l
#define T35 35l
#define T36 36l
#define T37 37l
#define T38 38l
#define T39 39l
#define T40 40l
#define T41 41l
#define T42 42l
#define T43 43l
#define T44 44l
#define T45 45l
#define T46 46l
#define T47 47l
#define T48 48l
#define T49 49l
#define T50 50l
#define T51 51l
#define T52 52l
#define T53 53l
#define T54 54l
#define T55 55l
#define T59 59l
#define T63 63l
#endif

static char ZeroStr[] = "0";
static const tStrComp ZeroComp = { { -1, 0 }, {0, ZeroStr, 0} };

/*-------------------------------------------------------------------------*/

static void PutCode(LongWord Code)
{
#if 0
  memcpy(BAsmCode, &Code, 4);
  if (!TargetBigEndian)
    DSwap((void *)BAsmCode, 4);
#endif
  DAsmCode[0] = Code;
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeGenRegCore(const char *pArg, LongWord *pValue)
 * \brief  check whether argument is general register
 * \param  pArg source argument
 * \param  pValue register # if it's a register
 * \return True if it's a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeGenRegCore(const char *pArg, LongWord *pValue)
{
  if ((strlen(pArg) < 2) || (as_toupper(*pArg) != 'R'))
    return False;
  else
  {
    Boolean OK;

    *pValue = ConstLongInt(pArg + 1, &OK, 10);
    return (OK && (*pValue <= 31));
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPRegCore(const char *pArg, LongWord *pValue)
 * \brief  check whether argument is floating point register
 * \param  pArg source argument
 * \param  pValue register # if it's a register
 * \return True if it's a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeFPRegCore(const char *pArg, LongWord *pValue)
{
  if ((strlen(pArg) < 3) || (as_toupper(*pArg) != 'F') || (as_toupper(pArg[1]) != 'R'))
    return False;
  else
  {
    Boolean OK;

    *pValue = ConstLongInt(pArg + 2, &OK, 10);
    return OK && (*pValue <= 31);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeGenReg(const tStrComp *pArg, LongWord *pValue)
 * \brief  check whether argument is general register, including aliases
 * \param  pArg source argument
 * \param  pValue register # if it's a register
 * \return True if it's a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeGenReg(const tStrComp *pArg, LongWord *pValue)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeGenRegCore(pArg->str.p_str, pValue))
    return True;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, True);
  *pValue = RegDescr.Reg;
  return (RegEvalResult == eIsReg);
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_601(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - PPC variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_601(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    case eSymbolSizeFloat64Bit:
      as_snprintf(pDest, DestSize, "FR%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFPReg(const tStrComp *pArg, LongWord *pValue)
 * \brief  check whether argument is floating point register, including aliases
 * \param  pArg source argument
 * \param  pValue register # if it's a register
 * \return True if it's a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeFPReg(const tStrComp *pArg, LongWord *pValue)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeFPRegCore(pArg->str.p_str, pValue))
    return True;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeFloat64Bit, True);
  *pValue = RegDescr.Reg;
  return (RegEvalResult == eIsReg);
}

static Boolean DecodeCondReg(const tStrComp *pComp, LongWord *Erg)
{
  Boolean OK, Result;

  *Erg = EvalStrIntExpression(pComp, UInt3, &OK) << 2;
  Result = (OK && (*Erg <= 31));
  if (!Result)
    WrStrErrorPos(ErrNum_InvAddrMode, pComp);
  return Result;
}

static Boolean DecodeCondBit(const tStrComp *pComp, LongWord *Erg)
{
  Boolean OK, Result;

  *Erg = EvalStrIntExpression(pComp, UInt5, &OK);
  Result = (OK && (*Erg <= 31));
  if (!Result)
    WrStrErrorPos(ErrNum_InvAddrMode, pComp);
  return Result;
}

static Boolean DecodeRegDisp(tStrComp *pComp, LongWord *Erg)
{
  char *p;
  int l = strlen(pComp->str.p_str);
  LongInt Disp;
  Boolean OK;
  tStrComp DispArg, RegArg;

  if (pComp->str.p_str[l - 1] != ')')
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pComp);
    return False;
  }
  pComp->str.p_str[l - 1] = '\0';  l--;
  p = pComp->str.p_str + l - 1;
  while ((p >= pComp->str.p_str) && (*p != '('))
    p--;
  if (p < pComp->str.p_str)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pComp);
    return False;
  }
  StrCompSplitRef(&DispArg, &RegArg, pComp, p);
  if (!DecodeGenReg(&RegArg, Erg))
    return False;
  *p = '\0';
  Disp = EvalStrIntExpression(&DispArg, Int16, &OK);
  if (!OK)
    return False;

  *Erg = (*Erg << 16) + (Disp & 0xffff);
  return True;
}

/*-------------------------------------------------------------------------*/

static LongWord ExtractPoint(Word Code)
{
  return (Code >> 15) & 1;
}

static void ChkSup(void)
{
  if (!SupAllowed)
    WrError(ErrNum_PrivOrder);
}

static void SwapCode(LongWord *Code)
{
  *Code = ((*Code & 0x1f) << 5) | ((*Code >> 5) & 0x1f);
}

/*-------------------------------------------------------------------------*/

/* ohne Argument */

static void DecodeFixed(Word Index)
{
  const BaseOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0))
  {
    CodeLen = 4;
    PutCode(pOrder->Code);
    if (pOrder->CPUMask & M_SUP)
      ChkSup();
  }
}

/* ein Register */

static void DecodeReg1(Word Index)
{
  const BaseOrder *pOrder = Reg1Orders + Index;
  LongWord Dest;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21));
    if (pOrder->CPUMask & M_SUP)
      ChkSup();
  }
}

/* ein Steuerregister */

static void DecodeCReg1(Word Index)
{
  const BaseOrder *pOrder = CReg1Orders + Index;
  LongWord Dest;

  if (!ChkArgCnt(1, 1));
  else if (ChkExactCPUMask(pOrder->CPUMask, CPU403) < 0);
  else if (!DecodeCondReg(&ArgStr[1], &Dest));
  else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
  else
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21));
  }
}

/* ein Steuerregisterbit */

static void DecodeCBit1(Word Index)
{
  const BaseOrder *pOrder = CBit1Orders + Index;
  LongWord Dest;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeCondBit(&ArgStr[1], &Dest))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21));
  }
}

/* ein Gleitkommaregister */

static void DecodeFReg1(Word Index)
{
  const BaseOrder *pOrder = FReg1Orders + Index;
  LongWord Dest;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21));
  }
}

/* 1/2 Integer-Register */

static void DecodeReg2(Word Index)
{
  const BaseOrder *pOrder = Reg2Orders + Index;
  LongWord Dest, Src1;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[1];

  if (ChkArgCnt(1, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16));
  }
}

/* 2 Bedingungs-Bits */

static void DecodeCReg2(Word Index)
{
  const BaseOrder *pOrder = CReg2Orders + Index;
  LongWord Dest, Src1;

  if (!ChkArgCnt(2, 2));
  else if (ChkExactCPUMask(pOrder->CPUMask, CPU403) < 0);
  else if (!DecodeCondReg(&ArgStr[1], &Dest));
  else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
  else if (!DecodeCondReg(&ArgStr[2], &Src1));
  else if (Src1 & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[2]);
  else
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16));
  }
}

/* 1/2 Float-Register */

static void DecodeFReg2(Word Index)
{
  const BaseOrder *pOrder = FReg2Orders + Index;
  LongWord Dest, Src1;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[1];

  if (ChkArgCnt(1, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeFPReg(pArg2, &Src1))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 11));
  }
}

/* 1/2 Integer-Register, Quelle in B */

static void DecodeReg2B(Word Index)
{
  const BaseOrder *pOrder = Reg2BOrders + Index;
  LongWord Dest, Src1;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[1];

  if (ChkArgCnt(1, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 11));
    ChkSup();
  }
}

/* 1/2 Integer-Register, getauscht */

static void DecodeReg2Swap(Word Index)
{
  const BaseOrder *pOrder = Reg2SwapOrders + Index;
  LongWord Dest, Src1;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[1];

  if (ChkArgCnt(1, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 16) + (Src1 << 21));
  }
}

/* 2 Integer-Register, kein Ziel */

static void DecodeNoDest(Word Index)
{
  const BaseOrder *pOrder = NoDestOrders + Index;
  LongWord Src2, Src1;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Src1)
   && DecodeGenReg(&ArgStr[2], &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Src1 << 16) + (Src2 << 11));
  }
}

/* 2/3 Integer-Register */

static void DecodeReg3(Word Index)
{
  const BaseOrder *pOrder = Reg3Orders + Index;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];
  LongWord Src2, Src1, Dest;

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1)
   && DecodeGenReg(pArg3, &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Src2 << 11));
  }
}

/* 2/3 Bedingungs-Bits */

static void DecodeCReg3(Word Index)
{
  const BaseOrder *pOrder = CReg3Orders + Index;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];
  LongWord Src2, Src1, Dest;

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeCondBit(&ArgStr[1], &Dest)
   && DecodeCondBit(pArg2, &Src1)
   && DecodeCondBit(pArg3, &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Src2 << 11));
  }
}

/* 2/3 Float-Register */

static void DecodeFReg3(Word Index)
{
  const BaseOrder *pOrder = FReg3Orders + Index;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];
  LongWord Src2, Src1, Dest;

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeFPReg(pArg2, &Src1)
   && DecodeFPReg(pArg3, &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Src2 << 11));
  }
}

/* 2/3 Integer-Register, Ziel & Quelle 1 getauscht */

static void DecodeReg3Swap(Word Index)
{
  const BaseOrder *pOrder = Reg3SwapOrders + Index;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];
  LongWord Src2, Src1, Dest;

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1)
   && DecodeGenReg(pArg3, &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 16) + (Src1 << 21) + (Src2 << 11));
  }
}

/* 1 Float und 2 Integer-Register */

static void DecodeMixed(Word Index)
{
  const BaseOrder *pOrder = MixedOrders + Index;
  LongWord Src2, Src1, Dest;

  if (ChkArgCnt(3, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1)
   && DecodeGenReg(&ArgStr[3], &Src2))
   {
     CodeLen = 4;
     PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Src2 << 11));
   }
 }

/* 3/4 Float-Register */

static void DecodeFReg4(Word Index)
{
  const BaseOrder *pOrder = FReg4Orders + Index;
  LongWord Src3, Src2, Src1, Dest;
  const tStrComp *pArg2 = (ArgCnt == 3) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 3) ? &ArgStr[2] : &ArgStr[3],
                 *pArg4 = (ArgCnt == 3) ? &ArgStr[3] : &ArgStr[4];

  if (ChkArgCnt(3, 4)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeFPReg(pArg2, &Src1)
   && DecodeFPReg(pArg3, &Src3)
   && DecodeFPReg(pArg4, &Src2))
  {
    CodeLen = 4;
    PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Src2 << 11) + (Src3 << 6));
  }
}

/* Register mit indiziertem Speicheroperanden */

static void DecodeRegDispOrder(Word Index)
{
  const BaseOrder *pOrder = RegDispOrders + Index;
  LongWord Src1, Dest;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeRegDisp(&ArgStr[2], &Src1))
  {
    PutCode(pOrder->Code + (Dest << 21) + Src1);
    CodeLen = 4;
  }
}

/* Gleitkommaregister mit indiziertem Speicheroperandem */

static void DecodeFRegDisp(Word Index)
{
  const BaseOrder *pOrder = FRegDispOrders + Index;
  LongWord Src1, Dest;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeRegDisp(&ArgStr[2], &Src1))
  {
    PutCode(pOrder->Code + (Dest << 21) + Src1);
    CodeLen = 4;
  }
}

/* 2 verdrehte Register mit immediate */

static void DecodeReg2Imm(Word Index)
{
  const BaseOrder *pOrder = Reg2ImmOrders + Index;
  LongWord Src1, Dest, Src2;
  Boolean OK;

  if (ChkArgCnt(3, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1))
  {
    Src2 = EvalStrIntExpression(&ArgStr[3], UInt5, &OK);
    if (OK)
    {
      PutCode(pOrder->Code + (Src1 << 21) + (Dest << 16) + (Src2 << 11));
      CodeLen = 4;
    }
  }
}

/* 2 Register+immediate */

static void DecodeImm16(Word Index)
{
  const BaseOrder *pOrder = Imm16Orders + Index;
  LongWord Src1, Dest, Imm;
  Boolean OK;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1))
  {
    Imm = EvalStrIntExpression(pArg3, Int16, &OK);
    if (OK)
    {
      CodeLen = 4;
      PutCode(pOrder->Code + (Dest << 21) + (Src1 << 16) + (Imm & 0xffff));
    }
  }
}

/* 2 Register+immediate, Ziel & Quelle 1 getauscht */

static void DecodeImm16Swap(Word Index)
{
  const BaseOrder *pOrder = Imm16SwapOrders + Index;
  LongWord Src1, Dest, Imm;
  Boolean OK;
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];

  if (ChkArgCnt(2, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU403) >= 0)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(pArg2, &Src1))
  {
    Imm = EvalStrIntExpression(pArg3, Int16, &OK);
    if (OK)
    {
      CodeLen = 4;
      PutCode(pOrder->Code + (Dest << 16) + (Src1 << 21) + (Imm & 0xffff));
    }
  }
}

/* Ausreisser... */

static void DecodeFMUL_FMULS(Word Code)
{
  const tStrComp *pArg2 = (ArgCnt == 2) ? &ArgStr[1] : &ArgStr[2],
                 *pArg3 = (ArgCnt == 2) ? &ArgStr[2] : &ArgStr[3];
  LongWord Dest, Src1, Src2, LCode = (Code & 0x7fff);

  if (ChkArgCnt(2, 3)
   && DecodeFPReg(&ArgStr[1], &Dest)
   && DecodeFPReg(pArg2, &Src1)
   && DecodeFPReg(pArg3, &Src2))
  {
    PutCode((LCode << 26) + (25 << 1) + (Dest << 21) + (Src1 << 16) + (Src2 << 6) + ExtractPoint(Code));
    CodeLen = 4;
  }
}

static void DecodeLSWI_STSWI(Word Code)
{
  LongWord Dest, Src1, Src2, LCode = Code;
  Boolean OK;

  if (ChkArgCnt(3, 3)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1))
  {
    Src2 = EvalStrIntExpression(&ArgStr[3], UInt5, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (LCode << 1) + (Dest << 21) + (Src1 << 16) + (Src2 << 11));
      CodeLen = 4;
    }
  }
}

static void DecodeMTFB_MTTB(Word Code)
{
  LongWord LCode = Code, Src1, Dest;
  Boolean OK;
  const tStrComp *pArg1 = &ArgStr[1], *pArg2 = &ArgStr[2];
  tStrComp TmpComp = { { -1, 0 }, { 0, NULL, 0 } };

  if (ChkExactCPUList(ErrNum_InstructionNotSupported, CPU821, CPU505, CPUNone) < 0);
  else if (ArgCnt == 1)
  {
    pArg1 = &ArgStr[1];
    if      ((Memo("MFTB")) || (Memo("MFTBL"))) TmpComp.str.p_str = (char*)"268";
    else if (Memo("MFTBU")) TmpComp.str.p_str = (char*)"269";
    else if ((Memo("MTTB")) || (Memo("MTTBL"))) TmpComp.str.p_str = (char*)"284";
    else if (Memo("MTTBU")) TmpComp.str.p_str = (char*)"285";
    pArg2 = &TmpComp;
    /* already swapped */
  }
  else if ((ArgCnt == 2) && (Code == 467)) /* MTxx */
  {
    pArg1 = &ArgStr[2];
    pArg2 = &ArgStr[1];
  }
  if (ChkArgCnt(1, 2)
   && DecodeGenReg(pArg1, &Dest))
  {
    Src1 = EvalStrIntExpression(pArg2, UInt10, &OK);
    if (OK)
    {
      if ((Src1 == 268) || (Src1 == 269) || (Src1 == 284) || (Src1 == 285))
      {
        SwapCode(&Src1);
        PutCode((T31 << 26) + (Dest << 21) + (Src1 << 11) + (LCode << 1));
        CodeLen = 4;
      }
      else
        WrError(ErrNum_InvCtrlReg);
    }
  }
}

static void DecodeMFSPR_MTSPR(Word Code)
{
  LongWord Dest, Src1, LCode = Code;
  const tStrComp *pArg1 = (Code == 467) ? &ArgStr[2] : &ArgStr[1],
                 *pArg2 = (Code == 467) ? &ArgStr[1] : &ArgStr[2];
  Boolean OK;

  if (ChkArgCnt(2, 2)
   && DecodeGenReg(pArg1, &Dest))
  {
    Src1 = EvalStrIntExpression(pArg2, UInt10, &OK);
    if (OK)
    {
      SwapCode(&Src1);
      PutCode((T31 << 26) + (Dest << 21) + (Src1 << 11) + (LCode << 1));
      CodeLen = 4;
    }
  }
}

static void DecodeMFDCR_MTDCR(Word Code)
{
  LongWord LCode = Code, Src1, Dest;
  const tStrComp *pArg1 = (Code == 451) ? &ArgStr[2] : &ArgStr[1],
                 *pArg2 = (Code == 451) ? &ArgStr[1] : &ArgStr[2];
  Boolean OK;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUList(ErrNum_InstructionNotSupported, CPU403, CPU403C, CPUNone) >= 0)
   && DecodeGenReg(pArg1, &Dest))
  {
    Src1 = EvalStrIntExpression(pArg2, UInt10, &OK);
    if (OK)
    {
      SwapCode(&Src1);
      PutCode((T31 << 26) + (Dest << 21) + (Src1 << 11) + (LCode << 1));
      CodeLen = 4;
    }
  }
}

static void DecodeMFSR_MTSR(Word Code)
{
  LongWord LCode = Code, Src1, Dest;
  const tStrComp *pArg1 = (Code == 210) ? &ArgStr[2] : &ArgStr[1],
                 *pArg2 = (Code == 210) ? &ArgStr[1] : &ArgStr[2];
  Boolean OK;

  if (ChkArgCnt(2, 2)
   && DecodeGenReg(pArg1, &Dest))
  {
    Src1 = EvalStrIntExpression(pArg2, UInt4, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (Dest << 21) + (Src1 << 16) + (LCode << 1));
      CodeLen = 4;
      ChkSup();
    }
  }
}

static void DecodeMTCRF(Word Code)
{
  LongWord Src1, Dest;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 2)
   && DecodeGenReg(&ArgStr[ArgCnt], &Src1))
  {
    OK = True;
    Dest = (ArgCnt == 1) ? 0xff : EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (Src1 << 21) + (Dest << 12) + (144 << 1));
      CodeLen = 4;
    }
  }
}

static void DecodeMTFSF(Word Code)
{
  LongWord Dest, Src1;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeFPReg(&ArgStr[2], &Src1))
  {
    Dest = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
    if (OK)
    {
      PutCode((T63 << 26) + (Dest << 17) + (Src1 << 11) + (711 << 1) + ExtractPoint(Code));
      CodeLen = 4;
    }
  }
}

static void DecodeMTFSFI(Word Code)
{
  LongWord Dest, Src1;
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeCondReg(&ArgStr[1], &Dest));
  else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
  else
  {
    Src1 = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
    if (OK)
    {
      PutCode((T63 << 26) + (Dest << 21) + (Src1 << 12) + (134 << 1) + ExtractPoint(Code));
      CodeLen = 4;
    }
  }
}

static void DecodeRLMI(Word Code)
{
  Integer Imm;
  LongWord Dest, Src1, Src2, Src3;
  Boolean OK;

  if (ChkArgCnt(5, 5)
   && ChkMinCPU(CPU6000)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1)
   && DecodeGenReg(&ArgStr[3], &Src2))
  {
    Src3 = EvalStrIntExpression(&ArgStr[4], UInt5, &OK);
    if (OK)
    {
      Imm = EvalStrIntExpression(&ArgStr[5], UInt5, &OK);
      if (OK)
      {
        PutCode((T22 << 26) + (Src1 << 21) + (Dest << 16)
                     + (Src2 << 11) + (Src3 << 6) + (Imm << 1) + ExtractPoint(Code));
        CodeLen = 4;
      }
    }
  }
}

static void DecodeRLWNM(Word Code)
{
  Integer Imm;
  LongWord Dest, Src1, Src2, Src3;
  Boolean OK;

  if (ChkArgCnt(5, 5)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1)
   && DecodeGenReg(&ArgStr[3], &Src2))
  {
    Src3 = EvalStrIntExpression(&ArgStr[4], UInt5, &OK);
    if (OK)
    {
      Imm = EvalStrIntExpression(&ArgStr[5], UInt5, &OK);
      if (OK)
      {
        PutCode((T23 << 26) + (Src1 << 21) + (Dest << 16)
                     + (Src2 << 11) + (Src3 << 6) + (Imm << 1) + ExtractPoint(Code));
        CodeLen = 4;
      }
    }
  }
}

static void DecodeRLWIMI_RLWINM(Word Code)
{
  Integer Imm;
  LongWord Dest, Src1, Src2, Src3, LCode = Code & 0x7fff;
  Boolean OK;

  if (ChkArgCnt(5, 5)
   && DecodeGenReg(&ArgStr[1], &Dest)
   && DecodeGenReg(&ArgStr[2], &Src1))
  {
    Src2 = EvalStrIntExpression(&ArgStr[3], UInt5, &OK);
    if (OK)
    {
      Src3 = EvalStrIntExpression(&ArgStr[4], UInt5, &OK);
      if (OK)
      {
        Imm = EvalStrIntExpression(&ArgStr[5], UInt5, &OK);
        if (OK)
        {
          PutCode((T20 << 26) + (Dest << 16) + (Src1 << 21)
                + (Src2 << 11) + (Src3 << 6) + (Imm << 1)
                + (LCode << 26) + ExtractPoint(Code));
          CodeLen = 4;
        }
      }
    }
  }
}

static void DecodeTLBIE(Word Code)
{
  LongWord Src1;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && DecodeGenReg(&ArgStr[1], &Src1))
  {
    PutCode((T31 << 26) + (Src1 << 11) + (306 << 1));
    CodeLen = 4;
    ChkSup();
  }
}

static void DecodeTW(Word Code)
{
  LongWord Src1, Src2, Dest;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && DecodeGenReg(&ArgStr[2], &Src1)
   && DecodeGenReg(&ArgStr[3], &Src2))
  {
    Dest = EvalStrIntExpression(&ArgStr[1], UInt5, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (Dest << 21) + (Src1 << 16) + (Src2 << 11) + (4 << 1));
      CodeLen = 4;
    }
  }
}

static void DecodeTWI(Word Code)
{
  Integer Imm;
  LongWord Dest, Src1;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(3, 3)
   && DecodeGenReg(&ArgStr[2], &Src1))
  {
    Imm = EvalStrIntExpression(&ArgStr[3], Int16, &OK);
    if (OK)
    {
      Dest = EvalStrIntExpression(&ArgStr[1], UInt5, &OK);
      if (OK)
      {
        PutCode((T3 << 26) + (Dest << 21) + (Src1 << 16) + (Imm & 0xffff));
        CodeLen = 4;
      }
    }
  }
}

static void DecodeWRTEEI(Word Code)
{
  LongWord  Src1;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUList(ErrNum_InstructionNotSupported, CPU403, CPU403C, CPUNone) >= 0))
  {
    Src1 = EvalStrIntExpression(&ArgStr[1], UInt1, &OK) << 15;
    if (OK)
    {
      PutCode((T31 << 26) + Src1 + (163 << 1));
      CodeLen = 4;
    }
  }
}

static void DecodeCMP_CMPL(Word Code)
{
  LongWord Src1, Src2, Src3, Dest, LCode = Code;
  Boolean OK;
  const tStrComp *pArg4 = (ArgCnt == 3) ? &ArgStr[3] : &ArgStr[4],
                 *pArg3 = (ArgCnt == 3) ? &ArgStr[2] : &ArgStr[3],
                 *pArg2 = (ArgCnt == 3) ? &ZeroComp : &ArgStr[2];

  if (!ChkArgCnt(3, 4));
  else if (!DecodeGenReg(pArg4, &Src2));
  else if (!DecodeGenReg(pArg3, &Src1));
  else if (!DecodeCondReg(&ArgStr[1], &Dest));
  else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
  else
  {
    Src3 = EvalStrIntExpression(pArg2, UInt1, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (Dest << 21) + (Src3 << 21) + (Src1 << 16)
                   + (Src2 << 11) + (LCode << 1));
      CodeLen = 4;
    }
  }
}

/* Vergleiche */

static void DecodeFCMPO_FCMPU(Word Code)
{
  LongWord Src1, Src2, Dest, LCode = Code;

  if (!ChkArgCnt(3, 3));
  else if (!DecodeFPReg(&ArgStr[3], &Src2));
  else if (!DecodeFPReg(&ArgStr[2], &Src1));
  else if (!DecodeCondReg(&ArgStr[1], &Dest));
  else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
  else
  {
    PutCode((T63 << 26) + (Dest << 21) + (Src1 << 16) + (Src2 << 11) + (LCode << 1));
    CodeLen = 4;
  }
}

static void DecodeCMPI_CMPLI(Word Code)
{
  LongWord Src1, Src2, Src3, Dest, LCode = Code;
  Boolean OK;
  const tStrComp *pArg4 = (ArgCnt == 3) ? &ArgStr[3] : &ArgStr[4],
                 *pArg3 = (ArgCnt == 3) ? &ArgStr[2] : &ArgStr[3],
                 *pArg2 = (ArgCnt == 3) ? &ZeroComp : &ArgStr[2];

  if (ChkArgCnt(3, 4))
  {
    Src2 = EvalStrIntExpression(pArg4, Int16, &OK);
    if (OK)
    {
      if (!DecodeGenReg(pArg3, &Src1));
      else if (!DecodeCondReg(&ArgStr[1], &Dest));
      else if (Dest & 3) WrStrErrorPos(ErrNum_AddrMustBeAligned, &ArgStr[1]);
      else
      {
        Src3 = EvalStrIntExpression(pArg2, UInt1, &OK);
        if (OK)
        {
          PutCode((T10 << 26) + (Dest << 21) + (Src3 << 21)
                       + (Src1 << 16) + (Src2 & 0xffff) + (LCode << 26));
          CodeLen = 4;
        }
      }
    }
  }
}

/* Spruenge */

static void DecodeB_BL_BA_BLA(Word Code)
{
  LongWord LCode = Code;
  LongInt Dist;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    Dist = EvalStrIntExpressionWithFlags(&ArgStr[1], Int32, &OK, &Flags);
    if (OK)
    {
      if (!(Code & 2))
        Dist -= EProgCounter();
      if (!mSymbolQuestionable(Flags) && (Dist > 0x1ffffff)) WrError(ErrNum_OverRange);
      else if (!mSymbolQuestionable(Flags) && (Dist < -0x2000000l)) WrError(ErrNum_UnderRange);
      else if ((Dist & 3) != 0) WrError(ErrNum_DistIsOdd);
      else
      {
        PutCode((T18 << 26) + (Dist & 0x03fffffc) + LCode);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeBC_BCL_BCA_BCLA(Word Code)
{
  LongWord LCode = Code, Src1, Src2;
  LongInt Dist;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(3, 3))
  {
    Src1 = EvalStrIntExpression(&ArgStr[1], UInt5, &OK); /* BO */
    if (OK)
    {
      Src2 = EvalStrIntExpression(&ArgStr[2], UInt5, &OK); /* BI */
      if (OK)
      {
        Dist = EvalStrIntExpressionWithFlags(&ArgStr[3], Int32, &OK, &Flags); /* ADR */
        if (OK)
        {
          if (!(Code & 2))
            Dist -= EProgCounter();
          if (!mSymbolQuestionable(Flags) && (Dist > 0x7fff)) WrError(ErrNum_OverRange);
          else if (!mSymbolQuestionable(Flags) && (Dist < -0x8000l)) WrError(ErrNum_UnderRange);
          else if ((Dist & 3) != 0) WrError(ErrNum_DistIsOdd);
          else
          {
            PutCode((T16 << 26) + (Src1 << 21) + (Src2 << 16) + (Dist & 0xfffc) + LCode);
            CodeLen = 4;
          }
        }
      }
    }
  }
}

static void DecodeBCCTR_BCCTRL_BCLR_BCLRL(Word Code)
{
  LongWord Src1, Src2, LCode = Code;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    Src1 = EvalStrIntExpression(&ArgStr[1], UInt5, &OK);
    if (OK)
    {
      Src2 = EvalStrIntExpression(&ArgStr[2], UInt5, &OK);
      if (OK)
      {
        PutCode((T19 << 26) + (Src1 << 21) + (Src2 << 16) + LCode);
        CodeLen = 4;
      }
    }
  }
}

static void DecodeTLBRE_TLBWE(Word Code)
{
  LongWord Src1, Src2, Src3, LCode = Code;
  Boolean OK;

  if (ChkArgCnt(3, 3)
   && ChkExactCPU(CPU403C)
   && DecodeGenReg(&ArgStr[1], &Src1)
   && DecodeGenReg(&ArgStr[2], &Src2))
  {
    Src3 = EvalStrIntExpression(&ArgStr[3], UInt1, &OK);
    if (OK)
    {
      PutCode((T31 << 26) + (Src1 << 21) + (Src2 << 16) +
              (Src3 << 11) + (946 << 1) + (LCode << 1));
      CodeLen = 4;
    }
  }
}

/*-------------------------------------------------------------------------*/

static void AddFixed(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, MomCPU == CPU6000 ? NName2 : NName1, InstrZ++, DecodeFixed);
}

static void AddReg1(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{ 
  order_array_rsv_end(Reg1Orders, BaseOrder);
  Reg1Orders[InstrZ].Code = NCode;
  Reg1Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, MomCPU == CPU6000 ? NName2 : NName1, InstrZ++, DecodeReg1);
}

static void AddCReg1(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(CReg1Orders, BaseOrder);
  CReg1Orders[InstrZ].Code = NCode;
  CReg1Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, MomCPU == CPU6000 ? NName2 : NName1, InstrZ++, DecodeCReg1);
}

static void AddCBit1(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(CBit1Orders, BaseOrder);
  CBit1Orders[InstrZ].Code = NCode;
  CBit1Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, MomCPU == CPU6000 ? NName2 : NName1, InstrZ++, DecodeCBit1);
}

static void AddFReg1(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FReg1Orders, BaseOrder);
  FReg1Orders[InstrZ].Code = NCode;
  FReg1Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, MomCPU == CPU6000 ? NName2 : NName1, InstrZ++, DecodeFReg1);
}

static void AddSReg2(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg2Orders, BaseOrder);
  Reg2Orders[InstrZ].Code = NCode;
  Reg2Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg2);
}

static void AddReg2(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithOE, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSReg2(pSrcName, NCode, NMask);
  if (WithOE)
  {
    as_snprintf(NName, sizeof(NName), "%sO", pSrcName);
    AddSReg2(NName, NCode | 0x400, NMask);
  }
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSReg2(NName, NCode | 0x001, NMask);
    if (WithOE)
    {
      as_snprintf(NName, sizeof(NName), "%sO.", pSrcName);
      AddSReg2(NName, NCode | 0x401, NMask);
    }
  }
}

static void AddCReg2(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(CReg2Orders, BaseOrder);
  CReg2Orders[InstrZ].Code = NCode;
  CReg2Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeCReg2);
}

static void AddSFReg2(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FReg2Orders, BaseOrder);
  if (!NName) exit(255);
  FReg2Orders[InstrZ].Code = NCode;
  FReg2Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFReg2);
}

static void AddFReg2(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithFL)
{
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSFReg2(pSrcName, NCode, NMask);
  if (WithFL)
  {
    String NName;

    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSFReg2(NName, NCode | 0x001, NMask);
  }
}

static void AddReg2B(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg2BOrders, BaseOrder);
  Reg2BOrders[InstrZ].Code = NCode;
  Reg2BOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeReg2B);
}

static void AddSReg2Swap(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg2SwapOrders, BaseOrder);
  if (!NName) exit(255);
  Reg2SwapOrders[InstrZ].Code = NCode;
  Reg2SwapOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg2Swap);
}

static void AddReg2Swap(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithOE, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSReg2Swap(pSrcName, NCode, NMask);
  if (WithOE)
  {
    as_snprintf(NName, sizeof(NName), "%sO", pSrcName);
    AddSReg2Swap(NName, NCode | 0x400, NMask);
  }
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSReg2Swap(NName, NCode | 0x001, NMask);
    if (WithOE)
    {
      as_snprintf(NName, sizeof(NName), "%sO.", pSrcName);
      AddSReg2Swap(NName, NCode | 0x401, NMask);
    }
  }
}

static void AddNoDest(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(NoDestOrders, BaseOrder);
  NoDestOrders[InstrZ].Code = NCode;
  NoDestOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeNoDest);
}

static void AddSReg3(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg3Orders, BaseOrder);
  Reg3Orders[InstrZ].Code = NCode;
  Reg3Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg3);
}

static void AddReg3(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithOE, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSReg3(pSrcName, NCode, NMask);
  if (WithOE)
  {
    as_snprintf(NName, sizeof(NName), "%sO", pSrcName);
    AddSReg3(NName, NCode | 0x400, NMask);
    NName[strlen(NName) - 1] = '\0';
  }
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSReg3(NName, NCode | 0x001, NMask);
    NName[strlen(NName) - 1] = '\0';
    if (WithOE)
    {
      as_snprintf(NName, sizeof(NName), "%sO.", pSrcName);
      AddSReg3(NName, NCode | 0x401, NMask);
    }
  }
}

static void AddCReg3(const char *NName, LongWord NCode, CPUVar NMask)
{
  order_array_rsv_end(CReg3Orders, BaseOrder);
  CReg3Orders[InstrZ].Code = NCode;
  CReg3Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeCReg3);
}

static void AddSFReg3(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FReg3Orders, BaseOrder);
  FReg3Orders[InstrZ].Code = NCode;
  FReg3Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFReg3);
}

static void AddFReg3(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSFReg3(pSrcName, NCode, NMask);
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSFReg3(NName, NCode | 0x001, NMask);
  }
}

static void AddSReg3Swap(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg3SwapOrders, BaseOrder);
  Reg3SwapOrders[InstrZ].Code = NCode;
  Reg3SwapOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg3Swap);
}

static void AddReg3Swap(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSReg3Swap(pSrcName, NCode, NMask);
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSReg3Swap(NName, NCode | 0x001, NMask);
  }
}

static void AddMixed(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(MixedOrders, BaseOrder);
  MixedOrders[InstrZ].Code = NCode;
  MixedOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeMixed);
}

static void AddSFReg4(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FReg4Orders, BaseOrder);
  FReg4Orders[InstrZ].Code = NCode;
  FReg4Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFReg4);
}

static void AddFReg4(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSFReg4(pSrcName, NCode, NMask);
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSFReg4(NName, NCode | 0x001, NMask);
  }
}

static void AddRegDisp(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(RegDispOrders, BaseOrder);
  RegDispOrders[InstrZ].Code = NCode;
  RegDispOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeRegDispOrder);
}

static void AddFRegDisp(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(FRegDispOrders, BaseOrder);
  FRegDispOrders[InstrZ].Name = (MomCPU == CPU6000) ? NName2 : NName1;
  FRegDispOrders[InstrZ].Code = NCode;
  FRegDispOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeFRegDisp);
}

static void AddSReg2Imm(const char *NName, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Reg2ImmOrders, BaseOrder);
  if (!NName) exit(255);
  Reg2ImmOrders[InstrZ].Code = NCode;
  Reg2ImmOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeReg2Imm);
}

static void AddReg2Imm(const char *NName1, const char *NName2, LongWord NCode, Byte NMask, Boolean WithFL)
{
  String NName;
  const char *pSrcName = (MomCPU == CPU6000) ? NName2 : NName1;

  AddSReg2Imm(pSrcName, NCode, NMask);
  if (WithFL)
  {
    as_snprintf(NName, sizeof(NName), "%s.", pSrcName);
    AddSReg2Imm(NName, NCode | 0x001, NMask);
  }
}

static void AddImm16(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Imm16Orders, BaseOrder);
  Imm16Orders[InstrZ].Code = NCode;
  Imm16Orders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeImm16);
}

static void AddImm16Swap(const char *NName1, const char *NName2, LongWord NCode, Byte NMask)
{
  order_array_rsv_end(Imm16SwapOrders, BaseOrder);
  Imm16SwapOrders[InstrZ].Code = NCode;
  Imm16SwapOrders[InstrZ].CPUMask = NMask;
  AddInstTable(InstTable, (MomCPU == CPU6000) ? NName2 : NName1, InstrZ++, DecodeImm16Swap);
}

static void AddPoint(const char *pName, Word Code, InstProc Proc)
{
  char PointName[30];

  AddInstTable(InstTable, pName, Code, Proc);
  as_snprintf(PointName, sizeof(PointName), "%s.", pName);
  AddInstTable(InstTable, PointName, Code | 0x8000, Proc);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(407);
  SetDynamicInstTable(InstTable);

  AddPoint("FMULS", 59, DecodeFMUL_FMULS);
  AddPoint((MomCPU == CPU6000) ? "FM" : "FMUL", 63, DecodeFMUL_FMULS);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "LSI" : "LSWI", 597, DecodeLSWI_STSWI);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "STSI" : "STSWI", 597 + 128, DecodeLSWI_STSWI);
  AddInstTable(InstTable, "MFTB", 371, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MFTBU", 371, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MFTBL", 371, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MTTB", 467, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MTTBU", 467, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MTTBL", 467, DecodeMTFB_MTTB);
  AddInstTable(InstTable, "MFSPR", 339, DecodeMFSPR_MTSPR);
  AddInstTable(InstTable, "MTSPR", 467, DecodeMFSPR_MTSPR);
  AddInstTable(InstTable, "MFDCR", 323, DecodeMFDCR_MTDCR);
  AddInstTable(InstTable, "MTDCR", 451, DecodeMFDCR_MTDCR);
  AddInstTable(InstTable, "MFSR", 595, DecodeMFSR_MTSR);
  AddInstTable(InstTable, "MTSR", 210, DecodeMFSR_MTSR);
  AddInstTable(InstTable, "MTCRF", 0, DecodeMTCRF);
  AddPoint("MTFSF", 0, DecodeMTFSF);
  AddPoint("MTFSFI", 0, DecodeMTFSFI);
  AddPoint("RLMI", 0, DecodeRLMI);
  AddPoint((MomCPU == CPU6000) ? "RLNM" : "RLWNM", 0, DecodeRLWNM);
  AddPoint((MomCPU == CPU6000) ? "RLIMI" : "RLWIMI", 0, DecodeRLWIMI_RLWINM);
  AddPoint((MomCPU == CPU6000) ? "RLINM" : "RLWINM", 1, DecodeRLWIMI_RLWINM);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "TLBI" :  "TLBIE", 0, DecodeTLBIE);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "T" : "TW", 0, DecodeTW);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "TI" : "TWI", 0, DecodeTWI);
  AddInstTable(InstTable, "WRTEEI", 0, DecodeWRTEEI);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP_CMPL);
  AddInstTable(InstTable, "CMPL", 32, DecodeCMP_CMPL);
  AddInstTable(InstTable, "FCMPO", 32, DecodeFCMPO_FCMPU);
  AddInstTable(InstTable, "FCMPU", 0, DecodeFCMPO_FCMPU);
  AddInstTable(InstTable, "CMPI", 1, DecodeCMPI_CMPLI);
  AddInstTable(InstTable, "CMPLI", 0, DecodeCMPI_CMPLI);
  AddInstTable(InstTable, "B", 0, DecodeB_BL_BA_BLA);
  AddInstTable(InstTable, "BL", 1, DecodeB_BL_BA_BLA);
  AddInstTable(InstTable, "BA", 2, DecodeB_BL_BA_BLA);
  AddInstTable(InstTable, "BLA", 3, DecodeB_BL_BA_BLA);
  AddInstTable(InstTable, "BC", 0, DecodeBC_BCL_BCA_BCLA);
  AddInstTable(InstTable, "BCL", 1, DecodeBC_BCL_BCA_BCLA);
  AddInstTable(InstTable, "BCA", 2, DecodeBC_BCL_BCA_BCLA);
  AddInstTable(InstTable, "BCLA", 3, DecodeBC_BCL_BCA_BCLA);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "BCC" : "BCCTR", 528 << 1, DecodeBCCTR_BCCTRL_BCLR_BCLRL);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "BCCL" : "BCCTRL", (528 << 1) | 1, DecodeBCCTR_BCCTRL_BCLR_BCLRL);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "BCR" : "BCLR", 16 << 1, DecodeBCCTR_BCCTRL_BCLR_BCLRL);
  AddInstTable(InstTable, (MomCPU == CPU6000) ? "BCRL" : "BCLRL", (16 << 1) | 1, DecodeBCCTR_BCCTRL_BCLR_BCLRL);
  AddInstTable(InstTable, "TLBRE", 0, DecodeTLBRE_TLBWE);
  AddInstTable(InstTable, "TLBWE", 32, DecodeTLBRE_TLBWE);
  AddInstTable(InstTable, "REG", 0, CodeREG);

  /* --> 0 0 0 */

  InstrZ = 0;
  AddFixed("EIEIO"  , "EIEIO"  , (T31 << 26) + (854 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddFixed("ISYNC"  , "ICS"    , (T19 << 26) + (150 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddFixed("RFI"    , "RFI"    , (T19 << 26) + ( 50 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000 | M_SUP);
  AddFixed("SC"     , "SVCA"   , (T17 << 26) + (  1 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddFixed("SYNC"   , "DCS"    , (T31 << 26) + (598 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddFixed("RFCI"   , "RFCI"   , (T19 << 26) + ( 51 << 1), M_403 | M_403C                         );
  AddFixed("TLBIA"  , "TLBIA"  , (T31 << 26) + (370 << 1),         M_403C | M_821                 );
  AddFixed("TLBSYNC", "TLBSYNC", (T31 << 26) + (566 << 1),         M_403C | M_821                 );

  /* D --> D 0 0 */

  InstrZ = 0;
  AddReg1("MFCR"   , "MFCR"    , (T31 << 26) + ( 19 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddReg1("MFMSR"  , "MFMSR"   , (T31 << 26) + ( 83 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddReg1("MTMSR"  , "MTMSR"   , (T31 << 26) + (146 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000 | M_SUP);
  AddReg1("WRTEE"  , "WRTEE"   , (T31 << 26) + (131 << 1), M_403 | M_403C | M_505 |         M_601 | M_6000);

  /* crD --> D 0 0 */

  InstrZ = 0;
  AddCReg1("MCRXR"  , "MCRXR"  , (T31 << 26) + (512 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);

  /* crbD --> D 0 0 */

  InstrZ = 0;
  AddCBit1("MTFSB0" , "MTFSB0" , (T63 << 26) + ( 70 << 1)    , M_601 | M_6000);
  AddCBit1("MTFSB0.", "MTFSB0.", (T63 << 26) + ( 70 << 1) + 1, M_601 | M_6000);
  AddCBit1("MTFSB1" , "MTFSB1" , (T63 << 26) + ( 38 << 1)    , M_601 | M_6000);
  AddCBit1("MTFSB1.", "MTFSB1.", (T63 << 26) + ( 38 << 1) + 1, M_601 | M_6000);

  /* frD --> D 0 0 */

  InstrZ = 0;
  AddFReg1("MFFS"   , "MFFS"  , (T63 << 26) + (583 << 1)    , M_601 | M_6000);
  AddFReg1("MFFS."  , "MFFS." , (T63 << 26) + (583 << 1) + 1, M_601 | M_6000);

  /* D,A --> D A 0 */

  InstrZ = 0;
  AddReg2("ABS"   , "ABS"  , (T31 << 26) + (360 << 1),                                          M_6000, True , True );
  AddReg2("ADDME" , "AME"  , (T31 << 26) + (234 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg2("ADDZE" , "AZE"  , (T31 << 26) + (202 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg2("CLCS"  , "CLCS" , (T31 << 26) + (531 << 1),                                          M_6000, False, False);
  AddReg2("NABS"  , "NABS" , (T31 << 26) + (488 << 1),                                          M_6000, True , True );
  AddReg2("NEG"   , "NEG"  , (T31 << 26) + (104 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg2("SUBFME", "SFME" , (T31 << 26) + (232 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg2("SUBFZE", "SFZE" , (T31 << 26) + (200 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );

  /* cD,cS --> D S 0 */

  InstrZ = 0;
  AddCReg2("MCRF"  , "MCRF"  , (T19 << 26) + (  0 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg2("MCRFS" , "MCRFS" , (T63 << 26) + ( 64 << 1),                          M_601 | M_6000);

  /* fD,fB --> D 0 B */

  InstrZ = 0;
  AddFReg2("FABS"  , "FABS"  , (T63 << 26) + (264 << 1), M_601 | M_6000, True );
  AddFReg2("FCTIW" , "FCTIW" , (T63 << 26) + ( 14 << 1), M_601 | M_6000, True );
  AddFReg2("FCTIWZ", "FCTIWZ", (T63 << 26) + ( 15 << 1), M_601 | M_6000, True );
  AddFReg2("FMR"   , "FMR"   , (T63 << 26) + ( 72 << 1), M_601 | M_6000, True );
  AddFReg2("FNABS" , "FNABS" , (T63 << 26) + (136 << 1), M_601 | M_6000, True );
  AddFReg2("FNEG"  , "FNEG"  , (T63 << 26) + ( 40 << 1), M_601 | M_6000, True );
  AddFReg2("FRSP"  , "FRSP"  , (T63 << 26) + ( 12 << 1), M_601 | M_6000, True );

  /* D,B --> D 0 B */

  InstrZ = 0;
  AddReg2B("MFSRIN", "MFSRIN", (T31 << 26) + (659 << 1), M_601 | M_6000);
  AddReg2B("MTSRIN", "MTSRI" , (T31 << 26) + (242 << 1), M_601 | M_6000);

  /* A,S --> S A 0 */

  InstrZ = 0;
  AddReg2Swap("CNTLZW", "CNTLZ" , (T31 << 26) + ( 26 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, True );
  AddReg2Swap("EXTSB ", "EXTSB" , (T31 << 26) + (954 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, True );
  AddReg2Swap("EXTSH ", "EXTS"  , (T31 << 26) + (922 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, True );

  /* A,B --> 0 A B */

  InstrZ = 0;
  AddNoDest("DCBF"  , "DCBF"  , (T31 << 26) + (  86 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCBI"  , "DCBI"  , (T31 << 26) + ( 470 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCBST" , "DCBST" , (T31 << 26) + (  54 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCBT"  , "DCBT"  , (T31 << 26) + ( 278 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCBTST", "DCBTST", (T31 << 26) + ( 246 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCBZ"  , "DCLZ"  , (T31 << 26) + (1014 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("DCCCI" , "DCCCI" , (T31 << 26) + ( 454 << 1), M_403 | M_403C                         );
  AddNoDest("ICBI"  , "ICBI"  , (T31 << 26) + ( 982 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddNoDest("ICBT"  , "ICBT"  , (T31 << 26) + ( 262 << 1), M_403 | M_403C                         );
  AddNoDest("ICCCI" , "ICCCI" , (T31 << 26) + ( 966 << 1), M_403 | M_403C                         );

  /* D,A,B --> D A B */

  InstrZ = 0;
  AddReg3("ADD"   , "CAX"   , (T31 << 26) + (266 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True,  True );
  AddReg3("ADDC"  , "A"     , (T31 << 26) + ( 10 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("ADDE"  , "AE"    , (T31 << 26) + (138 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("DIV"   , "DIV"   , (T31 << 26) + (331 << 1),                                          M_6000, True , True );
  AddReg3("DIVS"  , "DIVS"  , (T31 << 26) + (363 << 1),                                          M_6000, True , True );
  AddReg3("DIVW"  , "DIVW"  , (T31 << 26) + (491 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("DIVWU" , "DIVWU" , (T31 << 26) + (459 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("DOZ"   , "DOZ"   , (T31 << 26) + (264 << 1),                                          M_6000, True , True );
  AddReg3("ECIWX" , "ECIWX" , (T31 << 26) + (310 << 1),                          M_821 |         M_6000, False, False);
  AddReg3("LBZUX" , "LBZUX" , (T31 << 26) + (119 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LBZX"  , "LBZX"  , (T31 << 26) + ( 87 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LHAUX" , "LHAUX" , (T31 << 26) + (375 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LHAX"  , "LHAX"  , (T31 << 26) + (343 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LHBRX" , "LHBRX" , (T31 << 26) + (790 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LHZUX" , "LHZUX" , (T31 << 26) + (311 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LHZX"  , "LHZX"  , (T31 << 26) + (279 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LSCBX" , "LSCBX" , (T31 << 26) + (277 << 1),                                          M_6000, False, True );
  AddReg3("LSWX"  , "LSX"   , (T31 << 26) + (533 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LWARX" , "LWARX" , (T31 << 26) + ( 20 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LWBRX" , "LBRX"  , (T31 << 26) + (534 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LWZUX" , "LUX"   , (T31 << 26) + ( 55 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("LWZX"  , "LX"    , (T31 << 26) + ( 23 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("MUL"   , "MUL"   , (T31 << 26) + (107 << 1),                                          M_6000, True , True );
  AddReg3("MULHW" , "MULHW" , (T31 << 26) + ( 75 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, True );
  AddReg3("MULHWU", "MULHWU", (T31 << 26) + ( 11 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, True );
  AddReg3("MULLW" , "MULS"  , (T31 << 26) + (235 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("STBUX" , "STBUX" , (T31 << 26) + (247 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STBX"  , "STBX"  , (T31 << 26) + (215 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STHBRX", "STHBRX", (T31 << 26) + (918 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STHUX" , "STHUX" , (T31 << 26) + (439 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STHX"  , "STHX"  , (T31 << 26) + (407 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STSWX" , "STSX"  , (T31 << 26) + (661 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STWBRX", "STBRX" , (T31 << 26) + (662 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STWCX.", "STWCX.", (T31 << 26) + (150 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STWUX" , "STUX"  , (T31 << 26) + (183 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("STWX"  , "STX"   , (T31 << 26) + (151 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, False, False);
  AddReg3("SUBF"  , "SUBF"  , (T31 << 26) + ( 40 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("SUB"   , "SUB"   , (T31 << 26) + ( 40 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("SUBFC" , "SF"    , (T31 << 26) + (  8 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("SUBC"  , "SUBC"  , (T31 << 26) + (  8 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("SUBFE" , "SFE"   , (T31 << 26) + (136 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True , True );
  AddReg3("TLBSX" , "TLBSX" , (T31 << 26) + (914 << 1),         M_403C                                 , False, True );

  /* cD,cA,cB --> D A B */

  InstrZ = 0;
  AddCReg3("CRAND"  , (T19 << 26) + (257 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CRANDC" , (T19 << 26) + (129 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CREQV"  , (T19 << 26) + (289 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CRNAND" , (T19 << 26) + (225 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CRNOR"  , (T19 << 26) + ( 33 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CROR"   , (T19 << 26) + (449 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CRORC"  , (T19 << 26) + (417 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddCReg3("CRXOR"  , (T19 << 26) + (193 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);

  /* fD,fA,fB --> D A B */

  InstrZ = 0;
  AddFReg3("FADD"  , "FA"    , (T63 << 26) + (21 << 1), M_601 | M_6000, True );
  AddFReg3("FADDS" , "FADDS" , (T59 << 26) + (21 << 1), M_601 | M_6000, True );
  AddFReg3("FDIV"  , "FD"    , (T63 << 26) + (18 << 1), M_601 | M_6000, True );
  AddFReg3("FDIVS" , "FDIVS" , (T59 << 26) + (18 << 1), M_601 | M_6000, True );
  AddFReg3("FSUB"  , "FS"    , (T63 << 26) + (20 << 1), M_601 | M_6000, True );

  /* A,S,B --> S A B */

  InstrZ = 0;
  AddReg3Swap("AND"   , "AND"   , (T31 << 26) + (  28 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("ANDC"  , "ANDC"  , (T31 << 26) + (  60 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("ECOWX" , "ECOWX" , (T31 << 26) + ( 438 << 1),                          M_821 | M_601 | M_6000, False);
  AddReg3Swap("EQV"   , "EQV"   , (T31 << 26) + ( 284 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("MASKG" , "MASKG" , (T31 << 26) + (  29 << 1),                                          M_6000, True );
  AddReg3Swap("MASKIR", "MASKIR", (T31 << 26) + ( 541 << 1),                                          M_6000, True );
  AddReg3Swap("NAND"  , "NAND"  , (T31 << 26) + ( 476 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("NOR"   , "NOR"   , (T31 << 26) + ( 124 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("OR"    , "OR"    , (T31 << 26) + ( 444 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("ORC"   , "ORC"   , (T31 << 26) + ( 412 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("RRIB"  , "RRIB"  , (T31 << 26) + ( 537 << 1),                                          M_6000, True );
  AddReg3Swap("SLE"   , "SLE"   , (T31 << 26) + ( 153 << 1),                                          M_6000, True );
  AddReg3Swap("SLEQ"  , "SLEQ"  , (T31 << 26) + ( 217 << 1),                                          M_6000, True );
  AddReg3Swap("SLLQ"  , "SLLQ"  , (T31 << 26) + ( 216 << 1),                                          M_6000, True );
  AddReg3Swap("SLQ"   , "SLQ"   , (T31 << 26) + ( 152 << 1),                                          M_6000, True );
  AddReg3Swap("SLW"   , "SL"    , (T31 << 26) + (  24 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("SRAQ"  , "SRAQ"  , (T31 << 26) + ( 920 << 1),                                          M_6000, True );
  AddReg3Swap("SRAW"  , "SRA"   , (T31 << 26) + ( 792 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("SRE"   , "SRE"   , (T31 << 26) + ( 665 << 1),                                          M_6000, True );
  AddReg3Swap("SREA"  , "SREA"  , (T31 << 26) + ( 921 << 1),                                          M_6000, True );
  AddReg3Swap("SREQ"  , "SREQ"  , (T31 << 26) + ( 729 << 1),                                          M_6000, True );
  AddReg3Swap("SRLQ"  , "SRLQ"  , (T31 << 26) + ( 728 << 1),                                          M_6000, True );
  AddReg3Swap("SRQ"   , "SRQ"   , (T31 << 26) + ( 664 << 1),                                          M_6000, True );
  AddReg3Swap("SRW"   , "SR"    , (T31 << 26) + ( 536 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True );
  AddReg3Swap("XOR"   , "XOR"   , (T31 << 26) + ( 316 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000,True );

  /* fD,A,B --> D A B */

  InstrZ = 0;
  AddMixed("LFDUX" , "LFDUX" , (T31 << 26) + (631 << 1), M_601 | M_6000);
  AddMixed("LFDX"  , "LFDX"  , (T31 << 26) + (599 << 1), M_601 | M_6000);
  AddMixed("LFSUX" , "LFSUX" , (T31 << 26) + (567 << 1), M_601 | M_6000);
  AddMixed("LFSX"  , "LFSX"  , (T31 << 26) + (535 << 1), M_601 | M_6000);
  AddMixed("STFDUX", "STFDUX", (T31 << 26) + (759 << 1), M_601 | M_6000);
  AddMixed("STFDX" , "STFDX" , (T31 << 26) + (727 << 1), M_601 | M_6000);
  AddMixed("STFSUX", "STFSUX", (T31 << 26) + (695 << 1), M_601 | M_6000);
  AddMixed("STFSX" , "STFSX" , (T31 << 26) + (663 << 1), M_601 | M_6000);

  /* fD,fA,fC,fB --> D A B C */

  InstrZ = 0;
  AddFReg4("FMADD"  , "FMA"    , (T63 << 26) + (29 << 1), M_601 | M_6000, True );
  AddFReg4("FMADDS" , "FMADDS" , (T59 << 26) + (29 << 1), M_601 | M_6000, True );
  AddFReg4("FMSUB"  , "FMS"    , (T63 << 26) + (28 << 1), M_601 | M_6000, True );
  AddFReg4("FMSUBS" , "FMSUBS" , (T59 << 26) + (28 << 1), M_601 | M_6000, True );
  AddFReg4("FNMADD" , "FNMA"   , (T63 << 26) + (31 << 1), M_601 | M_6000, True );
  AddFReg4("FNMADDS", "FNMADDS", (T59 << 26) + (31 << 1), M_601 | M_6000, True );
  AddFReg4("FNMSUB" , "FNMS"   , (T63 << 26) + (30 << 1), M_601 | M_6000, True );
  AddFReg4("FNMSUBS", "FNMSUBS", (T59 << 26) + (30 << 1), M_601 | M_6000, True );

  /* D,d(A) --> D A d */

  InstrZ = 0;
  AddRegDisp("LBZ"   , "LBZ"   , (T34 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LBZU"  , "LBZU"  , (T35 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LHA"   , "LHA"   , (T42 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LHAU"  , "LHAU"  , (T43 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LHZ"   , "LHZ"   , (T40 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LHZU"  , "LHZU"  , (T41 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LMW"   , "LM"    , (T46 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LWZ"   , "L"     , (T32 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("LWZU"  , "LU"    , (T33 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STB"   , "STB"   , (T38 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STBU"  , "STBU"  , (T39 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STH"   , "STH"   , (T44 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STHU"  , "STHU"  , (T45 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STMW"  , "STM"   , (T47 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STW"   , "ST"    , (T36 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddRegDisp("STWU"  , "STU"   , (T37 << 26), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);

  /* fD,d(A) --> D A d */

  InstrZ = 0;
  AddFRegDisp("LFD"   , "LFD"   , (T50 << 26), M_601 | M_6000);
  AddFRegDisp("LFDU"  , "LFDU"  , (T51 << 26), M_601 | M_6000);
  AddFRegDisp("LFS"   , "LFS"   , (T48 << 26), M_601 | M_6000);
  AddFRegDisp("LFSU"  , "LFSU"  , (T49 << 26), M_601 | M_6000);
  AddFRegDisp("STFD"  , "STFD"  , (T54 << 26), M_601 | M_6000);
  AddFRegDisp("STFDU" , "STFDU" , (T55 << 26), M_601 | M_6000);
  AddFRegDisp("STFS"  , "STFS"  , (T52 << 26), M_601 | M_6000);
  AddFRegDisp("STFSU" , "STFSU" , (T53 << 26), M_601 | M_6000);

  /* A,S,Imm5 --> S A Imm */

  InstrZ = 0;
  AddReg2Imm("SLIQ"  , "SLIQ"  , (T31 << 26) + (184 << 1),                                          M_6000, True);
  AddReg2Imm("SLLIQ" , "SLLIQ" , (T31 << 26) + (248 << 1),                                          M_6000, True);
  AddReg2Imm("SRAIQ" , "SRAIQ" , (T31 << 26) + (952 << 1),                                          M_6000, True);
  AddReg2Imm("SRAWI" , "SRAI"  , (T31 << 26) + (824 << 1), M_403 | M_403C | M_505 | M_821 | M_601 | M_6000, True);
  AddReg2Imm("SRIQ"  , "SRIQ"  , (T31 << 26) + (696 << 1),                                          M_6000, True);
  AddReg2Imm("SRLIQ" , "SRLIQ" , (T31 << 26) + (760 << 1),                                          M_6000, True);

  /* D,A,Imm --> D A Imm */

  InstrZ = 0;
  AddImm16("ADDI"   , "CAL"    , T14 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16("ADDIC"  , "AI"     , T12 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16("ADDIC." , "AI."    , T13 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16("ADDIS"  , "CAU"    , T15 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16("DOZI"   , "DOZI"   ,  T9 << 26,                                          M_6000);
  AddImm16("MULLI"  , "MULI"   ,  T7 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16("SUBFIC" , "SFI"    ,  T8 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);

  /* A,S,Imm --> S A Imm */

  InstrZ = 0;
  AddImm16Swap("ANDI."  , "ANDIL." , T28 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16Swap("ANDIS." , "ANDIU." , T29 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16Swap("ORI"    , "ORIL"   , T24 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16Swap("ORIS"   , "ORIU"   , T25 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16Swap("XORI"   , "XORIL"  , T26 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
  AddImm16Swap("XORIS"  , "XORIU"  , T27 << 26, M_403 | M_403C | M_505 | M_821 | M_601 | M_6000);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(Reg1Orders);
  order_array_free(FReg1Orders);
  order_array_free(CReg1Orders);
  order_array_free(CBit1Orders);
  order_array_free(Reg2Orders);
  order_array_free(CReg2Orders);
  order_array_free(FReg2Orders);
  order_array_free(Reg2BOrders);
  order_array_free(Reg2SwapOrders);
  order_array_free(NoDestOrders);
  order_array_free(Reg3Orders);
  order_array_free(CReg3Orders);
  order_array_free(FReg3Orders);
  order_array_free(Reg3SwapOrders);
  order_array_free(MixedOrders);
  order_array_free(FReg4Orders);
  order_array_free(RegDispOrders);
  order_array_free(FRegDispOrders);
  order_array_free(Reg2ImmOrders);
  order_array_free(Imm16Orders);
  order_array_free(Imm16SwapOrders);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_601(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* Nullanweisung */

  if (Memo("") && !*AttrPart.str.p_str && (ArgCnt == 0))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(TargetBigEndian))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_601(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_601(char *Asc, TempResult *Erg)
 * \brief  handle built.in symbols for PPC
 * \param  Asc source argument
 * \param  Erg result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_601(char *Asc, TempResult *Erg)
{
  LongWord RegValue;
  int l = strlen(Asc);

  Erg->Typ = TempNone;
  if (((l == 3) || (l == 4))
   && ((as_toupper(*Asc) == 'C') && (as_toupper(Asc[1]) == 'R'))
   && ((Asc[l - 1] >= '0') && (Asc[l - 1] <= '7'))
   && ((l == 3) != ((as_toupper(Asc[2]) == 'F') || (as_toupper(Asc[3]) == 'B'))))
    as_tempres_set_int(Erg, Asc[l - 1] - '0');
  else if (DecodeGenRegCore(Asc, &RegValue))
  {
    Erg->Typ = TempReg;
    Erg->Contents.RegDescr.Dissect = DissectReg_601;
    Erg->Contents.RegDescr.compare = NULL;
    Erg->Contents.RegDescr.Reg = RegValue;
    Erg->DataSize = eSymbolSize32Bit;
  }
  else if (DecodeFPRegCore(Asc, &RegValue))
  {
    Erg->Typ = TempReg;
    Erg->Contents.RegDescr.Dissect = DissectReg_601;
    Erg->Contents.RegDescr.compare = NULL;
    Erg->Contents.RegDescr.Reg = RegValue;
    Erg->DataSize = eSymbolSizeFloat64Bit;
  }
}

static void SwitchTo_601(void)
{
  const TFamilyDescr *FoundDscr;

  TurnWords = True;
  SetIntConstMode(eIntConstModeC);

  FoundDscr = FindFamilyByName("MPC601");
  if (!FoundDscr)
    exit(255);

  PCSymbol = "*";
  HeaderID = FoundDscr->Id;
  NOPCode = 0x000000000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 4; SegInits[SegCode] = 0;
  SegLimits[SegCode] = (LargeWord)IntTypeDefs[UInt32].Max;

  MakeCode = MakeCode_601;
  IsDef = IsDef_601;
  SwitchFrom = DeinitFields;
  InternSymbol = InternSymbol_601;
  DissectReg = DissectReg_601;
  onoff_supmode_add();
  onoff_bigendian_add();

  InitFields();
}

void code601_init(void)
{
  CPU403  = AddCPU("PPC403", SwitchTo_601);
  CPU403C = AddCPU("PPC403GC", SwitchTo_601);
  CPU505  = AddCPU("MPC505", SwitchTo_601);
  CPU601  = AddCPU("MPC601", SwitchTo_601);
  CPU821  = AddCPU("MPC821", SwitchTo_601);
  CPU6000 = AddCPU("RS6000", SwitchTo_601);

  AddCopyright("Motorola MPC821 Additions (C) 2012 Marcin Cieslak");
}
