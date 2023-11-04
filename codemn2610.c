/* codemn2610.c */
/****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                    */
/*                                                                          */
/* AS, C-Version                                                            */
/*                                                                          */
/* Code Generator for MN161x Processor - alternate version                  */
/*                                                                          */
/****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "be_le.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "motpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"
#include "codepseudo.h"
#include "ibmfloat.h"
#include "onoff_common.h"
#include "chartrans.h"

#include "codemn2610.h"

/*--------------------------------------------------------------------------*/
/* Definitions */

typedef struct
{
  const char *pName;
  Word Code;
} tCodeTable;

typedef struct
{
  Word Code;
  CPUVar MinCPU;
} tFixedOrder;

enum
{
  eAddrRegX0,
  eAddrRegX1,
  eAddrRegIC,
  eAddrRegCnt
};

static CPUVar CPUMN1610, CPUMN1613;
static tSymbolSize OpSize;
static LongInt BaseRegVals[4];

#define ASSUMEMN1613Count 4
static ASSUMERec ASSUMEMN1613[ASSUMEMN1613Count] =
{
  { "CSBR", BaseRegVals + 0, 0, 15, 16, NULL },
  { "SSBR", BaseRegVals + 1, 0, 15, 16, NULL },
  { "TSR0", BaseRegVals + 2, 0, 15, 16, NULL },
  { "TSR1", BaseRegVals + 3, 0, 15, 16, NULL }
};

static tFixedOrder *FixedOrders;

/*--------------------------------------------------------------------------*/
/* Adress Decoders */

static Boolean DecodeTable(const char *pArg, Word *pResult, const tCodeTable *pTable)
{
  for (; pTable->pName; pTable++)
    if (!as_strcasecmp(pArg, pTable->pName))
    {
      *pResult = pTable->Code;
      return True;
    }
  return False;
}

static const tCodeTable RegCodes[] =
{
  { "R0",  0 },
  { "R1",  1 },
  { "R2",  2 },
  { "R3",  3 },
  { "R4",  4 },
  { "SP",  5 },
  { "STR", 6 },
  { "IC",  7 },
  { "X0",  3 },
  { "X1",  4 },
  { NULL,  0 }
};
#define DecodeRegCore(pArg, pResult) (DecodeTable((pArg), (pResult), RegCodes))

static Boolean DecodeReg(const tStrComp *pArg, Word *pResult, Byte Mask)
{
  Boolean Result = DecodeRegCore(pArg->str.p_str, pResult) && (Mask & (1 << *pResult));

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, pArg);
  return Result;
}

static const tCodeTable DRegCodes[] =
{
  { "DR0",  0 },
  { NULL ,  0 }
};
#define DecodeDRegCore(pArg, pResult) (DecodeTable((pArg), (pResult), DRegCodes))

static Boolean DecodeDReg(const tStrComp *pArg)
{
  Word DummyReg;
  Boolean Result = DecodeDRegCore(pArg->str.p_str, &DummyReg) && !DummyReg;

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, pArg);
  return Result;
}

static const tCodeTable SkipCodes[] =
{
  { ""   ,  0 },
  { "SKP",  1 },
  { "M"  ,  2 },
  { "PZ" ,  3 },
  { "E"  ,  4 },
  { "Z"  ,  4 },
  { "NE" ,  5 },
  { "NZ" ,  5 },
  { "MZ" ,  6 },
  { "P"  ,  7 },
  { "EZ" ,  8 },
  { "ENZ",  9 },
  { "OZ" , 10 },
  { "ONZ", 11 },
  { "LMZ", 12 },
  { "LP" , 13 },
  { "LPZ", 14 },
  { "LM" , 15 },
  { NULL ,  0 }
};
#define DecodeSkipCore(pArg, pResult) (DecodeTable((pArg), (pResult), SkipCodes))

static Boolean DecodeSkip(const tStrComp *pArg, Word *pResult)
{
  Boolean Result = DecodeSkipCore(pArg->str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_UndefCond, pArg);
  return Result;
}

static const tCodeTable AddrRegCodes[] =
{
  { "(X0)", eAddrRegX0 },
  { "(X1)", eAddrRegX1 },
  { "(IC)", eAddrRegIC },
  { NULL  ,  0         }
};
#define DecodeAddrReg(pArg, pResult) (DecodeTable((pArg), (pResult), AddrRegCodes))

static const tCodeTable EECodes[] =
{
  { ""  , 0 },
  { "RE", 1 },
  { "SE", 2 },
  { "CE", 3 },
  { NULL, 0 }
};
#define DecodeEECore(pArg, pResult) (DecodeTable((pArg), (pResult), EECodes))

static Boolean DecodeEE(const tStrComp *pArg, Word *pResult)
{
  Boolean Result = DecodeEECore(pArg->str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, pArg);
  return Result;
}

static const tCodeTable AllBRCodes[] =
{
  { "CSBR", 0 },
  { "SSBR", 1 },
  { "TSR0", 2 },
  { "TSR1", 3 },
  { "OSR0", 4 },
  { "OSR1", 5 },
  { "OSR2", 6 },
  { "OSR3", 7 },
  { NULL ,  0 }
};
#define DecodeAllBRCore(pArg, pResult, IsWrite) (DecodeTable((pArg), (pResult), AllBRCodes + IsWrite))

