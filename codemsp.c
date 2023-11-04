/* codemsp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator MSP430                                                      */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "be_le.h"
#include "strutil.h"
#include "bpemu.h"
#include "chunks.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmcode.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "chartrans.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "codevars.h"

typedef struct
{
  Boolean MayByte;
  Word Code;
} OneOpOrder;

typedef enum
{
  eModeReg = 0,
  eModeRegDisp = 1,
  eModeIReg = 2,
  eModeIRegAutoInc = 3,
  eModeNone = 0xff
} tMode;

#define MModeReg (1 << eModeReg)
#define MModeRegDisp (1 << eModeRegDisp)
#define MModeIReg (1 << eModeIReg)
#define MModeIRegAutoInc (1 << eModeIRegAutoInc)
#define MModeAs 15
#define MModeAd 3

typedef enum
{
  eExtModeNo = 0,
  eExtModeYes = 1
} tExtMode;

typedef enum
{
  eOpSizeB = 0,
  eOpSizeW = 1,
  eOpSizeA = 2,
  eOpSizeCnt,
  eOpSizeDefault = eOpSizeW
} tOpSize;

#define RegPC 0
#define RegSP 1
#define RegSR 2
#define RegCG1 2
#define RegCG2 3

#define REG_PC 0
#define REG_SP 1
#define REG_SR 2

typedef struct
{
  Word Mode, Part, Cnt;
  LongWord Val;
  Boolean WasImm, WasAbs;
} tAdrParts;

/*  float exp (8bit bias 128) sign mant (impl. norm.)
   double exp (8bit bias 128) sign mant (impl. norm.) */

static CPUVar CPUMSP430, CPUMSP430X;

static OneOpOrder *OneOpOrders;

static tOpSize OpSize;
static Word PCDist, MultPrefix;
static IntType AdrIntType, DispIntType;
static const IntType OpSizeIntTypes[eOpSizeCnt] = { Int8, Int16, Int20 };

/*-------------------------------------------------------------------------*/

static void ResetAdr(tAdrParts *pAdrParts)
{
  pAdrParts->Mode = eModeNone;
  pAdrParts->Part = 0;
  pAdrParts->Cnt = 0;
  pAdrParts->WasImm =
  pAdrParts->WasAbs = False;
}

