/* codez8000.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Zilog Z8000                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "cpulist.h"
#include "headids.h"
#include "strutil.h"
#include "intformat.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "codepseudo.h"
#include "codevars.h"
#include "intpseudo.h"
#include "operator.h"

#include "codez8000.h"

typedef enum
{
  eCoreNone = 0,
  eCoreZ8001 = 1,
  eCoreZ8003 = 2
} tCore;

typedef struct
{
  const char *pName;
  tCore Core;
  Boolean SuppSegmented;
} tCPUProps;

typedef enum eAdrMode
{
  eModNone = 0,
  eModReg = 1,
  eModIReg = 2,
  eModDirect = 3,
  eModIndexed = 4,
  eModBaseAddress = 5,
  eModBaseIndexed = 6,
  eModImm = 7,
  eModCtl = 8
} tAdrMode;

#define MModReg (1 << eModReg)
#define MModIReg (1 << eModIReg)
#define MModDirect (1 << eModDirect)
#define MModIndexed (1 << eModIndexed)
#define MModBaseAddress (1 << eModBaseAddress)
#define MModBaseIndexed (1 << eModBaseIndexed)
#define MModImm (1 << eModImm)
#define MModCtl (1 << eModCtl)

#define MModIO (1 << 15)

#define MModNoImm (MModReg | MModIReg | MModDirect | MModIndexed | MModBaseAddress | MModBaseIndexed)
#define MModAll (MModNoImm | MModImm)

#define eSymbolSize4Bit ((tSymbolSize)-2)
#define eSymbolSize3Bit ((tSymbolSize)-3)

typedef struct
{
  tAdrMode Mode;
  unsigned Cnt;
  Word Val, Vals[3];
} tAdrVals;

typedef struct
{
  Word Code;
  Boolean Privileged;
} FixedOrder;

typedef enum
{
  ePrivileged = 1 << 0,
  eSegMode = 1 << 1,
  eNonSegMode = 1 << 2
} tCtlFlags;

typedef struct
{
  const char *pName;
  Word Code;
  tCtlFlags Flags;
  tSymbolSize Size;
} tCtlReg;

typedef struct
{
  const char *pName;
  Word Code;
} tCondition;

/* Auto-optimization of LD #imm4,Rn -> LDK disabled for the moment,
   until we find a syntax to control it: */

#define OPT_LD_LDK 0

static const tCPUProps *pCurrCPUProps;

static FixedOrder *FixedOrders;
static tCtlReg *CtlRegs;
static tCondition *Conditions;
static int CtlRegCnt;

static tSymbolSize OpSize;
static ShortInt ImmOpSize;
static IntType MemIntType;

static LongInt AMDSyntax;

#ifdef __cplusplus
#include "codez8000.hpp"
#endif

/*--------------------------------------------------------------------------*/
/* Helper Functions */

/*!------------------------------------------------------------------------
 * \fn     CheckSup(Boolean Required)
 * \brief  check whether supervisor mode requirement and complain if violated
 * \param  Required is supervisor mode required?
 * \return False if violated
 * ------------------------------------------------------------------------ */