static const tCodeTable BRCodes[] =
{
  { "CSBR", 0 },
  { "SSBR", 1 },
  { "TSR0", 2 },
  { "TSR1", 3 },
  { NULL  , 0 }
};
#define DecodeBRCore(pArg, pResult) (DecodeTable((pArg), (pResult), BRCodes))

static Boolean DecodeBR(const tStrComp *pArg, Word *pResult)
{
  Boolean Result = DecodeBRCore(pArg->str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_UnknownSegReg, pArg);
  return Result;
}


static const tCodeTable SRegCodes[] =
{
  { "SBRB", 0 },
  { "ICB" , 1 },
  { "NPP" , 2 },
  { NULL  , 0 }
};
#define DecodeSRegCore(pArg, pResult) (DecodeTable((pArg), (pResult), SRegCodes))

static Boolean DecodeSOrAllBReg(const tStrComp *pArg, Word *pResult, unsigned IsS, Boolean IsWrite)
{
  Boolean Result = IsS ? DecodeSRegCore(pArg->str.p_str, pResult) : DecodeAllBRCore(pArg->str.p_str, pResult, IsWrite);

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, pArg);
  return Result;
}

static tCodeTable HRegCodes[] =
{
  { "TCR" , 0 },
  { "TIR" , 1 },
  { "TSR" , 2 },
  { "SCR" , 3 },
  { "SSR" , 4 },
  { NULL  , 5 }, /* filled @ runtime with SOR/SIR */
  { "IISR", 6 },
  { NULL  , 0 }
};
#define DecodeHRegCore(pArg, pResult) (DecodeTable((pArg), (pResult), HRegCodes))