static Boolean ChkAdr(Byte Mask, tAdrParts *pAdrParts)
{
  if ((pAdrParts->Mode != 0xff) && ((Mask & (1 << pAdrParts->Mode)) == 0))
  {
    ResetAdr(pAdrParts);
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pResult)
{
  if (!as_strcasecmp(pArg, "PC"))
  {
    *pResult = REGSYM_FLAG_ALIAS | REG_PC; return True;
  }
  else if (!as_strcasecmp(pArg,"SP"))
  {
    *pResult = REGSYM_FLAG_ALIAS | REG_SP; return True;
  }
  else if (!as_strcasecmp(pArg, "SR"))
  {
    *pResult = REGSYM_FLAG_ALIAS | REG_SR; return True;
  }
  if ((as_toupper(*pArg) == 'R') && (strlen(pArg) >= 2) && (strlen(pArg) <= 3))
  {
    Boolean OK;

    *pResult = ConstLongInt(pArg + 1, &OK, 10);
    return OK && (*pResult < 16);
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_MSP(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - MSP variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_MSP(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      switch (Value)
      {
        case REGSYM_FLAG_ALIAS | REG_PC:
          as_snprintf(pDest, DestSize, "PC");
          break;
        case REGSYM_FLAG_ALIAS | REG_SP:
          as_snprintf(pDest, DestSize, "SP");
          break;
        case REGSYM_FLAG_ALIAS | REG_SR:
          as_snprintf(pDest, DestSize, "SR");
          break;
        default:
          as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      }
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  MustBeReg excpecting register as arg
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return True;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize8Bit, MustBeReg);
  *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  return (RegEvalResult == eIsReg);
}

static void FillAdrPartsImm(tAdrParts *pAdrParts, LongWord Value, Boolean ForceLong)
{
  ResetAdr(pAdrParts);
  pAdrParts->WasImm = True;
  pAdrParts->Val = Value;

  /* assume no usage of constant generators */

  pAdrParts->Part = RegPC;

  /* constant generators allowed at all? */

  if (!ForceLong)
  {
    /* special treatment for -1 since it depends on the operand size: */

    if ((Value == 0xffffffff)
     || ((OpSize == eOpSizeB) && (Value == 0xff))
     || ((OpSize == eOpSizeW) && (Value == 0xffff))
     || ((OpSize == eOpSizeA) && (Value == 0xfffff)))
    {
      pAdrParts->Cnt = 0;
      pAdrParts->Part = RegCG2;
      pAdrParts->Mode = eModeIRegAutoInc;
    }
    else switch (Value)
    {
      case 0:
        pAdrParts->Part = RegCG2;
        pAdrParts->Mode = eModeReg;
        break;
      case 1:
        pAdrParts->Part = RegCG2;
        pAdrParts->Mode = eModeRegDisp;
        break;
      case 2:
        pAdrParts->Part = RegCG2;
        pAdrParts->Mode = eModeIReg;
        break;
      case 4:
        pAdrParts->Part = RegCG1;
        pAdrParts->Mode = eModeIReg;
        break;
      case 8:
        pAdrParts->Part = RegCG1;
        pAdrParts->Mode = eModeIRegAutoInc;
        break;
      default:
        break;
    }
  }

  /* constant generators not used, in one or the other way -> use
     @PC++ to dispose constant */

  if (pAdrParts->Part == RegPC)
  {
    pAdrParts->Cnt = 1;
    pAdrParts->Mode = eModeIRegAutoInc;
  }
}

static Boolean DecodeAdr(const tStrComp *pArg, tExtMode ExtMode, Byte Mask, Boolean MayImm, tAdrParts *pAdrParts)
{
  LongWord AdrWord, CurrPC;
  Word Reg;
  Boolean OK;
  char *p;
  IntType ThisAdrIntType = (ExtMode == eExtModeYes) ? AdrIntType : UInt16;
  IntType ThisDispIntType = (ExtMode == eExtModeYes) ? DispIntType : Int16;
  int ArgLen;

  ResetAdr(pAdrParts);

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    if (!MayImm) WrError(ErrNum_InvAddrMode);
    else
    {
      int ForceLong = (pArg->str.p_str[1] == '>') ? 1 : 0;

      AdrWord = EvalStrIntExpressionOffs(pArg, 1 + ForceLong, OpSizeIntTypes[OpSize], &OK);
      if (OK)
      {
        FillAdrPartsImm(pAdrParts, AdrWord, ForceLong);
      }
    }
    return ChkAdr(Mask, pAdrParts);
  }

  /* absolut */

  if (*pArg->str.p_str == '&')
  {
    pAdrParts->Val = EvalStrIntExpressionOffs(pArg, 1, ThisAdrIntType, &OK);
    if (OK)
    {
      pAdrParts->WasAbs = True;
      pAdrParts->Mode = eModeRegDisp;
      pAdrParts->Part = RegCG1; /* == 0 with As/Ad=1 */
      pAdrParts->Cnt = 1;
    }
    return ChkAdr(Mask, pAdrParts);
  }

  /* Register */

  switch (DecodeReg(pArg, &Reg, False))
  {
    case eIsReg:
      if (Reg == RegCG2) WrStrErrorPos(ErrNum_InvReg, pArg);
      else
      {
        pAdrParts->Mode = eModeReg;
        pAdrParts->Part = Reg;
      }
      return ChkAdr(Mask, pAdrParts);
    case eIsNoReg:
      break;
    case eRegAbort:
      return False;
  }

  /* Displacement */

  ArgLen = strlen(pArg->str.p_str);
  if ((*pArg->str.p_str) && (pArg->str.p_str[ArgLen - 1] == ')'))
  {
    tStrComp Arg = *pArg;

    StrCompShorten(&Arg, 1);
    p = RQuotPos(Arg.str.p_str, '(');
    if (p)
    {
      tStrComp RegComp, OffsComp;
      char Save;

      Save = StrCompSplitRef(&OffsComp, &RegComp, &Arg, p);
      if (DecodeReg(&RegComp, &Reg, True) == eIsReg)
      {
        pAdrParts->Val = EvalStrIntExpression(&OffsComp, ThisDispIntType, &OK);
        if (OK)
        {
          if ((Reg == 2) || (Reg == 3)) WrStrErrorPos(ErrNum_InvReg, &RegComp);
          else if ((pAdrParts->Val == 0) && ((Mask & 4) != 0))
          {
            pAdrParts->Part = Reg;
            pAdrParts->Mode = eModeIReg;
          }
          else
          {
            pAdrParts->Part = Reg;
            pAdrParts->Cnt = 1;
            pAdrParts->Mode = eModeRegDisp;
          }
        }
      }
      *p = Save;
    }
    pArg->str.p_str[ArgLen - 1] = ')';

    if (pAdrParts->Mode != eModeNone)
      return ChkAdr(Mask, pAdrParts);
  }

  /* indirekt mit/ohne Autoinkrement */

  if ((*pArg->str.p_str == '@') || (*pArg->str.p_str == '*'))
  {
    Boolean AutoInc = False;
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    ArgLen = strlen(Arg.str.p_str);
    if (Arg.str.p_str[ArgLen - 1] == '+')
    {
      AutoInc = True;
      StrCompShorten(&Arg, 1);
    }
    if (DecodeReg(&Arg, &Reg, True) != eIsReg);
    else if ((Reg == 2) || (Reg == 3)) WrStrErrorPos(ErrNum_InvReg, &Arg);
    else if (!AutoInc && ((Mask & MModeIReg) == 0))
    {
      pAdrParts->Part = Reg;
      pAdrParts->Val = 0;
      pAdrParts->Cnt = 1;
      pAdrParts->Mode = eModeRegDisp;
    }
    else
    {
      pAdrParts->Part = Reg;
      pAdrParts->Mode = AutoInc ? eModeIRegAutoInc : eModeIReg;
    }
    return ChkAdr(Mask, pAdrParts);
  }

  /* bleibt PC-relativ aka 'symbolic mode': */

  if (!PCDist)
  {
    fprintf(stderr, "internal error: PCDist not set for '%s'\n", OpPart.str.p_str);
    exit(10);
  }
  CurrPC = EProgCounter() + PCDist;

  /* extended instruction (on 430X): use the full 20 bit displacement: */

  if (ExtMode == eExtModeYes)
  {
    AdrWord = (EvalStrIntExpression(pArg, UInt20, &OK) - CurrPC) & 0xfffff;
  }

  /* non-extended instruction on 430X: if the current PC is within the
     first 64K, bits 16..19 will be cleared after addition, i.e. the
     target address must also be within the first 64K: */

  else if (MomCPU >= CPUMSP430X)
  {
    if (CurrPC <= 0xffff)
    {
      AdrWord = (EvalStrIntExpression(pArg, UInt16, &OK) - CurrPC) & 0xffff;
    }
    else
    {
      AdrWord = (EvalStrIntExpression(pArg, UInt20, &OK) - CurrPC) & 0xfffff;
      if ((AdrWord > 0x7fff) && (AdrWord < 0xf8000))
      {
        WrError(ErrNum_OverRange);
        OK = False;
      }
    }
  }

  /* non-extended instruction on 430: all within 64K with wraparound */

  else
  {
    AdrWord = (EvalStrIntExpression(pArg, UInt16, &OK) - CurrPC) & 0xffff;
  }

  if (OK)
  {
    pAdrParts->Part = RegPC;
    pAdrParts->Mode = eModeRegDisp;
    pAdrParts->Cnt = 1;
    pAdrParts->Val = AdrWord;
  }

  return ChkAdr(Mask, pAdrParts);
}

static Word GetBW(void)
{
  return (OpSize == eOpSizeB) || (OpSize == eOpSizeA) ? 0x0040 : 0x0000;
}

static Word GetAL(void)
{
  return (OpSize == eOpSizeW) || (OpSize == eOpSizeB) ? 0x0040 : 0x0000;
}

static Word GetMult(const tStrComp *pArg, Boolean *pOK)
{
  Word Result = 0x0000;

  switch (DecodeReg(pArg, &Result, False))
  {
    case eIsReg:
      *pOK = True;
      return Result | 0x0080;
    case eIsNoReg:
      break;
    case eRegAbort:
      *pOK = False;
      return 0;
  }

  if (*pArg->str.p_str == '#')
  {
    tSymbolFlags Flags;

    Result = EvalStrIntExpressionOffsWithFlags(pArg, 1, UInt5, pOK, &Flags);
    if (*pOK)
    {
      if (mFirstPassUnknown(Flags))
        Result = 1;
      if (!ChkRange(Result, 1, 16))
        *pOK = False;
      else
        Result--;
    }
  }
  else
    *pOK = False;
  return Result;
}

/*-------------------------------------------------------------------------*/

static void PutByte(Word Value)
{
  if (CodeLen & 1)
    WAsmCode[CodeLen >> 1] = (Value << 8) | BAsmCode[CodeLen - 1];
  else
    BAsmCode[CodeLen] = Value;
  CodeLen++;
}

static void AppendAdrVals(const tAdrParts *pParts)
{
  Word i;

  for (i = 0; i < pParts->Cnt; i++)
  {
    WAsmCode[CodeLen >> 1] = pParts->Val;
    CodeLen += 2;
  }
}

static void ConstructTwoOp(Word Code, const tAdrParts *pSrcParts, const tAdrParts *pDestParts)
{
  WAsmCode[CodeLen >> 1] = Code | (pSrcParts->Part << 8) | (pDestParts->Mode << 7)
                         | GetBW() | (pSrcParts->Mode << 4) | pDestParts->Part;
  CodeLen += 2;
  AppendAdrVals(pSrcParts);
  AppendAdrVals(pDestParts);
}

static void ConstructTwoOpX(Word Code, const tAdrParts *pSrcParts, const tAdrParts *pDestParts)
{
  Word Prefix = 0x1800 | GetAL();

  if ((eModeReg != pSrcParts->Mode) || (eModeReg != pDestParts->Mode))
  {
    if (pSrcParts->Cnt)
      Prefix |= ((pSrcParts->Val >> 16) & 15) << 7;
    if (pDestParts->Cnt)
      Prefix |= ((pDestParts->Val >> 16) & 15);
  }

  /* take over multiply prefix for register<->register ops only */

  else
  {
    Prefix |= MultPrefix;
    MultPrefix = 0;
  }
  WAsmCode[CodeLen >> 1] = Prefix; CodeLen += 2;
  ConstructTwoOp(Code, pSrcParts, pDestParts);
}

static void DecodeFixed(Word Code)
{
  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (OpSize != eOpSizeDefault) WrError(ErrNum_InvOpSize);
  else
  {
    if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
    WAsmCode[0] = Code; CodeLen = 2;
  }
}

static void DecodeTwoOp(Word Code)
{
  tAdrParts SrcParts, DestParts;

  if (!ChkArgCnt(2, 2));
  else if (OpSize > eOpSizeW) WrError(ErrNum_InvOpSize);
  else
  {
    PCDist = 2;
    if (DecodeAdr(&ArgStr[1], eExtModeNo, 15, True, &SrcParts))
    {
      PCDist += SrcParts.Cnt << 1;
      if (DecodeAdr(&ArgStr[2], eExtModeNo, 3, False, &DestParts))
      {
        if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
        ConstructTwoOp(Code, &SrcParts, &DestParts);
      }
    }
  }
}

static void DecodeTwoOpX(Word Code)
{
  tAdrParts SrcParts, DestParts;

  Code &= ~1;

  if (!ChkArgCnt(2, 2))
    return;

  PCDist = 4;
  if (DecodeAdr(&ArgStr[1], eExtModeYes, MModeAs, True, &SrcParts))
  {
    PCDist += SrcParts.Cnt << 1;
    if (DecodeAdr(&ArgStr[2], eExtModeYes, MModeAd, False, &DestParts))
    {
      if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
      ConstructTwoOpX(Code, &SrcParts, &DestParts);
    }
  }
}

static void DecodeEmulOneToTwo(Word Code)
{
  Byte SrcSpec;
  tAdrParts SrcParts, DestParts;

  /* separate src spec & opcode */

  SrcSpec = Lo(Code);
  Code &= 0xff00;

  if (!ChkArgCnt(1, 1))
    return;

  if (OpSize > eOpSizeW)
  {
    WrError(ErrNum_InvOpSize);
    return;
  }

  /* Decode operand:
      - Ad modes always allowed
      - for Src == Dest, also allow @Rn+: */

  PCDist = 2;
  if (!DecodeAdr(&ArgStr[1], eExtModeNo, MModeAd | ((SrcSpec == 0xaa) ? MModeIRegAutoInc : 0), False, &DestParts))
    return;

  /* filter immediate out separately (we get it as d(PC): */

  if (DestParts.WasImm)
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }

  /* deduce src operand: 0xaa = special value for Src == Dest: */

  if (SrcSpec == 0xaa)
  {
    /* default assumption: */

    SrcParts = DestParts;

    /* @Rn+: is transformed to @Rn+,-opsize(Rn): */

    if (SrcParts.Mode == eModeIRegAutoInc)
    {
      static const Byte MemLen[3] = { 1, 2, 4 };

      DestParts.Mode = eModeRegDisp;
      DestParts.Val = (0 - MemLen[OpSize]) & 0xffff;
      DestParts.Cnt = 1;
    }

    /* for PC-relative addressing, fix up destination displacement and
       complain on displacement overflow: */

    else if ((DestParts.Mode == eModeRegDisp) && (DestParts.Part == RegPC))
    {
      LongWord NewDist = DestParts.Val - 2;

      if ((NewDist & 0x8000) != (DestParts.Val & 0x8000))
      {
        WrError(ErrNum_DistTooBig);
        return;
      }
      DestParts.Val = NewDist;
    }

    /* transform 0(Rn) as Dest back to @Rn as Src: */

    else if ((SrcParts.Mode == eModeRegDisp) && (DestParts.Val == 0))
    {
      SrcParts.Mode = eModeIReg;
      SrcParts.Cnt = 0;
    }
  }

  /* Src == other (constant) value: 0xff means -1: */

  else
    FillAdrPartsImm(&SrcParts, SrcSpec == 0xff ? 0xffffffff : SrcSpec, False);

  /* assemble like 2-op instruction: */

  ConstructTwoOp(Code, &SrcParts, &DestParts);
}

static void DecodeBR(Word Code)
{
  tAdrParts DstParts, SrcParts;

  PCDist = 2;
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeAdr(&ArgStr[1], eExtModeNo, MModeAs, True, &SrcParts))
  {
    if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
    ResetAdr(&DstParts);
    DstParts.Mode = eModeReg;
    DstParts.Part = RegPC;
    ConstructTwoOp(Code, &SrcParts, &DstParts);
  }
}