static Boolean CheckSup(Boolean Required)
{
  if (!SupAllowed && Required)
  {
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Segmented(void)
 * \brief  operating in segmented mode?
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean Segmented(void)
{
  return pCurrCPUProps->SuppSegmented;
}

/*!------------------------------------------------------------------------
 * \fn     AddrRegSize(void)
 * \brief  return size of address register
 * \return 16 or 32 bit
 * ------------------------------------------------------------------------ */

static tSymbolSize AddrRegSize(void)
{
  return Segmented() ? eSymbolSize32Bit : eSymbolSize16Bit;
}

/*!------------------------------------------------------------------------
 * \fn     GetSegment(LongWord Address)
 * \brief  extract segment from (linear) address
 * \param  Address (linear) address
 * \return segment
 * ------------------------------------------------------------------------ */

static Word GetSegment(LongWord Address)
{
 return (Address >> 16) & 0x7f;
}

/*--------------------------------------------------------------------------*/
/* Register Handling */

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Word *pValue, tSymbolSize *pSize)
 * \brief  check whether argument describes a CPU (general purpose) register
 * \param  pArg source argument
 * \param  pValue register number if yes
 * \param  pSize register size if yes
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Word *pValue, tSymbolSize *pSize)
{
  Word Offset = 0, MaskVal, MaxVal;

  if (as_toupper(*pArg) != 'R')
    return False;
  pArg++;

  switch (as_toupper(*pArg))
  {
    case 'H':
      *pSize = eSymbolSize8Bit;
      MaskVal = 0; MaxVal = 7;
      pArg++;
      goto Num;
    case 'L':
      *pSize = eSymbolSize8Bit;
      Offset = 8;
      MaskVal = 0; MaxVal = 15;
      pArg++;
      goto Num;
    case 'R':
      *pSize = eSymbolSize32Bit;
      MaskVal = 1; MaxVal = 15;
      pArg++;
      goto Num;
    case 'Q':
      *pSize = eSymbolSize64Bit;
      MaskVal = 3; MaxVal = 15;
      pArg++;
      goto Num;
    default:
      *pSize = eSymbolSize16Bit;
      MaskVal = 0; MaxVal = 15;
      /* fall-thru */
    Num:
    {
      char *pEnd;

      *pValue = strtoul(pArg, &pEnd, 10) + Offset;
      return !*pEnd && (*pValue <= MaxVal) && !(*pValue & MaskVal);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_Z8000(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - Z8000 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_Z8000(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "R%c%u", "HL"[(Value >> 3) & 1], (unsigned)(Value & 7));
      break;
    case eSymbolSize16Bit:
      as_snprintf(pDest, DestSize, "R%u", (unsigned)Value);
      break;
    case eSymbolSize32Bit:
      as_snprintf(pDest, DestSize, "RR%u", (unsigned)Value);
      break;
    case eSymbolSize64Bit:
      as_snprintf(pDest, DestSize, "RQ%u", (unsigned)Value);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkRegOverlap(Word FirstReg, int FirstSize, ...)
 * \brief  check for register overlap
 * \param  FirstReg first register #
 * \param  FirstSize first register size
 * \param  ... further num/size pairs
 * \return -1 if no overlap, otherwise index of first conflicting register
 * ------------------------------------------------------------------------ */

static int ChkRegOverlap(Word FirstReg, int FirstSize, ...)
{
  LongWord RegUse = 0, ThisUse;
  int Index = 0;
  va_list ap;

  va_start(ap, FirstSize);
  while (True)
  {
    switch (FirstSize)
    {
      case eSymbolSize8Bit:
        ThisUse = ((FirstReg & 8) ? 2ul : 1ul) << ((FirstReg & 7) * 2);
        break;
      case eSymbolSize16Bit:
        ThisUse = 3ul << (FirstReg * 2);
        break;
      case eSymbolSize32Bit:
        ThisUse = 15ul << (FirstReg * 2);
        break;
      default:
        va_end(ap);
        return Index;
    }
    if (RegUse & ThisUse)
    {
      va_end(ap);
      return Index;
    }
    RegUse |= ThisUse;
    Index++;
    FirstReg = va_arg(ap, unsigned);
    FirstSize = va_arg(ap, int);
    if (FirstSize == eSymbolSizeUnknown)
      break;
  }
  va_end(ap);
  return -1;
}

/*--------------------------------------------------------------------------*/
/* Address Parsing */

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Word *pValue, tSymbolSize *pSize, tChkRegSize ChkRegSize, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or user-defined register alias
 * \param  pArg argument
 * \param  pValue resulting register # if yes
 * \param  pSize resulting register size if yes
 * \param  ChkReqSize optional check callback for register size
 * \param  MustBeReg expecting register or maybe not?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

typedef Boolean (*tChkRegSize)(const tStrComp *pArg, tSymbolSize ActSize);

static Boolean ChkRegSize_Idx(const tStrComp *pArg, tSymbolSize ActSize)
{
  if (ActSize != eSymbolSize16Bit)
  {
    WrStrErrorPos(ErrNum_IndexRegMustBe16Bit, pArg);
    return False;
  }
  return True;
}

static Boolean ChkRegSize_8To32(const tStrComp *pArg, tSymbolSize ActSize)
{
  if (ActSize > eSymbolSize32Bit)
  {
    WrStrErrorPos(ErrNum_InvOpSize, pArg);
    return False;
  }
  return True;
}

static Boolean ChkRegSize_IOAddr(const tStrComp *pArg, tSymbolSize ActSize)
{
  if (ActSize != eSymbolSize16Bit)
  {
    WrStrErrorPos(ErrNum_IOAddrRegMustBe16Bit, pArg);
    return False;
  }
  return True;
}

static Boolean ChkRegSize_MemAddr(const tStrComp *pArg, tSymbolSize ActSize)
{
  if (Segmented())
  {
    if (ActSize != eSymbolSize32Bit)
    {
      WrStrErrorPos(ErrNum_SegAddrRegMustBe32Bit, pArg);
      return False;
    }
  }
  else
  {
    if (ActSize != eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_NonSegAddrRegMustBe16Bit, pArg);
      return False;
    }
  }
  return True;
}

static tRegEvalResult DecodeReg(const tStrComp *pArg, Word *pValue, tSymbolSize *pSize, tChkRegSize ChkRegSize, Boolean MustBeReg)
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

  if ((RegEvalResult == eIsReg)
   && ChkRegSize
   && !ChkRegSize(pArg, EvalResult.DataSize))
    RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;

  if (pSize) *pSize = EvalResult.DataSize;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     ClearAdrVals(tAdrVals *pAdrVals)
 * \brief  clear address expression result buffer
 * \param  pAdrVals buffer to clear
 * ------------------------------------------------------------------------ */

static void ClearAdrVals(tAdrVals *pAdrVals)
{
  pAdrVals->Mode = eModNone;
  pAdrVals->Cnt = 0;
  pAdrVals->Val = 0;
}

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
 * \fn     GetImmIntType(tSymbolSize Size)
 * \brief  retrieve immediate int type fitting to operand size
 * \param  Size operand size
 * \return int type
 * ------------------------------------------------------------------------ */

static IntType GetImmIntType(tSymbolSize Size)
{
  switch ((int)Size)
  {
    case eSymbolSize3Bit:
      return UInt3;
    case eSymbolSize4Bit:
      return UInt4;
    case eSymbolSize8Bit:
      return Int8;
    case eSymbolSize16Bit:
      return Int16;
    case eSymbolSize32Bit:
      return Int32;
    default:
      return IntTypeCnt;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAddrPartNum(const tStrComp *pArg, LongInt Disp, tEvalResult *pEvalResult, Boolean IsDirect, Boolean IsIO, Boolean *pForceShortt, Boolean *pIsDirect)
 * \brief  decode address part, for direct/immediate or indexed mode
 * \param  pArg source argument
 * \param  Disp displacement to add to value
 * \param  pAdrVals binary coding of addressing mode
 * \param  IsDirect force treatment as direct address mode
 * \param  IsIO I/O space instead of memory space
 * \param  pForceShort short coding required?
 * \param  pIsDirect classified as direct addressing?
 * \return True if success
 * ------------------------------------------------------------------------ */

static LongWord DecodeAddrPartNum(const tStrComp *pArg, LongInt Disp, tEvalResult *pEvalResult, Boolean IsDirect, Boolean IsIO, Boolean *pForceShort, Boolean *pIsDirect)
{
  tStrComp CopyComp;
  String Copy;
  Boolean HasSeg = False;
  LongWord Result, SegNum = 0;
  IntType ThisIntType;

  StrCompMkTemp(&CopyComp, Copy, sizeof(Copy));
  StrCompCopy(&CopyComp, pArg);
  *pForceShort = False;

  /* deal with |....| and <<seg>>offs only in segmented mode: */

  if (!IsIO && Segmented())
  {
    int Len = strlen(CopyComp.str.p_str);

    if ((Len >= 2) && (CopyComp.str.p_str[0] == '|') && (CopyComp.str.p_str[Len - 1] == '|'))
    {
      StrCompShorten(&CopyComp, 1);
      StrCompCutLeft(&CopyComp, 1);
      KillPrefBlanksStrComp(&CopyComp);
      KillPostBlanksStrComp(&CopyComp);
      *pForceShort = True;
      IsDirect = True;
      Len -= 2;
    }
    if (!strncmp(CopyComp.str.p_str, "<<", 2))
    {
      char *pRun, *pSplitPos = NULL;
      int Nest = 0;
      Boolean InSgl = False, InDbl = False, Found = False;

      for (pRun = CopyComp.str.p_str + 2; *pRun && !Found; pRun++)
      {
        switch (*pRun)
        {
          case '\'':
            if (!InDbl) InSgl = !InSgl;
            break;
          case '"':
            if (!InSgl) InDbl = !InDbl;
            break;
          case '(':
            if (!InSgl && !InDbl) Nest++;
            break;
          case ')':
            if (!InSgl && !InDbl) Nest--;
            break;
          case '>':
            if (!InSgl && !InDbl && !Nest && (pRun[1] == '>'))
              pSplitPos = pRun;
            break;
          default:
            if (pSplitPos)
            {
              if (as_isspace(*pRun)); /* delay decision to to next non-blank */
              else if (as_isalnum(*pRun) || (*pRun == '('))
                Found = True;
              else
                pSplitPos = NULL;
            }
        }
      }
      if (pSplitPos)
      {
        tStrComp SegArg;

        StrCompSplitRef(&SegArg, &CopyComp, &CopyComp, pSplitPos);
        StrCompIncRefLeft(&CopyComp, 1);
        SegNum = EvalStrIntExpressionOffsWithResult(&SegArg, 2, UInt7, pEvalResult);
        if (!pEvalResult->OK)
          return 0;
        HasSeg = True;
        IsDirect = True;
      }
    }
  }

  /* If we know it's direct, right away limit to the correct address range.  Otherwise, it might
     be immediate up to 32 bits: */

  if (IsDirect)
    ThisIntType = (IsIO || HasSeg) ? UInt16 : MemIntType;
  else
    ThisIntType = Int32;
  Result = EvalStrIntExpressionWithResult(&CopyComp, ThisIntType, pEvalResult) + Disp;

  /* For AMD syntax, treat as direct if no untyped constant */

  if (AMDSyntax && !IsDirect && pEvalResult->AddrSpaceMask)
    IsDirect = True;

  /* for forwards, truncate to allowed range */

  if (IsDirect)
    ThisIntType = (IsIO || HasSeg) ? UInt16 : MemIntType;
  else
    ThisIntType = GetImmIntType(OpSize);
  if (mFirstPassUnknown(pEvalResult->Flags))
    Result &= IntTypeDefs[(int)ThisIntType].Mask;

  if (IsDirect)
  {
    Result |= (SegNum << 16);
    if (pEvalResult->OK
     && !mFirstPassUnknownOrQuestionable(pEvalResult->Flags)
     && *pForceShort
     && (Result & 0x00ff00ul))
    {
      WrStrErrorPos(ErrNum_NoShortAddr, pArg);
      pEvalResult->OK = False;
    }
  }
  if (pIsDirect)
    *pIsDirect = IsDirect;
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     FillAbsAddr(tAdrVals *pAdrVals, LongWord Address, Boolean IsDirect, Boolean IsIO, Boolean ForceShort, Boolean *pIsDirect)
 * \brief  pack absolute addess into instruction extension word(s)
 * \param  pAdrVals destination buffer
 * \param  Address address to pack
 * \param  IsDirect force treatment as direct address mode
 * \param  IsIO I/O (16b) or memory (23/16b) address?
 * \param  ForceShort force short encoding
 * \param  pIsDirect classified as direct addressing?
 * ------------------------------------------------------------------------ */

static void FillAbsAddr(tAdrVals *pAdrVals, LongWord Address, Boolean IsIO, Boolean ForceShort)
{
  Word Offset = Address & 0xffff;

  if (Segmented() && !IsIO)
  {
    pAdrVals->Vals[pAdrVals->Cnt++] = (Address >> 8) & 0x7f00;
    if ((Offset <= 0xff) && ForceShort)
      pAdrVals->Vals[pAdrVals->Cnt - 1] |= Offset;
    else
    {
      pAdrVals->Vals[pAdrVals->Cnt - 1] |= 0x8000;
      pAdrVals->Vals[pAdrVals->Cnt++] = Offset;
    }
  }
  else
    pAdrVals->Vals[pAdrVals->Cnt++] = Offset;
}

/*!------------------------------------------------------------------------
 * \fn     FillImmVal(tAdrVals *pAdrVals, LongWord Value, tSymbolSize OpSize)
 * \brief  fill address values from immediate value
 * \param  pAdrVals dest buffer
 * \param  Value value to fill in
 * \param  OpSize used operand size
 * ------------------------------------------------------------------------ */

static void FillImmVal(tAdrVals *pAdrVals, LongWord Value, tSymbolSize OpSize)
{
  switch ((int)OpSize)
  {
    case eSymbolSize3Bit:
      pAdrVals->Val = Value & 7;
      break;
    case eSymbolSize4Bit:
      pAdrVals->Val = Value & 15;
      break;
    case eSymbolSize8Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = (Value & 0xff) | ((Value & 0xff) << 8);
      break;
    case eSymbolSize16Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = Value & 0xffff;
      break;
    case eSymbolSize32Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = (Value >> 16) & 0xffff;
      pAdrVals->Vals[pAdrVals->Cnt++] = Value & 0xffff;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAddrPart(const tStrComp *pArg, LongInt Disp, tAdrVals *pAdrVals, Boolean IsDirect, Boolean IsIO, Boolean *pIsDirect)
 * \brief  decode address part, for direct/immediate or indexed mode
 * \param  pArg source argument
 * \param  Disp displacement to add to value
 * \param  pAdrVals binary coding of addressing mode
 * \param  IsDirect force treatment as direct address mode
 * \param  IsIO I/O space instead of memory space
 * \param  pIsDirect classified as direct addressing?
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeAddrPart(const tStrComp *pArg, LongInt Disp, tAdrVals *pAdrVals, Boolean IsDirect, Boolean IsIO, Boolean *pIsDirect)
{
  LongWord Address;
  tEvalResult EvalResult;
  Boolean ForceLong;

  Address = DecodeAddrPartNum(pArg, Disp, &EvalResult, IsDirect, IsIO, &ForceLong, pIsDirect);
  if (EvalResult.OK)
  {
    ChkSpace(IsIO ? SegIO : SegCode, EvalResult.AddrSpaceMask);
    if (!pIsDirect || *pIsDirect)
      FillAbsAddr(pAdrVals, Address, IsIO, ForceLong);
    else
      FillImmVal(pAdrVals, Address, OpSize);
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, unsigned ModeMask, tAdrVals *pAdrVals)
 * \brief  decode address expression
 * \param  pArg source argument
 * \param  ModeMask bit mask of allowed addressing modes
 * \param  pAdrVals binary coding of addressing mode
 * \return decoded mode
 * ------------------------------------------------------------------------ */

/* This is necessary to find the split position when a short address is used as
   base, i.e. |addr|(rn).  | is also the OR operator, and I don't want to get
   false positives on other targets on stuff like (...)|(...): */

static int ShortQualifier(const char *pArg, int NextNonBlankPos, int SplitPos)
{
  int FirstNonBlankPos;

  for (FirstNonBlankPos = 0; FirstNonBlankPos < NextNonBlankPos; FirstNonBlankPos++)
    if (!as_isspace(pArg[FirstNonBlankPos]))
      break;
  return ((FirstNonBlankPos < NextNonBlankPos) && (pArg[FirstNonBlankPos] == '|') && (pArg[NextNonBlankPos] == '|')) ? SplitPos : -1;
}

static tAdrMode DecodeAdr(const tStrComp *pArg, unsigned ModeMask, tAdrVals *pAdrVals)
{
  tSymbolSize ArgSize;
  int ArgLen, SplitPos, z;
  Boolean IsIO = !!(ModeMask & MModIO);
  tChkRegSize ChkRegSize_Addr = IsIO ? ChkRegSize_IOAddr : ChkRegSize_MemAddr, ChkRegSizeForIOIndir;
  Boolean IsDirect;

  ClearAdrVals(pAdrVals);

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    LongWord Result;
    Boolean OK;

    if (ImmOpSize == eSymbolSizeUnknown)
      ImmOpSize = OpSize;
    switch (ImmOpSize)
    {
      case eSymbolSize3Bit:
        pAdrVals->Val = EvalStrIntExpressionOffs(pArg, 1, UInt3, &OK);
        break;
      case eSymbolSize4Bit:
        pAdrVals->Val = EvalStrIntExpressionOffs(pArg, 1, UInt4, &OK);
        break;
      case eSymbolSize8Bit:
        Result = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        if (OK)
          pAdrVals->Vals[pAdrVals->Cnt++] = (Result & 0xff) | ((Result & 0xff) << 8);
        break;
      case eSymbolSize16Bit:
        Result = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        if (OK)
          pAdrVals->Vals[pAdrVals->Cnt++] = Result & 0xffff;
        break;
      case eSymbolSize32Bit:
        Result = EvalStrIntExpressionOffs(pArg, 1, Int32, &OK);
        if (OK)
        {
          pAdrVals->Vals[pAdrVals->Cnt++] = (Result >> 16) & 0xffff;
          pAdrVals->Vals[pAdrVals->Cnt++] = Result & 0xffff;
        }
        break;
      default:
        WrStrErrorPos(ErrNum_UndefOpSizes, pArg);
        return pAdrVals->Mode;
    }
    if (OK)
    {
      /* Immediate: is the same coding as indirect, however with register field = 0.
         This why register 0 is not allowed for indirect mode.
         As an exception, immediate value is placed in pAdrVals->Val for bit number: */

      pAdrVals->Mode = eModImm;
      if (OpSize == eSymbolSizeUnknown)
        OpSize = (tSymbolSize)ImmOpSize;
    }
    goto chk;
  }

  /* Register (R): For AMD syntax, Rn is equivalent to Rn^ resp. @Rn, if I/O ports
     are addressed indirectly: */

  ChkRegSizeForIOIndir = (AMDSyntax && !(ModeMask & MModReg) && (ModeMask & MModIReg) && IsIO) ? ChkRegSize_Addr : NULL;
  switch (DecodeReg(pArg, &pAdrVals->Val, &ArgSize, ChkRegSizeForIOIndir, False))
  {
    case eRegAbort:
      return pAdrVals->Mode;
    case eIsReg:
    {
      if (ChkRegSizeForIOIndir)
        pAdrVals->Mode = eModIReg;
      else
      {
        if (!SetOpSize(ArgSize, pArg))
          return pAdrVals->Mode;
        pAdrVals->Mode = eModReg;
      }
      goto chk;
    }
    default:
      break;
  }

  /* control register */

  for (z = 0; z < CtlRegCnt; z++)
    if (!as_strcasecmp(pArg->str.p_str, CtlRegs[z].pName))
    {
      if (!(CtlRegs[z].Flags & (Segmented() ? eSegMode : eNonSegMode)))
      {
        WrStrErrorPos(ErrNum_InvCtrlReg, pArg);
        return pAdrVals->Mode;
      }
      if (!SetOpSize(CtlRegs[z].Size, pArg))
        return pAdrVals->Mode;
      if (!CheckSup(!!(CtlRegs[z].Flags & ePrivileged)))
        return pAdrVals->Mode;
      pAdrVals->Val = CtlRegs[z].Code;
      pAdrVals->Mode = eModCtl;
      goto chk;
    }

  /* Register indirect (IR): */

  if (*pArg->str.p_str == '@')
  {
    tStrComp RegComp;

    StrCompRefRight(&RegComp, pArg, 1);
    if (eIsReg == DecodeReg(&RegComp, &pAdrVals->Val, &ArgSize, ChkRegSize_Addr, True))
    {
      if (!pAdrVals->Val) WrStrErrorPos(ErrNum_InvAddrMode, &RegComp);
      else
        pAdrVals->Mode = eModIReg;
    }
    goto chk;
  }
  if (AMDSyntax
   && ((ArgLen = strlen(pArg->str.p_str))> 1)
   && (pArg->str.p_str[ArgLen - 1] == '^'))
  {
    String Reg;
    tStrComp RegComp;

    StrCompMkTemp(&RegComp, Reg, sizeof(Reg));
    StrCompCopySub(&RegComp, pArg, 0, ArgLen - 1);
    switch (DecodeReg(&RegComp, &pAdrVals->Val, &ArgSize, ChkRegSize_Addr, False))
    {
      case eRegAbort:
        return pAdrVals->Mode;
      case eIsReg:
        if (!pAdrVals->Val) WrStrErrorPos(ErrNum_InvAddrMode, &RegComp);
        else
          pAdrVals->Mode = eModIReg;
        goto chk;
      default:
        break;
    }
  }

  /* Indexed, base... */

  SplitPos = FindDispBaseSplitWithQualifier(pArg->str.p_str, &ArgLen, ShortQualifier, "()");
  if (SplitPos > 0)
  {
    String OutStr, InStr;
    tStrComp OutArg, InArg;

    /* copy out base + index components */

    StrCompMkTemp(&OutArg, OutStr, sizeof(OutStr));
    StrCompMkTemp(&InArg, InStr, sizeof(InStr));
    StrCompCopySub(&OutArg, pArg, 0, SplitPos);
    KillPostBlanksStrComp(&OutArg);
    StrCompCopySub(&InArg, pArg, SplitPos + 1, ArgLen - SplitPos - 2);
    switch (DecodeReg(&OutArg, &pAdrVals->Val, &ArgSize, ChkRegSize_Addr, False))
    {
      case eIsReg: /* [R]Rx(...) */
        if (!pAdrVals->Val) WrStrErrorPos(ErrNum_InvAddrMode, &OutArg);
        else if (*InArg.str.p_str == '#')
        {
          Boolean OK;

          pAdrVals->Vals[pAdrVals->Cnt++] = EvalStrIntExpressionOffs(&InArg, 1, Int16, &OK);
          if (OK)
            pAdrVals->Mode = eModBaseAddress;
        }
        else if (DecodeReg(&InArg, &pAdrVals->Vals[pAdrVals->Cnt], &ArgSize, ChkRegSize_Idx, True) == eIsReg)
        {
          pAdrVals->Vals[pAdrVals->Cnt] <<= 8;
          pAdrVals->Cnt++;
          pAdrVals->Mode = eModBaseIndexed;
        }
        goto chk;
      case eIsNoReg: /* abs(...) */
      {
        switch (DecodeReg(&InArg, &pAdrVals->Val, &ArgSize, ChkRegSize_Idx, False))
        {
          case eIsReg: /* abs(Rx) */
            if (DecodeAddrPart(&OutArg, 0, pAdrVals, True, IsIO, NULL))
              pAdrVals->Mode = eModIndexed;
            break;
          case eIsNoReg: /* abs/imm(delta) -> direct/imm*/
          {
            Boolean OK;
            LongInt Disp = EvalStrIntExpression(&InArg, Int16, &OK);

            if (OK && DecodeAddrPart(&OutArg, Disp, pAdrVals, !AMDSyntax || !(ModeMask & MModImm), IsIO, &IsDirect))
              goto DirectOrImmediate;
            break;
          }
          default:
            return pAdrVals->Mode;
        }
        goto chk;
      }
      case eRegAbort:
        return pAdrVals->Mode;
    }
  }

  /* Absolute: is the same coding as indexed, however with index register field = 0.
     This is why register 0 is not allowed for indexed mode. */

  if (DecodeAddrPart(pArg, 0, pAdrVals, !AMDSyntax || !(ModeMask & MModImm), IsIO, &IsDirect))
  {
DirectOrImmediate:
    if (IsDirect)
    {
      pAdrVals->Val = 0;
      pAdrVals->Mode = eModDirect;
    }
    else
      pAdrVals->Mode = eModImm;
  }

chk:
  if ((pAdrVals->Mode != eModNone) & !((ModeMask >> pAdrVals->Mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    ClearAdrVals(pAdrVals);
  }
  return pAdrVals->Mode;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * bits 0...2/3: bit position
 * bits 3/4...25/26: address in memory space (7+16 bits)
 * bit  28: operand size (0/1 for 8/16 bits)
 * bit  29: force short address in segmented mode
 */

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, tSymbolSize OpSize, LongWord Address, Boolean ForceShort)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in word
 * \param  OpSize operand size (8/16)
 * \param  Address memory address
 * \param  ForceShort force short address in segmented mode
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, tSymbolSize OpSize, LongWord Address, Boolean ForceShort)
{
  LongWord CodeOpSize = (OpSize == eSymbolSize16Bit) ? 1 : 0;
  int AddrShift = CodeOpSize + 3;

  return BitPos
       | ((Address & 0x7fffff) << AddrShift)
       | (CodeOpSize << 28)
       | ((!!ForceShort) << 29);
}

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const char *pBitArg, Boolean *pOK, ShortInt OpSize)
 * \brief  evaluate constant bit position, with bit range depending on operand size
 * \param  pBitArg bit position argument
 * \param  pOK returns True if OK
 * \param  OpSize operand size (0,1,2 -> 8,16,32 bits)
 * \return bit position as number
 * ------------------------------------------------------------------------ */

static Byte EvalBitPosition(const tStrComp *pBitArg, Boolean *pOK, tSymbolSize OpSize)
{
  IntType Type;

  switch (OpSize)
  {
    case eSymbolSize8Bit:
      Type = UInt3;
      goto common;
    case eSymbolSize16Bit:
      Type = UInt4;
      goto common;
    default:
      WrStrErrorPos(ErrNum_InvOpSize, pBitArg);
      *pOK = False;
      return 0;
    common:
      return EvalStrIntExpressionOffs(pBitArg, !!(*pBitArg->str.p_str == '#'), Type, pOK);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pMemArg, const tStrComp *pBitArg, tSymbolSize OpSize)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pMemArg register argument
 * \param  pBitArg bit argument
 * \param  OpSize register size (0/1 = 8/16 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pMemArg, const tStrComp *pBitArg, tSymbolSize OpSize)
{
  tEvalResult EvalResult;
  LongWord Addr;
  Byte BitPos;
  Boolean ForceShort;

  BitPos = EvalBitPosition(pBitArg, &EvalResult.OK, OpSize);
  if (!EvalResult.OK)
    return False;

  Addr = DecodeAddrPartNum(pMemArg, 0, &EvalResult, True, False, &ForceShort, NULL);
  if (!EvalResult.OK)
    return False;

  *pResult = AssembleBitSymbol(BitPos, OpSize, Addr, ForceShort);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop, tSymbolSize OpSize)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \param  OpSize register size (0/1 = 8/16 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg(LongWord *pResult, int Start, int Stop, tSymbolSize OpSize)
{
  *pResult = 0;

  /* Just one argument -> parse as bit argument */

  if (Start == Stop)
  {
    tEvalResult EvalResult;

    *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start], UInt32, &EvalResult);
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
 * \fn     DissectBitSymbol(LongWord BitSymbol, LongWord *pAddress, Byte *pBitPos, tSymbolSize *pOpSize, Boolean *pForceShort)
 * \brief  transform compact represenation of bit symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \param  pOpSize returns register size (0/1 for 8/16 bits)
 * \param  pForceShort returns force of short address in segmented mode
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, LongWord *pAddress, Byte *pBitPos, tSymbolSize *pOpSize, Boolean *pForceShort)
{
  *pForceShort = ((BitSymbol >> 29) & 1);
  *pOpSize = (tSymbolSize)((BitSymbol >> 28) & 1);
  switch (*pOpSize)
  {
    case eSymbolSize8Bit:
      *pAddress = (BitSymbol >> 3) & 0x7ffffful;
      *pBitPos = BitSymbol & 7;
      break;
    case eSymbolSize16Bit:
      *pAddress = (BitSymbol >> 4) & 0x7ffffful;
      *pBitPos = BitSymbol & 15;
      break;
    default:
      break;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_Z8000(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_Z8000(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  LongWord Address;
  tSymbolSize OpSize;
  Boolean ForceShort;
  char Attribute;

  DissectBitSymbol(Inp, &Address, &BitPos, &OpSize, &ForceShort);
  Attribute = "bw"[OpSize];
  if (HexStartCharacter == 'A')
    Attribute = as_toupper(Attribute);
  as_snprintf(pDest, DestSize, "%s%~.*u%s%s(%c).%u",
              ForceShort ? "|" : "",
              ListRadixBase, (unsigned)Address, GetIntConstIntelSuffix(ListRadixBase),
              ForceShort ? "|" : "",
              Attribute, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandZ8000Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandZ8000Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    tSymbolSize OpSize = (pStructElem->OpSize == eSymbolSizeUnknown) ? eSymbolSize8Bit : pStructElem->OpSize;

    if (!ChkRange(Address, 0, 0x7fffff)
     || !ChkRange(pStructElem->BitPos, 0, (8 << OpSize) - 1))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, OpSize, Address, False), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*!------------------------------------------------------------------------
 * \fn     SetOpSizeFromCode(Word Code)
 * \brief  set operand size of instruction from insn name
 * \param  Code contains size in MSB
 * ------------------------------------------------------------------------ */

static Boolean SetOpSizeFromCode(Word Code)
{
  Byte Size = Hi(Code);

  return SetOpSize((Size == 0xff) ? eSymbolSizeUnknown : (tSymbolSize)Size, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     AppendCode(Word Code)
 * \brief  append instruction word
 * \param  pVals code word to append
 * ------------------------------------------------------------------------ */

static void AppendCode(Word Code)
{
  WAsmCode[CodeLen >> 1] = Code;
  CodeLen += 2;
}

/*!------------------------------------------------------------------------
 * \fn     AppendAdrVals(const tAdrVals *pVals)
 * \brief  append address argument(s)
 * \param  pVals values to append
 * ------------------------------------------------------------------------ */

static void AppendAdrVals(const tAdrVals *pVals)
{
  memcpy(&WAsmCode[CodeLen >> 1], pVals->Vals, pVals->Cnt << 1);
  CodeLen += pVals->Cnt << 1;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCondition(const tStrComp *pArg, Word *pCondition)
 * \brief  decode condition argument
 * \param  pArg source argument (NULL for non-present condition)
 * \param  pCondition result buffer
 * \return True if found
 * ------------------------------------------------------------------------ */

static Boolean DecodeCondition(const tStrComp *pArg, Word *pCondition)
{
  int z;

  for (z = 0; Conditions[z].pName; z++)
    if (!as_strcasecmp(pArg ? pArg->str.p_str : "", Conditions[z].pName))
    {
      *pCondition = Conditions[z].Code;
      return True;
    }
  WrStrErrorPos(ErrNum_UndefCond, pArg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeImm1_16(const tStrComp *pArg, tEvalResult *pEvalResult)
 * \brief  decode immediate argument from 1..16 and return as 0..15
 * \param  pArg source argument
 * \param  pEvalResult evaluation result
 * \return value
 * ------------------------------------------------------------------------ */

static Word DecodeImm1_16(const tStrComp *pArg, tEvalResult *pEvalResult)
{
  Word Result;

  Result = EvalStrIntExpressionOffsWithResult(pArg, !!(*pArg->str.p_str == '#'), UInt5, pEvalResult);
  if (pEvalResult->OK)
  {
    if (mFirstPassUnknownOrQuestionable(pEvalResult->Flags))
      Result = 1;
    else
      pEvalResult->OK = ChkRange(Result, 1, 16);
    Result--;
  }
  return Result;
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Index)
 * \brief  decode instruction without arguments
 * \param  Index machine code of instruction
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && CheckSup(pOrder->Privileged))
  {
    CodeLen = 2;
    WAsmCode[0] = pOrder->Code;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLD(Word Code)
 * \brief  decode LD/LDB/LDL instruction
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeLD(Word Code)
{
  tAdrVals DestAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  switch (DecodeAdr(&ArgStr[1], MModNoImm, &DestAdrVals))
  {
    case eModReg:
    {
      tAdrVals SrcAdrVals;

      switch (DecodeAdr(&ArgStr[2], MModAll, &SrcAdrVals))
      {
        case eModReg:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x9400 : (0xa000 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModIReg:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1400 : (0x2000 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x5400 : (0x6000 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        case eModBaseAddress:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x3500 : (0x3000 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        case eModBaseIndexed:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x7500 : (0x7000 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        case eModImm:
          if (OpSize == eSymbolSize8Bit)
            AppendCode(0xc000 | (DestAdrVals.Val << 8) | (SrcAdrVals.Vals[0] & 0xff));
#if OPT_LD_LDK
          else if ((OpSize == eSymbolSize16Bit) && (SrcAdrVals.Vals[0] < 16))
            AppendCode(0xbd00 | (DestAdrVals.Val << 4) | (SrcAdrVals.Vals[0] & 0x0f));
#endif
          else
          {
            AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1400 : (0x2000 | (OpSize << 8)))
                     | DestAdrVals.Val);
            AppendAdrVals(&SrcAdrVals);
          }
          break;
        default:
          break;
      }
      break;
    }
    case eModIReg:
    {
      tAdrVals SrcAdrVals;

      ImmOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;
      switch (DecodeAdr(&ArgStr[2], MModReg | MModImm, &SrcAdrVals))
      {
        case eModReg:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1d00 : (0x2e00 | (OpSize << 8)))
                   | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
          break;
        case eModImm:
          if (OpSize > eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
          else
          {
            AppendCode(0x0c05 | (OpSize << 8) | (DestAdrVals.Val << 4));
            AppendAdrVals(&SrcAdrVals);
          }
          break;
        default:
          break;
      }
      break;
    }
    case eModDirect:
    case eModIndexed:
    {
      tAdrVals SrcAdrVals;

      ImmOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;
      switch (DecodeAdr(&ArgStr[2], MModReg | MModImm, &SrcAdrVals))
      {
        case eModReg:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x5d00 : (0x6e00 | (OpSize << 8)))
                   | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
          AppendAdrVals(&DestAdrVals);
          break;
        case eModImm:
          if (OpSize > eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
          else
          {
            AppendCode(0x4c05 | (OpSize << 8) | (DestAdrVals.Val << 4));
            AppendAdrVals(&DestAdrVals);
            AppendAdrVals(&SrcAdrVals);
          }
          break;
        default:
          break;
      }
      break;
    }
    case eModBaseAddress:
    {
      tAdrVals SrcAdrVals;

      if (DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals) == eModReg)
      {
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x3700 : (0x3200 | (OpSize << 8)))
                 | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
        AppendAdrVals(&DestAdrVals);
      }
      break;
    }
    case eModBaseIndexed:
    {
      tAdrVals SrcAdrVals;

      if (DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals) == eModReg)
      {
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x7700 : (0x7200 | (OpSize << 8)))
                 | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
        AppendAdrVals(&DestAdrVals);
      }
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDA(Word Code)
 * \brief  decode LDA instruction
 * ------------------------------------------------------------------------ */

static void DecodeLDA(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;

  UNUSED(Code);

  SetOpSize(AddrRegSize(), &OpPart);
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals))
    switch (DecodeAdr(&ArgStr[2], MModDirect | MModIndexed | MModBaseAddress | MModBaseIndexed, &SrcAdrVals))
    {
      case eModDirect:
      case eModIndexed:
        AppendCode(0x7600 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      case eModBaseAddress:
        AppendCode(0x3400 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      case eModBaseIndexed:
        AppendCode(0x7400 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDAR(Word Code)
 * \brief  decode LDAR instruction
 * ------------------------------------------------------------------------ */

static void DecodeLDAR(Word Code)
{
  tAdrVals DestAdrVals;

  SetOpSize(AddrRegSize(), &OpPart);
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals))
  {
    tEvalResult EvalResult;
    LongWord Addr = EvalStrIntExpressionWithResult(&ArgStr[2], MemIntType, &EvalResult);
    if (EvalResult.OK)
    {
      Word Delta;

      if (!mFirstPassUnknownOrQuestionable(EvalResult.Flags) && (GetSegment(Addr) != GetSegment(EProgCounter())))
        WrStrErrorPos(ErrNum_InAccSegment, &ArgStr[2]);

      Delta = (Addr & 0xffff) - ((EProgCounter() + 4) & 0xffff);
      AppendCode(Code | DestAdrVals.Val);
      AppendCode(Delta);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDCTL(Word Code)
 * \brief  decode LDCTL(B) instruction
 * ------------------------------------------------------------------------ */

static void DecodeLDCTL(Word Code)
{
  tAdrVals DestAdrVals;

  UNUSED(Code);
  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2)
   && CheckSup(True))
    switch (DecodeAdr(&ArgStr[1], MModReg | MModCtl, &DestAdrVals))
    {
      case eModReg:
      {
        tAdrVals SrcAdrVals;

        if (DecodeAdr(&ArgStr[2], MModCtl, &SrcAdrVals))
          AppendCode((OpSize ? 0x7d00 : 0x8c00) | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
        break;
      }
      case eModCtl:
      {
        tAdrVals SrcAdrVals;

        if (DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals))
          AppendCode((OpSize ? 0x7d08 : 0x8c08) | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        break;
      }
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDR(Word Code)
 * \brief  decode LDR[L|B]
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDR(Word Code)
{
  Word Reg;
  tSymbolSize RegSize;
  int RegArgIndex = 1;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2))
  switch (DecodeReg(&ArgStr[RegArgIndex], &Reg, &RegSize, ChkRegSize_8To32, False))
  {
    case eIsNoReg:
      RegArgIndex = 2;
      if (DecodeReg(&ArgStr[RegArgIndex], &Reg, &RegSize, ChkRegSize_8To32, True) != eIsReg)
        return;
      Code |= 2;
      /* fall-thru */
    case eIsReg:
    {
      tEvalResult EvalResult;
      LongWord Addr;

      if (!SetOpSize(RegSize, &ArgStr[RegArgIndex]))
        return;
      if (OpSize > eSymbolSize32Bit)
      {
        WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[RegArgIndex]);
        return;
      }

      Addr = EvalStrIntExpressionWithResult(&ArgStr[3 - RegArgIndex], MemIntType, &EvalResult);
      if (EvalResult.OK)
      {
        Word Delta;

        if (!mFirstPassUnknownOrQuestionable(EvalResult.Flags) && (GetSegment(Addr) != GetSegment(EProgCounter())))
          WrStrErrorPos(ErrNum_InAccSegment, &ArgStr[2]);

        Delta = (Addr & 0xffff) - ((EProgCounter() + 4) & 0xffff);
        AppendCode(((Lo(Code) | ((OpSize == eSymbolSize32Bit) ? 0x5 : OpSize)) << 8) | Reg);
        AppendCode(Delta);
      }
      break;
    }
    case eRegAbort:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeADD(Word Code)
 * \brief  decode ADD/ADDB/ADDL instruction
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeADD(Word Code)
{
  tAdrVals DestAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  if (DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals) == eModReg)
  {
    tAdrVals SrcAdrVals;

    switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModDirect | MModIndexed, &SrcAdrVals))
    {
      case eModReg:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x9600 : (0x8000 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        break;
      case eModIReg:
      case eModImm:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1600 : (0x0000 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      case eModDirect:
      case eModIndexed:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x5600 : (0x4000 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSUB(Word Code)
 * \brief  decode SUB/SUBB/SUBL instruction
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeSUB(Word Code)
{
  tAdrVals DestAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  if (DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals) == eModReg)
  {
    tAdrVals SrcAdrVals;

    switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModDirect | MModIndexed, &SrcAdrVals))
    {
      case eModReg:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x9200 : (0x8200 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        break;
      case eModIReg:
      case eModImm:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1200 : (0x0200 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      case eModDirect:
      case eModIndexed:
        AppendCode(((OpSize == eSymbolSize32Bit) ? 0x5200 : (0x4200 | (OpSize << 8)))
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCP(Word Code)
 * \brief  decode CP/CPB/CPL instruction
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeCP(Word Code)
{
  tAdrVals DestAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  switch (DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &DestAdrVals))
  {
    case eModReg:
    {
      tAdrVals SrcAdrVals;

      switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModDirect | MModIndexed, &SrcAdrVals))
      {
        case eModReg:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x9000 : (0x8a00 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModIReg:
        case eModImm:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x1000 : (0x0a00 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(((OpSize == eSymbolSize32Bit) ? 0x5000 : (0x4a00 | (OpSize << 8)))
                   | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        default:
          break;
      }
      break;
    }
    case eModIReg:
    {
      tAdrVals SrcAdrVals;

      ImmOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;
      if (OpSize == eSymbolSize32Bit) WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
      else if (eModImm == DecodeAdr(&ArgStr[2], MModImm, &SrcAdrVals))
      {
        AppendCode(0x0c01 | (DestAdrVals.Val << 4)  | (OpSize << 8));
        AppendAdrVals(&SrcAdrVals);
      }
      break;
    }
    case eModDirect:
    case eModIndexed:
    {
      tAdrVals SrcAdrVals;

      ImmOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;
      if (OpSize == eSymbolSize32Bit) WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
      else if (eModImm == DecodeAdr(&ArgStr[2], MModImm, &SrcAdrVals))
      {
        AppendCode(0x4c01 | (DestAdrVals.Val << 4)  | (OpSize << 8));
        AppendAdrVals(&DestAdrVals);
        AppendAdrVals(&SrcAdrVals);
      }
      break;
    }
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeADC_SBC(Word Code)
 * \brief  decode ADC/SBC instructions
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeADC_SBC(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  if ((DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals) == eModReg)
   && (DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals) == eModReg))
  {
    if (OpSize > eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else
      AppendCode(0x8000 | (((Code & 0xfe) | OpSize) << 8)
               | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAND_OR_XOR(Word Code)
 * \brief  decode AND/OR/XOR instructions
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeAND_OR_XOR(Word Code)
{
  tAdrVals DestAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(2, 2))
    return;

  if (DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals) == eModReg)
  {
    tAdrVals SrcAdrVals;

    if (OpSize > eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModDirect | MModIndexed, &SrcAdrVals))
    {
      case eModReg:
        AppendCode(0x8000 | (((Code & 0xfe) | OpSize) << 8)
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        break;
      case eModIReg:
      case eModImm:
        AppendCode(0x0000 | (((Code & 0xfe) | OpSize) << 8)
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      case eModDirect:
      case eModIndexed:
        AppendCode(0x4000 | (((Code & 0xfe) | OpSize) << 8)
                 | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
        AppendAdrVals(&SrcAdrVals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINC_DEC(Word Code)
 * \brief  decode increment/decrement instructions
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeINC_DEC(Word Code)
{
  tAdrVals AdrVals;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(1, 2)
   && DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &AdrVals))
  {
    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
    if (OpSize > eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
    else
    {
      tEvalResult EvalResult;
      Word ImmVal = 0;

      if (2 == ArgCnt)
        ImmVal = DecodeImm1_16(&ArgStr[2], &EvalResult);
      else
        EvalResult.OK = True;

      if (EvalResult.OK)
        switch (AdrVals.Mode)
        {
          case eModReg:
            AppendCode(0x8000 | ((Code | OpSize) << 8) | (AdrVals.Val << 4) | ImmVal);
            break;
          case eModIReg:
            AppendCode(0x0000 | ((Code | OpSize) << 8) | (AdrVals.Val << 4) | ImmVal);
            break;
          case eModDirect:
          case eModIndexed:
            AppendCode(0x4000 | ((Code | OpSize) << 8) | (AdrVals.Val << 4) | ImmVal);
            AppendAdrVals(&AdrVals);
            break;
          default:
            break;
        }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitOp(Word Code)
 * \brief  decode bit set/reset/test instructions
 * \param  Code instruction type
 * ------------------------------------------------------------------------ */

static void DecodeBitOp(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;
  tSymbolSize DestOpSize;

  if (!SetOpSizeFromCode(Code))
    return;

  switch (ArgCnt)
  {
    case 1:
    {
      LongWord PacketBit;

      if (DecodeBitArg(&PacketBit, 1, 1, OpSize))
      {
        LongWord Address;
        Byte BitPos;
        Boolean ForceShort;

        if (DissectBitSymbol(PacketBit, &Address, &BitPos, &DestOpSize, &ForceShort)
         && SetOpSize(DestOpSize, &ArgStr[1]))
        {
          ClearAdrVals(&SrcAdrVals);
          SrcAdrVals.Val = BitPos;
          ClearAdrVals(&DestAdrVals);
          FillAbsAddr(&DestAdrVals, Address, False, ForceShort);
          goto Abs2Imm; /* yeah! */
        }
      }
      break;
    }
    case 2:
      if (DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &DestAdrVals))
      {
        DestOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;

        switch (DestOpSize)
        {
          case eSymbolSize8Bit:
            ImmOpSize = eSymbolSize3Bit; break;
          case eSymbolSize16Bit:
            ImmOpSize = eSymbolSize4Bit; break;
          default:
            WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]); return;
        }
        OpSize = eSymbolSizeUnknown;

        switch (DecodeAdr(&ArgStr[2], MModReg | MModImm, &SrcAdrVals))
        {
          case eModImm:
            switch (DestAdrVals.Mode)
            {
              case eModReg:
                AppendCode(0x8000 | (((Code & 0xfe) | DestOpSize) << 8)
                         | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
                break;
              case eModIReg:
                AppendCode(0x0000 | (((Code & 0xfe) | DestOpSize) << 8)
                         | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
                break;
              case eModDirect:
              case eModIndexed:
              Abs2Imm:
                AppendCode(0x4000 | (((Code & 0xfe) | DestOpSize) << 8)
                         | (DestAdrVals.Val << 4) | SrcAdrVals.Val);
                AppendAdrVals(&DestAdrVals);
                break;
              default:
                break;
            }
            break;
          case eModReg:
            switch (DestAdrVals.Mode)
            {
              case eModReg:
                if (OpSize != eSymbolSize16Bit) WrStrErrorPos (ErrNum_InvOpSize, &ArgStr[2]);
                else
                {
                  AppendCode(0x0000 | (((Code & 0xfe) | DestOpSize) << 8) | SrcAdrVals.Val);
                  AppendCode(DestAdrVals.Val << 8);
                }
                break;
              case eModNone:
                break;
              default:
                WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
            }
            break;
          default:
            break;
        }
      }
      break;
    default:
      (void)ChkArgCnt(1, 2);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALL_JP(Word Code)
 * \brief  decode jump/call instructions
 * \param  instruction code
 * ------------------------------------------------------------------------ */

static void DecodeCALL_JP(Word Code)
{
  tAdrVals AdrVals;
  Boolean IsCALL = Code & 1;

  if (ChkArgCnt(1, 2 - IsCALL))
  {
    Word Condition;

    if (IsCALL)
      Condition = 0;
    else if (!DecodeCondition((ArgCnt == 2) ? &ArgStr[1] : NULL, &Condition))
      return;

    switch (DecodeAdr(&ArgStr[ArgCnt], MModIReg | MModDirect | MModIndexed, &AdrVals))
    {
      case eModIReg:
        AppendCode(0x0000 | (Code << 8) | (AdrVals.Val << 4) | Condition);
        break;
      case eModDirect:
      case eModIndexed:
        AppendCode(0x4000 | (Code << 8) | (AdrVals.Val << 4) | Condition);
        AppendAdrVals(&AdrVals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBranch(Word CodeWord MaxDist, Boolean Inverse, const tStrComp *pArg)
 * \brief  relative branch core decode
 * \param  Code instruction code (upper bits)
 * \param  MaxDist maximum positive distance (in bytes, e.g. even value)
 * \param  Inverse Displacement is subtraced and not added by CPU
 * \param  pArg source argument
 * ------------------------------------------------------------------------ */

static void DecodeBranch(Word Code, Word MaxDist, Boolean Inverse, const tStrComp *pArg)
{
  tEvalResult EvalResult;
  LongWord Addr = EvalStrIntExpressionWithResult(pArg, MemIntType, &EvalResult);

  if (EvalResult.OK)
  {
    Word Delta;

    /* considering wraparound in segment makes things a bit more complicated: */

    if (!mFirstPassUnknownOrQuestionable(EvalResult.Flags) && (GetSegment(Addr) != GetSegment(EProgCounter())))
      WrStrErrorPos(ErrNum_InAccSegment, pArg);

    Delta = (Addr & 0xffff) - ((EProgCounter() + 2) & 0xffff);
    if (Inverse)
      Delta = (~Delta) + 1;
    if ((Delta & 1) && !mFirstPassUnknownOrQuestionable(EvalResult.Flags)) WrStrErrorPos(ErrNum_AddrMustBeEven, pArg);
    else if ((Delta > MaxDist) && (Delta < (0xfffe - MaxDist)) && !mFirstPassUnknownOrQuestionable(EvalResult.Flags)) WrStrErrorPos(ErrNum_JmpDistTooBig, pArg);
    else
      AppendCode(Code | ((Delta >> 1) & (MaxDist + 1)));
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALR(Word Code)
 * \brief  decode CALR instruction
 * \param  Code instruction code (upper bits)
 * ------------------------------------------------------------------------ */

static void DecodeCALR(Word Code)
{
  if (ChkArgCnt(1, 1))
    DecodeBranch(Code, 4094, True, &ArgStr[1]);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJR(Word Code)
 * \brief  decode JR instruction
 * \param  Code instruction code (upper bits)
 * ------------------------------------------------------------------------ */

static void DecodeJR(Word Code)
{
  Word Condition;

  if (ChkArgCnt(1, 2)
   && DecodeCondition(ArgCnt == 2 ? &ArgStr[1] : NULL, &Condition))
    DecodeBranch(Code | (Condition << 8), 254, False, &ArgStr[ArgCnt]);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDJNZ(Word Code)
 * \brief  decode DJNZ instructions
 * \param  size spec
 * ------------------------------------------------------------------------ */

static void DecodeDJNZ(Word Code)
{
  tAdrVals AdrVals;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg, &AdrVals))
  {
    if (OpSize > eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
    else
    {
      tEvalResult EvalResult;
      LongWord Addr = EvalStrIntExpressionWithResult(&ArgStr[2], MemIntType, &EvalResult);

      if (EvalResult.OK)
      {
        Word Delta;

        /* considering wraparound in segment makes things a bit more complicated: */

        if (!mFirstPassUnknownOrQuestionable(EvalResult.Flags) && (GetSegment(Addr) != GetSegment(EProgCounter())))
          WrStrErrorPos(ErrNum_InAccSegment, &ArgStr[2]);

        Delta = (Addr & 0xffff) - ((EProgCounter() + 2) & 0xffff);
        if ((Delta & 1) && !mFirstPassUnknownOrQuestionable(EvalResult.Flags)) WrStrErrorPos(ErrNum_AddrMustBeEven, &ArgStr[2]);
        else if ((Delta > 0) && (Delta < 0xff02) && !mFirstPassUnknownOrQuestionable(EvalResult.Flags)) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[2]);
        else
        {
          Delta = (0x10000lu - Delta) >> 1;
          AppendCode(0xf000 | (AdrVals.Val << 8) | (OpSize << 7) | (Delta & 0x7f));
        }
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCLR_COM_NEG_TSET(Word Code)
 * \brief  decode CLR/COM/NEG/TSET instructions
 * \param  Code machine code + size spec
 * ------------------------------------------------------------------------ */

static void DecodeCLR_COM_NEG_TSET(Word Code)
{
  tAdrVals AdrVals;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(1,1)
   && DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &AdrVals))
  {
    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
    if (OpSize > eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else
    {
      Code = ((Code >> 4) & 0x0f) | (((Code & 0x0e) | OpSize) << 8);
      switch (AdrVals.Mode)
      {
        case eModReg:
          AppendCode(0x8000 | Code | (AdrVals.Val << 4));
          break;
        case eModIReg:
          AppendCode(0x0000 | Code | (AdrVals.Val << 4));
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(0x4000 | Code | (AdrVals.Val << 4));
          AppendAdrVals(&AdrVals);
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTEST(Word Code)
 * \brief  decode TEST instruction
 * \param  Code machine code + size spec
 * ------------------------------------------------------------------------ */

static void DecodeTEST(Word Code)
{
  tAdrVals AdrVals;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &AdrVals))
  {
    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
    if (OpSize > eSymbolSize32Bit) WrError(ErrNum_InvOpSize);
    else
    {
      Code = (Lo(Code) | ((OpSize == eSymbolSize32Bit) ? 0x10 : OpSize)) << 8 | ((OpSize == eSymbolSize32Bit) ? 8 : 4);
      switch (AdrVals.Mode)
      {
        case eModReg:
          AppendCode(0x8000 | Code | (AdrVals.Val << 4));
          break;
        case eModIReg:
          AppendCode(0x0000 | Code | (AdrVals.Val << 4));
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(0x4000 | Code | (AdrVals.Val << 4));
          AppendAdrVals(&AdrVals);
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEX(Word Code)
 * \brief  decode EX instruction
 * \param  Code machine code + size spec
 * ------------------------------------------------------------------------ */

static void DecodeEX(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &DestAdrVals)
   && DecodeAdr(&ArgStr[2], MModReg | ((DestAdrVals.Mode == eModReg) ? (MModIReg | MModDirect | MModIndexed) : 0), &SrcAdrVals))
  {
    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
    if (OpSize > eSymbolSize16Bit) WrError(ErrNum_InvOpSize);
    else
    {
      if (DestAdrVals.Mode != eModReg)
      {
        tAdrVals Swap;
        Swap = SrcAdrVals;
        SrcAdrVals = DestAdrVals;
        DestAdrVals = Swap;
      }
      Code = (Lo(Code) | OpSize) << 8;
      switch (SrcAdrVals.Mode)
      {
        case eModReg:
          AppendCode(0x8000 | Code | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModIReg:
          AppendCode(0x0000 | Code | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(0x4000 | Code | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEXTS(Word Code)
 * \brief  decode EXTS instruction
 * \param  Code machine code + size spec
 * ------------------------------------------------------------------------ */

static void DecodeEXTS(Word Code)
{
  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(1, 1))
  {
    tAdrVals AdrVals;

    if (OpSize != eSymbolSizeUnknown)
      OpSize++;
    if (DecodeAdr(&ArgStr[1], MModReg, &AdrVals))
    {
      Code = (Lo(Code) << 8) | 0x8000 | (AdrVals.Val << 4);
      switch (OpSize)
      {
        case eSymbolSize16Bit:
          AppendCode(Code | 0x0000);
          break;
        case eSymbolSize32Bit:
          AppendCode(Code | 0x000a);
          break;
        case eSymbolSize64Bit:
          AppendCode(Code | 0x0007);
          break;
        default:
          WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMULT_DIV(Word Code)
 * \brief  decode multiply/divide instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMULT_DIV(Word Code)
{
  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2))
  {
    tAdrVals DestAdrVals;

    if (OpSize != eSymbolSizeUnknown)
      OpSize++;
    if (DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals) == eModReg)
    {
      tAdrVals SrcAdrVals;

      if ((OpSize < eSymbolSize32Bit) || (OpSize > eSymbolSize64Bit))
      {
        WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
        return;
      }

      OpSize--;
      switch (DecodeAdr(&ArgStr[2], MModReg | MModImm | MModIReg | MModDirect | MModIndexed, &SrcAdrVals))
      {
        case eModReg:
          AppendCode(0x8000 | ((Code | (OpSize & 1)) << 8) | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          break;
        case eModImm:
        case eModIReg:
          AppendCode(0x0000 | ((Code | (OpSize & 1)) << 8) | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(0x4000 | ((Code | (OpSize & 1)) << 8) | (SrcAdrVals.Val << 4) | DestAdrVals.Val);
          AppendAdrVals(&SrcAdrVals);
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDAB(Word Code)
 * \brief  decode DAB instruction
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeDAB(Word Code)
{
  tAdrVals AdrVals;

  SetOpSize(eSymbolSize8Bit, &OpPart);
  if (ChkArgCnt(1,1) && DecodeAdr(&ArgStr[1], MModReg, &AdrVals))
    AppendCode(Code | (AdrVals.Val << 4));
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFLG(Word Code)
 * \brief  decode flag set/reset/complement instructions
 * \param  machine code
 * ------------------------------------------------------------------------ */

static void DecodeFLG(Word Code)
{
  if (ChkArgCnt(1, ArgCntMax))
  {
    int z;
    Word Num;
    static const char FlagNames[] = "PSZCV";

    for (z = 1; z <= ArgCnt; z++)
    {
      if (!as_strcasecmp(ArgStr[z].str.p_str, "P/V"))
        Num = 1 << 0;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "ZR"))
        Num = 1 << 2;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "CY"))
        Num = 1 << 3;
      else if (1 == strlen(ArgStr[z].str.p_str))
      {
        const char *pPos = strchr(FlagNames, as_toupper(*ArgStr[z].str.p_str));
        Num = pPos ? (1 << ((pPos - FlagNames) % 4)) : 0;
      }
      else
        Num = 0;

      if (!Num)
      {
        WrStrErrorPos(ErrNum_UnknownFlag, &ArgStr[z]);
        return;
      }
      else if (Code & (Num << 4))
      {
        WrStrErrorPos(ErrNum_DuplicateFlag, &ArgStr[z]);
        return;
      }
      else
        Code |= Num << 4;
    }
    AppendCode(Code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDI_EI(Word Code)
 * \brief  decode interrupt enable/disable instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeDI_EI(Word Code)
{
  if (ChkArgCnt(1, ArgCntMax)
   && CheckSup(True))
  {
    int z;
    Word Num;

    Code |= 3;
    for (z = 1; z <= ArgCnt; z++)
    {
      if (!as_strcasecmp(ArgStr[z].str.p_str, "VI"))
        Num = 1 << 1;
      else if (!as_strcasecmp(ArgStr[z].str.p_str, "NVI"))
        Num = 1 << 0;
      else
        Num = 0;

      if (!Num)
      {
        WrStrErrorPos(ErrNum_UnknownInt, &ArgStr[z]);
        return;
      }
      else if (!(Code & Num))
      {
        WrStrErrorPos(ErrNum_DuplicateInt, &ArgStr[z]);
        return;
      }
      else
        Code &= ~Num;
    }
    AppendCode(Code);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIN_SIN_OUT_SOUT(Word Code)
 * \brief  decode IN/SIN/OUT/SOUT
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeIN_SIN_OUT_SOUT(Word Code)
{
  tAdrVals RegAdrVals;
  Word IsOUT = Code & 2,
       IsSpecial = Code & 1;
  const tStrComp *pRegArg = IsOUT ? &ArgStr[2] : &ArgStr[1],
                 *pIOArg  = IsOUT ? &ArgStr[1] : &ArgStr[2];

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2)
   && CheckSup(True)
   && DecodeAdr(pRegArg, MModReg, &RegAdrVals))
  {
    tAdrVals IOAdrVals;

    if (OpSize > eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, pRegArg);
      return;
    }
    switch (DecodeAdr(pIOArg, MModIO | MModDirect | (IsSpecial ? 0 : MModIReg), &IOAdrVals))
    {
      case eModIReg:
        AppendCode(0x3c00 | (IsOUT << 8) | (OpSize << 8) | (IOAdrVals.Val << 4) | RegAdrVals.Val);
        break;
      case eModDirect:
        AppendCode(0x3a04 | (OpSize << 8) | (RegAdrVals.Val << 4) | Lo(Code));
        AppendAdrVals(&IOAdrVals);
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCPRep(Word Code)
 * \brief  decode CP[S](I/D)[R][D] instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeCPRep(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals, CntAdrVals;
  Word Condition;
  tAdrMode DestMode = (Code & 0x02) ? eModIReg : eModReg;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(3, 4)
   || !DecodeCondition((ArgCnt == 4) ? &ArgStr[4] : NULL, &Condition))
    return;

  if ((DecodeAdr(&ArgStr[1], 1 << DestMode, &DestAdrVals) == DestMode)
   && (DecodeAdr(&ArgStr[2], MModIReg, &SrcAdrVals) == eModIReg))
  {
    tSymbolSize SaveOpSize;

    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
     SaveOpSize = OpSize;
    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[3], MModReg, &CntAdrVals) == eModReg)
    {
      if (SaveOpSize == eSymbolSize32Bit) WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      else
      {
        int Index =
          ChkRegOverlap((unsigned)DestAdrVals.Val, ((DestMode == eModIReg) ? AddrRegSize() : SaveOpSize),
                        (unsigned)SrcAdrVals.Val, (int)AddrRegSize(),
                        (unsigned)CntAdrVals.Val, (int)eSymbolSize16Bit,
                        0, eSymbolSizeUnknown);

        if (Index >= 0)
          WrStrErrorPos(ErrNum_OverlapReg, &ArgStr[Index + 1]);
        AppendCode(0xba00 | (SaveOpSize << 8) | (SrcAdrVals.Val << 4) | Lo(Code));
        AppendCode((CntAdrVals.Val << 8) | (DestAdrVals.Val << 4) | Condition);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeINOUTRep(Word Code)
 * \brief  decode string I/O instructions
 * \param  Code instruction machine code
 * ------------------------------------------------------------------------ */

static void DecodeINOUTRep(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;
  Word IsOUT = Code & 0x20;

  if (!SetOpSizeFromCode(Code)
   || !CheckSup(True)
   || !ChkArgCnt(3, 3))
    return;

  if (DecodeAdr(&ArgStr[1], MModIReg | (IsOUT ? MModIO : 0), &DestAdrVals)
   && DecodeAdr(&ArgStr[2], MModIReg | (IsOUT ? 0 : MModIO), &SrcAdrVals))
  {
    tAdrVals CntAdrVals;
    tSymbolSize SaveOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;

    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[3], MModReg, &CntAdrVals))
    {
      AppendCode(0x3a00 | (SaveOpSize << 8) | (SrcAdrVals.Val << 4) | ((Code >> 4) & 15));
      AppendCode((CntAdrVals.Val << 8) | (DestAdrVals.Val << 4) | (Code & 15));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDRep(Word Code)
 * \brief  decode string copy instructions
 * \param  Code instruction machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDRep(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;

  if (!SetOpSizeFromCode(Code)
   || !ChkArgCnt(3, 3))
    return;

  if (DecodeAdr(&ArgStr[1], MModIReg, &DestAdrVals)
   && DecodeAdr(&ArgStr[2], MModIReg, &SrcAdrVals))
  {
    tAdrVals CntAdrVals;
    tSymbolSize SaveOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;

    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[3], MModReg, &CntAdrVals))
    {
      AppendCode(0xba00 | (SaveOpSize << 8) | (SrcAdrVals.Val << 4) | ((Code >> 4) & 15));
      AppendCode((CntAdrVals.Val << 8) | (DestAdrVals.Val << 4) | (Code & 15));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTRRep(Word Code)
 * \brief  decode string translate instructions
 * \param  Code instruction machine code
 * ------------------------------------------------------------------------ */

static void DecodeTRRep(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals, CntAdrVals;

  /* op size only applies to counter reg */

  OpSize = eSymbolSize16Bit;
  if (ChkArgCnt(3, 3)
   && DecodeAdr(&ArgStr[1], MModIReg, &DestAdrVals)
   && DecodeAdr(&ArgStr[2], MModIReg, &SrcAdrVals)
   && DecodeAdr(&ArgStr[3], MModReg, &CntAdrVals))
  {
    Boolean OK = True;
    int Index;

    /* R0/R1 in non-seg mode or RR0 in seg mode must not be used as src/dest.
       R1 should not be used as counter.
       This is only spelled out for TRDB, but due to the implicit usage of RH1
       in all translate instructions, this restriction effectively applies to
       them all.
       Since (R)R0 is anyway not allowed for indirect addressing, this reduces
       to check fo R1: */

    if (!Segmented() && (DestAdrVals.Val == 1))
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      OK = False;
    }
    if (!Segmented() && (SrcAdrVals.Val == 1))
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
      OK = False;
    }
    if (CntAdrVals.Val == 1)
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
      OK = False;
    }

    Index = ChkRegOverlap((unsigned)DestAdrVals.Val, AddrRegSize(),
                          (unsigned)SrcAdrVals.Val, (int)AddrRegSize(),
                          (unsigned)CntAdrVals.Val, (int)eSymbolSize16Bit,
                          0, eSymbolSizeUnknown);
    if (Index >= 0)
      WrStrErrorPos(ErrNum_OverlapReg, &ArgStr[Index + 1]);

    if (OK)
    {
      AppendCode(0xb800 | (DestAdrVals.Val << 4) | ((Code >> 4) & 15));
      AppendCode((CntAdrVals.Val << 8) | (SrcAdrVals.Val << 4) | (Code & 15));
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDK(Word Code)
 * \brief  decode LDK instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDK(Word Code)
{
  tAdrVals DestAdrVals;

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals))
  {
    Word Value;
    Boolean OK;

    if (OpSize != eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      return;
    }
    Value = EvalStrIntExpressionOffs(&ArgStr[2], !!(*ArgStr[2].str.p_str == '#'), UInt4, &OK);
    if (OK)
      AppendCode(Code | (DestAdrVals.Val << 4) | Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDM(Word Code)
 * \brief  decode LDM instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDM(Word Code)
{
  Word Count;
  tEvalResult EvalResult;
  tAdrVals RegAdrVals, MemAdrVals;
  int RegArgIndex;

  if (!ChkArgCnt(3, 3))
    return;

  Count = DecodeImm1_16(&ArgStr[3], &EvalResult);
  if (!EvalResult.OK)
    return;

  switch (DecodeAdr(&ArgStr[1], MModReg | MModIReg | MModDirect | MModIndexed, &RegAdrVals))
  {
    case eModReg:
      RegArgIndex = 1;
      if (!DecodeAdr(&ArgStr[2], MModIReg | MModDirect | MModIndexed, &MemAdrVals))
        return;
      goto common;
    case eModIReg:
    case eModDirect:
    case eModIndexed:
      MemAdrVals = RegAdrVals;
      Code |= 0x0008;
      if (!DecodeAdr(&ArgStr[2], MModReg, &RegAdrVals))
        return;
      RegArgIndex = 2;
      /* fall-thru */
    common:
      if (OpSize != eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[RegArgIndex]);
      else
      {
        if (RegAdrVals.Val + Count >= 16) WrStrErrorPos(ErrNum_RegNumWraparound, &ArgStr[3]);
        switch (MemAdrVals.Mode)
        {
          case eModIReg:
            AppendCode(Code | (MemAdrVals.Val << 4));
            AppendCode((RegAdrVals.Val << 8) | Count);
            break;
          case eModDirect:
          case eModIndexed:
            AppendCode(Code | 0x4000 | (MemAdrVals.Val << 4));
            AppendCode((RegAdrVals.Val << 8) | Count);
            AppendAdrVals(&MemAdrVals);
            break;
          default:
            break;
        }
      }
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDPS(Word Code)
 * \brief  decode LDPS instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDPS(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1)
   && CheckSup(True))
    switch (DecodeAdr(&ArgStr[1], MModIReg | MModDirect | MModIndexed, &AdrVals))
    {
      case eModIReg:
        AppendCode(Code | 0x0000 | (AdrVals.Val << 4));
        break;
      case eModDirect:
      case eModIndexed:
        AppendCode(Code | 0x4000 | (AdrVals.Val << 4));
        AppendAdrVals(&AdrVals);
        break;
      default:
        break;
    }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMREQ(Word Code)
 * \brief  decode MREQ instruction
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMREQ(Word Code)
{
  tAdrVals AdrVals;

  SetOpSize(eSymbolSize16Bit, &OpPart);
  if (ChkArgCnt(1, 1)
   && CheckSup(True)
   && DecodeAdr(&ArgStr[1], MModReg, &AdrVals))
    AppendCode(Code | (AdrVals.Val << 4));
}

/*!------------------------------------------------------------------------
 * \fn     DecodePUSH_POP(Word Code)
 * \brief  decode PUSH(L)/POP(L)
 * \param  Code machine code & size
 * ------------------------------------------------------------------------ */

static void DecodePUSH_POP(Word Code)
{
  tAdrVals RegAdrVals;
  Word IsPOP = Code & 0x04;
  int OpArg = !IsPOP + 1, RegArg = 3 - OpArg;

  if (SetOpSizeFromCode(Code)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[RegArg], MModIReg, &RegAdrVals))
  {
    tAdrVals OpAdrVals;

    ImmOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;
    if (DecodeAdr(&ArgStr[OpArg], MModReg | MModIReg | MModDirect | MModIndexed | (IsPOP ? 0 : MModImm), &OpAdrVals))
    {
      int Index;

      if (OpSize == eSymbolSizeUnknown)
        OpSize = eSymbolSize16Bit;
      if ((OpSize < eSymbolSize16Bit) || (OpSize > eSymbolSize32Bit))
      {
        WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[OpArg]);
        return;
      }
      switch (OpAdrVals.Mode)
      {
        case eModReg:
          Index = ChkRegOverlap((unsigned)OpAdrVals.Val, OpSize,
                                (unsigned)RegAdrVals.Val, (int)AddrRegSize(),
                                0, eSymbolSizeUnknown);
          break;
        case eModIReg:
          Index = ChkRegOverlap((unsigned)OpAdrVals.Val, AddrRegSize(),
                                (unsigned)RegAdrVals.Val, (int)AddrRegSize(),
                                0, eSymbolSizeUnknown);
          break;
        case eModIndexed:
          Index = ChkRegOverlap((unsigned)OpAdrVals.Val, eSymbolSize16Bit,
                                (unsigned)RegAdrVals.Val, (int)AddrRegSize(),
                                0, eSymbolSizeUnknown);
          break;
        default:
          Index = -1;
          break;
      }
      if (Index >= 0)
        WrStrErrorPos(ErrNum_OverlapReg, &ArgStr[Index ? RegArg : OpArg]);
      Code = Lo(Code) | ((OpSize == eSymbolSize16Bit) ? 2 : 0);
      switch (OpAdrVals.Mode)
      {
        case eModImm:
          if (OpSize != eSymbolSize16Bit) WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
          else
          {
            AppendCode(0x0d09 | (RegAdrVals.Val << 4));
            AppendAdrVals(&OpAdrVals);
          }
          break;
        case eModReg:
          AppendCode(0x8000 | (Code << 8) | (RegAdrVals.Val << 4) | OpAdrVals.Val);
          break;
        case eModIReg:
          AppendCode(0x0000 | (Code << 8) | (RegAdrVals.Val << 4) | OpAdrVals.Val);
          break;
        case eModDirect:
        case eModIndexed:
          AppendCode(0x4000 | (Code << 8) | (RegAdrVals.Val << 4) | OpAdrVals.Val);
          AppendAdrVals(&OpAdrVals);
          break;
        default:
          break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRET(Word Code)
 * \brief  decode RET instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeRET(Word Code)
{
  Word Condition;

  if (ChkArgCnt(0, 1)
   && DecodeCondition((ArgCnt >= 1) ? &ArgStr[1] : NULL, &Condition))
    AppendCode(Code | Condition);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRotate(Word Code)
 * \brief  handle rotate instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeRotate(Word Code)
{
  tAdrVals RegAdrVals;

  if (ChkArgCnt(1, 2)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], MModReg, &RegAdrVals))
  {
    Word Count = 1;
    tEvalResult EvalResult;

    if (OpSize > eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      return;
    }
    if (ArgCnt >= 2)
    {
      Count = EvalStrIntExpressionOffsWithResult(&ArgStr[2], !!(*ArgStr[2].str.p_str == '#'), UInt2, &EvalResult);
      if (EvalResult.OK && mFirstPassUnknownOrQuestionable(EvalResult.Flags))
        Count = 1;
    }
    if (ChkRange(Count, 1, 2))
      AppendCode(0xb200 | (OpSize << 8) | (RegAdrVals.Val << 4) | Lo(Code) | ((Count - 1) << 1));
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRotateDigit(Word Code)
 * \brief  decode nibble-wide rotates
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeRotateDigit(Word Code)
{
  tAdrVals LinkAdrVals, SrcAdrVals;

  SetOpSize(eSymbolSize8Bit, &OpPart);
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModReg, &LinkAdrVals)
   && DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals))
    AppendCode(Code | (SrcAdrVals.Val << 4) | LinkAdrVals.Val);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSC(Word Code)
 * \brief  decode SC instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Arg = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), Int8, &OK);

    if (OK)
      AppendCode(Code | (Arg & 0xff));
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSD(Word Code)
 * \brief  decode dynamic shifts
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSD(Word Code)
{
  tAdrVals DestAdrVals;

  if (ChkArgCnt(2, 2)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals))
  {
    tAdrVals SrcAdrVals;
    tSymbolSize SaveOpSize = (OpSize == eSymbolSizeUnknown) ? eSymbolSize16Bit : OpSize;

    if (SaveOpSize > eSymbolSize32Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      return;
    }
    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[2], MModReg, &SrcAdrVals))
    {
      AppendCode(0xb200 | (((SaveOpSize == eSymbolSize32Bit) ? 1 : SaveOpSize) << 8) | (DestAdrVals.Val << 4) | Lo(Code) | (SaveOpSize == eSymbolSize32Bit ? 4 : 0));
      AppendCode(SrcAdrVals.Val << 8);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeShift(Word Code)
 * \brief  decode statuc shifts
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeShift(Word Code)
{
  tAdrVals DestAdrVals;

  if (ChkArgCnt(1, 2)
   && SetOpSizeFromCode(Code)
   && DecodeAdr(&ArgStr[1], MModReg, &DestAdrVals))
  {
    Word Count;
    Word Negate = Code & 0x80;

    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize16Bit;
    if (OpSize > eSymbolSize32Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[1]);
      return;
    }

    if (ArgCnt >= 2)
    {
      tEvalResult EvalResult;

      Count = EvalStrIntExpressionOffsWithResult(&ArgStr[2], !!(*ArgStr[2].str.p_str == '#'), (OpSize == eSymbolSize32Bit) ? UInt6 : (OpSize == eSymbolSize16Bit) ? UInt5 : UInt4, &EvalResult);
      if (mFirstPassUnknownOrQuestionable(EvalResult.Flags))
        Count = 1;
      if (!ChkRange(Count, Negate ? 1 : 0, 8 << OpSize))
        return;
    }
    else
      Count = 1;

    if (Negate)
    {
      Code ^= Negate;
      Count = 0x10000ul - Count;
      if (OpSize == eSymbolSize8Bit)
        Count &= 0xff;
    }
    AppendCode(0xb200 | (((OpSize == eSymbolSize32Bit) ? 1 : OpSize) << 8) | (DestAdrVals.Val << 4) | Lo(Code) | (OpSize == eSymbolSize32Bit ? 4 : 0));
    AppendCode(Count);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTCC(Word Code)
 * \brief  decode TCC instruction
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeTCC(Word Code)
{
  Word Condition;
  tAdrVals DestAdrVals;

  if (ChkArgCnt(1, 2)
   && SetOpSizeFromCode(Code)
   && DecodeCondition((ArgCnt == 2) ? &ArgStr[1] : NULL, &Condition)
   && DecodeAdr(&ArgStr[ArgCnt], MModReg, &DestAdrVals))
  {
    if (OpSize > eSymbolSize16Bit)
    {
      WrStrErrorPos(ErrNum_InvOpSize, &ArgStr[2]);
      return;
    }
    AppendCode(0xae00 | (OpSize << 8) | (DestAdrVals.Val << 4) | Condition);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePORT(Word Code)
 * \brief  decode PORT instruction
 * ------------------------------------------------------------------------ */

static void DecodePORT(Word Code)
{
  UNUSED(Code);
  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDEFBIT(Word Code)
 * \brief  decode DEFBIT(B) instructions
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeDEFBIT(Word Code)
{
  LongWord BitSpec;

  OpSize = (tSymbolSize)Code;

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Boolean OK;
    Byte BitPos;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    BitPos = EvalBitPosition(&ArgStr[2], &OK, OpSize);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = OpSize;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandZ8000Bit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (DecodeBitArg(&BitSpec, 1, ArgCnt, OpSize))
    {
      *ListLine = '=';
      DissectBit_Z8000(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Instruction Table Buildup/Teardown */

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  Set up hash table
 * ------------------------------------------------------------------------ */

static void AddFixed(const char *NName, Word Code, Boolean Privileged)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = Code;
  FixedOrders[InstrZ].Privileged = Privileged;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddCtl(const char *pName, Word Code, tCtlFlags Flags, tSymbolSize Size)
{
  order_array_rsv_end(CtlRegs, tCtlReg);
  CtlRegs[InstrZ  ].pName = pName;
  CtlRegs[InstrZ  ].Code  = Code;
  CtlRegs[InstrZ  ].Flags = Flags;
  CtlRegs[InstrZ++].Size  = Size;
}

static void AddCondition(const char *pName, Word Code)
{
  order_array_rsv_end(Conditions, tCondition);
  Conditions[InstrZ].pName = pName;
  Conditions[InstrZ++].Code = Code;
}

static void AddSizeInstTable(const char *pName, unsigned SizeMask, Word Code, InstProc Proc)
{
  char Name[20];

  AddInstTable(InstTable, pName, Code | 0xff00, Proc);
  if (SizeMask & (1 << eSymbolSize8Bit))
  {
    as_snprintf(Name, sizeof(Name), "%sB", pName);
    AddInstTable(InstTable, Name, Code | (eSymbolSize8Bit << 8), Proc);
  }
  if (SizeMask & (1 << eSymbolSize32Bit))
  {
    as_snprintf(Name, sizeof(Name), "%sL", pName);
    AddInstTable(InstTable, Name, Code | (eSymbolSize32Bit << 8), Proc);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  InstrZ = 0;
  AddFixed("HALT" , 0x7a00 , True );
  AddFixed("IRET" , 0x7b00 , True );
  AddFixed("MBIT" , 0x7b0a , True );
  AddFixed("MRES" , 0x7b09 , True );
  AddFixed("MSET" , 0x7b08 , True );
  AddFixed("NOP"  , NOPCode, False);

  InstrZ = 0;
  AddCtl("FCW"     , 2, ePrivileged | eSegMode | eNonSegMode , eSymbolSize16Bit);
  AddCtl("REFRESH" , 3, ePrivileged | eSegMode | eNonSegMode , eSymbolSize16Bit);
  AddCtl("PSAPSEG" , 4, ePrivileged | eSegMode               , eSymbolSize16Bit);
  AddCtl("PSAPOFF" , 5, ePrivileged | eSegMode | eNonSegMode , eSymbolSize16Bit);
  AddCtl("PSAP"    , 5, ePrivileged            | eNonSegMode , eSymbolSize16Bit);
  AddCtl("NSPSEG"  , 6, ePrivileged | eSegMode               , eSymbolSize16Bit);
  AddCtl("NSPOFF"  , 7, ePrivileged | eSegMode | eNonSegMode , eSymbolSize16Bit);
  AddCtl("NSP"     , 7, ePrivileged            | eNonSegMode , eSymbolSize16Bit);
  AddCtl("FLAGS"   , 1,               eSegMode | eNonSegMode , eSymbolSize8Bit );
  CtlRegCnt = InstrZ;

  InstrZ = 0;
  AddCondition(""   , 0x08);
  AddCondition("F"  , 0x00);
  AddCondition("LT" , 0x01);
  AddCondition("LE" , 0x02);
  AddCondition("ULE", 0x03);
  AddCondition("PE" , 0x04);
  AddCondition("MI" , 0x05);
  AddCondition("Z"  , 0x06);
  AddCondition("ULT", 0x07);
  AddCondition("GE" , 0x09);
  AddCondition("GT" , 0x0a);
  AddCondition("UGT", 0x0b);
  AddCondition("PO" , 0x0c);
  AddCondition("PL" , 0x0d);
  AddCondition("NZ" , 0x0e);
  AddCondition("UGE", 0x0f);
  AddCondition("OV" , 0x04);
  AddCondition("EQ" , 0x06);
  AddCondition("C"  , 0x07);
  AddCondition("NOV", 0x0c);
  AddCondition("NE" , 0x0e);
  AddCondition("NC" , 0x0f);

  /* non-Zilog conditions */

  AddCondition("ZR" , 0x06);
  AddCondition("CY" , 0x07);
  AddCondition("LLE", 0x03);
  AddCondition("LLT", 0x07);
  AddCondition("LGT", 0x0b);
  AddCondition("LGE", 0x0f);
  AddCondition(NULL , 0);


  AddSizeInstTable("LD"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0, DecodeLD);
  AddInstTable(InstTable, "LDA" , 0, DecodeLDA);
  AddInstTable(InstTable, "LDAR", 0x3400, DecodeLDAR);
  AddInstTable(InstTable, "LDK" , 0xbd00, DecodeLDK);
  AddInstTable(InstTable, "LDM" , 0x1c01, DecodeLDM);
  AddInstTable(InstTable, "LDPS", 0x3900, DecodeLDPS);
  AddSizeInstTable("LDR", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x30, DecodeLDR);
  AddSizeInstTable("LDCTL", 1 << eSymbolSize8Bit, 0x00, DecodeLDCTL);
  AddSizeInstTable("ADD" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0, DecodeADD);
  AddSizeInstTable("SUB" , (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0, DecodeSUB);
  AddSizeInstTable("CP"  , (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0, DecodeCP);
  AddSizeInstTable("ADC" , 1 << eSymbolSize8Bit, 0x34, DecodeADC_SBC);
  AddSizeInstTable("SBC" , 1 << eSymbolSize8Bit, 0x36, DecodeADC_SBC);
  AddSizeInstTable("AND" , 1 << eSymbolSize8Bit, 0x06, DecodeAND_OR_XOR);
  AddSizeInstTable("OR"  , 1 << eSymbolSize8Bit, 0x04, DecodeAND_OR_XOR);
  AddSizeInstTable("XOR" , 1 << eSymbolSize8Bit, 0x08, DecodeAND_OR_XOR);
  AddSizeInstTable("INC" , 1 << eSymbolSize8Bit, 0x28, DecodeINC_DEC);
  AddSizeInstTable("DEC" , 1 << eSymbolSize8Bit, 0x2a, DecodeINC_DEC);
  AddSizeInstTable("BIT" , 1 << eSymbolSize8Bit, 0x26, DecodeBitOp);
  AddSizeInstTable("DIV" , 1 << eSymbolSize32Bit, 0x1a, DecodeMULT_DIV);
  AddSizeInstTable("MULT", 1 << eSymbolSize32Bit, 0x18, DecodeMULT_DIV);
  AddSizeInstTable("RES" , 1 << eSymbolSize8Bit, 0x22, DecodeBitOp);
  AddSizeInstTable("SET" , 1 << eSymbolSize8Bit, 0x24, DecodeBitOp);
  AddInstTable(InstTable, "CALL", 0x1f, DecodeCALL_JP);
  AddInstTable(InstTable, "JP"  , 0x1e, DecodeCALL_JP);
  AddInstTable(InstTable, "CALR", 0xd000, DecodeCALR);
  AddInstTable(InstTable, "JR"  , 0xe000, DecodeJR);
  AddInstTable(InstTable, "DJNZ" , 0xff00, DecodeDJNZ);
  AddInstTable(InstTable, "DBJNZ", eSymbolSize8Bit << 8, DecodeDJNZ);
  AddInstTable(InstTable, "RET", 0x9e00, DecodeRET);
  AddSizeInstTable("CLR" , 1 << eSymbolSize8Bit, 0x8c, DecodeCLR_COM_NEG_TSET);
  AddSizeInstTable("COM" , 1 << eSymbolSize8Bit, 0x0c, DecodeCLR_COM_NEG_TSET);
  AddSizeInstTable("NEG" , 1 << eSymbolSize8Bit, 0x2c, DecodeCLR_COM_NEG_TSET);
  AddSizeInstTable("TSET", 1 << eSymbolSize8Bit, 0x6c, DecodeCLR_COM_NEG_TSET);
  AddSizeInstTable("TEST", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x0c, DecodeTEST);
  AddSizeInstTable("EX", 1 << eSymbolSize8Bit, 0x2c, DecodeEX);
  AddSizeInstTable("EXTS", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x31, DecodeEXTS);
  AddInstTable(InstTable, "DAB", 0xb000, DecodeDAB);
  AddInstTable(InstTable, "COMFLG", 0x8d05, DecodeFLG);
  AddInstTable(InstTable, "SETFLG", 0x8d01, DecodeFLG);
  AddInstTable(InstTable, "RESFLG", 0x8d03, DecodeFLG);
  AddInstTable(InstTable, "DI", 0x7c00, DecodeDI_EI);
  AddInstTable(InstTable, "EI", 0x7c04, DecodeDI_EI);
  AddInstTable(InstTable, "MREQ", 0x7b0d, DecodeMREQ);
  AddSizeInstTable("IN"  , 1 << eSymbolSize8Bit, 0x00, DecodeIN_SIN_OUT_SOUT);
  AddSizeInstTable("SIN" , 1 << eSymbolSize8Bit, 0x01, DecodeIN_SIN_OUT_SOUT);
  AddSizeInstTable("OUT" , 1 << eSymbolSize8Bit, 0x02, DecodeIN_SIN_OUT_SOUT);
  AddSizeInstTable("SOUT", 1 << eSymbolSize8Bit, 0x03, DecodeIN_SIN_OUT_SOUT);
  AddSizeInstTable("CPD"  , 1 << eSymbolSize8Bit, 0x08, DecodeCPRep);
  AddSizeInstTable("CPDR" , 1 << eSymbolSize8Bit, 0x0c, DecodeCPRep);
  AddSizeInstTable("CPI"  , 1 << eSymbolSize8Bit, 0x00, DecodeCPRep);
  AddSizeInstTable("CPIR" , 1 << eSymbolSize8Bit, 0x04, DecodeCPRep);
  AddSizeInstTable("CPSD" , 1 << eSymbolSize8Bit, 0x0a, DecodeCPRep);
  AddSizeInstTable("CPSDR", 1 << eSymbolSize8Bit, 0x0e, DecodeCPRep);
  AddSizeInstTable("CPSI" , 1 << eSymbolSize8Bit, 0x02, DecodeCPRep);
  AddSizeInstTable("CPSIR", 1 << eSymbolSize8Bit, 0x06, DecodeCPRep);
  AddSizeInstTable("IND"  , 1 << eSymbolSize8Bit, 0x88, DecodeINOUTRep);
  AddSizeInstTable("INDR" , 1 << eSymbolSize8Bit, 0x80, DecodeINOUTRep);
  AddSizeInstTable("SIND" , 1 << eSymbolSize8Bit, 0x98, DecodeINOUTRep);
  AddSizeInstTable("SINDR", 1 << eSymbolSize8Bit, 0x90, DecodeINOUTRep);
  AddSizeInstTable("INI"  , 1 << eSymbolSize8Bit, 0x08, DecodeINOUTRep);
  AddSizeInstTable("INIR" , 1 << eSymbolSize8Bit, 0x00, DecodeINOUTRep);
  AddSizeInstTable("SINI" , 1 << eSymbolSize8Bit, 0x18, DecodeINOUTRep);
  AddSizeInstTable("SINIR", 1 << eSymbolSize8Bit, 0x10, DecodeINOUTRep);
  AddSizeInstTable("OUTD" , 1 << eSymbolSize8Bit, 0xa8, DecodeINOUTRep);
  AddSizeInstTable("OTDR" , 1 << eSymbolSize8Bit, 0xa0, DecodeINOUTRep);
  AddSizeInstTable("SOUTD", 1 << eSymbolSize8Bit, 0xb8, DecodeINOUTRep);
  AddSizeInstTable("SOTDR", 1 << eSymbolSize8Bit, 0xb0, DecodeINOUTRep);
  AddSizeInstTable("OUTI" , 1 << eSymbolSize8Bit, 0x28, DecodeINOUTRep);
  AddSizeInstTable("OTIR" , 1 << eSymbolSize8Bit, 0x20, DecodeINOUTRep);
  AddSizeInstTable("SOUTI", 1 << eSymbolSize8Bit, 0x38, DecodeINOUTRep);
  AddSizeInstTable("SOTIR", 1 << eSymbolSize8Bit, 0x30, DecodeINOUTRep);
  AddSizeInstTable("LDD"  , 1 << eSymbolSize8Bit, 0x98, DecodeLDRep);
  AddSizeInstTable("LDDR" , 1 << eSymbolSize8Bit, 0x90, DecodeLDRep);
  AddSizeInstTable("LDI"  , 1 << eSymbolSize8Bit, 0x18, DecodeLDRep);
  AddSizeInstTable("LDIR" , 1 << eSymbolSize8Bit, 0x10, DecodeLDRep);
  AddInstTable(InstTable, "TRDB"  , 0x80, DecodeTRRep);
  AddInstTable(InstTable, "TRDRB" , 0xc0, DecodeTRRep);
  AddInstTable(InstTable, "TRIB"  , 0x00, DecodeTRRep);
  AddInstTable(InstTable, "TRIRB" , 0x40, DecodeTRRep);
  AddInstTable(InstTable, "TRTDB" , 0xa0, DecodeTRRep);
  AddInstTable(InstTable, "TRTDRB", 0xee, DecodeTRRep);
  AddInstTable(InstTable, "TRTIB" , 0x20, DecodeTRRep);
  AddInstTable(InstTable, "TRTIRB", 0x6e, DecodeTRRep);
  AddSizeInstTable("POP"  , 1 << eSymbolSize32Bit, 0x15, DecodePUSH_POP);
  AddSizeInstTable("PUSH" , 1 << eSymbolSize32Bit, 0x11, DecodePUSH_POP);
  AddSizeInstTable("RL"   , 1 << eSymbolSize8Bit, 0x00, DecodeRotate);
  AddSizeInstTable("RLC"  , 1 << eSymbolSize8Bit, 0x08, DecodeRotate);
  AddSizeInstTable("RR"   , 1 << eSymbolSize8Bit, 0x04, DecodeRotate);
  AddSizeInstTable("RRC"  , 1 << eSymbolSize8Bit, 0x0c, DecodeRotate);
  AddInstTable(InstTable, "RLDB" , 0xbe00, DecodeRotateDigit);
  AddInstTable(InstTable, "RRDB" , 0xbc00, DecodeRotateDigit);
  AddSizeInstTable("SDA", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x0b, DecodeSD);
  AddSizeInstTable("SDL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x03, DecodeSD);
  AddSizeInstTable("SLA", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x09, DecodeShift);
  AddSizeInstTable("SLL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x01, DecodeShift);
  AddSizeInstTable("SRA", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x89, DecodeShift);
  AddSizeInstTable("SRL", (1 << eSymbolSize8Bit) | (1 << eSymbolSize32Bit), 0x81, DecodeShift);
  AddInstTable(InstTable, "SC", 0x7f00, DecodeSC);
  AddSizeInstTable("TCC", 1 << eSymbolSize8Bit, 0x00, DecodeTCC);

  AddInstTable(InstTable, "PORT", 0, DecodePORT);
  AddInstTable(InstTable, "REG" , 0, CodeREG);
  AddInstTable(InstTable, "DEFBIT" , eSymbolSize16Bit, DecodeDEFBIT);
  AddInstTable(InstTable, "DEFBITB", eSymbolSize8Bit , DecodeDEFBIT);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  Tear down hash table
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  order_array_free(CtlRegs);
  order_array_free(FixedOrders);
  order_array_free(Conditions);

  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     MakeCode_Z8000(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void MakeCode_Z8000(void)
{
  CodeLen = 0; DontPrint = False;
  ImmOpSize = OpSize = eSymbolSizeUnknown;

  /* to be ignored */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_Z8000(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on Z8000
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_Z8000(char *pArg, TempResult *pResult)
{
  Word Reg;
  tSymbolSize Size;

  if (DecodeRegCore(pArg, &Reg, &Size))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = Size;
    pResult->Contents.RegDescr.Reg = Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_Z8000;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_Z8000(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDef_Z8000(void)
{
  return Memo("REG")
      || Memo("PORT")
      || Memo("DEFBIT")
      || Memo("DEFBITB");
}

/*!------------------------------------------------------------------------
 * \fn     PotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
 * \brief  special ^ operator for AMD syntax
 * \param  pErg operator result
 * \param  pRVal input argument
 * ------------------------------------------------------------------------ */

static void PotOp(TempResult *pErg, TempResult *pLVal, TempResult *pRVal)
{
  UNUSED(pLVal);

  /* If in front of a label, takes the address as an 'untyped' value.  This
     will instruct the address decode to use immediate instead of direct
     addressing: */

  if (pRVal->AddrSpaceMask)
    pErg->AddrSpaceMask = 0;

  /* Vice-versa, for a constant, this makes an address of it: */

  else
    pErg->AddrSpaceMask |= (1 << SegCode);

  /* clone remainder as-is */

  pErg->Typ = pRVal->Typ;
  pErg->Contents = pRVal->Contents;
  pErg->Flags |= (pLVal->Flags & eSymbolFlags_Promotable);
  if (pErg->DataSize == eSymbolSizeUnknown) pErg->DataSize = pLVal->DataSize;
}

static const Operator PotMonadicOperator =
{
  "^" ,1 , False, 8, { TempInt | (TempInt << 4), 0, 0, 0 }, PotOp
};

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_Z8000(void *pUser)
 * \brief  prepare to assemble code for this target
 * \param  pUser CPU properties
 * ------------------------------------------------------------------------ */

static Boolean TrueFnc(void)
{
  return True;
}

static void SwitchTo_Z8000(void *pUser)
{
  const TFamilyDescr *pDescr = FindFamilyByName("Z8000");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);

  pCurrCPUProps = (const tCPUProps*)pUser;

  PCSymbol = "$"; HeaderID = pDescr->Id;
  NOPCode = 0x8d07;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode] = 1; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = pCurrCPUProps->SuppSegmented ? 0x7fffff : 0xffff;
  Grans[SegIO] = 1; ListGrans[SegIO] = 1; SegInits[SegIO] = 0;
  SegLimits[SegIO] = 0xffff;
  MemIntType = Segmented() ? UInt23 : UInt16;

  MakeCode = MakeCode_Z8000;
  IsDef = IsDef_Z8000;
  DissectReg = DissectReg_Z8000;
  DissectBit = DissectBit_Z8000;
  InternSymbol = InternSymbol_Z8000;
  SwitchFrom = DeinitFields;
  InitFields();
  SetIsOccupiedFnc = TrueFnc;
  if (AMDSyntax)
    pPotMonadicOperator = &PotMonadicOperator;
  onoff_supmode_add();
}

/*!------------------------------------------------------------------------
 * \fn     codez8000_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "Z8001" , eCoreZ8001   , True  },
  { "Z8002" , eCoreZ8001   , False },
  { "Z8003" , eCoreZ8003   , True  },
  { "Z8004" , eCoreZ8003   , False },
  { NULL    , eCoreNone    , False }
};

void codez8000_init(void)
{
  const tCPUProps *pRun;
  static const tCPUArg Z8000Args[] =
  {
    { "AMDSYNTAX", 0, 1, 0, &AMDSyntax },
    { NULL       , 0, 0, 0, NULL       }
  };

  for (pRun = CPUProps; pRun->pName; pRun++)
    (void)AddCPUUserWithArgs(pRun->pName, SwitchTo_Z8000, (void*)pRun, NULL, Z8000Args);
}
