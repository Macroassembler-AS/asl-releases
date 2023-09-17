/* codexa.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegenerator Philips XA                                               */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codexa.h"

/*-------------------------------------------------------------------------*/
/* Definitionen */

#define ModNone (-1)
#define ModReg 0
#define MModReg (1 << ModReg)
#define ModMem 1
#define MModMem (1 << ModMem)
#define ModImm 2
#define MModImm (1 << ModImm)
#define ModAbs 3
#define MModAbs (1 << ModAbs)

#define REG_SP 7

#define RETICode 0xd690

#define eSymbolSize5Bit ((tSymbolSize)-4)
#define eSymbolSizeSigned4Bit ((tSymbolSize)-3)
#define eSymbolSize4Bit ((tSymbolSize)-2)

typedef struct
{
  Byte SizeMask;
  Byte Code;
} RegOrder;

typedef struct
{
  const char *Name;
  Byte SizeMask;
  Byte Code;
  Byte Inversion;
} InvOrder;

static CPUVar CPUXAG1,CPUXAG2,CPUXAG3;

static InvOrder *JBitOrders;
static RegOrder *RegOrders;
static InvOrder *RelOrders;

static LongInt Reg_DS;

static ShortInt AdrMode;
static Byte AdrPart,MemPart;
static Byte AdrVals[4];
static tSymbolSize OpSize;
static Boolean DoBranchExt; /* automatically extend branches */

#define ASSUMEXACount 1
static ASSUMERec ASSUMEXAs[ASSUMEXACount] =
{
  {"DS", &Reg_DS, 0, 0xff, 0x100, NULL}
};

/*-------------------------------------------------------------------------*/
/* Hilfsroutinen */