static void DecodeEmulOneToTwoX(Word Code)
{
  Byte SrcSpec;
  tAdrParts SrcParts, DestParts;

  /* separate src spec & opcode */

  SrcSpec = Lo(Code);
  Code &= 0xff00;

  if (!ChkArgCnt(1, 1))
    return;

  /* Decode operand:
      - Ad modes always allowed
      - for Src == Dest, also allow @Rn+: */

  PCDist = 4;
  if (!DecodeAdr(&ArgStr[1], eExtModeYes, MModeAd | ((SrcSpec == 0xaa) ? MModeIRegAutoInc : 0), False, &DestParts))
    return;

  /* filter immediate out separately (we get it as d(PC): */

  if (DestParts.WasImm)
  {
    WrError(ErrNum_InvAddrMode);
    return;
  }

  /* deduce src operand: 0xaa = special value for Src == Dest: */

  if (SrcSpec == 0xaa)
  {
    /* default assumption: */

    SrcParts = DestParts;

    /* @Rn+: is transformed to @Rn+,-opsize(Rn): */

    if (SrcParts.Mode == eModeIRegAutoInc)
    {
      static const Byte MemLen[3] = { 1, 2, 4 };

      DestParts.Mode = eModeRegDisp;
      DestParts.Val = (0 - MemLen[OpSize]) & 0xfffff;
      DestParts.Cnt = 1;
    }

    /* for PC-relative addressing, fix up destination displacement and
       complain on displacement overflow: */

    else if ((DestParts.Mode == eModeRegDisp) && (DestParts.Part == RegPC))
    {
      LongWord NewDist = DestParts.Val - 2;

      if ((NewDist & 0x8000) != (DestParts.Val & 0x8000))
      {
        WrError(ErrNum_DistTooBig);
        return;
      }
      DestParts.Val = NewDist;
    }

    /* transform 0(Rn) as Dest back to @Rn as Src: */

    else if ((SrcParts.Mode == eModeRegDisp) && (DestParts.Val == 0))
    {
      SrcParts.Mode = eModeIReg;
      SrcParts.Cnt = 0;
    }
  }

  /* Src == other (constant) value: 0xff means -1: */

  else
    FillAdrPartsImm(&SrcParts, SrcSpec == 0xff ? 0xffffffff : SrcSpec, False);

  /* assemble like 2-op instruction: */

  ConstructTwoOpX(Code, &SrcParts, &DestParts);
}

