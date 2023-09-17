/* codez8.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Zilog Z8                                                    */
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
#include "asmstructs.h"
#include "asmitree.h"
#include "asmallg.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "codez8.h"

typedef enum
{
  eCoreNone = 0,
  eCoreZ8NMOS = 1 << 0,
  eCoreZ8CMOS = 1 << 1,
  eCoreSuper8 = 1 << 2,
  eCoreZ8Encore = 1 << 3,
  eCoreAll = eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore
} tCoreFlags;

typedef struct
{
  const char *pName;
  Word Code;
  tCoreFlags CoreFlags;
} BaseOrder;

typedef struct
{
  Word Code;
  tCoreFlags CoreFlags;
  Boolean Is16;
} ALU1Order;

typedef struct
{
  const char *Name;
  Byte Code;
} Condition;

#ifdef __cplusplus
# include "codez8.hpp"
#endif

typedef struct
{
  const char *pName;
  tCoreFlags CoreFlags;
  Word WorkOfs;
  Word RAMEnd, SFRStart;
} tCPUProps;

#define LongWorkOfs 0xee0   /* ditto with 12-bit-addresses */

#define EXTPREFIX 0x1f

#define mIsSuper8() (pCurrCPUProps->CoreFlags & eCoreSuper8)
#define mIsZ8Encore() (pCurrCPUProps->CoreFlags & eCoreZ8Encore)

/* CAUTION: ModIReg and ModIRReg are mutually exclusive
            ModReg  and ModRReg  are mutually exclusive */

enum
{
  ModNone = -1,
  ModWReg = 0,      /* working register R0..R15, 'r' */
  ModReg = 1,       /* general register 'R' */
  ModRReg = 2,      /* general register pair 'RR' (must be even) */
  ModIWReg = 3,     /* indirect working register @R0...@R15 'Ir' */
  ModIReg = 4,      /* indirect general register 'IR' */
  ModImm = 5,       /* immediate value 'IM' */
  ModWRReg = 6,     /* working register pair 'rr' (must be even) */
  ModIWRReg = 7,    /* indirect working register pair 'Irr' (must be even) */
  ModIRReg = 8,     /* indirect general register pair 'IRR' (must be even) */
  ModInd = 9,
  ModXReg = 10,
  ModIndRR = 11,
  ModIndRR16 = 12,
  ModWeird = 13,
  ModDA = 14
};

#define MModWReg   (1 << ModWReg)
#define MModReg    (1 << ModReg)
#define MModRReg   (1 << ModRReg)
#define MModIWReg  (1 << ModIWReg)
#define MModIReg   (1 << ModIReg)
#define MModImm    (1 << ModImm)
#define MModWRReg  (1 << ModWRReg)
#define MModIWRReg (1 << ModIWRReg)
#define MModIRReg  (1 << ModIRReg)
#define MModInd    (1 << ModInd)
#define MModXReg   (1 << ModXReg)
#define MModIndRR  (1 << ModIndRR)
#define MModIndRR16  (1 << ModIndRR16)
#define MModWeird  (1 << ModWeird)
#define MModDA     (1 << ModDA)

static ShortInt AdrType, OpSize;
static Byte AdrVal;
static Word AdrWVal;
static LongInt AdrIndex;

static BaseOrder *FixedOrders;
static BaseOrder *ALU2Orders;
static BaseOrder *ALUXOrders;
static ALU1Order *ALU1Orders;
static Condition *Conditions;

static int CondCnt, TrueCond;

static const tCPUProps *pCurrCPUProps;

static LongInt RPVal, RP0Val, RP1Val;
static IntType RegSpaceType;

/*--------------------------------------------------------------------------*/
/* address expression decoding routines */

/*!------------------------------------------------------------------------
 * \fn     IsWRegCore(const char *pArg, Byte *pResult)
 * \brief  Is argument a working register? (Rn, n=0..15)
 * \param  pArg argument
 * \param  pResult resulting register number if it is
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean IsWRegCore(const char *pArg, Byte *pResult)
{
  if ((strlen(pArg) < 2) || (as_toupper(*pArg) != 'R')) return False;
  else
  {
    Boolean OK;

    *pResult = ConstLongInt(pArg + 1, &OK, 10);
    return OK && (*pResult <= 15);
  }
}

/*!------------------------------------------------------------------------
 * \fn     IsWReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
 * \brief  Is argument a working register (Rn, n=0..15) or register alias?
 * \param  pArg argument
 * \param  pResult resulting register number if it is
 * \param  MustBeReg expecting register?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static Boolean IsWReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsWRegCore(pArg->str.p_str, pResult))
    return True;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize8Bit, MustBeReg);
  *pResult = RegDescr.Reg;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     IsWRRegCore(const char *pArg, Byte *pResult)
 * \brief  Is argument a working register pair? (RRn, n=0..15)
 * \param  pArg argument
 * \param  pResult resulting value if it is
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean IsWRRegCore(const char *pArg, Byte *pResult)
{
  if ((strlen(pArg) < 3) || as_strncasecmp(pArg, "RR", 2)) return False;
  else
  {
    Boolean OK;

    *pResult = ConstLongInt(pArg + 2, &OK, 10);
    return OK && (*pResult <= 15);
  }
}

#if 0
/*!------------------------------------------------------------------------
 * \fn     IsWRReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
 * \brief  Is argument a working register pair (RRn, n=0..15) or register pair alias?
 * \param  pArg argument
 * \param  pResult resulting value if it is
 * \param  MustBeReg expecting register?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static Boolean IsWRReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsWRRegCore(pArg->str.p_str, pResult))
    return True;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize16Bit, MustBeReg);
  *pResult = RegDescr.Reg;
  return RegEvalResult;
}
#endif

/*!------------------------------------------------------------------------
 * \fn     IsWRegOrWRReg(const tStrComp *pArg, Byte *pResult, tSymbolSize *pSize, Boolean MustBeReg)
 * \brief  Is argument a working register (pair) ((R)Rn, n=0..15) or register (pair) alias?
 * \param  pArg argument
 * \param  pResult resulting value if it is
 * \param  pSize register size if it is
 * \param  MustBeReg expecting register?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult IsWRegOrWRReg(const tStrComp *pArg, Byte *pResult, tSymbolSize *pSize, Boolean MustBeReg)
{
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsWRegCore(pArg->str.p_str, pResult))
  {
    EvalResult.DataSize = eSymbolSize8Bit;
    RegEvalResult = eIsReg;
  }
  else if (IsWRRegCore(pArg->str.p_str, pResult))
  {
    EvalResult.DataSize = eSymbolSize16Bit;
    RegEvalResult = eIsReg;
  }
  else
  {
    tRegDescr RegDescr;

    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
    *pResult = RegDescr.Reg;
  }

  if ((eIsReg == RegEvalResult) && (EvalResult.DataSize == eSymbolSize16Bit) && (*pResult & 1))
  {
    WrStrErrorPos(ErrNum_AddrMustBeEven, pArg);
    RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;
  }

  *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_Z8(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - Z8 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_Z8(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    case eSymbolSize16Bit:
      as_snprintf(pDest, DestSize, "RR%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     CorrMode8(Word Mask, ShortInt Old, ShortInt New)
 * \brief  upgrade from working reg mode to gen. reg mode if necessary & possible?
 * \param  Mask bit mask of allowed addressing modes
 * \param  Old currently selected (working reg) mode
 * \param  New possible new mode
 * \return True if converted
 * ------------------------------------------------------------------------ */

static Boolean CorrMode8(Word Mask, ShortInt Old, ShortInt New)
{
   if ((AdrType == Old) && ((Mask & (1 << Old)) == 0) && ((Mask & (1 << New)) != 0))
   {
     AdrType = New;
     AdrVal += pCurrCPUProps->WorkOfs;
     return True;
   }
   else
     return False;
}

/*!------------------------------------------------------------------------
 * \fn     Boolean CorrMode12(Word Mask, ShortInt Old, ShortInt New)
 * \brief  upgrade from working reg mode to ext. reg (12 bit) mode if necessary & possible?
 * \param  Mask bit mask of allowed addressing modes
 * \param  Old currently selected (working reg) mode
 * \param  New possible new mode
 * \return True if converted
 * ------------------------------------------------------------------------ */

static Boolean CorrMode12(Word Mask, ShortInt Old, ShortInt New)
{
   if ((AdrType == Old) && ((Mask & (1 << Old)) == 0) && ((Mask & (1 << New)) != 0))
   {
     AdrType = New;
     AdrWVal = AdrVal + LongWorkOfs;
     return True;
   }
   else
     return False;
}

/*!------------------------------------------------------------------------
 * \fn     ChkAdr(Word Mask, const tStrComp *pArg)
 * \brief  check for validity of decoded addressing mode
 * \param  Mask bit mask of allowed addressing modes
 * \param  pArg original expression
 * \return true if OK
 * ------------------------------------------------------------------------ */

static Boolean ChkAdr(Word Mask, const tStrComp *pArg)
{
   CorrMode8(Mask, ModWReg, ModReg);
   CorrMode12(Mask, ModWReg, ModXReg);
   CorrMode8(Mask, ModIWReg, ModIReg);

   if ((AdrType != ModNone) && !(Mask & (1 << AdrType)))
   {
     WrStrErrorPos(ErrNum_InvAddrMode, pArg); AdrType = ModNone;
     return False;
   }
   return True;
}

/*!------------------------------------------------------------------------
 * \fn     IsWRegAddress(Word Address, Byte *pWorkReg)
 * \brief  check whether data address is accessible as work register
 * \param  Address data address in 8/12 bit data space
 * \param  pWorkReg resulting work register # if yes
 * \param  FirstPassUnknown flag about questionable value
 * \return true if accessible as work register
 * ------------------------------------------------------------------------ */

static Boolean ChkInRange(Word Address, Word Base, Word Length, Byte *pOffset)
{
  if ((Address >= Base) && (Address < Base + Length))
  {
    *pOffset = Address - Base;
    return True;
  }
  return False;
}