static void SetOpSize(tSymbolSize NSize)
{
  if (OpSize == eSymbolSizeUnknown) OpSize = NSize;
  else if (OpSize != NSize)
  {
    AdrMode = ModNone; AdrCnt = 0; WrError(ErrNum_ConfOpSizes);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, tSymbolSize *pSize, Byte *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg source argument
 * \param  pSize register size if yes
 * \param  pResult register number if yes
 * \return Reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeRegCore(const char *pArg, tSymbolSize *pSize, Byte *pResult)
{
  int len;

  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REG_SP | REGSYM_FLAG_ALIAS;
    *pSize = eSymbolSize16Bit;
    return eIsReg;
  }

  len = strlen(pArg);
  if ((len >= 2) && (as_toupper(*pArg) == 'R') && (pArg[1] >= '0') && (pArg[1] <= '7'))
  {
    *pResult = pArg[1] - '0';
    if (len == 2)
    {
      if (OpSize == eSymbolSize32Bit)
      {
        *pSize = eSymbolSize32Bit;
        if (*pResult & 1)
        {
          WrError(ErrNum_InvRegPair);
          (*pResult)--;
          return eRegAbort;
        }
        else
          return eIsReg;
      }
      else
      {
        *pSize = eSymbolSize16Bit;
        return eIsReg;
      }
    }
    else if ((len == 3) && (as_toupper(pArg[2]) == 'L'))
    {
      *pResult <<= 1;
      *pSize = eSymbolSize8Bit;
      return eIsReg;
    }
    else if ((len == 3) && (as_toupper(pArg[2]) == 'H'))
    {
      *pResult = (*pResult << 1) + 1;
      *pSize = eSymbolSize8Bit;
      return eIsReg;
    }
    else
      return eIsNoReg;
  }
  return eIsNoReg;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_XA(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - XA variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_XA(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u%c", (unsigned)(Value >> 1), "LH"[Value & 1]);
      break;
    case eSymbolSize16Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    case eSymbolSize32Bit:
      as_snprintf(pDest, DestSize, "R%u.D", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}


/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, tSymbolSize *pSize, Byte *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg source argument
 * \param  pSize register size if yes
 * \param  pResult register number if yes
 * \param  MustBeReg expecting register?
 * \return Reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, tSymbolSize *pSize, Byte *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  RegEvalResult = DecodeRegCore(pArg->str.p_str, pSize, pResult);
  if (RegEvalResult != eIsNoReg)
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return RegEvalResult;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
  if (RegEvalResult == eIsReg)
  {
    *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
    *pSize = EvalResult.DataSize;
  }
  return RegEvalResult;
}

static Boolean ChkAdr(Word Mask, const tStrComp *pComp)
{
  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pComp);
    AdrMode = ModNone;
    AdrCnt = 0;
    return False;
  }
  return True;
}

static Boolean DecodeAdrIndirect(tStrComp *pArg, Word Mask)
{
  unsigned ArgLen;
  Byte Reg;
  tSymbolSize NSize;

  ArgLen = strlen(pArg->str.p_str);
  if (pArg->str.p_str[ArgLen - 1] == '+')
  {
    StrCompShorten(pArg, 1); ArgLen--;
    if (!DecodeReg(pArg, &NSize, &AdrPart, True));
    else if (NSize != eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    else
    {
      AdrMode = ModMem; MemPart = 3;
    }
  }
  else
  {
    char *pSplit;
    Boolean FirstFlag = False, NegFlag = False, NextNegFlag, ErrFlag = False;
    tStrComp ThisComp = *pArg, RemComp;
    LongInt DispAcc = 0;

    AdrPart = 0xff;
    do
    {
      pSplit = QuotMultPos(ThisComp.str.p_str, "+-");
      NextNegFlag = (pSplit && (*pSplit == '-'));
      if (pSplit)
        StrCompSplitRef(&ThisComp, &RemComp, &ThisComp, pSplit);
      KillPrefBlanksStrComp(&ThisComp);
      KillPostBlanksStrComp(&ThisComp);

      switch (DecodeReg(&ThisComp, &NSize, &Reg, False))
      {
        case eIsReg:
          if ((NSize != eSymbolSize16Bit) || (AdrPart != 0xff) || NegFlag)
          {
            WrStrErrorPos(ErrNum_InvAddrMode, &ThisComp); ErrFlag = True;
          }
          else
            AdrPart = Reg;
          break;
        case eRegAbort:
          return False;
        default:
        {
          LongInt DispPart;
          tSymbolFlags Flags;

          DispPart = EvalStrIntExpressionWithFlags(&ThisComp, Int32, &ErrFlag, &Flags);
          ErrFlag = !ErrFlag;
          if (!ErrFlag)
          {
            FirstFlag = FirstFlag || mFirstPassUnknown(Flags);
            DispAcc += NegFlag ? -DispPart : DispPart;
          }
        }
      }

      NegFlag = NextNegFlag;
      if (pSplit)
        ThisComp = RemComp;
    }
    while (pSplit && !ErrFlag);

    if (FirstFlag) DispAcc &= 0x7fff;
    if (AdrPart == 0xff) WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    else if (DispAcc == 0)
    {
      AdrMode = ModMem; MemPart = 2;
    }
    else if ((DispAcc >= -128) && (DispAcc < 127))
    {
      AdrMode = ModMem; MemPart = 4;
      AdrVals[0] = DispAcc & 0xff; AdrCnt = 1;
    }
    else if (ChkRange(DispAcc, -0x8000l, 0x7fffl))
    {
      AdrMode = ModMem; MemPart = 5;
      AdrVals[0] = (DispAcc >> 8) & 0xff;
      AdrVals[1] = DispAcc & 0xff;
      AdrCnt = 2;
    }
  }
  return ChkAdr(Mask, pArg);
}

static Boolean DecodeAdr(tStrComp *pArg, Word Mask)
{
  tSymbolSize NSize;
  LongInt AdrLong;
  tEvalResult EvalResult;
  Word AdrInt;
  int ArgLen;

  AdrMode = ModNone; AdrCnt = 0;
  KillPrefBlanksStrComp(pArg);
  KillPostBlanksStrComp(pArg);

  switch (DecodeReg(pArg, &NSize, &AdrPart, False))
  {
    case eIsReg:
      if (Mask & MModReg)
      {
        AdrMode = ModReg;
        SetOpSize(NSize);
      }
      else
      {
        AdrMode = ModMem;
        MemPart = 1;
        SetOpSize(NSize);
      }
      return ChkAdr(Mask, pArg);
    case eRegAbort:
      return False;
    default:
      break;
  }

  if (*pArg->str.p_str == '#')
  {
    tStrComp ImmComp;
    Boolean OK;

    StrCompRefRight(&ImmComp, pArg, 1);
    switch ((int)OpSize)
    {
      case eSymbolSize5Bit:
        AdrVals[0] = EvalStrIntExpression(&ImmComp, UInt5, &OK);
        if (OK)
        {
          AdrCnt = 1; AdrMode = ModImm;
        }
        break;
      case eSymbolSizeSigned4Bit:
        AdrVals[0] = EvalStrIntExpression(&ImmComp, SInt4, &OK);
        if (OK)
        {
          AdrCnt = 1; AdrMode = ModImm;
        }
        break;
      case eSymbolSize4Bit:
        AdrVals[0] = EvalStrIntExpression(&ImmComp, UInt4, &OK);
        if (OK)
        {
          AdrCnt = 1; AdrMode = ModImm;
        }
        break;
      case eSymbolSizeUnknown:
        WrError(ErrNum_UndefOpSizes);
        break;
      case eSymbolSize8Bit:
        AdrVals[0] = EvalStrIntExpression(&ImmComp, Int8, &OK);
        if (OK)
        {
          AdrCnt = 1; AdrMode = ModImm;
        }
        break;
      case eSymbolSize16Bit:
        AdrInt = EvalStrIntExpression(&ImmComp, Int16, &OK);
        if (OK)
        {
          AdrVals[0] = Hi(AdrInt);
          AdrVals[1] = Lo(AdrInt);
          AdrCnt = 2;
          AdrMode = ModImm;
        }
        break;
      case eSymbolSize32Bit:
        AdrLong = EvalStrIntExpression(&ImmComp, Int32, &OK);
        if (OK)
        {
          AdrVals[0] = (AdrLong >> 24) & 0xff;
          AdrVals[1] = (AdrLong >> 16) & 0xff;
          AdrVals[2] = (AdrLong >> 8) & 0xff;
          AdrVals[3] = AdrLong & 0xff;
          AdrCnt = 4;
          AdrMode = ModImm;
        }
        break;
      default:
        break;
    }
    return ChkAdr(Mask, pArg);
  }

  ArgLen = strlen(pArg->str.p_str);
  if ((*pArg->str.p_str == '[') && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    tStrComp IndirComp;

    StrCompShorten(pArg, 1);
    StrCompRefRight(&IndirComp, pArg, 1);
    KillPrefBlanksStrComp(&IndirComp);
    KillPostBlanksStrComp(&IndirComp);

    return DecodeAdrIndirect(&IndirComp, Mask);
  }

  AdrLong = EvalStrIntExpressionWithResult(pArg, UInt24, &EvalResult);
  if (EvalResult.OK)
  {
    if (mFirstPassUnknown(EvalResult.Flags))
    {
      if (!(Mask & MModAbs)) AdrLong &= 0x3ff;
    }
    if ((AdrLong & 0xffff) > 0x7ff) WrError(ErrNum_AdrOverflow);
    else if ((AdrLong & 0xffff) <= 0x3ff)
    {
      if ((AdrLong >> 16) != Reg_DS) WrError(ErrNum_InAccPage);
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
      AdrMode = ModMem; MemPart = 6;
      AdrPart = Hi(AdrLong);
      AdrVals[0] = Lo(AdrLong);
      AdrCnt = 1;
    }
    else if (AdrLong > 0x7ff) WrError(ErrNum_AdrOverflow);
    else
    {
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      AdrMode = ModMem; MemPart = 6;
      AdrPart = Hi(AdrLong);
      AdrVals[0] = Lo(AdrLong);
      AdrCnt = 1;
    }
  }

  return ChkAdr(Mask, pArg);
}

static Boolean DecodeBitAddr(tStrComp *pArg, LongInt *Erg)
{
  char *p;
  Byte BPos, Reg;
  ShortInt Res;
  tSymbolSize Size;
  LongInt AdrLong;
  tEvalResult EvalResult;

  p = RQuotPos(pArg->str.p_str, '.'); Res = 0;
  if (!p)
  {
    AdrLong = EvalStrIntExpressionWithResult(pArg, UInt24, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags)) AdrLong &= 0x3ff;
    *Erg = AdrLong; Res = 1;
  }
  else
  {
    int l = p - pArg->str.p_str;

    BPos = EvalStrIntExpressionOffsWithResult(pArg, l + 1, UInt4, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags)) BPos &= 7;

    *p = '\0'; pArg->Pos.Len -= l + 1;
    if (EvalResult.OK)
    {
      switch (DecodeReg(pArg, &Size, &Reg, False))
      {
        case eRegAbort:
          return False;
        case eIsReg:
          if ((Size == eSymbolSize8Bit) && (BPos > 7)) WrError(ErrNum_OverRange);
          else
          {
            if (Size == eSymbolSize8Bit) *Erg = (Reg << 3) + BPos;
            else *Erg = (Reg << 4) + BPos;
            Res = 1;
          }
          break;
        default:
          if (BPos > 7) WrError(ErrNum_OverRange);
          else
          {
            AdrLong = EvalStrIntExpressionWithResult(pArg, UInt24, &EvalResult);
            if (EvalResult.AddrSpaceMask & (1 << SegIO))
            {
              ChkSpace(SegIO, EvalResult.AddrSpaceMask);
              if (mFirstPassUnknown(EvalResult.Flags)) AdrLong = (AdrLong & 0x3f) | 0x400;
              if (ChkRange(AdrLong, 0x400, 0x43f))
              {
                *Erg = 0x200 + ((AdrLong & 0x3f) << 3) + BPos;
                Res = 1;
              }
              else
                Res = -1;
            }
            else
            {
              ChkSpace(SegData, EvalResult.AddrSpaceMask);
              if (mFirstPassUnknown(EvalResult.Flags)) AdrLong = (AdrLong & 0x00ff003f) | 0x20;
              if (ChkRange(AdrLong & 0xff, 0x20, 0x3f))
              {
                *Erg = 0x100 + ((AdrLong & 0x1f) << 3) + BPos + (AdrLong & 0xff0000);
                Res = 1;
              }
              else
                Res = -1;
            }
          }
        break;
      }
    }
    *p = '.';
  }
  if (Res == 0) WrError(ErrNum_InvAddrMode);
  return (Res == 1);
}

static void ChkBitPage(LongInt Adr)
{
  if ((Adr >> 16) != Reg_DS) WrError(ErrNum_InAccPage);
}

static LongWord MkEven24(LongWord Inp)
{
  return Inp & 0xfffffeul;
}

static LongWord GetBranchDest(const tStrComp *pComp, tEvalResult *pEvalResult)
{
  LongWord Result;

  Result = EvalStrIntExpressionWithResult(pComp, UInt24, pEvalResult);
  if (pEvalResult->OK)
  {
    if (mFirstPassUnknown(pEvalResult->Flags)) Result = MkEven24(Result);
    ChkSpace(SegCode, pEvalResult->AddrSpaceMask);
    if (Result & 1)
    {
      WrStrErrorPos(ErrNum_NotAligned, pComp);
      pEvalResult->OK = False;
    }
  }
  return Result;
}

