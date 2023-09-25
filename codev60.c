/* codenv60.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator NEC V60                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include "bpemu.h"
#include "strutil.h"

#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "asmitree.h"
#include "codevars.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "onoff_common.h"

#include "codev60.h"

#define REG_SP 31
#define REG_FP 30
#define REG_AP 29
#define REG_PC 64

/* NOTE: only 3 code flags are available, since machine sub code
   is 5 bits long: */

#define CODE_FLAG_SUPMODE (1 << 15)
#define CODE_FLAG_OP2_IMM (1 << 14)
#define CODE_FLAG_LIM32 (1 << 13)

typedef enum
{
  ModReg = 0,
  ModImm = 1,
  ModMem = 2,
  ModIO  = 3
} tAdrMode;

#define MModReg (1 << ModReg)
#define MModImm (1 << ModImm)
#define MModMem (1 << ModMem)
#define MModIO  (1 << ModIO)
#define MModImm7Bit (1 << 7)
#define MModImmCondition (1 << 6)

/* Count = 0 implies error/invalid, since at least mod byte must be present */

typedef struct
{
  unsigned count;
  Byte m, vals[10];
  tSymbolSize forced_disp_size, data_op_size;
  tSymbolFlags immediate_flags;
  Boolean bit_offset_immediate;
  LongInt bit_offset;
} tAdrVals;

typedef struct
{
  Byte val;
  tSymbolFlags immediate_flags;
} tLenVals;

typedef struct
{
  char name[3];
  Byte code;
} tCondition;

static tSymbolSize OpSize;
static tCondition *Conditions;