static Boolean IsWRegAddress(Word Address, Byte *pWorkReg)
{
  if (mIsSuper8())
  {
    if ((RP0Val <= 0xff) && ChkInRange(Address, RP0Val & 0xf8, 8, pWorkReg))
      return True;
    if ((RP1Val <= 0xff) && ChkInRange(Address, RP1Val & 0xf8, 8, pWorkReg))
    {
      *pWorkReg += 8;
      return True;
    }
  }
  else if (mIsZ8Encore())
  {
    if ((RPVal <= 0xff) && ChkInRange(Address, (RPVal & 0xf0) | ((RPVal & 0x0f) << 8), 16, pWorkReg))
      return True;
  }
  else
  {
    if ((RPVal <= 0xff) && ChkInRange(Address, RPVal & 0xf0, 16, pWorkReg))
      return True;
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     IsRegAddress(Word Address)
 * \brief  check whether data address is accessible via 8-bit address
 * \param  Address data address in 8/12 bit data space
 * \return true if accessible via 8-bit address
 * ------------------------------------------------------------------------ */

static Boolean IsRegAddress(Word Address)
{
  /* simple Z8 does not support 12 bit register addresses, so
     always force this to TRUE for it */

  if (!(pCurrCPUProps->CoreFlags & eCoreZ8Encore))
    return TRUE;
  return ((RPVal <= 0xff)
       && (Hi(Address) == (RPVal & 15)));
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, Word Mask)
 * \brief  decode address expression
 * \param  pArg expression in source code
 * \param  Mask bit mask of allowed modes
 * \return True if successfully decoded to an allowed mode
 * ------------------------------------------------------------------------ */

int GetForceLen(const char *pArg)
{
  int Result = 0;

  while ((Result < 2) && (pArg[Result] == '>'))
    Result++;
  return Result;
}

static ShortInt IsWRegWithRP(const tStrComp *pComp, Byte *pResult, Word Mask16Modes, Word Mask8Modes)
{
  tEvalResult EvalResult;
  Word Address;
  tSymbolSize Size;

  switch (IsWRegOrWRReg(pComp, pResult, &Size, False))
  {
    case eIsReg:
      return Size;
    case eIsNoReg:
      break;
    case eRegAbort:
      return eSymbolSizeUnknown;
  }

  /* It's neither Rn nor RRn.  Since an address by itself has no
     operand size, only one mode may be allowed to keep things
     unambiguous: */

  if (Mask16Modes && Mask8Modes)
  {
    WrStrErrorPos(ErrNum_InvReg, pComp);
    return eSymbolSizeUnknown;
  }

  Address = EvalStrIntExpressionWithResult(pComp, UInt8, &EvalResult);
  if (!EvalResult.OK)
    return eSymbolSizeUnknown;
  /* if (mFirstPassUnknown(EvalResult.Flags)) ... */

  if (Mask16Modes && IsWRegAddress(Address, pResult))
  {
    if (mFirstPassUnknown(EvalResult.Flags)) *pResult &= ~1;
    if (*pResult & 1)
    {
      WrStrErrorPos(ErrNum_AddrMustBeEven, pComp);
      return eSymbolSizeUnknown;
    }
    return eSymbolSize16Bit;
  }

  if (Mask8Modes && IsWRegAddress(Address, pResult))
    return eSymbolSize8Bit;

  WrStrErrorPos(ErrNum_InvReg, pComp);
  return eSymbolSizeUnknown;
}

static Boolean DecodeAdr(const tStrComp *pArg, Word Mask)
{
  Boolean OK;
  tEvalResult EvalResult;
  char  *p;
  int ForceLen, l;
  tSymbolSize Size;

  if (!mIsSuper8() && !mIsZ8Encore())
    Mask &= ~MModIndRR;
  if (!mIsSuper8())
    Mask &= ~MModIndRR16;
  if (!mIsZ8Encore())
    Mask &= ~(MModXReg | MModWeird);

  AdrType = ModNone;

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case eSymbolSize8Bit:
        AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        break;
      case eSymbolSize16Bit:
        AdrWVal = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        break;
      default:
        OK = False;
    }
    if (OK) AdrType = ModImm;
    return ChkAdr(Mask, pArg);
  }

  /* Register ? */

  switch (IsWRegOrWRReg(pArg, &AdrVal, &Size, False))
  {
    case eIsReg:
      AdrType = (Size == eSymbolSize16Bit) ? ModWRReg : ModWReg;
      return ChkAdr(Mask, pArg);
    case eIsNoReg:
      break;
    case eRegAbort:
      return False;
  }

  /* treat absolute address as register? */

  if (*pArg->str.p_str == '!')
  {
    AdrWVal = EvalStrIntExpressionOffsWithResult(pArg, 1, UInt16, &EvalResult);
    if (EvalResult.OK)
    {
      if (!mFirstPassUnknown(EvalResult.Flags) && !IsWRegAddress(AdrWVal, &AdrVal))
        WrError(ErrNum_InAccPage);
      AdrType = ModWReg;
      return ChkAdr(Mask, pArg);
    }
    return False;
  }

  /* indirekte Konstrukte ? */

  if (*pArg->str.p_str == '@')
  {
    tStrComp Comp;
    tRegEvalResult RegEvalResult;

    StrCompRefRight(&Comp, pArg, 1);
    if ((strlen(Comp.str.p_str) >= 6) && (!as_strncasecmp(Comp.str.p_str, ".RR", 3)) && (IsIndirect(Comp.str.p_str + 3)))
    {
      AdrVal = EvalStrIntExpressionOffsWithResult(&Comp, 3, Int8, &EvalResult);
      if (EvalResult.OK)
      {
        AdrType = ModWeird;
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
      }
    }
    else if ((RegEvalResult = IsWRegOrWRReg(&Comp, &AdrVal, &Size, False)) != eIsNoReg)
    {
      if (RegEvalResult == eRegAbort)
        return False;
      AdrType = (Size == eSymbolSize16Bit) ? ModIWRReg : ModIWReg;
    }
    else
    {
      /* Trying to do a working register optimization at this place is
         extremely tricky since an expression like @<address> has no
         inherent operand size (8/16 bit).  So the optimization IRR->Irr
         will only be allowed if IR is not allowed, or Irr is the only
         mode allowed: */

      Word ModeMask = Mask & (MModIRReg | MModIWRReg | MModIReg | MModIWReg);

      if (ModeMask == (MModIWReg | MModIWRReg))
      {
        WrStrErrorPos(ErrNum_UndefRegSize, &Comp);
        return False;
      }

      AdrWVal = EvalStrIntExpressionOffsWithResult(&Comp, ForceLen = GetForceLen(pArg->str.p_str), Int8, &EvalResult);
      if (EvalResult.OK)
      {
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        if (!(ModeMask & MModIReg) || (ModeMask == MModIWRReg))
        {
          if (mFirstPassUnknown(EvalResult.Flags)) AdrWVal &= ~1;
          if (AdrWVal & 1) WrStrErrorPos(ErrNum_AddrMustBeEven, &Comp);
          else if ((Mask & MModIWRReg) && (ForceLen <= 0) && IsWRegAddress(AdrWVal, &AdrVal))
            AdrType = ModIWRReg;
          else
          {
            AdrVal = AdrWVal;
            AdrType = ModIRReg;
          }
        }
        else
        {
          if ((Mask & MModIWReg) && (ForceLen <= 0) && IsWRegAddress(AdrWVal, &AdrVal))
            AdrType = ModIWReg;
          else
          {
            AdrVal = AdrWVal;
            AdrType = ModIReg;
          }
        }
      }
    }
    return ChkAdr(Mask, pArg);
  }

  /* indiziert ? */

  l = strlen(pArg->str.p_str);
  if ((l > 4) && (pArg->str.p_str[l - 1] == ')'))
  {
    tStrComp Left, Right;

    StrCompRefRight(&Right, pArg, 0);
    StrCompShorten(&Right, 1);
    p = RQuotPos(pArg->str.p_str, '(');
    if (!p)
    {
      WrStrErrorPos(ErrNum_BrackErr, pArg);
      return False;
    }
    StrCompSplitRef(&Left, &Right, &Right, p);

    switch (IsWRegWithRP(&Right, &AdrVal, Mask & (MModIndRR | MModIndRR16), Mask & MModInd))
    {
      case eSymbolSize8Bit:
        /* We are operating on a single base register and therefore in a 8-bit address space.
           So we may allow both a signed or unsigned displacements since addresses will wrap
           around anyway: */

        AdrIndex = EvalStrIntExpressionWithResult(&Left, Int8, &EvalResult);
        if (EvalResult.OK)
        {
          AdrType = ModInd; ChkSpace(SegData, EvalResult.AddrSpaceMask);
        }
        return ChkAdr(Mask, pArg);

      case eSymbolSize16Bit:
        /* 16 bit index only allowed if index register is not zero */
        AdrIndex = EvalStrIntExpressionWithResult(&Left, ((Mask & MModIndRR16) && (AdrVal != 0)) ? Int16 : SInt8, &EvalResult);
        if (EvalResult.OK)
        {
          if ((Mask & MModIndRR) && RangeCheck(AdrIndex, SInt8))
            AdrType = ModIndRR;
          else
            AdrType = ModIndRR16;
          /* TODO: differentiate LDC/LDE */
          ChkSpace(SegData, EvalResult.AddrSpaceMask);
        }
        return ChkAdr(Mask, pArg);

      default:
        return False;
    }
  }

  /* simple direct address ? */

  AdrWVal = EvalStrIntExpressionOffsWithResult(pArg, ForceLen = GetForceLen(pArg->str.p_str),
                                      (Mask & MModDA) ? UInt16 : RegSpaceType, &EvalResult);
  if (EvalResult.OK)
  {
    if (Mask & MModDA)
    {
      AdrType = ModDA;
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
    else
    {
      if (mFirstPassUnknown(EvalResult.Flags) && !(Mask & ModXReg))
        AdrWVal = Lo(AdrWVal) | ((RPVal & 15) << 8);
      if (IsWRegAddress(AdrWVal, &AdrVal) && (Mask & MModWReg) && (ForceLen <= 0))
      {
        AdrType = ModWReg;
      }
      else if (IsRegAddress(AdrWVal) && (Mask & (MModReg | MModRReg)) && (ForceLen <= 1))
      {
        if (Mask & MModRReg)
        {
          if (mFirstPassUnknown(EvalResult.Flags))
            AdrWVal &= ~1;
          if (AdrWVal & 1)
          {
            WrStrErrorPos(ErrNum_AddrMustBeEven, pArg);
            return False;
          }
          AdrType = ModRReg;
        }
        else
          AdrType = ModReg;
        AdrVal = Lo(AdrWVal);
      }
      else
        AdrType = ModXReg;
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
    }
    return ChkAdr(Mask, pArg);
  }
  else
    return False;
}

static int DecodeCond(const tStrComp *pArg)
{
  int z;

  NLS_UpString(pArg->str.p_str);
  for (z = 0; z < CondCnt; z++)
    if (strcmp(Conditions[z].Name, pArg->str.p_str) == 0)
      break;

  if (z >= CondCnt)
    WrStrErrorPos(ErrNum_UndefCond, pArg);

  return z;
}

static Boolean ChkCoreFlags(tCoreFlags CoreFlags)
{
  if (pCurrCPUProps->CoreFlags & CoreFlags)
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  return False;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits and bit fields in symbol table:
 * bits 0..2: (start) bit position
 * bits 3...10/14: register address in SFR space (256B/4KB)
 */

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const char *pBitArg, Boolean *pOK, ShortInt OpSize)
 * \brief  evaluate constant bit position, with bit range depending on operand size
 * \param  pBitArg bit position argument
 * \param  pOK returns True if OK
 * \param  OpSize operand size (0 -> 8 bits)
 * \return bit position as number
 * ------------------------------------------------------------------------ */

static Byte EvalBitPosition(const tStrComp *pBitArg, Boolean *pOK, ShortInt OpSize)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return EvalStrIntExpressionOffs(pBitArg, !!(*pBitArg->str.p_str == '#'), UInt3, pOK);
    default:
      WrStrErrorPos(ErrNum_InvOpSize, pBitArg);
      *pOK = False;
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, ShortInt OpSize, Word Address)
 * \brief  build the compact internal representation of a bit field symbol
 * \param  BitPos bit position in word
 * \param  Width width of bit field
 * \param  OpSize operand size (0..2)
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, ShortInt OpSize, Word Address)
{
  UNUSED(OpSize);
  return BitPos
       | (((LongWord)Address & 0xfff) << 3);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, ShortInt OpSize)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \param  OpSize register size (0 = 8 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, ShortInt OpSize)
{
  Boolean OK;
  LongWord Addr;
  Byte BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK, OpSize);
  if (!OK)
    return False;

  /* all I/O registers reside in the first 256/4K byte of the address space */

  DecodeAdr(pRegArg, MModWReg | (mIsZ8Encore() ? MModXReg : MModReg));
  switch (AdrType)
  {
    case ModXReg:
      Addr = AdrWVal;
      break;
    case ModWReg:
      Addr = AdrVal + pCurrCPUProps->WorkOfs;
      break;
    case ModReg:
      Addr = AdrVal;
      break;
    default:
      return False;
  }

  *pResult = AssembleBitSymbol(BitPos, OpSize, Addr);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \param  OpSize register size (0 = 8 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
{
  *pResult = 0;

  /* Just one argument -> parse as bit argument */

  if (Start == Stop)
  {
    tEvalResult EvalResult;

    *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start],
                                    mIsZ8Encore() ? UInt15 : UInt11,
                                    &EvalResult);
    if (EvalResult.OK)
      ChkSpace(SegBData, EvalResult.AddrSpaceMask);
    return EvalResult.OK;
  }

  /* register & bit position are given as separate arguments */

  else if (Stop == Start + 1)
    return DecodeBitArg2(pResult, &ArgStr[Start], &ArgStr[Stop], OpSize);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ExpandZ8Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandZ8Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
{
  LongWord Address = Base + pStructElem->Offset;

  if (pInnermostNamedStruct)
  {
    PStructElem pElem = CloneStructElem(pVarName, pStructElem);

    if (!pElem)
      return;
    pElem->Offset = Address;
    AddStructElem(pInnermostNamedStruct->StructRec, pElem);
  }
  else
  {
    if (!ChkRange(Address, 0, 0x7ff)
     || !ChkRange(pStructElem->BitPos, 0, 7))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, eSymbolSize8Bit, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, ShortInt *pOpSize)
 * \brief  transform compact represenation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \param  pWidth pWidth width of bit field, always one for individual bit
 * \param  pOpSize returns register size (0 for 8 bits)
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, ShortInt *pOpSize)
{
  *pAddress = (BitSymbol >> 3) & 0xfff;
  *pBitPos = BitSymbol & 7;
  *pOpSize = eSymbolSize8Bit;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeWRBitArg(int StartArg, int EndArg, Byte *pResult)
 * \brief  decode bit argument in working register
 * \param  StartArg 1st argument
 * \param  EndArg last argument
 * \param  pResult resulting encoded bit
 * \return TRUE if successfully decoded
 * ------------------------------------------------------------------------ */

static Boolean DecodeWRBitArg(int StartArg, int EndArg, Byte *pResult)
{
  LongWord Result;
  Word Address;
  Byte BitPos;
  ShortInt OpSize;

  if (!DecodeBitArg(&Result, StartArg, EndArg, eSymbolSize8Bit))
    return False;
  (void)DissectBitSymbol(Result, &Address, &BitPos, &OpSize);
  if ((Address < pCurrCPUProps->WorkOfs) || (Address >= pCurrCPUProps->WorkOfs + 16))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[StartArg]);
    return False;
  }
  *pResult = ((Address & 15) << 3) | BitPos;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_Z8(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_Z8(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  Word Address;
  ShortInt OpSize;

  DissectBitSymbol(Inp, &Address, &BitPos, &OpSize);
  UNUSED(OpSize);

  UNUSED(DestSize);
  if ((Address >= pCurrCPUProps->WorkOfs) && (Address <= pCurrCPUProps->WorkOfs + 15))
    as_snprintf(pDest, DestSize, "%c%u", HexStartCharacter + ('r' - 'a'), (unsigned)(Address & 15));
  else
    SysString(pDest, DestSize, Address, ListRadixBase,
              mIsZ8Encore() ? 3 : 2, (16 == ListRadixBase) && (IntConstMode == eIntConstModeIntel),
              HexStartCharacter, SplitByteCharacter);
  as_snprcatf(pDest, DestSize, ".%u", (unsigned)BitPos);
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Index)
{
  BaseOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && ChkCoreFlags(pOrder->CoreFlags))
  {
    CodeLen = 1;
    BAsmCode[0] = pOrder->Code;
  }
}

