/* errmsg.h */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* Cross Assembler                                                           */
/*                                                                           */
/* Error message definition & associated checking                            */
/*                                                                           */
/*****************************************************************************/

#include <string.h>
#include <stdarg.h>
#include "strutil.h"

#include "datatypes.h"
#include "asmdef.h"
#include "asmpars.h"
#include "strutil.h"
#include "asmsub.h"
#include "nlmessages.h"
#include "cpulist.h"
#include "as.rsc"
#include "errmsg.h"

static tErrorNum GetDefaultCPUErrorNum(tErrorNum ThisNum)
{
  return ThisNum ? ThisNum : ErrNum_InstructionNotSupported;
}

/*!------------------------------------------------------------------------
 * \fn     report_error_range(LargeInt value, LargeInt ref, char comp_op, tErrorNum error_num, const tStrComp *p_comp)
 * \brief  core routine to emit range error message
 * \param  value offending value
 * \param  ref reference value was checked against
 * \param  comp_op comparison op that failed
 * \param  error_num error message to emit
 * \param  p_comp source argument (may be NULL)
 * \return constant False
 * ------------------------------------------------------------------------ */

static Boolean report_error_range(LargeInt value, LargeInt ref, char comp_op, tErrorNum error_num, const tStrComp *p_comp)
{
  char s[100];

  as_snprintf(s, sizeof(s), "%llld %c %llld", value, comp_op, ref);
  if (p_comp)
    WrXErrorPos(error_num, s, &p_comp->Pos);
  else
    WrXError(error_num, s);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     ChkRangePos(LargeInt Value, LargeInt Min, LargeInt Max, const tStrComp *p_comp)
 * \brief  check whether integer is in range and issue error if not
 * \param  Value value to check
 * \param  Min minimum of range
 * \param  Max maximum of range
 * \param  p_comp corresponding source argument (may be NULL)
 * \return TRUE if in-range and no error
 * ------------------------------------------------------------------------ */

Boolean ChkRangePos(LargeInt Value, LargeInt Min, LargeInt Max, const tStrComp *p_comp)
{
  if (Value < Min)
    return report_error_range(Value, Min, '<', ErrNum_UnderRange, p_comp);
  else if (Value > Max)
    return report_error_range(Value, Max, '>', ErrNum_OverRange, p_comp);
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkRangeWarnPos(LargeInt Value, LargeInt Min, LargeInt Max, const tStrComp *p_comp)
 * \brief  check whether integer is in range and issue error if not
 * \param  Value value to check
 * \param  Min minimum of range
 * \param  Max maximum of range
 * \param  p_comp corresponding source argument (may be NULL)
 * \return TRUE if in-range and no error
 * ------------------------------------------------------------------------ */

Boolean ChkRangeWarnPos(LargeInt Value, LargeInt Min, LargeInt Max, const tStrComp *p_comp)
{
  if (Value < Min)
    return report_error_range(Value, Min, '<', ErrNum_WUnderRange, p_comp);
  else if (Value > Max)
    return report_error_range(Value, Max, '>', ErrNum_WOverRange, p_comp);
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkArgCntExtPos(int ThisCnt, int MinCnt, int MaxCnt, const struct sLineComp *pComp)
 * \brief  check whether argument count is within given range and issue error if not
 * \param  ThisCnt count to check
 * \param  MinCnt minimum allowed count
 * \param  MaxCnt maximum allowed count
 * \param  pComp position in source line (optional)
 * \return TRUE if in-range and no error
 * ------------------------------------------------------------------------ */

Boolean ChkArgCntExtPos(int ThisCnt, int MinCnt, int MaxCnt, const struct sLineComp *pComp)
{
  if ((ThisCnt < MinCnt) || (ThisCnt > MaxCnt))
  {
    char Str[100];

    if (MinCnt != MaxCnt)
      as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgArgCntFromTo), MinCnt, MaxCnt, ThisCnt);
    else switch (MinCnt)
    {
      case 0:
        as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgArgCntZero), ThisCnt);
        break;
      case 1:
        as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgArgCntOne), ThisCnt);
        break;
      default:
        as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgArgCntMulti), MinCnt, ThisCnt);
    }
    if (pComp)
      WrXErrorPos(ErrNum_WrongArgCnt, Str, pComp);
    else
      WrXError(ErrNum_WrongArgCnt, Str);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkArgCntExtEitherOr(int ThisCnt, int EitherCnt, int OrCnt)
 * \brief  check whether argument count is according to given values and issue error if not
 * \param  ThisCnt count to check
 * \param  EitherCnt allowed count
 * \param  OrCnt other allowed count
 * \return TRUE if count OK and no error
 * ------------------------------------------------------------------------ */