static Boolean ChkShortEvenDist(LongInt *pDist, tSymbolFlags Flags)
{
  if (!mSymbolQuestionable(Flags) && ((*pDist > 254) || (*pDist < -256)))
    return False;
  else
  {
    *pDist >>= 1;
    return True;
  }
}

static Boolean ChkLongEvenDist(LongInt *pDist, const tStrComp *pComp, tSymbolFlags Flags)
{
  if (!mSymbolQuestionable(Flags) && ((*pDist > 65534) || (*pDist < -65536)))
  {
    WrStrErrorPos(ErrNum_JmpDistTooBig, pComp);
    return False;
  }
  else
  {
    *pDist >>= 1;
    return True;
  }
}

/*-------------------------------------------------------------------------*/
/* Befehlsdekoder */

static void DecodePORT(Word Index)
{
  UNUSED(Index);

  CodeEquate(SegIO, 0x400, 0x7ff);
}

static void DecodeBIT(Word Index)
{
  LongInt BAdr;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeBitAddr(&ArgStr[1], &BAdr))
  {
    EnterIntSymbol(&LabPart, BAdr, SegNone, False);
    switch ((BAdr & 0x3ff) >> 8)
    {
      case 0:
        as_snprintf(ListLine, STRINGSIZE, "=R%d.%d",
                    (unsigned)((BAdr >> 4) & 15),
                    (unsigned) (BAdr & 15));
        break;
      case 1:
        as_snprintf(ListLine, STRINGSIZE, "=%x:%x.%d",
                    (unsigned)((BAdr >> 16) & 255),
                    (unsigned)((BAdr & 0x1f8) >> 3), (unsigned)(BAdr & 7));
        break;
      default:
        as_snprintf(ListLine, STRINGSIZE, "=S:%x.%d",
                    (unsigned)(((BAdr >> 3) & 0x3f) + 0x400),
                    (unsigned)(BAdr & 7));
        break;
    }
  }
}

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    if (Hi(Code) != 0) BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code);
    if ((Code == RETICode) && (!SupAllowed)) WrError(ErrNum_PrivOrder);
  }
}

static void DecodeStack(Word Code)
{
  Byte HReg;
  Boolean OK;
  Word Mask;
  int i;

  if (ChkArgCnt(1, ArgCntMax))
  {
    HReg = 0xff; OK = True; Mask = 0;
    for (i = 1; i <= ArgCnt; i++)
      if (OK)
      {
        DecodeAdr(&ArgStr[i], MModMem);
        if (AdrMode == ModNone) OK = False;
        else switch (MemPart)
        {
          case 1:
            if (HReg == 0)
            {
              WrError(ErrNum_InvAddrMode); OK = False;
            }
            else
            {
              HReg = 1; Mask |= (1 << AdrPart);
            }
            break;
          case 6:
            if (HReg != 0xff)
            {
              WrError(ErrNum_InvAddrMode); OK = False;
            }
            else
              HReg = 0;
            break;
          default:
            WrError(ErrNum_InvAddrMode); OK = False;
        }
      }
    if (OK)
    {
      if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
      else if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
      else if (HReg == 0)
      {
        BAsmCode[CodeLen++] = 0x87 + (OpSize << 3);
        BAsmCode[CodeLen++] = Hi(Code) + AdrPart;
        BAsmCode[CodeLen++] = AdrVals[0];
      }
      else if (Code < 0x2000)  /* POP(U): obere Register zuerst */
      {
        if (Hi(Mask) != 0)
        {
          BAsmCode[CodeLen++] = Lo(Code) + (OpSize << 3) + 0x40;
          BAsmCode[CodeLen++]=Hi(Mask);
        }
        if (Lo(Mask) != 0)
        {
          BAsmCode[CodeLen++] = Lo(Code) + (OpSize << 3);
          BAsmCode[CodeLen++] = Lo(Mask);
        }
        if ((OpSize == eSymbolSize16Bit) && (Code == 0x1027) && (Mask & 0x80)) WrError(ErrNum_Unpredictable);
      }
      else              /* PUSH(U): untere Register zuerst */
      {
        if (Lo(Mask) != 0)
        {
          BAsmCode[CodeLen++] = Lo(Code) + (OpSize << 3);
          BAsmCode[CodeLen++] = Lo(Mask);
        }
        if (Hi(Mask) != 0)
        {
          BAsmCode[CodeLen++] = Lo(Code) + (OpSize << 3) + 0x40;
          BAsmCode[CodeLen++] = Hi(Mask);
        }
      }
    }
  }
}

static void DecodeALU(Word Index)
{
  Byte HReg, HCnt, HVals[3], HMem;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModMem);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize >= eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
        else if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
        else
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModMem | MModImm);
          switch (AdrMode)
          {
            case ModMem:
              BAsmCode[CodeLen++] = (Index << 4) + (OpSize << 3) + MemPart;
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              if ((MemPart == 3) && ((HReg >> (1 - OpSize)) == AdrPart)) WrError(ErrNum_Unpredictable);
              break;
            case ModImm:
              BAsmCode[CodeLen++] = 0x91 + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + Index;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              break;
          }
        }
        break;
      case ModMem:
        HReg = AdrPart; HMem = MemPart; HCnt = AdrCnt;
        memcpy(HVals, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
            else if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else
            {
              BAsmCode[CodeLen++] = (Index << 4) + (OpSize << 3) + HMem;
              BAsmCode[CodeLen++] = (AdrPart << 4) + 8 + HReg;
              memcpy(BAsmCode + CodeLen, HVals, HCnt);
              CodeLen += HCnt;
              if ((HMem == 3) && ((AdrPart >> (1 - OpSize)) == HReg)) WrError(ErrNum_Unpredictable);
            }
            break;
          case ModImm:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
            else if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else
            {
              BAsmCode[CodeLen++] = 0x90 + HMem + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + Index;
              memcpy(BAsmCode + CodeLen, HVals, HCnt);
              memcpy(BAsmCode + CodeLen + HCnt, AdrVals, AdrCnt);
              CodeLen += AdrCnt + HCnt;
            }
            break;
        }
        break;
    }
  }
}

static void DecodeRegO(Word Index)
{
  RegOrder *Op = RegOrders + Index;

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        if ((Op->SizeMask & (1 << OpSize)) == 0) WrError(ErrNum_InvOpSize);
        else
        {
          BAsmCode[CodeLen++] = 0x90 + (OpSize << 3);
          BAsmCode[CodeLen++] = (AdrPart << 4) + Op->Code;
        }
        break;
    }
  }
}

static void DecodeShift(Word Index)
{
  Byte HReg, HMem;

  if (!ChkArgCnt(2, 2));
  else if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
  else
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        HReg = AdrPart; HMem = OpSize;
        if (*ArgStr[2].str.p_str == '#')
          OpSize = (HMem == 2) ? eSymbolSize5Bit : eSymbolSize4Bit;
        else
          OpSize = eSymbolSize8Bit;
        DecodeAdr(&ArgStr[2], MModReg | ((Index == 3) ? 0 : MModImm));
        switch (AdrMode)
        {
          case ModReg:
            BAsmCode[CodeLen++] = 0xc0 + ((HMem & 1) << 3) + Index;
            if (HMem == 2) BAsmCode[CodeLen - 1] += 12;
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            if (Index == 3)
            {
              if (HMem == 2)
              {
                if ((AdrPart >> 2) == (HReg >> 1)) WrError(ErrNum_Unpredictable);
              }
              else if ((AdrPart >> HMem) == HReg) WrError(ErrNum_Unpredictable);
            }
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0xd0 + ((HMem & 1) << 3) + Index;
            if (HMem == 2)
            {
              BAsmCode[CodeLen - 1] += 12;
              BAsmCode[CodeLen++] = ((HReg & 14) << 4) + AdrVals[0];
            }
            else
              BAsmCode[CodeLen++] = (HReg << 4) + AdrVals[0];
            break;
        }
        break;
    }
  }
}