static void DecodePOP(Word Code)
{
  tAdrParts DstParts, SrcParts;

  PCDist = 2;
  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], eExtModeNo, MModeAd, True, &DstParts))
  {
    if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
    ResetAdr(&SrcParts);
    SrcParts.Mode = eModeIRegAutoInc;
    SrcParts.Part = RegSP;
    ConstructTwoOp(Code, &SrcParts, &DstParts);
  }
}

static void DecodePOPX(Word Code)
{
  tAdrParts DstParts, SrcParts;

  PCDist = 4;
  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], eExtModeYes, MModeAd, True, &DstParts))
  {
    if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
    ResetAdr(&SrcParts);
    SrcParts.Mode = eModeIRegAutoInc;
    SrcParts.Part = RegSP;
    ConstructTwoOpX(Code, &SrcParts, &DstParts);
  }
}

static void DecodeOneOp(Word Index)
{
  const OneOpOrder *pOrder = OneOpOrders + Index;

  if (!ChkArgCnt(1, 1));
  else if (OpSize > eOpSizeW) WrError(ErrNum_InvOpSize);
  else if ((OpSize == eOpSizeB) && (!pOrder->MayByte)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrParts AdrParts;

    PCDist = 2;
    if (DecodeAdr(&ArgStr[1], eExtModeNo, 15, True, &AdrParts))
    {
      if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
      WAsmCode[0] = pOrder->Code | GetBW() | (AdrParts.Mode << 4) | AdrParts.Part; CodeLen += 2;
      AppendAdrVals(&AdrParts);
    }
  }
}