static void DecodeALU2(Word Index)
{
  BaseOrder *pOrder = ALU2Orders + Index;
  Byte Save;
  int l = 0;

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(pOrder->CoreFlags))
  {
    if (Hi(pOrder->Code))
      BAsmCode[l++] = Hi(pOrder->Code);
    DecodeAdr(&ArgStr[1], MModReg | MModWReg | ((pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0 : MModIReg));
    switch (AdrType)
    {
      case ModReg:
       Save = AdrVal;
       DecodeAdr(&ArgStr[2], MModReg | MModIReg | MModImm);
       switch (AdrType)
       {
         case ModReg:
          BAsmCode[l++] = pOrder->Code + 4;
          BAsmCode[l++] = AdrVal;
          BAsmCode[l++] = Save;
          CodeLen = l;
          break;
         case ModIReg:
          BAsmCode[l++] = pOrder->Code + 5;
          BAsmCode[l++] = AdrVal;
          BAsmCode[l++] = Save;
          CodeLen = l;
          break;
         case ModImm:
          BAsmCode[l++] = pOrder->Code + 6;
          BAsmCode[l++] = Save;
          BAsmCode[l++] = AdrVal;
          CodeLen = l;
          break;
       }
       break;
      case ModWReg:
       Save = AdrVal;
       DecodeAdr(&ArgStr[2], MModWReg| MModReg | MModIWReg | MModIReg | MModImm);
       switch (AdrType)
       {
         case ModWReg:
          BAsmCode[l++] = pOrder->Code + 2;
          BAsmCode[l++] = (Save << 4) + AdrVal;
          CodeLen = l;
          break;
         case ModReg:
          BAsmCode[l++] = pOrder->Code + 4;
          BAsmCode[l++] = AdrVal;
          BAsmCode[l++] = pCurrCPUProps->WorkOfs + Save;
          CodeLen = l;
          break;
         case ModIWReg:
          BAsmCode[l++] = pOrder->Code + 3;
          BAsmCode[l++] = (Save << 4) + AdrVal;
          CodeLen = l;
          break;
         case ModIReg:
          BAsmCode[l++] = pOrder->Code + 5;
          BAsmCode[l++] = AdrVal;
          BAsmCode[l++] = pCurrCPUProps->WorkOfs + Save;
          CodeLen = l;
          break;
         case ModImm:
          BAsmCode[l++] = pOrder->Code + 6;
          BAsmCode[l++] = Save + pCurrCPUProps->WorkOfs;
          BAsmCode[l++] = AdrVal;
          CodeLen = l;
          break;
       }
       break;
      case ModIReg:
       Save = AdrVal;
       if (DecodeAdr(&ArgStr[2], MModImm))
       {
         BAsmCode[l++] = pOrder->Code + 7;
         BAsmCode[l++] = Save;
         BAsmCode[l++] = AdrVal;
         CodeLen = l;
       }
       break;
    }
  }
}

static void DecodeALUX(Word Index)
{
  BaseOrder *pOrder = ALUXOrders + Index;
  int l = 0;

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(pOrder->CoreFlags))
  {
    if (Hi(pOrder->Code))
      BAsmCode[l++] = Hi(pOrder->Code);
    if (DecodeAdr(&ArgStr[1], MModXReg))
    {
      BAsmCode[l + 3] = Lo(AdrWVal);
      BAsmCode[l + 2] = Hi(AdrWVal) & 15;
      DecodeAdr(&ArgStr[2], MModXReg | MModImm);
      switch (AdrType)
      {
        case ModXReg:
          BAsmCode[l + 0] = pOrder->Code;
          BAsmCode[l + 1] = AdrWVal >> 4;
          BAsmCode[l + 2] |= (AdrWVal & 15) << 4;
          CodeLen = l + 4;
          break;
        case ModImm:
          BAsmCode[l + 0] = pOrder->Code + 1;
          BAsmCode[l + 1] = AdrVal;
          CodeLen = l + 4;
          break;
      }
    }
  }
}