static void DecodeRotate(Word Code)
{
  Byte HReg, HMem;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    switch (AdrMode)
    {
      case ModReg:
        if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
        else
        {
          HReg = AdrPart; HMem = OpSize; OpSize = eSymbolSize4Bit;
          DecodeAdr(&ArgStr[2], MModImm);
          switch (AdrMode)
          {
            case ModImm:
              BAsmCode[CodeLen++] = Code + (HMem << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrVals[0];
              break;
          }
        }
        break;
    }
  }
}

static void DecodeRel(Word Index)
{
  InvOrder *Op = RelOrders + Index;
  LongWord Dest;
  LongInt Dist;
  tEvalResult EvalResult;

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Dest = GetBranchDest(&ArgStr[1], &EvalResult);
    if (EvalResult.OK)
    {
      Dist = Dest - MkEven24(EProgCounter() + CodeLen + 2);
      if (ChkShortEvenDist(&Dist, EvalResult.Flags))
      {
        BAsmCode[CodeLen++] = Op->Code;
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
      else if (!DoBranchExt) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else if (Op->Inversion == 255)  /* BR */
      {
        Dist = Dest - MkEven24(EProgCounter() + CodeLen + 3);
        if (ChkLongEvenDist(&Dist, &ArgStr[1], EvalResult.Flags))
        {
          Dist >>= 1;
          BAsmCode[CodeLen++] = 0xd5;
          BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
          BAsmCode[CodeLen++] = Dist        & 0xff;
        }
      }
      else
      {
        Dist = Dest - MkEven24(EProgCounter() + CodeLen + 5);
        if (ChkLongEvenDist(&Dist, &ArgStr[1], EvalResult.Flags))
        {
          BAsmCode[CodeLen++] = RelOrders[Op->Inversion].Code;
          BAsmCode[CodeLen++] = 2;
          BAsmCode[CodeLen++] = 0xd5;
          BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
          BAsmCode[CodeLen++] = Dist        & 0xff;
          if (Odd(EProgCounter() + CodeLen)) BAsmCode[CodeLen++] = 0;
        }
      }
    }
  }
}

static void DecodeJBit(Word Index)
{
  LongInt BitAdr, Dist, odd;
  LongWord Dest;
  tEvalResult EvalResult;
  InvOrder *Op = JBitOrders + Index;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeBitAddr(&ArgStr[1], &BitAdr))
  {
    Dest = GetBranchDest(&ArgStr[2], &EvalResult);
    if (EvalResult.OK)
    {
      Dist = Dest - MkEven24(EProgCounter() + CodeLen + 4);
      if (ChkShortEvenDist(&Dist, EvalResult.Flags))
      {
        BAsmCode[CodeLen++] = 0x97;
        BAsmCode[CodeLen++] = Op->Code + Hi(BitAdr);
        BAsmCode[CodeLen++] = Lo(BitAdr);
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
      else if (!DoBranchExt) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
      else if (Op->Inversion == 255)
      {
        odd = EProgCounter() & 1;
        Dist = Dest - MkEven24(EProgCounter() + CodeLen + 9 + odd);
        if (ChkLongEvenDist(&Dist, &ArgStr[2], EvalResult.Flags))
        {
          BAsmCode[CodeLen++] = 0x97;
          BAsmCode[CodeLen++] = Op->Code + Hi(BitAdr);
          BAsmCode[CodeLen++] = Lo(BitAdr);
          BAsmCode[CodeLen++] = 1 + odd;
          BAsmCode[CodeLen++] = 0xfe;
          BAsmCode[CodeLen++] = 2 + odd;
          if (odd) BAsmCode[CodeLen++] = 0;
          BAsmCode[CodeLen++] = 0xd5;
          BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
          BAsmCode[CodeLen++] = Dist        & 0xff;
          BAsmCode[CodeLen++] = 0;
        }
      }
      else
      {
        Dist = Dest - MkEven24(EProgCounter() + CodeLen + 7);
        if (ChkLongEvenDist(&Dist, &ArgStr[2], EvalResult.Flags))
        {
          BAsmCode[CodeLen++] = 0x97;
          BAsmCode[CodeLen++] = JBitOrders[Op->Inversion].Code + Hi(BitAdr);
          BAsmCode[CodeLen++] = Lo(BitAdr);
          BAsmCode[CodeLen++] = 2;
          BAsmCode[CodeLen++] = 0xd5;
          BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
          BAsmCode[CodeLen++] = Dist        & 0xff;
          if (Odd(EProgCounter() + CodeLen)) BAsmCode[CodeLen++] = 0;
        }
      }
    }
  }
}


static void DecodeMOV(Word Index)
{
  LongInt AdrLong;
  Byte HVals[3], HReg, HPart, HCnt;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "C"))
  {
    if (DecodeBitAddr(&ArgStr[2], &AdrLong))
    {
      if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
      else
      {
        ChkBitPage(AdrLong);
        BAsmCode[CodeLen++] = 0x08;
        BAsmCode[CodeLen++] = 0x20 + Hi(AdrLong);
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "C"))
  {
    if (DecodeBitAddr(&ArgStr[1], &AdrLong))
    {
      if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
      else
      {
        ChkBitPage(AdrLong);
        BAsmCode[CodeLen++] = 0x08;
        BAsmCode[CodeLen++] = 0x30 + Hi(AdrLong);
        BAsmCode[CodeLen++] = Lo(AdrLong);
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "USP"))
  {
    SetOpSize(eSymbolSize16Bit);
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrMode == ModReg)
    {
      BAsmCode[CodeLen++] = 0x98;
      BAsmCode[CodeLen++] = (AdrPart << 4) + 0x0f;
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "USP"))
  {
    SetOpSize(eSymbolSize16Bit);
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      BAsmCode[CodeLen++] = 0x90;
      BAsmCode[CodeLen++] = (AdrPart << 4)+0x0f;
    }
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg | MModMem);
    switch (AdrMode)
    {
      case ModReg:
        if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
        else
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModMem | MModImm);
          switch (AdrMode)
          {
            case ModMem:
              BAsmCode[CodeLen++] = 0x80 + (OpSize << 3) + MemPart;
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              if ((MemPart == 3) && ((HReg >> (1 - OpSize)) == AdrPart)) WrError(ErrNum_Unpredictable);
              break;
            case ModImm:
              BAsmCode[CodeLen++] = 0x91 + (OpSize << 3);
              BAsmCode[CodeLen++] = 0x08 + (HReg << 4);
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              break;
          }
        }
        break;
      case ModMem:
        memcpy(HVals, AdrVals, AdrCnt); HCnt = AdrCnt; HPart = MemPart; HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg | MModMem | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0x80 + (OpSize << 3) + HPart;
              BAsmCode[CodeLen++] = (AdrPart << 4) + 0x08 + HReg;
              memcpy(BAsmCode + CodeLen, HVals, HCnt);
              CodeLen += HCnt;
              if ((HPart == 3) && ((AdrPart >> (1 - OpSize)) == HReg)) WrError(ErrNum_Unpredictable);
            }
            break;
          case ModMem:
            if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else if ((OpSize != eSymbolSize8Bit) & (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else if ((HPart == 6) && (MemPart == 6))
            {
              BAsmCode[CodeLen++] = 0x97 + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4)+AdrPart;
              BAsmCode[CodeLen++] = HVals[0];
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if ((HPart == 6) && (MemPart == 2))
            {
              BAsmCode[CodeLen++] = 0xa0 + (OpSize << 3);
              BAsmCode[CodeLen++] = 0x80 + (AdrPart << 4) + HReg;
              BAsmCode[CodeLen++] = HVals[0];
            }
            else if ((HPart == 2) && (MemPart == 6))
            {
              BAsmCode[CodeLen++] = 0xa0 + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              BAsmCode[CodeLen++] = AdrVals[0];
            }
            else if ((HPart == 3) && (MemPart == 3))
            {
              BAsmCode[CodeLen++] = 0x90 + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              if (HReg == AdrPart) WrError(ErrNum_Unpredictable);
            }
            else
              WrError(ErrNum_InvAddrMode);
            break;
          case ModImm:
            if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0x90 + (OpSize << 3) + HPart;
              BAsmCode[CodeLen++] = 0x08 + (HReg << 4);
              memcpy(BAsmCode + CodeLen, HVals, HCnt);
              memcpy(BAsmCode + CodeLen + HCnt, AdrVals, AdrCnt);
              CodeLen += HCnt + AdrCnt;
            }
            break;
        }
        break;
    }
  }
}