static Boolean DecodeHReg(const tStrComp *pArg, Word *pResult, Boolean IsWrite)
{
  Boolean Result;

  HRegCodes[5].pName = IsWrite ? "SOR" : "SIR";
  Result = DecodeHRegCore(pArg->str.p_str, pResult);

  if (!Result)
    WrStrErrorPos(ErrNum_InvReg, pArg);
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     Word ChkPage(LongWord Addr, Word Base, const tStrComp *pArg, bool Warn)
 * \brief  check whether a linear address is reachable via the given segment register, and compute offset
 * \param  Addr linear address in 64/256K space
 * \param  Base index of segment register to be used
 * \param  pArg textual argument for warnings (may be NULL)
 * \return resulting offset
 * ------------------------------------------------------------------------ */

static Word ChkPage(LongWord Addr, Word Base, const tStrComp *pArg)
{
  Addr -= BaseRegVals[Base] << 14;
  Addr &= SegLimits[SegCode];
  if ((Addr >= 0x10000ul) && pArg)
    WrStrErrorPos(ErrNum_InAccPage, pArg);
  return Addr & 0xffff;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMem(tStrComp *pArg, Word *pResult)
 * \brief  parse memory address expression
 * \param  pArg memory argument in source
 * \param  pResult resulting code for instruction word (xxMMMxxxDDDDDDDD)
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean DecodeMem(tStrComp *pArg, Word *pResult)
{
  tStrComp Arg;
  Boolean TotIndirect = IsIndirect(pArg->str.p_str), OK;
  Word R;
  Integer Disp;
  int l;
  tSymbolFlags Flags;

  if (TotIndirect)
  {
    StrCompRefRight(&Arg, pArg, 1);
    KillPrefBlanksStrCompRef(&Arg);
    StrCompShorten(&Arg, 1);
    KillPostBlanksStrComp(&Arg);
  }
  else
    StrCompRefRight(&Arg, pArg, 0);

  l = strlen(Arg.str.p_str);
  if ((l >= 4) && DecodeAddrReg(Arg.str.p_str + l - 4, &R))
  {
    Boolean DispIndirect;

    StrCompShorten(&Arg, 4);
    KillPostBlanksStrComp(&Arg);
    DispIndirect = IsIndirect(Arg.str.p_str);
    Disp = EvalStrIntExpression(&Arg, (R == 2) ? SInt8 : UInt8, &OK);
    if (!OK)
      return False;
    if (R == 2)
    {
      if (DispIndirect)  /* ((disp)(IC)), (disp)(IC) not possible */
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return False;
      }
      else /* (disp(IC)) */
      {
        *pResult = ((TotIndirect ? 3 : 1) << 11) | (Disp & 0xff);
        return True;
      }
    }
    else if (TotIndirect) /* (disp(Xn)), ((disp)(Xn)) not possible */
    {
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    }
    else /* disp(Xn), (disp)(Xn) */
    {
      *pResult = (((DispIndirect ? 6 : 4) + R) << 11) | (Disp & 0xff);
      return True;
    }
  }
  else if (TotIndirect && DecodeRegCore(Arg.str.p_str, &R)) /* plain (ic) (x0) (x1) without displacement */
  {
    switch (R)
    {
      case 3:
        *pResult = (4 << 11);
        return True;
      case 4:
        *pResult = (5 << 11);
        return True;
      case 7:
        *pResult = (1 << 11);
        return True;
      default:
        goto plaindisp;
    }
  }
  else
  {
    int ArgOffset, ForceMode;
    LongWord Addr;

plaindisp:
    /* For (Addr), there is only the 'zero-page' variant: */

    if (TotIndirect)
    {
      ArgOffset = 0;
      ForceMode = 1;
    }

    /* For direct addressing, either zero-page or IC-relative may be used.
       Check for explicit request: */

    else switch (*Arg.str.p_str)
    {
      case '>':
        ArgOffset = 1;
        ForceMode = 2;
        break;
      case '<':
        ArgOffset = 1;
        ForceMode = 1;
        break;
      default:
        ArgOffset = ForceMode = 0;
    }

    /* evaluate expression */

    Addr = EvalStrIntExpressionOffsWithFlags(&Arg, ArgOffset, (MomCPU == CPUMN1613) ? UInt18 : UInt16, &OK, &Flags);
    if (!OK)
      return False;

    /* now generate mode */

    switch (ForceMode)
    {
      case 0:
      {
        if (ChkPage(Addr, 0, NULL) < 256)
          goto case1;
        else
          goto case2;
      }
      case 1:
      case1:
      {
        Word Offset = ChkPage(Addr, 0, NULL);

        if (!mFirstPassUnknownOrQuestionable(Flags) && (Offset > 255))
        {
          WrStrErrorPos(ErrNum_OverRange, pArg);
          return False;
        }
        else
        {
          *pResult = ((TotIndirect ? 2 : 0) << 11) | (Offset & 0xff);
          return True;
        }
      }
      case 2:
      case2:
      {
        LongInt Disp = Addr - EProgCounter();

        if (!mFirstPassUnknownOrQuestionable(Flags) && ((Disp > 127) || (Disp < -128)))
        {
          WrStrErrorPos(ErrNum_DistTooBig, pArg);
          return False;
        }
        else
        {
          *pResult = (1 << 11) | (Disp & 0xff);
          return True;
        }
      }
      default:
        return False;
    }
  }
}

static Boolean DecodeIReg(tStrComp *pStrArg, Word *pResult, Word Mask, Word Allowed)
{
  char *pArg = pStrArg->str.p_str;
  int l = strlen(pArg);
  char *pEnd = pArg + l, Save;
  Word Reg;
  Boolean OK;

  if ((l > 2) && (*pArg == '(') && (*(pEnd - 1) == ')'))
  {
    pArg++; pEnd--;
    *pResult = 1 << 6;
  }
  else if ((l > 3) && (*pArg == '(') && (*(pEnd - 2) == ')') && (*(pEnd - 1) == '+'))
  {
    pArg++; pEnd -= 2;
    *pResult = 3 << 6;
  }
  else if ((l > 3) && (*pArg == '-') && (pArg[1] == '(') && (*(pEnd - 1) == ')'))
  {
    pArg += 2; pEnd--;
    *pResult = 2 << 6;
  }
  else
    goto error;

  while ((pArg < pEnd) && isspace(*pArg))
    pArg++;
  while ((pEnd > pArg) && isspace(*(pEnd - 1)))
    pEnd--;
  Save = *pEnd; *pEnd = '\0';

  OK = DecodeRegCore(pArg, &Reg);
  *pEnd = Save;
  if (!OK || (Reg < 1) || (Reg > 4))
    goto error;

  *pResult |= (Reg - 1);
  if ((*pResult & Mask) != Allowed)
    goto error;
  return True;

error:
  WrStrErrorPos(ErrNum_InvAddrMode, pStrArg);
  return False;
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Index)
{
  const tFixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0) && ChkMinCPU(pOrder->MinCPU))
  {
    WAsmCode[0] = pOrder->Code;
    CodeLen = 1;
  }
}

static void DecodeOneReg(Word Code)
{
  Word Reg;

  if (ChkArgCnt(1, 1)
   && DecodeReg(&ArgStr[1], &Reg, 0x7f)) /* do not allow IC as register */
  {
    WAsmCode[0] = Code | (Reg << 8);
    CodeLen = 1;
  }
}

static void DecodeRegImm(Word Code)
{
  Word Reg;

  if (ChkArgCnt(2, 2)
   && DecodeReg(&ArgStr[1], &Reg, 0x7f)) /* do not allow IC as register */
  {
    tEvalResult EvalResult;

    WAsmCode[0] = EvalStrIntExpressionWithResult(&ArgStr[2], Int8, &EvalResult) & 0xff;
    if (EvalResult.OK)
    {
      if (Code & 0x1000)
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      WAsmCode[0] |= Code | (Reg << 8);
      CodeLen = 1;
    }
  }
}

static void DecodeTwoReg(Word Code)
{
  Word Rd, Rs, Skip = 0;

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[1], &Rd, 0x7f) /* do not allow IC as register */
   && DecodeReg(&ArgStr[2], &Rs, 0x7f) /* do not allow IC as register */
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    WAsmCode[0] = Code | (Rd << 8) | (Skip << 4) | Rs;
    CodeLen = 1;
  }
}

static void DecodeEAReg(Word Code)
{
  Word R, EA;

  if (ChkArgCnt(2, 2)
   && DecodeReg(&ArgStr[1], &R, 0x3f)
   && DecodeMem(&ArgStr[2], &EA))
  {
    WAsmCode[0] = Code | EA | (R << 8);
    CodeLen = 1;
  }
}