static void DecodeALU1(Word Index)
{
  ALU1Order *pOrder = ALU1Orders + Index;
  int l = 0;

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(pOrder->CoreFlags))
  {
    if (Hi(pOrder->Code))
      BAsmCode[l++] = Hi(pOrder->Code);
    DecodeAdr(&ArgStr[1], (pOrder->Is16 ? (MModWRReg | MModRReg) : MModReg) | MModIReg);
    switch (AdrType)
    {
      case ModReg:
      case ModRReg:
       BAsmCode[l++] = pOrder->Code;
       BAsmCode[l++] = AdrVal;
       CodeLen = l;
       break;
      case ModWRReg:
       BAsmCode[l++] = pOrder->Code;
       BAsmCode[l++] = pCurrCPUProps->WorkOfs + AdrVal;
       CodeLen = l;
       break;
      case ModIReg:
       BAsmCode[l++] = pOrder->Code + 1;
       BAsmCode[l++] = AdrVal;
       CodeLen = l;
       break;
    }
  }
}

static void DecodeLD(Word Index)
{
  Word Save;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModReg | MModWReg | MModIReg | MModIWReg | MModInd);
    switch (AdrType)
    {
      case ModReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModReg | MModWReg | MModIReg | MModImm);
        switch (AdrType)
        {
         case ModReg: /* Super8 OK */
           BAsmCode[0] = 0xe4;
           BAsmCode[1] = AdrVal;
           BAsmCode[2] = Save;
           CodeLen = 3;
           break;
         case ModWReg:
           if (pCurrCPUProps->CoreFlags & eCoreZ8Encore)
           {
             BAsmCode[0] = 0xe4;
             BAsmCode[1] = AdrVal + pCurrCPUProps->WorkOfs;
             BAsmCode[2] = Save;
             CodeLen = 3;
           }
           else /** non-eZ8 **/ /* Super8 OK */
           {
             BAsmCode[0] = (AdrVal << 4) + 9;
             BAsmCode[1] = Save;
             CodeLen = 2;
           }
           break;
         case ModIReg: /* Super8 OK */
           BAsmCode[0] = 0xe5;
           BAsmCode[1] = AdrVal;
           BAsmCode[2] = Save;
           CodeLen = 3;
           break;
         case ModImm: /* Super8 OK */
           BAsmCode[0] = 0xe6;
           BAsmCode[1] = Save;
           BAsmCode[2] = AdrVal;
           CodeLen = 3;
           break;
        }
        break;
      case ModWReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModWReg | MModReg | MModIWReg | MModIReg | MModImm | MModInd);
        switch (AdrType)
        {
          case ModWReg:
            if (pCurrCPUProps->CoreFlags & eCoreZ8Encore)
            {
              BAsmCode[0] = 0xe4;
              BAsmCode[1] = AdrVal + pCurrCPUProps->WorkOfs;
              BAsmCode[2] = Save + pCurrCPUProps->WorkOfs;
              CodeLen = 3;
            }
            else /** non-eZ8 */ /* Super8 OK */
            {
              BAsmCode[0] = (Save << 4) + 8;
              BAsmCode[1] = AdrVal + pCurrCPUProps->WorkOfs;
              CodeLen = 2;
            }
            break;
          case ModReg:
            if (pCurrCPUProps->CoreFlags & eCoreZ8Encore)
            {
              BAsmCode[0] = 0xe4;
              BAsmCode[1] = AdrVal;
              BAsmCode[2] = Save + pCurrCPUProps->WorkOfs;
              CodeLen = 3;
            }
            else /** non-eZ8 **/ /* Super8 OK */
            {
              BAsmCode[0] = (Save << 4) + 8;
              BAsmCode[1] = AdrVal;
              CodeLen = 2;
            }
            break;
          case ModIWReg:
            /* is C7 r,IR or r,ir? */
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0xc7 : 0xe3;
            BAsmCode[1] = (Save << 4) + AdrVal;
            CodeLen = 2;
            break;
          case ModIReg: /* Super8 OK */
            BAsmCode[0] = 0xe5;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = pCurrCPUProps->WorkOfs + Save;
            CodeLen = 3;
            break;
          case ModImm: /* Super8 OK */
            BAsmCode[0] = (Save << 4) + 12;
            BAsmCode[1] = AdrVal;
            CodeLen = 2;
            break;
          case ModInd:
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0x87 : 0xc7;
            BAsmCode[1] = (Save << 4) + AdrVal;
            BAsmCode[2] = AdrIndex;
            CodeLen = 3;
            break;
        }
        break;
      case ModIReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModReg | MModImm);
        switch (AdrType)
        {
          case ModReg: /* Super8 OK */
            BAsmCode[0] = 0xf5;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0xd6 : 0xe7;
            BAsmCode[1] = Save;
            BAsmCode[2] = AdrVal;
            CodeLen = 3;
            break;
        }
        break;
      case ModIWReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModWReg | MModReg | MModImm);
        switch (AdrType)
        {
          case ModWReg:
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0xd7 : 0xf3;
            BAsmCode[1] = (Save << 4) + AdrVal;
            CodeLen = 2;
            break;
          case ModReg: /* Super8 OK */
            BAsmCode[0] = 0xf5;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = pCurrCPUProps->WorkOfs + Save;
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0xd6 : 0xe7;
            BAsmCode[1] = pCurrCPUProps->WorkOfs + Save;
            BAsmCode[2] = AdrVal;
            CodeLen = 3;
            break;
        }
        break;
      case ModInd:
        Save = AdrVal;
        if (DecodeAdr(&ArgStr[2], MModWReg))
        {
          BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreSuper8) ? 0x97 : 0xd7;
          BAsmCode[1] = (AdrVal << 4) + Save;
          BAsmCode[2] = AdrIndex;
          CodeLen = 3;
        }
        break;
    }
  }
}

static void DecodeLDCE(Word Code)
{
  Byte Save, Super8Add = mIsSuper8() && !!(Code == 0x82);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    LongWord DestMask = MModWReg | MModIWRReg, SrcMask;

    if ((pCurrCPUProps->CoreFlags & eCoreZ8Encore) && (Code == 0xc2))
      DestMask |= MModIWReg;
    if (mIsSuper8())
      DestMask |= MModIndRR | MModIndRR16 | MModDA;
    DecodeAdr(&ArgStr[1], DestMask);
    switch (AdrType)
    {
      case ModWReg:
        SrcMask = MModIWRReg;
        if (pCurrCPUProps->CoreFlags & eCoreSuper8)
          SrcMask |= MModIndRR | MModIndRR16 | MModDA;
        Save = AdrVal; DecodeAdr(&ArgStr[2], SrcMask);
        switch (AdrType)
        {
          case ModIWRReg:
            BAsmCode[0] = mIsSuper8() ? 0xc3 : Code;
            BAsmCode[1] = (Save << 4) | AdrVal | Super8Add;
            CodeLen = 2;
            break;
          case ModDA:
            BAsmCode[0] = 0xa7;
            BAsmCode[1] = (Save << 4) | Super8Add;
            BAsmCode[2] = Lo(AdrWVal);
            BAsmCode[3] = Hi(AdrWVal);
            CodeLen = 4;
            break;
          case ModIndRR:
            BAsmCode[0] = 0xe7;
            BAsmCode[1] = (Save << 4) | AdrVal | Super8Add;
            BAsmCode[2] = Lo(AdrIndex);
            CodeLen = 3;
            break;
          case ModIndRR16:
            BAsmCode[0] = 0xa7;
            BAsmCode[1] = (Save << 4) | AdrVal | Super8Add;
            BAsmCode[2] = Lo(AdrIndex);
            BAsmCode[3] = Hi(AdrIndex);
            CodeLen = 4;
            break;
        }
        break;
      case ModIWReg:
        Save = AdrVal; DecodeAdr(&ArgStr[2], MModIWRReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xc5;
          BAsmCode[1] = (Save << 4) | AdrVal;
          CodeLen = 2;
        }
        break;
      case ModIWRReg:
        Save = AdrVal; DecodeAdr(&ArgStr[2], MModWReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = mIsSuper8() ? 0xd3 : Code + 0x10;
          BAsmCode[1] = (AdrVal << 4) | Save | Super8Add;
          CodeLen = 2;
        }
        break;
      case ModDA: /* Super8 only */
        BAsmCode[2] = Lo(AdrWVal);
        BAsmCode[3] = Hi(AdrWVal);
        DecodeAdr(&ArgStr[2], MModWReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xb7;
          BAsmCode[1] = (AdrVal << 4) | Super8Add;
          CodeLen = 4;
        }
        break;
      case ModIndRR: /* Super8 only */
        BAsmCode[2] = Lo(AdrIndex);
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModWReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xf7;
          BAsmCode[1] = (AdrVal << 4) | Save | Super8Add;
          CodeLen = 3;
        }
        break;
      case ModIndRR16: /* Super8 only */
        BAsmCode[2] = Lo(AdrIndex);
        BAsmCode[3] = Hi(AdrIndex);
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModWReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xb7;
          BAsmCode[1] = (AdrVal << 4) | Save | Super8Add;
          CodeLen = 4;
        }
        break;
    }
  }
}

static void DecodeLDCEI(Word Index)
{
  Byte Save;

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModIWReg | MModIWRReg);
    switch (AdrType)
    {
      case ModIWReg:
        Save = AdrVal; DecodeAdr(&ArgStr[2], MModIWRReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = Index;
          BAsmCode[1] = (Save << 4) + AdrVal;
          CodeLen = 2;
        }
        break;
      case ModIWRReg:
        Save = AdrVal; DecodeAdr(&ArgStr[2], MModIWReg);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = Index + 0x10;
          BAsmCode[1] = (AdrVal << 4) + Save;
          CodeLen = 2;
        }
        break;
    }
  }
}

