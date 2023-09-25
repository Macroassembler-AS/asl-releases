/* code166.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegenerator Siemens 80C16x                                           */
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
#include "asmitree.h"
#include "asmallg.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code166.h"

typedef struct
{
  CPUVar MinCPU;
  Word Code1, Code2;
} BaseOrder;

typedef struct
{
  const char *Name;
  Byte Code;
} Condition;

#define COND_CODE_TRUE 0x0

#define DPPCount 4
static const char RegNames[6][5] = { "DPP0", "DPP1", "DPP2", "DPP3", "CP", "SP" };

static CPUVar CPU80C166, CPU80C167, CPU80C167CS;

static BaseOrder *FixedOrders;
static Condition *Conditions;
static int TrueCond;

static LongInt DPPAssumes[DPPCount];
static IntType MemInt, MemInt2;
static tSymbolSize OpSize;

static Boolean DPPChanged[DPPCount], N_DPPChanged[DPPCount];
static Boolean SPChanged, CPChanged, N_SPChanged, N_CPChanged;

static ShortInt ExtCounter;
static enum
{
  MemModeStd,       /* normal */
  MemModeNoCheck,   /* EXTS Rn */
  MemModeZeroPage,  /* EXTP Rn */
  MemModeFixedBank, /* EXTS nn */
  MemModeFixedPage  /* EXTP nn */
} MemMode;
static Word MemPage;
static Boolean ExtSFRs;

#define ASSUME166Count 4
static ASSUMERec ASSUME166s[ASSUME166Count] =
{
  { "DPP0", DPPAssumes + 0, 0, 15, -1, NULL },
  { "DPP1", DPPAssumes + 1, 0, 15, -1, NULL },
  { "DPP2", DPPAssumes + 2, 0, 15, -1, NULL },
  { "DPP3", DPPAssumes + 3, 0, 15, -1, NULL }
};

/*-------------------------------------------------------------------------*/

enum
{
  ModNone = -1,
  ModReg = 0,
  ModImm = 1,
  ModIReg = 2,
  ModPreDec = 3,
  ModPostInc = 4,
  ModIndex = 5,
  ModAbs = 6,
  ModMReg = 7,
  ModLAbs = 8
};

typedef enum
{
  eForceNone = 0,
  eForceShort = 1,
  eForceLong = 2
} tForceSize;

#define MModReg (1 << ModReg)
#define MModImm (1 << ModImm)
#define MModIReg (1 << ModIReg)
#define MModPreDec (1 << ModPreDec)
#define MModPostInc (1 << ModPostInc)
#define MModIndex (1 << ModIndex)
#define MModAbs (1 << ModAbs)
#define MModMReg (1 << ModMReg)
#define MModLAbs (1 << ModLAbs)

#define M_InCode (1 << 14)
#define M_Dest (1 << 13)

typedef struct
{
  Byte Mode;
  Byte Vals[2];
  ShortInt Type;
  tSymbolFlags SymFlags;
  tForceSize ForceSize;
  int Cnt;
} tAdrResult;

/*!------------------------------------------------------------------------
 * \fn     IsRegCore(const char *pArg, tRegInt *pValue, tSymbolSize *pSize)
 * \brief  check whether argument describes a CPU (general purpose) register
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \return true if yes
 * ------------------------------------------------------------------------ */

static Boolean IsRegCore(const char *pArg, tRegInt *pValue, tSymbolSize *pSize)
{
  int l = strlen(pArg);
  Boolean OK;

  if ((l < 2) || (as_toupper(*pArg) != 'R'))
    return False;
  else if ((l > 2) && (as_toupper(pArg[1]) == 'L'))
  {
    *pValue = ConstLongInt(pArg + 2, &OK, 10) << 1;
    *pSize = eSymbolSize8Bit;
    return (OK && (*pValue <= 15));
  }
  else if ((l > 2) && (as_toupper(pArg[1]) == 'H'))
  {
    *pValue = (ConstLongInt(pArg + 2, &OK, 10) << 1) + 1;
    *pSize = eSymbolSize8Bit;
    return (OK && (*pValue <= 15));
  }
  else
  {
    *pValue = ConstLongInt(pArg + 1, &OK, 10);
    *pSize = eSymbolSize16Bit;
    return (OK && (*pValue <= 15));
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_166(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - C16x variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_166(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%c%u", Value & 1 ? 'H' : 'L', (unsigned)(Value >> 1));
      break;
    case eSymbolSize16Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     IsReg(const tStrComp *pArg, Byte *pValue, tSymbolSize *pSize, tSymbolSize ReqSize, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or user-defined register alias
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \param  ReqSize requested register size
 * \param  MustBeReg expecting register or maybe not?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

/* NOTE: If requester register size is 8 bits, R0..R15 is allowed as
   alias for R0L,R0H,R1L,...,R7H: */

static Boolean ChkRegSize(tSymbolSize ReqSize, tSymbolSize ActSize)
{
  return (ReqSize == eSymbolSizeUnknown)
      || (ReqSize == ActSize)
      || ((ReqSize == eSymbolSize8Bit) && (ActSize == eSymbolSize16Bit));
}

static tRegEvalResult IsReg(const tStrComp *pArg, Byte *pValue, tSymbolSize *pSize, tSymbolSize ReqSize, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsRegCore(pArg->str.p_str, &RegDescr.Reg, &EvalResult.DataSize))
    RegEvalResult = eIsReg;
  else
    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);

  if (RegEvalResult == eIsReg)
  {
    if (!ChkRegSize(ReqSize, EvalResult.DataSize))
    {
      WrStrErrorPos(ErrNum_InvOpSize, pArg);
      RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;
    }
  }

  *pValue = RegDescr.Reg;
  if (pSize) *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

static tRegEvalResult IsRegM1(const tStrComp *pArg, Byte *pValue, tSymbolSize ReqSize, Boolean MustBeReg)
{
  if (*pArg->str.p_str)
  {
    int l;
    char tmp = pArg->str.p_str[l = (strlen(pArg->str.p_str) - 1)];
    tRegEvalResult b;

    pArg->str.p_str[l] = '\0';
    b = IsReg(pArg, pValue, NULL, ReqSize, MustBeReg);
    pArg->str.p_str[l] = tmp;
    return b;
  }
  else
    return eIsNoReg;
}

static tRegEvalResult IsRegP1(const tStrComp *pArg, Byte *pValue, tSymbolSize ReqSize, Boolean MustBeReg)
{
  tStrComp Arg;

  StrCompRefRight(&Arg, pArg, 1);
  return IsReg(&Arg, pValue, NULL, ReqSize, MustBeReg);
}

static LongInt SFRStart(void)
{
  return (ExtSFRs) ? 0xf000 : 0xfe00;
}

static LongInt SFREnd(void)
{
  return (ExtSFRs) ? 0xf1de : 0xffde;
}

static Boolean CalcPage(LongInt *Adr, Boolean DoAnyway)
{
  int z;
  Word Bank;

  switch (MemMode)
  {
    case MemModeStd:
      z = 0;
      while ((z <= 3) && (((*Adr) >> 14) != DPPAssumes[z]))
        z++;
      if (z > 3)
      {
        WrError(ErrNum_InAccPage);
        (*Adr) &= 0xffff;
        return DoAnyway;
      }
      else
      {
        *Adr = ((*Adr) & 0x3fff) + (z << 14);
        if (DPPChanged[z])
          WrXError(ErrNum_Pipeline, RegNames[z]);
        return True;
      }
    case MemModeZeroPage:
      (*Adr) &= 0x3fff;
      return True;
    case MemModeFixedPage:
      Bank = (*Adr) >> 14;
      (*Adr) &= 0x3fff;
      if (Bank != MemPage)
      {
        WrError(ErrNum_InAccPage);
        return (DoAnyway);
      }
      else
        return True;
    case MemModeNoCheck:
      (*Adr) &= 0xffff;
      return True;
    case MemModeFixedBank:
      Bank = (*Adr) >> 16; (*Adr) &= 0xffff;
      if (Bank != MemPage)
      {
        WrError(ErrNum_InAccPage);
        return (DoAnyway);
      }
      else
        return True;
    default:
      return False;
  }
}

static void DecideAbsolute(LongInt DispAcc, Word Mask, tAdrResult *pResult)
{
#define DPPAdr 0xfe00
#define SPAdr 0xfe12
#define CPAdr 0xfe10

  int z;

  if (Mask & M_InCode)
  {
    if ((HiWord(EProgCounter()) == HiWord(DispAcc)) && (Mask & MModAbs))
    {
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc);
    }
    else
    {
      pResult->Type = ModLAbs;
      pResult->Cnt = 2;
      pResult->Mode = DispAcc >> 16;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc);
    }
  }
  else if (((Mask & MModMReg) != 0) && (DispAcc >= SFRStart()) && (DispAcc <= SFREnd()) && (!(DispAcc & 1)))
  {
    pResult->Type = ModMReg;
    pResult->Cnt = 1;
    pResult->Vals[0] = (DispAcc - SFRStart()) >> 1;
  }
  else switch (MemMode)
  {
    case MemModeStd:
      z = 0;
      while ((z <= 3) && ((DispAcc >> 14) != DPPAssumes[z]))
        z++;
      if (z > 3)
      {
        WrError(ErrNum_InAccPage);
        z = (DispAcc >> 14) & 3;
      }
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = (Hi(DispAcc) & 0x3f) + (z << 6);
      if (DPPChanged[z])
        WrXError(ErrNum_Pipeline, RegNames[z]);
      break;
    case MemModeZeroPage:
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc) & 0x3f;
      break;
    case MemModeFixedPage:
      if ((DispAcc >> 14) != MemPage)
        WrError(ErrNum_InAccPage);
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc) & 0x3f;
      break;
    case MemModeNoCheck:
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc);
      break;
    case MemModeFixedBank:
      if ((DispAcc >> 16) != MemPage)
        WrError(ErrNum_InAccPage);
      pResult->Type = ModAbs;
      pResult->Cnt = 2;
      pResult->Vals[0] = Lo(DispAcc);
      pResult->Vals[1] = Hi(DispAcc);
      break;
  }

  if ((pResult->Type != ModNone) && (Mask & M_Dest))
  {
    switch ((Word)DispAcc)
    {
      case SPAdr    : N_SPChanged = True; break;
      case CPAdr    : N_CPChanged = True; break;
      case DPPAdr   :
      case DPPAdr + 1 : N_DPPChanged[0] = True; break;
      case DPPAdr + 2 :
      case DPPAdr + 3 : N_DPPChanged[1] = True; break;
      case DPPAdr + 4 :
      case DPPAdr + 5 : N_DPPChanged[2] = True; break;
      case DPPAdr + 6 :
      case DPPAdr + 7 : N_DPPChanged[3] = True; break;
    }
  }
}