Boolean ChkArgCntExtEitherOr(int ThisCnt, int EitherCnt, int OrCnt)
{
  if ((ThisCnt != EitherCnt) && (ThisCnt != OrCnt))
  {
    char Str[100];

    as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgArgCntEitherOr), EitherCnt, OrCnt, ThisCnt);
    WrXError(ErrNum_WrongArgCnt, Str);
    return False;
  }
  else
    return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkMinCPUExt(CPUVar MinCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is at least given one and issue error if not
 * \param  MinCPU minimum required CPU
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \return TRUE if currently selected CPU is OK and no error
 * ------------------------------------------------------------------------ */

extern Boolean ChkMinCPUExt(CPUVar MinCPU, tErrorNum ErrorNum)
{
  if (MomCPU < MinCPU)
  {
    const tCPUDef *pCPUDef;
    ErrorNum = GetDefaultCPUErrorNum(ErrorNum);

    pCPUDef = LookupCPUDefByVar(MinCPU);
    if (pCPUDef)
    {
      char Str[100];

      as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgMinCPUSupported), pCPUDef->Name);
      WrXError(ErrorNum, Str);
    }
    else
      WrError(ErrorNum);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     AChkMinCPUExtPos(CPUVar MinCPU, tErrorNum ErrorNum, const struct sStrComp *pComp)
 * \brief  check for minimum CPU of this addressing mode
 * \param  MinCPU min. CPU required
 * \param  ErrorNum error message to print
 * \param  pComp argument to complain about
 * \return True if CPU is OK
 * ------------------------------------------------------------------------ */

Boolean AChkMinCPUExtPos(CPUVar MinCPU, tErrorNum ErrorNum, const struct sStrComp *pComp)
{
  if (MomCPU < MinCPU)
  {
    WrStrErrorPos(ErrorNum, pComp);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkMaxCPUExt(CPUVar MaxCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is at most given one and issue error if not
 * \param  MaxCPU maximum required CPU
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \return TRUE if currently selected CPU is OK and no error
 * ------------------------------------------------------------------------ */

extern Boolean ChkMaxCPUExt(CPUVar MaxCPU, tErrorNum ErrorNum)
{
  if (MomCPU > MaxCPU)
  {
    const tCPUDef *pCPUDef;
    ErrorNum = GetDefaultCPUErrorNum(ErrorNum);

    pCPUDef = LookupCPUDefByVar(MaxCPU);
    if (pCPUDef)
    {
      char Str[100];

      as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgMaxCPUSupported), pCPUDef->Name);
      WrXError(ErrorNum, Str);
    }
    else
      WrError(ErrorNum);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkRangeCPUExt(CPUVar MinCPU, CPUVar MaxCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is within given range
 * \param  MinCPU minimum required CPU
 * \param  MaxCPU maximum required CPU
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \return TRUE if currently selected CPU is OK and no error
 * ------------------------------------------------------------------------ */

extern Boolean ChkRangeCPUExt(CPUVar MinCPU, CPUVar MaxCPU, tErrorNum ErrorNum)
{
  if ((MomCPU < MinCPU) || (MomCPU > MaxCPU))
  {
    const tCPUDef *pCPUDefMin, *pCPUDefMax;
    ErrorNum = GetDefaultCPUErrorNum(ErrorNum);

    pCPUDefMin = LookupCPUDefByVar(MinCPU);
    pCPUDefMax = LookupCPUDefByVar(MaxCPU);
    if (pCPUDefMin && pCPUDefMax)
    {
      char Str[100];

      as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgRangeCPUSupported), pCPUDefMin->Name, pCPUDefMax->Name);
      WrXError(ErrorNum, Str);
    }
    else
      WrError(ErrorNum);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkMinCPUExt(CPUVar MatchCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is given one and issue error if not
 * \param  MatchCPU required CPU
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \return TRUE if currently selected CPU is OK and no error
 * ------------------------------------------------------------------------ */

extern Boolean ChkExactCPUExt(CPUVar MatchCPU, tErrorNum ErrorNum)
{
  if (MomCPU != MatchCPU)
  {
    const tCPUDef *pCPUDef;
    ErrorNum = GetDefaultCPUErrorNum(ErrorNum);

    pCPUDef = LookupCPUDefByVar(MatchCPU);
    if (pCPUDef)
    {
      char Str[100];

      as_snprintf(Str, sizeof(Str), "%s%s%s", getmessage(Num_ErrMsgOnlyCPUSupported1), pCPUDef->Name, getmessage(Num_ErrMsgOnlyCPUSupported2));
      WrXError(ErrorNum, Str);
    }
    else
      WrError(ErrorNum);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkExcludeCPUExt(CPUVar MatchCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is NOT given one and issue error if not
 * \param  MatchCPU disallowed CPU
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \return TRUE if currently selected CPU is OK and no error
 * ------------------------------------------------------------------------ */

typedef struct
{
  const tCPUDef *pExcludeCPUDef;
  const tCPUDef *pLastCPUDef;
  String Str;
  Boolean First;
  Word ExcludeMask;
  CPUVar ExcludeCPUFirst;
} tExcludeContext;

static void IterateExclude(const tCPUDef *pThisCPUDef, void *pUser)
{
  tExcludeContext *pContext = (tExcludeContext*)pUser;

  /* ignore other families or aliases */

  if (pThisCPUDef)
  {
    if ((pThisCPUDef->SwitchProc != pContext->pExcludeCPUDef->SwitchProc)
     || ((1 << (pThisCPUDef->Number - pContext->ExcludeCPUFirst)) & pContext->ExcludeMask)
     || (pThisCPUDef->Number != pThisCPUDef->Orig))
      return;
  }

  if (pContext->pLastCPUDef)
  {
    if (!pContext->First)
      strmaxcat(pContext->Str, pThisCPUDef ? ", " : getmessage(Num_ErrMsgOnlyCPUSupportedOr), sizeof(pContext->Str));
    strmaxcat(pContext->Str, pContext->pLastCPUDef->Name, sizeof(pContext->Str));
    pContext->First = False;
  }
  pContext->pLastCPUDef = pThisCPUDef;
}

extern Boolean ChkExcludeCPUExt(CPUVar MatchCPU, tErrorNum ErrorNum)
{
  tExcludeContext Context;

  if (MomCPU != MatchCPU)
    return True;

  Context.pExcludeCPUDef = LookupCPUDefByVar(MatchCPU);

  if (Context.pExcludeCPUDef)
  {
    *Context.Str = '\0';
    Context.First = True;
    Context.pLastCPUDef = NULL;
    Context.ExcludeMask = 1;
    Context.ExcludeCPUFirst = MatchCPU;
    strmaxcat(Context.Str, getmessage(Num_ErrMsgOnlyCPUSupported1), sizeof(Context.Str));
    IterateCPUList(IterateExclude, &Context);
    IterateExclude(NULL, &Context);
    WrXError(GetDefaultCPUErrorNum(ErrorNum), Context.Str);
  }
  else
    WrError(GetDefaultCPUErrorNum(ErrorNum));
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     ChkExcludeCPUList(int ErrorNum, ...)
 * \brief  check whether currently selected CPU is one of the given ones and issue error if it is
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \param  ... List of CPUs terminated by CPUNone
 * \return Index (-1...-n) of matching CPU or 0 if current CPU does not match any
 * ------------------------------------------------------------------------ */

int ChkExcludeCPUList(int ErrorNum, ...)
{
  va_list ap;
  int Index = -1, FoundIndex = 0;
  CPUVar ThisCPU;

  va_start(ap, ErrorNum);
  while (True)
  {
    ThisCPU = va_arg(ap, CPUVar);
    if (ThisCPU == CPUNone)
      break;
    if (MomCPU == ThisCPU)
    {
      FoundIndex = Index;
      break;
    }
  }
  va_end(ap);

  if (FoundIndex < 0)
  {
    tExcludeContext Context;

    *Context.Str = '\0';
    Context.First = True;
    Context.pExcludeCPUDef =
    Context.pLastCPUDef = NULL;
    strmaxcat(Context.Str, getmessage(Num_ErrMsgOnlyCPUSupported1), sizeof(Context.Str));

    /* convert vararg list to bitmap */

    Context.ExcludeMask = 0;
    Context.ExcludeCPUFirst = CPUNone;
    va_start(ap, ErrorNum);
    while (TRUE)
    {
      ThisCPU = va_arg(ap, CPUVar);
      if (ThisCPU == CPUNone)
        break;
      if (!Context.pExcludeCPUDef)
        Context.pExcludeCPUDef = LookupCPUDefByVar(ThisCPU);
      if (Context.ExcludeCPUFirst == CPUNone)
      {
        Context.ExcludeCPUFirst = ThisCPU;
        Context.ExcludeMask = 1;
      }
      else if (ThisCPU > Context.ExcludeCPUFirst)
        Context.ExcludeMask |= 1 << (ThisCPU - Context.ExcludeCPUFirst);
      else if (ThisCPU < Context.ExcludeCPUFirst)
      {
        Context.ExcludeMask <<= Context.ExcludeCPUFirst - ThisCPU;
        Context.ExcludeMask |= 1;
        Context.ExcludeCPUFirst = ThisCPU;
      }
    }
    va_end(ap);
    IterateCPUList(IterateExclude, &Context);
    IterateExclude(NULL, &Context);
    WrXError(GetDefaultCPUErrorNum((tErrorNum)ErrorNum), Context.Str);
  }

  return FoundIndex;
}

/*!------------------------------------------------------------------------
 * \fn     ChkExactCPUList(int ErrorNum)
 * \brief  check whether currently selected CPU is one of the given ones and issue error if not
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \param  ... List of CPUs terminated by CPUNone
 * \return Index (0...) of matching CPU or -1 if current CPU does not match
 * ------------------------------------------------------------------------ */

extern int ChkExactCPUList(int ErrorNum, ...)
{
  va_list ap;
  String Str;
  CPUVar ThisCPU, NextCPU;
  const tCPUDef *pCPUDef;
  Boolean First = True;
  int FoundIndex = 0;

  va_start(ap, ErrorNum);
  while (True)
  {
    ThisCPU = va_arg(ap, CPUVar);
    if ((ThisCPU == CPUNone) || (MomCPU == ThisCPU))
      break;
    FoundIndex++;
  }
  va_end(ap);
  if (ThisCPU != CPUNone)
    return FoundIndex;

  va_start(ap, ErrorNum);
  *Str = '\0';
  strmaxcat(Str, getmessage(Num_ErrMsgOnlyCPUSupported1), sizeof(Str));
  ThisCPU = CPUNone;
  while (True)
  {
    NextCPU = va_arg(ap, CPUVar);
    pCPUDef = (ThisCPU != CPUNone) ? LookupCPUDefByVar(ThisCPU) : NULL;
    if (pCPUDef)
    {
      if (!First)
        strmaxcat(Str, (NextCPU == CPUNone) ? getmessage(Num_ErrMsgOnlyCPUSupportedOr) : ", ", sizeof(Str));
      strmaxcat(Str, pCPUDef->Name, sizeof(Str));
      First = False;
    }
    if (NextCPU == CPUNone)
      break;
    ThisCPU = NextCPU;
  }
  va_end(ap);
  strmaxcat(Str, getmessage(Num_ErrMsgOnlyCPUSupported2), sizeof(Str));
  WrXError(GetDefaultCPUErrorNum((tErrorNum)ErrorNum), Str);
  return -1;
}

/*!------------------------------------------------------------------------
 * \fn     ChkExactCPUMaskExt(Word CPUMask, CPUVar FirstCPU, tErrorNum ErrorNum)
 * \brief  check whether currently selected CPU is one of the given ones and issue error if not
 * \param  CPUMask bit mask of allowed CPUs
 * \param  CPUVar CPU corresponding to bit 0 in mask
 * \param  ErrorNum error to issue if not OK (0 = default message)
 * \param  ... List of CPUs terminated by CPUNone
 * \return Index (0...) of matching CPU or -1 if current CPU does not match
 * ------------------------------------------------------------------------ */

int ChkExactCPUMaskExt(Word CPUMask, CPUVar FirstCPU, tErrorNum ErrorNum)
{
  int Bit = MomCPU - FirstCPU;
  String Str;
  const tCPUDef *pCPUDef;
  Boolean First = True;
  CPUVar ThisCPU;

  if (CPUMask & (1 << Bit))
    return Bit;

  *Str = '\0';
  strmaxcat(Str, getmessage(Num_ErrMsgOnlyCPUSupported1), sizeof(Str));
  for (Bit = 0, ThisCPU = FirstCPU; Bit < 16; Bit++, ThisCPU++)
  {
    if (!(CPUMask & (1 << Bit)))
      continue;
    CPUMask &= ~(1 << Bit);
    pCPUDef = LookupCPUDefByVar(ThisCPU);
    if (pCPUDef)
    {
      if (!First)
        strmaxcat(Str, CPUMask ? ", " : getmessage(Num_ErrMsgOnlyCPUSupportedOr), sizeof(Str));
      strmaxcat(Str, pCPUDef->Name, sizeof(Str));
      First = False;
    }
  }
  strmaxcat(Str, getmessage(Num_ErrMsgOnlyCPUSupported2), sizeof(Str));
  WrXError(ErrorNum ? ErrorNum : ErrNum_InstructionNotSupported, Str);
  return -1;
}

/*!------------------------------------------------------------------------
 * \fn     ChkSamePage(LargeWord Addr1, LargeWord Addr2, unsigned PageBits)
 * \brief  check whether two addresses are of same page
 * \param  CurrAddr, DestAddr addresses to check
 * \param  PageBits page size in bits
 * \param  DestFlags symbol flags of DestAddr
 * \return TRUE if OK
 * ------------------------------------------------------------------------ */

Boolean ChkSamePage(LargeWord CurrAddr, LargeWord DestAddr, unsigned PageBits, tSymbolFlags DestFlags)
{
  LargeWord Mask = ~((1ul << PageBits) - 1);
  Boolean Result = ((CurrAddr & Mask) == (DestAddr & Mask))
                || mFirstPassUnknownOrQuestionable(DestFlags);
  if (!Result)
    WrError(ErrNum_JmpTargOnDiffPage);
  return Result;
}