static void DecodeLDCEDI(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeAdr(&ArgStr[1], MModWReg))
  {
    BAsmCode[0] = Lo(Code);
    BAsmCode[1] = AdrVal << 4;
    DecodeAdr(&ArgStr[2], MModIWRReg);
    if (AdrType == ModIWRReg)
    {
      BAsmCode[1] |= AdrVal | Hi(Code);
      CodeLen = 2;
    }
  }
}

static void DecodeLDCEPDI(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeAdr(&ArgStr[1], MModIWRReg))
  {
    BAsmCode[0] = Lo(Code);
    BAsmCode[1] = AdrVal | Hi(Code);
    DecodeAdr(&ArgStr[2], MModWReg);
    if (AdrType == ModWReg)
    {
      BAsmCode[1] |= AdrVal << 4;
      CodeLen = 2;
    }
  }
}

static void DecodeINC(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModWReg | MModReg | MModIReg);
    switch (AdrType)
    {
      case ModWReg:
        BAsmCode[0] = (AdrVal << 4) + 0x0e;
        CodeLen = 1;
        break;
      case ModReg:
        BAsmCode[0] = 0x20;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
      case ModIReg:
        BAsmCode[0] = 0x21;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeJR(Word Index)
{
  Integer AdrInt;
  int z;
  tEvalResult EvalResult;

  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    z = (ArgCnt == 1) ? TrueCond : DecodeCond(&ArgStr[1]);
    if (z < CondCnt)
    {
      AdrInt = EvalStrIntExpressionWithResult(&ArgStr[ArgCnt], Int16, &EvalResult) - (EProgCounter() + 2);
      if (EvalResult.OK)
      {
        if (!mSymbolQuestionable(EvalResult.Flags)
         && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          ChkSpace(SegCode, EvalResult.AddrSpaceMask);
          BAsmCode[0] = (Conditions[z].Code << 4) + 0x0b;
          BAsmCode[1] = Lo(AdrInt);
          CodeLen = 2;
        }
      }
    }
  }
}

static void DecodeDJNZ(Word Index)
{
  Integer AdrInt;
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore)
   && DecodeAdr(&ArgStr[1], MModWReg))
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], Int16, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags)
       && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = (AdrVal << 4) + 0x0a;
        BAsmCode[1] = Lo(AdrInt);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeCPIJNE(Word Code)
{
  if (ChkArgCnt(3, 3)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeAdr(&ArgStr[1], MModWReg))
  {
    BAsmCode[1] = AdrVal & 0x0f;

    DecodeAdr(&ArgStr[2], MModIWReg);
    if (AdrType != ModNone)
    {
      Boolean OK;
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], Int16, &OK, &Flags) - (EProgCounter() + 3);

      BAsmCode[1] |= AdrVal << 4;

      if (OK)
      {
        if (!mSymbolQuestionable(Flags)
         && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = Code;
          BAsmCode[2] = Lo(AdrInt);
          CodeLen = 3;
        }
      }
    }
  }
}

static void DecodeCALL(Word Index)
{
  Boolean IsSuper8 = !!(pCurrCPUProps->CoreFlags & eCoreSuper8);
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModIWRReg | MModIRReg | MModDA | (IsSuper8 ? MModImm : 0));
    switch (AdrType)
    {
      case ModIWRReg:
        BAsmCode[0] = IsSuper8 ? 0xf4 : 0xd4;
        BAsmCode[1] = pCurrCPUProps->WorkOfs + AdrVal;
        CodeLen = 2;
        break;
      case ModIRReg:
        BAsmCode[0] = IsSuper8 ? 0xf4 : 0xd4;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
      case ModDA:
        BAsmCode[0] = IsSuper8 ? 0xf6 : 0xd6;
        BAsmCode[1] = Hi(AdrWVal);
        BAsmCode[2] = Lo(AdrWVal);
        CodeLen = 3;
        break;
      case ModImm:
        BAsmCode[0] = 0xd4;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeJP(Word Index)
{
  int z;

  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && ChkCoreFlags(eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore))
  {
    z = (ArgCnt == 1) ? TrueCond : DecodeCond(&ArgStr[1]);
    if (z < CondCnt)
    {
      DecodeAdr(&ArgStr[ArgCnt], MModIWRReg | MModIRReg | MModDA);
      switch (AdrType)
      {
        case ModIWRReg:
          if (z != TrueCond) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreZ8Encore) ? 0xc4 : 0x30;
            BAsmCode[1] = pCurrCPUProps->WorkOfs + AdrVal;
            CodeLen = 2;
          }
          break;
        case ModIRReg:
          if (z != TrueCond) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreZ8Encore) ? 0xc4 : 0x30;
            BAsmCode[1] = AdrVal;
            CodeLen = 2;
          }
          break;
        case ModDA:
          BAsmCode[0] = (Conditions[z].Code << 4) + 0x0d;
          BAsmCode[1] = Hi(AdrWVal);
          BAsmCode[2] = Lo(AdrWVal);
          CodeLen = 3;
          break;
      }
    }
  }
}

static void DecodeSRP(Word Code)
{
  Boolean Valid;

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags((Hi(Code) ? eCoreNone : (eCoreZ8NMOS | eCoreZ8CMOS | eCoreZ8Encore)) | eCoreSuper8)
   && DecodeAdr(&ArgStr[1], MModImm))
  {
    if (pCurrCPUProps->CoreFlags & eCoreZ8Encore || Memo("RDR"))
      Valid = True;
    else
    {
      Byte MuteMask = Hi(Code) ? 7 : 15;

      Valid = (((AdrVal & MuteMask) == 0) && ((AdrVal <= pCurrCPUProps->RAMEnd) || (AdrVal >= pCurrCPUProps->SFRStart)));
    }
    if (!Valid) WrError(ErrNum_InvRegisterPointer);
    BAsmCode[0] = (pCurrCPUProps->CoreFlags & eCoreZ8Encore) ? 0x01 : Lo(Code);
    BAsmCode[1] = AdrVal | Hi(Code);
    CodeLen = 2;
  }
}

static void DecodeStackExt(Word Index)
{
  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8Encore)
   && DecodeAdr(&ArgStr[1], MModXReg))
  {
    BAsmCode[0] = Index;
    BAsmCode[1] = AdrWVal >> 4;
    BAsmCode[2] = (AdrWVal & 15) << 4;
    CodeLen = 3;
  }
}

static void DecodeStackDI(Word Code)
{
  int MemIdx = ((Code >> 4) & 15) - 7;

  if (ChkArgCnt(2, 2)
     && ChkCoreFlags(eCoreSuper8))
  {
    Byte Reg;

    DecodeAdr(&ArgStr[3 - MemIdx], MModReg | MModWReg);
    Reg = (AdrType == ModWReg) ? AdrVal + pCurrCPUProps->WorkOfs : AdrVal;
    if (AdrType != ModNone)
    {
      DecodeAdr(&ArgStr[MemIdx], MModIReg | MModIWReg);
      if (AdrType == ModIWReg)
        AdrVal += pCurrCPUProps->WorkOfs;
      if (AdrType != ModNone)
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVal;
        BAsmCode[2] = Reg;
        CodeLen = 3;
      }
    }
  }
}

static void DecodeTRAP(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8Encore)
   && DecodeAdr(&ArgStr[1], MModImm))
  {
    BAsmCode[0] = 0xf2;
    BAsmCode[1] = AdrVal;
    CodeLen = 2;
  }
}

static void DecodeBSWAP(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8Encore)
   && DecodeAdr(&ArgStr[1], MModReg))
  {
    BAsmCode[0] = 0xd5;
    BAsmCode[1] = AdrVal;
    CodeLen = 2;
  }
}