static int SplitForceSize(const char *pArg, tForceSize *pForceSize)
{
  switch (*pArg)
  {
    case '>': *pForceSize = eForceLong; return 1;
    case '<': *pForceSize = eForceShort; return 1;
    default: return 0;
  }
}

static ShortInt DecodeAdr(const tStrComp *pArg, Word Mask, tAdrResult *pResult)
{
  LongInt HDisp, DispAcc;
  Boolean OK, NegFlag, NNegFlag;
  Byte HReg;
  int Offs;
  tRegEvalResult RegEvalResult;

  pResult->Type = ModNone;
  pResult->Cnt = 0;
  pResult->ForceSize = eForceNone;

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    Offs = SplitForceSize(pArg->str.p_str + 1, &pResult->ForceSize);
    switch (OpSize)
    {
      case eSymbolSize8Bit:
        pResult->Vals[0] = EvalStrIntExpressionOffsWithFlags(pArg, 1 + Offs, Int8, &OK, &pResult->SymFlags);
        pResult->Vals[1] = 0;
        break;
      case eSymbolSize16Bit:
        HDisp = EvalStrIntExpressionOffsWithFlags(pArg, 1 + Offs, Int16, &OK, &pResult->SymFlags);
        pResult->Vals[0] = Lo(HDisp);
        pResult->Vals[1] = Hi(HDisp);
        break;
      default:
        OK = False;
        break;
    }
    if (OK)
    {
      pResult->Type = ModImm;
      AdrCnt = OpSize + 1;
    }
  }

  /* Register ? */

  else if ((RegEvalResult = IsReg(pArg, &pResult->Mode, NULL, OpSize, False)) != eIsNoReg)
  {
    if (RegEvalResult == eRegAbort)
      return pResult->Type;
    if ((Mask & MModReg) != 0)
      pResult->Type = ModReg;
    else
    {
      pResult->Type = ModMReg;
      pResult->Vals[0] = 0xf0 + pResult->Mode;
      AdrCnt = 1;
    }
    if (CPChanged)
      WrXError(ErrNum_Pipeline, RegNames[4]);
  }

  /* indirekt ? */

  else if ((*pArg->str.p_str == '[') && (pArg->str.p_str[strlen(pArg->str.p_str) - 1] == ']'))
  {
    tStrComp Arg;
    int ArgLen;

    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);
    ArgLen = strlen(Arg.str.p_str);

    /* Predekrement ? */

    if ((ArgLen > 2) && (*Arg.str.p_str == '-') && ((RegEvalResult = IsRegP1(&Arg, &pResult->Mode, eSymbolSize16Bit, False)) != eIsNoReg))
    {
      if (eRegAbort == RegEvalResult)
        return pResult->Type;
      pResult->Type = ModPreDec;
    }

    /* Postinkrement ? */

    else if ((ArgLen > 2) && (Arg.str.p_str[ArgLen - 1] == '+') && ((RegEvalResult = IsRegM1(&Arg, &pResult->Mode, eSymbolSize16Bit, False)) != eIsNoReg))
    {
      if (eRegAbort == RegEvalResult)
        return pResult->Type;
      pResult->Type = ModPostInc;
    }

    /* indiziert ? */

    else
    {
      tStrComp Remainder;
      char *pSplitPos;

      NNegFlag = NegFlag = False;
      DispAcc = 0;
      pResult->Mode = 0xff;
      do
      {
        pSplitPos = QuotMultPos(Arg.str.p_str, "-+");
        if (pSplitPos)
        {
          NNegFlag = *pSplitPos == '-';
          StrCompSplitRef(&Arg, &Remainder, &Arg, pSplitPos);
        }
        if ((RegEvalResult = IsReg(&Arg, &HReg, NULL, eSymbolSize16Bit, False)) != eIsNoReg)
        {
          if (RegEvalResult == eRegAbort)
            return pResult->Type;
          if (NegFlag || (pResult->Mode != 0xff))
            WrError(ErrNum_InvAddrMode);
          else
            pResult->Mode = HReg;
        }
        else
        {
          HDisp = EvalStrIntExpressionOffs(&Arg, !!(*Arg.str.p_str == '#'), Int32, &OK);
          if (OK)
            DispAcc = NegFlag ? DispAcc - HDisp : DispAcc + HDisp;
        }
        if (pSplitPos)
        {
          NegFlag = NNegFlag;
          Arg = Remainder;
        }
      }
      while (pSplitPos);
      if (pResult->Mode == 0xff)
        DecideAbsolute(DispAcc, Mask, pResult);
      else if (DispAcc == 0)
        pResult->Type = ModIReg;
      else if (DispAcc > 0xffff)
        WrError(ErrNum_OverRange);
      else if (DispAcc < -0x8000l)
        WrError(ErrNum_UnderRange);
      else
      {
        pResult->Vals[0] = Lo(DispAcc);
        pResult->Vals[1] = Hi(DispAcc);
        pResult->Type = ModIndex;
        pResult->Cnt = 2;
      }
    }
  }
  else
  {
    int Offset = SplitForceSize(pArg->str.p_str, &pResult->ForceSize);

    DispAcc = EvalStrIntExpressionOffsWithFlags(pArg, Offset, MemInt, &OK, &pResult->SymFlags);
    if (OK)
      DecideAbsolute(DispAcc, Mask, pResult);
  }

  if ((pResult->Type != ModNone) && (!((1 << pResult->Type) & Mask)))
  {
    WrError(ErrNum_InvAddrMode);
    pResult->Type = ModNone;
    pResult->Cnt = 0;
  }
  return pResult->Type;
}