static void DecodeEA(Word Code)
{
  Word EA;

  if (ChkArgCnt(1, 1)
   && DecodeMem(&ArgStr[1], &EA))
  {
    WAsmCode[0] = Code | EA;
    CodeLen = 1;
  }
}

static void DecodeShift(Word Code)
{
  Word R, Skip = 0, EE = 0;

  if (ChkArgCnt(1, 3)
   && DecodeReg(&ArgStr[1], &R, 0x7f)) /* do not allow IC as register */
  {
    Boolean OK;

    if (ArgCnt == 3)
      OK = DecodeEE(&ArgStr[2], &EE) && DecodeSkip(&ArgStr[3], &Skip);
    else if (ArgCnt == 1)
      OK = True;
    else
      OK = DecodeEECore(ArgStr[2].str.p_str, &EE) || DecodeSkip(&ArgStr[2], &Skip);
    if (OK)
    {
      WAsmCode[0] = Code | (R << 8) | (Skip << 4) | EE;
      CodeLen = 1;
    }
  }
}

static void DecodeImm4(Word Code)
{
  Word R, Skip = 0;

  if (ChkArgCnt(2, 3)
   && DecodeReg(&ArgStr[1], &R, 0x7f)
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    Word Num;
    Boolean OK;

    Num = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
    if (OK)
    {
      WAsmCode[0] = Code | (R << 8) | (Skip << 4) | (Num & 15);
      CodeLen = 1;
    }
  }
}

static void DecodeImm2(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[0] = Code | EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeLD_STD(Word Code)
{
  Word R, Base = 0;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x3f) /* do not allow IC/STR as register */
   && ((ArgCnt <= 2) || DecodeBR(&ArgStr[2], &Base)))
  {
    Boolean OK;
    LongWord Addr;

    Addr = EvalStrIntExpression(&ArgStr[ArgCnt], UInt18, &OK);
    if (OK)
    {
      WAsmCode[0] = Code | R | (Base << 4);
      WAsmCode[1] = ChkPage(Addr, Base, &ArgStr[ArgCnt]);
      CodeLen = 2;
    }
  }
}

static void DecodeLR_STR(Word Code)
{
  Word R, IR, Base = 0;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x3f) /* do not allow STR/IC as register */
   && ((ArgCnt <= 2) || DecodeBR(&ArgStr[2], &Base))
   && DecodeIReg(&ArgStr[ArgCnt], &IR, 0x00, 0x00))
  {
    WAsmCode[0] = Code | (R << 8) | IR | (Base << 4);
    CodeLen = 1;
  }
}

static void DecodeR0RISkip(Word Code)
{
  Word R, RI, Skip = 0;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x01)
   && DecodeIReg(&ArgStr[2], &RI, 0xc0, 0x40)
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4) | (RI & 3);
    CodeLen = 1;
  }
}

static void DecodeRImmSkip(Word Code)
{
  Word R, Skip = 0;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x7f) /* do not allow IC as register */
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    Boolean OK;

    WAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int16, &OK);
    if (OK)
    {
      WAsmCode[0] = Code | (R << 8) | (Skip << 4);
      CodeLen = 2;
    }
  }
}

static Boolean ChkCarry(int StartIndex, Boolean *pHasCarryArg, Word *pCarryVal)
{
  if (ArgCnt < StartIndex)
  {
    *pHasCarryArg = False;
    *pCarryVal = 0;
    return True;
  }
  else
  {
    Boolean Result;

    *pHasCarryArg = !as_strcasecmp(ArgStr[StartIndex].str.p_str, "C")
                 || !as_strcasecmp(ArgStr[StartIndex].str.p_str, "0")
                 || !as_strcasecmp(ArgStr[StartIndex].str.p_str, "1");
    *pCarryVal = *pHasCarryArg && (ArgStr[StartIndex].str.p_str[0] != '0');
    Result = (ArgCnt == StartIndex) || *pHasCarryArg;
    if (!Result)
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[StartIndex]);
    return Result;
  }
}

static void DecodeDR0RISkip(Word Code)
{
  Word Skip = 0, Ri, CarryVal = 0;
  Boolean OptCarry = !!(Code & 0x10), HasCarryArg = False;

  Code &= ~0x10;
  if (ChkArgCnt(2, 3 + OptCarry)
   && ChkMinCPU(CPUMN1613)
   && DecodeDReg(&ArgStr[1])
   && DecodeIReg(&ArgStr[2], &Ri, 0xc0, 0x40)
   && (!OptCarry || ChkCarry(3, &HasCarryArg, &CarryVal))
   && ((ArgCnt < 3 + HasCarryArg) || DecodeSkip(&ArgStr[3 + HasCarryArg], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4) | (Ri & 3) | (CarryVal << 3);
    CodeLen = 1;
  }
}

static void DecodeDAA_DAS(Word Code)
{
  Word R, Ri, Skip = 0, CarryVal = 0;
  Boolean HasCarryArg = False;

  if (ChkArgCnt(2, 4)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x01)
   && DecodeIReg(&ArgStr[2], &Ri, 0xc0, 0x40)
   && ChkCarry(3, &HasCarryArg, &CarryVal)
   && ((ArgCnt < 3 + HasCarryArg) || DecodeSkip(&ArgStr[3 + HasCarryArg], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4) | (Ri & 3) | (CarryVal << 3);
    CodeLen = 1;
  }
}

static void DecodeNEG(Word Code)
{
  Word R, Skip = 0, CarryVal = 0;
  Boolean HasCarryArg = False;

  if (ChkArgCnt(1, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x7f) /* do not allow IC as register */
   && ChkCarry(2, &HasCarryArg, &CarryVal)
   && ((ArgCnt < 2 + HasCarryArg) || DecodeSkip(&ArgStr[2 + HasCarryArg], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4) | R | (CarryVal << 3);
    CodeLen = 1;
  }
}

static void DecodeRDR_WTR(Word Code)
{
  Word R, Ri;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x3f) /* do not allow IC/STR */
   && DecodeIReg(&ArgStr[2], &Ri, 0xc0, 0x40))
  {
    WAsmCode[0] = Code | (R << 8) | (Ri & 3);
    CodeLen = 1;
  }
}