static void DecodeMULT(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkCoreFlags(eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModWRReg | MModReg);
    switch (AdrType)
    {
      case ModWRReg:
        BAsmCode[0] = 0xf4;
        BAsmCode[1] = AdrVal + pCurrCPUProps->WorkOfs;
        CodeLen = 2;
        break;
      case ModReg:
        BAsmCode[0] = 0xf4;
        BAsmCode[1] = AdrVal;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeMULT_DIV(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeAdr(&ArgStr[1], MModWRReg | MModRReg))
  {
    BAsmCode[2] = (AdrType == ModWRReg) ? AdrVal + pCurrCPUProps->WorkOfs : AdrVal;
    DecodeAdr(&ArgStr[2], MModWReg | MModReg | MModIReg | MModImm);
    switch (AdrType)
    {
      case ModWReg:
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVal + pCurrCPUProps->WorkOfs;
        CodeLen = 3;
        break;
      case ModReg:
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVal;
        CodeLen = 3;
        break;
      case ModIReg:
        BAsmCode[0] = Code  + 1;
        BAsmCode[1] = AdrVal;
        CodeLen = 3;
        break;
      case ModImm:
        BAsmCode[0] = Code  + 2;
        BAsmCode[1] = AdrVal;
        CodeLen = 3;
        break;
    }
  }
}

static void DecodeLDX(Word Index)
{
  Word Save;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModWReg | MModIWReg | MModReg | MModIReg | MModIWRReg | MModIndRR | MModXReg | MModWeird);
    switch (AdrType)
    {
      case ModWReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModXReg | MModIndRR | MModImm);
        switch (AdrType)
        {
          case ModXReg:
            Save += LongWorkOfs;
            BAsmCode[0] = 0xe8;
            BAsmCode[1] = AdrWVal >> 4;
            BAsmCode[2] = ((AdrWVal & 15) << 4) | (Hi(Save) & 15);
            BAsmCode[3] = Lo(Save);
            CodeLen = 4;
            break;
          case ModIndRR:
            BAsmCode[0] = 0x88;
            BAsmCode[1] = (Save << 4) | AdrVal;
            BAsmCode[2] = AdrIndex;
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = 0xe9;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Hi(LongWorkOfs | Save);
            BAsmCode[3] = Lo(LongWorkOfs | Save);
            CodeLen = 4;
            break;
        }
        break;
      case ModIWReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModXReg);
        switch (AdrType)
        {
          case ModXReg:
            BAsmCode[0] = 0x85;
            BAsmCode[1] = (Save << 4) | (Hi(AdrWVal) & 15);
            BAsmCode[2] = Lo(AdrWVal);
            CodeLen = 3;
            break;
        }
        break;
      case ModReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModIReg);
        switch (AdrType)
        {
          case ModIReg:
            BAsmCode[0] = 0x86;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
        }
        break;
      case ModIReg:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModReg | MModWeird);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x96;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
          case ModWeird:
            BAsmCode[0] = 0x87;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
        }
        break;
      case ModIWRReg:
        Save = pCurrCPUProps->WorkOfs + AdrVal;
        DecodeAdr(&ArgStr[2], MModReg);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x96;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
        }
        break;
      case ModIndRR:
        BAsmCode[2] = AdrIndex;
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModWReg);
        switch (AdrType)
        {
          case ModWReg:
            BAsmCode[0] = 0x89;
            BAsmCode[1] = (Save << 4) | AdrVal;
            CodeLen = 3;
            break;
        }
        break;
      case ModXReg:
        Save = AdrWVal;
        DecodeAdr(&ArgStr[2], MModWReg | MModIWReg | MModXReg | MModImm);
        switch (AdrType)
        {
          case ModWReg:
            BAsmCode[0] = 0x94;
            BAsmCode[1] = (AdrVal << 4) | (Hi(Save) & 15);
            BAsmCode[2] = Lo(Save);
            CodeLen = 3;
            break;
          case ModIWReg:
            BAsmCode[0] = 0x95;
            BAsmCode[1] = (AdrVal << 4) | (Hi(Save) & 15);
            BAsmCode[2] = Lo(Save);
            CodeLen = 3;
            break;
          case ModXReg:
            BAsmCode[0] = 0xe8;
            BAsmCode[1] = AdrWVal >> 4;
            BAsmCode[2] = ((AdrWVal & 15) << 4) | (Hi(Save) & 15);
            BAsmCode[3] = Lo(Save);
            CodeLen = 4;
            break;
          case ModImm:
            BAsmCode[0] = 0xe9;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = (Hi(Save) & 15);
            BAsmCode[3] = Lo(Save);
            CodeLen = 4;
            break;
        }
        break;
      case ModWeird:
        Save = AdrVal;
        DecodeAdr(&ArgStr[2], MModIReg);
        switch (AdrType)
        {
          case ModIReg:
            BAsmCode[0] = 0x97;
            BAsmCode[1] = AdrVal;
            BAsmCode[2] = Save;
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeLDW(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreSuper8))
  {
    DecodeAdr(&ArgStr[1], MModRReg | MModWRReg);
    if (AdrType != ModNone)
    {
      Byte Dest = (AdrType == ModRReg) ? AdrVal : AdrVal + pCurrCPUProps->WorkOfs;

      OpSize = eSymbolSize16Bit;
      DecodeAdr(&ArgStr[2], MModRReg | MModWRReg | MModIReg | MModImm);
      switch (AdrType)
      {
        case ModWRReg:
        case ModRReg:
          BAsmCode[0] = Code;
          BAsmCode[1] = (AdrType == ModRReg) ? AdrVal : AdrVal + pCurrCPUProps->WorkOfs;
          BAsmCode[2] = Dest;
          CodeLen = 3;
          break;
        case ModIReg:
          BAsmCode[0] = Code + 1;
          BAsmCode[1] = AdrVal;
          BAsmCode[2] = Dest;
          CodeLen = 3;
          break;
        case ModImm:
          BAsmCode[0] = Code + 2;
          BAsmCode[1] = Dest;
          BAsmCode[2] = Hi(AdrWVal);
          BAsmCode[3] = Lo(AdrWVal);
          CodeLen = 4;
          break;
      }
    }
  }
}

static void DecodeLDWX(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8Encore)
   && DecodeAdr(&ArgStr[1], MModXReg))
  {
    BAsmCode[0] = 0x1f;
    BAsmCode[1] = 0xe8;
    BAsmCode[3] = Hi(AdrWVal);
    BAsmCode[4] = Lo(AdrWVal);
    if (DecodeAdr(&ArgStr[2], MModXReg))
    {
      BAsmCode[2] = AdrWVal >> 4;
      BAsmCode[3] |= (AdrWVal & 0x0f) << 4;
      CodeLen = 5;
    }
  }
}

static void DecodeLEA(Word Index)
{
  Byte Save;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkCoreFlags(eCoreZ8Encore))
  {
    DecodeAdr(&ArgStr[1], MModWReg | MModWRReg);
    switch (AdrType)
    {
      case ModWReg:
        Save = AdrVal;
        if (DecodeAdr(&ArgStr[2], MModInd))
        {
          BAsmCode[0] = 0x98;
          BAsmCode[1] = (Save << 4) | AdrVal;
          BAsmCode[2] = AdrIndex;
          CodeLen = 3;
        }
        break;
      case ModWRReg:
        Save = AdrVal;
        if (DecodeAdr(&ArgStr[2], MModIndRR))
        {
          BAsmCode[0] = 0x99;
          BAsmCode[1] = (Save << 4) | AdrVal;
          BAsmCode[2] = AdrIndex;
          CodeLen = 3;
        }
        break;
    }
  }
}

static void DecodeBIT(Word Index)
{
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(3, 3)
   && ChkCoreFlags(eCoreZ8Encore))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt1, &OK) << 7;
    if (OK)
    {
      BAsmCode[1] |= EvalStrIntExpression(&ArgStr[2], UInt3, &OK) << 4;
      if (OK)
      {
        DecodeAdr(&ArgStr[3], MModWReg);
        switch (AdrType)
        {
          case ModWReg:
            BAsmCode[0] = 0xe2;
            BAsmCode[1] |= AdrVal;
            CodeLen = 2;
            break;
        }
      }
    }
  }
}

static void DecodeBit(Word Index)
{
  Boolean OK;

  if (ChkCoreFlags(eCoreZ8Encore))
    switch (ArgCnt)
    {
      case 1:
      {
        LongWord BitArg;
        ShortInt OpSize = eSymbolSize8Bit;

        if (DecodeBitArg(&BitArg, 1, 1, OpSize))
        {
          Word Address;
          Byte BitPos;

          (void)DissectBitSymbol(BitArg, &Address, &BitPos, &OpSize);
          if ((Address & 0xff0) == pCurrCPUProps->WorkOfs)
          {
            BAsmCode[0] = 0xe2;
            BAsmCode[1] = Index | (BitPos << 4) | (Address & 15);
            CodeLen = 2;
          }
          else /* -> ANDX,ORX ER,IM */
          {
            BAsmCode[0] = Index ? 0x49 : 0x59;
            BAsmCode[1] = Index ? (1 << BitPos) : ~(1 << BitPos);
            BAsmCode[2] = Hi(Address);
            BAsmCode[3] = Lo(Address);
            CodeLen = 4;
          }
        }
        break;
      }
      case 2:
        BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
        if (OK)
        {
          DecodeAdr(&ArgStr[2], MModWReg | MModXReg);
          switch (AdrType)
          {
            case ModWReg:
              BAsmCode[0] = 0xe2;
              BAsmCode[1] = (BAsmCode[1] << 4) | Index | AdrVal;
              CodeLen = 2;
              break;
            case ModXReg: /* -> ANDX,ORX ER,IM */
              BAsmCode[0] = Index ? 0x49 : 0x59;
              BAsmCode[1] = Index ? (1 << BAsmCode[1]) : ~(1 << BAsmCode[1]);
              BAsmCode[2] = Hi(AdrWVal);
              BAsmCode[3] = Lo(AdrWVal);
              CodeLen = 4;
              break;
          }
        }
        break;
      default:
        (void)ChkArgCnt(1, 2);
    }
}

static void DecodeBTJCore(Word Index, int ArgOffset)
{
  int TmpCodeLen = 0;

  switch (ArgCnt - ArgOffset)
  {
    case 2:
    {
      LongWord BitArg;
      ShortInt OpSize = eSymbolSize8Bit;

      if (DecodeBitArg(&BitArg, 1 + ArgOffset, 1 + ArgOffset, OpSize))
      {
        Word Address;
        Byte BitPos;

        (void)DissectBitSymbol(BitArg, &Address, &BitPos, &OpSize);
        if ((Address & 0xff0) == pCurrCPUProps->WorkOfs)
        {
          BAsmCode[0] = 0xf6;
          BAsmCode[1] = (Address & 15) | (BitPos << 4);
          TmpCodeLen = 2;
        }
        else
          WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      }
      break;
    }
    case 3:
    {
      Boolean OK;

      BAsmCode[1] = EvalStrIntExpression(&ArgStr[1 + ArgOffset], UInt3, &OK) << 4;
      if (OK)
      {
        DecodeAdr(&ArgStr[2 + ArgOffset], MModWReg | MModIWReg);
        switch (AdrType)
        {
          case ModWReg:
            BAsmCode[0] = 0xf6;
            BAsmCode[1] |= AdrVal;
            TmpCodeLen = 2;
            break;
          case ModIWReg:
            BAsmCode[0] = 0xf7;
            BAsmCode[1] |= AdrVal;
            TmpCodeLen = 2;
            break;
        }
      }
      break;
    }
    default:
      (void)ChkArgCnt(3, 4);
  }
  if (TmpCodeLen > 0)
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionWithResult(&ArgStr[ArgCnt], Int16, &EvalResult) - (EProgCounter() + TmpCodeLen + 1);

    BAsmCode[1] |= Index;
    if (EvalResult.OK)
    {
      if (!mSymbolQuestionable(EvalResult.Flags)
       && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        BAsmCode[TmpCodeLen] = Lo(AdrInt);
        CodeLen = TmpCodeLen + 1;
      }
    }
  }
}

static void DecodeBTJ(Word Index)
{
  if (ChkCoreFlags(eCoreZ8Encore)
   && ChkArgCnt(3, 4))
  {
    Boolean OK;

    Index = EvalStrIntExpression(&ArgStr[1], UInt1, &OK) << 7;
    if (OK)
      DecodeBTJCore(Index, 1);
  }
}