static Boolean DecodeCondition(const char *Name, Byte *p_cond_code)
{
  int z;

  for (z = 0; Conditions[z].Name; z++)
    if (!as_strcasecmp(Conditions[z].Name, Name))
    {
      *p_cond_code = Conditions[z].Code;
      return True;
    }
  return False;
}

static Boolean DecodeBitAddr(const tStrComp *pArg, Word *Adr, Byte *Bit, Boolean MayBeOut)
{
  char *p;
  Word LAdr;
  Byte Reg;
  Boolean OK;

  p = QuotPos(pArg->str.p_str, '.');
  if (!p)
  {
    LAdr = EvalStrIntExpression(pArg, UInt16, &OK) & 0x1fff;
    if (OK)
    {
      if ((!MayBeOut) && ((LAdr >> 12) != Ord(ExtSFRs)))
      {
        WrError(ErrNum_InAccReg);
        return False;
      }
      *Adr = LAdr >> 4;
      *Bit = LAdr & 15;
      if (!MayBeOut)
        *Adr = Lo(*Adr);
      return True;
    }
    else return False;
  }
  else if (p == pArg->str.p_str)
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  else
  {
    tStrComp AddrComp, BitComp;

    StrCompSplitRef(&AddrComp, &BitComp, pArg, p);

    switch (IsReg(&AddrComp, &Reg, NULL, eSymbolSize16Bit, False))
    {
      case eIsReg:
        *Adr = 0xf0 + Reg;
        break;
      case eRegAbort:
        return False;
      case eIsNoReg:
      {
        tSymbolFlags Flags;

        LAdr = EvalStrIntExpressionWithFlags(&AddrComp, UInt16, &OK, &Flags);
        if (!OK)
          return False;
        if (mFirstPassUnknown(Flags))
          LAdr = 0xfd00;

        /* full addresses must be even, since bitfields in memory are 16 bit: */

        if ((LAdr > 0xff) && (LAdr & 1))
        {
          WrStrErrorPos(ErrNum_NotAligned, &AddrComp);
          return False;
        }

        /* coded bit address: */

        if (LAdr <= 0xff)
          *Adr = LAdr;

        /* 1st RAM bank: */

        else if ((LAdr >= 0xfd00) && (LAdr <= 0xfdfe))
          *Adr = (LAdr - 0xfd00)/2;

        /* SFR space: */

        else if ((LAdr >= 0xff00) && (LAdr <= 0xffde))
        {
          if ((ExtSFRs) && (!MayBeOut))
          {
            WrStrErrorPos(ErrNum_InAccReg, &AddrComp);
            return False;
          }
          *Adr = 0x80 + ((LAdr - 0xff00) / 2);
        }

        /* extended SFR space: */

        else if ((LAdr >= 0xf100) && (LAdr <= 0xf1de))
        {
          if ((!ExtSFRs) && (!MayBeOut))
          {
            WrStrErrorPos(ErrNum_InAccReg, &AddrComp);
            return False;
          }
          *Adr = 0x80 + ((LAdr - 0xf100) / 2);
          if (MayBeOut)
            (*Adr) += 0x100;
        }
        else
        {
          WrStrErrorPos(ErrNum_OverRange, &AddrComp);
          return False;
        }
      }
    }

    *Bit = EvalStrIntExpression(&BitComp, UInt4, &OK);
    return OK;
  }
}

static Word WordVal(const tAdrResult *pResult)
{
  return pResult->Vals[0] + (((Word)pResult->Vals[1]) << 8);
}

static Boolean DecodePref(const tStrComp *pArg, Byte *Erg)
{
  Boolean OK;
  tSymbolFlags Flags;

  if (*pArg->str.p_str != '#')
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }
  *Erg = EvalStrIntExpressionOffsWithFlags(pArg, 1, UInt3, &OK, &Flags);
  if (mFirstPassUnknown(Flags))
    *Erg = 1;
  if (!OK)
    return False;
  if (*Erg < 1)
    WrError(ErrNum_UnderRange);
  else if (*Erg > 4)
    WrError(ErrNum_OverRange);
  else
  {
    (*Erg)--;
    return True;
  }
  return False;
}