static void DecodeMOVC(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else
  {
    if (!*AttrPart.str.p_str && !as_strcasecmp(ArgStr[1].str.p_str, "A")) OpSize = eSymbolSize8Bit;
    if (!as_strcasecmp(ArgStr[2].str.p_str, "[A+DPTR]"))
    {
      if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
      else if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        BAsmCode[CodeLen++] = 0x90;
        BAsmCode[CodeLen++] = 0x4e;
      }
    }
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "[A+PC]"))
    {
      if (as_strcasecmp(ArgStr[1].str.p_str, "A")) WrError(ErrNum_InvAddrMode);
      else if (OpSize != eSymbolSize8Bit) WrError(ErrNum_InvOpSize);
      else
      {
        BAsmCode[CodeLen++] = 0x90;
        BAsmCode[CodeLen++] = 0x4c;
      }
    }
    else
    {
      DecodeAdr(&ArgStr[1], MModReg);
      if (AdrMode != ModNone)
      {
        if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
        else
        {
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModMem);
          if (AdrMode != ModNone)
          {
            if (MemPart != 3) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[CodeLen++] = 0x80 + (OpSize << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              if ((MemPart == 3) && ((HReg >> (1 - OpSize)) == AdrPart)) WrError(ErrNum_Unpredictable);
            }
          }
        }
      }
    }
  }
}

static void DecodeMOVX(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else
  {
    DecodeAdr(&ArgStr[1], MModMem);
    if (AdrMode == ModMem)
    {
      switch (MemPart)
      {
        case 1:
          if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
          else
          {
            HReg = AdrPart; DecodeAdr(&ArgStr[2], MModMem);
            if (AdrMode == ModMem)
            {
              if (MemPart != 2) WrError(ErrNum_InvAddrMode);
              else
              {
                BAsmCode[CodeLen++] = 0xa7 + (OpSize << 3);
                BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              }
            }
          }
          break;
        case 2:
          HReg = AdrPart; DecodeAdr(&ArgStr[2], MModReg);
          if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
          else
          {
            BAsmCode[CodeLen++] = 0xa7 + (OpSize << 3);
            BAsmCode[CodeLen++] = 0x08 + (AdrPart << 4) + HReg;
          }
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
    }
  }
}

static void DecodeXCH(Word Index)
{
  Byte HReg, HPart, HVals[3];
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModMem);
    if (AdrMode == ModMem)
    {
      switch (MemPart)
      {
        case 1:
          HReg = AdrPart; DecodeAdr(&ArgStr[2], MModMem);
          if (AdrMode == ModMem)
          {
            if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize8Bit)) WrError(ErrNum_InvOpSize);
            else switch (MemPart)
            {
              case 1:
                BAsmCode[CodeLen++] = 0x60 + (OpSize << 3);
                BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
                if (HReg == AdrPart) WrError(ErrNum_Unpredictable);
                break;
              case 2:
                BAsmCode[CodeLen++] = 0x50 + (OpSize << 3);
                BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
                break;
              case 6:
                BAsmCode[CodeLen++] = 0xa0 + (OpSize << 3);
                BAsmCode[CodeLen++] = 0x08 + (HReg << 4) + AdrPart;
                BAsmCode[CodeLen++] = AdrVals[0];
                break;
              default:
                WrError(ErrNum_InvAddrMode);
            }
          }
          break;
        case 2:
          HReg = AdrPart;
          DecodeAdr(&ArgStr[2], MModReg);
          if (AdrMode == ModReg)
          {
            if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0x50 + (OpSize << 3);
              BAsmCode[CodeLen++] = (AdrPart << 4) + HReg;
            }
          }
          break;
        case 6:
          HPart = AdrPart; HVals[0] = AdrVals[0];
          DecodeAdr(&ArgStr[2], MModReg);
          if (AdrMode == ModReg)
          {
            if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0xa0 + (OpSize << 3);
              BAsmCode[CodeLen++] = 0x08 + (AdrPart << 4) + HPart;
              BAsmCode[CodeLen++] = HVals[0];
            }
          }
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
    }
  }
}

static void DecodeADDSMOVS(Word Index)
{
  Byte HReg;
  tSymbolSize HSize;

  if (ChkArgCnt(2, 2))
  {
    HSize = OpSize;
    OpSize = eSymbolSizeSigned4Bit;
    DecodeAdr(&ArgStr[2], MModImm);
    switch (AdrMode)
    {
      case ModImm:
        HReg = AdrVals[0]; OpSize = HSize;
        DecodeAdr(&ArgStr[1], MModMem);
        switch (AdrMode)
        {
          case ModMem:
            if (OpSize == eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
            else if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else
            {
               BAsmCode[CodeLen++] = 0xa0 + (Index << 4) + (OpSize << 3) + MemPart;
               BAsmCode[CodeLen++] = (AdrPart << 4) + (HReg & 0x0f);
               memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
               CodeLen += AdrCnt;
            }
            break;
        }
        break;
    }
  }
}

static void DecodeDIV(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      if ((OpSize != eSymbolSize16Bit) && (OpSize != eSymbolSize32Bit)) WrError(ErrNum_InvOpSize);
      else
      {
        HReg = AdrPart; OpSize--; DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            BAsmCode[CodeLen++] = 0xe7 + (OpSize << 3);
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0xe8 + OpSize;
            BAsmCode[CodeLen++] = (HReg << 4) + 0x0b - (OpSize << 1);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
        }
      }
    }
  }
}

static void DecodeDIVU(Word Index)
{
  Byte HReg;
  int z;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      if ((OpSize == eSymbolSize8Bit) && (AdrPart & 1)) WrError(ErrNum_InvReg);
      else
      {
        HReg = AdrPart; z = OpSize; if (OpSize != eSymbolSize8Bit) OpSize--;
        DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            BAsmCode[CodeLen++] = 0xe1 + (z << 2);
            if (z == 2) BAsmCode[CodeLen - 1] += 4;
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0xe8 + Ord(z == 2);
            BAsmCode[CodeLen++] = (HReg << 4) + 0x01 + (Ord(z == 1) << 1);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
        }
      }
    }
  }
}