static void DecodeOneOpX(Word Index)
{
  const OneOpOrder *pOrder = OneOpOrders + Index;

  if (!ChkArgCnt(1, 1));
  else if ((OpSize == eOpSizeB) && (!pOrder->MayByte)) WrError(ErrNum_InvOpSize);
  else
  {
    tAdrParts AdrParts;

    PCDist = 4;
    if (DecodeAdr(&ArgStr[1], eExtModeYes, 15, True, &AdrParts))
    {
      /* B/W for 20 bit size is 0 instead of 1 for SXT/SWPB */

      Word ActBW = pOrder->MayByte ? GetBW() : 0;

      if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
      WAsmCode[CodeLen >> 1] = 0x1800 | GetAL();

      /* put bits 16:19 of operand into bits 0:3 or 7:10 of extension word? */

      if (AdrParts.Cnt)
        WAsmCode[CodeLen >> 1] |= (((AdrParts.Val >> 16) & 15) << 7);

      /* repeat only supported for register op */

      if (AdrParts.Mode == eModeReg)
      {
        WAsmCode[CodeLen >> 1] |= MultPrefix;
        MultPrefix = 0;
      }
      CodeLen += 2;
      WAsmCode[CodeLen >> 1] = pOrder->Code | ActBW | (AdrParts.Mode << 4) | AdrParts.Part; CodeLen += 2;
      AppendAdrVals(&AdrParts);
    }
  }
}

static void DecodeMOVA(Word Code)
{
  tAdrParts AdrParts;

  UNUSED(Code);

  OpSize = eOpSizeA;
  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    PCDist = 2;
    DecodeAdr(&ArgStr[2], eExtModeYes, 15, False, &AdrParts);
    if (AdrParts.WasAbs)
    {
      if (DecodeReg(&ArgStr[1], &WAsmCode[0], True) == eIsReg)
      {
        if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
        WAsmCode[0] = 0x0060 | (WAsmCode[0] << 8) | ((AdrParts.Val >> 16) & 0x0f);
        WAsmCode[1] = AdrParts.Val & 0xffff;
        CodeLen = 4;
      }
    }
    else switch (AdrParts.Mode)
    {
      case eModeReg:
        WAsmCode[0] = AdrParts.Part;
        DecodeAdr(&ArgStr[1], eExtModeYes, 15, True, &AdrParts);
        if (AdrParts.WasImm)
        {
          if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
          WAsmCode[0] |= ((AdrParts.Val >> 8) & 0x0f00) | 0x0080;
          WAsmCode[1] = AdrParts.Val & 0xffff;
          CodeLen = 4;
        }
        else if (AdrParts.WasAbs)
        {
          if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
          WAsmCode[0] |= ((AdrParts.Val >> 8) & 0x0f00) | 0x0020;
          WAsmCode[1] = AdrParts.Val & 0xffff;
          CodeLen = 4;
        }
        else switch (AdrParts.Mode)
        {
          case eModeReg:
           if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
            WAsmCode[0] |= (AdrParts.Part << 8) | 0x00c0;
            CodeLen = 2;
            break;
          case eModeIReg:
            if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
            WAsmCode[0] |= (AdrParts.Part << 8) | 0x0000;
            CodeLen = 2;
            break;
          case eModeIRegAutoInc:
            if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
            WAsmCode[0] |= (AdrParts.Part << 8) | 0x0010;
            CodeLen = 2;
            break;
          case eModeRegDisp:
            if (ChkRange(AdrParts.Val, 0, 0xffff))
            {
              if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
              WAsmCode[0] |= (AdrParts.Part << 8) | 0x0030;
              WAsmCode[1] = AdrParts.Val & 0xffff;
              CodeLen = 4;
            }
            break;
        }
        break;

      /* 'MOVA ...,@Rn' is not defined, treat like 'MOVA ...,0(Rn)' */

      case eModeIReg:
        AdrParts.Mode = eModeRegDisp;
        AdrParts.Val = 0;
        /* fall-thru */
      case eModeRegDisp:
        if (ChkRange(AdrParts.Val, 0, 0xffff)
         && (DecodeReg(&ArgStr[1], &WAsmCode[0], True) == eIsReg))
        {
          if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
          WAsmCode[0] = 0x0070 | (WAsmCode[0] << 8) | AdrParts.Part;
          WAsmCode[1] = AdrParts.Val & 0xffff;
          CodeLen = 4;
        }
        break;
    }
  }
}

static void DecodeBRA(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    const char PCArg[] = "PC";

    AppendArg(strlen(PCArg));
    strcpy(ArgStr[ArgCnt].str.p_str, PCArg);
    DecodeMOVA(Code);
  }
}

static void DecodeCLRA(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (DecodeReg(&ArgStr[1], &WAsmCode[0], True) == eIsReg))
  {
    WAsmCode[0] |= Code;
    CodeLen = 2;
  }
}

static void DecodeTSTA(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (DecodeReg(&ArgStr[1], &WAsmCode[0], True) == eIsReg))
  {
    WAsmCode[0] |= Code;
    WAsmCode[1] = 0x0000;
    CodeLen = 4;
  }
}

static void DecodeDECDA_INCDA(Word Code)
{
  if (ChkArgCnt(1, 1)
   && (DecodeReg(&ArgStr[1], &WAsmCode[0], True) == eIsReg))
  {
    WAsmCode[0] |= Code;
    WAsmCode[1] = 2;
    CodeLen = 4;
  }
}

static void DecodeADDA_SUBA_CMPA(Word Code)
{
  OpSize = eOpSizeA;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeReg(&ArgStr[2], &WAsmCode[0], False) == eIsReg)
  {
    tAdrParts AdrParts;

    DecodeAdr(&ArgStr[1], eExtModeYes, 15, True, &AdrParts);
    if (AdrParts.WasImm)
    {
      if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
      WAsmCode[0] |= Code | ((AdrParts.Val >> 8) & 0xf00);
      WAsmCode[1] = AdrParts.Val & 0xffff;
      CodeLen = 4;
    }
    else if (eModeReg == AdrParts.Mode)
    {
      if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
      WAsmCode[0] |= Code | 0x0040 | (AdrParts.Part << 8);
      CodeLen = 2;
    }
    else
      WrError(ErrNum_InvOpSize);
  }
}