/*-------------------------------------------------------------------------*/
/* Register Symbols */

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Byte *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Byte *pResult)
{
  size_t l;

  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REG_SP | REGSYM_FLAG_ALIAS;
    return True;
  }
  else if (!as_strcasecmp(pArg, "FP"))
  {
    *pResult = REG_FP | REGSYM_FLAG_ALIAS;
    return True;
  }
  else if (!as_strcasecmp(pArg, "AP"))
  {
    *pResult = REG_AP | REGSYM_FLAG_ALIAS;
    return True;
  }

  l = strlen(pArg);
  if ((l < 2) || (l > 3) || (as_toupper(*pArg) != 'R'))
    return False;
  *pResult = 0;
  for (pArg++; *pArg; pArg++)
  {
    if (!isdigit(*pArg))
      return False;
    *pResult = (*pResult * 10) + (*pArg - '0');
  }
  return (*pResult < 32);
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_V60(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - V60 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_V60(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  if (InpSize == eSymbolSize32Bit)
  {
    switch (Value)
    {
      case REGSYM_FLAG_ALIAS | REG_SP:
        as_snprintf(pDest, DestSize, "SP");
        break;
      case REGSYM_FLAG_ALIAS | REG_FP:
        as_snprintf(pDest, DestSize, "FP");
        break;
      case REGSYM_FLAG_ALIAS | REG_AP:
        as_snprintf(pDest, DestSize, "AP");
        break;
      default:
        as_snprintf(pDest, DestSize, "R%u", (unsigned)(Value & 31));
    }
  }
  else
    as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
}

/*--------------------------------------------------------------------------*/
/* Address Expression Parser */

/*!------------------------------------------------------------------------
 * \fn     ResetAdrVals(tAdrVals *p_vals)
 * \brief  reset to invalid/empty state
 * \param  p_vals arg to clear
 * ------------------------------------------------------------------------ */

static void ResetAdrVals(tAdrVals *p_vals)
{
  p_vals->count = 0;
  p_vals->m = 0;
  p_vals->forced_disp_size =
  p_vals->data_op_size = eSymbolSizeUnknown;
  p_vals->immediate_flags = eSymbolFlag_None;
  p_vals->bit_offset_immediate = False;
  p_vals->bit_offset = 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
 * \brief  check whether argument is a CPU register or register alias
 * \param  pArg argument to check
 * \param  pResult numeric register value if yes
 * \param  MustBeReg argument is expected to be a register
 * \return RegEvalResult
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pResult))
  {
    *pResult &= ~REGSYM_FLAG_ALIAS;
    return eIsReg;
  }

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
  *pResult = RegDescr.Reg & ~REGSYM_FLAG_ALIAS;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCondition(const char *p_arg, LongWord *p_result)
 * \brief  check whether argument is a condition identifier
 * \param  p_arg source argument
 * \param  p_result result buffer if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean DecodeCondition(const char *p_arg, LongWord *p_result)
{
  int z;

  for (z = 0; Conditions[z].name[0]; z++)
    if (!as_strcasecmp(p_arg, Conditions[z].name))
    {
      *p_result = Conditions[z].code;
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, tAdrVals *pResult, unsigned ModeMask)
 * \brief  decode address expression
 * \param  pArg source argument
 * \param  pResult result buffer
 * \param  MayImm allow immediate?
 * \return True if successfully decoded
 * ------------------------------------------------------------------------ */

static Boolean check_mode_mask(const tStrComp *p_arg, tAdrMode address_mode, unsigned mode_mask)
{
  if ((mode_mask >> address_mode) & 1)
    return True;
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
}

static int BaseQualifier(const char *pArg, int NextNonBlankPos, int SplitPos)
{
  return (pArg[NextNonBlankPos] == ']') ? SplitPos : -1;
}

static void CutSize(tStrComp *pArg, tSymbolSize *pSize)
{
  static const char Suffixes[6][3] = { "B", "8", "H", "16", "W", "32" };
  static const tSymbolSize Sizes[3] = { eSymbolSize8Bit, eSymbolSize16Bit, eSymbolSize32Bit };
  size_t ArgLen = strlen(pArg->str.p_str), ThisLen;
  unsigned z;

  for (z = 0; z < 6; z++)
  {
    ThisLen = strlen(Suffixes[z]);
    if ((ArgLen > ThisLen + 1)
     && (pArg->str.p_str[ArgLen - ThisLen - 1] == '.')
     && !as_strcasecmp(pArg->str.p_str + ArgLen - ThisLen, Suffixes[z]))
    {
      *pSize = Sizes[z / 2];
      StrCompShorten(pArg, ThisLen + 1);
      break;
    }
  }
}

static void AppendToVals(tAdrVals *p_vals, LongWord Value, tSymbolSize Size)
{
  p_vals->vals[p_vals->count++] = Value & 0xff;
  if (Size >= eSymbolSize16Bit)
  {
    p_vals->vals[p_vals->count++] = (Value >> 8) & 0xff;
    if (Size >= eSymbolSize32Bit)
    {
      p_vals->vals[p_vals->count++] = (Value >> 16) & 0xff;
      p_vals->vals[p_vals->count++] = (Value >> 24) & 0xff;
    }
  }
}

static Boolean IsPredecrement(const tStrComp *pArg, Byte *pResult, tRegEvalResult *pRegEvalResult)
{
  tStrComp RegComp;

  if (*pArg->str.p_str != '-')
    return False;
  StrCompRefRight(&RegComp, pArg, 1);
  KillPrefBlanksStrComp(&RegComp);
  *pRegEvalResult = DecodeReg(&RegComp, pResult, False);
  return (*pRegEvalResult != eIsNoReg);
}

static Boolean IsPostincrement(const tStrComp *pArg, Byte *pResult, tRegEvalResult *pRegEvalResult)
{
  String Reg;
  tStrComp RegComp;
  size_t ArgLen = strlen(pArg->str.p_str);

  if (!ArgLen || (pArg->str.p_str[ArgLen - 1] != '+'))
    return False;
  StrCompMkTemp(&RegComp, Reg, sizeof(Reg));
  StrCompCopySub(&RegComp, pArg, 0, ArgLen - 1);
  KillPostBlanksStrComp(&RegComp);
  *pRegEvalResult = DecodeReg(&RegComp, pResult, False);
  return (*pRegEvalResult != eIsNoReg);
}

static Boolean DecodeRegOrPC(const tStrComp *pArg, Byte *pResult)
{
  if (!as_strcasecmp(pArg->str.p_str, "PC"))
  {
    *pResult = REG_PC;
    return True;
  }
  else
    return (DecodeReg(pArg, pResult, True) == eIsReg);
}

static LongInt EvalDispOpt(const tStrComp *pArg, tEvalResult *pEvalResult, Boolean PCRelative)
{
  if (*pArg->str.p_str)
    return EvalStrIntExpressionWithResult(pArg, Int32, pEvalResult) - (PCRelative ? EProgCounter() : 0);
  else
  {
    memset(pEvalResult, 0, sizeof(*pEvalResult));
    pEvalResult->OK = True;
    return 0;
  }
}

static Boolean ProcessAbsolute(const tStrComp *pArg, tAdrVals *pResult, Byte IndexReg, Byte ModByte, unsigned mode_mask)
{
  tEvalResult eval_result;
  LongWord Address;

  Address = EvalStrIntExpressionOffsWithResult(pArg, 1, UInt32, &eval_result);
  pResult->count = 0;
  if (!eval_result.OK)
    return False;
  if (mode_mask & MModIO)
    ChkSpace(SegIO, eval_result.AddrSpaceMask);
  if (!check_mode_mask(pArg, ModMem, mode_mask))
    return False;

  if (IndexReg != REGSYM_FLAG_ALIAS)
  {
    pResult->vals[pResult->count++] = 0xc0 | IndexReg;
    pResult->m = 1;
  }
  else
    pResult->m = 0;
  pResult->vals[pResult->count++] = ModByte;
  AppendToVals(pResult, Address, eSymbolSize32Bit);

  return True;
}

static Boolean DeduceAndCheckDispSize(const tStrComp *pArg, LongInt Value, tSymbolSize *pSize, const tEvalResult *pEvalResult)
{
  if (*pSize == eSymbolSizeUnknown)
  {
    if (RangeCheck(Value, SInt8))
      *pSize = eSymbolSize8Bit;
    else if (RangeCheck(Value, SInt16))
      *pSize = eSymbolSize16Bit;
    else
      *pSize = eSymbolSize32Bit;
  }
  if (mFirstPassUnknownOrQuestionable(pEvalResult->Flags))
    return True;
  switch (*pSize)
  {
    case eSymbolSize8Bit:
      return ChkRangePos(Value, -128, 127, pArg);
    case eSymbolSize16Bit:
      return ChkRangePos(Value, -32768, 32767, pArg);
    case eSymbolSize32Bit:
      return True;
    default:
      WrStrErrorPos(ErrNum_OverRange, pArg);
  }
  return False;
}

static Boolean DecodeAdr(tStrComp *pArg, tAdrVals *pResult, unsigned ModeMask)
{
  int IndexSplitPos, OuterDispSplitPos, ArgLen;
  Byte IndexReg = REGSYM_FLAG_ALIAS, BaseReg = REGSYM_FLAG_ALIAS;
  tEvalResult EvalResult;

  ResetAdrVals(pResult);
  pResult->data_op_size = OpSize;

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    LongWord Value;

    if (!check_mode_mask(pArg, ModImm, ModeMask))
      return False;
    if ((ModeMask & MModImmCondition) && DecodeCondition(pArg->str.p_str + 1, &Value))
      EvalResult.OK = True;
    else switch (OpSize)
    {
      case eSymbolSize8Bit:
        Value = EvalStrIntExpressionOffsWithResult(pArg, 1, (ModeMask & MModImm7Bit) ? UInt7 : Int8, &EvalResult);
        break;
      case eSymbolSize16Bit:
        Value = EvalStrIntExpressionOffsWithResult(pArg, 1, Int16, &EvalResult);
        break;
      case eSymbolSize32Bit:
        Value = EvalStrIntExpressionOffsWithResult(pArg, 1, Int32, &EvalResult);
        break;
      default:
        WrStrErrorPos(ErrNum_InvOpSize, pArg);
        EvalResult.OK = False;
        break;
    }
    if (!EvalResult.OK)
      return EvalResult.OK;

    pResult->m = 0;
    if (Value <= 15)
    {
      pResult->vals[0] = 0xe0 | (Value & 0x0f);
      pResult->count = 1;
    }
    else
    {
      pResult->vals[0] = 0xf4;
      pResult->count = 1;
      AppendToVals(pResult, Value, OpSize);
    }
    pResult->immediate_flags = EvalResult.Flags;
    return EvalResult.OK;
  }

  /* split off index register '(Rx)': */

  IndexSplitPos = FindDispBaseSplitWithQualifier(pArg->str.p_str, &ArgLen, BaseQualifier, "()");
  if (IndexSplitPos > 0)
  {
    String RegStr;
    tStrComp RegComp;

    StrCompMkTemp(&RegComp, RegStr, sizeof(RegStr));
    StrCompCopySub(&RegComp, pArg, IndexSplitPos + 1, ArgLen - IndexSplitPos - 2);
    KillPostBlanksStrComp(&RegComp);
    KillPrefBlanksStrComp(&RegComp);
    switch (DecodeReg(&RegComp, &IndexReg, False))
    {
      case eRegAbort:
        return False;
      case eIsReg:
        StrCompShorten(pArg, ArgLen - IndexSplitPos);
        break;
      default:
        break;
    }
  }

  /* absolute */

  if (*pArg->str.p_str == '/')
    return ProcessAbsolute(pArg, pResult, IndexReg, 0xf3, ModeMask);

  /* indirect modes remain (must end on ]) */

  KillPostBlanksStrComp(pArg);
  OuterDispSplitPos = FindDispBaseSplitWithQualifier(pArg->str.p_str, &ArgLen, NULL, "[]");
  if (OuterDispSplitPos >= 0)
  {
    tStrComp OuterDisp, InnerComp, InnerDisp;
    LongInt OuterDispValue = 0, InnerDispValue = 0;
    tSymbolSize OuterDispSize = eSymbolSizeUnknown,
                InnerDispSize = eSymbolSizeUnknown;
    tEvalResult OuterEvalResult, InnerEvalResult;
    tRegEvalResult RegEvalResult;
    int InnerDispSplitPos;

    /* split off outer displacement */

    StrCompSplitRef(&OuterDisp, &InnerComp, pArg, &pArg->str.p_str[OuterDispSplitPos]);
    StrCompShorten(&InnerComp, 1);
    KillPostBlanksStrComp(&InnerComp);
    KillPrefBlanksStrCompRef(&InnerComp);
    KillPostBlanksStrComp(&OuterDisp);
#if 0
    DumpStrComp("OuterDisp", &OuterDisp);
    DumpStrComp("InnerComp", &InnerComp);
#endif

    /* nested indirect? */

    InnerDispSplitPos = FindDispBaseSplitWithQualifier(InnerComp.str.p_str, &ArgLen, NULL, "[]");
    if (InnerDispSplitPos >= 0)
    {
      StrCompSplitRef(&InnerDisp, &InnerComp, &InnerComp, &InnerComp.str.p_str[InnerDispSplitPos]);
      StrCompShorten(&InnerComp, 1);
      KillPostBlanksStrComp(&InnerComp);
      KillPrefBlanksStrCompRef(&InnerComp);
      KillPostBlanksStrComp(&InnerDisp);
#if 0
      DumpStrComp("InnerDisp", &InnerDisp);
      DumpStrComp("InnerComp(2)", &InnerComp);
#endif

      /* inner comp must be register */

      if (!DecodeRegOrPC(&InnerComp, &BaseReg))
        return False;

      if (*InnerDisp.str.p_str)
        CutSize(&InnerDisp, &InnerDispSize);
      InnerDispValue = EvalDispOpt(&InnerDisp, &InnerEvalResult, BaseReg == REG_PC);
      if (!InnerEvalResult.OK)
        return False;
      (void)InnerDispValue;
    }

    /* postincr/predecr? */

    else if (IsPredecrement(&InnerComp, &pResult->vals[0], &RegEvalResult))
    {
      if ((eRegAbort == RegEvalResult) || !check_mode_mask(pArg, ModMem, ModeMask))
        return False;
      if (*OuterDisp.str.p_str)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return False;
      }
      pResult->vals[0] |= 0xa0;
      pResult->count = 1;
      pResult->m = 1;
      return True;
    }

    else if (IsPostincrement(&InnerComp, &pResult->vals[0], &RegEvalResult))
    {
      if ((eRegAbort == RegEvalResult) || !check_mode_mask(pArg, ModMem, ModeMask))
        return False;
      if (*OuterDisp.str.p_str)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return False;
      }
      pResult->vals[0] |= 0x80;
      pResult->count = 1;
      pResult->m = 1;
      return True;
    }

    /* direct deferred: */

    else if (*InnerComp.str.p_str == '/')
      return ProcessAbsolute(&InnerComp, pResult, IndexReg, 0xfb, ModeMask);

    /* no pre/post, double indir, direct -> must be plain register */

    else
    {
      if (!DecodeRegOrPC(&InnerComp, &BaseReg))
        return False;
    }

    /* for both single & double indirect, outer displacement */

    if (*OuterDisp.str.p_str)
      CutSize(&OuterDisp, &OuterDispSize);
    OuterDispValue = EvalDispOpt(&OuterDisp, &OuterEvalResult, (BaseReg == REG_PC) && (InnerDispSplitPos < 0));
    if (!OuterEvalResult.OK)
      return False;

    /* now, decide about the whole mess: no double []: */

    if (InnerDispSplitPos < 0)
    {
      /* [Rn] */

      if (!*OuterDisp.str.p_str && (BaseReg != REG_PC))
      {
        if (!check_mode_mask(pArg, ModMem, ModeMask))
          return False;
        if (IndexReg != REGSYM_FLAG_ALIAS)
        {
          pResult->vals[pResult->count++] = 0xc0 | IndexReg;
          pResult->vals[pResult->count++] = 0x60 | BaseReg;
          pResult->m = 1;
        }
        else
        {
          pResult->vals[pResult->count++] = 0x60 | BaseReg;
          pResult->m = 0;
        }
        return True;
      }

      pResult->forced_disp_size = InnerDispSize;
      if (!DeduceAndCheckDispSize(&OuterDisp, OuterDispValue, &OuterDispSize, &OuterEvalResult))
        return False;

      if (IndexReg != REGSYM_FLAG_ALIAS)
      {
        pResult->vals[pResult->count++] = 0xc0 | IndexReg;
        pResult->m = 1;
      }
      else
        pResult->m = 0;
      if (REG_PC == BaseReg)
        pResult->vals[pResult->count++] = 0xf0 + OuterDispSize;
      else
        pResult->vals[pResult->count++] = (OuterDispSize << 5) | BaseReg;
      AppendToVals(pResult, OuterDispValue, OuterDispSize);
    }

    /* double [[]] and no outer displacement: */

    else if (!*OuterDisp.str.p_str)
    {
      pResult->forced_disp_size = InnerDispSize;
      if (!DeduceAndCheckDispSize(&InnerDisp, InnerDispValue, &InnerDispSize, &InnerEvalResult))
        return False;

      if (IndexReg != REGSYM_FLAG_ALIAS)
      {
        pResult->vals[pResult->count++] = 0xc0 | IndexReg;
        pResult->m = 1;
      }
      else
        pResult->m = 0;
      if (REG_PC == BaseReg)
        pResult->vals[pResult->count++] = 0xf8 + InnerDispSize;
      else
        pResult->vals[pResult->count++] = 0x80 | (InnerDispSize << 5) | BaseReg;
      AppendToVals(pResult, InnerDispValue, InnerDispSize);
    }

    /* double [[]] and both displacements - no index register allowed! */

    else if (IndexReg != REGSYM_FLAG_ALIAS)
    {
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    }
    else
    {
      /* Both displacements must have same size.  If both are left undefined, use the maximum needed: */

      if ((InnerDispSize == eSymbolSizeUnknown) && (OuterDispSize == eSymbolSizeUnknown))
      {
        DeduceAndCheckDispSize(&InnerDisp, InnerDispValue, &InnerDispSize, &InnerEvalResult);
        DeduceAndCheckDispSize(&OuterDisp, OuterDispValue, &OuterDispSize, &OuterEvalResult);
        if (InnerDispSize > OuterDispSize)
          OuterDispSize = InnerDispSize;
        else if (OuterDispSize > InnerDispSize)
          InnerDispSize = OuterDispSize;
      }

      /* Otherwise, accept one size hint also for the other displacement: */

      else
      {
        if (InnerDispSize == eSymbolSizeUnknown)
          pResult->forced_disp_size = InnerDispSize = OuterDispSize;
        else if (OuterDispSize == eSymbolSizeUnknown)
          pResult->forced_disp_size = OuterDispSize = InnerDispSize;
        if (InnerDispSize != OuterDispSize)
        {
          WrStrErrorPos(ErrNum_ConfOpSizes, pArg);
          return False;
        }
        else
          pResult->forced_disp_size = OuterDispSize;
        if (!DeduceAndCheckDispSize(&InnerDisp, InnerDispValue, &InnerDispSize, &InnerEvalResult)
         || !DeduceAndCheckDispSize(&OuterDisp, OuterDispValue, &OuterDispSize, &OuterEvalResult))
          return False;
      }

      if (REG_PC == BaseReg)
      {
        pResult->vals[pResult->count++] = 0xfc + InnerDispSize;
        pResult->m = 0;
      }
      else
      {
        pResult->vals[pResult->count++] = (InnerDispSize << 5) | BaseReg;
        pResult->m = 1;
      }
      AppendToVals(pResult, InnerDispValue, InnerDispSize);
      AppendToVals(pResult, OuterDispValue, OuterDispSize);
    }
  }
  else
  {
    tSymbolSize DispSize = eSymbolSizeUnknown;
    LongInt DispValue;

    /* bare address: register? */

    switch (DecodeReg(pArg, pResult->vals, False))
    {
      case eIsReg:
        if (!check_mode_mask(pArg, ModReg, ModeMask))
          return False;
        if (((OpSize == eSymbolSize64Bit) || (OpSize == eSymbolSizeFloat64Bit))
         && (pResult->vals[0] == 31))
          WrStrErrorPos(ErrNum_Unpredictable, pArg);
        pResult->vals[0] |= 0x60;
        pResult->m = 1;
        pResult->count = 1;
        return True;
      case eRegAbort:
        return False;
      default:
        break;
    }

    /* treat bare address as PC-relative */

    CutSize(pArg, &DispSize);
    DispValue = EvalDispOpt(pArg, &EvalResult, True);
    if (!EvalResult.OK)
      return False;
    pResult->forced_disp_size = DispSize;
    if (!DeduceAndCheckDispSize(pArg, DispValue, &DispSize, &EvalResult))
      return False;
    if (IndexReg != REGSYM_FLAG_ALIAS)
    {
      pResult->vals[pResult->count++] = 0xc0 | IndexReg;
      pResult->m = 1;
    }
    else
      pResult->m = 0;
    pResult->vals[pResult->count++] = 0xf0 + DispSize;
    AppendToVals(pResult, DispValue, DispSize);
  }

  return (pResult->count > 0) && check_mode_mask(pArg, ModMem, ModeMask);
}