static void DecodeMUL(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
      else if (AdrPart & 1) WrError(ErrNum_InvReg);
      else
      {
        HReg = AdrPart; DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            BAsmCode[CodeLen++] = 0xe6;
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0xe9;
            BAsmCode[CodeLen++] = (HReg << 4) + 0x08;
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
        }
      }
    }
  }
}

static void DecodeMULU(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      if (AdrPart & 1) WrError(ErrNum_InvReg);
      else
      {
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrMode)
        {
          case ModReg:
            BAsmCode[CodeLen++] = 0xe0 + (OpSize << 2);
            BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
            break;
          case ModImm:
            BAsmCode[CodeLen++] = 0xe8 + OpSize;
            BAsmCode[CodeLen++] = (HReg << 4);
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
        }
      }
    }
  }
}

static void DecodeLEA(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg);
    if (AdrMode == ModReg)
    {
      if (OpSize != eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
      else
      {
        HReg = AdrPart;
        DecodeAdrIndirect(&ArgStr[2], MModMem);
        if (AdrMode == ModMem)
          switch (MemPart)
          {
            case 4:
            case 5:
              BAsmCode[CodeLen++] = 0x20 + (MemPart << 3);
              BAsmCode[CodeLen++] = (HReg << 4) + AdrPart;
              memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
              CodeLen += AdrCnt;
              break;
            default:
              WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          }
      }
    }
  }
}

static void DecodeANLORL(Word Index)
{
  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (as_strcasecmp(ArgStr[1].str.p_str, "C")) WrError(ErrNum_InvAddrMode);
  else
  {
    Byte Invert = 0;
    LongInt AdrLong;
    Boolean Result;

    if (*ArgStr[2].str.p_str == '/')
    {
      tStrComp Comp;

      StrCompRefRight(&Comp, &ArgStr[2], 1);
      Result = DecodeBitAddr(&Comp, &AdrLong);
      Invert = 1;
    }
    else
      Result = DecodeBitAddr(&ArgStr[2], &AdrLong);
    if (Result)
    {
      ChkBitPage(AdrLong);
      BAsmCode[CodeLen++] = 0x08;
      BAsmCode[CodeLen++] = 0x40 | (Index << 5) | (Ord(Invert) << 4) | (Hi(AdrLong) & 3);
      BAsmCode[CodeLen++] = Lo(AdrLong);
    }
  }
}

static void DecodeCLRSETB(Word Index)
{
  LongInt AdrLong;

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (DecodeBitAddr(&ArgStr[1], &AdrLong))
  {
    ChkBitPage(AdrLong);
    BAsmCode[CodeLen++] = 0x08;
    BAsmCode[CodeLen++] = (Index << 4) + (Hi(AdrLong) & 3);
    BAsmCode[CodeLen++] = Lo(AdrLong);
  }
}

static void DecodeTRAP(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    OpSize = eSymbolSize4Bit;
    DecodeAdr(&ArgStr[1], MModImm);
    switch (AdrMode)
    {
      case ModImm:
        BAsmCode[CodeLen++] = 0xd6;
        BAsmCode[CodeLen++] = 0x30 + AdrVals[0];
        break;
    }
  }
}

static void DecodeCALL(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str == '[')
  {
    DecodeAdr(&ArgStr[1], MModMem);
    if (AdrMode != ModNone)
    {
      if (MemPart != 2) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[CodeLen++] = 0xc6;
        BAsmCode[CodeLen++] = AdrPart;
      }
    }
  }
  else
  {
    tEvalResult EvalResult;
    LongWord Dest = GetBranchDest(&ArgStr[1], &EvalResult);

    if (EvalResult.OK)
    {
      LongInt Dist = Dest - MkEven24(EProgCounter() + CodeLen + 3);

      if (ChkLongEvenDist(&Dist, &ArgStr[1], EvalResult.Flags))
      {
        BAsmCode[CodeLen++] = 0xc5;
        BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
    }
  }
}

static void DecodeJMP(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "[A+DPTR]"))
  {
    BAsmCode[CodeLen++] = 0xd6;
    BAsmCode[CodeLen++] = 0x46;
  }
  else if (!strncmp(ArgStr[1].str.p_str, "[[", 2))
  {
    tStrComp Comp;

    ArgStr[1].str.p_str[strlen(ArgStr[1].str.p_str) - 1] = '\0';
    ArgStr[1].Pos.Len--;
    StrCompRefRight(&Comp, &ArgStr[1], 1);
    DecodeAdr(&Comp, MModMem);
    if (AdrMode == ModMem)
     switch (MemPart)
     {
       case 3:
         BAsmCode[CodeLen++] = 0xd6;
         BAsmCode[CodeLen++] = 0x60 + AdrPart;
         break;
       default:
         WrError(ErrNum_InvAddrMode);
     }
  }
  else if (*ArgStr[1].str.p_str == '[')
  {
    DecodeAdr(&ArgStr[1], MModMem);
    if (AdrMode == ModMem)
      switch (MemPart)
      {
        case 2:
          BAsmCode[CodeLen++] = 0xd6;
          BAsmCode[CodeLen++] = 0x70 + AdrPart;
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
  }
  else
  {
    tEvalResult EvalResult;
    LongWord Dest = GetBranchDest(&ArgStr[1], &EvalResult);

    if (EvalResult.OK)
    {
      LongInt Dist = Dest - MkEven24(EProgCounter() + CodeLen + 3);

      if (ChkLongEvenDist(&Dist, &ArgStr[1], EvalResult.Flags))
      {
        BAsmCode[CodeLen++] = 0xd5;
        BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
        BAsmCode[CodeLen++] = Dist & 0xff;
      }
    }
  }
}

static void DecodeCJNE(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(3, 3))
  {
    tEvalResult EvalResult;
    LongWord Dest = GetBranchDest(&ArgStr[3], &EvalResult);

    if (EvalResult.OK)
    {
      EvalResult.OK = False; HReg = 0;
      DecodeAdr(&ArgStr[1], MModMem);
      if (AdrMode == ModMem)
      {
        switch (MemPart)
        {
          case 1:
            if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              HReg = AdrPart; DecodeAdr(&ArgStr[2], MModMem | MModImm);
              switch (AdrMode)
              {
                case ModMem:
                  if (MemPart != 6) WrError(ErrNum_InvAddrMode);
                  else
                  {
                    BAsmCode[CodeLen] = 0xe2 + (OpSize << 3);
                    BAsmCode[CodeLen + 1] = (HReg << 4) + AdrPart;
                    BAsmCode[CodeLen + 2] = AdrVals[0];
                    HReg=CodeLen + 3;
                    CodeLen += 4; EvalResult.OK = True;
                  }
                  break;
                case ModImm:
                  BAsmCode[CodeLen] = 0xe3 + (OpSize << 3);
                  BAsmCode[CodeLen + 1] = HReg << 4;
                  HReg=CodeLen + 2;
                  memcpy(BAsmCode + CodeLen + 3, AdrVals, AdrCnt);
                  CodeLen += 3 + AdrCnt; EvalResult.OK = True;
                  break;
              }
            }
            break;
          case 2:
            if ((OpSize != eSymbolSizeUnknown) && (OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              HReg = AdrPart; DecodeAdr(&ArgStr[2], MModImm);
              if (AdrMode == ModImm)
              {
                BAsmCode[CodeLen] = 0xe3 + (OpSize << 3);
                BAsmCode[CodeLen + 1] = (HReg << 4)+8;
                HReg = CodeLen + 2;
                memcpy(BAsmCode + CodeLen + 3, AdrVals, AdrCnt);
                CodeLen += 3 + AdrCnt; EvalResult.OK = True;
              }
            }
            break;
          default:
            WrError(ErrNum_InvAddrMode);
        }
      }
      if (EvalResult.OK)
      {
        LongInt Dist = Dest - MkEven24(EProgCounter() + CodeLen);

        EvalResult.OK = False;
        if (ChkShortEvenDist(&Dist, EvalResult.Flags))
        {
          BAsmCode[HReg] = Dist & 0xff;
          EvalResult.OK = True;
        }
        else if (!DoBranchExt) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[3]);
        else
        {
          LongWord odd = (EProgCounter() + CodeLen) & 1;

          Dist = Dest - MkEven24(EProgCounter() + CodeLen + 5 + odd);
          if (ChkLongEvenDist(&Dist, &ArgStr[3], EvalResult.Flags))
          {
            BAsmCode[HReg] = 1 + odd;
            BAsmCode[CodeLen++] = 0xfe;
            BAsmCode[CodeLen++] = 2 + odd;
            if (odd) BAsmCode[CodeLen++] = 0;
            BAsmCode[CodeLen++] = 0xd5;
            BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
            BAsmCode[CodeLen++] = Dist        & 0xff;
            BAsmCode[CodeLen++] = 0;
            EvalResult.OK = True;
          }
        }
      }
      if (!EvalResult.OK)
        CodeLen = 0;
    }
  }
}