static void DecodeRxM(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (OpSize == eOpSizeB) WrError(ErrNum_InvOpSize);
  else if (DecodeReg(&ArgStr[2], &WAsmCode[0], True) != eIsReg);
  else if (ArgStr[1].str.p_str[0] != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Word Mult;
    tSymbolFlags Flags;
    Boolean OK;

    Mult = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt3, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags))
        Mult = 1;
      if (ChkRange(Mult, 1, 4))
      {
        if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
        WAsmCode[0] |= Code | ((Mult - 1) << 10) | (GetAL() >> 2);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeCALLA(Word Code)
{
  tAdrParts AdrParts;

  UNUSED(Code);

  OpSize = eOpSizeA;
  PCDist = 2;
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeAdr(&ArgStr[1], eExtModeYes, 15, True, &AdrParts))
  {
    if (AdrParts.WasImm)
    {
      WAsmCode[0] = 0x13b0 | ((AdrParts.Val >> 16) & 15);
      WAsmCode[1] = AdrParts.Val & 0xffff;
      CodeLen = 4;
    }
    else if (AdrParts.WasAbs)
    {
      WAsmCode[0] = 0x1380 | ((AdrParts.Val >> 16) & 15);
      WAsmCode[1] = AdrParts.Val & 0xffff;
      CodeLen = 4;
    }
    else if ((AdrParts.Mode == eModeRegDisp) && (AdrParts.Part == RegPC))
    {
      WAsmCode[0] = 0x1390 | ((AdrParts.Val >> 16) & 15);
      WAsmCode[1] = AdrParts.Val & 0xffff;
      CodeLen = 4;
    }
    else if ((AdrParts.Mode == eModeRegDisp) && (((AdrParts.Val & 0xfffff) > 0x7fff) && ((AdrParts.Val & 0xfffff) < 0xf8000))) WrError(ErrNum_OverRange);
    else
    {
      WAsmCode[CodeLen >> 1] = 0x1340 | (AdrParts.Mode << 4) | (AdrParts.Part); CodeLen += 2;
      AppendAdrVals(&AdrParts);
    }
  }
}

static void DecodePUSHM_POPM(Word Code)
{
  if (!ChkArgCnt(2, 2));
  else if (OpSize == 0) WrError(ErrNum_InvOpSize);
  else if (DecodeReg(&ArgStr[2], &WAsmCode[0], True) != eIsReg);
  else if (ArgStr[1].str.p_str[0] != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Boolean OK;
    Word Cnt;
    tSymbolFlags Flags;

    Cnt = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt5, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Cnt = 1;
    if (OK && ChkRange(Cnt, 1, 16))
    {
      Cnt--;
      if (Code & 0x0200)
        WAsmCode[0] = (WAsmCode[0] - Cnt) & 15;
      WAsmCode[0] |= Code | (Cnt << 4) | (GetAL() << 2);
      CodeLen = 2;
    }
  }
}

static void DecodeJmp(Word Code)
{
  Integer AdrInt;
  tSymbolFlags Flags;
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (OpSize != eOpSizeDefault) WrError(ErrNum_InvOpSize);
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (Odd(AdrInt)) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((AdrInt < -1024) || (AdrInt > 1022))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        if (Odd(EProgCounter())) WrError(ErrNum_AddrNotAligned);
        WAsmCode[0] = Code | ((AdrInt >> 1) & 0x3ff);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeBYTE(Word Index)
{
  TempResult t;

  UNUSED(Index);

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
          if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xff;
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
        {
          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else
          {
            unsigned l = t.Contents.str.len;

            if (SetMaxCodeLen(l + CodeLen))
            {
              WrStrErrorPos(ErrNum_CodeOverflow, pArg);
              OK = False;
            }
            else
            {
              char *pEnd = t.Contents.str.p_str + l, *p;

              for (p = t.Contents.str.p_str; p < pEnd; PutByte(*(p++)));
            }
          }
          break;
        }
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          /* fall-through */
        default:
          OK = False;
          break;
      }
    }
    if (!OK)
      CodeLen = 0;
  }
  as_tempres_free(&t);
}

static void DecodeWORD(Word Index)
{
  int z;
  Word HVal16;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(1, ArgCntMax))
  {
    z = 1; OK = True;
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
    if (!OK) CodeLen = 0;
  }
}

static void DecodeBSS(Word Index)
{
  Word HVal16;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    HVal16 = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    else if (OK)
    {
      if (!HVal16) WrError(ErrNum_NullResMem);
      DontPrint = True; CodeLen = HVal16;
      BookKeeping();
    }
  }
}

static void DecodeRPT(Word Code)
{
  char *pOpPart, *pArgPart1, *pAttrPart;
  Boolean OK;

  /* fundamentals */

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  if (*AttrPart.str.p_str != '\0')
  {
    WrError(ErrNum_UseLessAttr);
    return;
  }

  /* multiplier argument */

  pOpPart = FirstBlank(ArgStr[1].str.p_str);
  if (!pOpPart)
  {
    WrError(ErrNum_CannotSplitArg);
    return;
  }
  *pOpPart++ = '\0';
  MultPrefix = Code | GetMult(&ArgStr[1], &OK);
  if (!OK)
    return;

  /* new OpPart: */

  KillPrefBlanks(pOpPart);
  pArgPart1 = FirstBlank(pOpPart);
  if (!pArgPart1)
  {
    WrError(ErrNum_CannotSplitArg);
    return;
  }
  *pArgPart1++ = '\0';
  strcpy(OpPart.str.p_str, pOpPart);
  UpString(OpPart.str.p_str);
  KillPrefBlanks(pArgPart1);
  strmov(ArgStr[1].str.p_str, pArgPart1);

  /* split off new attribute part: */

  pAttrPart = strrchr(OpPart.str.p_str, '.');
  if (pAttrPart)
  {
    AttrPart.Pos.Len = strmemcpy(AttrPart.str.p_str, STRINGSIZE, pAttrPart + 1, strlen(pAttrPart + 1));
    *pAttrPart = '\0';
  }
  else
    StrCompReset(&AttrPart);

  /* prefix 0x0000 is rptc #1 and effectively a NOP prefix: */

  MakeCode();
  if (MultPrefix)
  {
    WrError(ErrNum_NotRepeatable);
    CodeLen = 0;
  }
}

