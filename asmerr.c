/* asmerr.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Error Message Writing                                                     */
/*                                                                           */
/*****************************************************************************/


#include "stdinc.h"
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "console.h"
#include "strutil.h"
#include "nlmessages.h"
#include "stdhandl.h"
#include "as.h"
#include "as.rsc"
#include "ioerrs.h"
#include "cmdarg.h"
#include "asmerr.h"

typedef struct sExpectError
{
  struct sExpectError *pNext;
  tErrorNum Num;
} tExpectError;

Word ErrorCount, WarnCount;
static tExpectError *pExpectErrors = NULL;
static Boolean InExpect = False;
static Boolean treat_warnings_as_errors,
               warn_sign_extension;

static void ClearExpectErrors(void)
{
  tExpectError *pOld;

  while (pExpectErrors)
  {
    pOld = pExpectErrors;
    pExpectErrors = pExpectErrors->pNext;
    free(pOld);
  }
}

static void AddExpectError(tExpectError *pExpectError)
{
  pExpectError->pNext = pExpectErrors;
  pExpectErrors = pExpectError;
}

/*!------------------------------------------------------------------------
 * \fn     FindAndTakeExpectError(tErrorNum Num)
 * \brief  check whether a certain error was expected
 * \param  Num number of error message that was expected
 * \return True if ther was an expectation
 * ------------------------------------------------------------------------ */