/*-------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  const BaseOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0))
  {
    CodeLen = 2;
    BAsmCode[0] = Lo(pOrder->Code1);
    BAsmCode[1] = Hi(pOrder->Code1);
    if (pOrder->Code2 != 0)
    {
      CodeLen = 4;
      BAsmCode[2] = Lo(pOrder->Code2);
      BAsmCode[3] = Hi(pOrder->Code2);
      if ((!strncmp(OpPart.str.p_str, "RET", 3)) && (SPChanged))
        WrXError(ErrNum_Pipeline, RegNames[5]);
    }
  }
}

static void DecodeMOV(Word Code)
{
  LongInt AdrLong;

  OpSize = (tSymbolSize)Hi(Code);
  Code = 1 - OpSize;

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg | MModMReg | MModIReg | MModPreDec | MModPostInc | MModIndex | MModAbs | M_Dest, &DestResult))
    {
      case ModReg:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModPostInc | MModIndex | MModAbs, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
             BAsmCode[0] = 0xf0 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModImm:
          {
            Boolean IsShort = WordVal(&SrcResult) <= 15;

            if (!SrcResult.ForceSize)
              SrcResult.ForceSize = IsShort ? eForceShort : eForceLong;
            if (SrcResult.ForceSize == eForceShort)
            {
              if (!IsShort && !mSymbolQuestionable(SrcResult.SymFlags)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
              else
              {
                CodeLen = 2;
                BAsmCode[0] = 0xe0 + Code;
                BAsmCode[1] = (WordVal(&SrcResult) << 4) + DestResult.Mode;
              }
            }
            else
            {
              CodeLen = 4;
              BAsmCode[0] = 0xe6 + Code;
              BAsmCode[1] = DestResult.Mode + 0xf0;
              memcpy(BAsmCode + 2, SrcResult.Vals, 2);
            }
            break;
          }
          case ModIReg:
            CodeLen = 2;
            BAsmCode[0] = 0xa8 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModPostInc:
            CodeLen = 2;
            BAsmCode[0] = 0x98 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModIndex:
            CodeLen = 2 + SrcResult.Cnt;
            BAsmCode[0] = 0xd4 + (Code << 5);
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
          case ModAbs:
            CodeLen = 2 + SrcResult.Cnt;
            BAsmCode[0] = 0xf2 + Code;
            BAsmCode[1] = 0xf0 + DestResult.Mode;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
        }
        break;
      }
      case ModMReg:
      {
        tAdrResult SrcResult;

        BAsmCode[1] = DestResult.Vals[0];
        switch (DecodeAdr(&ArgStr[2], MModImm | MModMReg | ((DPPAssumes[3] == 3) ? MModIReg : 0) | MModAbs, &SrcResult))
        {
          case ModImm:
            CodeLen = 4;
            BAsmCode[0] = 0xe6 + Code;
            memcpy(BAsmCode + 2, SrcResult.Vals, 2);
            break;
          case ModMReg: /* BAsmCode[1] sicher absolut darstellbar, da Rn vorher */
                        /* abgefangen wird! */
            BAsmCode[0] = 0xf6 + Code;
            AdrLong = SFRStart() + (((Word)BAsmCode[1]) << 1);
            CalcPage(&AdrLong, True);
            BAsmCode[2] = Lo(AdrLong);
            BAsmCode[3] = Hi(AdrLong);
            BAsmCode[1] = SrcResult.Vals[0];
            CodeLen = 4;
            break;
          case ModIReg:
            CodeLen = 4; BAsmCode[0] = 0x94 + (Code << 5);
            BAsmCode[2] = BAsmCode[1] << 1;
            BAsmCode[3] = 0xfe + (BAsmCode[1] >> 7); /* ANSI :-0 */
            BAsmCode[1] = SrcResult.Mode;
            break;
          case ModAbs:
            CodeLen = 2 + SrcResult.Cnt;
            BAsmCode[0] = 0xf2 + Code;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
        }
        break;
      }
      case ModIReg:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModPostInc | MModAbs, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0xb8 + Code;
            BAsmCode[1] = DestResult.Mode + (SrcResult.Mode << 4);
            break;
          case ModIReg:
            CodeLen = 2;
           BAsmCode[0] = 0xc8 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModPostInc:
            CodeLen = 2;
            BAsmCode[0] = 0xe8 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModAbs:
            CodeLen = 2 + SrcResult.Cnt;
            BAsmCode[0] = 0x84 + (Code << 5);
            BAsmCode[1] = DestResult.Mode;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
        }
        break;
      }
      case ModPreDec:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0x88 + Code;
            BAsmCode[1] = DestResult.Mode + (SrcResult.Mode << 4);
            break;
        }
        break;
      }
      case ModPostInc:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModIReg, &SrcResult))
        {
          case ModIReg:
            CodeLen = 2;
            BAsmCode[0] = 0xd8 + Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
        }
        break;
      }
      case ModIndex:
      {
        tAdrResult SrcResult;

        BAsmCode[1] = DestResult.Mode;
        memcpy(BAsmCode + 2, DestResult.Vals, DestResult.Cnt);
        switch (DecodeAdr(&ArgStr[2], MModReg, &SrcResult))
        {
          case ModReg:
            BAsmCode[0] = 0xc4 + (Code << 5);
            CodeLen = 4;
            BAsmCode[1] += SrcResult.Mode << 4;
            break;
        }
        break;
      }
      case ModAbs:
      {
        tAdrResult SrcResult;

        memcpy(BAsmCode + 2, DestResult.Vals, DestResult.Cnt);
        switch (DecodeAdr(&ArgStr[2], MModIReg | MModMReg, &SrcResult))
        {
          case ModIReg:
            CodeLen = 4;
            BAsmCode[0] = 0x94 + (Code << 5);
            BAsmCode[1] = SrcResult.Mode;
            break;
          case ModMReg:
            CodeLen = 4;
            BAsmCode[0] = 0xf6 + Code;
            BAsmCode[1] = SrcResult.Vals[0];
            break;
        }
        break;
      }
    }
  }
}

static void DecodeMOVBS_MOVBZ(Word Code)
{
  LongInt AdrLong;

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[1], MModReg | MModMReg | MModAbs | M_Dest, &DestResult))
    {
      case ModReg:
      {
        tAdrResult SrcResult;

        OpSize = eSymbolSize8Bit;
        switch (DecodeAdr(&ArgStr[2], MModReg | MModAbs, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0xc0 + Code;
            BAsmCode[1] = DestResult.Mode + (SrcResult.Mode << 4);
            break;
          case ModAbs:
            CodeLen = 4;
            BAsmCode[0] = 0xc2 + Code;
            BAsmCode[1] = 0xf0 + DestResult.Mode;
            memcpy(BAsmCode + 2, SrcResult.Vals, 2);
            break;
        }
        break;
      }
      case ModMReg:
      {
        tAdrResult SrcResult;

        BAsmCode[1] = DestResult.Vals[0];
        OpSize = eSymbolSize8Bit;
        switch (DecodeAdr(&ArgStr[2], MModAbs | MModMReg, &SrcResult))
        {
          case ModMReg: /* BAsmCode[1] sicher absolut darstellbar, da Rn vorher */
                        /* abgefangen wird! */
            BAsmCode[0] = 0xc5 + Code;
            AdrLong = SFRStart() + (((Word)BAsmCode[1]) << 1);
            CalcPage(&AdrLong, True);
            BAsmCode[2] = Lo(AdrLong);
            BAsmCode[3] = Hi(AdrLong);
            BAsmCode[1] = SrcResult.Vals[0];
            CodeLen = 4;
            break;
          case ModAbs:
            CodeLen = 2 + SrcResult.Cnt;
            BAsmCode[0] = 0xc2 + Code;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
        }
        break;
      }
      case ModAbs:
      {
        tAdrResult SrcResult;

        OpSize = eSymbolSize8Bit;
        memcpy(BAsmCode + 2, DestResult.Vals, DestResult.Cnt);
        switch (DecodeAdr(&ArgStr[2], MModMReg, &SrcResult))
        {
          case ModMReg:
            CodeLen = 4;
            BAsmCode[0] = 0xc5 + Code;
            BAsmCode[1] = SrcResult.Vals[0];
            break;
        }
        break;
      }
    }
  }
}

static void DecodePUSH_POP(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModMReg | ((Code & 0x10) ? M_Dest : 0), &Result))
    {
      case ModMReg:
        CodeLen = 2;
        BAsmCode[0] = Code;
        BAsmCode[1] = Result.Vals[0];
        if (SPChanged) WrXError(ErrNum_Pipeline, RegNames[5]);
        break;
    }
  }
}

static void DecodeSCXT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModMReg | M_Dest, &Result))
    {
      case ModMReg:
        BAsmCode[1] = Result.Vals[0];
        if (DecodeAdr(&ArgStr[2], MModAbs | MModImm, &Result) != ModNone)
        {
          CodeLen = 4; BAsmCode[0] = 0xc6 + (Ord(Result.Type == ModAbs) << 4);
          memcpy(BAsmCode + 2, Result.Vals, 2);
        }
        break;
    }
  }
}