static void DecodeBD_BALD(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPUMN1613))
  {
    Boolean OK;
    LongWord Addr = EvalStrIntExpression(&ArgStr[1], UInt18, &OK);

    if (OK)
    {
      WAsmCode[0] = Code;
      WAsmCode[1] = ChkPage(Addr, 0, &ArgStr[1]);
      CodeLen = 2;
    }
  }
}

static void DecodeBL_BALL(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPUMN1613));
  else if (!IsIndirect(ArgStr[1].str.p_str)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
  else
  {
    Boolean OK;
    LongWord Addr = EvalStrIntExpression(&ArgStr[1], UInt18, &OK);

    if (OK)
    {
      WAsmCode[0] = Code;
      WAsmCode[1] = ChkPage(Addr, 0, &ArgStr[1]);
      CodeLen = 2;
    }
  }
}

static void DecodeBR_BALR(Word Code)
{
  Word Ri;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPUMN1613)
   && DecodeIReg(&ArgStr[1], &Ri, 0xc0, 0x40))
  {
    WAsmCode[0] = Code | (Ri & 3);
    CodeLen = 1;
  }
}

static void DecodeTSET_TRST(Word Code)
{
  Word R, Skip = 0;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x7f) /* do not allow IC as register */
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    Boolean OK;
    LongWord Addr = EvalStrIntExpression(&ArgStr[2], UInt18, &OK);

    if (OK)
    {
      WAsmCode[0] = Code | R | (Skip << 4);
      WAsmCode[1] = ChkPage(Addr, 0, &ArgStr[2]);
      CodeLen = 2;
    }
  }
}

static void DecodeFIX(Word Code)
{
  Word Skip = 0, R;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x01)
   && DecodeDReg(&ArgStr[2])
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4);
    CodeLen = 1;
  }
}

static void DecodeFLT(Word Code)
{
  Word Skip = 0, R;

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeDReg(&ArgStr[1])
   && DecodeReg(&ArgStr[2], &R, 0x01)
   && ((ArgCnt < 3) || DecodeSkip(&ArgStr[3], &Skip)))
  {
    WAsmCode[0] = Code | (Skip << 4);
    CodeLen = 1;
  }
}

static void DecodeSRBT(Word Code)
{
  Word R;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x01)
   && DecodeReg(&ArgStr[2], &R, 0x7f))
  {
    WAsmCode[0] = Code | R;
    CodeLen = 1;
  }
}

static void DecodeDEBP(Word Code)
{
  Word R;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[2], &R, 0x01)
   && DecodeReg(&ArgStr[1], &R, 0x7f))
  {
    WAsmCode[0] = Code | R;
    CodeLen = 1;
  }
}

static void DecodeBLK(Word Code)
{
  Word Ri;

  if (ChkArgCnt(3, 3)
   && ChkMinCPU(CPUMN1613)
   && DecodeIReg(&ArgStr[1], &Ri, 0xff, 0x41)
   && DecodeIReg(&ArgStr[2], &Ri, 0xff, 0x40)
   && DecodeReg(&ArgStr[3], &Ri, 0x01))
  {
    WAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeLBS_STBS(Word Code)
{
  Word R;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeSOrAllBReg(&ArgStr[1], &R, Code & 8, !(Code & 0x80)))
  {
    Boolean OK;
    LongWord Addr = EvalStrIntExpression(&ArgStr[2], UInt18, &OK);

    if (OK)
    {
      WAsmCode[0] = Code | (R << 4);
      WAsmCode[1] = ChkPage(Addr, 0, &ArgStr[2]);
      CodeLen = 2;
    }
  }
}

static void DecodeCPYBS_SETBS(Word Code)
{
  Word R, BSR;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0x7f) /* do not allow IC */
   && DecodeSOrAllBReg(&ArgStr[2], &BSR, Code & 8, !(Code & 0x80)))
  {
    WAsmCode[0] = Code | R | (BSR << 4);
    CodeLen = 1;
  }
}

static void DecodeCPYH_SETH(Word Code)
{
  Word R, BSR;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUMN1613)
   && DecodeReg(&ArgStr[1], &R, 0xff)
   && DecodeHReg(&ArgStr[2], &BSR, !(Code & 0x80)))
  {
    WAsmCode[0] = Code | R | (BSR << 4);
    CodeLen = 1;
  }
}