/*!------------------------------------------------------------------------
 * \fn     IsReg(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is register-direct ( Rn )
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsReg(const tAdrVals *p_vals)
{
  return ((p_vals->count == 1)
       && (p_vals->m == 1)
       && ((p_vals->vals[0] & 0xe0) == 0x60));
}

/*!------------------------------------------------------------------------
 * \fn     IsRegIndirect(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is register indirect ( [Rn] )
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsRegIndirect(const tAdrVals *p_vals)
{
  return ((p_vals->count == 1)
       && (p_vals->m == 0)
       && ((p_vals->vals[0] & 0xe0) == 0x60));
} 

/*!------------------------------------------------------------------------
 * \fn     IsPreDecrement(const tAdrVals *p_vals, Byte *p_reg)
 * \brief  check whether encoded address expression is pre-decrement ( [-Rn] )
 * \param  p_vals encoded address expression
 * \param  p_reg return buffer for used register
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsPreDecrement(const tAdrVals *p_vals, Byte *p_reg)
{
  if ((p_vals->count == 1)
   && (p_vals->m == 1)
   && ((p_vals->vals[0] & 0xe0) == 0xa0))
  {
    if (p_reg) *p_reg = p_vals->vals[0] & 0x1f;
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     IsPostIncrement(const tAdrVals *p_vals, Byte *p_reg)
 * \brief  check whether encoded address expression is post-increment ( [Rn+] )
 * \param  p_vals encoded address expression
 * \param  p_reg return buffer for used register
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsPostIncrement(const tAdrVals *p_vals, Byte *p_reg)
{
  if ((p_vals->count == 1)
   && (p_vals->m == 1)
   && ((p_vals->vals[0] & 0xe0) == 0x80))
  {
    if (p_reg) *p_reg = p_vals->vals[0] & 0x1f;
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     IsAbsolute(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is absolute ( /abs )
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsAbsolute(const tAdrVals *p_vals)
{
  return ((p_vals->count == 5)
       && (p_vals->m == 0)
       && (p_vals->vals[0] == 0xf3));
}

/*!------------------------------------------------------------------------
 * \fn     IsAbsoluteIndirect(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is absolute indirect ( [/abs] )
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsAbsoluteIndirect(const tAdrVals *p_vals)
{
  return ((p_vals->count == 5)
       && (p_vals->m == 0)
       && (p_vals->vals[0] == 0xfb));
}

/*!------------------------------------------------------------------------
 * \fn     IsDisplacement(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is displacement
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDisplacement(const tAdrVals *p_vals)
{
  Byte base_mode = p_vals->vals[0],
       base_mode_mode = base_mode & 0xe0;

  return (p_vals->m == 0)
      && (p_vals->count > 1)
      && ((base_mode_mode == 0x000) /* disp8[Rn]  */
       || (base_mode      == 0x0f0) /* disp8[PC]  */
       || (base_mode_mode == 0x020) /* disp16[Rn] */
       || (base_mode      == 0x0f1) /* disp16[PC] */
       || (base_mode_mode == 0x040) /* disp32[Rn] */
       || (base_mode      == 0x0f2)); /* disp32[PC] */
}

/*!------------------------------------------------------------------------
 * \fn     IsIndirectDisplacement(const tAdrVals *p_vals)
 * \brief  check whether encoded address expression is indirect displacement
 * \param  p_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsIndirectDisplacement(const tAdrVals *p_vals)
{
  Byte base_mode = p_vals->vals[0],
       base_mode_mode = base_mode & 0xe0;

  return (p_vals->m == 0)
      && (p_vals->count > 1)
      && ((base_mode_mode == 0x080) /* [disp8[Rn]]  */
       || (base_mode      == 0x0f8) /* [disp8[PC]]  */
       || (base_mode_mode == 0x0a0) /* [disp16[Rn]] */
       || (base_mode      == 0x0f9) /* [disp16[PC]] */
       || (base_mode_mode == 0x0c0) /* [disp32[Rn]] */
       || (base_mode      == 0x0fa)); /* [disp32[PC]] */
}

/*!------------------------------------------------------------------------
 * \fn     IsImmediate(const tAdrVals *p_adr_vals, LongWord *p_imm_value)
 * \brief  check whether encoded address expression is immediate
 * \param  p_vals encoded address expression
 * \param  p_imm_value return buffer for immediate value
 * \return True if immediate
 * ------------------------------------------------------------------------ */

static Boolean IsImmediate(const tAdrVals *p_adr_vals, LongWord *p_imm_value)
{
  if ((p_adr_vals->vals[0] & 0xf0) == 0xe0)
  {
    *p_imm_value = p_adr_vals->vals[0] & 0x0f;
    return True;
  }
  else if ((p_adr_vals->vals[0] == 0xf4) && (p_adr_vals->count > 1))
  {
    unsigned z;

    *p_imm_value = 0;
    for (z = p_adr_vals->count - 1; z > 0; z--)
      *p_imm_value = (*p_imm_value << 8) | p_adr_vals->vals[z];
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     IsSPAutoIncDec(const tAdrVals *p_adr_vals)
 * \brief  check whether encoded address expression is auto-increment/decrement with SP
 * \param  p_adr_vals encoded address expression
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsSPAutoIncDec(const tAdrVals *p_adr_vals)
{
  return ((p_adr_vals->count == 1)
       && p_adr_vals->m
       && ((p_adr_vals->vals[0] == (0x80 | REG_SP))
        || (p_adr_vals->vals[0] == (0xa0 | REG_SP))));
}

/*!------------------------------------------------------------------------
 * \fn     IsIndexed(const tAdrVals *p_adr_vals, Byte *p_reg)
 * \brief  check whether encoded address expression uses an index register
 * \param  p_adr_vals encoded address expression
 * \param  p_reg return buffer for index register if yes
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsIndexed(const tAdrVals *p_adr_vals, Byte *p_reg)
{
  if ((p_adr_vals->count >= 1)
   && p_adr_vals->m
   && ((p_adr_vals->vals[0] & 0xe0) == 0xc0))
  {
    if (p_reg) *p_reg = p_adr_vals->vals[0] & 0x1f;
    return True;
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitAdr(tStrComp *p_arg, tAdrVals *p_result, unsigned ModeMask)
 * \brief  parse bit address expression
 * \param  p_arg source argument
 * \param  p_result result buffer
 * \return True if success
 * ------------------------------------------------------------------------ */

static void insert_index(tAdrVals *p_result, Byte index_reg)
{
  memmove(p_result->vals + 1, p_result->vals, p_result->count);
  p_result->vals[0] = 0xc0 | index_reg;
  p_result->count++;
  p_result->m = 1;
}

static void extend_adrvals_size(tAdrVals *p_result, tSymbolSize *p_curr_size, tSymbolSize target_size, Byte mod_incr)
{
  while (*p_curr_size < target_size)
  {
    Byte ext = (p_result->vals[p_result->count - 1] & 0x80) ? 0xff : 0x00;
    p_result->vals[0] += mod_incr;
    *p_curr_size = (tSymbolSize)(*p_curr_size + 1);
    switch (*p_curr_size)
    {
      case eSymbolSize32Bit:
        p_result->vals[p_result->count++] = ext;
        /* FALL-THRU */
      case eSymbolSize16Bit:
        p_result->vals[p_result->count++] = ext;
        break;
      default:
        break;
    }
  }
}

static Boolean DecodeBitAdr(tStrComp *p_arg, tAdrVals *p_result, unsigned ModeMask)
{
  char *p_sep;
  tStrComp offset_arg, base_arg;
  Byte offs_reg;
  tEvalResult eval_result;

  /* find '@' that separates bit offset & byte base address, which MUST be present: */

  p_sep = QuotPos(p_arg->str.p_str, '@');
  if (!p_sep)
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  StrCompSplitRef(&offset_arg, &base_arg, p_arg, p_sep);
  KillPostBlanksStrComp(&offset_arg);
  KillPrefBlanksStrCompRef(&base_arg);

  /* parse the base address */

  (void)ModeMask;
  if (!DecodeAdr(&base_arg, p_result, MModMem))
    return False;

  /* no offset: the encoding remains the same, just check for valid addressing modes: */

  if (!offset_arg.str.p_str[0])
  {
    eval_result.OK =
         IsRegIndirect(p_result)
      || IsPostIncrement(p_result, NULL)
      || IsPreDecrement(p_result, NULL)
      || IsIndirectDisplacement(p_result)
      || IsAbsolute(p_result)
      || IsAbsoluteIndirect(p_result);
    if (!eval_result.OK)
      WrStrErrorPos(ErrNum_InvAddrMode, &base_arg);
  }

  else switch (DecodeReg(&offset_arg, &offs_reg, False))
  {
    /* offset is register: if this is a valid addressing mode, insert register
       as index register: */

    case eIsReg:
      eval_result.OK = True;
      if (IsRegIndirect(p_result)
       || IsDisplacement(p_result)
       || IsIndirectDisplacement(p_result)
       || IsAbsolute(p_result)
       || IsAbsoluteIndirect(p_result))
        insert_index(p_result, offs_reg);
      else
        eval_result.OK = False;
      if (!eval_result.OK)
        WrStrErrorPos(ErrNum_InvAddrMode, &base_arg);
      break;

    /* offset is immediate value: */

    case eIsNoReg:
    {
      tSymbolSize forced_offset_size = eSymbolSizeUnknown, offset_size;

      CutSize(&offset_arg, &forced_offset_size);
      p_result->bit_offset = EvalStrIntExpressionWithResult(&offset_arg, SInt32, &eval_result);
      if (!eval_result.OK)
        break;
      p_result->immediate_flags = eval_result.Flags;
      offset_size = forced_offset_size;
      if (!(eval_result.OK = DeduceAndCheckDispSize(&offset_arg, p_result->bit_offset, &offset_size, &eval_result)))
        break;

      /* base addressing mode has no displacement so far: append as first displacement */

      if (IsRegIndirect(p_result))
      {
        p_result->vals[0] = (p_result->vals[0] & 0x1f) | (offset_size << 5);
        AppendToVals(p_result, p_result->bit_offset, offset_size);
      }

      /* single (indirect) displacement becomes double displacement */

      else if (IsIndirectDisplacement(p_result))
      {
        Boolean base_pc = !!(p_result->vals[0] & 0x10);
        /* extract operand size of base displacement */
        tSymbolSize base_disp_size = (tSymbolSize)((p_result->vals[0] >> (base_pc ? 0 : 5)) & 3);

        /* Similar to disp1 & disp2, base & offset must have same length, so if
           lengths are unequal, extend one of them, or complain if we cannot make
           things fit: */

        /* (1) No explicit length given for either: extend the other one as needed
           if actual sizes differ: */

        if ((p_result->forced_disp_size == eSymbolSizeUnknown)
         && (forced_offset_size == eSymbolSizeUnknown))
        {
          if (offset_size < base_disp_size)
            offset_size = base_disp_size;
          else if (base_disp_size < offset_size)
            extend_adrvals_size(p_result, &base_disp_size, offset_size, p_result->vals[0] += base_pc ? 0x01 : 0x20);
        }

        /* (2a) Only base displacement size is fixed: see if offset size can be extended */

        else if (forced_offset_size == eSymbolSizeUnknown)
        {
          if (offset_size < base_disp_size)
            offset_size = base_disp_size;
          else
          {
            WrStrErrorPos(ErrNum_OverRange, &offset_arg);
            return False;
          }
        }

        /* (2b) Vice versa, if only offset size is fixed: */

        else if (p_result->forced_disp_size == eSymbolSizeUnknown)
        {
          if (base_disp_size < offset_size)
            extend_adrvals_size(p_result, &base_disp_size, offset_size, p_result->vals[0] += base_pc ? 0x01 : 0x20);
          else
          {
            WrStrErrorPos(ErrNum_OverRange, &base_arg);
            return False;
          }
        }

        /* (3) if both sizes are fixed, they must be equal: */

        else if (p_result->forced_disp_size != forced_offset_size)
        {
          WrStrErrorPos(ErrNum_ConfOpSizes, p_arg);
          return False;
        }

        /* All fine: Change addressing mode to double displacement,
           and add bit offset as second displacement */

        if (base_pc)
          p_result->vals[0] |= 0x04;
        else
        {
          p_result->m = 1;
          p_result->vals[0] &= ~0x80;
        }
        AppendToVals(p_result, p_result->bit_offset, offset_size);
      }

      /* everything else not allowed */

      else
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &base_arg);
        eval_result.OK = False;
      }

      p_result->bit_offset_immediate = True;
      break;
    }
    default: /* eRegAbort */
      eval_result.OK = False;
  }

  return eval_result.OK;
}

/*--------------------------------------------------------------------------*/
/* Utility Functions */

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