static void DecodeDJNZ(Word Index)
{
  Byte HReg;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    tEvalResult EvalResult;
    LongInt Dest = GetBranchDest(&ArgStr[2], &EvalResult);

    if (EvalResult.OK)
    {
      HReg = 0;
      DecodeAdr(&ArgStr[1], MModMem);
      EvalResult.OK = False; DecodeAdr(&ArgStr[1], MModMem);
      if (AdrMode == ModMem)
        switch (MemPart)
        {
          case 1:
            if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen] = 0x87 + (OpSize << 3);
              BAsmCode[CodeLen + 1] = (AdrPart << 4) + 0x08;
              HReg=CodeLen + 2;
              CodeLen += 3; EvalResult.OK = True;
            }
            break;
          case 6:
            if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else if ((OpSize != eSymbolSize8Bit) && (OpSize != eSymbolSize16Bit)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen] = 0xe2 + (OpSize << 3);
              BAsmCode[CodeLen+1] = 0x08 + AdrPart;
              BAsmCode[CodeLen+2] = AdrVals[0];
              HReg=CodeLen + 3;
              CodeLen += 4; EvalResult.OK = True;
            }
            break;
          default:
            WrError(ErrNum_InvAddrMode);
        }
      if (EvalResult.OK)
      {
        LongInt Dist = Dest - MkEven24(EProgCounter() + CodeLen);

        EvalResult.OK = False;
        if (ChkShortEvenDist(&Dist, EvalResult.Flags))
        {
          BAsmCode[HReg] = Dist & 0xff;
          EvalResult.OK = True;
        }
        else if (!DoBranchExt) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
        else
        {
          LongWord odd = (EProgCounter() + CodeLen) & 1;

          Dist = Dest - MkEven24(EProgCounter() + CodeLen + 5 + odd);
          if (ChkLongEvenDist(&Dist, &ArgStr[2], EvalResult.Flags))
          {
            BAsmCode[HReg] = 1 + odd;
            BAsmCode[CodeLen++] = 0xfe;
            BAsmCode[CodeLen++] = 2 + odd;
            if (odd) BAsmCode[CodeLen++] = 0;
            BAsmCode[CodeLen++] = 0xd5;
            BAsmCode[CodeLen++] = (Dist >> 8) & 0xff;
            BAsmCode[CodeLen++] = Dist        & 0xff;
            BAsmCode[CodeLen++] = 0;
            EvalResult.OK = True;
          }
        }
      }
      if (!EvalResult.OK)
        CodeLen = 0;
    }
  }
}

static void DecodeFCALLJMP(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    tEvalResult EvalResult;
    LongInt AdrLong = GetBranchDest(&ArgStr[1], &EvalResult);
    if (EvalResult.OK)
    {
      BAsmCode[CodeLen++] = 0xc4 | (Index << 4);
      BAsmCode[CodeLen++] = (AdrLong >> 8) & 0xff;
      BAsmCode[CodeLen++] = AdrLong & 0xff;
      BAsmCode[CodeLen++] = (AdrLong >> 16) & 0xff;
    }
  }
}

static Boolean IsRealDef(void)
{
  switch (*OpPart.str.p_str)
  {
    case 'P':
      return Memo("PORT");
    case 'B':
      return Memo("BIT");
    case 'R':
      return Memo("REG");
    default:
      return FALSE;
  }
}

static void ForceAlign(void)
{
  if (EProgCounter() & 1)
  {
    BAsmCode[0] = NOPCode; CodeLen = 1;
  }
}

static Boolean DecodeAttrPart_XA(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  if (*AttrPart.str.p_str)
    switch (as_toupper(*AttrPart.str.p_str))
    {
      case 'B': AttrPartOpSize[0] = eSymbolSize8Bit; break;
      case 'W': AttrPartOpSize[0] = eSymbolSize16Bit; break;
      case 'D': AttrPartOpSize[0] = eSymbolSize32Bit; break;
      default : WrStrErrorPos(ErrNum_UndefAttr, &AttrPart); return False;
    }
  return True;
}

static void MakeCode_XA(void)
{
  CodeLen = 0; DontPrint = False; OpSize = eSymbolSizeUnknown;

   /* Operandengroesse */

  if (*AttrPart.str.p_str)
    SetOpSize(AttrPartOpSize[0]);

  /* Labels muessen auf geraden Adressen liegen */

  if ( (ActPC == SegCode) && (!IsRealDef()) &&
       ((*LabPart.str.p_str != '\0') ||((ArgCnt == 1) && (!strcmp(ArgStr[1].str.p_str, "$")))) )
  {
    ForceAlign();
    if (*LabPart.str.p_str != '\0')
      EnterIntSymbol(&LabPart, EProgCounter() + CodeLen, (as_addrspace_t)ActPC, False);
  }

  if (DecodeMoto16Pseudo(OpSize, False)) return;
  if (DecodeIntelPseudo(False)) return;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* via Tabelle suchen */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*-------------------------------------------------------------------------*/
/* Codetabellenverwaltung */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddJBit(const char *NName, Word NCode)
{
  order_array_rsv_end(JBitOrders, InvOrder);
  JBitOrders[InstrZ].Name = NName;
  JBitOrders[InstrZ].Inversion = 255;
  JBitOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeJBit);
}

static void AddStack(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeStack);
}

static void AddReg(const char *NName, Byte NMask, Byte NCode)
{
  order_array_rsv_end(RegOrders, RegOrder);
  RegOrders[InstrZ].Code = NCode;
  RegOrders[InstrZ].SizeMask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRegO);
}

static void AddRotate(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRotate);
}

static void AddRel(const char *NName, Word NCode)
{
  order_array_rsv_end(RelOrders, InvOrder);
  RelOrders[InstrZ].Name = NName;
  RelOrders[InstrZ].Inversion = 255;
  RelOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRel);
}

