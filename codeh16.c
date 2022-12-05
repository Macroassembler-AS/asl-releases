/* codeh16.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator Hitachi H16                                                */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "headids.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"
#include "nls.h"

#include "codeh16.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPU641016;
static tSymbolSize OpSize;
static tStrComp FormatPart;

#define REG_SP 15

typedef enum
{
  eAdrModeNone = -1,
  eAdrModeReg = 0,
  eAdrModeIReg = 1,
  eAdrModePost = 2,
  eAdrModePre = 3,
  eAdrModeImm = 4,
  eAdrModeAbs = 5,
  eAdrModeIRegScale = 6,
  eAdrModeIdxScale = 7,
  eAdrModePCIdxScale = 8,
  eAdrModePCRel = 9,
  eAdrModeDoubleIndir = 10
} tAdrMode;

#define MModeReg (1 << eAdrModeReg)
#define MModeIReg (1 << eAdrModeIReg)
#define MModePost (1 << eAdrModePost)
#define MModePre (1 << eAdrModePre)
#define MModeImm (1 << eAdrModeImm)
#define MModeAbs (1 << eAdrModeAbs)
#define MModeIRegScale (1 << eAdrModeIRegScale)
#define MModeIdxScale (1 << eAdrModeIdxScale)
#define MModePCIdxScale (1 << eAdrModePCIdxScale)
#define MModePCRel (1 << eAdrModePCRel)
#define MModeDoubleIndir (1 << eAdrModeDoubleIndir)
#define MModeAll (MModeReg | MModeIReg | MModePost | MModePre | MModeImm | MModeAbs | MModeIRegScale | MModeIdxScale | MModePCIdxScale | MModePCRel | MModeDoubleIndir)

typedef enum
{
  eFormatNone,
  eFormatG,
  eFormatQ,
  eFormatR,
  eFormatRQ,
  eFormatF  /* no source-side format; used for MOV->MOVF conversion */
} tFormat;

#ifdef __cplusplus
# include "codeh16.hpp"
#endif

typedef struct
{
  tAdrMode Mode;
  unsigned Cnt;
  Byte Vals[10];
} tAdrVals;

#define NOREG 255
#define PCREG 254
#define NormalReg(Reg) ((Reg) <= 15)
#define DefIndexRegSize eSymbolSize32Bit /* correct? */

#define PREFIX_CRn 0x70
#define PREFIX_PRn 0x74

#define eSymbolSize5Bit ((tSymbolSize)-2)  /* for shift cnt/bit pos arg */
#define eSymbolSize4Bit ((tSymbolSize)-3)  /* for bit pos arg */
#define eSymbolSize3Bit ((tSymbolSize)-4)  /* for bit pos arg */

typedef struct
{
  LongInt OuterDisp, InnerDisp;
  Boolean OuterDispPresent, InnerDispPresent;
  tSymbolSize OuterDispSize, InnerDispSize;
  Byte BaseReg, IndexReg, InnerReg;
  ShortInt BaseRegIncr, IndexRegSize;
  Byte IndexRegScale;
  Byte RegPrefix;
} tAdrComps;

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     SetOpSize(tSymbolSize NewOpSize)
 * \brief  set (instruction) operand size; complain if size mismatch
 * \param  NewOpSize size to set
 * \return True if setting/confirmation succeeded
 * ------------------------------------------------------------------------ */