/*-------------------------------------------------------------------------*/

#define AddFixed(NName, NCode) \
        AddInstTable(InstTable, NName, NCode, DecodeFixed)

static void AddTwoOp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeTwoOp);
  if (MomCPU >= CPUMSP430X)
  {
    char XName[20];

    as_snprintf(XName, sizeof(XName), "%sX", NName);
    AddInstTable(InstTable, XName, NCode, DecodeTwoOpX);
  }
}

static void AddEmulOneToTwo(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeEmulOneToTwo);
}

static void AddEmulOneToTwoX(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeEmulOneToTwoX);
}

static void AddOneOp(const char *NName, Boolean NMay, Boolean AllowX, Word NCode)
{
  order_array_rsv_end(OneOpOrders, OneOpOrder);
  OneOpOrders[InstrZ].MayByte = NMay;
  OneOpOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ, DecodeOneOp);
  if ((MomCPU >= CPUMSP430X) && AllowX)
  {
    char XName[20];

    as_snprintf(XName, sizeof(XName), "%sX", NName);
    AddInstTable(InstTable, XName, InstrZ, DecodeOneOpX);
  }
  InstrZ++;
}

#define AddJmp(NName, NCode) \
        AddInstTable(InstTable, NName, NCode, DecodeJmp)

static void InitFields(void)
{
  InstTable = CreateInstTable(207);
  SetDynamicInstTable(InstTable);

  AddFixed("RETI", 0x1300);
  AddFixed("CLRC", 0xc312);
  AddFixed("CLRN", 0xc222);
  AddFixed("CLRZ", 0xc322);
  AddFixed("DINT", 0xc232);
  AddFixed("EINT", 0xd232);
  AddFixed("NOP" , NOPCode);
  AddFixed("RET" , 0x4130);
  AddFixed("SETC", 0xd312);
  AddFixed("SETN", 0xd222);
  AddFixed("SETZ", 0xd322);

  AddTwoOp("MOV" , 0x4000); AddTwoOp("ADD" , 0x5000);
  AddTwoOp("ADDC", 0x6000); AddTwoOp("SUBC", 0x7000);
  AddTwoOp("SUB" , 0x8000); AddTwoOp("CMP" , 0x9000);
  AddTwoOp("DADD", 0xa000); AddTwoOp("BIT" , 0xb000);
  AddTwoOp("BIC" , 0xc000); AddTwoOp("BIS" , 0xd000);
  AddTwoOp("XOR" , 0xe000); AddTwoOp("AND" , 0xf000);

  AddEmulOneToTwo("ADC" , 0x6000); /* ADDC #0, dst */
  AddInstTable(InstTable, "BR", 0x4000, DecodeBR); /* MOV dst, PC */
  AddEmulOneToTwo("CLR" , 0x4000); /* MOV #0, dst */
  AddEmulOneToTwo("DADC", 0xa000); /* DADD #0, dst */
  AddEmulOneToTwo("DEC" , 0x8001); /* SUB #1, dst */
  AddEmulOneToTwo("DECD", 0x8002); /* SUB #2, dst */
  AddEmulOneToTwo("INC" , 0x5001); /* ADD #1, dst */
  AddEmulOneToTwo("INCD", 0x5002); /* ADD #2, dst */
  AddEmulOneToTwo("INV" , 0xe0ff); /* XOR #-1, dst */
  AddInstTable(InstTable, "POP", 0x4000, DecodePOP); /* MOV @SP+,dst */
  AddEmulOneToTwo("RLA" , 0x50aa); /* ADD dst, dst */
  AddEmulOneToTwo("RLC" , 0x60aa); /* ADDC dst, dst */
  AddEmulOneToTwo("SBC" , 0x7000); /* SUBC #0, dst */
  AddEmulOneToTwo("TST" , 0x9000); /* CMP #0, dst */

  InstrZ = 0;
  AddOneOp("RRC" , True , True , 0x1000); AddOneOp("RRA" , True , True , 0x1100);
  AddOneOp("PUSH", True , True , 0x1200); AddOneOp("SWPB", False, True , 0x1080);
  AddOneOp("CALL", False, False, 0x1280); AddOneOp("SXT" , False, True , 0x1180);

  if (MomCPU >= CPUMSP430X)
  {
    /* what about  RRUX? */

    AddInstTable(InstTable, "MOVA", 0x0000, DecodeMOVA);
    AddInstTable(InstTable, "ADDA", 0x00a0, DecodeADDA_SUBA_CMPA);
    AddInstTable(InstTable, "CMPA", 0x0090, DecodeADDA_SUBA_CMPA);
    AddInstTable(InstTable, "SUBA", 0x00b0, DecodeADDA_SUBA_CMPA);

    AddInstTable(InstTable, "RRCM", 0x0040, DecodeRxM);
    AddInstTable(InstTable, "RRAM", 0x0140, DecodeRxM);
    AddInstTable(InstTable, "RLAM", 0x0240, DecodeRxM);
    AddInstTable(InstTable, "RRUM", 0x0340, DecodeRxM);

    AddInstTable(InstTable, "CALLA", 0x0000, DecodeCALLA);

    AddInstTable(InstTable, "PUSHM", 0x1400, DecodePUSHM_POPM);
    AddInstTable(InstTable, "POPM",  0x1600, DecodePUSHM_POPM);

    AddEmulOneToTwoX("ADCX", 0x6000); /* ADDCX #0, dst */
    AddInstTable(InstTable, "BRA", 0x4000, DecodeBRA); /* MOVA dst, PC */
    AddFixed("RETA", 0x0110); /* MOVA @SP+,PC */
    AddInstTable(InstTable, "CLRA", 0x4300, DecodeCLRA); /* MOV #0,Rdst */
    AddEmulOneToTwoX("CLRX", 0x4000); /* MOVX #0, dest */
    AddEmulOneToTwoX("DADCX", 0xa000); /* DADDX #0, dst */
    AddEmulOneToTwoX("DECX" , 0x8001); /* SUBX #1, dst */
    AddInstTable(InstTable, "DECDA", 0x00b0, DecodeDECDA_INCDA); /* SUBA #2,Rdst */
    AddEmulOneToTwoX("DECDX", 0x8002); /* SUBX #2, dst */
    AddEmulOneToTwoX("INCX" , 0x5001); /* SUBX #1, dst */
    AddInstTable(InstTable, "INCDA", 0x00a0, DecodeDECDA_INCDA); /* SUBA #2,Rdst */
    AddEmulOneToTwoX("INCDX", 0x5002); /* SUBX #2, dst */
    AddEmulOneToTwoX("INVX" , 0xe0ff); /* XORX #-1, dst */
    AddEmulOneToTwoX("RLAX" , 0x50aa); /* ADDX dst, dst */
    AddEmulOneToTwoX("RLCX" , 0x60aa); /* ADDCX dst, dst */
    AddEmulOneToTwoX("SBCX" , 0x7000); /* SUBCX #0, dst */
    AddInstTable(InstTable, "TSTA" , 0x0090, DecodeTSTA); /* CMPA #0,Rdst */
    AddEmulOneToTwoX("TSTX" , 0x9000); /* CMPX #0, dst */
    AddInstTable(InstTable, "POPX", 0x4000, DecodePOPX); /* MOVX @SP+,dst */

    AddInstTable(InstTable, "RPTC", 0x0000, DecodeRPT);
    AddInstTable(InstTable, "RPTZ", 0x0100, DecodeRPT);
  }

  AddJmp("JNE" , 0x2000); AddJmp("JNZ" , 0x2000);
  AddJmp("JE"  , 0x2400); AddJmp("JZ"  , 0x2400);
  AddJmp("JNC" , 0x2800); AddJmp("JC"  , 0x2c00);
  AddJmp("JN"  , 0x3000); AddJmp("JGE" , 0x3400);
  AddJmp("JL"  , 0x3800); AddJmp("JMP" , 0x3C00);
  AddJmp("JEQ" , 0x2400); AddJmp("JLO" , 0x2800);
  AddJmp("JHS" , 0x2c00);

  AddInstTable(InstTable, "WORD", 0, DecodeWORD);

  AddInstTable(InstTable, "REG", 0, CodeREG);
}