/*!------------------------------------------------------------------------
 * \fn     ChkOpSize(int Op8, int Op16, int Op32, int Op64)
 * \brief  check for valid (single) (integer) operand size
 * \param  Op8 machine code if size is 8 bits
 * \param  Op16 machine code if size is 16 bits
 * \param  Op32 machine code if size is 32 bits
 * \param  Op64 machine code if size is 64 bits
 * \return True if size is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkOpSize(int Op8, int Op16, int Op32, int Op64)
{
  if (AttrPartOpSize[1] != eSymbolSizeUnknown)
  {
    WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
    return False;
  }
  if (OpSize == eSymbolSizeUnknown)
  {
    if (AttrPartOpSize[0] != eSymbolSizeUnknown)
      OpSize = AttrPartOpSize[0];
    else if ((Op8 >= 0) && Hi(Op8))
      OpSize = eSymbolSize8Bit;
    else if ((Op16 >= 0) && Hi(Op16))
      OpSize = eSymbolSize16Bit;
    else if ((Op32 >= 0) && Hi(Op32))
      OpSize = eSymbolSize32Bit;
    else if ((Op64 >= 0) && Hi(Op64))
      OpSize = eSymbolSize64Bit;
  }
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      if (Op8 < 0)
        goto Bad;
      BAsmCode[CodeLen] = Op8;
      return True;
    case eSymbolSize16Bit:
      if (Op16 < 0)
        goto Bad;
      BAsmCode[CodeLen] = Op16;
      return True;
    case eSymbolSize32Bit:
      if (Op32 < 0)
        goto Bad;
      BAsmCode[CodeLen] = Op32;
      return True;
    case eSymbolSize64Bit:
      if (Op64 < 0)
        goto Bad;
      BAsmCode[CodeLen] = Op64;
      return True;
    default:
    Bad:
      WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkFOpSize(int op_short, int op_long)
 * \brief  check for valid (real) operand size
 * \param  op_short machine code if size short real
 * \param  op_long machine code if size is long real
 * \return True if size is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkFOpSize(int op_short, int op_long)
{
  if (AttrPartOpSize[1] != eSymbolSizeUnknown)
  {
    WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
    return False;
  }
  if (OpSize == eSymbolSizeUnknown)
  {
    if (AttrPartOpSize[0] != eSymbolSizeUnknown)
      OpSize = AttrPartOpSize[0];
    else if ((op_short >= 0) && Hi(op_short))
      OpSize = eSymbolSizeFloat32Bit;
    else if ((op_long >= 0) && Hi(op_long))
      OpSize = eSymbolSizeFloat64Bit;
  }
  switch (OpSize)
  {
    case eSymbolSizeFloat32Bit:
      if (op_short < 0)
        goto Bad;
      BAsmCode[CodeLen] = op_short;
      return True;
    case eSymbolSizeFloat64Bit:
      if (op_long < 0)
        goto Bad;
      BAsmCode[CodeLen] = op_long;
      return True;
    default:
    Bad:
      WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkPtrOpSize(int op_ptr)
 * \brief  check for valid (pointer) operand size
 * \param  op_ptr machine code if size is pointer
 * \return True if size is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkPtrOpSize(int op_ptr)
{
  if (AttrPartOpSize[1] != eSymbolSizeUnknown)
  {
    WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
    return False;
  }
  if (OpSize == eSymbolSizeUnknown)
  {
    if (AttrPartOpSize[0] != eSymbolSizeUnknown)
      OpSize = AttrPartOpSize[0];
    else if ((op_ptr >= 0) && Hi(op_ptr))
      OpSize = eSymbolSize24Bit;
  }
  switch (OpSize)
  {
    case eSymbolSize24Bit:
      if (op_ptr < 0)
        goto Bad;
      OpSize = eSymbolSize32Bit;
      BAsmCode[CodeLen] = Lo(op_ptr);
      return True;
    default:
    Bad:
      WrStrErrorPos(ErrNum_InvOpSize, &OpPart);
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     chk_sup_mode(Word code)
 * \brief  check whether privileged instructions are enabled
 * \param  code machine code with sup mode flag
 * \return True if enabled or not required
 * ------------------------------------------------------------------------ */