static void DecodeALU2(Word Code)
{
  LongInt AdrLong;

  OpSize = (tSymbolSize)Hi(Code);
  Code = (1 - OpSize) + (Lo(Code)  << 4);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg | MModMReg | MModAbs | M_Dest, &DestResult))
    {
      case ModReg:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModPostInc | MModAbs | MModImm, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = Code;
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
          case ModIReg:
            if (SrcResult.Mode > 3) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x08 + Code;
              BAsmCode[1] = (DestResult.Mode << 4) + 8 + SrcResult.Mode;
            }
            break;
          case ModPostInc:
            if (SrcResult.Mode > 3) WrError(ErrNum_InvAddrMode);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = 0x08 + Code;
              BAsmCode[1] = (DestResult.Mode << 4) + 12 + SrcResult.Mode;
            }
            break;
          case ModAbs:
            CodeLen = 4;
            BAsmCode[0] = 0x02 + Code;
            BAsmCode[1] = 0xf0 + DestResult.Mode;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
          case ModImm:
          {
            Boolean IsShort = WordVal(&SrcResult) <= 7;

            if (!SrcResult.ForceSize)
              SrcResult.ForceSize = IsShort ? eForceShort : eForceLong;
            if (SrcResult.ForceSize == eForceShort)
            {
              if (!IsShort && !mSymbolQuestionable(SrcResult.SymFlags)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
              else
              {
                CodeLen = 2;
                BAsmCode[0] = 0x08 + Code;
                BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Vals[0];
              }
            }
            else
            {
              CodeLen = 4;
              BAsmCode[0] = 0x06 + Code;
              BAsmCode[1] = 0xf0 + DestResult.Mode;
              memcpy(BAsmCode + 2, SrcResult.Vals, 2);
            }
            break;
          }
        }
        break;
      }
      case ModMReg:
      {
        tAdrResult SrcResult;

        BAsmCode[1] = DestResult.Vals[0];
        switch (DecodeAdr(&ArgStr[2], MModAbs | MModMReg | MModImm, &SrcResult))
        {
          case ModAbs:
            CodeLen = 4;
            BAsmCode[0] = 0x02 + Code;
            memcpy(BAsmCode + 2, SrcResult.Vals, SrcResult.Cnt);
            break;
          case ModMReg: /* BAsmCode[1] sicher absolut darstellbar, da Rn vorher */
                        /* abgefangen wird! */
            BAsmCode[0] = 0x04 + Code;
            AdrLong = SFRStart() + (((Word)BAsmCode[1]) << 1);
            CalcPage(&AdrLong, True);
            BAsmCode[2] = Lo(AdrLong);
            BAsmCode[3] = Hi(AdrLong);
            BAsmCode[1] = SrcResult.Vals[0];
            CodeLen = 4;
            break;
          case ModImm:
            CodeLen = 4;
            BAsmCode[0] = 0x06 + Code;
            memcpy(BAsmCode + 2, SrcResult.Vals, 2);
            break;
        }
        break;
      }
      case ModAbs:
      {
        tAdrResult SrcResult;

        memcpy(BAsmCode + 2, DestResult.Vals, DestResult.Cnt);
        switch (DecodeAdr(&ArgStr[2], MModMReg, &SrcResult))
        {
          case ModMReg:
            CodeLen = 4;
            BAsmCode[0] = 0x04 + Code;
            BAsmCode[1] = SrcResult.Vals[0];
            break;
        }
        break;
      }
    }
  }
}

static void DecodeCPL_NEG(Word Code)
{
  OpSize = (tSymbolSize)Hi(Code);

  Code = Lo(Code) + ((1 - OpSize) << 5);
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    if (DecodeAdr(&ArgStr[1], MModReg | M_Dest, &Result) == ModReg)
    {
      CodeLen = 2;
      BAsmCode[0] = Code;
      BAsmCode[1] = Result.Mode << 4;
    }
  }
}

static void DecodeDiv(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    if (DecodeAdr(&ArgStr[1], MModReg, &Result) == ModReg)
    {
      CodeLen = 2;
      BAsmCode[0] = 0x4b + (Code << 4);
      BAsmCode[1] = Result.Mode * 0x11;
    }
  }
}

static void DecodeLoop(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    if (DecodeAdr(&ArgStr[1], MModReg | M_Dest, &Result) == ModReg)
    {
      BAsmCode[1] = Result.Mode;
      switch (DecodeAdr(&ArgStr[2], MModAbs | MModImm, &Result))
      {
        case ModAbs:
          CodeLen = 4;
          BAsmCode[0] = Code + 2;
          BAsmCode[1] += 0xf0;
          memcpy(BAsmCode + 2, Result.Vals, 2);
          break;
        case ModImm:
        {
          Boolean IsShort = WordVal(&Result) < 16;

          if (!Result.ForceSize)
            Result.ForceSize = IsShort ? eForceShort : eForceLong;
          if (Result.ForceSize == eForceShort)
          {
            if (!IsShort && !mSymbolQuestionable(Result.SymFlags)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
            else
            {
              CodeLen = 2;
              BAsmCode[0] = Code;
              BAsmCode[1] += (WordVal(&Result) << 4);
            }
          }
          else
          {
            CodeLen = 4;
            BAsmCode[0] = Code + 6;
            BAsmCode[1] += 0xf0;
            memcpy(BAsmCode + 2, Result.Vals, 2);
          }
          break;
        }
      }
    }
  }
}

static void DecodeMul(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg, &DestResult))
    {
      case ModReg:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg, &SrcResult))
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0x0b + (Code << 4);
            BAsmCode[1] = (DestResult.Mode << 4) + SrcResult.Mode;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    OpSize = eSymbolSize16Bit;
    switch (DecodeAdr(&ArgStr[1], MModReg | M_Dest, &DestResult))
    {
      case ModReg:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | M_Dest, &SrcResult))
        {
          case ModReg:
            BAsmCode[0] = Code;
            BAsmCode[1] = SrcResult.Mode + (DestResult.Mode << 4);
            CodeLen = 2;
            break;
          case ModImm:
            if ((WordVal(&SrcResult) > 15) && !mSymbolQuestionable(SrcResult.SymFlags)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[2]);
            else
            {
              BAsmCode[0] = Code + 0x10;
              BAsmCode[1] = (WordVal(&SrcResult) << 4) + DestResult.Mode;
              CodeLen = 2;
            }
            break;
        }
        break;
      }
    }
  }
}

static void DecodeBit2(Word Code)
{
  Byte BOfs1, BOfs2;
  Word BAdr1, BAdr2;

  if (ChkArgCnt(2, 2)
   && DecodeBitAddr(&ArgStr[1], &BAdr1, &BOfs1, False)
   && DecodeBitAddr(&ArgStr[2], &BAdr2, &BOfs2, False))
  {
    CodeLen = 4;
    BAsmCode[0] = Code;
    BAsmCode[1] = BAdr2;
    BAsmCode[2] = BAdr1;
    BAsmCode[3] = (BOfs2 << 4) + BOfs1;
  }
}