static Boolean SetOpSize(tSymbolSize NewOpSize)
{
  if ((OpSize != eSymbolSizeUnknown) && (NewOpSize != OpSize))
  {
    WrError(ErrNum_ConfOpSizes);
    return False;
  }
  OpSize = NewOpSize;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkOpSize(tSymbolSize DefaultSize, Word Mask)
 * \brief  possibly set & check instruction operand size
 * \param  DefaultSize size to set if size is unknown so far
 * \param  Mask bit mask of allowed sizes (8/16/32 -> bit 0/1/2)
 * \return True if operand size is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkOpSize(tSymbolSize DefaultSize, Word Mask)
{
  if (OpSize == eSymbolSizeUnknown)
    OpSize = DefaultSize;
  if (!((Mask >> OpSize) & 1))
  {
    WrError(ErrNum_InvOpSize);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFormat(unsigned FormatMask)
 * \brief  decode instruction's format to internal enum
 * \param  FormatMask bit mask of allowed formats
 * \return resulting format or FormatNone if unknown/disallowed
 * ------------------------------------------------------------------------ */

static tFormat DecodeFormat(unsigned FormatMask)
{
  static const char Formats[][3] = { "G", "Q", "R", "RQ", "" };
  tFormat Result;

  FormatMask >>= 1;
  for (Result = (tFormat)0; *Formats[Result]; Result++, FormatMask >>= 1)
    if ((FormatMask & 1) && !as_strcasecmp(FormatPart.str.p_str, Formats[Result]))
      return Result + 1;
  return eFormatNone;
}

/*!------------------------------------------------------------------------
 * \fn     ChkEmptyFormat(void)
 * \brief  check whether no format spec was given and complain if there is one
 * \return True if no format spec was given
 * ------------------------------------------------------------------------ */

static Boolean ChkEmptyFormat(void)
{
  if (*FormatPart.str.p_str)
  {
    WrStrErrorPos(ErrNum_InvFormat, &FormatPart);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ResetAdrVals(tAdrVals *pAdrVals)
 * \brief  reset tAdrVals structure to initial state
 * \param  pAdrVals struct to reset
 * ------------------------------------------------------------------------ */

static void ResetAdrVals(tAdrVals *pAdrVals)
{
  pAdrVals->Mode = eAdrModeNone;
  pAdrVals->Cnt = 0;
}

/*!------------------------------------------------------------------------
 * \fn     AppendAdrVals(tAdrVals *pAdrVals, Byte Value)
 * \brief  append another byte to tAdrVals' expansion byte list; check for overflow
 * \param  pAdrVals AdrVals to extend
 * \param  Value new value to append
 * ------------------------------------------------------------------------ */

static void AppendAdrVals(tAdrVals *pAdrVals, Byte Value)
{
  if (pAdrVals->Cnt >= (sizeof(pAdrVals->Vals) / sizeof(*pAdrVals->Vals)))
  {
    WrError(ErrNum_CodeOverflow);
    return;
  }
  pAdrVals->Vals[pAdrVals->Cnt++] = Value;
}

/*!------------------------------------------------------------------------
 * \fn     AppendRegPrefix(tAdrVals *pAdrVals, Byte RegPrefix)
 * \brief  append register prefix to tAdrVals if not a global bank register
 * \param  pAdrVals AdrVals to extend
 * \param  RegPrefix register prefix (0x70/0x74 or 0x00 for no prefix)
 * ------------------------------------------------------------------------ */

static void AppendRegPrefix(tAdrVals *pAdrVals, Byte RegPrefix)
{
  if (RegPrefix)
    AppendAdrVals(pAdrVals, RegPrefix);
}

/*!------------------------------------------------------------------------
 * \fn     AppendBySize(Byte *pDest, LongInt Value, tSymbolSize OpSize)
 * \brief  append an integer value to byte array, BE order
 * \param  pDest where to write
 * \param  Value integer value to append
 * \param  OpSize operand size of value (3/4/5/8/16/32 bits)
 * \return # of bytes appended
 * ------------------------------------------------------------------------ */

static unsigned GetH16SymbolSizeBytes(tSymbolSize Size)
{
  return (Size <= eSymbolSize5Bit) ? 1 : GetSymbolSizeBytes(Size);
}

static unsigned AppendBySize(Byte *pDest, LongInt Value, tSymbolSize OpSize)
{
  int z;
  unsigned Cnt = GetH16SymbolSizeBytes(OpSize);

  for (z = Cnt - 1; z >= 0; z--)
  {
    pDest[z] = Value & 0xff;
    Value >>= 8;
  }
  return Cnt;
}

/*!------------------------------------------------------------------------
 * \fn     AppendToAdrValsBySize(tAdrVals *pAdrVals, LongInt Value, tSymbolSize OpSize)
 * \brief  append an integer value to tAdrVals
 * \param  pAdrVals AdrVals to extend
 * \param  Value integer value to append
 * \param  OpSize operand size of value (3/4/5/8/16/32 bits)
 * ------------------------------------------------------------------------ */

static void AppendToAdrValsBySize(tAdrVals *pAdrVals, LongInt Value, tSymbolSize OpSize)
{
  unsigned Cnt = GetH16SymbolSizeBytes(OpSize);

  if ((pAdrVals->Cnt + Cnt) > (sizeof(pAdrVals->Vals) / sizeof(*pAdrVals->Vals)))
  {
    WrError(ErrNum_CodeOverflow);
    return;
  }
  AppendBySize(pAdrVals->Vals + pAdrVals->Cnt, Value, OpSize);
  pAdrVals->Cnt += Cnt;
}

/*!------------------------------------------------------------------------
 * \fn     ResetAdrComps(tAdrComps *pComps)
 * \brief  reset indirect address parser context to empty state
 * \param  pComps context to reset
 * ------------------------------------------------------------------------ */

static void ResetAdrComps(tAdrComps *pComps)
{
  pComps->OuterDisp = pComps->InnerDisp = 0;
  pComps->OuterDispPresent = pComps->InnerDispPresent = False;
  pComps->OuterDispSize = pComps->InnerDispSize = eSymbolSizeUnknown;
  pComps->BaseReg = pComps->IndexReg = pComps->InnerReg = NOREG;
  pComps->IndexRegScale = pComps->BaseRegIncr = 0;
  pComps->IndexRegSize = DefIndexRegSize;
  pComps->RegPrefix = 0x00;
}

/*!------------------------------------------------------------------------
 * \fn     SplitOpSize(tStrComp *pArg, tSymbolSize *pSize)
 * \brief  split off explicit size spec from argument
 * \param  pArg argument to treat
 * \param  pSize resulting size if suffix was present
 * \return True if a suffix was split off
 * ------------------------------------------------------------------------ */

static Boolean SplitOpSize(tStrComp *pArg, tSymbolSize *pSize)
{
  static const char Suffixes[][4] = { ".B", ":8", ".W", ":16", ".L", ":32" };
  int l = strlen(pArg->str.p_str), l2;
  unsigned z;

  for (z = 0; z < sizeof(Suffixes) / sizeof(*Suffixes); z++)
  {
    l2 = strlen(Suffixes[z]);
    if ((l > l2) && !as_strcasecmp(pArg->str.p_str + l - l2, Suffixes[z]))
    {
      StrCompShorten(pArg, l2);
      *pSize = (tSymbolSize)(z / 2);
      return True;
    }
  }
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     SplitScale(tStrComp *pArg, Byte *pScale)
 * \brief  split off scaling option from (register) argument
 * \param  pArg argument to treat
 * \param  pScale resulting scale if suffix was present
 * \return True if a suffix was split off
 * ------------------------------------------------------------------------ */

static const char ScaleSuffixes[][3] = { "*1", "*2", "*4", "*8" };

static char SplitScale(tStrComp *pArg, Byte *pScale)
{
  int l = strlen(pArg->str.p_str);
  unsigned z;

  if (l >= 2)
    for (z = 0; z < sizeof(ScaleSuffixes) / sizeof(*ScaleSuffixes); z++)
      if (!as_strcasecmp(pArg->str.p_str + l - 2, ScaleSuffixes[z]))
      {
        StrCompShorten(pArg, 2);
        *pScale = z;
        return *ScaleSuffixes[z];
      }
  return '\0';
}

/*!------------------------------------------------------------------------
 * \fn     AppendScale(tStrComp *pArg, Byte Scale)
 * \brief  append scaling option to (register) argument
 * \param  pArg argument to augment by scaling
 * \param  Scale Scale to append
 * ------------------------------------------------------------------------ */

static void AppendScale(tStrComp *pArg, Byte Scale)
{
  strcat(pArg->str.p_str, ScaleSuffixes[Scale]);
  pArg->Pos.Len += 2;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const char *pArg, Byte *pResult, Byte *pPrefix)
 * \brief  check whether argument is a register and return register # if yes
 * \param  pArg argument to check
 * \param  pResult register # if argument is a valid (general) register
 * \param  pPrefix prefix for previous/current bank registers; 0 for global bank registers
 * \return True if argument is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const char *pArg, Byte *pResult, Byte *pPrefix)
{
  Boolean OK;

  if (!as_strcasecmp(pArg, "SP"))
  {
    *pResult = REGSYM_FLAG_ALIAS | REG_SP;
    *pPrefix = 0x00;
    return True;
  }

  if ((strlen(pArg) > 1)
   && (as_toupper(*pArg) == 'R'))
    *pPrefix = 0x00;
  else if ((strlen(pArg) > 2)
   && (as_toupper(*pArg) == 'C')
   && (as_toupper(pArg[1]) == 'R'))
    *pPrefix = PREFIX_CRn;
  else if ((strlen(pArg) > 2)
   && (as_toupper(*pArg) == 'P')
   && (as_toupper(pArg[1]) == 'R'))
    *pPrefix = PREFIX_PRn;
  else
    return False;

  *pResult = ConstLongInt(pArg + 1 + !!*pPrefix, &OK, 10);
  return (OK && (*pResult <= 15));
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_H16(char *pDest, size_t DestSize, tRegInt Reg, tSymbolSize Size)
 * \brief  dissect register symbols - H16 variant
 * \param  pDest destination buffer
 * \param  DestSize size of destination buffer
 * \param  Reg register number
 * \param  Size register size
 * ------------------------------------------------------------------------ */

static void DissectReg_H16(char *pDest, size_t DestSize, tRegInt Reg, tSymbolSize Size)
{
  switch (Size)
  {
    case eSymbolSize32Bit:
      if (Reg == (REGSYM_FLAG_ALIAS | REG_SP))
        as_snprintf(pDest, DestSize, "SP");
      else
      {
        if (Hi(Reg) == PREFIX_CRn)
          as_snprintf(pDest, DestSize, "CR");
        else if (Hi(Reg) == PREFIX_PRn)
          as_snprintf(pDest, DestSize, "PR");
        else if (!Hi(Reg))
          as_snprintf(pDest, DestSize, "R");
        else
          as_snprintf(pDest, DestSize, "%u-", Hi(Reg));
        as_snprcatf(pDest, DestSize, "%u", Lo(Reg));
      }
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", Size, Reg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     compare_reg_h16(tRegInt reg1_num, tSymbolSize reg1_size, tRegInt reg2_num, tRegInt reg2_size)
 * \brief  compare two register symbols
 * \param  reg1_num 1st register's number
 * \param  reg1_size 1st register's data size
 * \param  reg2_num 2nd register's number
 * \param  reg2_size 2nd register's data size
 * \return 0, -1, 1, -2
 * ------------------------------------------------------------------------ */

static int compare_reg_h16(tRegInt reg1_num, tSymbolSize size1, tRegInt reg2_num, tSymbolSize size2)
{
  if ((size1 != eSymbolSize32Bit)
   || (size2 != eSymbolSize32Bit)
   || (Hi(reg1_num) != Hi(reg2_num)))
    return -2;

  reg1_num = Lo(reg1_num) & ~REGSYM_FLAG_ALIAS;
  reg2_num = Lo(reg2_num) & ~REGSYM_FLAG_ALIAS;

  if (reg1_num < reg2_num)
    return -1;
  else if (reg1_num > reg2_num)
    return 1;
  else
    return 0;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, Byte *pReg, Byte *pPrefix, Boolean AllowPrefix, Boolean MustBeReg)
 * \brief  check whether argument is CPU register or register alias
 * \param  pArg source argument
 * \param  pReg register number
 * \param  pPrefix prefix for previous/current bank registers; 0 for global bank registers
 * \param  AllowPrefix allow previous/current bank registers
 * \param  MustBeReg argument must be register
 * \return True if argument is a register
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeReg(const tStrComp *pArg, Byte *pReg, Byte *pPrefix, Boolean AllowPrefix, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (DecodeRegCore(pArg->str.p_str, pReg, pPrefix))
    RegEvalResult = eIsReg;
  else
  {
    RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize32Bit, MustBeReg);
    *pPrefix = Hi(RegDescr.Reg);
    *pReg = Lo(RegDescr.Reg);
  }

  if (RegEvalResult == eIsReg)
  {
    if (!AllowPrefix && *pPrefix)
    {
      WrStrErrorPos(ErrNum_InvReg, pArg);
      RegEvalResult = MustBeReg ? eIsNoReg : eRegAbort;
    }
  }
  *pReg &= ~REGSYM_FLAG_ALIAS;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegList(Word *pDest, int StartIdx, int StopIdx)
 * \brief  parse a register list
 * \param  pDest destination buffer for register bit field
 * \param  StartIdx index of first argument of list
 * \param  StopIdx index of last argument of list
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegList(Word *pDest, int StartIdx, int StopIdx)
{
  int Index;
  char *pSep;
  Byte RegStart, RegStop, Prefix;
  Word Mask;

  *pDest = 0;
  for (Index = StartIdx; Index <= StopIdx; Index++)
  {
    pSep = strchr(ArgStr[Index].str.p_str, '-');
    if (pSep)
    {
      tStrComp StartComp, StopComp;

      StrCompSplitRef(&StartComp, &StopComp, &ArgStr[Index], pSep);
      KillPostBlanksStrComp(&StartComp);
      KillPrefBlanksStrCompRef(&StopComp);
      if (!DecodeReg(&StartComp, &RegStart, &Prefix, False, True))
        return False;
      if (!DecodeReg(&StopComp, &RegStop, &Prefix, False, True))
        return False;
      if (RegStart > RegStop)
      {
        Prefix = RegStart;
        RegStart = RegStop;
        RegStop = Prefix;
      }
      Mask = ((1ul << (RegStop + 1)) - 1) & ~((1ul << (RegStart)) - 1);
    }
    else
    {
      if (!DecodeReg(&ArgStr[Index], &RegStart, &Prefix, False, True))
        return False;
      Mask = 1ul << RegStart;
    }
    if (*pDest & Mask)
    {
      WrStrErrorPos(ErrNum_DoubleReg, &ArgStr[Index]);
      return False;
    }
    *pDest |= Mask;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegWithSize(const tStrComp *pArg, Byte *pReg, tSymbolSize *pSize, Byte *pPrefix, Boolean MustBeReg)
 * \brief  check whether argument is register with possible size spec
 * \param  pArg argument to check
 * \param  pReg returns register # if argument is a register
 * \param  pSize returns size spec if argument is a register (may be SizeUnknown)
 * \param  pPrefix prefix for previous/current bank registers; 0 for global bank registers
 * \param  MustBeReg argument must be register
 * \return RegEval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult DecodeRegWithSize(const tStrComp *pArg, Byte *pReg, tSymbolSize *pSize, Byte *pPrefix, Boolean MustBeReg)
{
  String Str;
  tStrComp Copy;

  StrCompMkTemp(&Copy, Str, sizeof(Str));
  StrCompCopy(&Copy, pArg);
  if (!SplitOpSize(&Copy, pSize))
    *pSize = eSymbolSizeUnknown;
  return DecodeReg(&Copy, pReg, pPrefix, True, MustBeReg);
}

/*!------------------------------------------------------------------------
 * \fn     CheckSup(const tStrComp *pArg)
 * \brief  check whether supervisor mode is enabled and warn if not
 * \param  pArg possible argument to print if sup mode is not enabled
 * \return True (only a warning, no error)
 * ------------------------------------------------------------------------ */

static Boolean CheckSup(const tStrComp *pArg)
{
  if (!SupAllowed)
    WrStrErrorPos(ErrNum_PrivOrder, pArg);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCReg(const tStrComp *pArg, Byte *pResult)
 * \brief  check whether argument id a control register
 * \param  pArg argument to check
 * \param  pResult control register # if argument is a control register
 * \return True if argument is a control register
 * ------------------------------------------------------------------------ */

typedef struct
{
  const char *pName;
  Byte Code;
  tSymbolSize Size;
} tCReg;

static const tCReg CRegs[] =
{
  { "CCR"  , 0x20, eSymbolSize8Bit  },
  { "VBNR" , 0x01, eSymbolSize8Bit  },
  { "CBNR" , 0x40, eSymbolSize32Bit },
  { "BSP"  , 0x41, eSymbolSize32Bit },
  { "BMR"  , 0x80, eSymbolSize8Bit  },
  { "GBNR" , 0x81, eSymbolSize8Bit  },
  { "SR"   , 0xa0, eSymbolSize16Bit },
  { "EBR"  , 0xc0, eSymbolSize32Bit },
  { "RBR"  , 0xc1, eSymbolSize32Bit },
  { "USP"  , 0xc2, eSymbolSize32Bit },
  { "IBR"  , 0xc3, eSymbolSize32Bit },
  { NULL   , 0x00, eSymbolSizeUnknown },
};

static Boolean DecodeCReg(const tStrComp *pArg, Byte *pResult)
{
  const tCReg *pReg;

  for (pReg = CRegs; pReg->pName; pReg++)
    if (!as_strcasecmp(pArg->str.p_str, pReg->pName))
    {
      if ((pReg->Code & 0x80) && !CheckSup(pArg))
        return False;
      if (!SetOpSize(pReg->Size))
        return False;
      *pResult = pReg->Code;
      return True;
    }
  WrStrErrorPos(ErrNum_InvCtrlReg, pArg);
  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DeduceSize(LongInt Value)
 * \brief  deduce minimum operand size needed to represent value, assuming sign-extension
 * \param  Value value to judge
 * \return resulting minimum size
 * ------------------------------------------------------------------------ */

static tSymbolSize DeduceSize(LongInt Value)
{
  if ((Value >= -128) && (Value <= 127))
    return eSymbolSize8Bit;
  if ((Value >= -32768) && (Value <= 32767))
    return eSymbolSize16Bit;
  return eSymbolSize32Bit;
}

/*!------------------------------------------------------------------------
 * \fn     DeduceSize8_32(LongInt Value)
 * \brief  similar to DeduceSize, but excluding 16 bits
 * \param  Value value to judge
 * \return resulting minimum size
 * ------------------------------------------------------------------------ */

static tSymbolSize DeduceSize8_32(LongInt Value)
{
  if ((Value >= -128) && (Value <= 127))
    return eSymbolSize8Bit;
  return eSymbolSize32Bit;
}

/*!------------------------------------------------------------------------
 * \fn     ChkSize(LongInt Value, tSymbolSize OpSize)
 * \brief  check whether given value can be represented with given size, assuming sign extension
 * \param  Value value to judge
 * \param  OpSize proposed operand size
 * \return True if value can be represented with this size
 * ------------------------------------------------------------------------ */

static Boolean ChkSize(LongInt Value, tSymbolSize OpSize)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit: return ChkRange(Value, -128, 127);
    case eSymbolSize16Bit: return ChkRange(Value, -32768, 32767);
    default: return True;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DeduceSize16M(LongInt Value)
 * \brief  deduce minimum size to represent memory address, assuming sign-extension
 * \param  Value address to judge
 * \return minimum operand size (8/16/32)
 * ------------------------------------------------------------------------ */

static tSymbolSize DeduceSize16M(LongInt Value)
{
  LongWord TmpVal;

  TmpVal = Value & 0xff;
  if (TmpVal & 0x80) TmpVal |= 0xffffff00;
  if ((TmpVal & 0xffffff) == ((LongWord)Value & 0xffffff))
    return eSymbolSize8Bit;

  TmpVal = Value & 0xffff;
  if (TmpVal & 0x8000) TmpVal |= 0xff0000;
  if ((TmpVal & 0xffffff) == ((LongWord)Value & 0xffffff))
    return eSymbolSize16Bit;

  return eSymbolSize32Bit;
}

/*!------------------------------------------------------------------------
 * \fn     ChkSize16M(LongInt Value, tSymbolSize OpSize)
 * \brief  check whether given memory address can be represented with given size, assuming sign extension
 * \param  Value address to judge
 * \param  OpSize proposed operand size
 * \return True if value can be represented with this size
 * ------------------------------------------------------------------------ */

static Boolean ChkSize16M(LongInt Value, tSymbolSize OpSize)
{
  LongWord TmpVal;
  Boolean OK;

  switch (OpSize)
  {
    case eSymbolSize8Bit:
      TmpVal = Value & 0xff;
      if (TmpVal & 0x80) TmpVal |= 0xffffff00;
      break;
    case eSymbolSize16Bit:
      TmpVal = Value & 0xffff;
      if (TmpVal & 0x8000) TmpVal |= 0xff0000;
      break;
    default:
      TmpVal = Value;
  }
  OK = (Value & 0xffffff) == (TmpVal & 0xffffff);
  if (!OK)
    WrError(ErrNum_OverRange);
  return OK;
}

/*!------------------------------------------------------------------------
 * \fn     DeduceSizePCRel(LongInt AbsAddr, LongInt DispAddr)
 * \brief  deduce minimum size to represent memory address as distance, assuming PC-relative addressing
 * \param  AbsAddr absolute memory addess in question
 * \param  DispAddr address of PC-relative displacement in code
 * \return minimum displacement size (8/16/32)
 * ------------------------------------------------------------------------ */

static tSymbolSize DeduceSizePCRel(LongInt AbsAddr, LongInt DispAddr)
{
  LongInt PCValue, Dist;

  PCValue = DispAddr + 1;
  Dist = PCValue - AbsAddr;
  if ((Dist >= -128) && (Dist <= 127))
    return eSymbolSize8Bit;
  PCValue = DispAddr + 2;
  Dist = PCValue - AbsAddr;
  if ((Dist >= -32768) && (Dist <= 32767))
    return eSymbolSize16Bit;
  return eSymbolSize32Bit;
}

/*!------------------------------------------------------------------------
 * \fn     ChkSizePCRelDisplacement(LongInt *pAbsAddr, tSymbolSize SymbolSize, LongInt DispAddr)
 * \brief  check whether given address can be represented with given size, assuming PC-relative addressing
 * \param  pAbsAddr (in) absolute memory addess in question (out) resulting displacement
 * \param  SymbolSize proposed displacement size
 * \param  DispAddr address of PC-relative displacement in code
 * \return True if value can be represented with this size and displacement was computed
 * ------------------------------------------------------------------------ */

static Boolean ChkSizePCRelDisplacement(LongInt *pAbsAddr, tSymbolSize SymbolSize, LongInt DispAddr)
{
  switch (SymbolSize)
  {
    case eSymbolSize8Bit:
      *pAbsAddr -= DispAddr + 1;
      return ChkRange(*pAbsAddr, -128, 127);
    case eSymbolSize16Bit:
      *pAbsAddr -= DispAddr + 2;
      return ChkRange(*pAbsAddr, -32768, 32767);
    case eSymbolSize32Bit:
      *pAbsAddr -= DispAddr + 4;
      return True;
    default:
      return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ChkSizePCRelBranch(const tStrComp *pArg, LongInt *pAbsAddr, tSymbolSize SymbolSize, tSymbolFlags Flags, LongInt DispAddr)
 * \brief  same as ChkSizePCRel(), but use error messages appropriate for branches
 * \param  pAbsAddr (in) destination address of branch  (out) resulting displacement
 * \param  SymbolSize proposed displacement size
 * \param  Flags symbol flags returned along with pAbsAddr
 * \param  DispAddr address of PC-relative displacement in code
 * \return True if displacement can be represented with this size and was computed
 * ------------------------------------------------------------------------ */

static Boolean ChkSizePCRelBranch(const tStrComp *pArg, LongInt *pAbsAddr, tSymbolSize SymbolSize, tSymbolFlags Flags, LongInt DispAddr)
{
  Boolean OK;

  switch (SymbolSize)
  {
    case eSymbolSize8Bit:
      *pAbsAddr -= DispAddr + 1;
      OK = mSymbolQuestionable(Flags) || RangeCheck(*pAbsAddr, SInt8);
      break;
    case eSymbolSize16Bit:
      *pAbsAddr -= DispAddr + 2;
      OK = mSymbolQuestionable(Flags) || RangeCheck(*pAbsAddr, SInt16);
      break;
    case eSymbolSize32Bit:
      *pAbsAddr -= DispAddr + 4;
      OK = True;
      break;
    default:
      OK = False;
  }
  if (!OK)
    WrStrErrorPos(ErrNum_JmpDistTooBig, pArg);
  return OK;
}

/*!------------------------------------------------------------------------
 * \fn     SetRegPrefix(Byte *pDest, Byte Src, const tStrComp *pArg)
 * \brief  set (new) previous/current bank register prefix
 * \param  pDest current prefix
 * \param  Src new prefix to set/confirm
 * \param  pArg argument to print in error msg if bank mismatch
 * \return True if register bank was set or confirmed
 * ------------------------------------------------------------------------ */

static Boolean SetRegPrefix(Byte *pDest, Byte Src, const tStrComp *pArg)
{
  if (*pDest && (*pDest != Src))
  {
    WrStrErrorPos(ErrNum_RegBankMismatch, pArg);
    return False;
  }
  *pDest = Src;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     EvalArg(const tStrComp *pArg, int Offset, tSymbolSize OpSize, Boolean *pOK)
 * \brief  evaluate integer expression accoring to operand size
 * \param  pArg argument to evaluate
 * \param  Offset offset into argument where expression begins
 * \param  OpSize operand size to use
 * \param  pOK returns True if evaluation succeeded
 * \return result of evaluation
 * ------------------------------------------------------------------------ */

static LongInt EvalArg(const tStrComp *pArg, int Offset, tSymbolSize OpSize, Boolean *pOK)
{
  switch ((int)OpSize)
  {
    case eSymbolSize8Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, Int8, pOK);
    case eSymbolSize16Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, Int16, pOK);
    case eSymbolSize32Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, Int32, pOK);
    case eSymbolSize5Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, UInt5, pOK);
    case eSymbolSize4Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, UInt4, pOK);
    case eSymbolSize3Bit:
      return EvalStrIntExpressionOffs(pArg, Offset, UInt3, pOK);
    default:
      *pOK = False;
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     ValidBaseRegSize(tSymbolSize Size)
 * \brief  check whether operand size is appropriate for base reg in indirect expression
 * \param  Size operand size to check
 * \return True if OK
 * ------------------------------------------------------------------------ */

static Boolean ValidBaseRegSize(tSymbolSize Size)
{
  return (Size == eSymbolSizeUnknown) /* no size given: regarded as 32 bits */
      || (Size == eSymbolSize32Bit);
}

/*!------------------------------------------------------------------------
 * \fn     ValidIndexRegSize(tSymbolSize Size)
 * \brief  check whether operand size is appropriate for indirect reg in indirect expression
 * \param  Size operand size to check
 * \return True if OK
 * ------------------------------------------------------------------------ */

static Boolean ValidIndexRegSize(tSymbolSize Size)
{
  return (Size == eSymbolSizeUnknown) /* no size given: regarded as default index reg size */
      || (Size == eSymbolSize16Bit)
      || (Size == eSymbolSize32Bit);
}

/*!------------------------------------------------------------------------
 * \fn     ClassComp(tStrComp *pArg, tAdrComps *pComps)
 * \brief  classify/parse argument of indirect address expression
 * \param  pComps indirect address evaluation context to update/augment
 * \param  pArg argument to parse
 * \return True if parsing and context update succeeded
 * ------------------------------------------------------------------------ */

static Boolean ClassCompList(tAdrComps *pComps, tStrComp *pArg);

static Boolean ClassComp(tStrComp *pArg, tAdrComps *pComps)
{
  tSymbolSize OpSize;
  Byte Reg, Scale, Prefix;
  char Save;
  LongInt Value;
  Boolean OK;

  KillPrefBlanksStrCompRef(pArg);
  KillPostBlanksStrComp(pArg);

  if (!as_strcasecmp(pArg->str.p_str, "PC"))
  {
    if (pComps->BaseReg == NOREG)
    {
      pComps->BaseReg = PCREG;
      pComps->BaseRegIncr = 0;
      return True;
    }
    else if (pComps->IndexReg == NOREG)
    {
      pComps->IndexReg = pComps->BaseReg;
      pComps->IndexRegScale = 0;
      pComps->IndexRegSize = eSymbolSize32Bit;
      pComps->BaseReg = PCREG;
      pComps->BaseRegIncr = 0;
      return True;
    }
    else
      return False;
  }
  if (*pArg->str.p_str == '@')
  {
    tAdrComps InnerComps;
    tStrComp InnerList;

    if (pComps->InnerReg != NOREG)
      return False;

    StrCompRefRight(&InnerList, pArg, 1);
    if (!ClassCompList(&InnerComps, &InnerList))
      return False;
    if (!InnerComps.InnerDispPresent
      && (InnerComps.InnerReg == NOREG)
      && (InnerComps.IndexReg == NOREG)
      && (InnerComps.BaseReg != NOREG)
      && !InnerComps.BaseRegIncr)
    {
      pComps->InnerReg = InnerComps.BaseReg;
      pComps->InnerDispPresent = InnerComps.OuterDispPresent;
      pComps->InnerDisp = InnerComps.OuterDisp;
      pComps->InnerDispSize = InnerComps.OuterDispSize;
    }

    return True;
  }
  switch (DecodeRegWithSize(pArg, &Reg, &OpSize, &Prefix, False))
  {
    case eIsReg:
      if (!SetRegPrefix(&pComps->RegPrefix, Prefix, pArg))
        return False;
      if ((pComps->BaseReg == NOREG) && (OpSize == eSymbolSizeUnknown))
      {
        pComps->BaseReg = Reg;
        pComps->BaseRegIncr = 0;
        return True;
      }
      else if ((pComps->IndexReg == NOREG) && ValidIndexRegSize(OpSize))
      {
        pComps->IndexReg = Reg;
        pComps->IndexRegScale = 0;
        if (OpSize != eSymbolSizeUnknown)
          pComps->IndexRegSize = OpSize;
        return True;
      }
      else
        return False;
    case eIsNoReg:
      break;
    case eRegAbort:
      return False;
  }
  if (*pArg->str.p_str == '-')
  {
    tStrComp RegComp;

    StrCompRefRight(&RegComp, pArg, 1);
    switch (DecodeReg(&RegComp, &Reg, &Prefix, True, False))
    {
      case eIsReg:
        if (!SetRegPrefix(&pComps->RegPrefix, Prefix, pArg))
          return False;
        if (pComps->BaseReg == NOREG)
        {
          pComps->BaseReg = Reg;
          pComps->BaseRegIncr = -1;
          return True;
        }
        else
          return False;
      case eRegAbort:
        return False;
      case eIsNoReg:
        break;
    }
  }
  if (pArg->str.p_str[pArg->Pos.Len - 1] == '+')
  {
    pArg->str.p_str[pArg->Pos.Len - 1] = '\0';
    switch (DecodeReg(pArg, &Reg, &Prefix, True, False))
    {
      case eIsReg:
        if (!SetRegPrefix(&pComps->RegPrefix, Prefix, pArg))
          return False;
        if (pComps->BaseReg == NOREG)
        {
          pComps->BaseReg = Reg;
          pComps->BaseRegIncr = +1;
          return True;
        }
        else
          return False;
      case eRegAbort:
        return False;
      case eIsNoReg:
        pArg->str.p_str[pArg->Pos.Len - 1] = '+';
        break;
    }
  }
  Save = SplitScale(pArg, &Scale);
  if (Save)
  {
    switch (DecodeRegWithSize(pArg, &Reg, &OpSize, &Prefix, False))
    {
      case eIsReg:
        if (!SetRegPrefix(&pComps->RegPrefix, Prefix, pArg))
          return False;
        if ((pComps->IndexReg == NOREG) && ValidIndexRegSize(OpSize))
        {
          pComps->IndexReg = Reg;
          pComps->IndexRegScale = Scale;
          if (OpSize != eSymbolSizeUnknown)
            pComps->IndexRegSize = OpSize;
        }
        else if (pComps->BaseReg == NOREG)
        {
          if ((Scale == 0) && ValidBaseRegSize(OpSize))
            pComps->BaseReg = Reg;
          else if ((Scale != 0) && (pComps->IndexRegScale == 0) && (pComps->IndexRegSize == eSymbolSize32Bit))
          {
            pComps->BaseReg = pComps->IndexReg;
            pComps->IndexReg = Reg;
            pComps->IndexRegScale = Scale;
            pComps->IndexRegSize = (OpSize == eSymbolSizeUnknown) ? DefIndexRegSize : OpSize;
          }
          else
            return False;
        }
        else
          return False;
        return True;
      case eIsNoReg:
        AppendScale(pArg, Scale);
        break;
      case eRegAbort:
        return False;
    }
  }

  if (!SplitOpSize(pArg, &OpSize))
    OpSize = eSymbolSizeUnknown;
  Value = EvalArg(pArg, 0, eSymbolSize32Bit, &OK);
  if (!OK)
    return False;
  pComps->OuterDispPresent = True;
  if (OpSize > pComps->OuterDispSize)
    pComps->OuterDispSize = OpSize;
  pComps->OuterDisp += Value;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ClassCompList(tAdrComps *pComps, tStrComp *pArg)
 * \brief  classify/parse argument list of indirect address expression
 * \param  pComps indirect address evaluation context to fill
 * \param  pArg address expression argument
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean ClassCompList(tAdrComps *pComps, tStrComp *pArg)
{
  ResetAdrComps(pComps);

  if (IsIndirect(pArg->str.p_str))
  {
    char *pSplit;
    tStrComp Remainder;

    StrCompIncRefLeft(pArg, 1);
    StrCompShorten(pArg, 1);
    do
    {
      pSplit = QuotPos(pArg->str.p_str, ',');
      if (pSplit)
        StrCompSplitRef(pArg, &Remainder, pArg, pSplit);
      if (!ClassComp(pArg, pComps))
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pArg);
        return False;
      }
      if (pSplit)
        *pArg = Remainder;
    }
    while (pSplit);
  }
  else
  {
    if (!ClassComp(pArg, pComps))
    {
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return False;
    }
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, unsigned Mask, tAdrVals *pAdrVals, LongInt EAAddr)
 * \brief  parse address expression
 * \param  pArg expression to parse
 * \param  Mask bit mask of allowed addressing modes
 * \param  pAdrVals returns parsed addressing mode
 * \param  EAAddr position of associared EA byte in code (needed for PC-relative addressing)
 * \return True if parsing succeeded
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr(const tStrComp *pArg, unsigned Mask, tAdrVals *pAdrVals, LongInt EAAddr)
{
  Byte Reg, Prefix;
  tStrComp Arg;

  ResetAdrVals(pAdrVals);

  StrCompRefRight(&Arg, pArg, 0);
  while (True)
  {
    if (!as_strncasecmp(Arg.str.p_str, "<CRn>", 5)
     || !as_strncasecmp(Arg.str.p_str, "<PRn>", 5))
    {
      AppendAdrVals(pAdrVals, (toupper(Arg.str.p_str[1]) == 'C') ? PREFIX_CRn : PREFIX_PRn);
      StrCompIncRefLeft(&Arg, 5);
      KillPrefBlanksStrCompRef(&Arg);
    }
    else
      break;
  }

  switch (DecodeReg(&Arg, &Reg, &Prefix, True, False))
  {
    case eIsReg:
      pAdrVals->Mode = eAdrModeReg;
      AppendRegPrefix(pAdrVals, Prefix);
      AppendAdrVals(pAdrVals, 0x40 | Reg);
      goto Check;
    case eIsNoReg:
      break;
    case eRegAbort:
      return False;
  }

  if (*Arg.str.p_str == '#')
  {
    tSymbolSize ImmOpSize;
    LongInt ArgVal;
    Boolean OK;

    if (!SplitOpSize(&Arg, &ImmOpSize))
      ImmOpSize = OpSize;
    ArgVal = EvalArg(&Arg, 1, ImmOpSize, &OK);
    if (OK)
    {
      AppendAdrVals(pAdrVals, 0x70 | (ImmOpSize + 1));
      AppendToAdrValsBySize(pAdrVals, ArgVal, ImmOpSize);
      pAdrVals->Mode = eAdrModeImm;
    }
    goto Check;
  }

  if (*Arg.str.p_str == '@')
  {
    tStrComp IndirComp;
    tAdrComps Comps;

    StrCompRefRight(&IndirComp, &Arg, 1);
    if (!ClassCompList(&Comps, &IndirComp))
      return False;

    /* only base register, no indirection, and optional displacement: */

    if (!Comps.InnerDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.IndexReg == NOREG)
     && NormalReg(Comps.BaseReg)
     && !Comps.BaseRegIncr)
    {
      if (Comps.OuterDispPresent)
      {
        if (Comps.OuterDispSize == eSymbolSizeUnknown)
          Comps.OuterDispSize = DeduceSize(Comps.OuterDisp);
        if (!ChkSize(Comps.OuterDisp, Comps.OuterDispSize))
          goto Check;
        AppendRegPrefix(pAdrVals, Comps.RegPrefix);
        AppendAdrVals(pAdrVals, ((Comps.OuterDispSize + 1) << 4) | Comps.BaseReg);
        AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      }
      else
        AppendAdrVals(pAdrVals, 0x00 | Comps.BaseReg);
      pAdrVals->Mode = eAdrModeIReg;
      goto Check;
    }

    /* postinc/predec */

    if (!Comps.InnerDispPresent
     && !Comps.OuterDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.IndexReg == NOREG)
     && NormalReg(Comps.BaseReg)
     && Comps.BaseRegIncr)
    {
      if (Comps.BaseRegIncr > 0)
      {
        AppendRegPrefix(pAdrVals, Comps.RegPrefix);
        AppendAdrVals(pAdrVals, 0x50 | Comps.BaseReg);
        pAdrVals->Mode = eAdrModePost;
      }
      else
      {
        AppendRegPrefix(pAdrVals, Comps.RegPrefix);
        AppendAdrVals(pAdrVals, 0x60 | Comps.BaseReg);
        pAdrVals->Mode = eAdrModePre;
      }
      goto Check;
    }

    /* absolute */

    if (!Comps.InnerDispPresent
     && Comps.OuterDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.IndexReg == NOREG)
     && (Comps.BaseReg == NOREG))
    {
      if (Comps.OuterDispSize == eSymbolSizeUnknown)
        Comps.OuterDispSize = DeduceSize16M(Comps.OuterDisp);
      if (!ChkSize16M(Comps.OuterDisp, Comps.OuterDispSize))
        goto Check;
      AppendAdrVals(pAdrVals, 0x74 | (Comps.OuterDispSize + 1));
      AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModeAbs;
      goto Check;
    }

    /* register indirect with scale */

    if (!Comps.InnerDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.BaseReg == NOREG)
     && NormalReg(Comps.IndexReg)
     && (Comps.IndexRegSize == eSymbolSize32Bit))
    {
      Byte EAVal = 0x78;

      if (Comps.OuterDispPresent)
      {
        if (Comps.OuterDispSize == eSymbolSizeUnknown)
          Comps.OuterDispSize = DeduceSize(Comps.OuterDisp);
        if (!ChkSize(Comps.OuterDisp, Comps.OuterDispSize))
          goto Check;
        EAVal |= (1 + Comps.OuterDispSize);
      }
      AppendRegPrefix(pAdrVals, Comps.RegPrefix);
      AppendAdrVals(pAdrVals, EAVal);
      AppendAdrVals(pAdrVals, (Comps.IndexRegScale << 4) | Comps.IndexReg);
      if (Comps.OuterDispPresent)
        AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModeIRegScale;
      goto Check;
    }

    /* register indirect with (scaled) index */

    if (!Comps.InnerDispPresent
     && (Comps.InnerReg == NOREG)
     && NormalReg(Comps.BaseReg)
     && NormalReg(Comps.IndexReg))
    {
      Byte Exp1Val = ((Comps.IndexRegSize - 1) << 6) | Comps.BaseReg;

      if (Comps.OuterDispPresent)
      {
        if (Comps.OuterDispSize == eSymbolSizeUnknown)
          Comps.OuterDispSize = DeduceSize(Comps.OuterDisp);
        if (!ChkSize(Comps.OuterDisp, Comps.OuterDispSize))
          goto Check;
        Exp1Val |= (1 + Comps.OuterDispSize) << 4;
      }
      AppendRegPrefix(pAdrVals, Comps.RegPrefix);
      AppendAdrVals(pAdrVals, 0x7c);
      AppendAdrVals(pAdrVals, Exp1Val);
      AppendAdrVals(pAdrVals, (Comps.IndexRegScale << 4) | Comps.IndexReg);
      if (Comps.OuterDispPresent)
        AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModeIdxScale;
      goto Check;
    }

    /* PC-relative with (scaled) index */

    if (!Comps.InnerDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.BaseReg == PCREG)
     && NormalReg(Comps.IndexReg))
    {
      Byte Exp1Val = ((Comps.IndexRegSize - 1) << 6);

      if (Comps.OuterDispPresent)
      {
        if (Comps.OuterDispSize == eSymbolSizeUnknown)
          Comps.OuterDispSize = DeduceSizePCRel(Comps.OuterDisp, EAAddr + 3);
        if (!ChkSizePCRelDisplacement(&Comps.OuterDisp, Comps.OuterDispSize, EAAddr + 3))
          goto Check;
        Exp1Val |= (1 + Comps.OuterDispSize) << 4;
      }
      AppendRegPrefix(pAdrVals, Comps.RegPrefix);
      AppendAdrVals(pAdrVals, 0x7d);
      AppendAdrVals(pAdrVals, Exp1Val);
      AppendAdrVals(pAdrVals, (Comps.IndexRegScale << 4) | Comps.IndexReg);
      if (Comps.OuterDispPresent)
        AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModePCIdxScale;
      goto Check;
    }

    /* PC-relative */

    if (!Comps.InnerDispPresent
     && (Comps.InnerReg == NOREG)
     && (Comps.BaseReg == PCREG)
     && (Comps.IndexReg == NOREG))
    {
      Byte Exp1Val = 0x80;

      if (Comps.OuterDispPresent)
      {
        if (Comps.OuterDispSize == eSymbolSizeUnknown)
          Comps.OuterDispSize = DeduceSizePCRel(Comps.OuterDisp, EAAddr + 2);
        if (!ChkSizePCRelDisplacement(&Comps.OuterDisp, Comps.OuterDispSize, EAAddr + 2))
          goto Check;
        Exp1Val |= (1 + Comps.OuterDispSize) << 4;
      }
      AppendRegPrefix(pAdrVals, Comps.RegPrefix); /* should never occur, no Rn in expression */
      AppendAdrVals(pAdrVals, 0x7d);
      AppendAdrVals(pAdrVals, Exp1Val);
      if (Comps.OuterDispPresent)
        AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModePCRel;
      goto Check;
    }

    /* Register double indirect */

    if (NormalReg(Comps.InnerReg)
     && (Comps.BaseReg == NOREG)
     && (Comps.IndexReg == NOREG))
    {
      /* no disp not allowed -> convert zero-disp to 8 bits */

      if (Comps.OuterDispSize == eSymbolSizeUnknown)
        Comps.OuterDispSize = DeduceSize8_32(Comps.OuterDisp);
      if (Comps.InnerDispSize == eSymbolSizeUnknown)
        Comps.InnerDispSize = DeduceSize8_32(Comps.InnerDisp);
      AppendRegPrefix(pAdrVals, Comps.RegPrefix);
      AppendAdrVals(pAdrVals, 0x7e);
      AppendAdrVals(pAdrVals, Comps.InnerReg | ((Comps.InnerDispSize + 1) << 4) | ((Comps.OuterDispSize + 1) << 6));
      AppendToAdrValsBySize(pAdrVals, Comps.InnerDisp, Comps.InnerDispSize);
      AppendToAdrValsBySize(pAdrVals, Comps.OuterDisp, Comps.OuterDispSize);
      pAdrVals->Mode = eAdrModeDoubleIndir;
      goto Check;
    }

    WrStrErrorPos(ErrNum_InvAddrMode, &Arg);
  }

  /* None of this.  Should we treat it as absolute? */

  WrStrErrorPos(ErrNum_InvAddrMode, pArg);

Check:
  if ((pAdrVals->Mode != eAdrModeNone)
   && !((Mask >> pAdrVals->Mode) & 1))
  {
    WrStrErrorPos(ErrNum_InvAddrMode, pArg);
    ResetAdrVals(pAdrVals);
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     GlobalBankReg(const tAdrVals *pAdrVals)
 * \brief  check whether parsed address expression is a global bank register
 * \param  pAdrVals parsed expression
 * \return True if it is a global bank register
 * ------------------------------------------------------------------------ */

static Boolean GlobalBankReg(const tAdrVals *pAdrVals)
{
  return (pAdrVals->Mode == eAdrModeReg)
      && (pAdrVals->Cnt == 1);
}

/*!------------------------------------------------------------------------
 * \fn     ImmValS8(const tAdrVals *pAdrVals)
 * \brief  check whether parsed address expression is an immediate value representable as 8 bits,
           assuming sign extension
 * \param  pAdrVals parsed expression
 * \return True if immediate & value range OK
 * ------------------------------------------------------------------------ */

static Boolean ImmValS8(const tAdrVals *pAdrVals)
{
  Byte Sign;
  unsigned z;

  if ((pAdrVals->Mode != eAdrModeImm)
   || (pAdrVals->Cnt < 1))
    return False;

  Sign = (pAdrVals->Vals[pAdrVals->Cnt - 1] & 0x80) ? 0xff: 0x00;
  for (z = 1; z < pAdrVals->Cnt - 1; z++)
    if (pAdrVals->Vals[z] != Sign)
      return False;

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ImmValS4(const tAdrVals *pAdrVals)
 * \brief  check whether parsed address expression is an immediate value representable as 4 bits,
           assuming sign extension
 * \param  pAdrVals parsed expression
 * \return True if immediate & value range OK
 * ------------------------------------------------------------------------ */

static Boolean ImmValS4(const tAdrVals *pAdrVals)
{
  Byte Sign;

  if (!ImmValS8(pAdrVals))
    return False;

  Sign = (pAdrVals->Vals[pAdrVals->Cnt - 1] & 0x08) ? 0xf0: 0x00;
  return (pAdrVals->Vals[pAdrVals->Cnt - 1] & 0xf0) == Sign;
}

/*!------------------------------------------------------------------------
 * \fn     AppendAdrValsToCode(const tAdrVals *pAdrVals)
 * \brief  append parsed address expression to instruction's code
 * \param  pAdrVals parsed expression
 * ------------------------------------------------------------------------ */

static void AppendAdrValsToCode(const tAdrVals *pAdrVals)
{
  memcpy(&BAsmCode[CodeLen], pAdrVals->Vals, pAdrVals->Cnt);
  CodeLen += pAdrVals->Cnt;
}

/*!------------------------------------------------------------------------
 * \fn     AppendAdrValPair(tAdrVals *pSrcAdrVals, const tAdrVals *pDestAdrVals)
 * \brief  append source & destination address expression to instruction's code
 * \param  pSrcAdrVals parsed source address expression
 * \param  pDestAdrVals parsed destination address expression
 * ------------------------------------------------------------------------ */

static void AppendAdrValPair(tAdrVals *pSrcAdrVals, const tAdrVals *pDestAdrVals)
{
  Byte *pEASrc = &BAsmCode[CodeLen];

  AppendAdrValsToCode(pSrcAdrVals);
  if (pDestAdrVals->Vals[0] == 0x40) /* EA=0x40 -> R0 -> accumulator mode */
    *pEASrc |= 0x80;
  else
    AppendAdrValsToCode(pDestAdrVals);
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits and bit fields in symbol table:
 * bits 0..2/3/4: (start) bit position
 * bits 3/4/5...26/27/28: register address
 * bits 30/31: register size (0/1/2 for 8/16/32 bits)
 */

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const char *pBitArg, Boolean *pOK, tSymbolSize OpSize)
 * \brief  evaluate constant bit position, with bit range depending on operand size
 * \param  pBitArg bit position argument
 * \param  pOK returns True if OK
 * \param  OpSize operand size (0,1,2 -> 8,16,32 bits)
 * \return bit position as number
 * ------------------------------------------------------------------------ */

static Byte EvalBitPosition(const tStrComp *pBitArg, Boolean *pOK, tSymbolSize OpSize)
{
  int Offset = !!(*pBitArg->str.p_str == '#');

  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return EvalStrIntExpressionOffs(pBitArg, Offset, UInt3, pOK);
    case eSymbolSize16Bit:
      return EvalStrIntExpressionOffs(pBitArg, Offset, UInt4, pOK);
    case eSymbolSize32Bit:
      return EvalStrIntExpressionOffs(pBitArg, Offset, UInt5, pOK);
    default:
      WrError(ErrNum_InvOpSize);
      *pOK = False;
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, tSymbolSize OpSize, LongWord Address)
 * \brief  build the compact internal representation of a bit field symbol
 * \param  BitPos bit position in word
 * \param  Width width of bit field
 * \param  OpSize operand size (8/16/32 bit)
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, tSymbolSize OpSize, LongWord Address)
{
  int AddrShift = 3 + OpSize;

  return BitPos
       | ((Address & 0xfffffful) << AddrShift)
       | ((LongWord)OpSize << 30);
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, LongWord *pAddress, Byte *pBitPos, tSymbolSize *pOpSize)
 * \brief  transform compact represenation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \param  pOpSize returns register size (0/1/2 for 8/16/32 bits)
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, LongWord *pAddress, Byte *pBitPos, tSymbolSize *pOpSize)
{
  *pOpSize = (tSymbolSize)((BitSymbol >> 30) & 3);
  switch (*pOpSize)
  {
    case eSymbolSize8Bit:
      *pAddress = (BitSymbol >> 3) & 0xfffffful;
      *pBitPos = BitSymbol & 7;
      break;
    case eSymbolSize16Bit:
      *pAddress = (BitSymbol >> 4) & 0xfffffful;
      *pBitPos = BitSymbol & 15;
      break;
    case eSymbolSize32Bit:
      *pAddress = (BitSymbol >> 5) & 0xfffffful;
      *pBitPos = BitSymbol & 31;
      break;
    default:
      *pAddress = 0;
      *pBitPos = 0;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_H16(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_H16(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  LongWord Address;
  tSymbolSize OpSize;
  char Attribute;

  DissectBitSymbol(Inp, &Address, &BitPos, &OpSize);
  Attribute = "bwl"[OpSize];

  as_snprintf(pDest, DestSize, "$%lx(%c).%u", (unsigned long)Address, Attribute, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandH16Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandH16Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
{
  tSymbolSize OpSize = (pStructElem->OpSize < 0) ? eSymbolSize8Bit : pStructElem->OpSize;
  LongWord Address = Base + pStructElem->Offset;

  if (!ChkRange(Address, 0, 0xfffffful)
   || !ChkRange(pStructElem->BitPos, 0, (8 << OpSize) - 1))
    return;

  PushLocHandle(-1);
  EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, OpSize, Address), SegBData, False);
  PopLocHandle();
  /* TODO: MakeUseList? */
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, tSymbolSize OpSize)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \param  OpSize register size (0/1/2 = 8/16/32 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, tSymbolSize OpSize)
{
  Boolean OK;
  LongWord Addr;
  Byte BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK, OpSize);
  if (!OK)
    return False;

  Addr = EvalStrIntExpressionOffs(pRegArg, !!(*pRegArg->str.p_str == '@'), UInt24, &OK);
  if (!OK)
    return False;

  *pResult = AssembleBitSymbol(BitPos, OpSize, Addr);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \param  OpSize register size (0/1/2 = 8/16/32 bit)
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

  /* Register & bit position are given as separate arguments:
     use same argument positions as for machine instructions */

  else if (Stop == Start + 1)
    return DecodeBitArg2(pResult, &ArgStr[Stop], &ArgStr[Start], OpSize);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeStringReg(Word Code)
 * \brief  decode register argument for string instruction
 * \param  pComp argument to parse
 * \param  Mask bit mask of allowed addressing modes
 * \param  pResult returns (global) register number
 * ------------------------------------------------------------------------ */

static Boolean DecodeStringReg(tStrComp *pComp, unsigned Mask, Byte *pResult)
{
  tAdrVals AdrVals;

  if (!DecodeAdr(pComp, Mask, &AdrVals, EProgCounter() + 2))
    return False;
  switch (AdrVals.Mode)
  {
    case eAdrModeReg:
      if (!GlobalBankReg(&AdrVals))
      {
        WrStrErrorPos(ErrNum_InvReg, pComp);
        return False;
      }
      break;
    case eAdrModeIReg:
      if (AdrVals.Cnt != 1)
      {
        WrStrErrorPos(ErrNum_InvAddrMode, pComp);
        return False;
      }
      break;
    case eAdrModePost:
    case eAdrModePre:
      break;
    default:
      return False;
  }
  *pResult = AdrVals.Vals[0] & 15;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ExtractDisp(const Byte *pVals, Byte SizeCode)
 * \brief  extract variable-length signed value
 * \param  pVals storage of value
 * \param  SizeCode # of bytes used (0..3 for 0/1/2/4 bytes)
 * \return effective value
 * ------------------------------------------------------------------------ */

static LongInt ExtractDisp(const Byte *pVals, Byte SizeCode)
{
  LongInt Result = 0;

  switch (SizeCode)
  {
    case 3:
      Result = (Result << 8) | (*pVals++);
      Result = (Result << 8) | (*pVals++);
      Result = (Result << 8) | (*pVals++);
      Result = (Result << 8) | (*pVals++);
      break;
    case 2:
      Result = (Result << 8) | (*pVals++);
      Result = (Result << 8) | (*pVals++);
      if (Result & 0x8000ul)
        Result -= 0x10000ul;
      break;
    case 1:
      Result = (Result << 8) | (*pVals++);
      if (Result & 0x80ul)
        Result -= 0x100;
      break;
    default:
      Result = 0;
  }
  return Result;
}

/*!------------------------------------------------------------------------
 * \fn     PCDistOK(const tAdrVals *pAdrVals, int Delta)
 * \brief  if expression is PC-relative, check whether change of distance does not change displacement size
 * \param  pAdrVals parsed address expression
 * \param  Delta displacement change
 * ------------------------------------------------------------------------ */

static Boolean PCDistOK(tAdrVals *pAdrVals, int Delta)
{
  Byte DispSizeCode;
  LongInt NewDisp;
  int DispOffs;
  tSymbolSize DispSize;

  /* only relevant for EA values that use PC-relative addressing: */

  if (pAdrVals->Vals[0] != 0x7d)
    return True;
  DispOffs = (pAdrVals->Vals[1] & 0x80) ? 2 : 3;

  /* extract displacement size Sd */

  DispSizeCode = (pAdrVals->Vals[1] >> 4) & 3;
  DispSize = (tSymbolSize)(DispSizeCode - 1);

  /* compute new displacement after correction */

  NewDisp = ExtractDisp(pAdrVals->Vals + DispOffs, DispSizeCode) + Delta;

  /* still fits size? */

  if (DeduceSize(NewDisp) != DispSize)
    return False;

  AppendBySize(pAdrVals->Vals + DispOffs, NewDisp, DispSize);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     NegSignedValOK(tAdrVals *pAdrVals)
 * \brief  tests whether negated (8 bit) immediate value still fits size
 * \param  pAdrVals parsed address expression
 * \return True if successfully inverted
 * ------------------------------------------------------------------------ */

static Boolean NegSigned8ValOK(tAdrVals *pAdrVals)
{
  Byte DispSizeCode;
  LongInt Value;
  tSymbolSize DispSize;

  /* only treats immediate argument: */

  if (pAdrVals->Mode != eAdrModeImm)
    return False;

  DispSizeCode = pAdrVals->Vals[0] & 3;
  if (!DispSizeCode)
    return False;
  DispSize = (tSymbolSize)(DispSizeCode - 1);

  /* extract immediate value */

  Value = ExtractDisp(pAdrVals->Vals + 1, DispSizeCode) * (-1);

  /* still fits size? */

  if (DeduceSize(Value) > eSymbolSize8Bit)
    return False;

  AppendBySize(pAdrVals->Vals + 1, Value, DispSize);
  return True;
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV_ADD_SUB_CMP(Word Code)
 * \brief  instruction parser for MOV, ADD, CMP and SUB
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOV_ADD_SUB_CMP(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  tFormat Format;
  unsigned DestMask;
  Boolean IsSUB = (Code == 0x04),
          IsCMP = (Code == 0x08);

  if (!ChkOpSize(eSymbolSize16Bit, 7))
    return;

  /* Immediate is allowed as destination for CMP since the result is
     anyway discarded: */

  DestMask = MModeAll;
  if (!IsCMP)
    DestMask &= ~MModeImm;

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], DestMask, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    /* TODO: auto-convert SUB:Q n,<ea> to ADD:q -n,<ea>? */

    if (!*FormatPart.str.p_str)
    {
      /* R format (all instructions) */

      if (GlobalBankReg(&SrcAdrVals) && GlobalBankReg(&DestAdrVals))
        Format = eFormatR;

      /* MOV R0,<ea> -> MOVF <ea> if PC-relative distance size does not change. */

      else if (GlobalBankReg(&SrcAdrVals) && (SrcAdrVals.Vals[0] == 0x40) && PCDistOK(&DestAdrVals, SrcAdrVals.Cnt))
        Format = eFormatF;
      else if (ImmValS4(&SrcAdrVals) && GlobalBankReg(&DestAdrVals))
        Format = eFormatRQ;

      /* SUB:Q #n,<ea> does not exist.  Convert to ADD:Q #-n,<ea> if possible..
         TODO: should not do this at all if immediate size >= 16 bit was forced.  */

      else if (ImmValS8(&SrcAdrVals) && (!IsSUB || NegSigned8ValOK(&SrcAdrVals)))
      {
        if (IsSUB)
        {
          Code = 0x00;
          IsSUB = False;
        }
        Format = eFormatQ;
      }
      else
        Format = eFormatG;
    }
    else if (!(Format = DecodeFormat((1 << eFormatG) | (1 << eFormatQ) | (1 << eFormatR) | (1 << eFormatRQ))))
    {
      WrStrErrorPos(ErrNum_InvFormat, &FormatPart);
      return;
    }
    switch (Format)
    {
      case eFormatG:
        BAsmCode[CodeLen++] = 0x00 | Code | OpSize;
        AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
        break;
      case eFormatR:
        if (!GlobalBankReg(&SrcAdrVals)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else if (!GlobalBankReg(&DestAdrVals)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          BAsmCode[CodeLen++] = 0x20 | Code | OpSize;
          BAsmCode[CodeLen++] = ((SrcAdrVals.Vals[0] & 15) << 4) | (DestAdrVals.Vals[0] & 15);
        }
        break;
      case eFormatRQ:
        if (SrcAdrVals.Mode != eAdrModeImm) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else if (!ImmValS4(&SrcAdrVals)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
        else if (!GlobalBankReg(&DestAdrVals)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        else
        {
          BAsmCode[CodeLen++] = 0x30 | Code | OpSize;
          BAsmCode[CodeLen++] = ((DestAdrVals.Vals[0] & 15) << 4) | (SrcAdrVals.Vals[SrcAdrVals.Cnt - 1] & 15);
        }
        break;
      case eFormatQ:
        if (IsSUB) WrStrErrorPos(ErrNum_InvAddrMode, &AttrPart);
        else if (SrcAdrVals.Mode != eAdrModeImm) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else if (!ImmValS8(&SrcAdrVals)) WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
        else
        {
          BAsmCode[CodeLen++] = 0x10 | Code | OpSize;
          BAsmCode[CodeLen++] = SrcAdrVals.Vals[SrcAdrVals.Cnt - 1];
          AppendAdrValsToCode(&DestAdrVals);
        }
        break;
      case eFormatF:
        if (!GlobalBankReg(&SrcAdrVals) || (SrcAdrVals.Vals[0] != 0x40)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
        else
        {
          BAsmCode[CodeLen++] = 0x5c | OpSize;
          AppendAdrValsToCode(&DestAdrVals);
        }
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSignExt(Word Code)
 * \brief  instruction parser for MOVS, ADDS, CMPS and SUBS
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeSignExt(Word Code)
{
  tSymbolSize SrcOpSize;
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean IsCMPS = (Code == 0x48);
  unsigned DestMask;

  /* Immediate is allowed as destination for CMPS since the result is
     anyway discarded: */

  DestMask = MModeAll;
  if (!IsCMPS)
    DestMask &= ~MModeImm;

  if (!ChkEmptyFormat()
   || !ChkOpSize(eSymbolSize16Bit, 7)
   || !ChkArgCnt(2, 2)
   || !DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 1))
    return;
  SrcOpSize = OpSize;
  OpSize = eSymbolSize32Bit;
  if (DecodeAdr(&ArgStr[2], DestMask, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | SrcOpSize;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTwo(Word Code)
 * \brief  instruction parser for generic two-operand instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeTwo(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Boolean Only8 = (Code & 3) == 3;

  if (ChkEmptyFormat()
   && ChkOpSize(Only8 ? eSymbolSize8Bit : eSymbolSize16Bit, Only8 ? 1 : 7)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeOne(Word Code)
 * \brief  instruction parser for generic one-operand instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeOne(Word Code)
{
  tAdrVals AdrVals;
  Boolean Only8 = !!(Code & 3);
  unsigned Mask;

  Mask = MModeAll;
  if (Code != 0x58) /* TST */
    Mask &= ~MModeImm;

  if (ChkEmptyFormat()
   && ChkOpSize(Only8 ? eSymbolSize8Bit : eSymbolSize16Bit, Only8 ? 1 : 7)
   && ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], Mask, &AdrVals, EProgCounter() + 1))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValsToCode(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMUL_DIV(Word Code)
 * \brief  instruction parser for multiply and divide instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMUL_DIV(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (!ChkEmptyFormat()
   || !ChkOpSize(eSymbolSize16Bit, 3)
   || !ChkArgCnt(2, 2)
   || !DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 2))
    return;
  OpSize++;
  if (!DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &DestAdrVals, EProgCounter() + 2 + SrcAdrVals.Cnt))
    return;
  OpSize--;

  BAsmCode[CodeLen++] = 0xee | (Code & 0x01);
  BAsmCode[CodeLen++] = (Code & 0x70) | (OpSize << 4);
  AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEXT(Word Code)
 * \brief  instruction parser for EXTS/EXTU
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeEXT(Word Code)
{
  Byte Reg, Prefix;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 7)
   && ChkArgCnt(1, 1))
  {
    if (DecodeReg(&ArgStr[1], &Reg, &Prefix, False, True))
    {
      OpSize = (OpSize == eSymbolSize8Bit) ? eSymbolSize32Bit : (OpSize - 1);
      BAsmCode[CodeLen++] = Code | OpSize;
      BAsmCode[CodeLen++] = Reg;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCInstr(Word Code)
 * \brief  instruction decoder for control-register related instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeCInstr(Word Code)
{
  Byte CReg;
  unsigned Mask = MModeAll;
  int CIdx = 2, EAIdx = 1;
  tAdrVals AdrVals;

  if (0xfc == Code) /* STC */
  {
    CIdx = 1;
    EAIdx = 2;
    Mask &= ~MModeImm;
  }

  if (ChkEmptyFormat()
   && ChkArgCnt(2, 2)
   && DecodeCReg(&ArgStr[CIdx], &CReg)
   && DecodeAdr(&ArgStr[EAIdx], Mask, &AdrVals, EProgCounter() + 2))
  {
    BAsmCode[CodeLen++] = Code;
    BAsmCode[CodeLen++] = CReg;
    AppendAdrValsToCode(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBranch(Word Code)
 * \brief  instruction decoder for non-generic branch instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBranch(Word Code)
{
  LongInt Addr;
  Boolean OK;
  tSymbolFlags Flags;

  if (!ChkEmptyFormat()
   || !ChkArgCnt(1, 1))
    return;

  Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '@'), UInt24, &OK, &Flags);
  if (!OK)
    return;

  if (OpSize == eSymbolSizeUnknown)
    OpSize = DeduceSizePCRel(Addr, EProgCounter() + 1);
  if (!ChkSizePCRelBranch(&ArgStr[1], &Addr, OpSize, Flags, EProgCounter() + 1))
    return;

  BAsmCode[CodeLen++] = Code | OpSize;
  CodeLen += AppendBySize(BAsmCode + CodeLen, Addr, OpSize);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBranchGen(Word Code)
 * \brief  instruction decoder for generic branch instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBranchGen(Word Code)
{
  LongInt Addr;
  Boolean OK;
  tSymbolFlags Flags;
  tFormat Format;

  if (!ChkArgCnt(1, 1))
    return;

  if (!*FormatPart.str.p_str)
  {
    if (Code == 7)
    {
      DecodeBranch(0xa0);
      return;
    }
    if (Code == 6)
    {
      DecodeBranch(0xb0);
      return;
    }
  }
  else
  {
    Format = DecodeFormat(1 << eFormatG);
    if (!Format)
    {
      WrStrErrorPos(ErrNum_InvFormat, &FormatPart);
      return;
    }
  }

  Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '@'), UInt24, &OK, &Flags);
  if (!OK)
    return;

  if (OpSize == eSymbolSizeUnknown)
    OpSize = DeduceSizePCRel(Addr, EProgCounter() + 2);
  if (!ChkSizePCRelBranch(&ArgStr[1], &Addr, OpSize, Flags, EProgCounter() + 2))
    return;

  BAsmCode[CodeLen++] = 0xa4 | OpSize;
  BAsmCode[CodeLen++] = Code;
  CodeLen += AppendBySize(BAsmCode + CodeLen, Addr, OpSize);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  instruction decoder for instructions without argument
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Code)
{
  if (ChkEmptyFormat()
   && (!(Code & 0x8000) || CheckSup(&OpPart))
   && ChkArgCnt(0, 0))
    BAsmCode[CodeLen++] = Lo(Code);
}

/*!------------------------------------------------------------------------
 * \fn     Decode(Word Code)
 * \brief  instruction decoder for shift & rotate instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeShift(Word Code)
{
  tAdrVals ShiftAdrVals, OpAdrVals;
  tSymbolSize SaveOpSize;

  if (!ChkEmptyFormat()
   || !ChkOpSize(eSymbolSize16Bit, 7)
   || !ChkArgCnt(2, 2)
   || !DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &OpAdrVals, EProgCounter() + 2))
    return;
  SaveOpSize = OpSize;

  OpSize = eSymbolSize5Bit;
  (void)DecodeAdr(&ArgStr[1], MModeImm | MModeReg, &ShiftAdrVals, EProgCounter() + 1);
  switch (ShiftAdrVals.Mode)
  {
    case eAdrModeReg:
      if (ShiftAdrVals.Cnt > 1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
      {
        BAsmCode[CodeLen++] = 0x60 | SaveOpSize;
        BAsmCode[CodeLen++] = Code | (ShiftAdrVals.Vals[0] & 15);
        goto common;
      }
      break;
    case eAdrModeImm:
      BAsmCode[CodeLen++] = 0x64 | SaveOpSize;
      BAsmCode[CodeLen++] = Code | (ShiftAdrVals.Vals[1] & 31);
      goto common;
    default:
      break;
    common:
      AppendAdrValsToCode(&OpAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSET(Word Code)
 * \brief  instruction decoder for SET/cc instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeSET(Word Code)
{
  tAdrVals AdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize8Bit, 1)
   && ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeImm, &AdrVals, EProgCounter() + 2))
  {
    BAsmCode[CodeLen++] = 0xb7;
    BAsmCode[CodeLen++] = Code;
    AppendAdrValsToCode(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTRAP(Word Code)
 * \brief  instruction decoder for TRAP/cc instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeTRAP(Word Code)
{
  if (ChkEmptyFormat()
   && ChkArgCnt(0, 0))
  {
    BAsmCode[CodeLen++] = 0xf3;
    BAsmCode[CodeLen++] = Code;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSCB(Word Code)
 * \brief  instruction decoder for SCB/cc instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeSCB(Word Code)
{
  LongInt Addr;
  Boolean OK;
  tSymbolFlags Flags;
  Byte Reg, Prefix;

  if (!ChkEmptyFormat()
   || !ChkArgCnt(2, 2))
    return;

  if (!DecodeReg(&ArgStr[1], &Reg, &Prefix, False, True))
    return;

  Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], !!(*ArgStr[1].str.p_str == '@'), UInt24, &OK, &Flags);
  if (!OK)
    return;

  if (OpSize == eSymbolSizeUnknown)
    OpSize = DeduceSizePCRel(Addr, EProgCounter() + 2);
  if (!ChkSizePCRelBranch(&ArgStr[2], &Addr, OpSize, Flags, EProgCounter() + 2))
    return;

  BAsmCode[CodeLen++] = 0xb4 | OpSize;
  BAsmCode[CodeLen++] = (Reg << 4) | Code;
  CodeLen += AppendBySize(BAsmCode + CodeLen, Addr, OpSize);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJMP_JSR(Word Code)
 * \brief  instruction decoder for JMP and JSR instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeJMP_JSR(Word Code)
{
  tAdrVals Arg;

  if (ChkEmptyFormat()
   && ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeReg - MModeImm, &Arg, EProgCounter() + 1))
  {
    BAsmCode[CodeLen++] = Code;
    AppendAdrValsToCode(&Arg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeSWAP(Word Code)
 * \brief  instruction decoder for SWAP instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeSWAP(Word Code)
{
  tAdrVals Arg;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 3)
   && ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeImm, &Arg, EProgCounter() + 1))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValsToCode(&Arg);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeXCH(Word Code)
 * \brief  instruction decoder for XCH instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeXCH(Word Code)
{
  Byte RegX, RegY, PrefixX, PrefixY;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize32Bit, 4)
   && ChkArgCnt(2, 2))
  {
    if (DecodeReg(&ArgStr[1], &RegX, &PrefixX, False, True)
     && DecodeReg(&ArgStr[2], &RegY, &PrefixY, False, True))
    {
      BAsmCode[CodeLen++] = Code;
      BAsmCode[CodeLen++] = (RegX << 4) | RegY;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLINK(Word Code)
 * \brief  instruction decoder for LINK instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeLINK(Word Code)
{
  Byte Reg, Prefix;

  if (!ChkEmptyFormat()
   || !ChkArgCnt(2, 2))
    return;
  if (!DecodeReg(&ArgStr[1], &Reg, &Prefix, False, True));
  else if (*ArgStr[2].str.p_str != '#') WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[2]);
  else
  {
    Boolean OK;
    LongInt Val = EvalArg(&ArgStr[2], 1, (OpSize == eSymbolSizeUnknown) ? eSymbolSize32Bit : OpSize, &OK);

    if (OK)
    {
      if (OpSize == eSymbolSizeUnknown)
        OpSize = DeduceSize(Val);
      BAsmCode[CodeLen++] = Code | OpSize;
      BAsmCode[CodeLen++] = Reg;
      CodeLen += AppendBySize(BAsmCode + CodeLen, Val, OpSize);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeUNLK(Word Code)
 * \brief  instruction decoder for UNLK instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeUNLK(Word Code)
{
  Byte Reg, Prefix;

  if (!ChkEmptyFormat()
   || !ChkArgCnt(1, 1))
    return;
  if (DecodeReg(&ArgStr[1], &Reg, &Prefix, False, True))
  {
    BAsmCode[CodeLen++] = Code;
    BAsmCode[CodeLen++] = Reg;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeTRAPA(Word Code)
 * \brief  instruction decoder for TRAPA instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeTRAPA(Word Code)
{
  if (!ChkEmptyFormat()
   || !ChkArgCnt(1, 1))
    return;
  if (*ArgStr[1].str.p_str != '#') WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[1]);
  else
  {
    Boolean OK;
    Byte Val = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt4, &OK);

    if (OK)
    {
      BAsmCode[CodeLen++] = Code;
      BAsmCode[CodeLen++] = Val;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRTD(Word Code)
 * \brief  instruction decoder for RTD instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeRTD(Word Code)
{
  if (!ChkEmptyFormat()
   || !ChkArgCnt(1, 1))
    return;
  else if (*ArgStr[1].str.p_str != '#') WrStrErrorPos(ErrNum_OnlyImmAddr, &ArgStr[1]);
  else
  {
    Boolean OK;
    LongInt Val = EvalArg(&ArgStr[1], 1, (OpSize == eSymbolSizeUnknown) ? eSymbolSize32Bit : OpSize, &OK);

    if (OK)
    {
      if (OpSize == eSymbolSizeUnknown)
        OpSize = DeduceSize(Val);
      BAsmCode[CodeLen++] = Code | OpSize;
      CodeLen += AppendBySize(BAsmCode + CodeLen, Val, OpSize);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDM_STM(Word Code)
 * \brief  instruction decoder for LDM and STM instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeLDM_STM(Word Code)
{
  Word RegList;
  tAdrVals AdrVals;
  Boolean IsLDM = !!(Code == 0x74);

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 7)
   && ChkArgCnt(2, ArgCntMax)
   && DecodeRegList(&RegList, 1 + IsLDM, ArgCnt - 1 + IsLDM)
   && DecodeAdr(&ArgStr[IsLDM ? 1 : ArgCnt], MModeAll - MModeImm - MModeReg, &AdrVals, EProgCounter() + 1))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValsToCode(&AdrVals);
    BAsmCode[CodeLen++] = Hi(RegList);
    BAsmCode[CodeLen++] = Lo(RegList);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCGBN(Word Code)
 * \brief  instruction decoder for CGBN instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeCGBN(Word Code)
{
  tAdrVals AdrVals;

  if (ChkEmptyFormat()
   && CheckSup(&OpPart)
   && ChkOpSize(eSymbolSize8Bit, 1)
   && ChkArgCnt(1, ArgCntMax)
   && DecodeAdr(&ArgStr[1], MModeReg | MModeImm, &AdrVals, EProgCounter() + 1))
  {
    Word RegList = 0;

    if (ArgCnt >= 2)
    {
      if (!DecodeRegList(&RegList, 2, ArgCnt))
        return;
    }
    BAsmCode[CodeLen++] = Code | ((AdrVals.Mode == eAdrModeImm) ? 2 : 0) | !!RegList;
    BAsmCode[CodeLen++] = (AdrVals.Mode == eAdrModeImm) ? AdrVals.Vals[1] : (AdrVals.Vals[0] & 0x0f);
    if (RegList)
    {
      BAsmCode[CodeLen++] = Hi(RegList);
      BAsmCode[CodeLen++] = Lo(RegList);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePGBN(Word Code)
 * \brief  instruction decoder for PGBN instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodePGBN(Word Code)
{
  if (ChkEmptyFormat()
   && CheckSup(&OpPart)
   && ChkOpSize(eSymbolSize8Bit, 1)
   && ChkArgCnt(0, ArgCntMax))
  {
    Word RegList = 0;

    if (ArgCnt >= 1)
    {
      if (!DecodeRegList(&RegList, 1, ArgCnt))
        return;
    }
    BAsmCode[CodeLen++] = Code | !!RegList;
    if (RegList)
    {
      BAsmCode[CodeLen++] = Hi(RegList);
      BAsmCode[CodeLen++] = Lo(RegList);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVFP(Word Code)
 * \brief  instruction decoder for MOVFP instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOVFP(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 6)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeReg - MModePre - MModePost - MModeImm, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | (OpSize - 1);
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVFPE(Word Code)
 * \brief  instruction decoder for MOVFPE instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOVFPE(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 7)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeReg - MModePre - MModePost - MModeImm, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVTP(Word Code)
 * \brief  instruction decoder for MOVTP instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOVTP(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 6)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeReg - MModePre - MModePost - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | (OpSize - 1);
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVTPE(Word Code)
 * \brief  instruction decoder for MOVTPE instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOVTPE(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 7)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeReg - MModePre - MModePost - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code | OpSize;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOVA(Word Code)
 * \brief  instruction decoder for MOVA instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeMOVA(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize32Bit, 4)
   && ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModeAll - MModeReg - MModeImm, &SrcAdrVals, EProgCounter() + 1)
   && DecodeAdr(&ArgStr[2], MModeAll - MModeImm, &DestAdrVals, EProgCounter() + 1 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeString(Word Code)
 * \brief  decode string instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeString(Word Code)
{
  int ExpectArgCnt = (Code & 0x80) ? 4 : 3;
  unsigned SrcMask, DestMask = MModeReg | ((Code & 0x40) ? MModePre : MModePost);
  Byte SrcReg, DestReg, CntReg, FinalReg, Prefix;

  SrcMask = MModeReg;
  if (Code & 0x30) /* SCMP/SMOV */
    SrcMask |= (Code & 0x40) ? MModePre : MModePost;
  else if (Code & 0x80) /* SSCH */
    SrcMask |= MModeIReg;

  if (ChkEmptyFormat()
   && ChkOpSize(eSymbolSize16Bit, 3)
   && ChkArgCnt(ExpectArgCnt, ExpectArgCnt)
   && DecodeStringReg(&ArgStr[1], SrcMask, &SrcReg)
   && DecodeStringReg(&ArgStr[2], DestMask, &DestReg))
  {
    if (DecodeReg(&ArgStr[3], &CntReg, &Prefix, False, True))
    {
      if (4 == ExpectArgCnt)
      {
        if (!DecodeReg(&ArgStr[4], &FinalReg, &Prefix, False, True))
          return;
      }
      BAsmCode[CodeLen++] = 0x94 | OpSize;
      BAsmCode[CodeLen++] = Code | CntReg;
      BAsmCode[CodeLen++] = (SrcReg << 4) | DestReg;
      if (4 == ExpectArgCnt)
        BAsmCode[CodeLen++] = (FinalReg << 4) | Hi(Code);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBField(Word Code)
 * \brief  decode bit field instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBField(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Byte PosReg, CntReg;

  if (OpSize != eSymbolSizeUnknown) WrError (ErrNum_InvOpSize);
  else if (ChkEmptyFormat()
        && ChkArgCnt(4, 4)
        && DecodeStringReg(&ArgStr[1], MModeReg, &PosReg)
        && DecodeStringReg(&ArgStr[2], MModeReg, &CntReg)
        && DecodeAdr(&ArgStr[3], MModeAll - MModePre - MModePost - MModeImm, &SrcAdrVals, EProgCounter() + 2)
        && DecodeAdr(&ArgStr[4], MModeAll - MModePre - MModePost - MModeImm, &DestAdrVals, EProgCounter() + 2 + SrcAdrVals.Cnt))
  {
    BAsmCode[CodeLen++] = Code;
    BAsmCode[CodeLen++] = (PosReg << 4) | CntReg;
    AppendAdrValPair(&SrcAdrVals, &DestAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBFMOV(Word Code)
 * \brief  decode bit field move instruction which is different from other bit field instructions
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBFMOV(Word Code)
{
  Byte XReg, BReg, YReg, OReg, SReg, DReg;

  if (OpSize != eSymbolSizeUnknown) WrError (ErrNum_InvOpSize);
  else if (ChkEmptyFormat()
        && ChkArgCnt(6, 6)
        && DecodeStringReg(&ArgStr[1], MModeReg, &XReg)
        && DecodeStringReg(&ArgStr[2], MModeReg, &BReg)
        && DecodeStringReg(&ArgStr[3], MModeReg, &YReg)
        && DecodeStringReg(&ArgStr[4], MModeReg, &OReg)
        && DecodeStringReg(&ArgStr[5], MModeReg | MModeIReg , &SReg)
        && DecodeStringReg(&ArgStr[6], MModeReg | MModeIReg , &DReg))
  {
    BAsmCode[CodeLen++] = Code;
    BAsmCode[CodeLen++] = (XReg << 4) | BReg;
    BAsmCode[CodeLen++] = (YReg << 4) | OReg;
    BAsmCode[CodeLen++] = (SReg << 4) | DReg;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBit(Word Code)
 * \brief  decode CPU bit handling instruction
 * \param  Code specific instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBit(Word Code)
{
  tSymbolSize ThisOpSize;

  if (!ChkEmptyFormat()
   || !ChkArgCnt(1, 2))
    return;

  /* bit "object": */

  if (1 == ArgCnt)
  {
    LongWord BitArg, Address;
    Byte ImmPos;

    if (DecodeBitArg(&BitArg, 1, 1, OpSize)
     && DissectBitSymbol(BitArg, &Address, &ImmPos, &ThisOpSize)
     && SetOpSize(ThisOpSize))
    {
      tSymbolSize AddressSize = DeduceSize16M(Address);

      /* SetOpSize() has set OpSize to 0..2 or confirmed it, no need to check for invalid size */

      BAsmCode[CodeLen++] = 0x6c | OpSize;
      BAsmCode[CodeLen++] = Code | ImmPos;
      BAsmCode[CodeLen++] = 0x74 | (AddressSize + 1);
      CodeLen += AppendBySize(BAsmCode + CodeLen, Address, AddressSize);
    }
  }

  /* bit + <ea> */

  else
  {
    tAdrVals EAAdrVals, BitAdrVals;
    unsigned Mask = MModeAll - ((Code == 0x60) ? 0 : MModeImm);

    if (ChkOpSize(eSymbolSize16Bit, 7)
     && DecodeAdr(&ArgStr[2], Mask, &EAAdrVals, EProgCounter() + 2))
    {
      ThisOpSize = OpSize;
      OpSize = eSymbolSize3Bit + OpSize;
      if (DecodeAdr(&ArgStr[1], MModeImm | MModeReg, &BitAdrVals, EProgCounter() + 1))
      {
        switch (BitAdrVals.Mode)
        {
          case eAdrModeReg:
            if (!GlobalBankReg(&BitAdrVals))
            {
              WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
              return;
            }
            BAsmCode[CodeLen++] = 0x64 | ThisOpSize;
            BAsmCode[CodeLen++] = Code | (BitAdrVals.Vals[0] & 15);
            break;
          case eAdrModeImm:
            BAsmCode[CodeLen++] = 0x6c | ThisOpSize;
            BAsmCode[CodeLen++] = Code | BitAdrVals.Vals[1];
            break;
          default:
            return;
        }
        AppendAdrValsToCode(&EAAdrVals);
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBIT(Word Code)
 * \brief  decode bit declaration instruction
 * ------------------------------------------------------------------------ */

static void DecodeBIT(Word Code)
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
    BitPos = EvalBitPosition(&ArgStr[1], &OK, (OpSize == eSymbolSizeUnknown) ? eSymbolSize32Bit : OpSize);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[2].str.p_str);
    /* undefined op size -> take over from ref elem */
    pElement->OpSize = OpSize;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandH16Bit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (!ChkOpSize(eSymbolSize16Bit, 7))
      return;

    if (DecodeBitArg(&BitSpec, 1, ArgCnt, OpSize))
    {
      *ListLine = '=';
      DissectBit_H16(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     AddCondition(const char *pName, Word Code)
 * \brief  register condition to instruction hash table
 * \param  pName name of condition
 * \param  Code binary coding of condition
 * ------------------------------------------------------------------------ */

static void AddCondition(const char *pName, Word Code)
{
  char Name[20];

  as_snprintf(Name, sizeof(Name), "B%s", pName);
  AddInstTable(InstTable, Name, Code, DecodeBranchGen);
  as_snprintf(Name, sizeof(Name), "SET/%s", pName);
  AddInstTable(InstTable, Name, Code, DecodeSET);
  as_snprintf(Name, sizeof(Name), "TRAP/%s", pName);
  AddInstTable(InstTable, Name, Code, DecodeTRAP);
  as_snprintf(Name, sizeof(Name), "SCB/%s", pName);
  AddInstTable(InstTable, Name, Code, DecodeSCB);
  as_snprintf(Name, sizeof(Name), "SCMP/%s/F", pName);
  AddInstTable(InstTable, Name, 0x00a0 | (Code << 8), DecodeString);
  as_snprintf(Name, sizeof(Name), "SCMP/%s/B", pName);
  AddInstTable(InstTable, Name, 0x00d0 | (Code << 8), DecodeString);
  as_snprintf(Name, sizeof(Name), "SSCH/%s/F", pName);
  AddInstTable(InstTable, Name, 0x0080 | (Code << 8), DecodeString);
  as_snprintf(Name, sizeof(Name), "SSCH/%s/B", pName);
  /* per-instr. description gives 000 for both SSCH variants, taken from Figure 16-9: */
  AddInstTable(InstTable, Name, 0x00c0 | (Code << 8), DecodeString);
}

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  build up instruction hash table
 * ------------------------------------------------------------------------ */

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "MOV", 0x0c, DecodeMOV_ADD_SUB_CMP);
  AddInstTable(InstTable, "ADD", 0x00, DecodeMOV_ADD_SUB_CMP);
  AddInstTable(InstTable, "SUB", 0x04, DecodeMOV_ADD_SUB_CMP);
  AddInstTable(InstTable, "CMP", 0x08, DecodeMOV_ADD_SUB_CMP);

  AddInstTable(InstTable, "MOVS", 0x4c, DecodeSignExt);
  AddInstTable(InstTable, "ADDS", 0x40, DecodeSignExt);
  AddInstTable(InstTable, "SUBS", 0x44, DecodeSignExt);
  AddInstTable(InstTable, "CMPS", 0x48, DecodeSignExt);

  AddInstTable(InstTable, "ADDX", 0x50, DecodeTwo);
  AddInstTable(InstTable, "SUBX", 0x54, DecodeTwo);
  AddInstTable(InstTable, "AND" , 0x80, DecodeTwo);
  AddInstTable(InstTable, "OR"  , 0x88, DecodeTwo);
  AddInstTable(InstTable, "XOR" , 0x84, DecodeTwo);
  AddInstTable(InstTable, "DADD", 0x83, DecodeTwo);
  AddInstTable(InstTable, "DSUB", 0x87, DecodeTwo);

  AddInstTable(InstTable, "MULXS" , 0x00, DecodeMUL_DIV);
  AddInstTable(InstTable, "MULXU" , 0x40, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIVXS" , 0x01, DecodeMUL_DIV);
  AddInstTable(InstTable, "DIVXU" , 0x41, DecodeMUL_DIV);

  AddInstTable(InstTable, "NEG"  , 0x8c, DecodeOne);
  AddInstTable(InstTable, "NEGX" , 0x9c, DecodeOne);
  AddInstTable(InstTable, "NOT"  , 0x90, DecodeOne);
  AddInstTable(InstTable, "TST"  , 0x58, DecodeOne);
  AddInstTable(InstTable, "CLR"  , 0x14, DecodeOne);
  AddInstTable(InstTable, "DNEG" , 0xaf, DecodeOne);
  AddInstTable(InstTable, "TAS"  , 0xee, DecodeOne);
  AddInstTable(InstTable, "MOVF" , 0x5c, DecodeOne);

  AddInstTable(InstTable, "EXTS" , 0xbc, DecodeEXT);
  AddInstTable(InstTable, "EXTU" , 0xac, DecodeEXT);

  AddInstTable(InstTable, "ANDC" , 0xf8, DecodeCInstr);
  AddInstTable(InstTable, "ORC"  , 0xf9, DecodeCInstr);
  AddInstTable(InstTable, "XORC" , 0xfa, DecodeCInstr);
  AddInstTable(InstTable, "LDC"  , 0xfb, DecodeCInstr);
  AddInstTable(InstTable, "STC"  , 0xfc, DecodeCInstr);

  AddInstTable(InstTable, "BRA"  , 0x98, DecodeBranch);
  AddInstTable(InstTable, "BSR"  , 0xa8, DecodeBranch);
  AddCondition("CC" , 0x04);
  AddCondition("HS" , 0x04);
  AddCondition("CS" , 0x05);
  AddCondition("LO" , 0x05);
  AddCondition("NE" , 0x06);
  AddCondition("EQ" , 0x07);
  AddCondition("GE" , 0x0c);
  AddCondition("LT" , 0x0d);
  AddCondition("GT" , 0x0e);
  AddCondition("LE" , 0x0f);
  AddCondition("HI" , 0x02);
  AddCondition("LS" , 0x03);
  AddCondition("PL" , 0x0a);
  AddCondition("MI" , 0x0b);
  AddCondition("VC" , 0x08);
  AddCondition("VS" , 0x09);
  AddCondition("T"  , 0x00);
  AddCondition("F"  , 0x01);

  AddInstTable(InstTable, "RTS"  , 0x00bb, DecodeFixed);
  AddInstTable(InstTable, "RESET", 0x80f0, DecodeFixed);
  AddInstTable(InstTable, "RTE"  , 0x80f1, DecodeFixed);
  AddInstTable(InstTable, "RTR"  , 0x00f4, DecodeFixed);
  AddInstTable(InstTable, "SLEEP", 0x80f5, DecodeFixed);
  AddInstTable(InstTable, "NOP"  , 0x00ff, DecodeFixed);
  AddInstTable(InstTable, "DCBN" , 0x00fe, DecodeFixed);
  AddInstTable(InstTable, "ICBN" , 0x00fd, DecodeFixed);

  AddInstTable(InstTable, "SHAR"  , 0 << 5, DecodeShift);
  AddInstTable(InstTable, "SHLR"  , 1 << 5, DecodeShift);
  AddInstTable(InstTable, "ROTR"  , 2 << 5, DecodeShift);
  AddInstTable(InstTable, "ROTXR" , 3 << 5, DecodeShift);
  AddInstTable(InstTable, "SHAL"  , 4 << 5, DecodeShift);
  AddInstTable(InstTable, "SHLL"  , 5 << 5, DecodeShift);
  AddInstTable(InstTable, "ROTL"  , 6 << 5, DecodeShift);
  AddInstTable(InstTable, "ROTXL" , 7 << 5, DecodeShift);

  AddInstTable(InstTable, "JMP" , 0x9b, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR" , 0xab, DecodeJMP_JSR);
  AddInstTable(InstTable, "SWAP", 0xea, DecodeSWAP);
  AddInstTable(InstTable, "XCH" , 0xb3, DecodeXCH);

  AddInstTable(InstTable, "LINK", 0xd0, DecodeLINK);
  AddInstTable(InstTable, "UNLK", 0xd3, DecodeUNLK);
  AddInstTable(InstTable, "TRAPA",0xf2, DecodeTRAPA);
  AddInstTable(InstTable, "RTD" , 0xb8, DecodeRTD);

  AddInstTable(InstTable, "LDM" , 0x74, DecodeLDM_STM);
  AddInstTable(InstTable, "STM" , 0x70, DecodeLDM_STM);
  AddInstTable(InstTable, "CGBN", 0xe4, DecodeCGBN);
  AddInstTable(InstTable, "PGBN", 0xe8, DecodePGBN);

  AddInstTable(InstTable, "MOVFP",0xe2, DecodeMOVFP);
  AddInstTable(InstTable, "MOVFPE",0x7c, DecodeMOVFPE);
  AddInstTable(InstTable, "MOVTP",0xe0, DecodeMOVTP);
  AddInstTable(InstTable, "MOVTPE",0x78, DecodeMOVTPE);
  AddInstTable(InstTable, "MOVA", 0xbf, DecodeMOVA);

  AddInstTable(InstTable, "SMOV/F", 0x0020, DecodeString);
  AddInstTable(InstTable, "SMOV/B", 0x0050, DecodeString);
  AddInstTable(InstTable, "SSTR/F", 0x0000, DecodeString);
  AddInstTable(InstTable, "SSTR/B", 0x0040, DecodeString);

  AddInstTable(InstTable, "BFEXT", 0xd4, DecodeBField);
  AddInstTable(InstTable, "BFINS", 0xd5, DecodeBField);
  AddInstTable(InstTable, "BFSCH", 0xd6, DecodeBField);
  AddInstTable(InstTable, "BFMOV", 0xd7, DecodeBFMOV);

  AddInstTable(InstTable, "BCLR", 0x40, DecodeBit);
  AddInstTable(InstTable, "BNOT", 0x20, DecodeBit);
  AddInstTable(InstTable, "BSET", 0x00, DecodeBit);
  AddInstTable(InstTable, "BTST", 0x60, DecodeBit);

  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "REG", 0, CodeREG);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  tear down instruction hash table
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     MakeCode_H16(void)
 * \brief  general entry point to parse machine instructions
 * ------------------------------------------------------------------------ */

static Boolean DecodeAttrPart_H16(void)
{
  tStrComp SizePart;
  char *p;
  static char EmptyStr[] = "";

  /* split off format and operand size */

  switch (AttrSplit)
  {
    case '.':
      p = strchr(AttrPart.str.p_str, ':');
      if (p)
        StrCompSplitRef(&SizePart, &FormatPart, &AttrPart, p);
      else
      {
        StrCompRefRight(&SizePart, &AttrPart, 0);
        StrCompMkTemp(&FormatPart, EmptyStr, 0);
      }
      break;
    case ':':
      p = strchr(AttrPart.str.p_str, '.');
      if (p)
        StrCompSplitRef(&FormatPart, &SizePart, &AttrPart, p);
      else
      {
        StrCompRefRight(&FormatPart, &AttrPart, 0);
        StrCompMkTemp(&SizePart, EmptyStr, 0);
      }
      break;
    default:
      StrCompMkTemp(&FormatPart, EmptyStr, 0);
      StrCompMkTemp(&SizePart, EmptyStr, 0);
      break;
  }

  /* process operand size part of attribute */

  if (*SizePart.str.p_str)
  {
    if (!DecodeMoto16AttrSizeStr(&SizePart, &AttrPartOpSize[0], False))
      return False;
  }
  return True;
}

static void MakeCode_H16(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  OpSize = AttrPartOpSize[0];
  if (DecodeMoto16Pseudo(OpSize, True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_H16(void)
 * \brief  instruction that uses up label field?
 * \return true if label field shall not be stored as label
 * ------------------------------------------------------------------------ */

static Boolean IsDef_H16(void)
{
  return Memo("BIT") || Memo("REG");
}

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_H16(char *pArg, TempResult *pResult)
 * \brief  handle built-in symbols on H16
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_H16(char *pArg, TempResult *pResult)
{
  Byte Reg, Prefix;

  if (DecodeRegCore(pArg, &Reg, &Prefix))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize32Bit;
    pResult->Contents.RegDescr.Reg = ((Word)Prefix) << 8 | Reg;
    pResult->Contents.RegDescr.Dissect = DissectReg_H16;
    pResult->Contents.RegDescr.compare = compare_reg_h16;
  }
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_H16(void)
 * \brief  things to do when switching to H16 as target
 * ------------------------------------------------------------------------ */

static void SwitchTo_H16(void)
{
  const TFamilyDescr *pDescr = FindFamilyByName("H16");

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = pDescr->Id;
  NOPCode = 0xff;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".:";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffffff;

  DecodeAttrPart = DecodeAttrPart_H16;
  MakeCode = MakeCode_H16;
  IsDef = IsDef_H16;
  SwitchFrom = DeinitFields;
  DissectBit = DissectBit_H16;
  DissectReg = DissectReg_H16;
  InternSymbol = InternSymbol_H16;
  InitFields();

  onoff_supmode_add();

  /* H16 code is byte-oriented, so no padding by default */

  AddMoto16PseudoONOFF(False);
}

/*!------------------------------------------------------------------------
 * \fn     codeh16_init(void)
 * \brief  register H16 to upper layers as target
 * ------------------------------------------------------------------------ */

void codeh16_init(void)
{
  CPU641016 = AddCPU("HD641016", SwitchTo_H16);
}