static void DecodeCLR(Word Code)
{
  Word R;

  if (ChkArgCnt(1, 1)
   && DecodeReg(&ArgStr[1], &R, 0x7f)) /* do not allow IC */
  {
    /* == EOR R,R */

    WAsmCode[0] = Code | (R << 8) | R;
    CodeLen = 1;
  }
}

static void DecodeSKIP(Word Code)
{
  Word R, Skip;

  if (ChkArgCnt(2, 2)
   && DecodeReg(&ArgStr[1], &R, 0x7f) /* do not allow IC */
   && DecodeSkip(&ArgStr[2], &Skip))
  {
    /* == MV R,R,SKIP */

    WAsmCode[0] = Code | (R << 8) | (Skip << 4) | R;
  }
}

void IncMaxCodeLen(unsigned NumWords)
{
  SetMaxCodeLen((CodeLen + NumWords) * 2);
}

static void AppendWord(Word data, Boolean *p_half_filled_word)
{
  IncMaxCodeLen(1);
  WAsmCode[CodeLen++] = data;
  *p_half_filled_word = False;
}

static void AppendByte(Byte data, Boolean *p_half_filled_word)
{
  if (*p_half_filled_word)
  {
    WAsmCode[CodeLen - 1] |= data & 0xff;
    *p_half_filled_word = False;
  }
  else
  {
    AppendWord(data << 8, p_half_filled_word);
    *p_half_filled_word = True;
  }
}

static void DecodeDC(Word Code)
{
  Boolean HalfFilledWord = False;
  TempResult t;

  UNUSED(Code);

  as_tempres_ini(&t);
  if (ChkArgCnt(1, ArgCntMax))
  {
    Boolean OK = True;
    tStrComp *pArg;

    forallargs(pArg, OK)
    {
      EvalStrExpression(pArg, &t);
      if (mFirstPassUnknown(t.Flags) && (t.Typ == TempInt)) t.Contents.Int &= 0x7fff;
      switch (t.Typ)
      {
        case TempInt:
          if (Packing)
          {
            if (mFirstPassUnknown(t.Flags))
              t.Contents.Int &= 127;
            if (ChkRange(t.Contents.Int, -128, 255))
              AppendByte(t.Contents.Int, &HalfFilledWord);
            else
              OK = False;
          }
          else
          {
          ToInt:
            if (mFirstPassUnknown(t.Flags))
              t.Contents.Int &= 32767;
            if (ChkRange(t.Contents.Int, -32768, 65535))
              AppendWord(t.Contents.Int, &HalfFilledWord);
            else
              OK = False;
          }
          break;
        case TempString:
        {
          Word Trans;
          int z2;

          if (MultiCharToInt(&t, 2))
            goto ToInt;

          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else
            for (z2 = 0; z2 < (int)t.Contents.str.len; z2++)
            {
              Trans = ((usint) t.Contents.str.p_str[z2]) & 0xff;
              AppendByte(Trans, &HalfFilledWord);
            }
          break;
        }
        case TempFloat:
        {
          IncMaxCodeLen(2);
          if (Double2IBMFloat(&WAsmCode[CodeLen], t.Contents.Float, False))
            CodeLen += 2;
          else
            OK = False;
          HalfFilledWord = False;
          break;
        }
        default:
          OK = False;
      }
    }
    if (!OK)
       CodeLen = 0;
  }
  as_tempres_free(&t);
}

static void DecodeDS(Word Index)
{
  Boolean OK;
  Word Size;
  tSymbolFlags Flags;

  UNUSED(Index);

  Size = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
  if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
  if (OK && !mFirstPassUnknown(Flags))
  {
    DontPrint = True;
    if (!Size) WrError(ErrNum_NullResMem);
    CodeLen = Size;
    BookKeeping();
  }
}

/*--------------------------------------------------------------------------*/
/* Codetabellen */