static void DecodeBCLR_BSET(Word Code)
{
  Byte BOfs;
  Word BAdr;

  if (ChkArgCnt(1, 1)
   && DecodeBitAddr(&ArgStr[1], &BAdr, &BOfs, False))
  {
    CodeLen = 2;
    BAsmCode[0] = (BOfs << 4) + Code;
    BAsmCode[1] = BAdr;
  }
}

static void DecodeBFLDH_BFLDL(Word Code)
{
  Byte BOfs;
  Word BAdr;

  if (ChkArgCnt(3, 3))
  {
    strmaxcat(ArgStr[1].str.p_str, ".0", STRINGSIZE);
    if (DecodeBitAddr(&ArgStr[1], &BAdr, &BOfs, False))
    {
      tAdrResult Result;

      OpSize = eSymbolSize8Bit;
      BAsmCode[1] = BAdr;
      if (DecodeAdr(&ArgStr[2], MModImm, &Result) == ModImm)
      {
        BAsmCode[2] = Result.Vals[0];
        if (DecodeAdr(&ArgStr[3], MModImm, &Result) == ModImm)
        {
          BAsmCode[3] = Result.Vals[0];
          CodeLen = 4;
          BAsmCode[0] = Code;
          if (Code & 0x10)
          {
            BAdr = BAsmCode[2];
            BAsmCode[2] = BAsmCode[3];
            BAsmCode[3] = BAdr;
          }
        }
      }
    }
  }
}

static void DecodeJMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    tAdrResult Result;

    if (ArgCnt == 1)
      cond_code = TrueCond;
    else if (!DecodeCondition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    switch (DecodeAdr(&ArgStr[ArgCnt], MModAbs | MModLAbs | MModIReg | M_InCode, &Result))
    {
      case ModLAbs:
        if (cond_code != COND_CODE_TRUE) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        else
        {
          CodeLen = 2 + Result.Cnt;
          BAsmCode[0] = 0xfa;
          BAsmCode[1] = Result.Mode;
          memcpy(BAsmCode + 2, Result.Vals, Result.Cnt);
        }
        break;
      case ModAbs:
      {
        LongInt AdrDist = WordVal(&Result) - (EProgCounter() + 2);
        Boolean IsShort = (AdrDist <= 254) && (AdrDist >= -256) && ((AdrDist & 1) == 0);

        if (!Result.ForceSize)
          Result.ForceSize = IsShort ? eForceShort : eForceLong;
        if (Result.ForceSize == eForceShort)
        {
          if (!IsShort && !mSymbolQuestionable(Result.SymFlags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[ArgCnt]);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0x0d + (cond_code << 4);
            BAsmCode[1] = (AdrDist / 2) & 0xff;
          }
        }
        else
        {
          CodeLen = 2 + Result.Cnt;
          BAsmCode[0] = 0xea;
          BAsmCode[1] = cond_code << 4;
          memcpy(BAsmCode + 2, Result.Vals, Result.Cnt);
        }
        break;
      }
      case ModIReg:
        CodeLen = 2; BAsmCode[0] = 0x9c;
        BAsmCode[1] = (cond_code << 4) + Result.Mode;
        break;
    }
  }
}

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    tAdrResult Result;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCondition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    switch (DecodeAdr(&ArgStr[ArgCnt], MModAbs | MModLAbs | MModIReg | M_InCode, &Result))
    {
      case ModLAbs:
        if (cond_code != COND_CODE_TRUE) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
        else
        {
          CodeLen = 2 + Result.Cnt;
          BAsmCode[0] = 0xda;
          BAsmCode[1] = Result.Mode;
          memcpy(BAsmCode + 2, Result.Vals, Result.Cnt);
        }
        break;
      case ModAbs:
      {
        LongInt AdrLong = WordVal(&Result) - (EProgCounter() + 2);
        Boolean IsShort = (AdrLong <= 254) && (AdrLong >= -256) && ((AdrLong & 1) == 0);

        if (!Result.ForceSize && (cond_code == COND_CODE_TRUE))
          Result.ForceSize = IsShort ? eForceShort : eForceLong;
        if (Result.ForceSize == eForceShort)
        {
          if (!IsShort && !mSymbolQuestionable(Result.SymFlags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[ArgCnt]);
          else
          {
            CodeLen = 2;
            BAsmCode[0] = 0xbb;
            BAsmCode[1] = (AdrLong / 2) & 0xff;
          }
        }
        else
        {
          CodeLen = 2 + Result.Cnt;
          BAsmCode[0] = 0xca;
          BAsmCode[1] = 0x00 + (cond_code << 4);
          memcpy(BAsmCode + 2, Result.Vals, Result.Cnt);
        }
        break;
      }
      case ModIReg:
        CodeLen = 2;
        BAsmCode[0] = 0xab;
        BAsmCode[1] = (cond_code << 4) + Result.Mode;
        break;
    }
  }
}

static void DecodeJMPR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCondition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], MemInt, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (AdrLong & 1) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((AdrLong > 254) || (AdrLong < -256))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0x0d + (cond_code << 4);
        BAsmCode[1] = (AdrLong / 2) & 0xff;
      }
    }
  }
}

static void DecodeCALLR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], MemInt, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (AdrLong & 1) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((AdrLong > 254) || (AdrLong < -256))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = 0xbb;
        BAsmCode[1] = (AdrLong / 2) & 0xff;
      }
    }
  }
}

static void DecodeJMPA_CALLA(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    Boolean OK;
    LongInt AdrLong;
    tSymbolFlags Flags;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCondition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], MemInt, &OK, &Flags);
    if (OK && ChkSamePage(AdrLong, EProgCounter(), 16, Flags))
    {
      CodeLen = 4;
      BAsmCode[0] = Code;
      BAsmCode[1] = 0x00 + (cond_code << 4);
      BAsmCode[2] = Lo(AdrLong);
      BAsmCode[3] = Hi(AdrLong);
    }
  }
}

static void DecodeJMPS_CALLS(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    Boolean OK;
    Word AdrWord;
    Byte AdrBank;
    LongInt AdrLong;

    if (ArgCnt == 1)
    {
      AdrLong = EvalStrIntExpression(&ArgStr[1], MemInt, &OK);
      AdrWord = AdrLong & 0xffff;
      AdrBank = AdrLong >> 16;
    }
    else
    {
      AdrWord = EvalStrIntExpression(&ArgStr[2], UInt16, &OK);
      AdrBank = OK ? EvalStrIntExpression(&ArgStr[1], MemInt2, &OK) : 0;
    }
    if (OK)
    {
      CodeLen = 4;
      BAsmCode[0] = Code;
      BAsmCode[1] = AdrBank;
      BAsmCode[2] = Lo(AdrWord);
      BAsmCode[3] = Hi(AdrWord);
    }
  }
}

static void DecodeJMPI_CALLI(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    Byte cond_code;
    tAdrResult Result;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCondition(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    switch (DecodeAdr(&ArgStr[ArgCnt], MModIReg | M_InCode, &Result))
    {
      case ModIReg:
        CodeLen = 2;
        BAsmCode[0] = Code;
        BAsmCode[1] = Result.Mode + (cond_code << 4);
        break;
    }
  }
}