static Boolean chk_sup_mode(Word code)
{
  if ((code & CODE_FLAG_SUPMODE) && !SupAllowed)
  {
    WrStrErrorPos(ErrNum_PrivOrder, &OpPart);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     AppendAdrVals(const tAdrVals *p_vals)
 * \brief  append addressing mode extension values to instruction
 * \param  p_vals holds encoded addressing expression
 * ------------------------------------------------------------------------ */

static void AppendAdrVals(const tAdrVals *p_vals)
{
  memcpy(&BAsmCode[CodeLen], p_vals->vals, p_vals->count);
  CodeLen += p_vals->count;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLength(tStrComp *p_arg, tLenVals *p_len_vals)
 * \brief  decode length argument of bit string insns
 * \param  p_arg source argument
 * \param  p_len_vals encoded length argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeLength(tStrComp *p_arg, tLenVals *p_len_vals)
{
  tAdrVals adr_vals;
  LongWord length;

  if (!DecodeAdr(p_arg, &adr_vals, MModReg | MModImm | MModImm7Bit))
    return False;
  p_len_vals->immediate_flags = adr_vals.immediate_flags;
  if (IsReg(&adr_vals))
    p_len_vals->val = 0x80 | (adr_vals.vals[0] & 0x1f);
  else if (IsImmediate(&adr_vals, &length))
    p_len_vals->val = length & 127;
  else
  {
    WrStrErrorPos(ErrNum_InvAddrMode, p_arg);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     IsImmediateLength(const tLenVals *p_len_vals, LongWord *p_imm_val)
 * \brief  check whether encoded bit field length is immediate value
 * \param  p_len_vals encoded length field from instruction
 * \param  p_imm_val return buffer if immediate
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsImmediateLength(const tLenVals *p_len_vals, LongWord *p_imm_val)
{
  if (p_len_vals->val & 0x80)
    return False;
  else
  {
    *p_imm_val = p_len_vals->val & 0x7f;
    return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     imm_mask_cond(Word code)
 * \brief  check whether immediate mode is allowed and return accordingly
 * \param  code machine code holding CODE_FLAG_OP2_IMM in MSB or not
 * \return immediate mask if allowed or 0
 * ------------------------------------------------------------------------ */

static unsigned imm_mask_cond(Word code)
{
  return (code & CODE_FLAG_OP2_IMM) ? MModImm : 0;
}

/*!------------------------------------------------------------------------
 * \fn     check_addrmodes_unpredictable(const tAdrVals *p_first_vals, const tAdrVals *p_second_vals)
 * \brief  check whether the combination of two addressing modes may yield unpredictable results
 * \param  p_first_vals encoded first operand
 * \param  p_second_vals encoded second operand
 * \return True if unpredictable result may occur
 * ------------------------------------------------------------------------ */

static Boolean check_addrmodes_unpredictable(const tAdrVals *p_first_vals, const tAdrVals *p_second_vals)
{
  Byte first_reg = 0, second_reg = 0;

  /* For all unpredictable combinations, first operand must be auto-increment: */

  if (!IsPreDecrement(p_first_vals, &first_reg)
   && !IsPostIncrement(p_first_vals, &first_reg))
    return False;

  /* If second operand is auto-increment with same register,
     result is unpredictable if both operand sizes differ: */

  if ((IsPreDecrement(p_second_vals, &second_reg) || IsPostIncrement(p_second_vals, &second_reg))
   && (second_reg == first_reg)
   && (p_first_vals->data_op_size != p_second_vals->data_op_size))
  {
    WrError(ErrNum_Unpredictable);
    return True;
  }

  /* If second operand uses same register as index register,
     result is unpredictable: */

  else if (IsIndexed(p_second_vals, &second_reg) && (second_reg == first_reg))
  {
    WrError(ErrNum_Unpredictable);
    return True;
  }

  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     EncodeFormat1(Byte Reg, const tAdrVals *pSecond)
 * \brief  encode instruction in format I
 * \param  Reg Register# + direction bit
 * \param  pSecond second operand
 * ------------------------------------------------------------------------ */

static void EncodeFormat1(Byte Reg, const tAdrVals *pSecond)
{
  CodeLen++;
  BAsmCode[CodeLen++] = Reg | (pSecond->m << 6);
  AppendAdrVals(pSecond);
}

/*!------------------------------------------------------------------------
 * \fn     EncodeFormat2(Byte SubOp, const tAdrVals *pFirst, const tAdrVals *pSecond)
 * \brief  encode instruction in format II
 * \param  SubOp extension byte
 * \param  pFirst first operand
 * \param  pSecond second operand
 * ------------------------------------------------------------------------ */

static void EncodeFormat2(Byte SubOp, const tAdrVals *pFirst, const tAdrVals *pSecond)
{
  CodeLen++;
  BAsmCode[CodeLen++] = SubOp | 0x80 | (pSecond->m << 5) | (pFirst->m << 6);
  AppendAdrVals(pFirst);
  AppendAdrVals(pSecond);
  check_addrmodes_unpredictable(pFirst, pSecond);
}

/*!------------------------------------------------------------------------
 * \fn     EncodeFormat1Or2(Byte SubOp, const tAdrVals *pFirst, const tAdrVals *pSecond)
 * \brief  encode instruction in format I if possible or format II
 * \param  SubOp extension byte for format II
 * \param  pFirst first operand
 * \param  pSecond second operand
 * ------------------------------------------------------------------------ */

static void EncodeFormat1Or2(Byte SubOp, const tAdrVals *pFirst, const tAdrVals *pSecond)
{
  if (IsReg(pFirst))
    EncodeFormat1(pFirst->vals[0] & 0x1f, pSecond);
  else if (IsReg(pSecond))
    EncodeFormat1((pSecond->vals[0] & 0x1f) | 0x20, pFirst);
  else
    EncodeFormat2(SubOp, pFirst, pSecond);
}

/*!------------------------------------------------------------------------
 * \fn     EncodeFormat3(const tAdrVals *p_adr_vals)
 * \brief  encode instruction in format III
 * \param  p_adr_vals operand
 * ------------------------------------------------------------------------ */

static void EncodeFormat3(const tAdrVals *p_adr_vals)
{
  BAsmCode[CodeLen] |= p_adr_vals->m;
  CodeLen++;
  AppendAdrVals(p_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     chk_imm_bitfield_range(const tStrComp *p_bitfield_src_arg, const tAdrVals *p_bitfield_adr_vals, const tLenVals *p_len_vals)
 * \brief  check whether bit field's length plus offset do not exceed allowed range
 * \param  p_bitfield_src_arg source argument of bit field start+offset
 * \param  p_bitfield_adr_vals encoded value of bit field start+offset
 * \param  p_len_vals encoded value of bit field length
 * ------------------------------------------------------------------------ */

static void chk_imm_bitfield_range(const tStrComp *p_bitfield_src_arg, const tAdrVals *p_bitfield_adr_vals, const tLenVals *p_len_vals)
{
  LongWord imm_length;

  /* optional check for offset+length <= 32, if possible: */

  if (p_bitfield_adr_vals->bit_offset_immediate
   && !mFirstPassUnknownOrQuestionable(p_bitfield_adr_vals->immediate_flags)
   && IsImmediateLength(p_len_vals, &imm_length)
   && !mFirstPassUnknownOrQuestionable(p_len_vals->immediate_flags)
   && (p_bitfield_adr_vals->bit_offset + imm_length > 32))
  {
    char Msg[64];

    as_snprintf(Msg, sizeof(Msg), "%u + %u > 32",
                (unsigned)p_bitfield_adr_vals->bit_offset, 
                (unsigned)imm_length);
    WrXErrorPos(ErrNum_ArgOutOfRange, Msg, &p_bitfield_src_arg->Pos);
  }
}

/*!------------------------------------------------------------------------
 * \fn     chk_imm_value_range(const tStrComp *p_src_arg, const tAdrVals *p_adr_vals, const char *p_name, unsigned max_value)
 * \brief  check whether immediate value is in range
 * \param  p_arg source argument of value
 * \param  p_adr_vals encoded value of value
 * ------------------------------------------------------------------------ */

static void chk_imm_value_range(const tStrComp *p_src_arg, const tAdrVals *p_adr_vals, const char *p_name, unsigned max_value)
{
  LongWord imm_value;

  if (IsImmediate(p_adr_vals, &imm_value)
   && !mFirstPassUnknownOrQuestionable(p_adr_vals->immediate_flags)
   && (imm_value > max_value))
  {
    char Msg[64];

    as_snprintf(Msg, sizeof(Msg), "%s %u > %u",
                p_name, (unsigned)imm_value, max_value);
    WrXErrorPos(ErrNum_ArgOutOfRange, Msg, &p_src_arg->Pos);
  }
}

/*!------------------------------------------------------------------------
 * \fn     chk_imm_level_range(const tStrComp *p_level_src_arg, const tAdrVals *p_level_adr_vals)
 * \brief  check whether (privilege) level is in range (0..3)
 * \param  p_level_src_arg source argument of level
 * \param  p_level_adr_vals encoded value of level
 * ------------------------------------------------------------------------ */

static void chk_imm_level_range(const tStrComp *p_level_src_arg, const tAdrVals *p_level_adr_vals)
{
  chk_imm_value_range(p_level_src_arg, p_level_adr_vals, "lvl", 3);
}

/*--------------------------------------------------------------------------*/
/* Instruction Handlers */

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  handle instructions with no argument
 * \param  Code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0)
   && chk_sup_mode(Code)
   && ChkNoAttrPart())
    BAsmCode[CodeLen++] = Lo(Code);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeArithF(Word code)
 * \brief  handle FPU arithmetic instructions
 * \param  code machine opcode for short op size
 * ------------------------------------------------------------------------ */

static void DecodeArithF(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkFOpSize(0x100 | Lo(code), Lo(code) | 0x02)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem | MModReg)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
  {
    EncodeFormat2(Hi(code), &src_adr_vals, &dest_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSCLF(Word code)
 * \brief  handle SCLF Instruction
 * \param  code machine opcode for short op size
 * ------------------------------------------------------------------------ */

static void DecodeSCLF(Word code)
{
  if (ChkArgCnt(2, 2)
   && ChkFOpSize(0x100 | Lo(code), Lo(code) | 0x02))
  {
    tAdrVals count_adr_vals;
    tSymbolSize save_op_size;

    save_op_size = OpSize;
    OpSize = eSymbolSize16Bit;
    if (DecodeAdr(&ArgStr[1], &count_adr_vals, MModMem | MModReg | MModImm))
    {
      tAdrVals  dest_adr_vals;

      OpSize = save_op_size;

      if (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
        EncodeFormat2(Hi(code), &count_adr_vals, &dest_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeArith(Word code)
 * \brief  handle arithmetic instructions
 * \param  code machine code for 8 bit in LSB, bit 8 -> dest may be immediate
 * ------------------------------------------------------------------------ */

static void DecodeArith(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | (Lo(code) + 4), -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem | MModReg | MModImm)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg | imm_mask_cond(code)))
    EncodeFormat1Or2(0x00, &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeArithX(Word code)
 * \brief  handle arithmetic instructions with double dest width
 * \param  code machine code for 8 bit in LSB
 * ------------------------------------------------------------------------ */

static void DecodeArithX(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(-1, -1, 0x100 | Lo(code), -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem | MModReg | MModImm)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
    EncodeFormat1Or2(0x00, &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeXCH(Word code)
 * \brief  handle XCH instruction
 * \param  code machine code for 8 bit operand size
 * ------------------------------------------------------------------------ */

static void DecodeXCH(Word code)
{
  tAdrVals dest1_adr_vals, dest2_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | (Lo(code) + 4), -1)
   && DecodeAdr(&ArgStr[1], &dest1_adr_vals, MModMem | MModReg)
   && DecodeAdr(&ArgStr[2], &dest2_adr_vals, MModMem | MModReg))
  {
    if (IsReg(&dest1_adr_vals))
      EncodeFormat1(dest1_adr_vals.vals[0] & 0x1f, &dest2_adr_vals);
    else if (IsReg(&dest2_adr_vals))
      EncodeFormat1(dest2_adr_vals.vals[0] & 0x1f, &dest1_adr_vals);
    else
      WrError(ErrNum_InvArgPair);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSETF(Word code)
 * \brief  handle SETF instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeSETF(Word code)
{
  tAdrVals cond_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(0x100 | Lo(code), -1, -1, -1)
   && DecodeAdr(&ArgStr[1], &cond_adr_vals, MModMem | MModReg | MModImm | MModImmCondition)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
  {
    EncodeFormat1Or2(0, &cond_adr_vals, &dest_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeShift(Word code)
 * \brief  handle shift instructions
 * \param  code machine code for 8 bit in LSB
 * ------------------------------------------------------------------------ */

static Boolean DecodeShiftCnt(tStrComp *p_arg, tAdrVals *p_adr_vals)
{
  tSymbolSize save_size;
  Boolean ret;

  save_size = OpSize;
  OpSize = eSymbolSize8Bit;
  ret = DecodeAdr(p_arg, p_adr_vals, MModMem | MModReg | MModImm);
  OpSize = save_size;
  return ret;
}

static void DecodeShift(Word code)
{
  tAdrVals shift_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | (Lo(code) + 4), -1)
   && DecodeShiftCnt(&ArgStr[1], &shift_adr_vals)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
    EncodeFormat1Or2(0x00, &shift_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeArith_7c(Word code)
 * \brief  handle arithmetic instructions
 * \param  code machine code in LSB, sub-code in MSB
 * ------------------------------------------------------------------------ */

static void DecodeArith_7c(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(3, 3)
   && ChkOpSize(0x100 | Lo(code), -1, -1, -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem | MModReg | MModImm)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
  {
    Boolean ok;
    Byte mask = EvalStrIntExpressionOffs(&ArgStr[3], ArgStr[3].str.p_str[0] == '#', Int8, &ok);

    if (ok)
    {
      EncodeFormat2(Hi(code), &src_adr_vals, &dest_adr_vals);
      BAsmCode[CodeLen++] = mask;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitString_7b(Word code)
 * \brief  handle bit string instructions
 * \param  code machine code in LSB, sub-code in MSB
 * ------------------------------------------------------------------------ */

static void DecodeBitString_7b(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;
  tLenVals length;

  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize8Bit;

    if (DecodeBitAdr(&ArgStr[1], &src_adr_vals, MModMem)
     && DecodeLength(&ArgStr[2], &length)
     && DecodeBitAdr(&ArgStr[3], &dest_adr_vals, MModMem))
    {
      BAsmCode[CodeLen++] = Lo(code);
      BAsmCode[CodeLen++] = Hi(code) | 0x80 | (dest_adr_vals.m << 5) | (src_adr_vals.m << 6);
      AppendAdrVals(&src_adr_vals);
      BAsmCode[CodeLen++] = length.val;
      AppendAdrVals(&dest_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitField_7b(Word code)
 * \brief  handle bit field instructions
 * \param  code machine code in LSB, sub-code in MSB
 * ------------------------------------------------------------------------ */

static void DecodeBitField_7b(Word code)
{
  tAdrVals bsrc_adr_vals, src_adr_vals;
  tLenVals length;

  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize32Bit;

    if (DecodeBitAdr(&ArgStr[1], &bsrc_adr_vals, MModMem)
     && DecodeLength(&ArgStr[2], &length)
     && DecodeAdr(&ArgStr[3], &src_adr_vals, MModReg | MModMem | imm_mask_cond(code)))
    {
      /* optional check for offset+length <= 32, if possible: */

      if (code & CODE_FLAG_LIM32)
        chk_imm_bitfield_range(&ArgStr[1], &bsrc_adr_vals, &length);

      BAsmCode[CodeLen++] = Lo(code);
      BAsmCode[CodeLen++] = (Hi(code) & 0x1f) | 0x80 | (src_adr_vals.m << 5) | (bsrc_adr_vals.m << 6);
      AppendAdrVals(&bsrc_adr_vals);
      BAsmCode[CodeLen++] = length.val;
      AppendAdrVals(&src_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitField_7c(Word code)
 * \brief  handle bit field instructions
 * \param  code machine code in LSB, sub-code in MSB
 * ------------------------------------------------------------------------ */

static void DecodeBitField_7c(Word code)
{
  tAdrVals src_adr_vals, bdst_adr_vals;
  tLenVals length;

  if (ChkArgCnt(3, 3)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize32Bit;

    if (DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg | MModMem | MModImm)
     && DecodeBitAdr(&ArgStr[2], &bdst_adr_vals, MModMem)
     && DecodeLength(&ArgStr[3], &length))
    {
      /* optional check for offset+length <= 32, if possible: */

      if (code & CODE_FLAG_LIM32)
        chk_imm_bitfield_range(&ArgStr[2], &bdst_adr_vals, &length);

      BAsmCode[CodeLen++] = Lo(code);
      BAsmCode[CodeLen++] = (Hi(code) & 0x1f) | 0x80 | (bdst_adr_vals.m << 5) | (src_adr_vals.m << 6);
      AppendAdrVals(&src_adr_vals);
      AppendAdrVals(&bdst_adr_vals);
      BAsmCode[CodeLen++] = length.val;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeString_7a(Word code)
 * \brief  Handle Format VIIa String Instructons
 * \param  code machine code (LSB) and subcode (MSB)
 * ------------------------------------------------------------------------ */

static void DecodeString_7a(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;
  tLenVals src_length, dest_length;

  if (ChkArgCnt(4, 4)
   && ChkOpSize(0x100 | (Lo(code)), Lo(code) + 2, -1, -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem)
   && DecodeLength(&ArgStr[2], &src_length)
   && DecodeAdr(&ArgStr[3], &dest_adr_vals, MModMem)
   && DecodeLength(&ArgStr[4], &dest_length))
  {
    CodeLen++;
    BAsmCode[CodeLen++] = (Hi(code) & 0x1f) | 0x80 | (dest_adr_vals.m << 5) | (src_adr_vals.m << 6);
    AppendAdrVals(&src_adr_vals);
    BAsmCode[CodeLen++] = src_length.val;
    AppendAdrVals(&dest_adr_vals);
    BAsmCode[CodeLen++] = dest_length.val;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeString_7b(Word code)
 * \brief  Handle Format VIIb String Instructons
 * \param  code machine code (LSB) and subcode (MSB)
 * ------------------------------------------------------------------------ */

static void DecodeString_7b(Word code)
{
  tAdrVals src_adr_vals, char_adr_vals;
  tLenVals src_length;

  if (ChkArgCnt(3, 3)
   && ChkOpSize(0x100 | (Lo(code)), Lo(code) + 2, -1, -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem)
   && DecodeLength(&ArgStr[2], &src_length)
   && DecodeAdr(&ArgStr[3], &char_adr_vals, MModReg | MModMem | MModImm))
  {
    CodeLen++;
    BAsmCode[CodeLen++] = (Hi(code) & 0x1f) | 0x80 | (char_adr_vals.m << 5) | (src_adr_vals.m << 6);
    AppendAdrVals(&src_adr_vals);
    BAsmCode[CodeLen++] = src_length.val;
    AppendAdrVals(&char_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     Decode_4(Word code)
 * \brief  handle relative branches
 * \param  code machine code for 16 bit displacement
 * ------------------------------------------------------------------------ */

static void Decode_4(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ok;
    const Boolean allow_8 = (code & 0xf0) == 0x70;
    tSymbolFlags flags;
    LongWord addr = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt32, &ok, &flags);

    if (ok)
    {
      LongInt delta = addr - EProgCounter();

      switch (AttrPartOpSize[0])
      {
        case eSymbolSizeUnknown:
          if ((delta > -128) && (delta < 127) && allow_8)
            goto br_short;
          else
            goto br_long;
        case eSymbolSizeFloat32Bit: /* .s */
        case eSymbolSize8Bit:  /* .b */
        br_short:
          if (!allow_8)
          {
            WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
            return;
          }
          code -= 0x10;
          if (!mFirstPassUnknownOrQuestionable(flags) && ((delta < -128) || (delta > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[CodeLen++] = code;
            BAsmCode[CodeLen++] = delta & 0xff;
          }
          break;
        case eSymbolSizeFloat64Bit: /* .l */
        case eSymbolSize16Bit: /* .h */
        br_long:
          if (!mFirstPassUnknownOrQuestionable(flags) && ((delta < -32768) || (delta > 32767))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[CodeLen++] = code;
            BAsmCode[CodeLen++] = delta & 0xff;
            BAsmCode[CodeLen++] = (delta >> 8) & 0xff;
          }
          break;
        default:
          WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
          return;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     Decode_6(Word code)
 * \brief  handle loop instructions
 * \param  code machine code for 16 bit displacement
 * ------------------------------------------------------------------------ */

static void Decode_6(Word code)
{
  tAdrVals reg_adr_vals;

  /* allow no attribute at all since it would be unclear whether to check
     for register operand's size (32 bits) or displacement operand's size
     (16 bits): */

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && DecodeAdr(&ArgStr[1], &reg_adr_vals, MModReg))
  {
    Boolean ok;
    tSymbolFlags flags;
    LongWord addr = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt32, &ok, &flags);

    if (ok)
    {
      LongInt delta = addr - EProgCounter();

      if (!mFirstPassUnknownOrQuestionable(flags) && ((delta < -32768) || (delta > 32767))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[CodeLen++] = Lo(code);
        BAsmCode[CodeLen++] = (reg_adr_vals.vals[0] & 0x1f) | (Hi(code) << 5);
        BAsmCode[CodeLen++] = delta & 0xff;
        BAsmCode[CodeLen++] = (delta >> 8) & 0xff;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCLRTLB(Word code)
 * \brief  handle CLRTLB instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCLRTLB(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && ChkPtrOpSize(Lo(code) | 0x100)
   && chk_sup_mode(code)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModImm | MModReg))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeGETPSW(Word code)
 * \brief  handle GETPSW instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeGETPSW(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && ChkOpSize(-1, -1, 0x100 | Lo(code), -1)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModReg))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeUPDPSW(Word code)
 * \brief  handle UPDPSW instruction
 * ------------------------------------------------------------------------ */

static void DecodeUPDPSW(Word code)
{
  UNUSED(code);

  if (ChkArgCnt(2, 2)
   && ChkOpSize(-1, 0x14a, 0x13, -1)
   && chk_sup_mode((OpSize == eSymbolSize32Bit) ? CODE_FLAG_SUPMODE : 0))
  {
    tAdrVals new_psw_adr_vals, mask_adr_vals;

    OpSize = eSymbolSize32Bit;

    if (DecodeAdr(&ArgStr[1], &new_psw_adr_vals, MModImm | MModMem | MModReg)
     && DecodeAdr(&ArgStr[2], &mask_adr_vals, MModImm | MModMem | MModReg))
      EncodeFormat1Or2(0, &new_psw_adr_vals, &mask_adr_vals);
  } 
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat3_bhw(Word code)
 * \brief  handle Format 3 instructions with 8/16/32 bit operand size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat3_bhw(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | (Lo(code) + 4), -1)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModReg | imm_mask_cond(code)))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJMP_JSR(Word code)
 * \brief  handle JMP/JSR instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeJMP_JSR(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && ChkNoAttrPart()
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem))
  {
    if (IsSPAutoIncDec(&adr_vals))
      WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[1]);
    BAsmCode[CodeLen] = code;
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat3_W(Word code)
 * \brief  handle instructions with single argument and 32 bit operand size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat3_W(Word code)
{
  tAdrVals adr_vals;

  /* TODO: allow PUSHM/POPM immediate arg as register list similar to 68K? */

  if (ChkArgCnt(1, 1)
   && chk_sup_mode(code)
   && ChkOpSize(-1, -1, 0x100 | Lo(code), -1)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModReg | imm_mask_cond(code)))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat3_H(Word code)
 * \brief  handle instructions with single argument and 16 bit operand size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat3_H(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && chk_sup_mode(code)
   && ChkOpSize(-1, 0x100 | Lo(code), -1, -1)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModReg | imm_mask_cond(code)))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat3_B(Word code)
 * \brief  handle instructions with single argument and 8 bit operand size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeFormat3_B(Word code)
{
  tAdrVals adr_vals;

  if (ChkArgCnt(1, 1)
   && chk_sup_mode(code)
   && ChkOpSize(0x100 | Lo(code), -1, -1, -1)
   && DecodeAdr(&ArgStr[1], &adr_vals, MModMem | MModReg | imm_mask_cond(code)))
  {
    EncodeFormat3(&adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCALL(Word code)
 * \brief  handle CALL instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCALL(Word code)
{
  tAdrVals target_adr_vals, arg_adr_vals;

  /* TODO: If register direct mode is disallowed for both arguments,
     checking for instruction format I does not make sense? */

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && DecodeAdr(&ArgStr[1], &target_adr_vals, MModMem)
   && DecodeAdr(&ArgStr[2], &arg_adr_vals, MModMem))
  {
    if (IsSPAutoIncDec(&target_adr_vals))
      WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[1]);
    if (IsSPAutoIncDec(&arg_adr_vals))
      WrStrErrorPos(ErrNum_Unpredictable, &ArgStr[2]);
    BAsmCode[CodeLen] = Lo(code);
    EncodeFormat1Or2(0x00, &target_adr_vals, &arg_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCHKA(Word Code)
 * \brief  Decode CHKA instruction(s)
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCHKA(Word code)
{
  tAdrVals va_adr_vals;

  /* TODO: va is an address operand, so Rn and #imm do not make sense for va? */

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart()
   && DecodeAdr(&ArgStr[1], &va_adr_vals, MModMem | MModReg | MModImm))
  {
    tAdrVals level_adr_vals;

    OpSize = eSymbolSize8Bit;
    if (DecodeAdr(&ArgStr[2], &level_adr_vals, MModMem | MModReg | MModImm))
    {
      chk_imm_level_range(&ArgStr[1], &level_adr_vals);

      BAsmCode[CodeLen] = Lo(code);
      EncodeFormat1Or2(0x00, &va_adr_vals, &level_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCAXI(Word Code)
 * \brief  Decode CAXI instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCAXI(Word code)
{
  tAdrVals reg_adr_vals, mem_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkOpSize(-1, -1, 0x100 | Lo(code), -1)
   && DecodeAdr(&ArgStr[1], &reg_adr_vals, MModReg)
   && DecodeAdr(&ArgStr[2], &mem_adr_vals, MModMem | MModReg))
    EncodeFormat1(reg_adr_vals.vals[0] & 0x1f, &mem_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCHLVL(Word Code)
 * \brief  Decode CHLVL instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCHLVL(Word code)
{
  tAdrVals level_adr_vals, arg_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize8Bit;

    if (DecodeAdr(&ArgStr[1], &level_adr_vals, MModMem | MModReg | MModImm)
     && DecodeAdr(&ArgStr[2], &arg_adr_vals, MModMem | MModReg | MModImm))
    {
      chk_imm_level_range(&ArgStr[1], &level_adr_vals);

      BAsmCode[CodeLen] = Lo(code);
      EncodeFormat1Or2(0, &level_adr_vals, &arg_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeGETXTE(Word Code)
 * \brief  Decode GETATE/GETPTE instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeGETXTE(Word code)
{
  tAdrVals va_adr_vals, dst_adr_vals;

  if (ChkArgCnt(2, 2)
   && chk_sup_mode(code)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize32Bit;

    if (DecodeAdr(&ArgStr[1], &va_adr_vals, MModMem | MModReg | MModImm)
     && DecodeAdr(&ArgStr[2], &dst_adr_vals, MModMem | MModReg | imm_mask_cond(code)))
    {
      BAsmCode[CodeLen] = Lo(code);
      EncodeFormat1Or2(0, &va_adr_vals, &dst_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCVT(Word code)
 * \brief  Handle CVT Instruction
 * \param  code machine code (abused as scratch)
 * ------------------------------------------------------------------------ */

static void DecodeCVT(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (!ChkArgCnt(2, 2))
    return;

  if ((AttrPartOpSize[0] == eSymbolSizeFloat32Bit) && (AttrPartOpSize[1] == eSymbolSizeFloat64Bit))
    code = 0x105f;
  else if ((AttrPartOpSize[0] == eSymbolSizeFloat64Bit) && (AttrPartOpSize[1] == eSymbolSizeFloat32Bit))
    code = 0x085f;
  else if ((AttrPartOpSize[0] == eSymbolSize32Bit) && (AttrPartOpSize[1] == eSymbolSizeFloat32Bit))
    code = 0x005f;
  else if ((AttrPartOpSize[0] == eSymbolSize32Bit) && (AttrPartOpSize[1] == eSymbolSizeFloat64Bit))
    code = 0x115f;
  else if ((AttrPartOpSize[0] == eSymbolSizeFloat32Bit) && (AttrPartOpSize[1] == eSymbolSize32Bit))
    code = 0x015f;
  else if ((AttrPartOpSize[0] == eSymbolSizeFloat64Bit) && (AttrPartOpSize[1] == eSymbolSize32Bit))
    code = 0x095f;
  else
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return;
  }

  OpSize = AttrPartOpSize[0];
  if (!DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg | MModMem))
    return;
  OpSize = AttrPartOpSize[1];
  if (!DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg | MModMem))
    return;

  BAsmCode[CodeLen] = Lo(code);
  EncodeFormat2(Hi(code), &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCVT(Word code)
 * \brief  Handle CVT Instruction
 * \param  code machine code (abused as scratch)
 * ------------------------------------------------------------------------ */

static void DecodeCVTD(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;
  Byte mask;
  Boolean ok;

  if (!ChkArgCnt(3, 3))
    return;

  if ((AttrPartOpSize[0] == eSymbolSize24Bit) && (AttrPartOpSize[1] == eSymbolSizeFloatDec96Bit));
  else if ((AttrPartOpSize[0] == eSymbolSizeFloatDec96Bit) && (AttrPartOpSize[1] == eSymbolSize24Bit))
    code |= 0x0800;
  else
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return;
  }

  OpSize = (AttrPartOpSize[0] == eSymbolSize24Bit) ? eSymbolSize8Bit : eSymbolSize16Bit;
  if (!DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg | MModMem | MModImm))
    return;
  OpSize = (AttrPartOpSize[1] == eSymbolSize24Bit) ? eSymbolSize8Bit : eSymbolSize16Bit;
  if (!DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg | MModMem))
    return;
  mask = EvalStrIntExpressionOffs(&ArgStr[3], ArgStr[3].str.p_str[0] == '#', Int8, &ok);

  BAsmCode[CodeLen] = Lo(code);
  EncodeFormat2(Hi(code), &src_adr_vals, &dest_adr_vals);
  BAsmCode[CodeLen++] = mask;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVS_MOVZ(Word code)
 * \brief  Handle MOVS/MOVZ Instructions
 * \param  code machine code for .bh
 * ------------------------------------------------------------------------ */

static void DecodeMOVS_MOVZ(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (!ChkArgCnt(2, 2))
    return;

  if ((AttrPartOpSize[0] == eSymbolSize8Bit) && (AttrPartOpSize[1] == eSymbolSize16Bit))
    ;
  else if ((AttrPartOpSize[0] == eSymbolSize8Bit) && (AttrPartOpSize[1] == eSymbolSize32Bit))
    code += 0x02;
  else if ((AttrPartOpSize[0] == eSymbolSize16Bit) && (AttrPartOpSize[1] == eSymbolSize32Bit))
    code += 0x12;
  else
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return;
  }

  OpSize = AttrPartOpSize[0];
  if (!DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg | MModMem | MModImm))
    return;
  OpSize = AttrPartOpSize[1];
  if (!DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg | MModMem))
    return;

  BAsmCode[CodeLen] = Lo(code);
  EncodeFormat1Or2(Lo(code), &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVT(Word code)
 * \brief  Handle MOVT Instructions
 * \param  code machine code for .hb
 * ------------------------------------------------------------------------ */

static void DecodeMOVT(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (!ChkArgCnt(2, 2))
    return;

  if ((AttrPartOpSize[0] == eSymbolSize16Bit) && (AttrPartOpSize[1] == eSymbolSize8Bit))
    ;
  else if ((AttrPartOpSize[0] == eSymbolSize32Bit) && (AttrPartOpSize[1] == eSymbolSize8Bit))
    code += 0x10;
  else if ((AttrPartOpSize[0] == eSymbolSize32Bit) && (AttrPartOpSize[1] == eSymbolSize16Bit))
    code += 0x12;
  else
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return;
  }

  OpSize = AttrPartOpSize[0];
  if (!DecodeAdr(&ArgStr[1], &src_adr_vals, MModReg | MModMem | MModImm))
    return;
  OpSize = AttrPartOpSize[1];
  if (!DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg | MModMem))
    return;

  BAsmCode[CodeLen] = Lo(code);
  EncodeFormat1Or2(Lo(code), &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBit(Word code)
 * \brief  decode bit operations
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeBit(Word code)
{
  tAdrVals offset_adr_vals, base_adr_vals;

  if (ChkArgCnt(2, 2)
   && ChkNoAttrPart())
  {
    OpSize = eSymbolSize32Bit;

    if (DecodeAdr(&ArgStr[1], &offset_adr_vals, MModMem | MModReg | MModImm)
     && DecodeAdr(&ArgStr[2], &base_adr_vals, MModMem | MModReg))
    {
      chk_imm_value_range(&ArgStr[1], &offset_adr_vals, "offset", 31);

      BAsmCode[CodeLen] = Lo(code);
      EncodeFormat1Or2(0, &offset_adr_vals, &base_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeIN_OUT(Word code)
 * \brief  handle IN & OUT instructions
 * \param  code machine code for 8 bit opsize; MSB -> is OUT
 * ------------------------------------------------------------------------ */

static void DecodeIN_OUT(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;
  Boolean is_out = !!(Hi(code) & 1);

  if (ChkArgCnt(2, 2)
   && chk_sup_mode(code)
   && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | Lo(code + 4), -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, is_out ? (MModImm | MModReg | MModMem) : (MModMem | MModIO))
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, is_out ? (MModMem | MModIO) : (MModReg | MModMem)))
    EncodeFormat1Or2(0, &src_adr_vals, &dest_adr_vals);
}


/*!------------------------------------------------------------------------
 * \fn     DecodeArith_B(Word code)
 * \brief  handle format 1/2 instructions with fixed 8 bit size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeArith_B(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && chk_sup_mode(code)
   && ChkOpSize(0x100 | Lo(code), -1, -1, -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModImm | MModReg | MModMem)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, imm_mask_cond(code) | MModReg | MModMem))
    EncodeFormat1Or2(0, &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeArith_W(Word code)
 * \brief  handle format 1/2 instructions with fixed 32 bit size
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeArith_W(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
   && chk_sup_mode(code)
   && ChkOpSize(-1, -1, 0x100 | Lo(code), -1)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModImm | MModReg | MModMem)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, imm_mask_cond(code) | MModReg | MModMem))
    EncodeFormat1Or2(0, &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDTASK(Word code)
 * \brief  Handle LDTASK Instruction
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeLDTASK(Word code)
{
  if (ChkArgCnt(2, 2)
   && chk_sup_mode(code)
   && ChkNoAttrPart())
  {
    tAdrVals list_adr_vals, tcb_ptr_adr_vals;

    OpSize = eSymbolSize32Bit;
    if (DecodeAdr(&ArgStr[1], &list_adr_vals, MModImm | MModReg | MModMem)
     && DecodeAdr(&ArgStr[2], &tcb_ptr_adr_vals, MModImm | MModReg | MModMem))
    {
      BAsmCode[CodeLen] = Lo(code);
      EncodeFormat1Or2(0, &list_adr_vals, &tcb_ptr_adr_vals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVEA(Word code)
 * \brief  Handle MOVEA Instruction
 * \param  code machine code for 8 bit operand size
 * ------------------------------------------------------------------------ */

static void DecodeMOVEA(Word code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  if (ChkArgCnt(2, 2)
    && ChkOpSize(Lo(code), Lo(code) + 2, 0x100 | (Lo(code) + 4), -1)
    && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem))
  {
    OpSize = eSymbolSize32Bit;
    if (DecodeAdr(&ArgStr[2], &dest_adr_vals, MModReg | MModMem))
      EncodeFormat1Or2(0, &src_adr_vals, &dest_adr_vals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV(Word Code)
 * \brief  handle MOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeMOV(Word Code)
{
  tAdrVals src_adr_vals, dest_adr_vals;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && ChkOpSize(0x09, 0x1b, 0x12d, 0x3f)
   && DecodeAdr(&ArgStr[1], &src_adr_vals, MModMem | MModImm | MModReg)
   && DecodeAdr(&ArgStr[2], &dest_adr_vals, MModMem | MModReg))
    EncodeFormat1Or2(0x00, &src_adr_vals, &dest_adr_vals);
}

/*!------------------------------------------------------------------------
 * \fn     CodePORT(Word code)
 * \brief  handle PORT statement
 * ------------------------------------------------------------------------ */

static void CodePORT(Word code)
{
  UNUSED(code);
  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

/*--------------------------------------------------------------------------*/
/* Instruction Lookup Table */

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  create lookup table
 * ------------------------------------------------------------------------ */

static void AddCondition(const char *p_name, Word code)
{
  char name[10];

  order_array_rsv_end(Conditions, tCondition);
  strmaxcpy(Conditions[InstrZ].name, p_name, sizeof(Conditions[InstrZ].name));
  Conditions[InstrZ].code = code;
  InstrZ++;

  if (*p_name)
  {
    as_snprintf(name, sizeof(name), "B%s", p_name);
    AddInstTable(InstTable, name, 0x70 | code, Decode_4);
    as_snprintf(name, sizeof(name), "DB%s", p_name);
    AddInstTable(InstTable, name, 0xc6 | (code & 1) | ((code << 7) & 0x0700), Decode_6);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "ABSF"   , 0x0a5c, DecodeArithF);
  AddInstTable(InstTable, "ADDF"   , 0x185c, DecodeArithF);
  AddInstTable(InstTable, "CMPF"   , 0x005c, DecodeArithF);
  AddInstTable(InstTable, "DIVF"   , 0x1b5c, DecodeArithF);
  AddInstTable(InstTable, "MOVF"   , 0x085c, DecodeArithF);
  AddInstTable(InstTable, "MULF"   , 0x1a5c, DecodeArithF);
  AddInstTable(InstTable, "NEGF"   , 0x095c, DecodeArithF);
  AddInstTable(InstTable, "SUBF"   , 0x195c, DecodeArithF);
  AddInstTable(InstTable, "SCLF"   , 0x105c, DecodeSCLF);

  AddInstTable(InstTable, "ADD"    , 0x80, DecodeArith);
  AddInstTable(InstTable, "ADDC"   , 0x90, DecodeArith);
  AddInstTable(InstTable, "AND"    , 0xa0, DecodeArith);
  AddInstTable(InstTable, "CMP"    , CODE_FLAG_OP2_IMM | 0xb8, DecodeArith);
  AddInstTable(InstTable, "DIV"    , 0xa1, DecodeArith);
  AddInstTable(InstTable, "DIVU"   , 0xb1, DecodeArith);
  AddInstTable(InstTable, "MUL"    , 0x81, DecodeArith);
  AddInstTable(InstTable, "MULU"   , 0x91, DecodeArith);
  AddInstTable(InstTable, "NEG"    , 0x39, DecodeArith);
  AddInstTable(InstTable, "NOT"    , 0x38, DecodeArith);
  AddInstTable(InstTable, "OR"     , 0x88, DecodeArith);
  AddInstTable(InstTable, "REM"    , 0x50, DecodeArith);
  AddInstTable(InstTable, "REMU"   , 0x51, DecodeArith);
  AddInstTable(InstTable, "SUB"    , 0xa8, DecodeArith);
  AddInstTable(InstTable, "SUBC"   , 0x98, DecodeArith);
  AddInstTable(InstTable, "XOR"    , 0xb0, DecodeArith);

  AddInstTable(InstTable, "DIVX"   , 0x00a6, DecodeArithX);
  AddInstTable(InstTable, "DIVUX"  , 0x00b6, DecodeArithX);
  AddInstTable(InstTable, "MULX"   , 0x0086, DecodeArithX);
  AddInstTable(InstTable, "MULUX"  , 0x0096, DecodeArithX);

  AddInstTable(InstTable, "ADDDC"  , 0x0059, DecodeArith_7c);
  AddInstTable(InstTable, "SUBDC"  , 0x0159, DecodeArith_7c);
  AddInstTable(InstTable, "SUBRDC" , 0x0259, DecodeArith_7c);

  AddInstTable(InstTable, "ANDBSU" , 0x105b, DecodeBitString_7b);
  AddInstTable(InstTable, "ANDBSD" , 0x115b, DecodeBitString_7b);
  AddInstTable(InstTable, "ANDNBSU", 0x125b, DecodeBitString_7b);
  AddInstTable(InstTable, "ANDNBSD", 0x135b, DecodeBitString_7b);
  AddInstTable(InstTable, "NOTBSU" , 0x0a5b, DecodeBitString_7b);
  AddInstTable(InstTable, "NOTBSD" , 0x0b5b, DecodeBitString_7b);
  AddInstTable(InstTable, "MOVBSU" , 0x085b, DecodeBitString_7b);
  AddInstTable(InstTable, "MOVBSD" , 0x095b, DecodeBitString_7b);
  AddInstTable(InstTable, "ORBSU"  , 0x145b, DecodeBitString_7b);
  AddInstTable(InstTable, "ORBSD"  , 0x155b, DecodeBitString_7b);
  AddInstTable(InstTable, "ORNBSU" , 0x165b, DecodeBitString_7b);
  AddInstTable(InstTable, "ORNBSD" , 0x175b, DecodeBitString_7b);
  AddInstTable(InstTable, "XORBSU" , 0x185b, DecodeBitString_7b);
  AddInstTable(InstTable, "XORBSD" , 0x195b, DecodeBitString_7b);
  AddInstTable(InstTable, "XORNBSU", 0x1a5b, DecodeBitString_7b);
  AddInstTable(InstTable, "XORNBSD", 0x1b5b, DecodeBitString_7b);

  AddInstTable(InstTable, "CMPC",   0x0058, DecodeString_7a);
  AddInstTable(InstTable, "CMPCF",  0x0158, DecodeString_7a);
  AddInstTable(InstTable, "CMPCS",  0x0258, DecodeString_7a);
  AddInstTable(InstTable, "MOVCU",  0x0858, DecodeString_7a);
  AddInstTable(InstTable, "MOVCD",  0x0958, DecodeString_7a);
  AddInstTable(InstTable, "MOVCFU", 0x0a58, DecodeString_7a);
  AddInstTable(InstTable, "MOVCFD", 0x0b58, DecodeString_7a);
  AddInstTable(InstTable, "MOVCS",  0x0c58, DecodeString_7a);
  AddInstTable(InstTable, "SCHCU",  0x1858, DecodeString_7b);
  AddInstTable(InstTable, "SCHCD",  0x1958, DecodeString_7b);
  AddInstTable(InstTable, "SKPCU",  0x1a58, DecodeString_7b);
  AddInstTable(InstTable, "SKPCD",  0x1b58, DecodeString_7b);

  AddInstTable(InstTable, "CMPBFS", CODE_FLAG_OP2_IMM | 0x005d, DecodeBitField_7b);
  AddInstTable(InstTable, "CMPBFZ", CODE_FLAG_OP2_IMM | 0x015d, DecodeBitField_7b);
  AddInstTable(InstTable, "CMPBFL", CODE_FLAG_OP2_IMM | 0x025d, DecodeBitField_7b);
  AddInstTable(InstTable, "SCH0BSU", 0x005b, DecodeBitField_7b);
  AddInstTable(InstTable, "SCH0BSD", 0x015b, DecodeBitField_7b);
  AddInstTable(InstTable, "SCH1BSU", 0x025b, DecodeBitField_7b);
  AddInstTable(InstTable, "SCH1BSD", 0x035b, DecodeBitField_7b);
  AddInstTable(InstTable, "EXTBFS", CODE_FLAG_LIM32 | 0x085d, DecodeBitField_7b);
  AddInstTable(InstTable, "EXTBFZ", CODE_FLAG_LIM32 | 0x095d, DecodeBitField_7b);
  AddInstTable(InstTable, "EXTBFL", CODE_FLAG_LIM32 | 0x0a5d, DecodeBitField_7b);
  AddInstTable(InstTable, "INSBFR", CODE_FLAG_LIM32 | 0x185d, DecodeBitField_7c);
  AddInstTable(InstTable, "INSBFL", CODE_FLAG_LIM32 | 0x195d, DecodeBitField_7c);

  AddInstTable(InstTable, "BRK"    , 0xc8, DecodeFixed);
  AddInstTable(InstTable, "BRKV"   , 0xc9, DecodeFixed);
  AddInstTable(InstTable, "CLRTLBA", CODE_FLAG_SUPMODE | 0x10, DecodeFixed);
  AddInstTable(InstTable, "DISPOSE", 0xcc, DecodeFixed);
  AddInstTable(InstTable, "HALT"   , 0x00, DecodeFixed);
  AddInstTable(InstTable, "NOP"    , 0xcd, DecodeFixed);
  AddInstTable(InstTable, "RSR"    , 0xca, DecodeFixed);
  AddInstTable(InstTable, "TRAPFL" , 0xcb, DecodeFixed);

  AddInstTable(InstTable, "MOV"    , 0x00, DecodeMOV);

  InstrZ = 0;
  AddCondition("GT", 0xf);
  AddCondition("GE", 0xd);
  AddCondition("LT", 0xc);
  AddCondition("LE", 0xe);
  AddCondition("H" , 0x7);
  AddCondition("NL", 0x3);
  AddCondition("L" , 0x2);
  AddCondition("NH", 0x6);
  AddCondition("E" , 0x4);
  AddCondition("NE", 0x5);
  AddCondition("V" , 0x0);
  AddCondition("NV", 0x1);
  AddCondition("N" , 0x8);
  AddCondition("P" , 0x9);
  AddCondition("C" , 0x2);
  AddCondition("NC", 0x3);
  AddCondition("Z" , 0x4);
  AddCondition("NZ", 0x5);
  AddCondition(""  , 0);
  AddInstTable(InstTable, "BR", 0x7a, Decode_4);
  AddInstTable(InstTable, "DBR", 0x05c6, Decode_6);
  AddInstTable(InstTable, "TB", 0x05c7, Decode_6);
  AddInstTable(InstTable, "BSR", 0x48, Decode_4);

  AddInstTable(InstTable, "CLRTLB", CODE_FLAG_SUPMODE | 0xfe, DecodeCLRTLB);
  AddInstTable(InstTable, "GETPSW", 0xf6, DecodeGETPSW);
  AddInstTable(InstTable, "UPDPSW", 0x134a, DecodeUPDPSW);
  AddInstTable(InstTable, "DEC", 0xd0, DecodeFormat3_bhw);
  AddInstTable(InstTable, "INC", 0xd8, DecodeFormat3_bhw);
  AddInstTable(InstTable, "TEST", 0xf0 | CODE_FLAG_OP2_IMM, DecodeFormat3_bhw);
  AddInstTable(InstTable, "JMP", 0xd6, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 0xe8, DecodeJMP_JSR);
  AddInstTable(InstTable, "POP", 0xe6, DecodeFormat3_W);
  AddInstTable(InstTable, "PUSH", CODE_FLAG_OP2_IMM | 0xee, DecodeFormat3_W);
  AddInstTable(InstTable, "POPM", CODE_FLAG_OP2_IMM | 0xe4, DecodeFormat3_W);
  AddInstTable(InstTable, "PUSHM", CODE_FLAG_OP2_IMM | 0xec, DecodeFormat3_W);
  AddInstTable(InstTable, "PREPARE", CODE_FLAG_OP2_IMM | 0xde, DecodeFormat3_W);
  AddInstTable(InstTable, "STTASK", CODE_FLAG_OP2_IMM | CODE_FLAG_SUPMODE | 0xfc, DecodeFormat3_W);
  AddInstTable(InstTable, "RET", CODE_FLAG_OP2_IMM | 0xe2, DecodeFormat3_H);
  AddInstTable(InstTable, "RETIS", CODE_FLAG_OP2_IMM | CODE_FLAG_SUPMODE | 0xfa, DecodeFormat3_H);
  AddInstTable(InstTable, "RETIU", CODE_FLAG_OP2_IMM | CODE_FLAG_SUPMODE | 0xea, DecodeFormat3_H);
  AddInstTable(InstTable, "TASI", 0xe0, DecodeFormat3_B);
  AddInstTable(InstTable, "TRAP", CODE_FLAG_OP2_IMM | 0xf8, DecodeFormat3_B);
  AddInstTable(InstTable, "CALL", 0x49, DecodeCALL);
  AddInstTable(InstTable, "ROT",  0x89, DecodeShift);
  AddInstTable(InstTable, "ROTC", 0x99, DecodeShift);
  AddInstTable(InstTable, "SHA",  0xb9, DecodeShift);
  AddInstTable(InstTable, "SHL",  0xa9, DecodeShift);
  AddInstTable(InstTable, "CHKAR", 0x4d, DecodeCHKA);
  AddInstTable(InstTable, "CHKAW", 0x4e, DecodeCHKA);
  AddInstTable(InstTable, "CHKAE", 0x4f, DecodeCHKA);
  AddInstTable(InstTable, "CAXI", 0x4c, DecodeCAXI);
  AddInstTable(InstTable, "CHLVL", 0x4b, DecodeCHLVL);
  AddInstTable(InstTable, "GETATE", CODE_FLAG_SUPMODE | 0x05, DecodeGETXTE);
  AddInstTable(InstTable, "UPDATE", CODE_FLAG_SUPMODE | 0x15, DecodeGETXTE);
  AddInstTable(InstTable, "GETPTE", CODE_FLAG_SUPMODE | 0x04, DecodeGETXTE);
  AddInstTable(InstTable, "UPDPTE", CODE_FLAG_OP2_IMM | CODE_FLAG_SUPMODE | 0x14, DecodeGETXTE);
  AddInstTable(InstTable, "GETRA", CODE_FLAG_SUPMODE | 0x03, DecodeGETXTE);
  AddInstTable(InstTable, "CLR1", 0xa7, DecodeBit);
  AddInstTable(InstTable, "NOT1", 0xb7, DecodeBit);
  AddInstTable(InstTable, "SET1", 0x97, DecodeBit);
  AddInstTable(InstTable, "TEST1", 0x87, DecodeBit);
  AddInstTable(InstTable, "CVT", 0, DecodeCVT);
  AddInstTable(InstTable, "CVTD", 0x1059, DecodeCVTD);
  AddInstTable(InstTable, "MOVS", 0x0a, DecodeMOVS_MOVZ);
  AddInstTable(InstTable, "MOVT", 0x19, DecodeMOVT);
  AddInstTable(InstTable, "MOVZ", 0x0b, DecodeMOVS_MOVZ);
  /* TODO: 0x31 for IN is guessed, same opcode for IN & OUT in manual? */
  AddInstTable(InstTable, "IN" , CODE_FLAG_SUPMODE | 0x0031, DecodeIN_OUT);
  AddInstTable(InstTable, "OUT", CODE_FLAG_SUPMODE | 0x0121, DecodeIN_OUT);
  AddInstTable(InstTable, "RVBIT", 0x08, DecodeArith_B);
  AddInstTable(InstTable, "RVBYTE", 0x2c, DecodeArith_W);
  AddInstTable(InstTable, "LDPR", CODE_FLAG_SUPMODE | CODE_FLAG_OP2_IMM | 0x12, DecodeArith_W);
  AddInstTable(InstTable, "STPR", CODE_FLAG_SUPMODE | 0x02, DecodeArith_W);
  AddInstTable(InstTable, "LDTASK", CODE_FLAG_SUPMODE  | 0x01, DecodeLDTASK);
  AddInstTable(InstTable, "MOVEA", 0x40, DecodeMOVEA);
  AddInstTable(InstTable, "XCH", 0x41, DecodeXCH);
  AddInstTable(InstTable, "SETF", 0x47, DecodeSETF);

  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "PORT", 0, CodePORT);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  destroy/cleanup lookup table
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  order_array_free(Conditions);
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Interface Functions */

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_V60(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for V60
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_V60(char *pArg, TempResult *pResult)
{
  Byte RegNum;

  if (DecodeRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_V60;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAttrPart_V60(void)
 * \brief  handle/decode attribute
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeAttrPart_V60(void)
{
  int l = strlen(AttrPart.str.p_str), z;

  if (l > 2)
  {
badattr:
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  for (z = 0; z < l; z++)
    switch (as_toupper(AttrPart.str.p_str[z]))
    {
      case 'B': AttrPartOpSize[z] = eSymbolSize8Bit; break;
      case 'H': AttrPartOpSize[z] = eSymbolSize16Bit; break;
      case 'W': AttrPartOpSize[z] = eSymbolSize32Bit; break;
      case 'D': AttrPartOpSize[z] = eSymbolSize64Bit; break;
      case 'P': AttrPartOpSize[z] = eSymbolSize24Bit; break;
      case 'S': AttrPartOpSize[z] = eSymbolSizeFloat32Bit; break;
      case 'L': AttrPartOpSize[z] = eSymbolSizeFloat64Bit; break;
      case 'Z': AttrPartOpSize[z] = eSymbolSizeFloatDec96Bit; break; /* just for cvtd.pz/zp */
      case '\0': break;
      default:
        goto badattr;
    }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     MakeCode_V60(void)
 * \brief  encode machine instruction
 * ------------------------------------------------------------------------ */

static void MakeCode_V60(void)
{
  CodeLen = 0; DontPrint = False;
  OpSize = eSymbolSizeUnknown;

  /* to be ignored */

  if (Memo("")) return;

  /* Pseudo Instructions */

  if (DecodeMoto16Pseudo(AttrPartOpSize[0], False))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_V60(void)
 * \brief  check whether insn makes own use of label
 * \return True if yes
 * ------------------------------------------------------------------------ */

static Boolean IsDef_V60(void)
{
  return Memo("REG")
      || Memo("PORT");
}

/*!------------------------------------------------------------------------
 * \fn     SwitchFrom_V60(void)
 * \brief  deinitialize as target
 * ------------------------------------------------------------------------ */

static void SwitchFrom_V60(void)
{
  DeinitFields();
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_V60(void *pUser)
 * \brief  prepare to assemble code for this target
 * \param  pUser CPU properties
 * ------------------------------------------------------------------------ */

static void SwitchTo_V60(void)
{
  const TFamilyDescr *pDescr = FindFamilyByName("V60");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$"; HeaderID = pDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode) | (1 << SegIO);
  Grans[SegCode] = Grans[SegIO] = 1;
  ListGrans[SegCode] = ListGrans[SegIO] = 1;
  SegInits[SegCode] = SegInits[SegIO] = 0;
  SegLimits[SegCode] = 0xfffffffful;
  SegLimits[SegIO] = 0xfffffful;

  DecodeAttrPart = DecodeAttrPart_V60;
  MakeCode = MakeCode_V60;
  IsDef = IsDef_V60;
  SwitchFrom = SwitchFrom_V60;
  InternSymbol = InternSymbol_V60;
  DissectReg = DissectReg_V60;
  AddMoto16PseudoONOFF(False);

  onoff_supmode_add();

  InitFields();
}

/*!------------------------------------------------------------------------
 * \fn     codev60_init(void)
 * \brief  register target to AS
 * ------------------------------------------------------------------------ */

void codev60_init(void)
{
  (void)AddCPU("70616", SwitchTo_V60);
}