static void DeinitFields(void)
{
  order_array_free(OneOpOrders);

  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_MSP(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for MSP
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_MSP(char *pArg, TempResult *pResult)
{
  Word RegNum;

  if (DecodeRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_MSP;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean DecodeAttrPart_MSP(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }
  switch (as_toupper(*AttrPart.str.p_str))
  {
    case '\0':
      break;
    case 'B':
      AttrPartOpSize[0] = eSymbolSize8Bit;
      break;
    case 'W':
      AttrPartOpSize[0] = eSymbolSize16Bit;
      break;
    case 'A':
      if (MomCPU >= CPUMSP430X)
      {
        AttrPartOpSize[0] = eSymbolSize24Bit; /* TODO: should be 20 bits */
        break;
      }
      /* else fall-through */
    default:
      WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
      return False;
  }
  return True;
}

static void MakeCode_MSP(void)
{
  CodeLen = 0; DontPrint = False; PCDist = 0;

  /* to be ignored: */

  if (Memo("")) return;

  /* process attribute */

  switch (AttrPartOpSize[0])
  {
    case eSymbolSize24Bit:
      OpSize = eOpSizeA;
      break;
    case eSymbolSize16Bit:
      OpSize = eOpSizeW;
      break;
    case eSymbolSize8Bit:
      OpSize = eOpSizeB;
      break;
    default:
      OpSize = eOpSizeDefault;
      break;
  }

  /* insns not requiring word alignment */

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

  /* all the rest from table */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_MSP(void)
{
  return Memo("REG");
}

static void SwitchTo_MSP(void)
{
  TurnWords = False; SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = 0x4a; NOPCode = 0x4303; /* = MOV #0,#0 */
  DivideChars = ","; HasAttrs = True; AttrChars = ".";

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  AdrIntType = (MomCPU == CPUMSP430X) ? UInt20 : UInt16;
  DispIntType = (MomCPU == CPUMSP430X) ? Int20 : Int16;
  SegLimits[SegCode] = IntTypeDefs[AdrIntType].Max;

  AddONOFF(DoPaddingName, &DoPadding, DoPaddingName, False);

  DecodeAttrPart = DecodeAttrPart_MSP;
  MakeCode = MakeCode_MSP;
  IsDef = IsDef_MSP;
  InternSymbol = InternSymbol_MSP;
  DissectReg = DissectReg_MSP;
  SwitchFrom = DeinitFields; InitFields();

  MultPrefix = 0x0000;
}

void codemsp_init(void)
{
  CPUMSP430 = AddCPU("MSP430", SwitchTo_MSP);
  CPUMSP430X = AddCPU("MSP430X", SwitchTo_MSP);
}