Boolean FindAndTakeExpectError(tErrorNum Num)
{
  tExpectError *pRun, *pPrev;

  for (pRun = pExpectErrors, pPrev = NULL; pRun; pPrev = pRun, pRun = pRun->pNext)
    if (Num == pRun->Num)
    {
      if (pPrev)
        pPrev->pNext = pRun->pNext;
      else
        pExpectErrors = pRun->pNext;
      free(pRun);
      return True;
    }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     GenLineMarker(char *pDest, unsigned DestSize, char Marker, const struct sLineComp *pLineComp, * const char *pPrefix)
 * \brief  print a line, optionally with a marking of a component below
 * \param  pDest where to write
 * \param  DestSize destination buffer size
 * \param  Marker character to use for marking
 * \param  pLineComp position and length of optional marker
 * \param  pPrefix what to print before (under)line
 * ------------------------------------------------------------------------ */

static void GenLineMarker(char *pDest, unsigned DestSize, char Marker, const struct sLineComp *pLineComp, const char *pPrefix)
{
  char *pRun;
  int z;

  strmaxcpy(pDest, pPrefix, DestSize);
  pRun = pDest + strlen(pDest);

  for (z = 0; (z < pLineComp->StartCol) && (pRun - pDest + 1 < (int)DestSize); z++)
    *pRun++ = ' ';
  for (z = 0; (z < (int)pLineComp->Len) && (pRun - pDest + 1 < (int)DestSize); z++)
    *pRun++ = Marker;
  *pRun = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     GenLineForMarking(char *pDest, unsigned DestSize, const char *pSrc, const char *pPrefix)
 * \brief  generate a line, in compressed form for optional marking of a component below
 * \param  pDest where to write
 * \param  DestSize destination buffer size
 * \param  pSrc line to print/underline
 * \param  pPrefix what to print before (under)line
 * ------------------------------------------------------------------------ */

static void GenLineForMarking(char *pDest, unsigned DestSize, const char *pSrc, const char *pPrefix)
{
  char *pRun;

  strmaxcpy(pDest, pPrefix, DestSize);
  pRun = pDest + strlen(pDest);

  /* replace TABs in line with spaces - column counting counts TAB as one character */

  for (; *pSrc && (pRun - pDest + 1 < (int)DestSize); pSrc++)
    *pRun++ = TabCompressed(*pSrc);
  *pRun = '\0';
}

/*!------------------------------------------------------------------------
 * \fn     EmergencyStop(void)
 * \brief  instantly stop upon fatal error
 * ------------------------------------------------------------------------ */

static void EmergencyStop(void)
{
  CloseIfOpen(&ErrorFile);
  CloseIfOpen(&LstFile);
  if (ShareMode != 0)
  {
    CloseIfOpen(&ShareFile);
    unlink(ShareName);
  }
  if (MacProOutput)
  {
    CloseIfOpen(&MacProFile);
    unlink(MacProName);
  }
  if (MacroOutput)
  {
    CloseIfOpen(&MacroFile);
    unlink(MacroName);
  }
  if (MakeDebug)
    CloseIfOpen(&Debug);
  if (CodeOutput)
  {
    CloseIfOpen(&PrgFile);
    unlink(OutName);
  }
}

/*!------------------------------------------------------------------------
 * \fn     ErrorNum2String(tErrorNum Num, char Buf, int BufSize)
 * \brief  convert error # to string
 * \param  Num numeric error
 * \param  Buf buffer for complex messages
 * \param  BufSize capacity of Buf
 * \return * to message
 * ------------------------------------------------------------------------ */

static const char *ErrorNum2String(tErrorNum Num, char *Buf, int BufSize)
{
  int msgno = -1;

  *Buf = '\0';
  switch (Num)
  {
    case ErrNum_UselessDisp:
      msgno = Num_ErrMsgUselessDisp; break;
    case ErrNum_ShortAddrPossible:
      msgno = Num_ErrMsgShortAddrPossible; break;
    case ErrNum_ShortJumpPossible:
      msgno = Num_ErrMsgShortJumpPossible; break;
    case ErrNum_NoShareFile:
      msgno = Num_ErrMsgNoShareFile; break;
    case ErrNum_BigDecFloat:
      msgno = Num_ErrMsgBigDecFloat; break;
    case ErrNum_PrivOrder:
      msgno = Num_ErrMsgPrivOrder; break;
    case ErrNum_DistNull:
      msgno = Num_ErrMsgDistNull; break;
    case ErrNum_WrongSegment:
      msgno = Num_ErrMsgWrongSegment; break;
    case ErrNum_InAccSegment:
      msgno = Num_ErrMsgInAccSegment; break;
    case ErrNum_PhaseErr:
      msgno = Num_ErrMsgPhaseErr; break;
    case ErrNum_Overlap:
      msgno = Num_ErrMsgOverlap; break;
    case ErrNum_OverlapReg:
      msgno = Num_ErrMsgOverlapReg; break;
    case ErrNum_NoCaseHit:
      msgno = Num_ErrMsgNoCaseHit; break;
    case ErrNum_InAccPage:
      msgno = Num_ErrMsgInAccPage; break;
    case ErrNum_RMustBeEven:
      msgno = Num_ErrMsgRMustBeEven; break;
    case ErrNum_Obsolete:
      msgno = Num_ErrMsgObsolete; break;
    case ErrNum_Unpredictable:
      msgno = Num_ErrMsgUnpredictable; break;
    case ErrNum_AlphaNoSense:
      msgno = Num_ErrMsgAlphaNoSense; break;
    case ErrNum_Senseless:
      msgno = Num_ErrMsgSenseless; break;
    case ErrNum_RepassUnknown:
      msgno = Num_ErrMsgRepassUnknown; break;
    case  ErrNum_AddrNotAligned:
      msgno = Num_ErrMsgAddrNotAligned; break;
    case ErrNum_IOAddrNotAllowed:
      msgno = Num_ErrMsgIOAddrNotAllowed; break;
    case ErrNum_Pipeline:
      msgno = Num_ErrMsgPipeline; break;
    case ErrNum_DoubleAdrRegUse:
      msgno = Num_ErrMsgDoubleAdrRegUse; break;
    case ErrNum_NotBitAddressable:
      msgno = Num_ErrMsgNotBitAddressable; break;
    case ErrNum_StackNotEmpty:
      msgno = Num_ErrMsgStackNotEmpty; break;
    case ErrNum_NULCharacter:
      msgno = Num_ErrMsgNULCharacter; break;
    case ErrNum_PageCrossing:
      msgno = Num_ErrMsgPageCrossing; break;
    case ErrNum_WUnderRange:
      msgno = Num_ErrMsgWUnderRange; break;
    case ErrNum_WOverRange:
      msgno = Num_ErrMsgWOverRange; break;
    case ErrNum_NegDUP:
      msgno = Num_ErrMsgNegDUP; break;
    case ErrNum_ConvIndX:
      msgno = Num_ErrMsgConvIndX; break;
    case ErrNum_NullResMem:
      msgno = Num_ErrMsgNullResMem; break;
    case ErrNum_BitNumberTruncated:
      msgno = Num_ErrMsgBitNumberTruncated; break;
    case ErrNum_InvRegisterPointer:
      msgno = Num_ErrMsgInvRegisterPointer; break;
    case ErrNum_MacArgRedef:
      msgno = Num_ErrMsgMacArgRedef; break;
    case ErrNum_Deprecated:
      msgno = Num_ErrMsgDeprecated; break;
    case ErrNum_SrcLEThanDest:
      msgno = Num_ErrMsgSrcLEThanDest; break;
    case ErrNum_TrapValidInstruction:
      msgno = Num_ErrMsgTrapValidInstruction; break;
    case ErrNum_PaddingAdded:
      msgno = Num_ErrMsgPaddingAdded; break;
    case ErrNum_RegNumWraparound:
      msgno = Num_ErrMsgRegNumWraparound; break;
    case ErrNum_IndexedForIndirect:
      msgno = Num_ErrMsgIndexedForIndirect; break;
    case ErrNum_NotInNormalMode:
      msgno = Num_ErrMsgNotInNormalMode; break;
    case ErrNum_NotInPanelMode:
      msgno = Num_ErrMsgNotInPanelMode; break;
    case ErrNum_ArgOutOfRange:
      msgno = Num_ErrMsgArgOutOfRange; break;
    case ErrNum_TrySkipMultiwordInstruction:
      msgno = Num_ErrMsgTrySkipMultiwordInstruction; break;
    case ErrNum_SignExtension:
      msgno = Num_ErrMsgSignExtension; break;
    case ErrNum_DoubleDef:
      msgno = Num_ErrMsgDoubleDef; break;
    case ErrNum_SymbolUndef:
      msgno = Num_ErrMsgSymbolUndef; break;
    case ErrNum_InvSymName:
      msgno = Num_ErrMsgInvSymName; break;
    case ErrNum_InvFormat:
      msgno = Num_ErrMsgInvFormat; break;
    case ErrNum_UseLessAttr:
      msgno = Num_ErrMsgUseLessAttr; break;
    case ErrNum_TooLongAttr:
      msgno = Num_ErrMsgTooLongAttr; break;
    case ErrNum_UndefAttr:
      msgno = Num_ErrMsgUndefAttr; break;
    case ErrNum_WrongArgCnt:
      msgno = Num_ErrMsgWrongArgCnt; break;
    case ErrNum_CannotSplitArg:
      msgno = Num_ErrMsgCannotSplitArg; break;
    case ErrNum_WrongOptCnt:
      msgno = Num_ErrMsgWrongOptCnt; break;
    case ErrNum_OnlyImmAddr:
      msgno = Num_ErrMsgOnlyImmAddr; break;
    case ErrNum_InvOpSize:
      msgno = Num_ErrMsgInvOpSize; break;
    case ErrNum_ConfOpSizes:
      msgno = Num_ErrMsgConfOpSizes; break;
    case ErrNum_UndefOpSizes:
      msgno = Num_ErrMsgUndefOpSizes; break;
    case ErrNum_StringOrIntButFloat:
      msgno = Num_ErrMsgStringOrIntButFloat; break;
    case ErrNum_IntButFloat:
      msgno = Num_ErrMsgIntButFloat; break;
    case ErrNum_FloatButString:
      msgno = Num_ErrMsgFloatButString; break;
    case ErrNum_OpTypeMismatch:
      msgno = Num_ErrMsgOpTypeMismatch; break;
    case ErrNum_StringButInt:
      msgno = Num_ErrMsgStringButInt; break;
    case ErrNum_StringButFloat:
      msgno = Num_ErrMsgStringButFloat; break;
    case ErrNum_TooManyArgs:
      msgno = Num_ErrMsgTooManyArgs; break;
    case ErrNum_IntButString:
      msgno = Num_ErrMsgIntButString; break;
    case ErrNum_IntOrFloatButString:
      msgno = Num_ErrMsgIntOrFloatButString; break;
    case ErrNum_ExpectString:
      msgno = Num_ErrMsgExpectString; break;
    case ErrNum_ExpectInt:
      msgno = Num_ErrMsgExpectInt; break;
    case ErrNum_StringOrIntOrFloatButReg:
      msgno = Num_ErrMsgStringOrIntOrFloatButReg; break;
    case ErrNum_ExpectIntOrString:
      msgno = Num_ErrMsgExpectIntOrString; break;
    case ErrNum_ExpectReg:
      msgno = Num_ErrMsgExpectReg; break;
    case ErrNum_RegWrongTarget:
      msgno = Num_ErrMsgRegWrongTarget; break;
    case ErrNum_FloatButInt:
      msgno = Num_ErrMsgFloatButInt; break;
    case ErrNum_NoRelocs:
      msgno = Num_ErrMsgNoRelocs; break;
    case ErrNum_IntOrFloatButReg:
      msgno = Num_ErrMsgIntOrFloatButReg; break;
    case ErrNum_IntOrStringButReg:
      msgno = Num_ErrMsgIntOrStringButReg; break;
    case ErrNum_IntButReg:
      msgno = Num_ErrMsgIntButReg; break;
    case ErrNum_UnresRelocs:
      msgno = Num_ErrMsgUnresRelocs; break;
    case ErrNum_Unexportable:
      msgno = Num_ErrMsgUnexportable; break;
    case ErrNum_UnknownInstruction:
      msgno = Num_ErrMsgUnknownInstruction; break;
    case ErrNum_BrackErr:
      msgno = Num_ErrMsgBrackErr; break;
    case ErrNum_DivByZero:
      msgno = Num_ErrMsgDivByZero; break;
    case ErrNum_UnderRange:
      msgno = Num_ErrMsgUnderRange; break;
    case ErrNum_OverRange:
      msgno = Num_ErrMsgOverRange; break;
    case ErrNum_NotPwr2:
      msgno = Num_ErrMsgNotPwr2; break;
    case ErrNum_NotAligned:
      msgno = Num_ErrMsgNotAligned; break;
    case ErrNum_DistTooBig:
      msgno = Num_ErrMsgDistTooBig; break;
    case ErrNum_TargOnDiffPage:
      msgno = Num_ErrMsgTargOnDiffPage; break;
    case ErrNum_InAccReg:
      msgno = Num_ErrMsgInAccReg; break;
    case ErrNum_NoShortAddr:
      msgno = Num_ErrMsgNoShortAddr; break;
    case ErrNum_InvAddrMode:
      msgno = Num_ErrMsgInvAddrMode; break;
    case ErrNum_AddrMustBeEven:
      msgno = Num_ErrMsgAddrMustBeEven; break;
    case ErrNum_AddrMustBeAligned:
      msgno = Num_ErrMsgAddrMustBeAligned; break;
    case ErrNum_InvParAddrMode:
      msgno = Num_ErrMsgInvParAddrMode; break;
    case ErrNum_UndefCond:
      msgno = Num_ErrMsgUndefCond; break;
    case ErrNum_IncompCond:
      msgno = Num_ErrMsgIncompCond; break;
    case ErrNum_UnknownFlag:
      msgno = Num_ErrMsgUnknownFlag; break;
    case ErrNum_DuplicateFlag:
      msgno = Num_ErrMsgDuplicateFlag; break;
    case ErrNum_UnknownInt:
      msgno = Num_ErrMsgUnknownInt; break;
    case ErrNum_DuplicateInt:
      msgno = Num_ErrMsgDuplicateInt; break;
    case ErrNum_JmpDistTooBig:
      msgno = Num_ErrMsgJmpDistTooBig; break;
    case ErrNum_JmpDistIsZero:
      msgno = Num_ErrMsgJmpDistIsZero; break;
    case ErrNum_DistIsOdd:
      msgno = Num_ErrMsgDistIsOdd; break;
    case ErrNum_SkipTargetMismatch:
      msgno = Num_ErrMsgSkipTargetMismatch; break;
    case ErrNum_InvShiftArg:
      msgno = Num_ErrMsgInvShiftArg; break;
    case ErrNum_Only1:
      msgno = Num_ErrMsgOnly1; break;
    case ErrNum_Range18:
      msgno = Num_ErrMsgRange18; break;
    case ErrNum_ShiftCntTooBig:
      msgno = Num_ErrMsgShiftCntTooBig; break;
    case ErrNum_InvRegList:
      msgno = Num_ErrMsgInvRegList; break;
    case ErrNum_InvCmpMode:
      msgno = Num_ErrMsgInvCmpMode; break;
    case ErrNum_InvCPUType:
      msgno = Num_ErrMsgInvCPUType; break;
    case ErrNum_InvFPUType:
      msgno = Num_ErrMsgInvFPUType; break;
    case ErrNum_InvPMMUType:
      msgno = Num_ErrMsgInvPMMUType; break;
    case ErrNum_InvCtrlReg:
      msgno = Num_ErrMsgInvCtrlReg; break;
    case ErrNum_InvReg:
      msgno = Num_ErrMsgInvReg; break;
    case ErrNum_DoubleReg:
      msgno = Num_ErrMsgDoubleReg; break;
    case ErrNum_RegBankMismatch:
      msgno = Num_ErrMsgRegBankMismatch; break;
    case ErrNum_UndefRegSize:
      msgno = Num_ErrMsgUndefRegSize; break;
    case ErrNum_InvOpOnReg:
      msgno = Num_ErrMsgInvOpOnReg; break;
    case ErrNum_NoSaveFrame:
      msgno = Num_ErrMsgNoSaveFrame; break;
    case ErrNum_NoRestoreFrame:
      msgno = Num_ErrMsgNoRestoreFrame; break;
    case ErrNum_UnknownMacArg:
      msgno = Num_ErrMsgUnknownMacArg; break;
    case ErrNum_MissEndif:
      msgno = Num_ErrMsgMissEndif; break;
    case ErrNum_InvIfConst:
      msgno = Num_ErrMsgInvIfConst; break;
    case ErrNum_DoubleSection:
      msgno = Num_ErrMsgDoubleSection; break;
    case ErrNum_InvSection:
      msgno = Num_ErrMsgInvSection; break;
    case ErrNum_MissingEndSect:
      msgno = Num_ErrMsgMissingEndSect; break;
    case ErrNum_WrongEndSect:
      msgno = Num_ErrMsgWrongEndSect; break;
    case ErrNum_NotInSection:
      msgno = Num_ErrMsgNotInSection; break;
    case ErrNum_UndefdForward:
      msgno = Num_ErrMsgUndefdForward; break;
    case ErrNum_ContForward:
      msgno = Num_ErrMsgContForward; break;
    case ErrNum_InvFuncArgCnt:
      msgno = Num_ErrMsgInvFuncArgCnt; break;
    case ErrNum_MsgMissingLTORG:
      msgno = Num_ErrMsgMissingLTORG; break;
    case ErrNum_InstructionNotSupported:
      as_snprintf(Buf, BufSize, getmessage(Num_ErrMsgInstructionNotOnThisCPUSupported), MomCPUIdent);
      break;
    case ErrNum_FPUNotEnabled:
      msgno = Num_ErrMsgFPUNotEnabled; break;
    case ErrNum_PMMUNotEnabled:
      msgno = Num_ErrMsgPMMUNotEnabled; break;
    case ErrNum_FullPMMUNotEnabled:
      msgno = Num_ErrMsgFullPMMUNotEnabled; break;
    case ErrNum_Z80SyntaxNotEnabled:
      msgno = Num_ErrMsgZ80SyntaxNotEnabled; break;
    case ErrNum_Z80SyntaxExclusive:
      msgno = Num_ErrMsgZ80SyntaxExclusive; break;
    case ErrNum_FPUInstructionNotSupported:
      as_snprintf(Buf, BufSize, getmessage(Num_ErrMsgInstructionNotOnThisFPUSupported), MomFPUIdent);
      break;
    case ErrNum_AddrModeNotSupported:
      as_snprintf(Buf, BufSize, getmessage(Num_ErrMsgAddrModeNotOnThisCPUSupported), MomCPUIdent);
      break;
    case ErrNum_CustomNotEnabled:
      msgno = Num_ErrMsgCustomNotEnabled; break;
    case ErrNum_InvBitPos:
      msgno = Num_ErrMsgInvBitPos; break;
    case ErrNum_OnlyOnOff:
      msgno = Num_ErrMsgOnlyOnOff; break;
    case ErrNum_StackEmpty:
      msgno = Num_ErrMsgStackEmpty; break;
    case ErrNum_NotOneBit:
      msgno = Num_ErrMsgNotOneBit; break;
    case ErrNum_MissingStruct:
      msgno = Num_ErrMsgMissingStruct; break;
    case ErrNum_OpenStruct:
      msgno = Num_ErrMsgOpenStruct; break;
    case ErrNum_WrongStruct:
      msgno = Num_ErrMsgWrongStruct; break;
    case ErrNum_PhaseDisallowed:
      msgno = Num_ErrMsgPhaseDisallowed; break;
    case ErrNum_InvStructDir:
      msgno = Num_ErrMsgInvStructDir; break;
    case ErrNum_DoubleStruct:
      msgno = Num_ErrMsgDoubleStruct; break;
    case ErrNum_UnresolvedStructRef:
      msgno = Num_ErrMsgUnresolvedStructRef; break;
    case ErrNum_DuplicateStructElem:
      msgno = Num_ErrMsgDuplicateStructElem; break;
    case ErrNum_NotRepeatable:
      msgno = Num_ErrMsgNotRepeatable; break;
    case ErrNum_ShortRead:
      msgno = Num_ErrMsgShortRead; break;
    case ErrNum_UnknownCodepage:
      msgno = Num_ErrMsgUnknownCodepage; break;
    case ErrNum_RomOffs063:
      msgno = Num_ErrMsgRomOffs063; break;
    case ErrNum_InvFCode:
      msgno = Num_ErrMsgInvFCode; break;
    case ErrNum_InvFMask:
      msgno = Num_ErrMsgInvFMask; break;
    case ErrNum_InvMMUReg:
      msgno = Num_ErrMsgInvMMUReg; break;
    case ErrNum_Level07:
      msgno = Num_ErrMsgLevel07; break;
    case ErrNum_InvBitMask:
      msgno = Num_ErrMsgInvBitMask; break;
    case ErrNum_InvRegPair:
      msgno = Num_ErrMsgInvRegPair; break;
    case ErrNum_OpenMacro:
      msgno = Num_ErrMsgOpenMacro; break;
    case ErrNum_OpenIRP:
      msgno = Num_ErrMsgOpenIRP; break;
    case ErrNum_OpenIRPC:
      msgno = Num_ErrMsgOpenIRPC; break;
    case ErrNum_OpenREPT:
      msgno = Num_ErrMsgOpenREPT; break;
    case ErrNum_OpenWHILE:
      msgno = Num_ErrMsgOpenWHILE; break;
    case ErrNum_EXITMOutsideMacro:
      msgno = Num_ErrMsgEXITMOutsideMacro; break;
    case ErrNum_TooManyMacParams:
      msgno = Num_ErrMsgTooManyMacParams; break;
    case ErrNum_UndefKeyArg:
      msgno = Num_ErrMsgUndefKeyArg; break;
    case ErrNum_NoPosArg:
      msgno = Num_ErrMsgNoPosArg; break;
    case ErrNum_DoubleMacro:
      msgno = Num_ErrMsgDoubleMacro; break;
    case ErrNum_FirstPassCalc:
      msgno = Num_ErrMsgFirstPassCalc; break;
    case ErrNum_TooManyNestedIfs:
      msgno = Num_ErrMsgTooManyNestedIfs; break;
    case ErrNum_MissingIf:
      msgno = Num_ErrMsgMissingIf; break;
    case ErrNum_RekMacro:
      msgno = Num_ErrMsgRekMacro; break;
    case ErrNum_UnknownFunc:
      msgno = Num_ErrMsgUnknownFunc; break;
    case ErrNum_InvFuncArg:
      msgno = Num_ErrMsgInvFuncArg; break;
    case ErrNum_FloatOverflow:
      msgno = Num_ErrMsgFloatOverflow; break;
    case ErrNum_InvArgPair:
      msgno = Num_ErrMsgInvArgPair; break;
    case ErrNum_NotOnThisAddress:
      msgno = Num_ErrMsgNotOnThisAddress; break;
    case ErrNum_NotFromThisAddress:
      msgno = Num_ErrMsgNotFromThisAddress; break;
    case ErrNum_JmpTargOnDiffPage:
      msgno = Num_ErrMsgJmpTargOnDiffPage; break;
    case ErrNum_TargOnDiffSection:
      msgno = Num_ErrMsgTargOnDiffSection; break;
    case ErrNum_CodeOverflow:
      msgno = Num_ErrMsgCodeOverflow; break;
    case ErrNum_AdrOverflow:
      msgno = Num_ErrMsgAdrOverflow; break;
    case ErrNum_MixDBDS:
      msgno = Num_ErrMsgMixDBDS; break;
    case ErrNum_NotInStruct:
      msgno = Num_ErrMsgNotInStruct; break;
    case ErrNum_ParNotPossible:
      msgno = Num_ErrMsgParNotPossible; break;
    case ErrNum_InvSegment:
      msgno = Num_ErrMsgInvSegment; break;
    case ErrNum_UnknownSegment:
      msgno = Num_ErrMsgUnknownSegment; break;
    case ErrNum_UnknownSegReg:
      msgno = Num_ErrMsgUnknownSegReg; break;
    case ErrNum_InvString:
      msgno = Num_ErrMsgInvString; break;
    case ErrNum_InvRegName:
      msgno = Num_ErrMsgInvRegName; break;
    case ErrNum_InvArg:
      msgno = Num_ErrMsgInvArg; break;
    case ErrNum_NoIndir:
      msgno = Num_ErrMsgNoIndir; break;
    case ErrNum_NotInThisSegment:
      msgno = Num_ErrMsgNotInThisSegment; break;
    case ErrNum_NotInMaxmode:
      msgno = Num_ErrMsgNotInMaxmode; break;
    case ErrNum_OnlyInMaxmode:
      msgno = Num_ErrMsgOnlyInMaxmode; break;
    case ErrNum_PackCrossBoundary:
      msgno = Num_ErrMsgPackCrossBoundary; break;
    case ErrNum_UnitMultipleUsed:
      msgno = Num_ErrMsgUnitMultipleUsed; break;
    case ErrNum_MultipleLongRead:
      msgno = Num_ErrMsgMultipleLongRead; break;
    case ErrNum_MultipleLongWrite:
      msgno = Num_ErrMsgMultipleLongWrite; break;
    case ErrNum_LongReadWithStore:
      msgno = Num_ErrMsgLongReadWithStore; break;
    case ErrNum_TooManyRegisterReads:
      msgno = Num_ErrMsgTooManyRegisterReads; break;
    case ErrNum_OverlapDests:
      msgno = Num_ErrMsgOverlapDests; break;
    case ErrNum_TooManyBranchesInExPacket:
      msgno = Num_ErrMsgTooManyBranchesInExPacket; break;
    case ErrNum_CannotUseUnit:
      msgno = Num_ErrMsgCannotUseUnit; break;
    case ErrNum_InvEscSequence:
      msgno = Num_ErrMsgInvEscSequence; break;
    case ErrNum_InvPrefixCombination:
      msgno = Num_ErrMsgInvPrefixCombination; break;
    case ErrNum_ConstantRedefinedAsVariable:
      msgno = Num_ErrMsgConstantRedefinedAsVariable; break;
    case ErrNum_VariableRedefinedAsConstant:
      msgno = Num_ErrMsgVariableRedefinedAsConstant; break;
    case ErrNum_StructNameMissing:
      msgno = Num_ErrMsgStructNameMissing; break;
    case ErrNum_EmptyArgument:
      msgno = Num_ErrMsgEmptyArgument; break;
    case ErrNum_Unimplemented:
      msgno = Num_ErrMsgUnimplemented; break;
    case ErrNum_FreestandingUnnamedStruct:
      msgno = Num_ErrMsgFreestandingUnnamedStruct; break;
    case ErrNum_STRUCTEndedByENDUNION:
      msgno = Num_ErrMsgSTRUCTEndedByENDUNION; break;
    case ErrNum_AddrOnDifferentPage:
      msgno = Num_ErrMsgAddrOnDifferentPage; break;
    case ErrNum_UnknownMacExpMod:
      msgno = Num_ErrMsgUnknownMacExpMod; break;
    case ErrNum_TooManyMacExpMod:
      msgno = Num_ErrMsgTooManyMacExpMod; break;
    case ErrNum_ConflictingMacExpMod:
      msgno = Num_ErrMsgConflictingMacExpMod; break;
    case ErrNum_InvalidPrepDir:
      msgno = Num_ErrMsgInvalidPrepDir; break;
    case ErrNum_ExpectedError:
      msgno = Num_ErrMsgExpectedError; break;
    case ErrNum_NoNestExpect:
      msgno = Num_ErrMsgNoNestExpect; break;
    case ErrNum_MissingENDEXPECT:
      msgno = Num_ErrMsgMissingENDEXPECT; break;
    case ErrNum_MissingEXPECT:
      msgno = Num_ErrMsgMissingEXPECT; break;
    case ErrNum_NoDefCkptReg:
      msgno = Num_ErrMsgNoDefCkptReg; break;
    case ErrNum_InvBitField:
      msgno = Num_ErrMsgInvBitField; break;
    case ErrNum_ArgValueMissing:
      msgno = Num_ErrMsgArgValueMissing; break;
    case ErrNum_UnknownArg:
      msgno = Num_ErrMsgUnknownArg; break;
    case ErrNum_IndexRegMustBe16Bit:
      msgno = Num_ErrMsgIndexRegMustBe16Bit; break;
    case ErrNum_IOAddrRegMustBe16Bit:
      msgno = Num_ErrMsgIOAddrRegMustBe16Bit; break;
    case ErrNum_SegAddrRegMustBe32Bit:
      msgno = Num_ErrMsgSegAddrRegMustBe32Bit; break;
    case ErrNum_NonSegAddrRegMustBe16Bit:
      msgno = Num_ErrMsgNonSegAddrRegMustBe16Bit; break;
    case ErrNum_InvStructArgument:
      msgno = Num_ErrMsgInvStructArgument; break;
    case ErrNum_TooManyArrayDimensions:
      msgno = Num_ErrMsgTooManyArrayDimensions; break;
    case ErrNum_InvIntFormat:
      msgno = Num_ErrMsgInvIntFormat; break;
    case ErrNum_InvIntFormatList:
      msgno = Num_ErrMsgInvIntFormatList; break;
    case ErrNum_InvScale:
      msgno = Num_ErrMsgInvScale; break;
    case ErrNum_ConfStringOpt:
      msgno = Num_ErrMsgConfStringOpt; break;
    case ErrNum_UnknownStringOpt:
      msgno = Num_ErrMsgUnknownStringOpt; break;
    case ErrNum_InvCacheInvMode:
      msgno = Num_ErrMsgInvCacheInvMode; break;
    case ErrNum_InvCfgList:
      msgno = Num_ErrMsgInvCfgList; break;
    case ErrNum_ConfBitBltOpt:
      msgno = Num_ErrMsgConfBitBltOpt; break;
    case ErrNum_UnknownBitBltOpt:
      msgno = Num_ErrMsgUnknownBitBltOpt; break;
    case ErrNum_InvCBAR:
      msgno = Num_ErrMsgInvCBAR; break;
    case ErrNum_InAccPageErr:
      msgno = Num_ErrMsgInAccPageErr; break;
    case ErrNum_InAccFieldErr:
      msgno = Num_ErrMsgInAccFieldErr; break;
    case ErrNum_TargInDiffField:
      msgno = Num_ErrMsgTargInDiffField; break;
    case ErrNum_InvCombination:
      msgno = Num_ErrMsgInvCombination; break;
    case ErrNum_InternalError:
      msgno = Num_ErrMsgInternalError; break;
    case ErrNum_OpeningFile:
      msgno = Num_ErrMsgOpeningFile; break;
    case ErrNum_ListWrError:
      msgno = Num_ErrMsgListWrError; break;
    case ErrNum_FileReadError:
      msgno = Num_ErrMsgFileReadError; break;
    case ErrNum_FileWriteError:
      msgno = Num_ErrMsgFileWriteError; break;
    case ErrNum_HeapOvfl:
      msgno = Num_ErrMsgHeapOvfl; break;
    case ErrNum_StackOvfl:
      msgno = Num_ErrMsgStackOvfl; break;
    case ErrNum_MaxIncLevelExceeded:
      msgno = Num_ErrMsgMaxIncLevelExceeded; break;
    default:
      as_snprintf(Buf, BufSize, "%s %d", getmessage(Num_ErrMsgIntError), (int) Num);
  }
  return (msgno != -1) ? getmessage(msgno) : Buf;
}

/*!------------------------------------------------------------------------
 * \fn     WrErrorString(const char *pMessage, const char *pAdd, Boolean Warning, Boolean Fatal,
                         const char *pExtendError, const struct sLineComp *pLineComp)
 * \brief  write error message, combined with string component of current src line
 * \param  pMessage textual error message
 * \param  Warning error is warning?
 * \param  Fatal error is fatal?
 * \param  pExtendError extended error explanation
 * \param  pLineComp associated string component
 * ------------------------------------------------------------------------ */

void WrErrorString(const char *pMessage, const char *pAdd, Boolean Warning, Boolean Fatal,
                   const char *pExtendError, const struct sLineComp *pLineComp)
{
  String ErrStr[4];
  unsigned ErrStrCount = 0, z;
  char *p;
  int l;
  const char *pLeadIn = GNUErrors ? "" : "> > > ";
  FILE *pErrFile;
  Boolean ErrorsWrittenToListing = False;

  if (treat_warnings_as_errors && Warning && !Fatal)
    Warning = False;

  strcpy(ErrStr[ErrStrCount], pLeadIn);
  p = GetErrorPos();
  if (p)
  {
    l = strlen(p) - 1;
    if ((l >= 0) && (p[l] == ' '))
      p[l] = '\0';
    strmaxcat(ErrStr[ErrStrCount], p, STRINGSIZE);
    free(p);
  }
  if (pLineComp)
  {
    char Num[20];

    as_snprintf(Num, sizeof(Num), ":%d", pLineComp->StartCol + 1);
    strmaxcat(ErrStr[ErrStrCount], Num, STRINGSIZE);
  }
  if (Warning || !GNUErrors)
  {
    strmaxcat(ErrStr[ErrStrCount], ": ", STRINGSIZE);
    strmaxcat(ErrStr[ErrStrCount], getmessage(Warning ? Num_WarnName : Num_ErrName), STRINGSIZE);
  }
  strmaxcat(ErrStr[ErrStrCount], pAdd, STRINGSIZE);
  strmaxcat(ErrStr[ErrStrCount], ": ", STRINGSIZE);
  if (Warning)
    WarnCount++;
  else
    ErrorCount++;

  strmaxcat(ErrStr[ErrStrCount], pMessage, STRINGSIZE);
  if ((ExtendErrors > 0) && pExtendError)
  {
    if (GNUErrors)
      strmaxcat(ErrStr[ErrStrCount], " '", STRINGSIZE);
    else
      strcpy(ErrStr[++ErrStrCount], pLeadIn);
    strmaxcat(ErrStr[ErrStrCount], pExtendError, STRINGSIZE);
    if (GNUErrors)
      strmaxcat(ErrStr[ErrStrCount], "'", STRINGSIZE);
  }
  if ((ExtendErrors > 1) || ((ExtendErrors > 0) && pLineComp))
  {
    strcpy(ErrStr[++ErrStrCount], "");
    GenLineForMarking(ErrStr[ErrStrCount], STRINGSIZE, OneLine.p_str, pLeadIn);
    if (pLineComp)
    {
      strcpy(ErrStr[++ErrStrCount], "");
      GenLineMarker(ErrStr[ErrStrCount], STRINGSIZE, '~', pLineComp, pLeadIn);
    }
  }

  if (strcmp(LstName, "/dev/null") && !Fatal)
  {
    for (z = 0; z <= ErrStrCount; z++)
      WrLstLine(ErrStr[z]);
    ErrorsWrittenToListing = True;
  }

  if (!ErrorFile)
    OpenWithStandard(&ErrorFile, ErrorName);
  pErrFile = ErrorFile ? ErrorFile : stdout;
  if (strcmp(LstName, "!1") || !ListOn || !ErrorsWrittenToListing)
  {
    for (z = 0; z <= ErrStrCount; z++)
      if (ErrorFile)
        fprintf(pErrFile, "%s\n", ErrStr[z]);
      else
        WrConsoleLine(ErrStr[z], True);
  }

  if (Fatal)
    fprintf(pErrFile, "%s\n", getmessage(Num_ErrMsgIsFatal));
  else if (MaxErrors && (ErrorCount >= MaxErrors))
  {
    fprintf(pErrFile, "%s\n", getmessage(Num_ErrMsgTooManyErrors));
    Fatal = True;
  }

  if (Fatal)
  {
    EmergencyStop();
    exit(3);
  }
}

/*!------------------------------------------------------------------------
 * \fn     WrXErrorPos(tErrorNum Num, const char *pExtendError, const struct sLineComp *pLineComp)
 * \brief  write number-coded error message, combined with extended explamation and string component of current src line
 * \param  Num error number
 * \param  pExtendError extended error explanation
 * \param  pLineComp associated string component
 * ------------------------------------------------------------------------ */

void WrXErrorPos(tErrorNum Num, const char *pExtendError, const struct sLineComp *pLineComp)
{
  String h;
  char Add[11];
  const char *pErrorMsg;

  if (FindAndTakeExpectError(Num))
    return;

  if (!CodeOutput && (Num == ErrNum_UnknownInstruction))
    return;

  if (SuppWarns && (Num < 1000))
    return;

  switch (Num)
  {
    case ErrNum_SignExtension:
      if (!warn_sign_extension)
        return;
      break;
    default:
      break;
  }

  pErrorMsg = ErrorNum2String(Num, h, sizeof(h));

  if (((Num == ErrNum_JmpTargOnDiffPage) || (Num == ErrNum_JmpDistTooBig))
   && !Repass)
    JmpErrors++;

  if (NumericErrors)
    as_snprintf(Add, sizeof(Add), " #%d", (int)Num);
  else
    *Add = '\0';
  WrErrorString(pErrorMsg, Add, Num < 1000, Num >= 10000, pExtendError, pLineComp);
}

/*!------------------------------------------------------------------------
 * \fn     WrStrErrorPos(tErrorNum Num, const const struct sLineComp *pLineComp)
 * \brief  write number-coded error message, combined with string component of current src line
 * \param  Num error number
 * \param  pStrComp associated string component
 * ------------------------------------------------------------------------ */

void WrStrErrorPos(tErrorNum Num, const struct sStrComp *pStrComp)
{
  WrXErrorPos(Num, pStrComp->str.p_str, &pStrComp->Pos);
}

/*!------------------------------------------------------------------------
 * \fn     WrError(tErrorNum Num)
 * \brief  write number-coded error message, without any explanation
 * \param  Num error number
 * ------------------------------------------------------------------------ */

void WrError(tErrorNum Num)
{
  WrXErrorPos(Num, NULL, NULL);
}

/*!------------------------------------------------------------------------
 * \fn     WrXError(tErrorNum Num, const char *pExtError)
 * \brief  write number-coded error message with extended explanation
 * \param  Num error number
 * \param  pExtendError extended error explanation
 * ------------------------------------------------------------------------ */

void WrXError(tErrorNum Num, const char *pExtError)
{
  WrXErrorPos(Num, pExtError, NULL);
}

/*!------------------------------------------------------------------------
 * \fn     void ChkIO(tErrorNum ErrNo)
 * \brief  check for I/O error and report given error if yes
 * \param  ErrNo error number to report if error occured
 * ------------------------------------------------------------------------ */

void ChkIO(tErrorNum ErrNo)
{
  int io;

  io = errno;
  if ((io == 0) || (io == 19) || (io == 25))
    return;

  WrXError(ErrNo, GetErrorMsg(io));
}

/*!------------------------------------------------------------------------
 * \fn     ChkXIO(tErrorNum ErrNo, char *pExtError)
 * \brief  check for I/O error and report given error if yes
 * \param  ErrNo error number to report if error occured
 * \param  pExtError, file name, as plain string
 * ------------------------------------------------------------------------ */

void ChkXIO(tErrorNum ErrNo, char *pExtError)
{
  tStrComp TmpComp;

  StrCompMkTemp(&TmpComp, pExtError, 0);
  ChkStrIO(ErrNo, &TmpComp);
}

/*!------------------------------------------------------------------------
 * \fn     void ChkIO(tErrorNum ErrNo)
 * \brief  check for I/O error and report given error if yes
 * \param  ErrNo error number to report if error occured
 * \param  pComp, file name, as string component with position
 * ------------------------------------------------------------------------ */

void ChkStrIO(tErrorNum ErrNo, const struct sStrComp *pComp)
{
  int io;
  String s;

  io = errno;
  if ((io == 0) || (io == 19) || (io == 25))
    return;

  as_snprintf(s, STRINGSIZE, "%s: %s", pComp->str.p_str, GetErrorMsg(io));
  if ((pComp->Pos.StartCol >= 0) || pComp->Pos.Len)
    WrXErrorPos(ErrNo, s, &pComp->Pos);
  else
    WrXError(ErrNo, s);
}

/*!------------------------------------------------------------------------
 * \fn     CodeEXPECT(Word Code)
 * \brief  process EXPECT command
 * ------------------------------------------------------------------------ */

void CodeEXPECT(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, ArgCntMax));
  else if (InExpect) WrStrErrorPos(ErrNum_NoNestExpect, &OpPart);
  else
  {
    int z;
    Boolean OK;
    tErrorNum Num;

    for (z = 1; z <= ArgCnt; z++)
    {
      Num = (tErrorNum)EvalStrIntExpression(&ArgStr[z], UInt16, &OK);
      if (OK)
      {
        tExpectError *pNew = (tExpectError*)calloc(1, sizeof(*pNew));
        pNew->Num = Num;
        AddExpectError(pNew);
      }
    }
    InExpect = True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     CodeENDEXPECT(Word Code)
 * \brief  process ENDEXPECT command
 * ------------------------------------------------------------------------ */

void CodeENDEXPECT(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(0, 0));
  else if (!InExpect) WrStrErrorPos(ErrNum_MissingEXPECT, &OpPart);
  else
  {
    tExpectError *pCurr;
    String h;

    while (pExpectErrors)
    {
      pCurr = pExpectErrors;
      pExpectErrors = pCurr->pNext;
      WrXError(ErrNum_ExpectedError, ErrorNum2String(pCurr->Num, h, sizeof(h)));
      free(pCurr);
    }
    InExpect = False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AsmErrPassInit(void)
 * \brief  module initialization prior to (another) pass through sources
 * ------------------------------------------------------------------------ */

void AsmErrPassInit(void)
{
  ErrorCount = 0;
  WarnCount = 0;
  ClearExpectErrors();
  InExpect = False;
}

/*!------------------------------------------------------------------------
 * \fn     AsmErrPassExit(void)
 * \brief  module checks & cleanups after a pass through sources
 * ------------------------------------------------------------------------ */

void AsmErrPassExit(void)
{
  if (InExpect)
    WrError(ErrNum_MissingENDEXPECT);
  ClearExpectErrors();
  InExpect = False;
}

static as_cmd_result_t cmd_treat_warnings_as_errors(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);

  treat_warnings_as_errors = !negate;
  return e_cmd_ok;
}

static as_cmd_result_t cmd_warn_sign_extension(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);

  if (negate)
    return e_cmd_err;
  warn_sign_extension = True;
  return e_cmd_ok;
}

static as_cmd_result_t cmd_no_warn_sign_extension(Boolean negate, const char *p_arg)
{
  UNUSED(p_arg);

  if (negate)
    return e_cmd_err;
  warn_sign_extension = False;
  return e_cmd_ok;
}

static const as_cmd_rec_t cmd_params[] =
{
  { "werror"                     , cmd_treat_warnings_as_errors },
  { "wimplicit-sign-extension"   , cmd_warn_sign_extension      },
  { "wno-implicit-sign-extension", cmd_no_warn_sign_extension   }
};

/*!------------------------------------------------------------------------
 * \fn     asmerr_init(void)
 * \brief  module setup
 * ------------------------------------------------------------------------ */

void asmerr_init(void)
{
  treat_warnings_as_errors = False;
  warn_sign_extension = True;
  as_cmd_register(cmd_params, as_array_size(cmd_params));
}