static void DecodeBtj(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 3)
   && ChkCoreFlags(eCoreZ8Encore))
    DecodeBTJCore(Index, 0);
}

static void DecodeBit1(Word Code)
{
  if (ChkArgCnt(1, 2)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeWRBitArg(1, ArgCnt, &BAsmCode[1]))
  {
    BAsmCode[1] = (BAsmCode[1] << 1) | Hi(Code);
    BAsmCode[0] = Lo(Code);
    CodeLen = 2;
  }
}

static void DecodeBit2(Word Code)
{
  if (ChkCoreFlags(eCoreSuper8))
  {
    LongWord BitValue;
    Byte Reg;

    switch (ArgCnt)
    {
      case 3:
      {
        int BitStart, BitEnd, RegIdx;

        if ((*ArgStr[2].str.p_str == '#') && Hi(Code))
        {
          BitStart = 1;
          BitEnd = 2;
          RegIdx = 3;
          BAsmCode[1] = 0x01;
        }
        else
        {
          BitStart = 2;
          BitEnd = 3;
          RegIdx = 1;
          BAsmCode[1] = 0x00;
        }
        DecodeAdr(&ArgStr[RegIdx], MModWReg); Reg = AdrVal;
        if ((AdrType == ModWReg) && DecodeBitArg2(&BitValue, &ArgStr[BitStart], &ArgStr[BitEnd], eSymbolSize8Bit));
        else
          return;
        break;
      }
      case 2:
      {
        if ((IsWReg(&ArgStr[1], &Reg, False) == eIsReg)
         && DecodeBitArg(&BitValue, 2, 2, eSymbolSize8Bit))
        {
          BAsmCode[1] = 0x00;
        }
        else if ((IsWReg(&ArgStr[2], &Reg, False) == eIsReg)
              && DecodeBitArg(&BitValue, 1, 1, eSymbolSize8Bit)
              && Hi(Code))
        {
          BAsmCode[1] = 0x01;
        }
        else
        {
          WrError(ErrNum_InvAddrMode);
          return;
        }
        break;
      }
      default:
        (void)ChkArgCnt(2, 3);
        return;
    }
    CodeLen = 3;
    BAsmCode[0] = Lo(Code);
    BAsmCode[1] |= (Reg << 4) | ((BitValue & 7) << 1);
    BAsmCode[2] = BitValue >> 3;
    CodeLen = 3;
  }
}

static void DecodeBitRel(Word Code)
{
  if (ChkArgCnt(2, 3)
   && ChkCoreFlags(eCoreSuper8)
   && DecodeWRBitArg(2, ArgCnt, &BAsmCode[1]))
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult) - (EProgCounter() + 3);
    if (EvalResult.OK)
    {
      if (!mSymbolQuestionable(EvalResult.Flags)
       && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        BAsmCode[0] = Lo(Code);
        BAsmCode[1] = (BAsmCode[1] << 1) | Hi(Code);
        BAsmCode[2] = Lo(AdrInt);
        CodeLen = 3;
      }
    }
  }
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, mIsZ8Encore() ? 0xfff : 0xff);
}

static void DecodeDEFBIT(Word Code)
{
  LongWord BitSpec;

  UNUSED(Code);

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    BitPos = EvalBitPosition(&ArgStr[2], &OK, eSymbolSize8Bit);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandZ8Bit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (DecodeBitArg(&BitSpec, 1, ArgCnt, eSymbolSize8Bit))
    {
      *ListLine = '=';
      DissectBit_Z8(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Instruction Table Buildup/Teardown */

static void AddFixed(const char *NName, Word Code, tCoreFlags CoreFlags)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].Code = Code;
  FixedOrders[InstrZ].CoreFlags = CoreFlags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddALU2(const char *NName, Word Code, tCoreFlags CoreFlags)
{
  order_array_rsv_end(ALU2Orders, BaseOrder);
  ALU2Orders[InstrZ].Code = Code;
  ALU2Orders[InstrZ].CoreFlags = CoreFlags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU2);
}

static void AddALUX(const char *NName, Word Code, tCoreFlags CoreFlags)
{
  order_array_rsv_end(ALUXOrders, BaseOrder);
  ALUXOrders[InstrZ].Code = Code;
  ALUXOrders[InstrZ].CoreFlags = CoreFlags;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALUX);
}