static void DecodeBJmp(Word Code)
{
  Byte BOfs;
  Word BAdr;

  if (ChkArgCnt(2, 2)
   && DecodeBitAddr(&ArgStr[1], &BAdr, &BOfs, False))
  {
    Boolean OK;
    tSymbolFlags Flags;
    LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[2], MemInt, &OK, &Flags) - (EProgCounter() + 4);
    if (OK)
    {
      if (AdrLong & 1) WrError(ErrNum_DistIsOdd);
      else if (!mSymbolQuestionable(Flags) && ((AdrLong < -256) || (AdrLong > 254))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 4; BAsmCode[0] = 0x8a + (Code << 4);
        BAsmCode[1] = BAdr;
        BAsmCode[2] = (AdrLong / 2) & 0xff;
        BAsmCode[3] = BOfs << 4;
      }
    }
  }
}

static void DecodePCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModMReg, &Result))
    {
      case ModMReg:
        BAsmCode[1] = Result.Vals[0];
        switch (DecodeAdr(&ArgStr[2], MModAbs | M_InCode, &Result))
        {
          case ModAbs:
            CodeLen = 4;
            BAsmCode[0] = 0xe2;
            memcpy(BAsmCode + 2, Result.Vals, 2);
            break;
        }
        break;
    }
  }
}

static void DecodeRETP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModMReg, &Result))
    {
      case ModMReg:
        BAsmCode[1] = Result.Vals[0];
        BAsmCode[0] = 0xeb;
        CodeLen = 2;
        if (SPChanged)
          WrXError(ErrNum_Pipeline, RegNames[5]);
        break;
    }
  }
}

static void DecodeTRAP(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt7, &OK) << 1;
    if (OK)
    {
      BAsmCode[0] = 0x9b;
      CodeLen = 2;
    }
  }
}

static void DecodeATOMIC(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80C167)
   && DecodePref(&ArgStr[1], &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0xd1;
    BAsmCode[1] = HReg << 4;
  }
}

static void DecodeEXTR(Word Code)
{
  Byte HReg;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU80C167)
   && DecodePref(&ArgStr[1], &HReg))
  {
    CodeLen = 2;
    BAsmCode[0] = 0xd1;
    BAsmCode[1] = 0x80 + (HReg << 4);
    ExtCounter = HReg + 1;
    ExtSFRs = True;
  }
}

static void DecodeEXTP_EXTPR(Word Code)
{
  Byte HReg;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80C167)
   && DecodePref(&ArgStr[2], &HReg))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg | MModImm, &Result))
    {
      case ModReg:
        CodeLen = 2;
        BAsmCode[0] = 0xdc;
        BAsmCode[1] = Code + 0x40 + (HReg << 4) + Result.Mode;
        ExtCounter = HReg + 1;
        MemMode = MemModeZeroPage;
        break;
      case ModImm:
        CodeLen = 4;
        BAsmCode[0] = 0xd7;
        BAsmCode[1] = Code + 0x40 + (HReg << 4);
        BAsmCode[2] = WordVal(&Result) & 0xff;
        BAsmCode[3] = (WordVal(&Result) >> 8) & 3;
        ExtCounter = HReg + 1;
        MemMode = MemModeFixedPage;
        MemPage = WordVal(&Result) & 0x3ff;
        break;
    }
  }
}

static void DecodeEXTS_EXTSR(Word Code)
{
  Byte HReg;

  OpSize = eSymbolSize8Bit;
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU80C167)
   && DecodePref(&ArgStr[2], &HReg))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg | MModImm, &Result))
    {
      case ModReg:
        CodeLen = 2;
        BAsmCode[0] = 0xdc;
        BAsmCode[1] = Code + 0x00 + (HReg << 4) + Result.Mode;
        ExtCounter = HReg + 1;
        MemMode = MemModeNoCheck;
        break;
      case ModImm:
        CodeLen = 4;
        BAsmCode[0] = 0xd7;
        BAsmCode[1] = Code + 0x00 + (HReg << 4);
        BAsmCode[2] = Result.Vals[0];
        BAsmCode[3] = 0;
        ExtCounter = HReg + 1;
        MemMode = MemModeFixedBank;
        MemPage = Result.Vals[0];
        break;
    }
  }
}

static void DecodeBIT(Word Code)
{
  Word Adr;
  Byte Bit;

  UNUSED(Code);

 if (ChkArgCnt(1, 1)
  && DecodeBitAddr(&ArgStr[1], &Adr, &Bit, True))
 {
   PushLocHandle(-1);
   EnterIntSymbol(&LabPart, (Adr << 4) + Bit, SegNone, False);
   PopLocHandle();
   as_snprintf(ListLine, STRINGSIZE, "=%02xH.%1x", (unsigned)Adr, (unsigned)Bit);
 }
}

/*-------------------------------------------------------------------------*/

static void AddBInstTable(const char *NName, Word NCode, InstProc Proc)
{
  char BName[30];

  AddInstTable(InstTable, NName, NCode | (eSymbolSize16Bit << 8), Proc);
  as_snprintf(BName, sizeof(BName), "%sB", NName);
  AddInstTable(InstTable, BName, NCode | (eSymbolSize8Bit << 8), Proc);
}

static void AddFixed(const char *NName, CPUVar NMin, Word NCode1, Word NCode2)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].MinCPU = NMin;
  FixedOrders[InstrZ].Code1 = NCode1;
  FixedOrders[InstrZ].Code2 = NCode2;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddShift(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeShift);
}

static void AddBit2(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit2);
}

static void AddLoop(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLoop);
}