static void SetInv(const char *Name1, const char *Name2, InvOrder *Orders)
{
  InvOrder *Order1, *Order2;

  for (Order1 = Orders; strcmp(Order1->Name, Name1); Order1++);
  for (Order2 = Orders; strcmp(Order2->Name, Name2); Order2++);
  Order1->Inversion = Order2 - Orders;
  Order2->Inversion = Order1 - Orders;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  AddInstTable(InstTable, "MOV"  , 0, DecodeMOV);
  AddInstTable(InstTable, "MOVC" , 0, DecodeMOVC);
  AddInstTable(InstTable, "MOVX" , 0, DecodeMOVX);
  AddInstTable(InstTable, "XCH"  , 0, DecodeXCH);
  AddInstTable(InstTable, "ADDS" , 0, DecodeADDSMOVS);
  AddInstTable(InstTable, "MOVS" , 1, DecodeADDSMOVS);
  AddInstTable(InstTable, "DIV"  , 0, DecodeDIV);
  AddInstTable(InstTable, "DIVU" , 0, DecodeDIVU);
  AddInstTable(InstTable, "MUL"  , 0, DecodeMUL);
  AddInstTable(InstTable, "MULU" , 0, DecodeMULU);
  AddInstTable(InstTable, "LEA"  , 0, DecodeLEA);
  AddInstTable(InstTable, "ANL"  , 0, DecodeANLORL);
  AddInstTable(InstTable, "ORL"  , 1, DecodeANLORL);
  AddInstTable(InstTable, "CLR"  , 0, DecodeCLRSETB);
  AddInstTable(InstTable, "SETB" , 1, DecodeCLRSETB);
  AddInstTable(InstTable, "TRAP" , 0, DecodeTRAP);
  AddInstTable(InstTable, "CALL" , 0, DecodeCALL);
  AddInstTable(InstTable, "JMP"  , 0, DecodeJMP);
  AddInstTable(InstTable, "CJNE" , 0, DecodeCJNE);
  AddInstTable(InstTable, "DJNZ" , 0, DecodeDJNZ);
  AddInstTable(InstTable, "FCALL", 0, DecodeFCALLJMP);
  AddInstTable(InstTable, "FJMP" , 1, DecodeFCALLJMP);
  AddInstTable(InstTable, "PORT" , 0, DecodePORT);
  AddInstTable(InstTable, "BIT"  , 0, DecodeBIT);
  AddInstTable(InstTable, "REG"  , 0, CodeREG);

  AddFixed("NOP"  , 0x0000);
  AddFixed("RET"  , 0xd680);
  AddFixed("RETI" , RETICode);
  AddFixed("BKPT" , 0x00ff);
  AddFixed("RESET", 0xd610);

  InstrZ = 0;
  AddJBit("JB"  , 0x80);
  AddJBit("JBC" , 0xc0);
  AddJBit("JNB" , 0xa0);
  SetInv("JB", "JNB", JBitOrders);

  AddStack("POP"  , 0x1027);
  AddStack("POPU" , 0x0037);
  AddStack("PUSH" , 0x3007);
  AddStack("PUSHU", 0x2017);

  InstrZ = 0;
  AddInstTable(InstTable, "ADD" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "ADDC", InstrZ++, DecodeALU);
  AddInstTable(InstTable, "SUB" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "SUBB", InstrZ++, DecodeALU);
  AddInstTable(InstTable, "CMP" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "AND" , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "OR"  , InstrZ++, DecodeALU);
  AddInstTable(InstTable, "XOR" , InstrZ++, DecodeALU);

  InstrZ = 0;
  AddReg("NEG" , 3, 0x0b);
  AddReg("CPL" , 3, 0x0a);
  AddReg("SEXT", 3, 0x09);
  AddReg("DA"  , 1, 0x08);

  AddInstTable(InstTable, "LSR" , 0, DecodeShift);
  AddInstTable(InstTable, "ASL" , 1, DecodeShift);
  AddInstTable(InstTable, "ASR" , 2, DecodeShift);
  AddInstTable(InstTable, "NORM", 3, DecodeShift);

  AddRotate("RR" , 0xb0); AddRotate("RL" , 0xd3);
  AddRotate("RRC", 0xb7); AddRotate("RLC", 0xd7);

  InstrZ = 0;
  AddRel("BCC", 0xf0); AddRel("BCS", 0xf1); AddRel("BNE", 0xf2);
  AddRel("BEQ", 0xf3); AddRel("BNV", 0xf4); AddRel("BOV", 0xf5);
  AddRel("BPL", 0xf6); AddRel("BMI", 0xf7); AddRel("BG" , 0xf8);
  AddRel("BL" , 0xf9); AddRel("BGE", 0xfa); AddRel("BLT", 0xfb);
  AddRel("BGT", 0xfc); AddRel("BLE", 0xfd); AddRel("BR" , 0xfe);
  AddRel("JZ" , 0xec); AddRel("JNZ", 0xee);
  SetInv("BCC", "BCS", RelOrders);
  SetInv("BNE", "BEQ", RelOrders);
  SetInv("BNV", "BOV", RelOrders);
  SetInv("BPL", "BMI", RelOrders);
  SetInv("BG" , "BL" , RelOrders);
  SetInv("BGE", "BLT", RelOrders);
  SetInv("BGT", "BLE", RelOrders);
  SetInv("JZ" , "JNZ", RelOrders);
}

static void DeinitFields(void)
{
  order_array_free(JBitOrders);
  order_array_free(RegOrders);
  order_array_free(RelOrders);

  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/
/* Callbacks */

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_XA(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on XA
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_XA(char *pArg, TempResult *pResult)
{
  Byte Reg;
  tSymbolSize Size;

  if (*AttrPart.str.p_str)
    OpSize = AttrPartOpSize[0];

  if (DecodeRegCore(pArg, &Size, &Reg))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_XA;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void InitCode_XA(void)
{
  Reg_DS = 0;
}

static Boolean ChkPC_XA(LargeWord Addr)
{
  switch (ActPC)
  {
    case SegCode:
    case SegData:
      return (Addr<0x1000000);
    case SegIO:
      return ((Addr > 0x3ff) && (Addr < 0x800));
    default:
      return False;
  }
}

static Boolean IsDef_XA(void)
{
  return (ActPC == SegCode) || IsRealDef();
}

static void SwitchTo_XA(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = 0x3c; NOPCode = 0x00;
  DivideChars = ","; HasAttrs = True; AttrChars = ".";

  ValidSegs =(1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode ] = 1; ListGrans[SegCode ] = 1; SegInits[SegCode ] = 0;
  Grans[SegData ] = 1; ListGrans[SegData ] = 1; SegInits[SegData ] = 0;
  Grans[SegIO   ] = 1; ListGrans[SegIO   ] = 1; SegInits[SegIO   ] = 0x400;

  DecodeAttrPart = DecodeAttrPart_XA;
  MakeCode = MakeCode_XA;
  ChkPC = ChkPC_XA;
  IsDef = IsDef_XA;
  InternSymbol = InternSymbol_XA;
  DissectReg = DissectReg_XA;
  SwitchFrom = DeinitFields; InitFields();
  onoff_supmode_add();
  if (!onoff_test_and_set(e_onoff_reg_branchext))
    SetFlag(&DoBranchExt, BranchExtSymName, False);
  AddONOFF(BranchExtCmdName, &DoBranchExt, BranchExtSymName , False);
  AddMoto16PseudoONOFF(False);

  pASSUMERecs = ASSUMEXAs;
  ASSUMERecCnt = ASSUMEXACount;
}

void codexa_init(void)
{
  CPUXAG1 = AddCPU("XAG1", SwitchTo_XA);
  CPUXAG2 = AddCPU("XAG2", SwitchTo_XA);
  CPUXAG3 = AddCPU("XAG3", SwitchTo_XA);

  AddInitPassProc(InitCode_XA);
}