static void AddALU1(const char *NName, Word Code, tCoreFlags CoreFlags, Boolean Is16)
{
  order_array_rsv_end(ALU1Orders, ALU1Order);
  ALU1Orders[InstrZ].Code = Code;
  ALU1Orders[InstrZ].CoreFlags = CoreFlags;
  ALU1Orders[InstrZ].Is16 = Is16;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU1);
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

  InstrZ = 0;
  AddFixed("CCF"  , 0xef   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("DI"   , 0x8f   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("EI"   , 0x9f   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("HALT" , 0x7f   ,               eCoreZ8CMOS               | eCoreZ8Encore);
  AddFixed("IRET" , 0xbf   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("NOP"  , NOPCode, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("RCF"  , 0xcf   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("RET"  , 0xaf   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("SCF"  , 0xdf   , eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddFixed("STOP" , 0x6f   ,               eCoreZ8CMOS               | eCoreZ8Encore);
  AddFixed("ATM"  , 0x2f   , eCoreZ8NMOS | eCoreZ8CMOS               | eCoreZ8Encore);
  AddFixed("BRK"  , 0x00   ,                                           eCoreZ8Encore);
  AddFixed("WDH"  , 0x4f   , eCoreZ8NMOS | eCoreZ8CMOS                              );
  AddFixed("WDT"  , 0x5f   , eCoreZ8NMOS | eCoreZ8CMOS               | eCoreZ8Encore);
  AddFixed("ENTER", 0x1f   ,                             eCoreSuper8                );
  AddFixed("EXIT" , 0x2f   ,                             eCoreSuper8                );
  AddFixed("NEXT" , 0x0f   ,                             eCoreSuper8                );
  AddFixed("SB0"  , 0x4f   ,                             eCoreSuper8                );
  AddFixed("SB1"  , 0x5f   ,                             eCoreSuper8                );
  AddFixed("WFI"  , 0x3f   ,                             eCoreSuper8                );

  InstrZ = 0;
  AddALU2("ADD" , 0x0000, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("ADC" , 0x0010, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("SUB" , 0x0020, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("SBC" , 0x0030, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("OR"  , 0x0040, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("AND" , 0x0050, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("TCM" , 0x0060, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("TM"  , 0x0070, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("CP"  , 0x00a0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("XOR" , 0x00b0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore);
  AddALU2("CPC" , (EXTPREFIX << 8) | 0xa0, eCoreZ8Encore);

  InstrZ = 0;
  AddALUX("ADDX", 0x0008, eCoreZ8Encore);
  AddALUX("ADCX", 0x0018, eCoreZ8Encore);
  AddALUX("SUBX", 0x0028, eCoreZ8Encore);
  AddALUX("SBCX", 0x0038, eCoreZ8Encore);
  AddALUX("ORX" , 0x0048, eCoreZ8Encore);
  AddALUX("ANDX", 0x0058, eCoreZ8Encore);
  AddALUX("TCMX", 0x0068, eCoreZ8Encore);
  AddALUX("TMX" , 0x0078, eCoreZ8Encore);
  AddALUX("CPX" , 0x00a8, eCoreZ8Encore);
  AddALUX("XORX", 0x00b8, eCoreZ8Encore);
  AddALUX("CPCX", (EXTPREFIX << 8) | 0xa8, eCoreZ8Encore);

  InstrZ = 0;
  AddALU1("DEC" , (pCurrCPUProps->CoreFlags & eCoreZ8Encore) ? 0x0030 : 0x0000, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("RLC" , 0x0010, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("DA"  , 0x0040, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("POP" , 0x0050, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("COM" , 0x0060, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("PUSH", 0x0070, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("DECW", 0x0080, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, True );
  AddALU1("RL"  , 0x0090, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("INCW", 0x00a0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, True );
  AddALU1("CLR" , 0x00b0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("RRC" , 0x00c0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("SRA" , 0x00d0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("RR"  , 0x00e0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("SWAP", 0x00f0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreSuper8 | eCoreZ8Encore, False);
  AddALU1("SRL" , (EXTPREFIX << 8) | 0xc0, eCoreZ8NMOS | eCoreZ8CMOS | eCoreZ8Encore, False);

  InstrZ = 0;
  AddCondition("F"  , 0); TrueCond = InstrZ; AddCondition("T"  , 8);
  AddCondition("C"  , 7); AddCondition("NC" ,15);
  AddCondition("Z"  , 6); AddCondition("NZ" ,14);
  AddCondition("MI" , 5); AddCondition("PL" ,13);
  AddCondition("OV" , 4); AddCondition("NOV",12);
  AddCondition("EQ" , 6); AddCondition("NE" ,14);
  AddCondition("LT" , 1); AddCondition("GE" , 9);
  AddCondition("LE" , 2); AddCondition("GT" ,10);
  AddCondition("ULT", 7); AddCondition("UGE",15);
  AddCondition("ULE", 3); AddCondition("UGT",11);
  CondCnt = InstrZ;

  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDX", 0, DecodeLDX);
  AddInstTable(InstTable, "LDW", 0xc4, DecodeLDW);
  AddInstTable(InstTable, "LDWX", 0, DecodeLDWX);
  AddInstTable(InstTable, "LDC", 0xc2, DecodeLDCE);
  AddInstTable(InstTable, "LDE", 0x82, DecodeLDCE);
  if (mIsSuper8())
  {
    AddInstTable(InstTable, "LDCI", 0x00e3, DecodeLDCEDI);
    AddInstTable(InstTable, "LDEI", 0x01e3, DecodeLDCEDI);
  }
  else
  {
    AddInstTable(InstTable, "LDCI", 0xc3, DecodeLDCEI);
    AddInstTable(InstTable, "LDEI", 0x83, DecodeLDCEI);
  }
  AddInstTable(InstTable, "LDCD", 0x00e2, DecodeLDCEDI);
  AddInstTable(InstTable, "LDED", 0x01e2, DecodeLDCEDI);
  AddInstTable(InstTable, "LDCPD", 0x00f2, DecodeLDCEPDI);
  AddInstTable(InstTable, "LDEPD", 0x01f2, DecodeLDCEPDI);
  AddInstTable(InstTable, "LDCPI", 0x00f3, DecodeLDCEPDI);
  AddInstTable(InstTable, "LDEPI", 0x01f3, DecodeLDCEPDI);
  AddInstTable(InstTable, "INC", 0, DecodeINC);
  AddInstTable(InstTable, "JR", 0, DecodeJR);
  AddInstTable(InstTable, "JP", 0, DecodeJP);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "SRP", 0x0031, DecodeSRP);
  AddInstTable(InstTable, "SRP0", 0x0231, DecodeSRP);
  AddInstTable(InstTable, "SRP1", 0x0131, DecodeSRP);
  AddInstTable(InstTable, "RDR", 0x01d5, DecodeSRP);
  AddInstTable(InstTable, "DJNZ", 0, DecodeDJNZ);
  AddInstTable(InstTable, "LEA", 0, DecodeLEA);

  AddInstTable(InstTable, "POPX" , 0xd8, DecodeStackExt);
  AddInstTable(InstTable, "PUSHX", 0xc8, DecodeStackExt);
  AddInstTable(InstTable, "POPUD", 0x92, DecodeStackDI);
  AddInstTable(InstTable, "POPUI", 0x93, DecodeStackDI);
  AddInstTable(InstTable, "PUSHUD", 0x82, DecodeStackDI);
  AddInstTable(InstTable, "PUSHUI", 0x83, DecodeStackDI);
  AddInstTable(InstTable, "TRAP" , 0, DecodeTRAP);
  AddInstTable(InstTable, "BSWAP", 0, DecodeBSWAP);
  if (mIsSuper8())
    AddInstTable(InstTable, "MULT" , 0x84, DecodeMULT_DIV);
  else
    AddInstTable(InstTable, "MULT" , 0, DecodeMULT);
  AddInstTable(InstTable, "DIV" , 0x94, DecodeMULT_DIV);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "BCLR", 0x00, DecodeBit);
  AddInstTable(InstTable, "BSET", 0x80, DecodeBit);
  AddInstTable(InstTable, "BTJ", 0, DecodeBTJ);
  AddInstTable(InstTable, "BTJZ", 0x00, DecodeBtj);
  AddInstTable(InstTable, "BTJNZ", 0x80, DecodeBtj);
  AddInstTable(InstTable, "CPIJNE", 0xd2, DecodeCPIJNE);
  AddInstTable(InstTable, "CPIJE", 0xc2, DecodeCPIJNE);
  AddInstTable(InstTable, "BITC", 0x0057, DecodeBit1);
  AddInstTable(InstTable, "BITR", 0x0077, DecodeBit1);
  AddInstTable(InstTable, "BITS", 0x0177, DecodeBit1);
  AddInstTable(InstTable, "BAND", 0x0167, DecodeBit2);
  AddInstTable(InstTable, "BOR",  0x0107, DecodeBit2);
  AddInstTable(InstTable, "BXOR", 0x0127, DecodeBit2);
  AddInstTable(InstTable, "LDB",  0x0147, DecodeBit2);
  AddInstTable(InstTable, "BCP",  0x0017, DecodeBit2);
  AddInstTable(InstTable, "BTJRF",0x0037, DecodeBitRel);
  AddInstTable(InstTable, "BTJRT",0x0137, DecodeBitRel);

  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "DEFBIT", 0, DecodeDEFBIT);
}

static void DeinitFields(void)
{
  order_array_free(FixedOrders);
  order_array_free(ALU2Orders);
  order_array_free(ALU1Orders);
  order_array_free(ALUXOrders);
  order_array_free(Conditions);

  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_Z8(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on Z8
 * \param  pArg source code argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_Z8(char *pArg, TempResult *pResult)
{
  Byte RegNum;

  if (IsWRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_Z8;
    pResult->Contents.RegDescr.compare = NULL;
  }
  else if (IsWRRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize16Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_Z8;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void MakeCode_Z8(void)
{
  CodeLen = 0; DontPrint = False; OpSize = eSymbolSize8Bit;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_Z8(void)
{
  RPVal = 0;
}

static Boolean IsDef_Z8(void)
{
  return (Memo("SFR") || Memo("REG") || Memo("DEFBIT"));
}

static void SwitchFrom_Z8(void)
{
  DeinitFields();
}

static void AdaptRP01(void)
{
  /* SRP on Super8 sets RP0 & RP1.  Act similar for ASSUME on Super8: */

  if (RPVal >= 0x100)
    RP0Val = RP1Val = RPVal;
  else
  {
    RP0Val = RPVal;
    RP1Val = RPVal + 8;
  }
}

#define ASSUMEeZ8Count 1
#define ASSUMESuper8Count 3
static ASSUMERec ASSUMEeZ8s[] =
{
  {"RP"  , &RPVal  , 0, 0xff, 0x100, AdaptRP01},
  {"RP0" , &RP0Val , 0, 0xff, 0x100, NULL},
  {"RP1" , &RP1Val , 0, 0xff, 0x100, NULL}
};

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_Z8(void *pUser)
 * \brief  prepare to assemble code for this target
 * \param  pUser CPU properties
 * ------------------------------------------------------------------------ */

static void SwitchTo_Z8(void *pUser)
{
  const TFamilyDescr *pDescr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  pCurrCPUProps = (const tCPUProps*)pUser;

  if (mIsSuper8())
    pDescr = FindFamilyByName("Super8");
  else if (mIsZ8Encore())
    pDescr = FindFamilyByName("eZ8");
  else
    pDescr = FindFamilyByName("Z8");

  PCSymbol = "$"; HeaderID = pDescr->Id;
  NOPCode = mIsZ8Encore() ? 0x0f : 0xff;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  if (pCurrCPUProps->CoreFlags & eCoreZ8Encore)
  {
    RegSpaceType = UInt12;
    SegLimits[SegData] = 0xfff;
    ValidSegs |= 1 << SegXData;
    Grans[SegXData] = 1; ListGrans[SegXData] = 1; SegInits[SegXData] = 0;
    SegLimits[SegXData] = 0xffff;
  }
  else
  {
    RegSpaceType = UInt8;
    SegLimits[SegData] = 0xff;
  }

  pASSUMERecs = ASSUMEeZ8s;
  ASSUMERecCnt = mIsSuper8() ? ASSUMESuper8Count : ASSUMEeZ8Count;

  MakeCode = MakeCode_Z8;
  IsDef = IsDef_Z8;
  InternSymbol = InternSymbol_Z8;
  DissectReg = DissectReg_Z8;
  DissectBit = DissectBit_Z8;
  SwitchFrom = SwitchFrom_Z8;
  InitFields();
}

/*!------------------------------------------------------------------------
 * \fn     codez8_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "Z8601"    , eCoreZ8NMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z8603"    , eCoreZ8NMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z86C03"   , eCoreZ8CMOS   , 0xe0,  0x3f,  0xf0 },
  { "Z86E03"   , eCoreZ8CMOS   , 0xe0,  0x3f,  0xf0 },
/*{ "Z8604"    , eCoreZ8       , 0xe0,  0x7f,  0xf0 },*/
  { "Z86C06"   , eCoreZ8CMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z86E06"   , eCoreZ8CMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z86C08"   , eCoreZ8CMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z86C30"   , eCoreZ8CMOS   , 0xe0,  0xef,  0xf0 },
  { "Z86C21"   , eCoreZ8CMOS   , 0xe0,  0xef,  0xf0 },
  { "Z86E21"   , eCoreZ8CMOS   , 0xe0,  0xef,  0xf0 },
  { "Z86C31"   , eCoreZ8CMOS   , 0xe0,  0x7f,  0xf0 },
  { "Z86C32"   , eCoreZ8CMOS   , 0xe0,  0xef,  0xf0 },
  { "Z86C40"   , eCoreZ8CMOS   , 0xe0,  0xef,  0xf0 },
  { "Z88C00"   , eCoreSuper8   , 0xc0,  0xbf,  0xe0 },
  { "Z88C01"   , eCoreSuper8   , 0xc0,  0xbf,  0xe0 },
  { "eZ8"      , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F0113"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F011A"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0123"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F012A"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0130"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0131"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0213"  , eCoreZ8Encore , 0xee0, 0x1ff, 0xf00 },
  { "Z8F021A"  , eCoreZ8Encore , 0xee0, 0x1ff, 0xf00 },
  { "Z8F0223"  , eCoreZ8Encore , 0xee0, 0x1ff, 0xf00 },
  { "Z8F022A"  , eCoreZ8Encore , 0xee0, 0x1ff, 0xf00 },
  { "Z8F0230"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0231"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0411"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0412"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0413"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F041A"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0421"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0422"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0423"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F042A"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0430"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0431"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0811"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0812"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0813"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F081A"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0821"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0822"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0823"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F082A"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F0830"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0831"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F0880"  , eCoreZ8Encore , 0xee0, 0x3ff, 0xf00 },
  { "Z8F1232"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F1233"  , eCoreZ8Encore , 0xee0,  0xff, 0xf00 },
  { "Z8F1621"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F1622"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F1680"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F1681"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F1682"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F2421"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F2422"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F2480"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F3221"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F3222"  , eCoreZ8Encore , 0xee0, 0x7ff, 0xf00 },
  { "Z8F3281"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F3282"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F4821"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F4822"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F4823"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6081"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6082"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6421"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6422"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6423"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6481"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { "Z8F6482"  , eCoreZ8Encore , 0xee0, 0xeff, 0xf00 },
  { NULL       , eCoreNone     , 0x00,  0x00, 0x000 }
};

void codez8_init(void)
{
  const tCPUProps *pRun;

  for (pRun = CPUProps; pRun->pName; pRun++)
    (void)AddCPUUser(pRun->pName, SwitchTo_Z8, (void*)pRun, NULL);

  AddInitPassProc(InitCode_Z8);
}