static void AddFixed(const char *pName, Word Code, CPUVar MinCPU)
{
  order_array_rsv_end(FixedOrders, tFixedOrder);
  FixedOrders[InstrZ].Code = Code;
  FixedOrders[InstrZ].MinCPU = MinCPU;
  AddInstTable(InstTable, pName, InstrZ++, DecodeFixed);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  InstrZ = 0;
  AddFixed("H"   , 0x2000 , CPUMN1610);
  AddFixed("RET" , 0x2003 , CPUMN1610);
  AddFixed("NOP" , NOPCode, CPUMN1610);
  AddFixed("PSHM", 0x170f , CPUMN1613);
  AddFixed("POPM", 0x1707 , CPUMN1613);
  AddFixed("RETL", 0x3f07 , CPUMN1613);

  AddInstTable(InstTable, "PUSH", 0x2001, DecodeOneReg);
  AddInstTable(InstTable, "POP" , 0x2002, DecodeOneReg);

  /* allow WR as alias to WT */

  AddInstTable(InstTable, "RD"  , 0x1800, DecodeRegImm);
  AddInstTable(InstTable, "WT"  , 0x1000, DecodeRegImm);
  AddInstTable(InstTable, "WR"  , 0x1000, DecodeRegImm);
  AddInstTable(InstTable, "MVI" , 0x0800, DecodeRegImm);

  AddInstTable(InstTable, "A"   , 0x5808, DecodeTwoReg);
  AddInstTable(InstTable, "S"   , 0x5800, DecodeTwoReg);
  AddInstTable(InstTable, "C"   , 0x5008, DecodeTwoReg);
  AddInstTable(InstTable, "CB"  , 0x5000, DecodeTwoReg);
  AddInstTable(InstTable, "MV"  , 0x7808, DecodeTwoReg);
  AddInstTable(InstTable, "MVB" , 0x7800, DecodeTwoReg);
  AddInstTable(InstTable, "BSWP", 0x7008, DecodeTwoReg);
  AddInstTable(InstTable, "DSWP", 0x7000, DecodeTwoReg);
  AddInstTable(InstTable, "LAD" , 0x6800, DecodeTwoReg);
  AddInstTable(InstTable, "AND" , 0x6808, DecodeTwoReg);
  AddInstTable(InstTable, "OR"  , 0x6008, DecodeTwoReg);
  AddInstTable(InstTable, "EOR" , 0x6000, DecodeTwoReg);

  AddInstTable(InstTable, "L"   , 0xc000, DecodeEAReg);
  AddInstTable(InstTable, "ST"  , 0x8000, DecodeEAReg);
  AddInstTable(InstTable, "B"   , 0xc700, DecodeEA);
  AddInstTable(InstTable, "BAL" , 0x8700, DecodeEA);
  AddInstTable(InstTable, "IMS" , 0xc600, DecodeEA);
  AddInstTable(InstTable, "DMS" , 0x8600, DecodeEA);

  AddInstTable(InstTable, "SL"  , 0x200c, DecodeShift);
  AddInstTable(InstTable, "SR"  , 0x2008, DecodeShift);

  AddInstTable(InstTable, "SBIT", 0x3800, DecodeImm4);
  AddInstTable(InstTable, "RBIT", 0x3000, DecodeImm4);
  AddInstTable(InstTable, "TBIT", 0x2800, DecodeImm4);
  AddInstTable(InstTable, "AI"  , 0x4800, DecodeImm4);
  AddInstTable(InstTable, "SI"  , 0x4000, DecodeImm4);

  AddInstTable(InstTable, "LPSW", 0x2004, DecodeImm2);

  /* new to MN1613 */

  AddInstTable(InstTable, "LD"  , 0x2708, DecodeLD_STD);
  AddInstTable(InstTable, "STD" , 0x2748, DecodeLD_STD);
  AddInstTable(InstTable, "LR"  , 0x2000, DecodeLR_STR);
  AddInstTable(InstTable, "STR" , 0x2004, DecodeLR_STR);

  AddInstTable(InstTable, "MVWR", 0x7f08, DecodeR0RISkip);
  AddInstTable(InstTable, "MVBR", 0x7f00, DecodeR0RISkip);
  AddInstTable(InstTable, "BSWR", 0x7708, DecodeR0RISkip);
  AddInstTable(InstTable, "DSWR", 0x7700, DecodeR0RISkip);
  AddInstTable(InstTable, "AWR" , 0x5f08, DecodeR0RISkip);
  AddInstTable(InstTable, "SWR" , 0x5f00, DecodeR0RISkip);
  AddInstTable(InstTable, "CWR" , 0x5708, DecodeR0RISkip);
  AddInstTable(InstTable, "CBR" , 0x5700, DecodeR0RISkip);
  AddInstTable(InstTable, "LADR", 0x6f00, DecodeR0RISkip);
  AddInstTable(InstTable, "ANDR", 0x6f08, DecodeR0RISkip);
  AddInstTable(InstTable, "ORR" , 0x6708, DecodeR0RISkip);
  AddInstTable(InstTable, "EORR", 0x6700, DecodeR0RISkip);

  AddInstTable(InstTable, "MVWI", 0x780f, DecodeRImmSkip);
  AddInstTable(InstTable, "AWI" , 0x580f, DecodeRImmSkip);
  AddInstTable(InstTable, "SWI" , 0x5807, DecodeRImmSkip);
  AddInstTable(InstTable, "CWI" , 0x500f, DecodeRImmSkip);
  AddInstTable(InstTable, "CBI" , 0x5007, DecodeRImmSkip);
  AddInstTable(InstTable, "LADI", 0x6807, DecodeRImmSkip);
  AddInstTable(InstTable, "ANDI", 0x680f, DecodeRImmSkip);
  AddInstTable(InstTable, "ORI" , 0x600f, DecodeRImmSkip);
  AddInstTable(InstTable, "EORI", 0x6007, DecodeRImmSkip);

  AddInstTable(InstTable, "AD"  , 0x4f14, DecodeDR0RISkip);
  AddInstTable(InstTable, "SD"  , 0x4714, DecodeDR0RISkip);
  AddInstTable(InstTable, "M"   , 0x7f0c, DecodeDR0RISkip);
  AddInstTable(InstTable, "D"   , 0x770c, DecodeDR0RISkip);
  AddInstTable(InstTable, "FA"  , 0x6f0c, DecodeDR0RISkip);
  AddInstTable(InstTable, "FS"  , 0x6f04, DecodeDR0RISkip);
  AddInstTable(InstTable, "FM"  , 0x670c, DecodeDR0RISkip);
  AddInstTable(InstTable, "FD"  , 0x6704, DecodeDR0RISkip);

  AddInstTable(InstTable, "DAA" , 0x5f04, DecodeDAA_DAS);
  AddInstTable(InstTable, "DAS" , 0x5704, DecodeDAA_DAS);

  AddInstTable(InstTable, "RDR" , 0x2014, DecodeRDR_WTR);
  AddInstTable(InstTable, "WTR" , 0x2010, DecodeRDR_WTR);

  AddInstTable(InstTable, "BD"  , 0x2607, DecodeBD_BALD);
  AddInstTable(InstTable, "BALD", 0x2617, DecodeBD_BALD);

  AddInstTable(InstTable, "BL"  , 0x270f, DecodeBL_BALL);
  AddInstTable(InstTable, "BALL", 0x271f, DecodeBL_BALL);

  AddInstTable(InstTable, "BR"  , 0x2704, DecodeBR_BALR);
  AddInstTable(InstTable, "BALR", 0x2714, DecodeBR_BALR);

  AddInstTable(InstTable, "TSET", 0x1708, DecodeTSET_TRST);
  AddInstTable(InstTable, "TRST", 0x1700, DecodeTSET_TRST);

  AddInstTable(InstTable, "NEG" , 0x1f00, DecodeNEG);
  AddInstTable(InstTable, "FIX" , 0x1f0f, DecodeFIX);
  AddInstTable(InstTable, "FLT" , 0x1f07, DecodeFLT);

  AddInstTable(InstTable, "SRBT", 0x3f70, DecodeSRBT);
  AddInstTable(InstTable, "DEBP", 0x3ff0, DecodeDEBP);
  AddInstTable(InstTable, "BLK" , 0x3f17, DecodeBLK);

  AddInstTable(InstTable, "LB"  , 0x0f07, DecodeLBS_STBS);
  AddInstTable(InstTable, "LS"  , 0x0f0f, DecodeLBS_STBS);
  AddInstTable(InstTable, "STB" , 0x0f87, DecodeLBS_STBS);
  AddInstTable(InstTable, "STS" , 0x0f8f, DecodeLBS_STBS);

  AddInstTable(InstTable, "CPYB", 0x0f80, DecodeCPYBS_SETBS);
  AddInstTable(InstTable, "CPYS", 0x0f88, DecodeCPYBS_SETBS);
  AddInstTable(InstTable, "SETB", 0x0f00, DecodeCPYBS_SETBS);
  AddInstTable(InstTable, "SETS", 0x0f08, DecodeCPYBS_SETBS);

  AddInstTable(InstTable, "CPYH", 0x3f80, DecodeCPYH_SETH);
  AddInstTable(InstTable, "SETH", 0x3f00, DecodeCPYH_SETH);

  /* aliases */

  AddInstTable(InstTable, "CLR",   0x6000, DecodeCLR);
  AddInstTable(InstTable, "CLEAR", 0x6000, DecodeCLR);
  AddInstTable(InstTable, "SKIP",  0x7808, DecodeSKIP);

  /* pseudo instructions */

  AddInstTable(InstTable, "DC"  , 0, DecodeDC);
  AddInstTable(InstTable, "DS"  , 0, DecodeDS);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
}

