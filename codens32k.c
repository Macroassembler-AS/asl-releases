/* codens32k.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator National Semiconductor NS32000                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "bpemu.h"
#include "strutil.h"
#include "ieeefloat.h"

#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "intpseudo.h"

#include "codepseudo.h"
#include "intpseudo.h"
#include "codens32k.h"

typedef enum
{
  eCoreNone,
  eCoreGen1,
  eCoreGen1Ext,
  eCoreGenE,
  eCoreGen2,
  eCoreCount
} tCore;

typedef enum
{
  eFPUNone,
  eFPU16081,
  eFPU32081,
  eFPU32181,
  eFPU32381,
  eFPU32580,
  eFPUCount
} tFPU;

typedef enum
{
  ePMMUNone,
  ePMMU16082,
  ePMMU32082,
  ePMMU32382,
  ePMMU32532,
  ePMMUCount
} tPMMU;

typedef struct
{
  const char *pName;
  Byte CPUType, DefFPU, DefPMMU;
  IntType MemIntType;
} tCPUProps;

static char FPUNames[eFPUCount][8] = { "OFF", "NS16081", "NS32081", "NS32181", "NS32381", "NS32580" };

static char PMMUNames[ePMMUCount][8] = { "OFF", "NS16082", "NS32082", "NS32382", "NS32532" };

typedef struct
{
  const char *pName;
  Word Code, Mask;
  Boolean Privileged;
} tCtlReg;

#ifdef __cplusplus
#include "codens32k.hpp"
#endif

#define MAllowImm (1 << 0)
#define MAllowReg (1 << 1)
#define MAllowRegPair (1 << 2)

static tCtlReg *CtlRegs, *MMURegs;

static tSymbolSize OpSize;
static const tCPUProps *pCurrCPUProps;
static tFPU MomFPU;
static tPMMU MomPMMU;
static Boolean CustomAvail;

/*!------------------------------------------------------------------------
 * \fn     SetOpSize(tSymbolSize Size, const tStrComp *pArg)
 * \brief  set (new) operand size of instruction
 * \param  Size size to set
 * \param  pArg source argument size was deduced from
 * \return True if no conflict
 * ------------------------------------------------------------------------ */