static void AddCondition(const char *NName, Byte NCode)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);
  AddBInstTable("MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVBS", 0x10, DecodeMOVBS_MOVBZ);
  AddInstTable(InstTable, "MOVBZ", 0x00, DecodeMOVBS_MOVBZ);
  AddInstTable(InstTable, "PUSH", 0xec, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 0xfc, DecodePUSH_POP);
  AddInstTable(InstTable, "SCXT", 0, DecodeSCXT);
  AddBInstTable("CPL", 0x91, DecodeCPL_NEG);
  AddBInstTable("NEG", 0x81, DecodeCPL_NEG);
  AddInstTable(InstTable, "BCLR", 0x0e, DecodeBCLR_BSET);
  AddInstTable(InstTable, "BSET", 0x0f, DecodeBCLR_BSET);
  AddInstTable(InstTable, "BFLDL", 0x0a, DecodeBFLDH_BFLDL);
  AddInstTable(InstTable, "BFLDH", 0x1a, DecodeBFLDH_BFLDL);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "JMPR", 0, DecodeJMPR);
  AddInstTable(InstTable, "CALLR", 0, DecodeCALLR);
  AddInstTable(InstTable, "JMPA", 0xea, DecodeJMPA_CALLA);
  AddInstTable(InstTable, "CALLA", 0xca, DecodeJMPA_CALLA);
  AddInstTable(InstTable, "JMPS", 0xfa, DecodeJMPS_CALLS);
  AddInstTable(InstTable, "CALLS", 0xda, DecodeJMPS_CALLS);
  AddInstTable(InstTable, "JMPI", 0x9c, DecodeJMPI_CALLI);
  AddInstTable(InstTable, "CALLI", 0xab, DecodeJMPI_CALLI);
  AddInstTable(InstTable, "PCALL", 0, DecodePCALL);
  AddInstTable(InstTable, "RETP", 0, DecodeRETP);
  AddInstTable(InstTable, "TRAP", 0, DecodeTRAP);
  AddInstTable(InstTable, "ATOMIC", 0, DecodeATOMIC);
  AddInstTable(InstTable, "EXTR", 0, DecodeEXTR);
  AddInstTable(InstTable, "EXTP", 0x00, DecodeEXTP_EXTPR);
  AddInstTable(InstTable, "EXTPR", 0x80, DecodeEXTP_EXTPR);
  AddInstTable(InstTable, "EXTS", 0x00, DecodeEXTS_EXTSR);
  AddInstTable(InstTable, "EXTSR", 0x80, DecodeEXTS_EXTSR);

  InstrZ = 0;
  AddFixed("DISWDT", CPU80C166, 0x5aa5, 0xa5a5);
  AddFixed("EINIT" , CPU80C166, 0x4ab5, 0xb5b5);
  AddFixed("IDLE"  , CPU80C166, 0x7887, 0x8787);
  AddFixed("NOP"   , CPU80C166, 0x00cc, 0x0000);
  AddFixed("PWRDN" , CPU80C166, 0x6897, 0x9797);
  AddFixed("RET"   , CPU80C166, 0x00cb, 0x0000);
  AddFixed("RETI"  , CPU80C166, 0x88fb, 0x0000);
  AddFixed("RETS"  , CPU80C166, 0x00db, 0x0000);
  AddFixed("SRST"  , CPU80C166, 0x48b7, 0xb7b7);
  AddFixed("SRVWDT", CPU80C166, 0x58a7, 0xa7a7);

  InstrZ = 0;
  AddCondition("UC" , COND_CODE_TRUE); AddCondition("Z"  , 0x2);
  AddCondition("NZ" , 0x3); AddCondition("V"  , 0x4);
  AddCondition("NV" , 0x5); AddCondition("N"  , 0x6);
  AddCondition("NN" , 0x7); AddCondition("C"  , 0x8);
  AddCondition("NC" , 0x9); AddCondition("EQ" , 0x2);
  AddCondition("NE" , 0x3); AddCondition("ULT", 0x8);
  AddCondition("ULE", 0xf); AddCondition("UGE", 0x9);
  AddCondition("UGT", 0xe); AddCondition("SLT", 0xc);
  AddCondition("SLE", 0xb); AddCondition("SGE", 0xd);
  AddCondition("SGT", 0xa); AddCondition("NET", 0x1);
  AddCondition(NULL, 0);

  InstrZ = 0;
  AddBInstTable("ADD" , InstrZ++, DecodeALU2);
  AddBInstTable("ADDC", InstrZ++, DecodeALU2);
  AddBInstTable("SUB" , InstrZ++, DecodeALU2);
  AddBInstTable("SUBC", InstrZ++, DecodeALU2);
  AddBInstTable("CMP" , InstrZ++, DecodeALU2);
  AddBInstTable("XOR" , InstrZ++, DecodeALU2);
  AddBInstTable("AND" , InstrZ++, DecodeALU2);
  AddBInstTable("OR"  , InstrZ++, DecodeALU2);

  AddShift("ASHR", 0xac); AddShift("ROL" , 0x0c);
  AddShift("ROR" , 0x2c); AddShift("SHL" , 0x4c);
  AddShift("SHR" , 0x6c);

  AddBit2("BAND", 0x6a); AddBit2("BCMP" , 0x2a);
  AddBit2("BMOV", 0x4a); AddBit2("BMOVN", 0x3a);
  AddBit2("BOR" , 0x5a); AddBit2("BXOR" , 0x7a);

  AddLoop("CMPD1", 0xa0); AddLoop("CMPD2", 0xb0);
  AddLoop("CMPI1", 0x80); AddLoop("CMPI2", 0x90);

  InstrZ = 0;
  AddInstTable(InstTable, "DIV"  , InstrZ++, DecodeDiv);
  AddInstTable(InstTable, "DIVU" , InstrZ++, DecodeDiv);
  AddInstTable(InstTable, "DIVL" , InstrZ++, DecodeDiv);
  AddInstTable(InstTable, "DIVLU", InstrZ++, DecodeDiv);

  InstrZ = 0;
  AddInstTable(InstTable, "JB"   , InstrZ++, DecodeBJmp);
  AddInstTable(InstTable, "JNB"  , InstrZ++, DecodeBJmp);
  AddInstTable(InstTable, "JBC"  , InstrZ++, DecodeBJmp);
  AddInstTable(InstTable, "JNBS" , InstrZ++, DecodeBJmp);

  InstrZ = 0;
  AddInstTable(InstTable, "MUL"  , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "MULU" , InstrZ++, DecodeMul);
  AddInstTable(InstTable, "PRIOR", InstrZ++, DecodeMul);

  AddInstTable(InstTable, "BIT" , 0, DecodeBIT);
  AddInstTable(InstTable, "REG" , 0, CodeREG);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(Conditions);
}

static void MakeCode_166(void)
{
  int z;

  CodeLen = 0;
  DontPrint = False;
  OpSize = eSymbolSize16Bit;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  /* Pipeline-Flags weiterschalten */

  SPChanged = N_SPChanged; N_SPChanged = False;
  CPChanged = N_CPChanged; N_CPChanged = False;
  for (z = 0; z < DPPCount; z++)
  {
    DPPChanged[z] = N_DPPChanged[z];
    N_DPPChanged[z] = False;
  }

  /* Praefixe herunterzaehlen */

  if (ExtCounter >= 0)
   if (--ExtCounter < 0)
   {
     MemMode = MemModeStd;
     ExtSFRs = False;
   }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_166(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on C16x
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_166(char *pArg, TempResult *pResult)
{
  tRegInt Erg;
  tSymbolSize Size;

  if (IsRegCore(pArg, &Erg, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Erg;
    pResult->Contents.RegDescr.Dissect = DissectReg_166;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void InitCode_166(void)
{
  int z;

  for (z = 0; z < DPPCount; z++)
  {
    DPPAssumes[z] = z;
    N_DPPChanged[z] = False;
  }
  N_CPChanged = False;
  N_SPChanged = False;

  MemMode = MemModeStd;
  ExtSFRs = False;
  ExtCounter = (-1);
}

static Boolean IsDef_166(void)
{
  return (Memo("BIT")) || (Memo("REG"));
}

static void SwitchFrom_166(void)
{
  DeinitFields();
}

static void SwitchTo_166(void)
{
  Byte z;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  OpSize = eSymbolSize16Bit;

  PCSymbol = "$";
  HeaderID = 0x4c;
  NOPCode = 0xcc00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;

  MakeCode = MakeCode_166;
  IsDef = IsDef_166;
  InternSymbol = InternSymbol_166;
  DissectReg = DissectReg_166;
  SwitchFrom = SwitchFrom_166;

  if (MomCPU == CPU80C166)
  {
    MemInt = UInt18;
    MemInt2 = UInt2;
    ASSUME166s[0].Max = 15;
    SegLimits[SegCode] = 0x3ffffl;
  }
  else
  {
    MemInt = UInt24;
    MemInt2 = UInt8;
    ASSUME166s[0].Max = 1023;
    SegLimits[SegCode] = 0xffffffl;
  }
  for (z = 1; z < 4; z++)
    ASSUME166s[z].Max = ASSUME166s[0].Max;

  pASSUMERecs = ASSUME166s;
  ASSUMERecCnt = ASSUME166Count;

  InitFields();
}

void code166_init(void)
{
  CPU80C166 = AddCPU("80C166", SwitchTo_166);
  CPU80C167 = AddCPU("80C167", SwitchTo_166);
  CPU80C167CS = AddCPU("80C167CS", SwitchTo_166);

  AddInitPassProc(InitCode_166);
}