/*--------------------------------------------------------------------------*/
/* Interface to AS */

static Boolean DecodeAttrPart_MN1610_Alt(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_MN1610_Alt(void)
{
  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize16Bit;

  /* Ignore empty instruction */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_MN1610_Alt(void)
{
  return FALSE;
}

static void SwitchFrom_MN1610_Alt(void)
{
  DeinitFields();
}

static void SwitchTo_MN1610_Alt(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("MN161x");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIBM);

  PCSymbol = "*";
  HeaderID = FoundDescr->Id;
  NOPCode = 0x7808; /* MV R0,R0 */
  DivideChars = ",";
  HasAttrs = False;
  AttrChars = ".";

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode]     = Grans[SegIO]     = 2;
  ListGrans[SegCode] = ListGrans[SegIO] = 2;
  SegInits[SegCode]  = SegInits[SegIO]  = 0;
  if (MomCPU == CPUMN1613)
  {
    SegLimits[SegCode] = 0x3ffff;
    SegLimits[SegIO] = 0xffff;
    pASSUMERecs = ASSUMEMN1613;
    ASSUMERecCnt = ASSUMEMN1613Count;
  }
  else
  {
    SegLimits[SegCode] = 0xffff;
    SegLimits[SegIO] = 0xff; /* no RDR/WTR insn */
  }

  onoff_packing_add(False);

  DecodeAttrPart = DecodeAttrPart_MN1610_Alt;
  MakeCode = MakeCode_MN1610_Alt;
  IsDef = IsDef_MN1610_Alt;
  SwitchFrom = SwitchFrom_MN1610_Alt;
  InitFields();
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

void codemn2610_init(void)
{
  CPUMN1610 = AddCPU("MN1610ALT", SwitchTo_MN1610_Alt);
  CPUMN1613 = AddCPU("MN1613ALT", SwitchTo_MN1610_Alt);
}