static Boolean SetOpSize(tSymbolSize Size, const tStrComp *pArg)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = Size;
  else if ((Size != eSymbolSizeUnknown) && (Size != OpSize))
  {
    WrStrErrorPos(ErrNum_ConfOpSizes, pArg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ResetOpSize(void)
 * \brief  back to undefined op size
 * \return constat True
 * ------------------------------------------------------------------------ */

static Boolean ResetOpSize(void)
{
  OpSize = eSymbolSizeUnknown;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     CheckCore(unsigned CoreMask)
 * \brief  check for CPU/Core requirement
 * \param  CoreMask bit mask of core type supported
 * \return True if fulfilled
 * ------------------------------------------------------------------------ */

static Boolean CheckCore(unsigned CoreMask)
{
  if ((CoreMask >> pCurrCPUProps->CPUType) & 1)
    return True;
  WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     CheckSup(Boolean Required, const tStrComp *pArg)
 * \brief  check whether supervisor mode requirement is violated
 * \param  Required is supervisor mode required?
 * \param  pArg source argument
 * \return False if violated
 * ------------------------------------------------------------------------ */

static Boolean CheckSup(Boolean Required, const tStrComp *pArg)
{
  if (!SupAllowed && Required)
  {
    WrStrErrorPos(ErrNum_PrivOrder, pArg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     SetMomFPU(tFPU NewFPU)
 * \brief  set new FPU type in use
 * \param  NewFPU new type
 * ------------------------------------------------------------------------ */

static void SetMomFPU(tFPU NewFPU)
{
  switch ((MomFPU = NewFPU))
  {
    case eFPU16081:
    case eFPU32081:
    case eFPU32181:
    case eFPU32381:
    case eFPU32580:
    {
      tStrComp TmpComp;
      String TmpCompStr;

      StrCompMkTemp(&TmpComp, TmpCompStr, sizeof(TmpCompStr));
      strmaxcpy(TmpCompStr, MomFPUIdentName, sizeof(TmpCompStr));
      strmaxcpy(MomFPUIdent, FPUNames[MomFPU], sizeof(MomFPUIdent));
      EnterStringSymbol(&TmpComp, FPUNames[MomFPU], True);
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     SetMomPMMU(tFPU NewPMMU)
 * \brief  set new PMMU type in use
 * \param  NewPMMU new type
 * ------------------------------------------------------------------------ */

static void SetMomPMMU(tPMMU NewPMMU)
{
  switch ((MomPMMU = NewPMMU))
  {
    case ePMMU16082:
    case ePMMU32082:
    case ePMMU32382:
    case ePMMU32532:
    {
      tStrComp TmpComp;
      String TmpCompStr;

      StrCompMkTemp(&TmpComp, TmpCompStr, sizeof(TmpCompStr));
      strmaxcpy(TmpCompStr, MomPMMUIdentName, sizeof(TmpCompStr));
      strmaxcpy(MomPMMUIdent, PMMUNames[MomPMMU], sizeof(MomPMMUIdent));
      EnterStringSymbol(&TmpComp, PMMUNames[MomPMMU], True);
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CheckFPUAvail(void)
 * \brief  check whether FPU instructions are enabled
 * \return False if not
 * ------------------------------------------------------------------------ */

static Boolean CheckFPUAvail(tFPU MinFPU)
{
  if (!FPUAvail)
  {
    WrStrErrorPos(ErrNum_FPUNotEnabled, &OpPart);
    return False;
  }
  else if (MomFPU < MinFPU)
  {
    WrStrErrorPos(ErrNum_FPUInstructionNotSupported, &OpPart);
    return False;
  }

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     CheckPMMUAvail(void)
 * \brief  check whether PMMU instructions are enabled
 * \return False if not
 * ------------------------------------------------------------------------ */

static Boolean CheckPMMUAvail(void)
{
  if (!PMMUAvail)
  {
    WrStrErrorPos(ErrNum_PMMUNotEnabled, &OpPart);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     CheckCustomAvail(void)
 * \brief  check whether Custom instructions are enabled
 * \return False if not
 * ------------------------------------------------------------------------ */

static Boolean CheckCustomAvail(void)
{
  if (!CustomAvail)
  {
    WrStrErrorPos(ErrNum_CustomNotEnabled, &OpPart);
    return False;
  }
  return True;
}

/*--------------------------------------------------------------------------*/
/* Register Handling */

static const char MemRegNames[][3] = { "FP", "SP", "SB" };
#define MemRegCount (sizeof(MemRegNames) / sizeof(*MemRegNames))

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pValue, tSymbolSize *pSize)
 * \brief  check whether argument describes a register
 * \param  pArg source argument
 * \param  pValue register number if yes
 * \param  pSize register size if yes
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pRegNum, tSymbolSize *pRegSize)
{
  unsigned z;

  for (z = 0; z < MemRegCount; z++)
    if (!as_strcasecmp(pArg, MemRegNames[z]))
    {
      *pRegNum = z + 8;
      *pRegSize = eSymbolSize32Bit;
      return True;
    }

  if ((strlen(pArg) != 2) || (pArg[1] < '0') || (pArg[1] > '7'))
    return False;
  *pRegNum = pArg[1] - '0';

  switch (as_toupper(*pArg))
  {
    case 'F': *pRegSize = eSymbolSizeFloat32Bit; return True;
    case 'R': *pRegSize = eSymbolSize32Bit; return True;
    case 'L': *pRegSize = eSymbolSizeFloat64Bit; return True;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_NS32K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - NS32000 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_NS32K(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize32Bit:
      if (Value < 8)
        as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      else if (Value < 8 + MemRegCount)
        as_snprintf(pDest, DestSize, "%s", MemRegNames[Value - 8]);
      else
        goto unknown;
      break;
    case eSymbolSizeFloat32Bit:
      as_snprintf(pDest, DestSize, "F%u", (unsigned)Value);
      break;
    case eSymbolSizeFloat64Bit:
      as_snprintf(pDest, DestSize, "L%u", (unsigned)Value);
      break;
    default:
    unknown:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pValue, tSymbolSize *pSize, tSymbolSize ChkRegSize, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or user-defined register alias
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \param  ChkReqSize explicitly request certain register size
 * \param  MustBeReg expecting register or maybe not?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static Boolean MatchRegSize(tSymbolSize IsSize, tSymbolSize ReqSize)
{
  return (ReqSize == eSymbolSizeUnknown)
      || (IsSize == ReqSize)
      || ((ReqSize == eSymbolSizeFloat64Bit) && (IsSize == eSymbolSizeFloat32Bit));
}

static tRegEvalResult DecodeReg(const tStrComp *pArg, Word *pValue, tSymbolSize *pSize, tSymbolSize ChkRegSize, Boolean MustBeReg)
{
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pValue, &EvalResult.DataSize))
    RegEvalResult = eIsReg;
  else
  {
    tRegDescr RegDescr;

    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSizeUnknown, MustBeReg);
    if (eIsReg == RegEvalResult)
      *pValue = RegDescr.Reg;
  }

  if ((RegEvalResult == eIsReg) && !MatchRegSize(EvalResult.DataSize, ChkRegSize))
  {
    WrStrErrorPos(ErrNum_InvOpSize, pArg);
    RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;
  }

  if (pSize) *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCtlReg(const tStrComp *pArg, Word *pResult)
 * \brief  decode control processor register
 * \param  pArg source argument
 * \param  pResult result buffer
 * \return True if src expr is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeCtlReg(const tStrComp *pArg, Word *pResult)
{
  unsigned z;

  for (z = 0; CtlRegs[z].pName; z++)
    if (!as_strcasecmp(pArg->str.p_str, CtlRegs[z].pName))
    {
      *pResult = CtlRegs[z].Code;
      return CheckSup(CtlRegs[z].Privileged, pArg);
    }
  WrStrErrorPos(ErrNum_InvReg, pArg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMMUReg(const tStrComp *pArg, Word *pResult)
 * \brief  decode MMU register
 * \param  pArg source argument
 * \param  pResult result buffer
 * \return True if src expr is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeMMUReg(const tStrComp *pArg, Word *pResult)
{
  unsigned z;

  for (z = 0; MMURegs[z].pName; z++)
    if (!as_strcasecmp(pArg->str.p_str, MMURegs[z].pName))
    {
      if (!((MMURegs[z].Mask >> MomPMMU) & 1))
      {
        WrStrErrorPos(ErrNum_InvPMMUReg, pArg);
        return False;
      }
      *pResult = MMURegs[z].Code;
      return CheckSup(MMURegs[z].Privileged, pArg);
    }
  WrStrErrorPos(ErrNum_InvPMMUReg, pArg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegList(const tStrComp *pArg, Boolean BitRev, Byte *pResult)
 * \brief  Decode Register List
 * \param  pArg Source Argument
 * \param  BitRev Reverse Bit Order, i.e. R0->Bit 7 ?
 * \param  pResult Result Buffer
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegList(const tStrComp *pArg, Boolean BitRev, Byte *pResult)
{
  tStrComp Part, Remainder;
  int l = strlen(pArg->str.p_str);
  Word Reg;
  char *pSep;

  if ((l < 2)
   || (pArg->str.p_str[0] != '[')
   || (pArg->str.p_str[l - 1] != ']'))
  {
    WrStrErrorPos(ErrNum_InvRegList, pArg);
    return False;
  }

  *pResult = 0;
  StrCompRefRight(&Part, pArg, 1);
  StrCompShorten(&Part, 1);
  while (True)
  {
    KillPrefBlanksStrCompRef(&Part);
    pSep = strchr(Part.str.p_str, ',');
    if (pSep)
      StrCompSplitRef(&Part, &Remainder, &Part, pSep);
    KillPostBlanksStrComp(&Part);
    if (DecodeReg(&Part, &Reg, NULL, eSymbolSize32Bit, True) != eIsReg)
      return False;
    if (Reg >= 8)
    {
      WrStrErrorPos(ErrNum_InvReg, &Part);
      return False;
    }
    *pResult |= 1 << (BitRev ? 7 - Reg : Reg);
    if (pSep)
      Part = Remainder;
    else
      break;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCfgList(const tStrComp *pArg, Byte *pResult)
 * \brief  Decode Config Option List
 * \param  pArg Source Argument
 * \param  pResult Result Buffer
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean DecodeCfgList(const tStrComp *pArg, Byte *pResult)
{
  tStrComp Part, Remainder;
  int l = strlen(pArg->str.p_str);
  char *pSep;
  const char Opts[] = "IFMC", *pOpt;

  if ((l < 2)
   || (pArg->str.p_str[0] != '[')
   || (pArg->str.p_str[l - 1] != ']'))
  {
    WrStrErrorPos(ErrNum_InvCfgList, pArg);
    return False;
  }

  *pResult = 0;
  StrCompRefRight(&Part, pArg, 1);
  StrCompShorten(&Part, 1);
  while (True)
  {
    KillPrefBlanksStrCompRef(&Part);
    pSep = strchr(Part.str.p_str, ',');
    if (pSep)
      StrCompSplitRef(&Part, &Remainder, &Part, pSep);
    KillPostBlanksStrComp(&Part);
    switch (strlen(Part.str.p_str))
    {
      case 0:
        break;
      case 1:
        pOpt = strchr(Opts, as_toupper(*Part.str.p_str));
        if (pOpt)
        {
          *pResult |= 1 << (pOpt - Opts);
          break;
        }
        /* else fall-through */
      default:
        WrStrErrorPos(ErrNum_InvCfgList, &Part);
        return False;
    }
    if (pSep)
      Part = Remainder;
    else
      break;
  }
  return True;
}

/*--------------------------------------------------------------------------*/
/* Address Decoder */

enum
{
  AddrCode_Reg = 0x00,
  AddrCode_Relative = 0x08,
  AddrCode_MemRelative = 0x10,
  AddrCode_Reserved = 0x13,
  AddrCode_Immediate = 0x14,
  AddrCode_Absolute = 0x15,
  AddrCode_External = 0x16,
  AddrCode_TOS = 0x17,
  AddrCode_MemSpace = 0x18,
  AddrCode_ScaledIndex = 0x1c
};

typedef struct
{
  Byte Code;
  Byte Index[1], Disp[8];
  unsigned IndexCnt, DispCnt;
} tAdrVals;

static void ClearAdrVals(tAdrVals *pVals)
{
  pVals->Code = AddrCode_Reserved;
  pVals->IndexCnt = pVals->DispCnt = 0;
}

/*!------------------------------------------------------------------------
 * \fn     EncodeDisplacement(LongInt Disp, tAdrVals *pDest, tErrorNum ErrorNum, const tStrComp *pArg)
 * \brief  encode signed displacement
 * \param  Disp displacement to encode
 * \param  pDest destination buffer
 * \param  ErrorNum error msg to print if out of range
 * \param  pArg source argument for error messages
 * \return True if successfully encoded
 * ------------------------------------------------------------------------ */

static Boolean EncodeDisplacement(LongInt Disp, tAdrVals *pDest, tErrorNum ErrorNum, const tStrComp *pArg)
{
  if ((Disp >= -64) && (Disp <= 63))
  {
    pDest->Disp[pDest->DispCnt++] = Disp & 0x7f;
    return True;
  }
  else if ((Disp >= -8192) && (Disp <= 8191))
  {
    pDest->Disp[pDest->DispCnt++] = 0x80 | ((Disp >> 8) & 0x3f);
    pDest->Disp[pDest->DispCnt++] = Disp & 0xff;
    return True;
  }
  else if ((Disp >= -520093696) && (Disp <= 536870911))
  {
    Byte HiByte = 0xc0 | ((Disp >> 24) & 0x3f);

    if (HiByte == 0xe0)
      goto error;

    pDest->Disp[pDest->DispCnt++] = HiByte;
    pDest->Disp[pDest->DispCnt++] = (Disp >> 16) & 0xff;
    pDest->Disp[pDest->DispCnt++] = (Disp >> 8) & 0xff;
    pDest->Disp[pDest->DispCnt++] = Disp & 0xff;
    return True;
  }
  else
  {
error:
    WrStrErrorPos(ErrorNum, pArg);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     EncodePCRel(const tStrComp *pArg, tAdrVals *pDest)
 * \brief  PC-relative encoding of address
 * \param  pArg address in source
 * \param  pDest dest buffer
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean EncodePCRel(const tStrComp *pArg, tAdrVals *pDest)
{
  tEvalResult EvalResult;
  LongInt Dist = EvalStrIntExpressionWithResult(pArg, pCurrCPUProps->MemIntType, &EvalResult) - EProgCounter();

  ClearAdrVals(pDest);
  if (!EvalResult.OK
   || !EncodeDisplacement(Dist, pDest, mFirstPassUnknownOrQuestionable(EvalResult.Flags) ? ErrNum_None : ErrNum_JmpDistTooBig, pArg))
    return False;

  pDest->Code = AddrCode_MemSpace + 3;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(tStrComp *pArg, tAdrVals *pDest, unsigned AddrModeMask)
 * \brief  decode address expression
 * \param  pArg source argument
 * \param  pDest dest buffer
 * \param  AddrModeMask allow immediate/register addressing?
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr(const tStrComp *pArg, tAdrVals *pDest, Boolean AddrModeMask)
{
  Byte IndexCode = 0;
  Boolean OK, NeedEvenReg;
  const Boolean IsFloat = (OpSize == eSymbolSizeFloat32Bit) || (OpSize == eSymbolSizeFloat64Bit);
  Word RegValue, IndexReg = 0;
  int ArgLen, SplitPos;
  tStrComp BaseArg;
  const char IndexChars[] = "BWDQ";
  char *pSplit, *pSplit2, Scale;

  ClearAdrVals(pDest);

  /* split off scaled indexing, which is orthogonal to (almost) all other modes */

  pSplit = MatchCharsRev(pArg->str.p_str, " : ? ] ", IndexChars, &Scale);
  pSplit2 = (pSplit && (pSplit > pArg->str.p_str))
          ? FindOpeningParenthese(pArg->str.p_str, pSplit - 1, "[]")
          : NULL;
  if (pSplit && pSplit2)
  {
    tStrComp Mid, Right;
    const char *pScaleIndex;

    StrCompSplitRef(&BaseArg, &Right, pArg, pSplit);
    StrCompSplitRef(&BaseArg, &Mid, &BaseArg, pSplit2);
    KillPrefBlanksStrCompRef(&Mid);
    KillPostBlanksStrComp(&BaseArg);
    pArg = &BaseArg;

    pScaleIndex = strchr(IndexChars, as_toupper(Scale));
    if (!pScaleIndex)
    {
      WrStrErrorPos(ErrNum_InvScale, &Right);
      return False;
    }
    IndexCode = AddrCode_ScaledIndex + (pScaleIndex - IndexChars);
    if ((DecodeReg(&Mid, &IndexReg, NULL, eSymbolSize32Bit, True) == eIsReg)
     && (IndexReg >= 8))
    {
      WrStrErrorPos(ErrNum_InvReg, &Mid);
      return False;
    }

    /* Immediate cannot be combined with scaling, so disallow regardless of caller's
       preference: */

    AddrModeMask &= ~MAllowImm;
  }

  /* absolute: */

  if (*pArg->str.p_str == '@')
  {
    tStrComp Arg;
    LongWord Addr;

    StrCompRefRight(&Arg, pArg, 1);
    Addr = EvalStrIntExpression(&Arg, pCurrCPUProps->MemIntType, &OK);
    if (!OK)
      return False;

    /* On a CPU with non-32-bit address bus, addresses @ the upper end may be encoded
       as negative 'displacements': */

    if ((pCurrCPUProps->MemIntType == UInt24)
     && (Addr & 0x800000ul))
      Addr |= 0xff000000ul;
    if (EncodeDisplacement((LongInt)Addr, pDest, ErrNum_OverRange, &Arg))
    {
      pDest->Code = AddrCode_Absolute;
      goto chk;
    }
    else
      return False;
  }

  /* TOS? */

  if (!as_strcasecmp(pArg->str.p_str, "TOS"))
  {
    pDest->Code = AddrCode_TOS;
    goto chk;
  }

  /* register? */

  if (IsFloat)
    NeedEvenReg = (OpSize == eSymbolSizeFloat64Bit) && (MomFPU < eFPU32181);
  else
    NeedEvenReg = !!(AddrModeMask & MAllowRegPair);
  switch (DecodeReg(pArg, &RegValue, NULL, IsFloat ? OpSize : eSymbolSize32Bit, False))
  {
    case eIsReg:
      if (NeedEvenReg && (RegValue & 1))
      {
        WrStrErrorPos(ErrNum_InvRegPair, pArg);
        return False;
      }
      else if (!(AddrModeMask & (MAllowReg | MAllowRegPair)))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return False;
      }
      else
      {
        pDest->Code = AddrCode_Reg + RegValue;
        goto chk;
      }
    case eIsNoReg:
      break;
    default:
      return False;
  }

  /* EXT mode */

  pSplit = MatchChars(pArg->str.p_str, " EXT (");
  if (pSplit)
  {
    tStrComp Left, Mid, Right;
    LongInt Disp1, Disp2 = 0;
    Boolean OK;

    StrCompSplitRef(&Left, &Mid, pArg, pSplit - 1);
    pSplit = FindClosingParenthese(Mid.str.p_str);
    if (!pSplit)
    {
      WrStrErrorPos(ErrNum_BrackErr, pArg);
      return False;
    }
    StrCompSplitRef(&Mid, &Right, &Mid, pSplit);
    Disp1 = EvalStrIntExpression(&Mid, Int30, &OK);
    if (!OK)
      return False;
    *(--Right.str.p_str) = '0'; Right.Pos.Len++; Right.Pos.StartCol--;
    if (*Right.str.p_str)
      Disp2 = EvalStrIntExpression(&Right, Int30, &OK);
    if (!OK)
      return False;

    pDest->Code = AddrCode_External;
    if (!EncodeDisplacement(Disp1, pDest, ErrNum_OverRange, &Mid)
     || !EncodeDisplacement(Disp2, pDest, ErrNum_OverRange, &Right))
      return False;
    goto chk;
  }

  /* PC-relative */

  if (!strcmp(pArg->str.p_str, "*")
   || MatchChars(pArg->str.p_str, "* ?", "+-", NULL))
  {
    LongInt Disp;
    Boolean OK;

    *pArg->str.p_str = '0';
    Disp = EvalStrIntExpression(pArg, Int30, &OK);
    *pArg->str.p_str = '*';
    if (!OK
     || !EncodeDisplacement(Disp, pDest, ErrNum_OverRange, pArg))
      return False;
    pDest->Code = AddrCode_MemSpace + 3;
    goto chk;
  }

  /* disp(...)? */

  SplitPos = FindDispBaseSplit(pArg->str.p_str, &ArgLen);
  if (SplitPos >= 0)
  {
    tStrComp OutDisp, InnerArg;
    LongInt OutDispVal;
    tEvalResult OutEvalResult;

    memset(&OutEvalResult, 0, sizeof(OutEvalResult));
    StrCompSplitRef(&OutDisp, &InnerArg, pArg, &pArg->str.p_str[SplitPos]);
    if (OutDisp.Pos.Len)
    {
      OutDispVal = EvalStrIntExpressionWithResult(&OutDisp, Int30, &OutEvalResult);
      if (!OutEvalResult.OK)
        return False;
    }
    else
    {
      OutEvalResult.OK = True;
      OutDispVal = 0;
    }

    StrCompShorten(&InnerArg, 1);
    KillPrefBlanksStrCompRef(&InnerArg);
    KillPostBlanksStrComp(&InnerArg);
    switch (DecodeReg(&InnerArg, &RegValue, NULL, eSymbolSize32Bit, False))
    {
      case eIsReg: /* disp(Rn/FP/SP/SB) */
        pDest->Code = (RegValue < 8) ? AddrCode_Relative + RegValue : AddrCode_MemSpace + (RegValue - 8);
        if (!EncodeDisplacement(OutDispVal, pDest, ErrNum_OverRange, &OutDisp))
          return False;
        goto chk;
      IsPCRel:
        if (EncodeDisplacement(OutDispVal - EProgCounter(), pDest, mFirstPassUnknownOrQuestionable(OutEvalResult.Flags) ? ErrNum_None : ErrNum_JmpDistTooBig, &OutDisp))
          pDest->Code = AddrCode_MemSpace + 3;
        goto chk;
      case eIsNoReg:
      {
        tStrComp InDisp;
        LongInt InDispVal = 0;

        if (!as_strcasecmp(InnerArg.str.p_str, "PC"))
          goto IsPCRel;

        SplitPos = FindDispBaseSplit(InnerArg.str.p_str, &ArgLen);
        if (SplitPos < 0)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &InnerArg);
          return False;
        }
        StrCompSplitRef(&InDisp, &InnerArg, &InnerArg, &InnerArg.str.p_str[SplitPos]);
        if (InDisp.Pos.Len)
        {
          InDispVal = EvalStrIntExpression(&InDisp, Int30, &OK);
          if (!OK)
            return False;
        }

        StrCompShorten(&InnerArg, 1);
        KillPrefBlanksStrCompRef(&InnerArg);
        KillPostBlanksStrComp(&InnerArg);

        /* disp2(disp1(ext)) is an alias for EXT(disp1)+disp2 */

        if (!as_strcasecmp(InnerArg.str.p_str, "EXT"))
        {
          pDest->Code = AddrCode_External;
        }

        /* disp2(disp1(FP/SP/SB)) */

        else
        {
          if (DecodeReg(&InnerArg, &RegValue, NULL, eSymbolSize32Bit, True) != eIsReg)
            return False;
          if (RegValue < 8)
          {
            WrStrErrorPos(ErrNum_InvReg, &InnerArg);
            return False;
          }
          pDest->Code = AddrCode_MemRelative + (RegValue - 8);
        }
        if (!EncodeDisplacement(InDispVal, pDest, ErrNum_OverRange, &InDisp)
         || !EncodeDisplacement(OutDispVal, pDest, ErrNum_OverRange, &OutDisp))
          return False;
        goto chk;
      }
      default:
        return False;
    }
  }

  /* -> immediate or PC-relative */

  if (AddrModeMask & MAllowImm)
  {
    switch (OpSize)
    {
      case eSymbolSize8Bit:
        pDest->Disp[0] = EvalStrIntExpression(pArg, Int8, &OK);
        if (OK)
          pDest->DispCnt = 1;
        break;
      case eSymbolSize16Bit:
      {
        Word Val = EvalStrIntExpression(pArg, Int16, &OK);
        if (OK)
        {
          pDest->Disp[pDest->DispCnt++] = Hi(Val);
          pDest->Disp[pDest->DispCnt++] = Lo(Val);
        }
        break;
      }
      case eSymbolSize32Bit:
      {
        LongWord Val = EvalStrIntExpression(pArg, Int32, &OK);
        if (OK)
        {
          pDest->Disp[pDest->DispCnt++] = (Val >> 24) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >> 16) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >>  8) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >>  0) & 0xff;
        }
        break;
      }
      case eSymbolSize64Bit:
      {
        LargeWord Val = EvalStrIntExpression(pArg, LargeIntType, &OK);
        if (OK)
        {
#ifdef HAS64
          pDest->Disp[pDest->DispCnt++] = (Val >> 56) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >> 48) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >> 40) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >> 32) & 0xff;
#else
          pDest->Disp[pDest->DispCnt + 0] =
          pDest->Disp[pDest->DispCnt + 1] =
          pDest->Disp[pDest->DispCnt + 2] =
          pDest->Disp[pDest->DispCnt + 3] = (Val & 0x80000000ul) ? 0xff : 0x00;
          pDest->DispCnt += 4;
#endif
          pDest->Disp[pDest->DispCnt++] = (Val >> 24) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >> 16) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >>  8) & 0xff;
          pDest->Disp[pDest->DispCnt++] = (Val >>  0) & 0xff;
        }
        break;
      }
      case eSymbolSizeFloat32Bit:
      {
        Double Val = EvalStrFloatExpression(pArg, Float32, &OK);
        if (OK)
        {
          Double_2_ieee4(Val, pDest->Disp, True);
          pDest->DispCnt = 4;
        }
        break;
      }
      case eSymbolSizeFloat64Bit:
      {
        Double Val = EvalStrFloatExpression(pArg, Float64, &OK);
        if (OK)
        {
          Double_2_ieee8(Val, pDest->Disp, True);
          pDest->DispCnt = 8;
        }
        break;
      }
      default:
        WrStrErrorPos(ErrNum_UndefOpSizes, pArg);
    }
    if (pDest->DispCnt)
      pDest->Code = AddrCode_Immediate;
    else
      return False;
  }
  else if (!EncodePCRel(pArg, pDest))
    return False;

chk:
  /* if we have an index code, check it's not immedate, otherwise relocate */

  if (IndexCode)
  {
    if (pDest->Code == AddrCode_Immediate)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    }
    pDest->Index[pDest->IndexCnt++] = (pDest->Code << 3) | IndexReg;
    pDest->Code = IndexCode;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     AppendIndex(const tAdrVals *pVals)
 * \brief  append optional index byte to code
 * \param  pVals encoded addressing mode
 * ------------------------------------------------------------------------ */

static void AppendIndex(const tAdrVals *pVals)
{
  if (pVals->IndexCnt)
  {
    memcpy(&BAsmCode[CodeLen], pVals->Index, pVals->IndexCnt);
    CodeLen += pVals->IndexCnt;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AppendDisp(const tAdrVals *pVals)
 * \brief  append optional displacement to code
 * \param  pVals encoded addressing mode
 * ------------------------------------------------------------------------ */

static void AppendDisp(const tAdrVals *pVals)
{
  if (pVals->DispCnt)
  {
    memcpy(&BAsmCode[CodeLen], pVals->Disp, pVals->DispCnt);
    CodeLen += pVals->DispCnt;
  }
}

/*--------------------------------------------------------------------------*/
/* Helper Functions */

/*!------------------------------------------------------------------------
 * \fn     SizeCodeI(tSymbolSize Size)
 * \brief  transform (integer) operand size to i size in instruction
 * \param  Size Operand Size
 * \return Size Code
 * ------------------------------------------------------------------------ */

static LongWord SizeCodeI(tSymbolSize Size)
{
  switch (Size)
  {
    case eSymbolSize8Bit:
      return 0;
    case eSymbolSize16Bit:
      return 1;
    case eSymbolSize32Bit:
      return 3;
    default:
      return 2;
  }
}

/*!------------------------------------------------------------------------
 * \fn     SizeCodeF(tSymbolSize Size)
 * \brief  transform (float) operand size to f size in instruction
 * \param  Size Operand Size
 * \return Size Code
 * ------------------------------------------------------------------------ */

static LongWord SizeCodeF(tSymbolSize Size)
{
  switch (Size)
  {
    case eSymbolSizeFloat64Bit:
      return 0;
    case eSymbolSizeFloat32Bit:
      return 1;
    default:
      return 0xff;
  }
}

/*!------------------------------------------------------------------------
 * \fn     SizeCodeC(tSymbolSize Size)
 * \brief  transform (custom) operand size to c size in instruction
 * \param  Size Operand Size
 * \return Size Code
 * ------------------------------------------------------------------------ */

static LongWord SizeCodeC(tSymbolSize Size)
{
  switch (Size)
  {
    case eSymbolSize64Bit:
      return 0;
    case eSymbolSize32Bit:
      return 1;
    default:
      return 0xff;
  }
}

/*!------------------------------------------------------------------------
 * \fn     GetOpSizeFromCode(Word Code)
 * \brief  get operand size of instruction from insn name
 * \param  Code contains size in MSB
 * \return operand size
 * ------------------------------------------------------------------------ */

static tSymbolSize GetOpSizeFromCode(Word Code)
{
  Byte Size = Hi(Code) & 15;

  return (Size == 0xff) ? eSymbolSizeUnknown : (tSymbolSize)Size;
}

/*!------------------------------------------------------------------------
 * \fn     SetOpSizeFromCode(Word Code)
 * \brief  set operand size of instruction from insn code MSB
 * \param  Code contains size in MSB
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean SetOpSizeFromCode(Word Code)
{
  return SetOpSize(GetOpSizeFromCode(Code), &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     SetFOpSizeFromCode(Word Code)
 * \brief  set FP operand size of instruction from insn code
 * \param  Code contains size in LSB
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean SetFOpSizeFromCode(Word Code)
{
  return SetOpSize((Code & 1) ? eSymbolSizeFloat32Bit : eSymbolSizeFloat64Bit, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     SetCOpSizeFromCode(Word Code)
 * \brief  set custom operand size of instruction from insn code
 * \param  Code contains size in LSB
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean SetCOpSizeFromCode(Word Code)
{
  return SetOpSize((Code & 1) ? eSymbolSize32Bit : eSymbolSize64Bit, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     PutCode(LongWord Code, unsigned Count)
 * \brief  write instruction opcode to output
 * \param  Code opcode to write
 * \param  Count # of bytes to write
 * ------------------------------------------------------------------------ */

static void PutCode(LongWord Code, unsigned Count)
{
  BAsmCode[CodeLen++] = Code & 0xff;
  while (Count > 1)
  {
    Code >>= 8;
    Count--;
    BAsmCode[CodeLen++] = Code & 0xff;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkNoAttrPart(void)
 * \brief  check for no attribute part
 * \return True if no attribute
 * ------------------------------------------------------------------------ */

static Boolean ChkNoAttrPart(void)
{
  if (*AttrPart.str.p_str)
  {
    WrError(ErrNum_UseLessAttr);
    return False;
  }
  return True;
}

/*--------------------------------------------------------------------------*/
/* Instruction De/Encoders */

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  decode instructions without argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code, !!Hi(Code));
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat0(Word Code)
 * \brief  Decode Format 0 Instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat0(Word Code)
{
  tAdrVals AdrVals;

  ClearAdrVals(&AdrVals);
  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && EncodePCRel(&ArgStr[1], &AdrVals))
  {
    PutCode(Code, 1);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRET(Word Code)
 * \brief  Decode RET/RETT/RXP Instructions
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeRET(Word Code)
{
  if (ChkArgCnt(0, 1)
   &&  ChkNoAttrPart())
  {
    tEvalResult EvalResult;
    LongInt Value;
    tAdrVals AdrVals;

    if (ArgCnt >= 1)
      Value = EvalStrIntExpressionWithResult(&ArgStr[1], SInt30, &EvalResult);
    else
    {
      Value = 0;
      EvalResult.OK = True;
      EvalResult.Flags = eSymbolFlag_None;
    }

    ClearAdrVals(&AdrVals);
    if (EncodeDisplacement(Value, &AdrVals, mFirstPassUnknownOrQuestionable(EvalResult.Flags) ? ErrNum_None : ErrNum_OverRange, &ArgStr[1]))
    {
      PutCode(Code, 1);
      AppendDisp(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCXP(Word Code)
 * \brief  Handle CXP Instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCXP(Word Code)
{
  if (ChkArgCnt(1, 1)
   &&  ChkNoAttrPart())
  {
    tEvalResult EvalResult;
    LongInt Value = EvalStrIntExpressionWithResult(&ArgStr[1], SInt30, &EvalResult);
    tAdrVals AdrVals;

    ClearAdrVals(&AdrVals);
    if (EncodeDisplacement(Value, &AdrVals, mFirstPassUnknownOrQuestionable(EvalResult.Flags) ? ErrNum_None : ErrNum_OverRange, &ArgStr[1]))
    {
      PutCode(Code, 1);
      AppendDisp(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeENTER(Word Code)
 * \brief  Handle ENTER Instruction (Format 1)
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeENTER(Word Code)
{
  Byte RegList;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && DecodeRegList(&ArgStr[1], False, &RegList))
  {
    tEvalResult EvalResult;
    LongInt Value = EvalStrIntExpressionWithResult(&ArgStr[2], SInt30, &EvalResult);
    tAdrVals AdrVals;

    ClearAdrVals(&AdrVals);
    if (EncodeDisplacement(Value, &AdrVals, mFirstPassUnknownOrQuestionable(EvalResult.Flags) ? ErrNum_None : ErrNum_OverRange, &ArgStr[1]))
    {
      PutCode(Code, 1);
      BAsmCode[CodeLen++] = RegList;
      AppendDisp(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEXIT(Word Code)
 * \brief  Handle EXIT Instruction (Format 1)
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeEXIT(Word Code)
{
  Byte RegList;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && DecodeRegList(&ArgStr[1], True, &RegList))
  {
    PutCode(Code, 1);
    BAsmCode[CodeLen++] = RegList;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSAVE_RESTORE(Word Code)
 * \brief  Decode SAVE/RESTORE Instructions
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeSAVE_RESTORE(Word Code)
{
  Byte RegList;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && DecodeRegList(&ArgStr[1], !!(Code & 0x10), &RegList))
  {
    PutCode(Code, 1);
    PutCode(RegList, 1);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCINV(Word Code)
 * \brief  handle CINV instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCINV(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(2, 4)
   && ChkNoAttrPart()
   && CheckCore(1 << eCoreGen2)
   && CheckSup(True, &OpPart)
   && DecodeAdr(&ArgStr[ArgCnt], &AdrVals, MAllowReg))
  {
    LongWord ActCode = (((LongWord)AdrVals.Code) << 19)
                     | Code;
    int z;

    for (z = 1; z < ArgCnt; z++)
      if (!as_strcasecmp(ArgStr[z].str.p_str, "A"))
        ActCode |= 1ul << 17;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "I"))
        ActCode |= 1ul << 16;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "D"))
        ActCode |= 1ul << 15;
      else
      {
        WrStrErrorPos(ErrNum_InvCacheInvMode, &ArgStr[z]);
        return;
      }
    PutCode(ActCode, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLPR_SPR(Word Code)
 * \brief  Decode LPR/SPR Instructions (Format 2)
 * \param  Code Machine Code & Operand Size
 * ------------------------------------------------------------------------ */

static void DecodeLPR_SPR(Word Code)
{
  tAdrVals SrcAdrVals;
  Word Reg;
  Boolean IsSPR = (Lo(Code) == 2);

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &SrcAdrVals, MAllowReg | (IsSPR ? 0 : MAllowImm))
   && DecodeCtlReg(&ArgStr[1], &Reg))
  {
    PutCode((((Word)SrcAdrVals.Code) << 11)
          | (Reg << 7)
          | (Lo(Code) << 4)
          | (SizeCodeI(OpSize) << 0)
          | 0x0c, 2);
    AppendIndex(&SrcAdrVals);
    AppendDisp(&SrcAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeScond(Word Code)
 * \brief  Handloe Scond Instructions
 * \param  Code condition & operand size
 * ------------------------------------------------------------------------ */

static void DecodeScond(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg))
  {
    PutCode((((Word)AdrVals.Code) << 11)
          | ((Code & 0xff) << 7)
          | (SizeCodeI(OpSize) << 0)
          | 0x3c, 2);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVQ(Word Code)
 * \brief  Handle MOVQ/ADDQ/CMPQ Instructions
 * \param  Code Machine Code & Operand Size
 * ------------------------------------------------------------------------ */

#define MOVQ_DESTMAYIMM 0x80

static void DecodeMOVQ(Word Code)
{
  tAdrVals DestAdrVals;
  Boolean DestMayImm = !!(Code & MOVQ_DESTMAYIMM);

  Code &= ~MOVQ_DESTMAYIMM;
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg | (DestMayImm ? MAllowImm : 0)))
  {
    Boolean OK;
    Integer Val = EvalStrIntExpression(&ArgStr[1], SInt4, &OK);

    if (OK)
    {
      PutCode((((Word)DestAdrVals.Code) << 11)
            | ((Val & 0x0f) << 7)
            | (Lo(Code) << 4)
            | (SizeCodeI(OpSize) << 0)
            | 0x0c, 2);
      AppendIndex(&DestAdrVals);
      AppendDisp(&DestAdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeACB(Word Code)
 * \brief  handle ACBi Instrucion
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeACB(Word Code)
{
  tAdrVals DestAdrVals;

  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    Boolean OK;
    tAdrVals DistAdrVals;
    Integer Val = EvalStrIntExpression(&ArgStr[1], SInt4, &OK);

    if (OK && EncodePCRel(&ArgStr[3], &DistAdrVals))
    {
      PutCode((((Word)DestAdrVals.Code) << 11)
            | ((Val & 0x0f) << 7)
            | (Lo(Code) << 4)
            | (SizeCodeI(OpSize) << 0)
            | 0x0c, 2);
      AppendIndex(&DestAdrVals);
      AppendDisp(&DestAdrVals);
      AppendDisp(&DistAdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat3(Word Code)
 * \brief  Decode Format 3 Instructions
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeFormat3(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg | MAllowImm))
  {
    PutCode((((Word)AdrVals.Code) << 11)
          | (Lo(Code) << 7)
          | (SizeCodeI(OpSize) << 0)
          | 0x7c, 2);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat4(Word Code)
 * \brief  decode Format 4 instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

#define FMT4_SRCISADDR 0x80
#define FMT4_DESTMAYIMM 0x40

static void DecodeFormat4(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean SrcIsAddr = !!(Code & FMT4_SRCISADDR),
          DestMayImm = !!(Code & FMT4_DESTMAYIMM);

  Code &= ~(FMT4_SRCISADDR | FMT4_DESTMAYIMM);
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | (SrcIsAddr ? 0 : MAllowImm))
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg | (DestMayImm ? MAllowImm : 0)))
  {
    PutCode((((Word)SrcAdrVals.Code) << 11)
          | (((Word)DestAdrVals.Code) << 6)
          | (Lo(Code) << 2)
          | (SizeCodeI(OpSize)), 2);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVS_CMPS_SKPS(Word Code)
 * \brief  Handle MOVSi/CMPSi/SKPSi Instructions (Format 5)
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeMOVS_CMPS_SKPS(Word Code)
{
  if (ChkArgCnt(0, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code))
  {
    Byte Options = 0;
    int z;

    for (z = 1; z <= ArgCnt; z++)
    {
      if (!as_strcasecmp(ArgStr[z].str.p_str, "B"))
        Options |= 1;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "U"))
      {
        if (Options & 6)
        {
          WrStrErrorPos(ErrNum_ConfStringOpt, &ArgStr[z]);
          return;
        }
        else
          Options |= 6;
      }
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "W"))
      {
        if (Options & 6)
        {
          WrStrErrorPos(ErrNum_ConfStringOpt, &ArgStr[z]);
          return;
        }
        else
          Options |= 2;
      }
      else
      {
        WrStrErrorPos(ErrNum_UnknownStringOpt, &ArgStr[z]);
        return;
      }
    }
    PutCode((((LongWord)Options) << 16)
          | ((Code & 0xff) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x0e, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSETCFG(Word Code)
 * \brief  Handle SETCFG Instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSETCFG(Word Code)
{
  Byte CfgOpts;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && CheckSup(True, &OpPart)
   && DecodeCfgList(&ArgStr[1], &CfgOpts))
    PutCode((((LongWord)CfgOpts) << 15)
            | Code, 3);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat6(Word Code)
 * \brief  Decode Format 6 Instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

#define FMT6_SRC8BIT 0x80

static void DecodeFormat6(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean SrcIs8Bit = !!(Code & FMT6_SRC8BIT);

  Code &= ~FMT6_SRC8BIT;
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(SrcIs8Bit ? 0x0000 : Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x4e, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat7(Word Code)
 * \brief  Decode Format 7 Instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat7(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0xce, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVM_CMPM(Word Code)
 * \brief  Decode MOVMi/CMPMi Instruction (Format 7)
 * \param  Code machine code & size
 * ------------------------------------------------------------------------ */

static void DecodeMOVM_CMPM(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    tEvalResult EvalResult;
    LongWord Count = EvalStrIntExpressionWithResult(&ArgStr[3], UInt30, &EvalResult);

    if (EvalResult.OK)
    {
      tAdrVals CntAdrVals;

      Count = (Count - 1) << OpSize;
      ClearAdrVals(&CntAdrVals);
      if (EncodeDisplacement(Count, &CntAdrVals, mFirstPassUnknownOrQuestionable(EvalResult.Flags) ? ErrNum_None : ErrNum_OverRange, &ArgStr[3]))
      {
        PutCode((((LongWord)SrcAdrVals.Code) << 19)
            | (((LongWord)DestAdrVals.Code) << 14)
            | (Lo(Code) << 10)
            | (SizeCodeI(OpSize) << 8)
            | 0xce, 3);
        AppendIndex(&SrcAdrVals);
        AppendIndex(&DestAdrVals);
        AppendDisp(&SrcAdrVals);
        AppendDisp(&DestAdrVals);
        AppendDisp(&CntAdrVals);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINSS_EXTS(Word Code)
 * \brief  Handle INSS/EXTS Instructions (Format 7)
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeINSS_EXTS(Word Code)
{
  tAdrVals Arg1AdrVals, Arg2AdrVals;

  if (ChkArgCnt(4, 4)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &Arg1AdrVals, MAllowReg | ((Lo(Code) == 2) ? MAllowImm : 0))
   && DecodeAdr(&ArgStr[2], &Arg2AdrVals, MAllowReg))
  {
    tEvalResult EvalResult;
    Byte Offs, Length;

    Offs = EvalStrIntExpressionWithResult(&ArgStr[3], UInt3, &EvalResult);
    if (!EvalResult.OK)
      return;

    Length = EvalStrIntExpressionWithResult(&ArgStr[4], UInt5, &EvalResult);
    if (!EvalResult.OK)
      return;
    if (mFirstPassUnknownOrQuestionable(EvalResult.Flags))
      Length = (Length & 31) + 1;
    if (ChkRange(Length, 1, 32))
    {
      PutCode((((LongWord)Arg1AdrVals.Code) << 19)
            | (((LongWord)Arg2AdrVals.Code) << 14)
            | (SizeCodeI(OpSize) << 8)
            | (Lo(Code) << 10)
            | 0xce, 3);
      AppendIndex(&Arg1AdrVals);
      AppendIndex(&Arg2AdrVals);
      AppendDisp(&Arg1AdrVals);
      AppendDisp(&Arg2AdrVals);
      BAsmCode[CodeLen++] = ((Offs & 7) << 5) | ((Length - 1) & 31);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVExt(Word Code)
 * \brief  Decode MOV{X|Z}{BW|BD|WD} Instructions (Format 7)
 * \param  Code machine code & operand sizes
 * ------------------------------------------------------------------------ */

static void DecodeMOVExt(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Byte SrcOpSize = (Code >> 8) & 15,
       DestOpSize = (Code >> 12) & 15;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSize((tSymbolSize)SrcOpSize, &OpPart)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm))
  {
    OpSize = (tSymbolSize)DestOpSize;

    if (DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
    {
      Code = ((DestOpSize == eSymbolSize32Bit) ? 6 : 5) ^ (Code & 1);
      PutCode((((LongWord)SrcAdrVals.Code) << 19)
            | (((LongWord)DestAdrVals.Code) << 14)
            | (Code << 10)
            | (((LongWord)SrcOpSize) << 8)
            | 0xce, 3);
      AppendIndex(&SrcAdrVals);
      AppendIndex(&DestAdrVals);
      AppendDisp(&SrcAdrVals);
      AppendDisp(&DestAdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDoubleDest(Word Code)
 * \brief  handle instruction with double-sized dest operand (Format 8)
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeDoubleDest(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowRegPair))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0xce, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCHECK_INDEX(Word Code)
 * \brief  Handle CHECK/CVTP/INDEX Instructions (Format 8)
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeCHECK_INDEX(Word Code)
{
  tAdrVals BoundsAdrVals, SrcAdrVals;
  Word DestReg;
  Boolean IsINDEX = Lo(Code) == 0x42,
          IsCVTP = Lo(Code) == 0x06;


  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &BoundsAdrVals, IsINDEX ? (MAllowReg | MAllowImm) : 0)
   && DecodeAdr(&ArgStr[3], &SrcAdrVals, MAllowReg | (IsCVTP ? 0 : MAllowImm))
   && DecodeReg(&ArgStr[1], &DestReg, NULL, eSymbolSize32Bit, True))
  {
    PutCode((((LongWord)BoundsAdrVals.Code) << 19)
        | (((LongWord)SrcAdrVals.Code) << 14)
        | (DestReg << 11)
        | (SizeCodeI(OpSize) << 8)
        | (Lo(Code) << 4)
        | 0x0e, 3);
    AppendIndex(&BoundsAdrVals);
    AppendIndex(&SrcAdrVals);
    AppendDisp(&BoundsAdrVals);
    AppendDisp(&SrcAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINS_EXT(Word Code)
 * \brief  Handle INS/EXT Instructions (Format 8)
 * \param  IsINS 1 for INS, 0 for EXT
 * ------------------------------------------------------------------------ */

static void DecodeINS_EXT(Word IsINS)
{
  tAdrVals Arg2AdrVals, Arg3AdrVals;
  Word OffsetReg;

  if (ChkArgCnt(4, 4)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(IsINS)
   && DecodeReg(&ArgStr[1], &OffsetReg, NULL, eSymbolSize32Bit, True)
   && DecodeAdr(&ArgStr[2], &Arg2AdrVals, MAllowReg | (Lo(IsINS) ? MAllowImm : 0))
   && DecodeAdr(&ArgStr[3], &Arg3AdrVals, MAllowReg))
  {
    tEvalResult EvalResult;
    LongInt Disp = EvalStrIntExpressionWithResult(&ArgStr[4], SInt30, &EvalResult);

    if (EvalResult.OK)
    {
      tAdrVals DispAdrVals;

      ClearAdrVals(&DispAdrVals);
      if (EncodeDisplacement(Disp, &DispAdrVals, ErrNum_OverRange, &ArgStr[4]))
      {
        PutCode((((LongWord)Arg2AdrVals.Code) << 19)
              | (((LongWord)Arg3AdrVals.Code) << 14)
              | (OffsetReg << 11)
              | (SizeCodeI(OpSize) << 8)
              | (Lo(IsINS) << 7)
              | 0x2e, 3);
        AppendIndex(&Arg2AdrVals);
        AppendIndex(&Arg3AdrVals);
        AppendDisp(&Arg2AdrVals);
        AppendDisp(&Arg3AdrVals);
        AppendDisp(&DispAdrVals);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFFS(Word Code)
 * \brief  Handle FFS Instruction
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeFFS(Word Code)
{
  tAdrVals BaseAdrVals, OffsetAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &BaseAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &OffsetAdrVals, MAllowReg))
  {
    PutCode((((LongWord)BaseAdrVals.Code) << 19)
          | (((LongWord)OffsetAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x6e, 3);
    AppendIndex(&BaseAdrVals);
    AppendIndex(&OffsetAdrVals);
    AppendDisp(&BaseAdrVals);
    AppendDisp(&OffsetAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVSU(Word Code)
 * \brief  Handle MOVSU/MOVUS Instruction
 * \param  Code machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeMOVSU(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckSup(True, &OpPart)
   && CheckCore((1 << eCoreGen1) | (1 << eCoreGen2))
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, 0)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, 0))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | ((Code & 0xff) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0xae, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFLOOR_ROUND_TRUNC(Word Code)
 * \brief  Handle FLOOR.../ROUND.../TRUNC... instructions (Format 9)
 * \param  Code machine code & (integer) op size
 * ------------------------------------------------------------------------ */

static void DecodeFLOOR_ROUND_TRUNC(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckFPUAvail(eFPU16081)
   && SetFOpSizeFromCode(Code & 0x1)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x3e, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVif(Word Code)
 * \brief  Handle MOV i->f Instructions (Format 9)
 * \param  Code machine code & (integer) op size
 * ------------------------------------------------------------------------ */

static void DecodeMOVif(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckFPUAvail(eFPU16081)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetFOpSizeFromCode(Code & 0x1)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | ((Code & 0xff) << 10)
          | (SizeCodeI(GetOpSizeFromCode(Code)) << 8)
          | 0x3e, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormatLFSR_SFSR(Word Code)
 * \brief  Handle LFSR/SFSR Instructions (Format 9)
 * \param  Code Machine Code & Flags
 * ------------------------------------------------------------------------ */

#define FMT9_MAYIMM (1 << 15)

static void DecodeLFSR_SFSR(Word Code)
{
  Boolean MayImm = !!(Code & FMT9_MAYIMM);
  tAdrVals AdrVals;

  Code &= ~FMT9_MAYIMM;
  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && CheckFPUAvail(eFPU16081)
   && SetOpSize(eSymbolSize32Bit, &OpPart)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg | (MayImm ? MAllowImm : 0)))
  {
    PutCode((((LongWord)AdrVals.Code) << (MayImm ? 19 : 14))
          | (((LongWord)Code) << 8)
          | 0x3e, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVF(Word Code)
 * \brief  Handle MOVFL/MOVLF Instructions (Format 9)
 * \param  Code Machine Code & Flags
 * ------------------------------------------------------------------------ */

static void DecodeMOVF(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Byte OpSize = (Code >> 8) & 1;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckFPUAvail(eFPU16081)
   && SetFOpSizeFromCode(OpSize)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetFOpSizeFromCode(OpSize ^ 1)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | Code, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat11(Word Code)
 * \brief  Handle Format 11 Instructions
 * \param  Code Machine Code & Operand Size
 * ------------------------------------------------------------------------ */

#define FMT11_DESTMAYIMM 0x80

static void DecodeFormat11(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean DestMayImm = !!(Code & FMT11_DESTMAYIMM);

  Code &= ~FMT11_DESTMAYIMM;
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckFPUAvail(eFPU16081)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg | (DestMayImm ? MAllowImm : 0)))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | ((Code & 0xff) << 10)
          | (SizeCodeF(OpSize) << 8)
          | 0xbe, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat12(Word Code)
 * \brief  Handle Format 12 Instructions
 * \param  Code Machine Code & Operand Size
 * ------------------------------------------------------------------------ */

#define FMT12_DESTMAYIMM 0x40
#define FMT12_580 0x20

static void DecodeFormat12(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean DestMayImm = !!(Code & FMT12_DESTMAYIMM),
          Req580 = !!(Code & FMT12_580);

  Code &= ~(FMT12_DESTMAYIMM | FMT12_580);
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckFPUAvail(Req580 ? eFPU32580 : eFPU32181)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg | (DestMayImm ? MAllowImm : 0)))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | ((Code & 0xff) << 10)
          | (SizeCodeF(OpSize) << 8)
          | 0xfe, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLMR_SMR(Word Code)
 * \brief  Decode LMR/SMR Instructions (Format 14)
 * \param  Code Machine Code & Operand Size
 * ------------------------------------------------------------------------ */

static void DecodeLMR_SMR(Word Code)
{
  tAdrVals AdrVals;
  Word Reg;
  Boolean IsStore = (Code == 3);

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckPMMUAvail()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &AdrVals, MAllowReg | (IsStore ? 0 : MAllowImm))
   && DecodeMMUReg(&ArgStr[1], &Reg))
  {
    if (!IsStore && (Reg >= 14))
      WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[1]);
    PutCode((((LongWord)AdrVals.Code) << 19)
          | ((LongWord)Reg << 15)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x1e, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRDVAL_WRVAL(Word Code)
 * \brief  Handle RDVAL/WRVAL Instructions
 * \param  Code Machine code & operand size
 * ------------------------------------------------------------------------ */

static void DecodeRDVAL_WRVAL(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && CheckSup(True, &OpPart)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg))
  {
    PutCode((((LongWord)AdrVals.Code) << 19)
        | (Lo(Code) << 10)
        | (SizeCodeI(OpSize) << 8)
        | 0x1e, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCCVci(Word Code)
 * \brief  Handle CCVnci Instructions (Format 15.1)
 * \param  Code Machine code & (integer) operand size
 * ------------------------------------------------------------------------ */

static void DecodeCCVci(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && SetCOpSizeFromCode(Code & 0x01)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x36, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCCVic(Word Code)
 * \brief  Handle CCVnic Instructions (Format 15.1)
 * \param  Code Machine code & (integer) operand size
 * ------------------------------------------------------------------------ */

static void DecodeCCVic(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetCOpSizeFromCode(Code & 0x01)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeI(GetOpSizeFromCode(Code)) << 8)
          | 0x36, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCCVcc(Word Code)
 * \brief  Handle CCVncc Instructions (Format 15.1)
 * \param  Code Machine Code & Flags
 * ------------------------------------------------------------------------ */

static void DecodeCCVcc(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Byte OpSize = Code & 1;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && SetCOpSizeFromCode(OpSize)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && ResetOpSize() && SetCOpSizeFromCode(OpSize ^ 1)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Code << 10)
          | 0x36, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCATST(Word Code)
 * \brief  Decode CATSTx Instructions (Format 15.0)
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeCATST(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && SetOpSize(eSymbolSize32Bit, &OpPart)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg | MAllowImm))
  {
    PutCode((((LongWord)AdrVals.Code) << 19)
          | (Code << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x16, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLCR_SCR(Word Code)
 * \brief  Handle LCR/SCR Instructions (Format 15.0)
 * \param  Code Machine Code & Flags
 * ------------------------------------------------------------------------ */

#define FMT15_0_MAYIMM (1 << 7)

static void DecodeLCR_SCR(Word Code)
{
  Boolean MayImm = !!(Code & FMT15_0_MAYIMM);
  tAdrVals AdrVals;

  Code &= ~FMT15_0_MAYIMM;
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && CheckSup(True, &OpPart)
   && SetOpSize(eSymbolSize32Bit, &OpPart)
   && DecodeAdr(&ArgStr[2], &AdrVals, MAllowReg | (MayImm ? MAllowImm : 0)))
  {
    Boolean OK;
    LongWord Reg = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK)
    {
      PutCode((((LongWord)AdrVals.Code) << 19)
           | (Reg << 15)
           | (Code << 10)
           |  (SizeCodeI(OpSize) << 8)
           | 0x16, 3);
      AppendIndex(&AdrVals);
      AppendDisp(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormatLCSR_SCSR(Word Code)
 * \brief  Handle LCSR/SCSR Instructions (Format 15.1)
 * \param  Code Machine Code & Flags
 * ------------------------------------------------------------------------ */

#define FMT15_1_MAYIMM (1 << 7)

static void DecodeLCSR_SCSR(Word Code)
{
  Boolean MayImm = !!(Code & FMT15_1_MAYIMM);
  tAdrVals AdrVals;

  Code &= ~FMT15_1_MAYIMM;
  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && SetOpSize(eSymbolSize32Bit, &OpPart)
   && DecodeAdr(&ArgStr[1], &AdrVals, MAllowReg | (MayImm ? MAllowImm : 0)))
  {
    PutCode((((LongWord)AdrVals.Code) << (MayImm ? 19 : 14))
          | (((LongWord)Code) << 10)
          | (SizeCodeI(OpSize) << 8)
          | 0x36, 3);
    AppendIndex(&AdrVals);
    AppendDisp(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat15_5_7(Word Code)
 * \brief  Handle Format 15.5/15.7 Instructions
 * \param  Code Machine code & operand size
 * ------------------------------------------------------------------------ */

#define FMT15_5_DESTMAYIMM 0x80
#define FMT15_7 0x40

static void DecodeFormat15_5_7(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  unsigned DestMayImm = (Code & FMT15_5_DESTMAYIMM) ? MAllowImm : 0,
           Is7 = !!(Code & FMT15_7);

  Code &= ~(FMT15_5_DESTMAYIMM | FMT15_7);
  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && CheckCustomAvail()
   && (!Is7 || CheckCore((1 << eCoreGen1Ext) | (1 << eCoreGen2)))
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], &SrcAdrVals, MAllowReg | MAllowImm)
   && DecodeAdr(&ArgStr[2], &DestAdrVals, MAllowReg| DestMayImm))
  {
    PutCode((((LongWord)SrcAdrVals.Code) << 19)
          | (((LongWord)DestAdrVals.Code) << 14)
          | (Lo(Code) << 10)
          | (SizeCodeC(OpSize) << 8)
          | (Is7 << 6)
          | 0xb6, 3);
    AppendIndex(&SrcAdrVals);
    AppendIndex(&DestAdrVals);
    AppendDisp(&SrcAdrVals);
    AppendDisp(&DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     void CodeFPU(Word Code)
 * \brief  Handle FPU Instruction
 * ------------------------------------------------------------------------ */

static void CodeFPU(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "OFF"))
    {
      SetFlag(&FPUAvail, FPUAvailName, False);
      SetMomFPU(eFPUNone);
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "ON"))
    {
      SetFlag(&FPUAvail, FPUAvailName, True);
      if (!MomFPU)
        SetMomFPU((tFPU)pCurrCPUProps->DefFPU);
    }
    else
    {
      tFPU FPU;

      for (FPU = (tFPU)1; FPU < eFPUCount; FPU++)
        if (!as_strcasecmp(ArgStr[1].str.p_str, FPUNames[FPU]))
        {
          SetFlag(&FPUAvail, FPUAvailName, True);
          SetMomFPU(FPU);
          break;
        }
      if (FPU >= eFPUCount)
        WrStrErrorPos(ErrNum_InvFPUType, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     void CodePMMU(Word Code)
 * \brief  Handle PMMU Instruction
 * ------------------------------------------------------------------------ */

static void CodePMMU(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "OFF"))
    {
      SetFlag(&PMMUAvail, PMMUAvailName, False);
      SetMomPMMU(ePMMUNone);
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "ON"))
    {
      SetFlag(&PMMUAvail, PMMUAvailName, True);
      if (!MomPMMU)
        SetMomPMMU((tPMMU)pCurrCPUProps->DefPMMU);
    }
    else
    {
      tPMMU PMMU;

      for (PMMU = (tPMMU)1; PMMU < ePMMUCount; PMMU++)
        if (!as_strcasecmp(ArgStr[1].str.p_str, PMMUNames[PMMU]))
        {
          SetFlag(&PMMUAvail, PMMUAvailName, True);
          SetMomPMMU(PMMU);
          break;
        }
      if (PMMU >= ePMMUCount)
        WrStrErrorPos(ErrNum_InvPMMUType, &ArgStr[1]);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBB(Word Code)
 * \brief  Handle BitBLT Instructions
 * \param  Code Machine Code & Options
 * ------------------------------------------------------------------------ */

#define BB_NOOPT (1 << 15)

typedef struct
{
  char Name[3];
  Byte Pos, Value;
} tBitBltOpt;

static const tBitBltOpt BitBltOpts[] =
{
  { "DA" , 17, 1 },
  { "-S" , 15, 1 },
  { "IA" , 17, 0 },
  { "S"  , 15, 0 },
  { ""   , 0 , 0 }
};

static void DecodeBB(Word Code)
{
  if (ChkArgCnt(0, 2)
   && ChkNoAttrPart()
   && CheckCore(1 << eCoreGenE))
  {
    LongWord OpCode = 0x0e | ((LongWord)(Code & ~BB_NOOPT) << 8), OptMask = 0;
    tStrComp *pArg;
    const tBitBltOpt *pOpt;

    forallargs(pArg, True)
    {
      for (pOpt = BitBltOpts + ((Code & BB_NOOPT) ? 2 : 0); pOpt->Name[0]; pOpt++)
        if (!as_strcasecmp(pArg->str.p_str, pOpt->Name))
        {
          LongWord ThisMask = 1ul << pOpt->Pos;

          if (OptMask & ThisMask)
          {
            WrStrErrorPos(ErrNum_ConfBitBltOpt, pArg);
            return;
          }
          OptMask |= ThisMask;
          OpCode = pOpt->Value ? (OpCode | ThisMask) : (OpCode & ~ThisMask);
          break;
        }
      if (!pOpt->Name[0])
      {
        WrStrErrorPos(ErrNum_UnknownBitBltOpt, pArg);
        return;
      }
    }
    PutCode(OpCode, 3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBITxT(Word Code)
 * \brief  Handle BITBLT instructions without argument
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeBITxT(Word Code)
{
  if (ChkArgCnt(0, 0)
   && ChkNoAttrPart()
   && CheckCore(1 << eCoreGenE))
    PutCode(((LongWord)Code) << 8 | 0x0e, 3);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTBITS(Word Code)
 * \brief  Handle TBITS Instruction
 * \param  Code Machine Code
 * ------------------------------------------------------------------------ */

static void DecodeTBITS(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && CheckCore(1 << eCoreGenE))
  {
    Boolean OK;
    LongWord Arg = EvalStrIntExpression(&ArgStr[1], UInt1, &OK);

    if (OK)
      PutCode((Arg << 15)
            | (((LongWord)Code) << 8)
            | 0x0e, 3);
  }
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static void AddSizeInstTable(const char *pName, unsigned SizeMask, Word Code, InstProc Proc)
{
  char Name[20];
  tSymbolSize Size;

  for (Size = eSymbolSize8Bit; Size <= eSymbolSize32Bit; Size++)
    if (SizeMask & (1 << Size))
    {
      as_snprintf(Name, sizeof(Name), "%s%c", pName, "BWD"[Size - eSymbolSize8Bit]);
      AddInstTable(InstTable, Name, Code | (Size << 8), Proc);
    }
}

static void AddFSizeInstTable(const char *pName, unsigned SizeMask, Word Code, InstProc Proc)
{
  char Name[20];
  tSymbolSize Size;

  for (Size = eSymbolSizeFloat32Bit; Size <= eSymbolSizeFloat64Bit; Size++)
    if (SizeMask & (1 << Size))
    {
      as_snprintf(Name, sizeof(Name), "%s%c", pName, "FL"[Size - eSymbolSizeFloat32Bit]);
      AddInstTable(InstTable, Name, Code | (Size << 8), Proc);
    }
}

static void AddCSizeInstTable(const char *pName, unsigned SizeMask, Word Code, InstProc Proc)
{
  char Name[20];
  tSymbolSize Size;

  for (Size = eSymbolSize32Bit; Size <= eSymbolSize64Bit; Size++)
    if (SizeMask & (1 << Size))
    {
      as_snprintf(Name, sizeof(Name), "%s%c", pName, "DQ"[Size - eSymbolSize32Bit]);
      AddInstTable(InstTable, Name, Code | (Size << 8), Proc);
    }
}

static void AddCondition(const char *pCondition, Word Code)
{
  char Str[20];

  as_snprintf(Str, sizeof(Str), "B%s", pCondition);
  AddInstTable(InstTable, Str, (Code << 4) | 0x0a, DecodeFormat0);
  as_snprintf(Str, sizeof(Str), "S%s", pCondition);
  AddSizeInstTable(Str, (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), Code, DecodeScond);
}

static void AddCtl(const char *pName, Word Code, Boolean Privileged)
{
  order_array_rsv_end(CtlRegs, tCtlReg);
  CtlRegs[InstrZ  ].pName      = pName;
  CtlRegs[InstrZ  ].Code       = Code;
  CtlRegs[InstrZ++].Privileged = Privileged;
}

static void AddMMU(const char *pName, Word Code, Word Mask, Boolean Privileged)
{
  order_array_rsv_end(MMURegs, tCtlReg);
  MMURegs[InstrZ  ].pName      = pName;
  MMURegs[InstrZ  ].Code       = Code;
  MMURegs[InstrZ  ].Mask       = Mask;
  MMURegs[InstrZ++].Privileged = Privileged;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(605);
  SetDynamicInstTable(InstTable);

  InstrZ = 0;
  AddCtl("UPSR"   , 0x00, True );
  AddCtl("DCR"    , 0x01, True );
  AddCtl("BPC"    , 0x02, True );
  AddCtl("DSR"    , 0x03, True );
  AddCtl("CAR"    , 0x04, True );
  AddCtl("FP"     , 0x08, False);
  AddCtl("SP"     , 0x09, False);
  AddCtl("SB"     , 0x0a, False);
  AddCtl("USP"    , 0x0b, True );
  AddCtl("CFG"    , 0x0c, True );
  AddCtl("PSR"    , 0x0d, False);
  AddCtl("INTBASE", 0x0e, True );
  AddCtl("MOD"    , 0x0f, False);
  AddCtl(NULL     , 0   , False);

  InstrZ = 0;
  AddMMU("BPR0"   , 0x00, (1 << ePMMU16082) | (1 << ePMMU32082)                                        , True );
  AddMMU("BPR1"   , 0x01, (1 << ePMMU16082) | (1 << ePMMU32082)                                        , True );
  AddMMU("MSR"    , 0x0a, (1 << ePMMU16082) | (1 << ePMMU32082)                     | (1 << ePMMU32532), True );
  AddMMU("BCNT"   , 0x0b, (1 << ePMMU16082) | (1 << ePMMU32082)                                        , True );
  AddMMU("PTB0"   , 0x0c, (1 << ePMMU16082) | (1 << ePMMU32082) | (1 << ePMMU32382) | (1 << ePMMU32532), True );
  AddMMU("PTB1"   , 0x0d, (1 << ePMMU16082) | (1 << ePMMU32082) | (1 << ePMMU32382) | (1 << ePMMU32532), True );
  AddMMU("EIA"    , 0x0f, (1 << ePMMU16082) | (1 << ePMMU32082)                                        , True );
  AddMMU("BAR"    , 0x00,                                         (1 << ePMMU32382)                    , True );
  AddMMU("BMR"    , 0x02,                                         (1 << ePMMU32382)                    , True );
  AddMMU("BDR"    , 0x03,                                         (1 << ePMMU32382)                    , True );
  AddMMU("BEAR"   , 0x06,                                         (1 << ePMMU32382)                    , True );
  AddMMU("FEW"    , 0x09,                                         (1 << ePMMU32382)                    , True );
  AddMMU("ASR"    , 0x0a,                                         (1 << ePMMU32382)                    , True );
  AddMMU("TEAR"   , 0x0b,                                         (1 << ePMMU32382) | (1 << ePMMU32532), True );
  /* TODO: 8 according to National docu, but 9 according to sample code */
  AddMMU("MCR"    , 0x09,                                                             (1 << ePMMU32532), True );
  AddMMU("IVAR0"  , 0x0e,                                         (1 << ePMMU32382) | (1 << ePMMU32532), True );/* w/o */
  AddMMU("IVAR1"  , 0x0f,                                         (1 << ePMMU32382) | (1 << ePMMU32532), True );/* w/o */
  AddMMU(NULL     , 0, 0, False);

  AddCondition("EQ",  0);
  AddCondition("NE",  1);
  AddCondition("CS",  2);
  AddCondition("CC",  3);
  AddCondition("HI",  4);
  AddCondition("LS",  5);
  AddCondition("GT",  6);
  AddCondition("LE",  7);
  AddCondition("FS",  8);
  AddCondition("FC",  9);
  AddCondition("LO", 10);
  AddCondition("HS", 11);
  AddCondition("LT", 12);
  AddCondition("GE", 13);

  /* Format 0 */

  AddInstTable(InstTable, "BR" , 0xea, DecodeFormat0);
  AddInstTable(InstTable, "BSR", 0x02, DecodeFormat0);

  /* Format 1 */

  AddInstTable(InstTable, "BPT" , 0xf2, DecodeFixed);
  AddInstTable(InstTable, "DIA" , 0xc2, DecodeFixed);
  AddInstTable(InstTable, "FLAG", 0xd2, DecodeFixed);
  AddInstTable(InstTable, "NOP" , NOPCode, DecodeFixed);
  AddInstTable(InstTable, "RETI", 0x52, DecodeFixed);
  AddInstTable(InstTable, "SVC" , 0xe2, DecodeFixed);
  AddInstTable(InstTable, "WAIT", 0xb2, DecodeFixed);
  AddInstTable(InstTable, "RET" , 0x12, DecodeRET);
  AddInstTable(InstTable, "RETT", 0x42, DecodeRET);
  AddInstTable(InstTable, "RXP" , 0x32, DecodeRET);
  AddInstTable(InstTable, "SAVE", 0x62, DecodeSAVE_RESTORE);
  AddInstTable(InstTable, "RESTORE", 0x72, DecodeSAVE_RESTORE);

  AddInstTable(InstTable, "CINV", 0x271e, DecodeCINV);
  AddInstTable(InstTable, "CXP" , 0x22, DecodeCXP);
  AddInstTable(InstTable, "ENTER", 0x82, DecodeENTER);
  AddInstTable(InstTable, "EXIT", 0x92, DecodeEXIT);

  /* Format 2 */

  AddSizeInstTable("ADDQ" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0, DecodeMOVQ);
  AddSizeInstTable("LPR"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 6, DecodeLPR_SPR);
  AddSizeInstTable("SPR"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 2, DecodeLPR_SPR);
  AddSizeInstTable("MOVQ" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 5, DecodeMOVQ);
  AddSizeInstTable("ACB"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 4, DecodeACB);
  AddSizeInstTable("CMPQ" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 1 | MOVQ_DESTMAYIMM, DecodeMOVQ);

  /* Format 3 */

  AddInstTable(InstTable, "JUMP"  , 0x04 | (eSymbolSize32Bit << 8), DecodeFormat3);
  AddInstTable(InstTable, "JSR"   , 0x0c | (eSymbolSize32Bit << 8), DecodeFormat3);
  AddSizeInstTable("ADJSP" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0a, DecodeFormat3);
  AddSizeInstTable("BICPSR", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit), 0x02, DecodeFormat3);
  AddSizeInstTable("BISPSR", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit), 0x06, DecodeFormat3);
  AddSizeInstTable("CASE"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeFormat3);
  AddInstTable(InstTable, "CXPD"  , 0x00 | (eSymbolSize32Bit << 8), DecodeFormat3);

  /* Format 4 */

  AddSizeInstTable("MOV" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x05, DecodeFormat4);
  AddSizeInstTable("ADD" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x00, DecodeFormat4);
  AddSizeInstTable("ADDC", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x04, DecodeFormat4);
  AddInstTable(InstTable, "ADDR", (eSymbolSize32Bit << 8) | FMT4_SRCISADDR | 0x09, DecodeFormat4);
  AddSizeInstTable("AND" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0a, DecodeFormat4);
  AddSizeInstTable("BIC" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x02, DecodeFormat4);
  AddSizeInstTable("CMP" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), FMT4_DESTMAYIMM | 0x01, DecodeFormat4);
  AddSizeInstTable("OR"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x06, DecodeFormat4);
  AddSizeInstTable("SUB" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x08, DecodeFormat4);
  AddSizeInstTable("SUBC", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0c, DecodeFormat4);
  AddSizeInstTable("TBIT", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0d, DecodeFormat4);
  AddSizeInstTable("XOR" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeFormat4);

  /* Format 5 */

  AddSizeInstTable("MOVS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x00, DecodeMOVS_CMPS_SKPS);
  AddSizeInstTable("CMPS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x01, DecodeMOVS_CMPS_SKPS);
  AddSizeInstTable("SKPS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x03, DecodeMOVS_CMPS_SKPS);
  AddInstTable(InstTable, "MOVST", (eSymbolSize8Bit << 8) | 0x20, DecodeMOVS_CMPS_SKPS);
  AddInstTable(InstTable, "CMPST", (eSymbolSize8Bit << 8) | 0x21, DecodeMOVS_CMPS_SKPS);
  AddInstTable(InstTable, "SKPST", (eSymbolSize8Bit << 8) | 0x23, DecodeMOVS_CMPS_SKPS);
  AddInstTable(InstTable, "SETCFG", 0xb0e, DecodeSETCFG);

  /* Format 6 */

  AddSizeInstTable("ABS"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0c, DecodeFormat6);
  AddSizeInstTable("ADDP"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0f, DecodeFormat6);
  AddSizeInstTable("ASH"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), FMT6_SRC8BIT | 0x01, DecodeFormat6);
  AddSizeInstTable("CBIT"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x02, DecodeFormat6);
  AddSizeInstTable("CBITI" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x03, DecodeFormat6);
  AddSizeInstTable("COM"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0d, DecodeFormat6);
  AddSizeInstTable("IBIT"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeFormat6);
  AddSizeInstTable("LSH"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), FMT6_SRC8BIT | 0x05, DecodeFormat6);
  AddSizeInstTable("NEG"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x08, DecodeFormat6);
  AddSizeInstTable("NOT"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x09, DecodeFormat6);
  AddSizeInstTable("ROT"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), FMT6_SRC8BIT | 0x00, DecodeFormat6);
  AddSizeInstTable("SBIT"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x06, DecodeFormat6);
  AddSizeInstTable("SBITI" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x07, DecodeFormat6);
  AddSizeInstTable("SUBP"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0b, DecodeFormat6);

  /* Format 7 */

  AddSizeInstTable("DIV"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0f, DecodeFormat7);
  AddSizeInstTable("MOD"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeFormat7);
  AddSizeInstTable("MUL"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x08, DecodeFormat7);
  AddSizeInstTable("QUO"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0c, DecodeFormat7);
  AddSizeInstTable("REM"   , (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0d, DecodeFormat7);

  AddInstTable(InstTable, "MOVXBW", (eSymbolSize8Bit  << 8) | (eSymbolSize16Bit << 12) | 1, DecodeMOVExt);
  AddInstTable(InstTable, "MOVXBD", (eSymbolSize8Bit  << 8) | (eSymbolSize32Bit << 12) | 1, DecodeMOVExt);
  AddInstTable(InstTable, "MOVXWD", (eSymbolSize16Bit << 8) | (eSymbolSize32Bit << 12) | 1, DecodeMOVExt);
  AddInstTable(InstTable, "MOVZBW", (eSymbolSize8Bit  << 8) | (eSymbolSize16Bit << 12) | 0, DecodeMOVExt);
  AddInstTable(InstTable, "MOVZBD", (eSymbolSize8Bit  << 8) | (eSymbolSize32Bit << 12) | 0, DecodeMOVExt);
  AddInstTable(InstTable, "MOVZWD", (eSymbolSize16Bit << 8) | (eSymbolSize32Bit << 12) | 0, DecodeMOVExt);

  AddSizeInstTable("MOVM", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x00, DecodeMOVM_CMPM);
  AddSizeInstTable("CMPM", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x01, DecodeMOVM_CMPM);

  AddSizeInstTable("DEI", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0b, DecodeDoubleDest);
  AddSizeInstTable("MEI", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x09, DecodeDoubleDest);

  AddSizeInstTable("EXTS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x03, DecodeINSS_EXTS);
  AddSizeInstTable("INSS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x02, DecodeINSS_EXTS);

  /* Format 8 */

  AddSizeInstTable("CHECK", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeCHECK_INDEX);
  AddInstTable(InstTable, "CVTP", (eSymbolSize32Bit << 8) | 0x06, DecodeCHECK_INDEX);
  AddSizeInstTable("INDEX", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x42, DecodeCHECK_INDEX);

  AddSizeInstTable("EXT", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0, DecodeINS_EXT);
  AddSizeInstTable("INS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 1, DecodeINS_EXT);

  AddSizeInstTable("FFS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x01, DecodeFFS);

  AddSizeInstTable("MOVSU", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x3, DecodeMOVSU);
  AddSizeInstTable("MOVUS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x7, DecodeMOVSU);

  /* Format 9 */

  AddSizeInstTable("FLOORF", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0f, DecodeFLOOR_ROUND_TRUNC);
  AddSizeInstTable("FLOORL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeFLOOR_ROUND_TRUNC);
  AddSizeInstTable("ROUNDF", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x09, DecodeFLOOR_ROUND_TRUNC);
  AddSizeInstTable("ROUNDL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x08, DecodeFLOOR_ROUND_TRUNC);
  AddSizeInstTable("TRUNCF", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0b, DecodeFLOOR_ROUND_TRUNC);
  AddSizeInstTable("TRUNCL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0a, DecodeFLOOR_ROUND_TRUNC);
  AddInstTable(InstTable, "MOVBF", 0x01 | (eSymbolSize8Bit << 8), DecodeMOVif);
  AddInstTable(InstTable, "MOVWF", 0x01 | (eSymbolSize16Bit << 8), DecodeMOVif);
  AddInstTable(InstTable, "MOVDF", 0x01 | (eSymbolSize32Bit << 8), DecodeMOVif);
  AddInstTable(InstTable, "MOVBL", 0x00 | (eSymbolSize8Bit << 8), DecodeMOVif);
  AddInstTable(InstTable, "MOVWL", 0x00 | (eSymbolSize16Bit << 8), DecodeMOVif);
  AddInstTable(InstTable, "MOVDL", 0x00 | (eSymbolSize32Bit << 8), DecodeMOVif);

  AddInstTable(InstTable, "LFSR", 0x0f | FMT9_MAYIMM, DecodeLFSR_SFSR);
  AddInstTable(InstTable, "SFSR", 0x37, DecodeLFSR_SFSR);

  AddInstTable(InstTable, "MOVFL"  , 0x1b3e, DecodeMOVF);
  AddInstTable(InstTable, "MOVLF"  , 0x163e, DecodeMOVF);

  /* Format 11 */

  AddFSizeInstTable("ABS", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x0d, DecodeFormat11);
  AddFSizeInstTable("ADD", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x00, DecodeFormat11);
  AddFSizeInstTable("CMP", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x02 | FMT11_DESTMAYIMM, DecodeFormat11);
  AddFSizeInstTable("DIV", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x08, DecodeFormat11);
  AddFSizeInstTable("MOV", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x01, DecodeFormat11);
  AddFSizeInstTable("MUL", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x0c, DecodeFormat11);
  AddFSizeInstTable("NEG", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x05, DecodeFormat11);
  AddFSizeInstTable("SUB", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x04, DecodeFormat11);

  /* Format 12 - only newer FPUs? */

  AddFSizeInstTable("DOT"  , (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x03 | FMT12_DESTMAYIMM, DecodeFormat12);
  AddFSizeInstTable("LOGB" , (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x05, DecodeFormat12);
  AddFSizeInstTable("POLY" , (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x02 | FMT12_DESTMAYIMM, DecodeFormat12);
  AddFSizeInstTable("SCALB", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x04, DecodeFormat12);
  AddFSizeInstTable("REM"  , (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), 0x00, DecodeFormat12);
  AddFSizeInstTable("SQRT" , (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), FMT12_580 | 0x01, DecodeFormat12);
  AddFSizeInstTable("ATAN2", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), FMT12_580 | 0x0c, DecodeFormat12);
  AddFSizeInstTable("SICOS", (1 << eSymbolSizeFloat32Bit) | (1 << eSymbolSizeFloat64Bit), FMT12_580 | 0x0d, DecodeFormat12);

  /* Format 14 */

  AddInstTable(InstTable, "LMR"  , 0x02 | (eSymbolSize32Bit << 8), DecodeLMR_SMR);
  AddInstTable(InstTable, "SMR"  , 0x03 | (eSymbolSize32Bit << 8), DecodeLMR_SMR);
  AddInstTable(InstTable, "RDVAL", 0x00 | (eSymbolSize32Bit << 8), DecodeRDVAL_WRVAL);
  AddInstTable(InstTable, "WRVAL", 0x01 | (eSymbolSize32Bit << 8), DecodeRDVAL_WRVAL);

  AddInstTable(InstTable, "REG" , 0, CodeREG);
  AddInstTable(InstTable, "BYTE"   , eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString , DecodeIntelDB);
  AddInstTable(InstTable, "WORD"   , eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString , DecodeIntelDW);
  AddInstTable(InstTable, "DOUBLE" , eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString , DecodeIntelDD);
  AddInstTable(InstTable, "FLOAT"  , eIntPseudoFlag_AllowFloat , DecodeIntelDD);
  AddInstTable(InstTable, "LONG"   , eIntPseudoFlag_AllowFloat , DecodeIntelDQ);
  AddInstTable(InstTable, "FPU"    , 0, CodeFPU);
  AddInstTable(InstTable, "PMMU"   , 0, CodePMMU);

  /* Format 15.0 */

  AddInstTable(InstTable, "LCR", 0x0a | FMT15_0_MAYIMM, DecodeLCR_SCR);
  AddInstTable(InstTable, "SCR", 0x0b                 , DecodeLCR_SCR);
  AddInstTable(InstTable, "CATST0", 0x00, DecodeCATST);
  AddInstTable(InstTable, "CATST1", 0x01, DecodeCATST);

  /* Format 15.1 */

  AddSizeInstTable("CCV0Q", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0e, DecodeCCVci);
  AddSizeInstTable("CCV0D", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0f, DecodeCCVci);
  AddSizeInstTable("CCV1Q", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0a, DecodeCCVci);
  AddSizeInstTable("CCV1D", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x0b, DecodeCCVci);
  AddSizeInstTable("CCV2Q", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x08, DecodeCCVci);
  AddSizeInstTable("CCV2D", (1 << eSymbolSize8Bit) | (1 << eSymbolSize16Bit) | (1 << eSymbolSize32Bit), 0x09, DecodeCCVci);
  AddInstTable(InstTable, "CCV3BQ", (eSymbolSize8Bit  << 8) | 0x00, DecodeCCVic);
  AddInstTable(InstTable, "CCV3WQ", (eSymbolSize16Bit << 8) | 0x00, DecodeCCVic);
  AddInstTable(InstTable, "CCV3DQ", (eSymbolSize32Bit << 8) | 0x00, DecodeCCVic);
  AddInstTable(InstTable, "CCV3BD", (eSymbolSize8Bit  << 8) | 0x01, DecodeCCVic);
  AddInstTable(InstTable, "CCV3WD", (eSymbolSize16Bit << 8) | 0x01, DecodeCCVic);
  AddInstTable(InstTable, "CCV3DD", (eSymbolSize32Bit << 8) | 0x01, DecodeCCVic);
  AddInstTable(InstTable, "CCV4DQ", 0x07, DecodeCCVcc);
  AddInstTable(InstTable, "CCV5QD", 0x04, DecodeCCVcc);
  AddInstTable(InstTable, "LCSR", 0x01 | FMT15_1_MAYIMM, DecodeLCSR_SCSR);
  AddInstTable(InstTable, "SCSR", 0x06, DecodeLCSR_SCSR);

  /* Format 15.5/15.7 */

  AddCSizeInstTable("CCAL0", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x00, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV0", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x01, DecodeFormat15_5_7);
  AddCSizeInstTable("CCMP" , (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_5_DESTMAYIMM | 0x02, DecodeFormat15_5_7);
  AddCSizeInstTable("CCMP1", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_5_DESTMAYIMM | 0x03, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL1", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x04, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV2", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x05, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL3", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x08, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV3", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x09, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL2", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x0c, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV1", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), 0x0d, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL4", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x00, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV4", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x01, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL8", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x02, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL9", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x03, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL5", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x04, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV6", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x05, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL7", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x08, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV7", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x09, DecodeFormat15_5_7);
  AddCSizeInstTable("CCAL6", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x0c, DecodeFormat15_5_7);
  AddCSizeInstTable("CMOV5", (1 << eSymbolSize32Bit) | (1 << eSymbolSize64Bit), FMT15_7 | 0x0d, DecodeFormat15_5_7);

  /* BITBLT */

  AddInstTable(InstTable, "BBAND"  , 0x12b, DecodeBB);
  AddInstTable(InstTable, "BBOR"   , 0x019, DecodeBB);
  AddInstTable(InstTable, "BBXOR"  , 0x039, DecodeBB);
  AddInstTable(InstTable, "BBFOR"  , BB_NOOPT | 0x031, DecodeBB);
  AddInstTable(InstTable, "BBSTOD" , 0x011, DecodeBB);

  AddInstTable(InstTable, "BITWT"  , 0x21, DecodeBITxT);
  AddInstTable(InstTable, "EXTBLT" , 0x17, DecodeBITxT);
  AddInstTable(InstTable, "MOVMPB" , 0x1c, DecodeBITxT);
  AddInstTable(InstTable, "MOVMPW" , 0x1d, DecodeBITxT);
  AddInstTable(InstTable, "MOVMPD" , 0x1f, DecodeBITxT);
  AddInstTable(InstTable, "SBITS"  , 0x37, DecodeBITxT);
  AddInstTable(InstTable, "SBITPS" , 0x2f, DecodeBITxT);

  AddInstTable(InstTable, "TBITS"  , 0x27, DecodeTBITS);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(CtlRegs);
  order_array_free(MMURegs);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     MakeCode_NS32K(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void MakeCode_NS32K(void)
{
  CodeLen = 0; DontPrint = False;
  OpSize = eSymbolSizeUnknown;

  /* to be ignored */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_NS32K(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on NS32000
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_NS32K(char *pArg, TempResult *pResult)
{
  Word Reg;
  tSymbolSize Size;

  if (DecodeRegCore(pArg, &Reg, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_NS32K;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     InitCode_NS32K(void)
 * \brief  target-specific initializations before starting a pass
 * ------------------------------------------------------------------------ */

static void InitCode_NS32K(void)
{
  SetMomFPU(eFPUNone);
  SetMomPMMU(ePMMUNone);
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_NS32K(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDef_NS32K(void)
{
  return Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_NS32K(void *pUser)
 * \brief  prepare to assemble code for this target
 * \param  pUser CPU properties
 * ------------------------------------------------------------------------ */

static Boolean ChkMoreZeroArg(void)
{
  return (ArgCnt > 0);
}

static void SwitchTo_NS32K(void *pUser)
{
  const TFamilyDescr *pDescr = FindFamilyByName("NS32000");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);
  SaveIsOccupiedFnc = ChkMoreZeroArg;
  RestoreIsOccupiedFnc = ChkMoreZeroArg;

  pCurrCPUProps = (const tCPUProps*)pUser;

  /* default selection of "typical" companion FPU/PMMU: */

  SetMomFPU((tFPU)pCurrCPUProps->DefFPU);
  SetMomPMMU((tPMMU)pCurrCPUProps->DefPMMU);

  PCSymbol = "*"; HeaderID = pDescr->Id;
  NOPCode = 0xa2;
  DivideChars = ",";
  HasAttrs = True; AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[pCurrCPUProps->MemIntType].Max;

  MakeCode = MakeCode_NS32K;
  IsDef = IsDef_NS32K;
  DissectReg = DissectReg_NS32K;
  InternSymbol = InternSymbol_NS32K;
  SwitchFrom = DeinitFields;
  IntConstModeIBMNoTerm = True;
  QualifyQuote = QualifyQuote_SingleQuoteConstant;
  InitFields();
  onoff_supmode_add();
  if (!onoff_test_and_set(e_onoff_reg_custom))
    SetFlag(&CustomAvail, CustomAvailSymName, False);
  AddONOFF(CustomAvailCmdName, &CustomAvail, CustomAvailSymName, False);
}

/*!------------------------------------------------------------------------
 * \fn     codens32000_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "NS16008", eCoreGen1   , eFPU16081, ePMMU16082, UInt24 },
  { "NS32008", eCoreGen1   , eFPU32081, ePMMU32082, UInt24 },
  { "NS08032", eCoreGen1   , eFPU32081, ePMMU32082, UInt24 },
  { "NS16032", eCoreGen1   , eFPU16081, ePMMU16082, UInt24 },
  { "NS32016", eCoreGen1   , eFPU32081, ePMMU32082, UInt24 },
  { "NS32032", eCoreGen1   , eFPU32081, ePMMU32082, UInt24 },
  { "NS32332", eCoreGen1Ext, eFPU32381, ePMMU32382, UInt32 },
  { "NS32CG16",eCoreGenE   , eFPU32181, ePMMU32082, UInt24 },
  { "NS32532", eCoreGen2   , eFPU32381, ePMMU32532, UInt32 },
  { NULL     , eCoreNone   , eFPUNone , ePMMUNone , UInt1  }
};

void codens32k_init(void)
{
  const tCPUProps *pRun;

  for (pRun = CPUProps; pRun->pName; pRun++)
    (void)AddCPUUser(pRun->pName, SwitchTo_NS32K, (void*)pRun, NULL);

  AddInitPassProc(InitCode_NS32K);
}
