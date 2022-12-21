/* codest7.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator SGS-Thomson ST7/STM8                                        */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "bpemu.h"
#include "strutil.h"
#include "nls.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "asmallg.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codest7.h"

typedef enum
{
  eModNone = -1,
  eModImm = 0,        /* #byte */
  eModAbs8,           /* shortmem */
  eModAbs16,          /* longmem */
  eModAbs24,          /* extmem */
  eModIX,             /* (X) */
  eModIX8,            /* (shortoff,X) */
  eModIX16,           /* (longoff,X) */
  eModIX24,           /* (extoff,X) */
  eModIY,             /* (Y) */
  eModIY8,            /* (shortoff,Y) */
  eModIY16,           /* (longoff,Y) */
  eModIY24,           /* (extoff,Y) */
  eModISP8,           /* (shortoff,SP) (STM8 only) */
  eModIAbs8,          /* [shortptr.b] (ST7 only) */
  eModIAbs16,         /* [shortptr.w] */
  eModI16Abs16,       /* [longptr.w] (STM8 only) */
  eModI16Abs24,       /* [longptr.e] (STM8 only) */
  eModIXAbs8,         /* ([shortptr.b],X) */
  eModIXAbs16,        /* ([<longptr.w],X) */
  eModI16XAbs16,      /* ([>longptr.w],X) (STM8 only) */
  eModI16XAbs24,      /* ([>longptr.e],X) (STM8 only) */
  eModIYAbs8,         /* ([shortptr.b],Y) */
  eModIYAbs16,        /* ([<longptr.w],Y) */
  eModI16YAbs24,      /* ([>longptr.e],Y) (STM8 only) */
  eModA,              /* A */
  eModX,              /* X */
  eModXL,             /* XL (STM8 only) */
  eModXH,             /* XH (STM8 only) */
  eModY,              /* Y */
  eModYL,             /* YL (STM8 only) */
  eModYH,             /* YH (STM8 only) */
  eModS               /* SP */
  /* bit mask is full, 32 modes! */
} tAdrMode;

#define MModImm (1ul << eModImm)
#define MModAbs8 (1ul << eModAbs8)
#define MModAbs16 (1ul << eModAbs16)
#define MModAbs24 (1ul << eModAbs24)
#define MModIX (1ul << eModIX)
#define MModIX8 (1ul << eModIX8)
#define MModIX16 (1ul << eModIX16)
#define MModIX24 (1ul << eModIX24)
#define MModIY (1ul << eModIY)
#define MModIY8 (1ul << eModIY8)
#define MModIY16 (1ul << eModIY16)
#define MModIY24 (1ul << eModIY24)
#define MModISP8 (1ul << eModISP8)
#define MModIAbs8 (1ul << eModIAbs8)
#define MModIAbs16 (1ul << eModIAbs16)
#define MModI16Abs16 (1ul << eModI16Abs16)
#define MModI16Abs24 (1ul << eModI16Abs24)
#define MModIXAbs8 (1ul << eModIXAbs8)
#define MModIXAbs16 (1ul << eModIXAbs16)
#define MModI16XAbs16 (1ul << eModI16XAbs16)
#define MModI16XAbs24 (1ul << eModI16XAbs24)
#define MModIYAbs8 (1ul << eModIYAbs8)
#define MModIYAbs16 (1ul << eModIYAbs16)
#define MModI16YAbs24 (1ul << eModI16YAbs24)
#define MModA (1ul << eModA)
#define MModX (1ul << eModX)
#define MModXL (1ul << eModXL)
#define MModXH (1ul << eModXH)
#define MModY (1ul << eModY)
#define MModYL (1ul << eModYL)
#define MModYH (1ul << eModYH)
#define MModS (1ul << eModS)

typedef enum
{
  eCoreST7,
  eCoreSTM8
} tCPUCore;

typedef struct
{
  const char *pName;
  IntType AddrIntType;
  tCPUCore Core;
} tCPUProps;

typedef struct
{
  tAdrMode Mode;
  Byte Part;
  unsigned Cnt;
  Byte Vals[3];
} tAdrVals;

static const tCPUProps *pCurrCPUProps;
static tSymbolSize OpSize;
static Byte PrefixCnt;

/*--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     ResetAdrVals(tAdrVals *pAdrVals)
 * \brief  clear AdrVals structure
 * \param  pAdrVals struct to clear/reset
 * ------------------------------------------------------------------------ */

static void ResetAdrVals(tAdrVals *pAdrVals)
{
  pAdrVals->Mode = eModNone;
  pAdrVals->Part = 0;
  pAdrVals->Cnt = 0;
}

static void FillAdrVals(tAdrVals *pAdrVals, LongWord Value, tSymbolSize Size)
{
  pAdrVals->Cnt = 0;
  switch (Size)
  {
    case eSymbolSize24Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = (Value >> 16) & 255;
      /* fall-through */
    case eSymbolSize16Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = (Value >> 8) & 255;
      /* fall-through */
    case eSymbolSize8Bit:
      pAdrVals->Vals[pAdrVals->Cnt++] = Value & 255;
      /* fall-through */
    default:
      break;
  }
}

static void ExtendAdrVals(tAdrVals *pAdrVals)
{
  switch (pAdrVals->Mode)
  {
    case eModAbs8:
      pAdrVals->Mode = eModAbs16;
      pAdrVals->Vals[1] = pAdrVals->Vals[0];
      pAdrVals->Vals[0] = 0;
      pAdrVals->Cnt = 2;
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     AddPrefix(Byte Pref)
 * \brief  add another prefix byte
 * \param  Pref prefix to add
 * ------------------------------------------------------------------------ */

static void AddPrefix(Byte Pref)
{
  BAsmCode[PrefixCnt++] = Pref;
}

/*!------------------------------------------------------------------------
 * \fn     ModeInMask(LongWord Mask, tAdrMode Mode)
 * \brief  check whether certain addressing mode is set in mask
 * \param  Mask list of allowed modes
 * \param  Mode addressing mode to check
 * ------------------------------------------------------------------------ */

static Boolean ModeInMask(LongWord Mask, tAdrMode Mode)
{
  return !!((Mask >> Mode) & 1);
}

/*!------------------------------------------------------------------------
 * \fn     CutSizeSuffix(tStrComp *pArg)
 * \brief  cut off possible size suffix (.b .w .e) from argument
 * \param  pArg argument
 * \return deduced size or unknown if no known suffix
 * ------------------------------------------------------------------------ */

static tSymbolSize CutSizeSuffix(const tStrComp *pArg)
{
  int l = strlen(pArg->str.p_str);

  if ((l >= 3) && (pArg->str.p_str[l - 2] == '.'))
  {
    switch (as_toupper(pArg->str.p_str[l - 1]))
    {
      case 'B':
        pArg->str.p_str[l - 2] = '\0';
        return eSymbolSize8Bit;
      case 'W':
        pArg->str.p_str[l - 2] = '\0';
        return eSymbolSize16Bit;
      case 'E':
        pArg->str.p_str[l - 2] = '\0';
        return eSymbolSize24Bit;
      default:
        break;
    }
  }
  return eSymbolSizeUnknown;
}

/*!------------------------------------------------------------------------
 * \fn     DecideSize(LongWord Mask, const tStrComp *pArg, tAdrMode Mode8, tAdrMode Mode16, tAdrMode Mode24, Byte Part8, Byte Part16, Byte Part24, Boolean IsCode, tAdrVals *pAdrVals)
 * \brief  decide about length of absolute or indexed operand
 * \param  Mask bit mask of allowed modes
 * \param  pArg address argument
 * \param  Mode8 AdrMode for 8-bit address/displacement
 * \param  Mode16 AdrMode for 16-bit address/displacement
 * \param  Mode24 AdrMode for 24-bit address/displacement
 * \param  Part8 AdrPart for 8-bit address/displacement
 * \param  Part16 AdrPart for 16-bit address/displacement
 * \param  Part24 AdrPart for 24-bit address/displacement
 * \param  IsCode Address is in code space (check for same page)
 * \param  pAdrVals destination to fill out
 * ------------------------------------------------------------------------ */

static void DecideSize(LongWord Mask, const tStrComp *pArg, tAdrMode Mode8, tAdrMode Mode16, tAdrMode Mode24, Byte Part8, Byte Part16, Byte Part24, Boolean IsCode, tAdrVals *pAdrVals)
{
  tSymbolSize Size = eSymbolSizeUnknown;
  IntType SizeType;
  LongWord Value;
  Boolean OK;
  tSymbolFlags Flags;

  if ((Mode8 != eModNone) && !ModeInMask(Mask, Mode8))
    Mode8 = eModNone;
  if ((Mode16 != eModNone) && !ModeInMask(Mask, Mode16))
    Mode16 = eModNone;
  if ((Mode24 != eModNone) && !ModeInMask(Mask, Mode24))
    Mode24 = eModNone;

  Size = CutSizeSuffix(pArg);
  switch (Size)
  {
    case eSymbolSize8Bit:
      if (Mode8 == eModNone)
        goto InvSize;
      break;
    case eSymbolSize16Bit:
      if (Mode16 == eModNone)
        goto InvSize;
      break;
    case eSymbolSize24Bit:
      if (Mode24 == eModNone)
        goto InvSize;
      break;
    default:
      break;
    InvSize:
      WrStrErrorPos(ErrNum_InvAddrMode, pArg);
      return;
  }

  if (IsCode)
    SizeType = pCurrCPUProps->AddrIntType;
  else switch (Size)
  {
    case eSymbolSize24Bit:
      SizeType = Int24;
      break;
    case eSymbolSize16Bit:
      SizeType = Int16;
      break;
    case eSymbolSize8Bit:
      SizeType = Int8;
      break;
    default:
      if (Mode24 != eModNone)
        SizeType = Int24;
      else if (Mode16 != eModNone)
        SizeType = Int16;
      else
        SizeType = Int8;
      break;
  }
  Value = EvalStrIntExpressionWithFlags(pArg, SizeType, &OK, &Flags);

  if (OK)
  {
    LongWord SrcAddress = IsCode ? EProgCounter() + 3 : 0;

    if (Size == eSymbolSizeUnknown)
    {
      if ((Value <= 0xff) && (Mode8 != eModNone))
        Size = eSymbolSize8Bit;
      else if (((Value >> 16) == (SrcAddress >> 16)) && (Mode16 != eModNone))
        Size = eSymbolSize16Bit;
      else
        Size = eSymbolSize24Bit;
    }

    /* this may only happen if SizeTypes was forced to 24 Bit because of code addessing: */

    if ((Size == eSymbolSize24Bit) && (Mode24 == eModNone) && !mSymbolQuestionable(Flags))
    {
      WrStrErrorPos(ErrNum_TargOnDiffSection, pArg);
      return;
    }

    FillAdrVals(pAdrVals, Value, Size);
    switch (Size)
    {
      case eSymbolSize8Bit:
        pAdrVals->Part = Part8;
        pAdrVals->Mode = Mode8;
        break;
      case eSymbolSize16Bit:
        pAdrVals->Part = Part16;
        pAdrVals->Mode = Mode16;
        break;
      case eSymbolSize24Bit:
        pAdrVals->Part = Part24;
        pAdrVals->Mode = Mode24;
        break;
      default:
        assert(0);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecideIndirectSize(LongWord Mask, const tStrComp *pArg,
                              tAdrMode Mode8_8, tAdrMode Mode8_16, tAdrMode Mode16_16, tAdrMode Mode16_24,
                              Byte Part8_8, Byte Part8_16, Byte Part16_16, Byte Part16_24,
                              tAdrVals *pAdrVals)
 * \brief  address mode decision for indirect operands []
 * \param  Mask bit mask of allowed modes
 * \param  pArg expression
 * \param  Mode8_8 requested mode for 8-bit pointer on 8-bit address
 * \param  Mode8_16 requested mode for 16-bit pointer on 8-bit address
 * \param  Mode16_16 requested mode for 16-bit pointer on 16-bit address
 * \param  Mode16_24 requested mode for 24-bit pointer on 16-bit address
 * \param  Part8_8 Address part for 8-bit pointer on 8-bit address
 * \param  Part8_16 Address part for 16-bit pointer on 8-bit address
 * \param  Part16_16 Address part for 16-bit pointer on 16-bit address
 * \param  Part16_24 Address part for 24-bit pointer on 16-bit address
 * \param  pAdrVals destination to fill out
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean DecideIndirectSize(LongWord Mask, const tStrComp *pArg,
                                  tAdrMode Mode8_8, tAdrMode Mode8_16, tAdrMode Mode16_16, tAdrMode Mode16_24,
                                  Byte Part8_8, Byte Part8_16, Byte Part16_16, Byte Part16_24,
                                  tAdrVals *pAdrVals)
{
  Boolean OK;
  int Offset;
  tSymbolSize AddrSize = eSymbolSizeUnknown,
              PtrSize = eSymbolSizeUnknown;
  IntType SizeType;
  Word Address;
  tSymbolFlags Flags;

  if ((Mode8_8 != eModNone) && !ModeInMask(Mask, Mode8_8))
    Mode8_8 = eModNone;
  if ((Mode8_16 != eModNone) && !ModeInMask(Mask, Mode8_16))
    Mode8_16 = eModNone;
  if ((Mode16_16 != eModNone) && !ModeInMask(Mask, Mode16_16))
    Mode16_16 = eModNone;
  if ((Mode16_24 != eModNone) && !ModeInMask(Mask, Mode16_24))
    Mode16_24 = eModNone;

  /* Cut off address byte size, signified by leading '<' or '>': */

  switch (*pArg->str.p_str)
  {
    case '>':
      Offset = 1;
      AddrSize = eSymbolSize16Bit;
      break;
    case '<':
      Offset = 1;
      AddrSize = eSymbolSize8Bit;
      break;
    default:
      Offset = 0;
      AddrSize = eSymbolSizeUnknown;
  }

  /* Cut off pointer size, signified by trailing '.w/.e' if 16/24 bit: */

  PtrSize = CutSizeSuffix(pArg);

  /* if no pointer size given, assume the smallest possible one: */

  if (PtrSize == eSymbolSizeUnknown)
  {
    if (Mode8_8 != eModNone)
      PtrSize = eSymbolSize8Bit;
    else if ((Mode8_16 != eModNone) || (Mode16_16 != eModNone))
      PtrSize = eSymbolSize16Bit;
    else
      PtrSize = eSymbolSize24Bit;
  }

  switch (AddrSize)
  {
    case eSymbolSize16Bit:
      SizeType = Int16;
      break;
    case eSymbolSize8Bit:
      SizeType = Int8;
      break;
    default:
      if ((Mode16_16 != eModNone) || (Mode16_24 != eModNone))
        SizeType = Int16;
      else
        SizeType = Int8;
      break;
  }

  Address = EvalStrIntExpressionOffsWithFlags(pArg, Offset, SizeType, &OK, &Flags);
  if (mFirstPassUnknown(Flags) && (Mode16_16 == eModNone) && (Mode16_24 == eModNone))
    Address &= 0xff;

  if (OK)
  {
    /* Finally decide about address size: */

    if (eSymbolSizeUnknown == AddrSize)
    {
      if ((Address <= 0xff) && (PtrSize == eSymbolSize8Bit) && (Mode8_8 != eModNone))
        AddrSize = eSymbolSize8Bit;
      else if ((Address <= 0xff) && (PtrSize == eSymbolSize16Bit) && (Mode8_16 != eModNone))
        AddrSize = eSymbolSize8Bit;
      else
        AddrSize = eSymbolSize16Bit;
    }

    FillAdrVals(pAdrVals, Address, AddrSize);
    if (eSymbolSize16Bit == AddrSize)
    {
      if (PtrSize == eSymbolSize24Bit)
      {
        pAdrVals->Part = Part16_24;
        pAdrVals->Mode = Mode16_24;
      }
      else
      {
        pAdrVals->Part = Part16_16;
        pAdrVals->Mode = Mode16_16;
      }
    }
    else
    {
      if (PtrSize == eSymbolSize16Bit)
      {
        pAdrVals->Part = Part8_16;
        pAdrVals->Mode = Mode8_16;
      }
      else
      {
        pAdrVals->Part = Part8_8;
        pAdrVals->Mode = Mode8_8;
      }
    }
  }
  return OK;
}

/*!------------------------------------------------------------------------
 * \fn     ChkAdrMode(tAdrMode *pAdrMode, LongWord Mask, tErrorNum ErrorNum, const tStrComp *pArg)
 * \brief  check for allowed addressing mode
 * \param  pAdrMode parsed addressing mode (in/out)
 * \param  Mask list of allowed modes
 * \param  ErrorNum error message to emit if not allowed
 * \param  pArg offending arg
 * \return True if mode is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkAdrMode(tAdrMode *pAdrMode, LongWord Mask, tErrorNum ErrorNum, const tStrComp *pArg)
{
  if ((*pAdrMode != eModNone) && (!(Mask & (1ul << *pAdrMode))))
  {
    WrStrErrorPos(ErrorNum, pArg);
    *pAdrMode = eModNone;
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     ChkAdrValsMode(tAdrVals *pAdrVals, LongWord Mask, tErrorNum ErrorNum, const tStrComp *pArg)
 * \brief  check for allowed addressing mode
 * \param  pAdrVals parsed addressing mode (in/out)
 * \param  Mask list of allowed modes
 * \param  ErrorNum error message to emit if not allowed
 * \param  pArg offending arg
 * \return True if mode is OK
 * ------------------------------------------------------------------------ */

static Boolean ChkAdrValsMode(tAdrVals *pAdrVals, LongWord Mask, tErrorNum ErrorNum, const tStrComp *pArg)
{
  if (!ChkAdrMode(&pAdrVals->Mode, Mask, ErrorNum, pArg))
  {
    pAdrVals->Cnt = 0;
    return False;
  }
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRegCore(const tStrComp *pArg, tAdrMode *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg argument to check
 * \param  pResult resulting mode if it is a register
 * \return True if argument is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeRegCore(const tStrComp *pArg, tAdrMode *pResult)
{
  *pResult = eModNone;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    *pResult = eModA;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "X"))
  {
    *pResult = eModX;
    return True;
  }
  if (!as_strcasecmp(pArg->str.p_str, "XL"))
  {
    *pResult = eModXL;
    return True;
  }
  if (!as_strcasecmp(pArg->str.p_str, "XH"))
  {
    *pResult = eModXH;
    return True;
  }

  if (!as_strcasecmp(pArg->str.p_str, "Y"))
  {
    *pResult = eModY;
    return True;
  }
  if (!as_strcasecmp(pArg->str.p_str, "YL"))
  {
    *pResult = eModYL;
    return True;
  }
  if (!as_strcasecmp(pArg->str.p_str, "YH"))
  {
    *pResult = eModYH;
    return True;
  }

  if ((!as_strcasecmp(pArg->str.p_str, "S"))
   || ((pCurrCPUProps->Core == eCoreSTM8) && !as_strcasecmp(pArg->str.p_str, "SP")))
  {
    *pResult = eModS;
    return True;
  }

  return False;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const tStrComp *pArg, tAdrMode *pResult, LongWord Mask)
 * \brief  decode addressing expression, registers-only
 * \param  pArg argument
 * \param  pResult resulting mode
 * \param  Mask list of allowed modes
 * \return True if argument is a CPU register
 * ------------------------------------------------------------------------ */

static Boolean DecodeReg(const tStrComp *pArg, tAdrMode *pResult, LongWord Mask)
{
  return DecodeRegCore(pArg, pResult)
      && ChkAdrMode(pResult, Mask, ErrNum_InvReg, pArg);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAdr(const tStrComp *pArg, LongWord Mask, Boolean IsCode, tAdrVals *pAdrVals)
 * \brief  decode addressing expression
 * \param  pArg argument
 * \param  Mask list of allowed masks
 * \param  IsCode is expression a jump/call address?
 * \param  pAdrVals destination to fill out
 * \return True if successfully parsed
 * ------------------------------------------------------------------------ */

static Boolean DecodeAdr(const tStrComp *pArg, LongWord Mask, Boolean IsCode, tAdrVals *pAdrVals)
{
  Boolean OK;
  int ArgLen;

  ArgLen = strlen(pArg->str.p_str);

  ResetAdrVals(pAdrVals);

  /* Register ? */

  if (DecodeRegCore(pArg, &pAdrVals->Mode))
  {
    switch (pAdrVals->Mode)
    {
      case eModY:
      case eModYL:
      case eModYH:
        AddPrefix(0x90);
        break;
      default:
        break;
    }
    goto chk;
  }

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    Word Value = EvalStrIntExpressionOffs(pArg, 1, (OpSize == eSymbolSize16Bit) ? Int16 : Int8, &OK);
    if (OK)
    {
      pAdrVals->Mode = eModImm;
      pAdrVals->Part = 0xa;
      FillAdrVals(pAdrVals, Value, OpSize);
    }
    goto chk;
  }

  /* speicherindirekt ? */

  if ((*pArg->str.p_str == '[') && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    tStrComp Comp;
    Boolean OK;

    StrCompRefRight(&Comp, pArg, 1);
    Comp.str.p_str[ArgLen - 2] = '\0'; Comp.Pos.Len--;
    OK = DecideIndirectSize(Mask, &Comp, eModIAbs8, eModIAbs16, eModI16Abs16, eModI16Abs24, 0xb, 0xc, 0xc, 0xb, pAdrVals);
    Comp.str.p_str[ArgLen - 2] = ']';
    if (OK)
      AddPrefix((pAdrVals->Mode == eModI16Abs16) ? 0x72: 0x92);
    goto chk;
  }

  /* sonstwie indirekt ? */

  if (IsIndirect(pArg->str.p_str))
  {
    tStrComp Comp, Left, Right;
    Boolean YReg = False, SPReg = False;
    char *p;

    StrCompRefRight(&Comp, pArg, 1);
    StrCompShorten(&Comp, 1);

    /* ein oder zwei Argumente ? */

    p = QuotPos(Comp.str.p_str, ',');
    if (!p)
    {
      pAdrVals->Part = 0xf;
      if (!as_strcasecmp(Comp.str.p_str, "X")) pAdrVals->Mode = eModIX;
      else if (!as_strcasecmp(Comp.str.p_str, "Y"))
      {
        pAdrVals->Mode = eModIY;
        AddPrefix(0x90);
      }
      else WrStrErrorPos(ErrNum_InvReg, &Comp);
      goto chk;
    }

    StrCompSplitRef(&Left, &Right, &Comp, p);

    if (!as_strcasecmp(Left.str.p_str, "X"))
      Left = Right;
    else if (!as_strcasecmp(Right.str.p_str, "X"));
    else if (!as_strcasecmp(Left.str.p_str, "Y"))
    {
      Left = Right;
      YReg = True;
    }
    else if (!as_strcasecmp(Right.str.p_str, "Y"))
      YReg = True;
    else if (!as_strcasecmp(Left.str.p_str, "SP"))
    {
      Left = Right;
      SPReg = True;
    }
    else if (!as_strcasecmp(Right.str.p_str, "SP"))
      SPReg = True;
    else
    {
      WrStrErrorPos(ErrNum_InvAddrMode, &Comp);
      return False;
    }

    /* speicherindirekt ? */

    ArgLen = strlen(Left.str.p_str);
    if ((*Left.str.p_str == '[') && (Left.str.p_str[ArgLen - 1] == ']'))
    {
      StrCompRefRight(&Right, &Left, 1);
      StrCompShorten(&Right, 1);
      if (YReg)
      {
        if (DecideIndirectSize(Mask, &Right, eModIYAbs8, eModIYAbs16, eModNone, eModI16YAbs24, 0xe, 0xd, 0x0, 0xa, pAdrVals))
          AddPrefix(0x91);
      }
      else
      {
        if (DecideIndirectSize(Mask, &Right, eModIXAbs8, eModIXAbs16, eModI16XAbs16, eModI16YAbs24, 0xe, 0xd, 0xd, 0xa, pAdrVals))
          AddPrefix((pAdrVals->Mode == eModI16XAbs16) ? 0x72 : 0x92);
      }
    }
    else
    {
      if (YReg) DecideSize(Mask, &Left, eModIY8, eModIY16, eModIY24, 0xe, 0xd, 0xa, IsCode, pAdrVals);
      else if (SPReg) DecideSize(Mask, &Left, eModISP8, eModNone, eModNone, 0x1, 0x0, 0x0, IsCode, pAdrVals);
      else DecideSize(Mask, &Left, eModIX8, eModIX16, eModIX24, 0xe, 0xd, 0xa, IsCode, pAdrVals);
      if ((pAdrVals->Mode != eModNone) && YReg) AddPrefix(0x90);
    }

    goto chk;
  }

  /* dann absolut */

  DecideSize(Mask, pArg, eModAbs8, eModAbs16, eModAbs24, 0xb, 0xc, 0xb, IsCode, pAdrVals);

chk:
  return ChkAdrValsMode(pAdrVals, Mask, ErrNum_InvAddrMode, pArg);
}

/*!------------------------------------------------------------------------
 * \fn     ConstructMask(LongWord TotMask, tSymbolSize OpSize)
 * \brief  construct actual bit mask of allowed addressing modes from overall list
 * \param  TotMask overall list
 * \param  OpSize operand size in use (8/16/unknown)
 * \return actual mask
 * ------------------------------------------------------------------------ */

static LongWord ConstructMask(LongWord TotMask, tSymbolSize OpSize)
{
  if (pCurrCPUProps->Core == eCoreST7)
  {
    /* not present on ST7 */

    TotMask &= ~(MModISP8 | MModI16Abs16 | MModI16XAbs16 | MModXL | MModYL | MModXH | MModYH);

    /* SP/X/Y are 8 bits wide on ST7 */

    if (OpSize == eSymbolSize16Bit)
      TotMask &= ~(MModX | MModY | MModS);
  }

  if (pCurrCPUProps->Core == eCoreSTM8)
  {
    /* removed on STM8 */

    TotMask &= ~(MModIAbs8 | MModIXAbs8 | MModIYAbs8);

    /* SP/X/Y changed to 16 bits on STM8 */

    if (OpSize == eSymbolSize8Bit)
      TotMask &= ~(MModX | MModY | MModS);
  }

  return TotMask;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits in symbol table:
 * bits 0..2: bit position
 * bits 3...10/18: register address in memory space (first 256/64K)
 */

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const tStrComp *pArg, Boolean *pOK)
 * \brief  evaluate bit position
 * \param  bit position argument (with or without #)
 * \param  pOK parsing OK?
 * \return numeric bit position
 * ------------------------------------------------------------------------ */

static LongWord EvalBitPosition(const tStrComp *pArg, Boolean *pOK)
{
  return EvalStrIntExpressionOffs(pArg, !!(*pArg->str.p_str == '#'), UInt3, pOK);
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, Word Address)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in word
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, Word Address)
{
  return (BitPos & 7)
       | (((LongWord)Address & 0xffff) << 3);
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg)
{
  Boolean OK;
  LongWord Addr;
  Byte BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK);
  if (!OK)
    return False;

  Addr = EvalStrIntExpression(pRegArg, (pCurrCPUProps->Core == eCoreSTM8) ? UInt16 : UInt8, &OK);
  if (!OK)
    return False;

  *pResult = AssembleBitSymbol(BitPos, Addr);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg(LongWord *pResult, int Start, int Stop)
{
  *pResult = 0;

  /* Just one argument -> parse as bit argument */

  if (Start == Stop)
  {
    tEvalResult EvalResult;

    *pResult = EvalStrIntExpressionWithResult(&ArgStr[Start], (pCurrCPUProps->Core == eCoreSTM8) ? UInt19 : UInt11, &EvalResult);
    if (EvalResult.OK)
      ChkSpace(SegBData, EvalResult.AddrSpaceMask);
    return EvalResult.OK;
  }

  /* register & bit position are given as separate arguments */

  else if (Stop == Start + 1)
    return DecodeBitArg2(pResult, &ArgStr[Start], &ArgStr[Stop]);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
 * \brief  transform compact representation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos)
{
  *pAddress = (BitSymbol >> 3) & 0xffff;
  *pBitPos = BitSymbol & 7;
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_ST7(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_ST7(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos;
  Word Address;

  DissectBitSymbol(Inp, &Address, &BitPos);

  as_snprintf(pDest, DestSize, "$%x.%u", (unsigned)Address, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandST7Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandST7Bit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    if (!ChkRange(Address, 0, (pCurrCPUProps->Core == eCoreSTM8) ? 0xffff : 0xff)
     || !ChkRange(pStructElem->BitPos, 0, 7))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitAdrWithIndir(int Start, int Stop, Byte *pBitPos, tAdrVals *pAdrVals)
 * \brief  decode bit expression, regarding indirect mode on ST7
 * \param  Start 1st argument of bit expression
 * \param  Stop 2nd argument of bit expression
 * \param  pAdrVals result to fill
 * \return True if parsing OK
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitAdrWithIndir(int Start, int Stop, Byte *pBitPos, tAdrVals *pAdrVals)
{
  Boolean OK;

  if (Start == Stop)
  {
    LongWord BitSym;
    Word Address;

    if (!DecodeBitArg(&BitSym, Start, Stop))
      return False;
    DissectBitSymbol(BitSym, &Address, pBitPos);
    if (pCurrCPUProps->Core == eCoreSTM8)
    {
      FillAdrVals(pAdrVals, Address, eSymbolSize16Bit);
      pAdrVals->Mode = eModAbs16;
    }
    else
    {
      FillAdrVals(pAdrVals, Address, eSymbolSize8Bit);
      pAdrVals->Mode = eModAbs8;
    }
    PrefixCnt = 0;
    return True;
  }
  else if (Start + 1 == Stop)
  {
    *pBitPos = EvalBitPosition(&ArgStr[Stop], &OK);
    if (!OK)
      return False;
    return DecodeAdr(&ArgStr[Start], (pCurrCPUProps->Core == eCoreSTM8) ? MModAbs16 : (MModAbs8 | MModIAbs8), False, pAdrVals);
  }
  else
    return False;
}

/*!------------------------------------------------------------------------
 * \fn     CompleteCode(const tAdrVals *pAdrVals)
 * \brief  assemble instruction from prefixes, opcode and address values
 * \param  pAdrVals values to append to code
 * ------------------------------------------------------------------------ */

static void CompleteCode(const tAdrVals *pAdrVals)
{
  memcpy(BAsmCode + PrefixCnt + 1, pAdrVals->Vals, pAdrVals->Cnt);
  CodeLen = PrefixCnt + 1 + pAdrVals->Cnt;
}

/*!------------------------------------------------------------------------
 * \fn     WriteMOV(tAdrVals *pDestAdrVals, tAdrVals *pSrcAdrVals)
 * \brief  core of writing code of MOV instruction
 * \param  pDestAdrVals parsed destination operand
 * \param  pSrcAdrVals parsed source operand
 * ------------------------------------------------------------------------ */

static void WriteMOV(tAdrVals *pDestAdrVals, tAdrVals *pSrcAdrVals)
{
  if ((pDestAdrVals->Mode == eModAbs16)
   || (pSrcAdrVals->Mode == eModAbs16)
   || (pSrcAdrVals->Mode == eModImm))
  {
    ExtendAdrVals(pSrcAdrVals);
    ExtendAdrVals(pDestAdrVals);
  }
  switch (pSrcAdrVals->Mode)
  {
    case eModImm:
      BAsmCode[0] = 0x35;
      break;
    case eModAbs8:
      BAsmCode[0] = 0x45;
      break;
    case eModAbs16:
      BAsmCode[0] = 0x55;
      break;
    default:
      break;
  }
  memcpy(&BAsmCode[1], pSrcAdrVals->Vals, pSrcAdrVals->Cnt);
  memcpy(&BAsmCode[1 + pSrcAdrVals->Cnt], pDestAdrVals->Vals, pDestAdrVals->Cnt);
  CodeLen = 1 + pSrcAdrVals->Cnt + pDestAdrVals->Cnt;
}

/*--------------------------------------------------------------------------*/
/* Code Generators */

/*!------------------------------------------------------------------------
 * \fn     DecodeFixed(Word Code)
 * \brief  decode instructions without argument
 * \param  per-instruction context
 * ------------------------------------------------------------------------ */

static void DecodeFixed(Word Code)
{
  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (Hi(Code) && (pCurrCPUProps->Core != eCoreSTM8)) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else
  {
    if (Hi(Code) >= 2)
      AddPrefix(Hi(Code));
    BAsmCode[PrefixCnt] = Lo(Code);
    CodeLen = PrefixCnt + 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLD(Word Code)
 * \brief  decode LD instruction
 * ------------------------------------------------------------------------ */

static void DecodeLD(Word Code)
{
  LongWord Mask;
  tAdrVals DestAdrVals, SrcAdrVals;

  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    /* NOTE: Set eSymbolSizeUnknown so we get X/Y/S also in STM8 mode.
             LD will work like LDW in thi case. */

    Mask = ConstructMask(MModA | MModX | MModY | MModS |  MModXL | MModYL | MModXH | MModYH
                       | MModImm | MModAbs8 | MModAbs16
                       | MModIX | MModIX8 | MModIX16 | MModIY | MModIY8 | MModIY16 | MModISP8
                       | MModIAbs8 | MModIAbs16 | MModI16Abs16
                       | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs8 | MModIYAbs16, eSymbolSizeUnknown);
    if (DecodeAdr(&ArgStr[1], Mask, False, &DestAdrVals))
    switch (DestAdrVals.Mode)
    {
      case eModA:
        Mask = ConstructMask(MModImm | MModX | MModY | MModS | MModXL | MModYL | MModXH | MModYH | MModAbs8 | MModAbs16
             | MModIX | MModIX8 | MModIX16 | MModIY | MModIY8 | MModIY16 | MModISP8
             | MModIAbs8 | MModIAbs16 | MModI16Abs16
             | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs8 | MModIYAbs16, eSymbolSize8Bit);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModX: case eModXL:
          case eModY: case eModYL:
            BAsmCode[PrefixCnt] = 0x9f;
            CodeLen = PrefixCnt + 1;
            break;
          case eModXH: case eModYH:
            BAsmCode[PrefixCnt] = 0x9e;
            CodeLen = PrefixCnt + 1;
            break;
          case eModS:
            BAsmCode[PrefixCnt] = 0x9e;
            CodeLen = PrefixCnt + 1;
            break;
          case eModISP8: /* irregular, cannot use default case */
            BAsmCode[PrefixCnt] = 0x7b;
            CompleteCode(&SrcAdrVals);
            break;
          default:
            BAsmCode[PrefixCnt] = 0x06 + (SrcAdrVals.Part << 4);
            CompleteCode(&SrcAdrVals);
        }
        break;
      case eModX:
        if (pCurrCPUProps->Core == eCoreSTM8)
          OpSize = eSymbolSize16Bit;
        Mask = ConstructMask(MModA | MModY | MModS | MModImm
                           | MModAbs8 | MModAbs16
                           | MModIX | MModIX8 | MModIX16 | MModISP8
                           | MModIAbs8 | MModIAbs16 | MModI16Abs16
                           | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16, OpSize);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModA:
            BAsmCode[PrefixCnt] = 0x97;
            CodeLen = PrefixCnt + 1;
            break;
          case eModY:
            BAsmCode[0] = 0x93;
            CodeLen = 1;
            break;
          case eModS:
            BAsmCode[PrefixCnt] = 0x96;
            CodeLen = PrefixCnt + 1;
            break;
          default:
            BAsmCode[PrefixCnt] = 0x0e + (SrcAdrVals.Part << 4); /* ANSI :-O */
            CompleteCode(&SrcAdrVals);
        }
        break;
      case eModXL:
      case eModYL:
        if (DecodeAdr(&ArgStr[2], MModA, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = 0x97;
          CodeLen = PrefixCnt + 1;
        }
        break;
      case eModXH:
      case eModYH:
        if (DecodeAdr(&ArgStr[2], MModA, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = 0x95;
          CodeLen = PrefixCnt + 1;
        }
        break;
      case eModY:
        PrefixCnt = 0;
        if (pCurrCPUProps->Core == eCoreSTM8)
          OpSize = eSymbolSize16Bit;
        Mask = ConstructMask(MModA | MModX | MModS | MModImm
                           | MModAbs8 | MModAbs16
                           | MModIY | MModIY8 | MModIY16 | MModISP8
                           | MModIAbs8 | MModIAbs16 | MModIYAbs8 | MModIYAbs16, OpSize);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModA:
            AddPrefix(0x90);
            BAsmCode[PrefixCnt] = 0x97;
            CodeLen = PrefixCnt + 1;
            break;
          case eModX:
            AddPrefix(0x90);
            BAsmCode[PrefixCnt] = 0x93;
            CodeLen = PrefixCnt + 1;
            break;
          case eModS:
            AddPrefix(0x90);
            BAsmCode[PrefixCnt] = 0x96;
            CodeLen = PrefixCnt + 1;
            break;
          case eModISP8:
            BAsmCode[PrefixCnt] = 0x16;
            goto common;
          default:
            if (PrefixCnt == 0) AddPrefix(0x90);
            if (BAsmCode[0] == 0x92) BAsmCode[0]--;
            BAsmCode[PrefixCnt] = 0x0e + (SrcAdrVals.Part << 4); /* ANSI :-O */
          common:
            CompleteCode(&SrcAdrVals);
        }
        break;
      case eModS:
        if (DecodeAdr(&ArgStr[2], MModA | MModX | MModY, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModA:
            BAsmCode[PrefixCnt] = 0x95;
            CodeLen = PrefixCnt + 1;
            break;
          case eModX:
          case eModY:
            BAsmCode[PrefixCnt] = 0x94;
            CodeLen = PrefixCnt + 1;
            break;
          default:
            break;
        }
        break;
      default:
      {
        Boolean Result;
        unsigned SavePrefixCnt = PrefixCnt;

        /* set unknown size to allow X&Y also on STM8 if LD is written instead of LDW */
        Mask = ConstructMask(MModA | MModX | MModY, eSymbolSizeUnknown);
        /* aliases for MOV */
        if (pCurrCPUProps->Core == eCoreSTM8)
          switch (DestAdrVals.Mode)
          {
            case eModAbs16:
              Mask |= MModAbs16 | MModImm;
              break;
            case eModAbs8:
              Mask |= MModAbs8 | MModAbs16 | MModImm;
              break;
            default:
              break;
          }
        Result = DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals);
        PrefixCnt = SavePrefixCnt;
        if (Result)
        switch (SrcAdrVals.Mode)
        {
          case eModA:
            Mask = ConstructMask(MModAbs8 | MModAbs16
                               | MModIX | MModIX8 | MModIX16 | MModIY | MModIY8 | MModIY16 | MModISP8
                               | MModIAbs8 | MModIAbs16 | MModI16Abs16 | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs8 | MModIYAbs16, eSymbolSize8Bit);
            if (ChkAdrValsMode(&DestAdrVals, Mask, ErrNum_InvAddrMode, &ArgStr[1]))
            {
              BAsmCode[PrefixCnt] = (DestAdrVals.Mode == eModISP8) ? 0x6b : (0x07 + (DestAdrVals.Part << 4));
              CompleteCode(&DestAdrVals);
            }
            break;
          case eModX:
            Mask = MModAbs8 | MModAbs16 | MModIAbs16;
            if (pCurrCPUProps->Core == eCoreST7)
              Mask |= MModIX | MModIX8 | MModIX16 | MModIAbs8 | MModIXAbs8 | MModIXAbs16;
            if (pCurrCPUProps->Core == eCoreSTM8)
              Mask |= MModIY | MModIY8 | MModIY16 | MModIYAbs16 | MModISP8 | MModI16Abs16;
            if (ChkAdrValsMode(&DestAdrVals, Mask, ErrNum_InvAddrMode, &ArgStr[1]))
            {
              BAsmCode[PrefixCnt] = 0x0f + (DestAdrVals.Part << 4);
              CompleteCode(&DestAdrVals);
            }
            break;
          case eModY:
            Mask = MModAbs8 | MModAbs16 | MModIAbs16;
            if (pCurrCPUProps->Core == eCoreST7)
              Mask |= MModIY | MModIY8 | MModIY16 | MModIAbs8 | MModIYAbs8 | MModIYAbs16;
            if (pCurrCPUProps->Core == eCoreSTM8)
              Mask |= MModIX | MModIX8 | MModIX16 | MModIXAbs16 | MModI16XAbs16 | MModISP8;
            if (ChkAdrValsMode(&DestAdrVals, Mask, ErrNum_InvAddrMode, &ArgStr[1]))
            switch (DestAdrVals.Mode)
            {
              case eModISP8:
                PrefixCnt = 0;
                BAsmCode[PrefixCnt] = 0x07 + (DestAdrVals.Part << 4);
                goto common_ysrc2;
              case eModIX:
              case eModIX8:
              case eModIX16:
                PrefixCnt = 0;
                goto common_ysrc;
              case eModIXAbs16:
                goto common_ysrc;
              default:
                if (PrefixCnt == 0) AddPrefix(0x90);
                if (BAsmCode[0] == 0x92) BAsmCode[0]--;
              common_ysrc:
                BAsmCode[PrefixCnt] = 0x0f + (DestAdrVals.Part << 4);
              common_ysrc2:
                CompleteCode(&DestAdrVals);
            }
            break;
          case eModImm: /* MOV aliases: only possible if Dest = Abs8/16 */
          case eModAbs8:
          case eModAbs16:
            WriteMOV(&DestAdrVals, &SrcAdrVals);
            break;
          default:
            break;
        }
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDW(Word Code)
 * \brief  decode LDW instruction
 * ------------------------------------------------------------------------ */

static void DecodeLDW(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;
  LongWord Mask;

  UNUSED(Code);
  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    Mask = ConstructMask(MModX | MModY | MModS
                       | MModAbs8 | MModAbs16
                       | MModIX | MModIX8 | MModIX16 | MModIY | MModIY8 | MModIY16 | MModISP8
                       | MModIAbs16 | MModI16Abs16 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs16, OpSize = eSymbolSize16Bit);
    if (DecodeAdr(&ArgStr[1], Mask, False, &DestAdrVals))
    switch (DestAdrVals.Mode)
    {
      case eModX:
        Mask = ConstructMask(MModY | MModS | MModImm
                           | MModAbs8 | MModAbs16
                           | MModIX | MModIX8 | MModIX16 | MModISP8
                           | MModIAbs16 | MModI16Abs16 | MModIXAbs16 | MModI16XAbs16, eSymbolSize16Bit);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModY:
            PrefixCnt = 0;
            BAsmCode[PrefixCnt] = 0x93;
            CodeLen = PrefixCnt + 1;
            break;
          case eModS:
            BAsmCode[PrefixCnt] = 0x96;
            CodeLen = PrefixCnt + 1;
            break;
          default:
            BAsmCode[PrefixCnt] = 0x0e | (SrcAdrVals.Part << 4);
            CompleteCode(&SrcAdrVals);
            break;
        }
        break;
      case eModY:
        PrefixCnt = 0;
        Mask = ConstructMask(MModX | MModS | MModImm
                           | MModAbs8 | MModAbs16
                           | MModIY | MModIY8 | MModIY16 | MModISP8
                           | MModIAbs16 | MModIYAbs16, eSymbolSize16Bit);
        DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals);
        if (!PrefixCnt)
          AddPrefix(0x90);
        if (SrcAdrVals.Mode != eModNone)
        switch (SrcAdrVals.Mode)
        {
          case eModX:
            BAsmCode[PrefixCnt] = 0x93;
            CodeLen = PrefixCnt + 1;
            break;
          case eModS:
            BAsmCode[PrefixCnt] = 0x96;
            CodeLen = PrefixCnt + 1;
            break;
          case eModISP8:
            PrefixCnt = 0;
            BAsmCode[PrefixCnt] = 0x16;
            goto common_x;
          case eModIAbs16:
            PrefixCnt = 0;
            AddPrefix(0x91);
            /* fall-through */
          default:
            BAsmCode[PrefixCnt] = 0x0e | (SrcAdrVals.Part << 4);
          common_x:
            CompleteCode(&SrcAdrVals);
            break;
        }
        break;
      case eModS:
        Mask = ConstructMask(MModX | MModY, eSymbolSize16Bit);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = 0x94;
          CodeLen = PrefixCnt + 1;
        }
        break;
      default:
      {
        Mask = ConstructMask(MModX | MModY, eSymbolSize16Bit);
        if (DecodeReg(&ArgStr[2], &SrcAdrVals.Mode, Mask))
        switch (DestAdrVals.Mode)
        {
          case eModIAbs16:
            if (eModY == SrcAdrVals.Mode)
            {
              PrefixCnt = 0;
              AddPrefix(0x91);
            }
            goto common_y2;
          case eModISP8:
            BAsmCode[PrefixCnt] = (eModY == SrcAdrVals.Mode) ? 0x17 : 0x1f;
            goto common_y;
          case eModAbs8:
          case eModAbs16:
            if (eModY == SrcAdrVals.Mode)
              AddPrefix(0x90);
            /* fall-through */
          default:
          common_y2:
            BAsmCode[PrefixCnt] = 0x0f | (DestAdrVals.Part << 4);
          common_y:
            CompleteCode(&DestAdrVals);
            break;
        }
        break;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLDF(Word Code)
 * \brief  decode LDF instruction
 * ------------------------------------------------------------------------ */

static void DecodeLDF(Word Code)
{
  tAdrVals AdrVals;

  UNUSED(Code);
  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[1], MModA | MModAbs24 | MModIX24 | MModIY24 | MModI16XAbs24 | MModI16YAbs24 | MModI16Abs24, False, &AdrVals))
  switch (AdrVals.Mode)
  {
    case eModA:
      if (DecodeAdr(&ArgStr[2], MModAbs24 | MModIX24 | MModIY24 | MModI16XAbs24 | MModI16YAbs24 | MModI16Abs24, False, &AdrVals))
      {
        switch (AdrVals.Mode)
        {
          case eModAbs24:
          case eModI16Abs24:
            BAsmCode[PrefixCnt] = 0x0c | (AdrVals.Part << 4);
            break;
          case eModIX24:
          case eModIY24:
          case eModI16XAbs24:
          case eModI16YAbs24:
            BAsmCode[PrefixCnt] = 0x0f | (AdrVals.Part << 4);
            break;
          default:
            break;
        }
        CompleteCode(&AdrVals);
      }
      break;
    default:
    {
      tAdrMode RegMode;

      if (DecodeReg(&ArgStr[2], &RegMode, MModA))
      {
        switch (AdrVals.Mode)
        {
          case eModAbs24:
          case eModI16Abs24:
            BAsmCode[PrefixCnt] = 0x0d | (AdrVals.Part << 4);
            break;
          case eModIX24:
          case eModIY24:
          case eModI16XAbs24:
          case eModI16YAbs24:
            BAsmCode[PrefixCnt] = 0x07 | (AdrVals.Part << 4);
            break;
          default:
            break;
        }
        CompleteCode(&AdrVals);
      }
      break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMOV(Word Code)
 * \brief  decode MOV instruction
 * ------------------------------------------------------------------------ */

static void DecodeMOV(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  UNUSED(Code);
  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[2], MModImm | MModAbs8 | MModAbs16, False, &SrcAdrVals))
  {
    if (DecodeAdr(&ArgStr[1], ((SrcAdrVals.Mode == eModAbs8) ? MModAbs8 : 0) | MModAbs16, False, &DestAdrVals))
      WriteMOV(&DestAdrVals, &SrcAdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodePUSH_POP(Word Code)
 * \brief  decode PUSH(W)/POP(W) instructions
 * \param  Code instruction context
 * ------------------------------------------------------------------------ */

static void DecodePUSH_POP(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (Hi(Code) && (pCurrCPUProps->Core != eCoreSTM8)) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    LongWord Mask;
    tAdrVals AdrVals;

    Mask = MModX | MModY | (Hi(Code) ? 0 : MModA);
    if (pCurrCPUProps->Core == eCoreSTM8)
    {
      Mask |= MModAbs16;
      if (Lo(Code))
        Mask |= MModImm;
    }
    if (!as_strcasecmp(ArgStr[1].str.p_str, "CC"))
    {
      BAsmCode[PrefixCnt] = 0x86 + Lo(Code);
      CodeLen = PrefixCnt + 1;
    }
    else if (DecodeAdr(&ArgStr[1], Mask, False, &AdrVals))
    {
      switch (AdrVals.Mode)
      {
        case eModA:
          BAsmCode[PrefixCnt] = 0x84 + Lo(Code);
          break;
        case eModX:
        case eModY:
          BAsmCode[PrefixCnt] = 0x85 + Lo(Code);
          break;
        case eModAbs16:
          BAsmCode[PrefixCnt] = Lo(Code) ? 0x4b : 0x32;
          break;
        case eModImm:
          BAsmCode[PrefixCnt] = 0x4b;
          break;
        default:
          break;
      }
      CompleteCode(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeCP(Word Code)
 * \brief  decode CP(W) instructions
 * \param  IsCPW 1 if CPW
 * ------------------------------------------------------------------------ */

static void DecodeCP(Word IsCPW)
{
  LongWord Mask;

  if (!ChkArgCnt(2, 2));
  else if (IsCPW && (pCurrCPUProps->Core != eCoreSTM8)) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    tAdrVals SrcAdrVals;
    tAdrMode DestMode;

    if (DecodeReg(&ArgStr[1], &DestMode, (IsCPW ? 0 : MModA) | MModX | MModY))
    switch (DestMode)
    {
      case eModA:
        Mask = ConstructMask(MModImm | MModAbs8 | MModAbs16
                           | MModIX | MModIX8 | MModIX16 | MModIY | MModIY8 | MModIY16 | MModISP8
                           | MModIAbs8 | MModIAbs16 | MModI16Abs16
                           | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs8 | MModIYAbs16, eSymbolSize8Bit);
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = 0x01 + (SrcAdrVals.Part << 4);
          CompleteCode(&SrcAdrVals);
        }
        break;
      case eModX:
        Mask = MModImm | MModAbs8 | MModAbs16 | MModIAbs16;
        if (pCurrCPUProps->Core == eCoreST7)
          Mask |= MModIAbs8 | MModIXAbs8 | MModIX | MModIX8 | MModIX16 | MModIXAbs16;
        if (pCurrCPUProps->Core == eCoreSTM8)
        {
          Mask |= MModIY | MModIY8 | MModIY16 | MModISP8 | MModI16Abs16 | MModIYAbs16;
          OpSize = eSymbolSize16Bit;
        }
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = 0x03 + (SrcAdrVals.Part << 4);
          CompleteCode(&SrcAdrVals);
        }
        break;
      case eModY:
        PrefixCnt = 0;
        Mask = MModImm | MModAbs8 | MModAbs16 | MModIAbs16;
        if (pCurrCPUProps->Core == eCoreST7)
          Mask |= MModIAbs8 | MModIYAbs8 | MModIY | MModIY8 | MModIY16 | MModIYAbs16;
        if (pCurrCPUProps->Core == eCoreSTM8)
        {
          Mask |= MModIX | MModIX8 | MModIX16 | MModIXAbs16 | MModI16XAbs16;
          OpSize = eSymbolSize16Bit;
        }
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        switch (SrcAdrVals.Mode)
        {
          case eModIXAbs16:
          case eModIX:
          case eModIX8:
          case eModIX16:
            goto common;
          default:
            if (PrefixCnt == 0) AddPrefix(0x90);
            if (BAsmCode[0] == 0x92) BAsmCode[0]--;
          common:
            BAsmCode[PrefixCnt] = 0x03 + (SrcAdrVals.Part << 4);
            CompleteCode(&SrcAdrVals);
        }
        break;
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAri16(Word Code)
 * \brief  decode 16-bit arithmetic/logic instructions with two operands
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeAri16(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[1], MModX | MModY | MModS, False, &DestAdrVals))
  switch (DestAdrVals.Mode)
  {
    case eModX:
    case eModY:
      OpSize = eSymbolSize16Bit;
      if (DecodeAdr(&ArgStr[2], MModImm | MModAbs16 | MModISP8, False, &SrcAdrVals))
      switch (SrcAdrVals.Mode)
      {
        case eModImm:
          if (PrefixCnt)
          {
            BAsmCode[PrefixCnt - 1] = 0x72;
            BAsmCode[PrefixCnt] = 0xa0 | Hi(Code);
          }
          else
            BAsmCode[PrefixCnt] = 0x1d - !!Lo(Code);
          goto common;
        case eModAbs16:
          if (PrefixCnt)
          {
            BAsmCode[PrefixCnt - 1] = 0x72;
            BAsmCode[PrefixCnt] = 0xb0 | Hi(Code);
          }
          else
          {
            BAsmCode[PrefixCnt++] = 0x72;
            BAsmCode[PrefixCnt] = 0xb0 | Lo(Code);
          }
          goto common;
        case eModISP8:
          if (PrefixCnt)
          {
            BAsmCode[PrefixCnt - 1] = 0x72;
            BAsmCode[PrefixCnt] = 0xf0 | Hi(Code);
          }
          else
          {
            BAsmCode[PrefixCnt++] = 0x72;
            BAsmCode[PrefixCnt] = 0xf0 | Lo(Code);
          }
          goto common;
        common:
          CompleteCode(&SrcAdrVals);
          break;
        default:
          break;
      }
      break;
    case eModS:
      OpSize = eSymbolSize8Bit;
      if (DecodeAdr(&ArgStr[2], MModImm, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x52 | Lo(Code);
        BAsmCode[1] = SrcAdrVals.Vals[0];
        CodeLen = 2;
      }
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeAri(Word Code)
 * \brief  decode 8-bit arithmetic/logic instructions with two operands
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeAri(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  LongWord Mask;
  Word Code16 = 0;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    Mask = MModA;
    if (pCurrCPUProps->Core == eCoreSTM8)
      switch (Lo(Code))
      {
        case 0x00: /* SUB->SUBW */
          Code16 = 0x0200;
          Mask |= MModS | MModX | MModY;
          break;
        case 0x0b: /* ADD->ADDW */
          Code16 = 0x090b;
          Mask |= MModS | MModX | MModY;
          break;
      }
    if (DecodeAdr(&ArgStr[1], Mask, False, &DestAdrVals))
    switch (DestAdrVals.Mode)
    {
      case eModA:
        Mask = MModAbs8 | MModAbs16 | MModIX | MModIX8 | MModIX16 | MModIY |
               MModIY8 | MModIY16 | MModIAbs16 | MModIXAbs16 | MModIYAbs16;
        if (pCurrCPUProps->Core == eCoreST7)
          Mask |= MModIAbs8 | MModIXAbs8 | MModIYAbs8;
        if (pCurrCPUProps->Core == eCoreSTM8)
          Mask |= MModISP8 | MModI16Abs16 | MModI16XAbs16;
        if (Hi(Code)) Mask |= MModImm;
        if (DecodeAdr(&ArgStr[2], Mask, False, &SrcAdrVals))
        {
          BAsmCode[PrefixCnt] = Lo(Code) + (SrcAdrVals.Part << 4);
          CompleteCode(&SrcAdrVals);
        }
        break;
      case eModS:
      case eModX:
      case eModY:
        PrefixCnt = 0;
        DecodeAri16(Code16);
      default:
        break;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRMW(Word Code)
 * \brief  decode 8-bit read/modify/write instructions with one operand
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeRMW(Word Code)
{
  Boolean IsW = Hi(Code);

  Code = Lo(Code);
  if (!ChkArgCnt(1, 1));
  else if (IsW && (pCurrCPUProps->Core != eCoreSTM8)) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    LongWord Mask;
    tAdrVals AdrVals;

    Mask = MModX | MModY;
    if (!IsW)
    {
      Mask |= MModA |  MModAbs8
            | MModIX | MModIX8 | MModIY | MModIY8;
      if (pCurrCPUProps->Core == eCoreST7)
        Mask |= MModIAbs8 | MModIXAbs8 | MModIYAbs8;
      if (pCurrCPUProps->Core == eCoreSTM8)
        Mask |= MModAbs16 | MModIX16 | MModIY16 | MModISP8
              | MModIAbs16 | MModI16Abs16 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs16;
    }
    if (DecodeAdr(&ArgStr[1], Mask, False, &AdrVals))
    switch (AdrVals.Mode)
    {
      case eModA:
        BAsmCode[PrefixCnt] = 0x40 + Code;
        CodeLen = PrefixCnt + 1;
        break;
      case eModX:
      case eModY:
        BAsmCode[PrefixCnt] = 0x50 + Code;
        CodeLen = PrefixCnt + 1;
        break;
      case eModAbs16:
        BAsmCode[PrefixCnt++] = 0x72;
        BAsmCode[PrefixCnt] = 0x50 | Code;
        goto common;
      case eModIX16:
        BAsmCode[PrefixCnt++] = 0x72;
        BAsmCode[PrefixCnt] = 0x40 | Code;
        goto common;
      case eModIY16:
        BAsmCode[PrefixCnt] = 0x40 | Code;
        goto common;
      case eModISP8:
        BAsmCode[PrefixCnt] = 0x00 | Code;
        goto common;
      case eModIAbs16:
      case eModI16Abs16:
        BAsmCode[PrefixCnt] = 0x30 | Code;
        goto common;
      case eModIXAbs16:
      case eModI16XAbs16:
      case eModIYAbs16:
        BAsmCode[PrefixCnt] = 0x60 | Code;
        goto common;
      default:
        BAsmCode[PrefixCnt] = Code + ((AdrVals.Part - 8) << 4);
        /* fall-through */
      common:
        CompleteCode(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeMUL(Word Code)
 * \brief  decode MUL instruction
 * ------------------------------------------------------------------------ */

static void DecodeMUL(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    tAdrMode SrcMode;
    tAdrVals DestVals;

    if (DecodeReg(&ArgStr[2], &SrcMode, MModA)
     && DecodeAdr(&ArgStr[1], MModX | MModY, False, &DestVals))
    {
      BAsmCode[PrefixCnt] = 0x42;
      CodeLen = PrefixCnt + 1;
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeDIV(Word Code)
 * \brief  decode DIV(W) instructions
 * \param  IsW True if DIVW
 * ------------------------------------------------------------------------ */

static void DecodeDIV(Word IsW)
{
  tAdrVals DestAdrVals, SrcAdrVals;

  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[1], MModX | (IsW ? 0 : MModY), False, &DestAdrVals)
        && DecodeAdr(&ArgStr[2], MModY | (IsW ? 0 : MModA), False, &SrcAdrVals))
  {
    BAsmCode[PrefixCnt] = (SrcAdrVals.Mode == eModA) ? 0x62 : 0x65;
    CodeLen = PrefixCnt + 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeEXG(Word Code)
 * \brief  decode EXG(W) instructions
 * \param  IsW True if EXGW
 * ------------------------------------------------------------------------ */

static void DecodeEXG(Word IsW)
{
  tAdrVals DestAdrVals, SrcAdrVals;

  if (!ChkArgCnt(2, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[1], (IsW ? 0 : (MModA | MModXL | MModYL | MModAbs16)) | MModX | MModY, False, &DestAdrVals))
  switch (DestAdrVals.Mode)
  {
    case eModA:
      if (DecodeAdr(&ArgStr[2], MModXL | MModYL | MModAbs16, False, &SrcAdrVals))
      switch (SrcAdrVals.Mode)
      {
        case eModXL:
          BAsmCode[0] = 0x41;
          CodeLen = 1;
          break;
        case eModYL:
          BAsmCode[0] = 0x61;
          CodeLen = 1;
          break;
        case eModAbs16:
          BAsmCode[0] = 0x31;
          CompleteCode(&SrcAdrVals);
          break;
        default:
          break;
      }
      break;
    case eModXL:
      if (DecodeAdr(&ArgStr[2], MModA, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x41;
        CodeLen = 1;
      }
      break;
    case eModYL:
      if (DecodeAdr(&ArgStr[2], MModA, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x61;
        CodeLen = 1;
      }
      break;
    case eModAbs16:
      if (DecodeAdr(&ArgStr[2], MModA, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x31;
        memcpy(&BAsmCode[1], SrcAdrVals.Vals, SrcAdrVals.Cnt);
        CodeLen = 3;
      }
      break;
    case eModX:
      if (DecodeAdr(&ArgStr[2], MModY, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x51;
        CodeLen = 1;
      }
      break;
    case eModY:
      if (DecodeAdr(&ArgStr[2], MModX, False, &SrcAdrVals))
      {
        BAsmCode[0] = 0x51;
        CodeLen = 1;
      }
      break;
    default:
      break;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitOp(Word Code)
 * \brief  decode bit operation instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBitOp(Word Code)
{
  Byte BitPos;
  tAdrVals AdrVals;

  if (!ChkArgCnt(1, 2));
  else if ((Hi(Code == 0x90)) && (pCurrCPUProps->Core != eCoreSTM8)) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeBitAdrWithIndir(1, ArgCnt, &BitPos, &AdrVals))
  {
    if (pCurrCPUProps->Core == eCoreSTM8)
    {
      PrefixCnt = 0;
      AddPrefix(Hi(Code));
      BAsmCode[PrefixCnt] = 0x10 | (BitPos << 1) | (Code & 1);
    }
    else
      BAsmCode[PrefixCnt] = Lo(Code) + (BitPos << 1);
    CompleteCode(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBTJF_BTJT(Word Code)
 * \brief  decode BTJF/BTJT instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeBTJF_BTJT(Word Code)
{
  Byte BitPos;
  tAdrVals AdrVals;

  if (!ChkArgCnt(2, 3));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeBitAdrWithIndir(1, ArgCnt - 1, &BitPos, &AdrVals))
  {
    Integer AdrInt;
    Boolean OK;
    tSymbolFlags Flags;

    if (pCurrCPUProps->Core == eCoreSTM8)
    {
      PrefixCnt = 0;
      AddPrefix(0x72);
    }
    BAsmCode[PrefixCnt] = Code + (BitPos << 1);
    memcpy(BAsmCode + 1 + PrefixCnt, AdrVals.Vals, AdrVals.Cnt);
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], pCurrCPUProps->AddrIntType, &OK, &Flags) - (EProgCounter() + PrefixCnt + 1 + AdrVals.Cnt + 1);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[3]);
      else
      {
        BAsmCode[PrefixCnt + 1 + AdrVals.Cnt] = AdrInt & 0xff;
        CodeLen = PrefixCnt + 1 + AdrVals.Cnt + 1;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJP_CALL(Word Code)
 * \brief  decode JP/CALL instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeJP_CALL(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else
  {
    LongWord Mask;
    tAdrVals AdrVals;

    Mask = ConstructMask(MModAbs8 | MModAbs16 | MModIX | MModIX8 | MModIX16 | MModIY
                       | MModIY8 | MModIY16 | MModIAbs8 | MModIAbs16 | MModI16Abs16
                       | MModIXAbs8 | MModIXAbs16 | MModI16XAbs16 | MModIYAbs8 | MModIYAbs16,
                         eSymbolSizeUnknown);
    if (DecodeAdr(&ArgStr[1], Mask, True, &AdrVals))
    {
      BAsmCode[PrefixCnt] = Code + (AdrVals.Part << 4);
      CompleteCode(&AdrVals);
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeJPF_CALLF(Word Code)
 * \brief  decode JPF/CALLF instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeJPF_CALLF(Word Code)
{
  tAdrVals AdrVals;

  if (!ChkArgCnt(1, 1));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (DecodeAdr(&ArgStr[1], MModAbs24 | MModI16Abs24, True, &AdrVals))
  {
    BAsmCode[PrefixCnt] = Code;
    CompleteCode(&AdrVals);
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRel(Word Code)
 * \brief  decode relative branch instructions
 * \param  instruction code
 * ------------------------------------------------------------------------ */

static void DecodeRel(Word Code)
{
  if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (!Code) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str == '[')
  {
    if (pCurrCPUProps->Core != eCoreST7) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
    {
      tAdrVals AdrVals;

      if (DecodeAdr(&ArgStr[1], MModIAbs8, False, &AdrVals))
      {
        BAsmCode[PrefixCnt] = Lo(Code);
        CompleteCode(&AdrVals);
      }
    }
  }
  else
  {
    Boolean OK;
    Integer AdrInt;
    tSymbolFlags Flags;

    if (Hi(Code))
      BAsmCode[PrefixCnt++] = Hi(Code);
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], pCurrCPUProps->AddrIntType, &OK, &Flags) - (EProgCounter() + 2 + PrefixCnt);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrStrErrorPos(ErrNum_JmpDistTooBig, &ArgStr[1]);
      else
      {
        BAsmCode[PrefixCnt] = Lo(Code);
        BAsmCode[PrefixCnt + 1] = AdrInt & 0xff;
        CodeLen = PrefixCnt + 2;
      }
    }
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeRxWA(Word Code)
 * \brief  decode RLWA/RRWA instructions
 * \param  Code instruction code
 * ------------------------------------------------------------------------ */

static void DecodeRxWA(Word Code)
{
  tAdrMode SrcMode;
  tAdrVals DestAdrVals;

  if (*AttrPart.str.p_str) WrStrErrorPos(ErrNum_UseLessAttr, &AttrPart);
  else if (!ChkArgCnt(1, 2));
  else if (pCurrCPUProps->Core != eCoreSTM8) WrStrErrorPos(ErrNum_InstructionNotSupported, &OpPart);
  else if (((ArgCnt == 1) || DecodeReg(&ArgStr[2], &SrcMode, MModA)) && DecodeAdr(&ArgStr[1], MModX | MModY, False, &DestAdrVals))
  {
    BAsmCode[PrefixCnt] = Code;
    CodeLen = PrefixCnt + 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBIT(Word Code)
 * \brief  decode BIT instruction
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
    BitPos = EvalBitPosition(&ArgStr[2], &OK);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = eSymbolSize8Bit;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandST7Bit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (DecodeBitArg(&BitSpec, 1, ArgCnt))
    {
      *ListLine = '=';
      DissectBit_ST7(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InitFields(void)
 * \brief  build up hash table of instructions
 * ------------------------------------------------------------------------ */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddAri(const char *NName, Word NCode, Boolean NMay)
{
  AddInstTable(InstTable, NName, NCode | (NMay << 8), DecodeAri);
}

static void AddAri16(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAri16);
}

static void AddRMW(const char *NName, Byte NCode)
{
  char WName[10];

  AddInstTable(InstTable, NName, NCode, DecodeRMW);
  as_snprintf(WName, sizeof(WName), "%sW", NName);
  AddInstTable(InstTable, WName, NCode | 0x100, DecodeRMW);
}

static void AddRel(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "LDF", 0, DecodeLDF);
  AddInstTable(InstTable, "LDW", 0, DecodeLDW);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "PUSH", 0x0004, DecodePUSH_POP);
  AddInstTable(InstTable, "POP", 0x0000, DecodePUSH_POP);
  AddInstTable(InstTable, "PUSHW", 0x0104, DecodePUSH_POP);
  AddInstTable(InstTable, "POPW", 0x0100, DecodePUSH_POP);
  AddInstTable(InstTable, "CP", 0, DecodeCP);
  AddInstTable(InstTable, "CPW", 1, DecodeCP);
  AddInstTable(InstTable, "MUL", 0, DecodeMUL);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "DIVW", 1, DecodeDIV);
  AddInstTable(InstTable, "EXG", 0, DecodeEXG);
  AddInstTable(InstTable, "EXGW", 1, DecodeEXG);
  AddInstTable(InstTable, "RLWA", 0x02, DecodeRxWA);
  AddInstTable(InstTable, "RRWA", 0x01, DecodeRxWA);
  AddInstTable(InstTable, "BCCM", 0x9001, DecodeBitOp);
  AddInstTable(InstTable, "BCPL", 0x9000, DecodeBitOp);
  AddInstTable(InstTable, "BRES", 0x7211, DecodeBitOp);
  AddInstTable(InstTable, "BSET", 0x7210, DecodeBitOp);
  AddInstTable(InstTable, "BTJF", 0x01, DecodeBTJF_BTJT);
  AddInstTable(InstTable, "BTJT", 0x00, DecodeBTJF_BTJT);
  AddInstTable(InstTable, "JP", 0x0c, DecodeJP_CALL);
  AddInstTable(InstTable, "CALL", 0x0d, DecodeJP_CALL);
  AddInstTable(InstTable, "JPF", 0xac, DecodeJPF_CALLF);
  AddInstTable(InstTable, "CALLF", 0x8d, DecodeJPF_CALLF);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);

  AddFixed("HALT" , 0x8e); AddFixed("IRET" , 0x80); AddFixed("NOP"  , 0x9d);
  AddFixed("RCF"  , 0x98); AddFixed("RET"  , 0x81); AddFixed("RIM"  , 0x9a);
  AddFixed("RSP"  , 0x9c); AddFixed("SCF"  , 0x99); AddFixed("SIM"  , 0x9b);
  AddFixed("TRAP" , 0x83); AddFixed("WFI"  , 0x8f); AddFixed("BREAK", 0x018b);
  AddFixed("WFE"  , 0x728f); AddFixed("RVF"  , 0x019c); AddFixed("RETF", 0x0187);
  AddFixed("CCF"  , 0x018c);

  AddAri("ADC" , 0x09, True ); AddAri("ADD" , 0x0b, True ); AddAri("AND" , 0x04, True );
  AddAri("BCP" , 0x05, True ); AddAri("OR"  , 0x0a, True ); AddAri("SBC" , 0x02, True );
  AddAri("SUB" , 0x00, True ); AddAri("XOR" , 0x08, True );

  AddAri16("ADDW", 0x090b); AddAri16("SUBW", 0x0200);

  AddRMW("CLR" , 0x0f); AddRMW("CPL" , 0x03); AddRMW("DEC" , 0x0a);
  AddRMW("INC" , 0x0c); AddRMW("NEG" , 0x00); AddRMW("RLC" , 0x09);
  AddRMW("RRC" , 0x06); AddRMW("SLA" , 0x08); AddRMW("SLL" , 0x08);
  AddRMW("SRA" , 0x07); AddRMW("SRL" , 0x04); AddRMW("SWAP", 0x0e);
  AddRMW("TNZ" , 0x0d);

  AddRel("CALLR", 0xad);
  AddRel("JRA"  , 0x20);
  AddRel("JRC"  , 0x25);
  AddRel("JREQ" , 0x27);
  AddRel("JRF"  , 0x21);
  AddRel("JRH"  , (pCurrCPUProps->Core == eCoreSTM8) ? 0x9029 : 0x29);
  AddRel("JRIH" , (pCurrCPUProps->Core == eCoreSTM8) ? 0x902f : 0x2f);
  AddRel("JRIL" , (pCurrCPUProps->Core == eCoreSTM8) ? 0x902e : 0x2e);
  AddRel("JRM"  , (pCurrCPUProps->Core == eCoreSTM8) ? 0x902d : 0x2d);
  AddRel("JRMI" , 0x2b);
  AddRel("JRNC" , 0x24);
  AddRel("JRNE" , 0x26);
  AddRel("JRNH" , (pCurrCPUProps->Core == eCoreSTM8) ? 0x9028 : 0x28);
  AddRel("JRNM" , (pCurrCPUProps->Core == eCoreSTM8) ? 0x902c : 0x2c);
  AddRel("JRNV" , (pCurrCPUProps->Core == eCoreSTM8) ? 0x28 : 0x00);
  AddRel("JRPL" , 0x2a);
  AddRel("JRSGE", (pCurrCPUProps->Core == eCoreSTM8) ? 0x2e : 0x00);
  AddRel("JRSGT", (pCurrCPUProps->Core == eCoreSTM8) ? 0x2c : 0x00);
  AddRel("JRSLE", (pCurrCPUProps->Core == eCoreSTM8) ? 0x2d : 0x00);
  AddRel("JRSLT", (pCurrCPUProps->Core == eCoreSTM8) ? 0x2f : 0x00);
  AddRel("JRT"  , 0x20);
  AddRel("JRUGE", 0x24);
  AddRel("JRUGT", 0x22);
  AddRel("JRULE", 0x23);
  AddRel("JRULT", 0x25);
  AddRel("JRV"  , (pCurrCPUProps->Core == eCoreSTM8) ? 0x29 : 0x00);

  init_moto8_pseudo(InstTable, e_moto_8_be);
}

/*!------------------------------------------------------------------------
 * \fn     DeinitFields(void)
 * \brief  tear down instruction hash table
 * ------------------------------------------------------------------------ */

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*!------------------------------------------------------------------------
 * \fn     MakeCode_ST7(void)
 * \brief  entry point to decode machine instructions
 * ------------------------------------------------------------------------ */

static Boolean DecodeAttrPart_ST7(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }
  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_ST7(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSize8Bit;
  PrefixCnt = 0;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize,True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

/*!------------------------------------------------------------------------
 * \fn     IsDef_ST7(void)
 * \brief  does instruction consume label field?
 * \return true if to be consumed
 * ------------------------------------------------------------------------ */

static Boolean IsDef_ST7(void)
{
  return Memo("BIT");
}

/*!------------------------------------------------------------------------
 * \fn     SwitchTo_ST7(void)
 * \brief  switch to target
 * ------------------------------------------------------------------------ */

static void SwitchTo_ST7(void *pUser)
{
  pCurrCPUProps = (const tCPUProps*)pUser;
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "PC"; HeaderID = 0x33; NOPCode = 0x9d;
  DivideChars = ","; HasAttrs = True; AttrChars = ".";


  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[pCurrCPUProps->AddrIntType].Max;

  DecodeAttrPart = DecodeAttrPart_ST7;
  MakeCode = MakeCode_ST7;
  IsDef = IsDef_ST7;
  SwitchFrom = DeinitFields;
  DissectBit = DissectBit_ST7;
  InitFields();
  AddMoto16PseudoONOFF(False);
}

/*!------------------------------------------------------------------------
 * \fn     codest7_init(void)
 * \brief  register ST7/STM8 target
 * ------------------------------------------------------------------------ */

static const tCPUProps CPUProps[] =
{
  { "ST7"         ,  Int16, eCoreST7  },
  { "ST7232AK1"   ,  Int16, eCoreST7  },
  { "ST7232AK2"   ,  Int16, eCoreST7  },
  { "ST7232AJ1"   ,  Int16, eCoreST7  },
  { "ST7232AJ2"   ,  Int16, eCoreST7  },
  { "ST72251G1"   ,  Int16, eCoreST7  },
  { "ST72251G2"   ,  Int16, eCoreST7  },
  { "ST72311J2"   ,  Int16, eCoreST7  },
  { "ST72311J4"   ,  Int16, eCoreST7  },
  { "ST72321BR6"  ,  Int16, eCoreST7  },
  { "ST72321BR7"  ,  Int16, eCoreST7  },
  { "ST72321BR9"  ,  Int16, eCoreST7  },
  { "ST72324J6"   ,  Int16, eCoreST7  },
  { "ST72324K6"   ,  Int16, eCoreST7  },
  { "ST72324J4"   ,  Int16, eCoreST7  },
  { "ST72324K4"   ,  Int16, eCoreST7  },
  { "ST72324J2"   ,  Int16, eCoreST7  },
  { "ST72324K2"   ,  Int16, eCoreST7  },
  { "ST72325S4"   ,  Int16, eCoreST7  },
  { "ST72325S6"   ,  Int16, eCoreST7  },
  { "ST72325J7"   ,  Int16, eCoreST7  },
  { "ST72325R9"   ,  Int16, eCoreST7  },
  { "ST72344K2"   ,  Int16, eCoreST7  },
  { "ST72344K4"   ,  Int16, eCoreST7  },
  { "ST72345C4"   ,  Int16, eCoreST7  },
  { "ST72521BR6"  ,  Int16, eCoreST7  },
  { "ST72521BM9"  ,  Int16, eCoreST7  },
  { "ST72361AR4"  ,  Int16, eCoreST7  },
  { "ST72361AR6"  ,  Int16, eCoreST7  },
  { "ST72361AR7"  ,  Int16, eCoreST7  },
  { "ST72361AR9"  ,  Int16, eCoreST7  },
  { "ST7FOXK1"    ,  Int16, eCoreST7  },
  { "ST7FOXK2"    ,  Int16, eCoreST7  },
  { "ST7LITES2Y0" ,  Int16, eCoreST7  },
  { "ST7LITES5Y0" ,  Int16, eCoreST7  },
  { "ST7LITE02Y0" ,  Int16, eCoreST7  },
  { "ST7LITE05Y0" ,  Int16, eCoreST7  },
  { "ST7LITE09Y0" ,  Int16, eCoreST7  },
  { "ST7LITE10F1" ,  Int16, eCoreST7  },
  { "ST7LITE15F1" ,  Int16, eCoreST7  },
  { "ST7LITE19F1" ,  Int16, eCoreST7  },
  { "ST7LITE10BF0",  Int16, eCoreST7  },
  { "ST7LITE15BF0",  Int16, eCoreST7  },
  { "ST7LITE15BF1",  Int16, eCoreST7  },
  { "ST7LITE19BF0",  Int16, eCoreST7  },
  { "ST7LITE19BF1",  Int16, eCoreST7  },
  { "ST7LITE20F2" ,  Int16, eCoreST7  },
  { "ST7LITE25F2" ,  Int16, eCoreST7  },
  { "ST7LITE29F2" ,  Int16, eCoreST7  },
  { "ST7LITE30F2" ,  Int16, eCoreST7  },
  { "ST7LITE35F2" ,  Int16, eCoreST7  },
  { "ST7LITE39F2" ,  Int16, eCoreST7  },
  { "ST7LITE49K2" ,  Int16, eCoreST7  },
  { "ST7MC1K2"    ,  Int16, eCoreST7  },
  { "ST7MC1K4"    ,  Int16, eCoreST7  },
  { "ST7MC2N6"    ,  Int16, eCoreST7  },
  { "ST7MC2S4"    ,  Int16, eCoreST7  },
  { "ST7MC2S6"    ,  Int16, eCoreST7  },
  { "ST7MC2S7"    ,  Int16, eCoreST7  },
  { "ST7MC2S9"    ,  Int16, eCoreST7  },
  { "ST7MC2R6"    ,  Int16, eCoreST7  },
  { "ST7MC2R7"    ,  Int16, eCoreST7  },
  { "ST7MC2R9"    ,  Int16, eCoreST7  },
  { "ST7MC2M9"    ,  Int16, eCoreST7  },

  { "STM8"        ,  Int24, eCoreSTM8 },
  { "STM8S001J3"  ,  Int16, eCoreSTM8 },
  { "STM8S003F3"  ,  Int16, eCoreSTM8 },
  { "STM8S003K3"  ,  Int16, eCoreSTM8 },
  { "STM8S005C6"  ,  Int16, eCoreSTM8 },
  { "STM8S005K6"  ,  Int16, eCoreSTM8 },
  { "STM8S007C8"  ,  Int24, eCoreSTM8 },
  { "STM8S103F2"  ,  Int16, eCoreSTM8 },
  { "STM8S103F3"  ,  Int16, eCoreSTM8 },
  { "STM8S103K3"  ,  Int16, eCoreSTM8 },
  { "STM8S105C4"  ,  Int16, eCoreSTM8 },
  { "STM8S105C6"  ,  Int16, eCoreSTM8 },
  { "STM8S105K4"  ,  Int16, eCoreSTM8 },
  { "STM8S105K6"  ,  Int16, eCoreSTM8 },
  { "STM8S105S4"  ,  Int16, eCoreSTM8 },
  { "STM8S105S6"  ,  Int16, eCoreSTM8 },
  { "STM8S207MB"  ,  Int24, eCoreSTM8 },
  { "STM8S207M8"  ,  Int24, eCoreSTM8 },
  { "STM8S207RB"  ,  Int24, eCoreSTM8 },
  { "STM8S207R8"  ,  Int24, eCoreSTM8 },
  { "STM8S207R6"  ,  Int16, eCoreSTM8 },
  { "STM8S207CB"  ,  Int24, eCoreSTM8 },
  { "STM8S207C8"  ,  Int24, eCoreSTM8 },
  { "STM8S207C6"  ,  Int16, eCoreSTM8 },
  { "STM8S207SB"  ,  Int24, eCoreSTM8 },
  { "STM8S207S8"  ,  Int24, eCoreSTM8 },
  { "STM8S207S6"  ,  Int16, eCoreSTM8 },
  { "STM8S207K8"  ,  Int24, eCoreSTM8 },
  { "STM8S207K6"  ,  Int16, eCoreSTM8 },
  { "STM8S208MB"  ,  Int24, eCoreSTM8 },
  { "STM8S208RB"  ,  Int24, eCoreSTM8 },
  { "STM8S208R8"  ,  Int24, eCoreSTM8 },
  { "STM8S208R6"  ,  Int24, eCoreSTM8 },
  { "STM8S208CB"  ,  Int24, eCoreSTM8 },
  { "STM8S208C8"  ,  Int24, eCoreSTM8 },
  { "STM8S208C6"  ,  Int16, eCoreSTM8 },
  { "STM8S208SB"  ,  Int24, eCoreSTM8 },
  { "STM8S208S8"  ,  Int24, eCoreSTM8 },
  { "STM8S208S6"  ,  Int16, eCoreSTM8 },
  { "STM8S903K3"  ,  Int16, eCoreSTM8 },
  { "STM8S903F3"  ,  Int16, eCoreSTM8 },
  { "STM8L050J3"  ,  Int16, eCoreSTM8 },
  { "STM8L051F3"  ,  Int16, eCoreSTM8 },
  { "STM8L052C6"  ,  Int16, eCoreSTM8 },
  { "STM8L052R8"  ,  Int24, eCoreSTM8 },
  { "STM8L001J3"  ,  Int16, eCoreSTM8 },
  { "STM8L101F1"  ,  Int16, eCoreSTM8 },
  { "STM8L101F2"  ,  Int16, eCoreSTM8 },
  { "STM8L101G2"  ,  Int16, eCoreSTM8 },
  { "STM8L101F3"  ,  Int16, eCoreSTM8 },
  { "STM8L101G3"  ,  Int16, eCoreSTM8 },
  { "STM8L101K3"  ,  Int16, eCoreSTM8 },
  { "STM8L151C2"  ,  Int16, eCoreSTM8 },
  { "STM8L151K2"  ,  Int16, eCoreSTM8 },
  { "STM8L151G2"  ,  Int16, eCoreSTM8 },
  { "STM8L151F2"  ,  Int16, eCoreSTM8 },
  { "STM8L151C3"  ,  Int16, eCoreSTM8 },
  { "STM8L151K3"  ,  Int16, eCoreSTM8 },
  { "STM8L151G3"  ,  Int16, eCoreSTM8 },
  { "STM8L151F3"  ,  Int16, eCoreSTM8 },
  { "STM8L151C4"  ,  Int16, eCoreSTM8 },
  { "STM8L151C6"  ,  Int16, eCoreSTM8 },
  { "STM8L151K4"  ,  Int16, eCoreSTM8 },
  { "STM8L151K6"  ,  Int16, eCoreSTM8 },
  { "STM8L151G4"  ,  Int16, eCoreSTM8 },
  { "STM8L151G6"  ,  Int16, eCoreSTM8 },
  { "STM8L152C4"  ,  Int16, eCoreSTM8 },
  { "STM8L152C6"  ,  Int16, eCoreSTM8 },
  { "STM8L152K4"  ,  Int16, eCoreSTM8 },
  { "STM8L152K6"  ,  Int16, eCoreSTM8 },
  { "STM8L151R6"  ,  Int16, eCoreSTM8 },
  { "STM8L151C8"  ,  Int24, eCoreSTM8 },
  { "STM8L151M8"  ,  Int16, eCoreSTM8 },
  { "STM8L151R8"  ,  Int24, eCoreSTM8 },
  { "STM8L152R6"  ,  Int16, eCoreSTM8 },
  { "STM8L152C8"  ,  Int24, eCoreSTM8 },
  { "STM8L152K8"  ,  Int24, eCoreSTM8 },
  { "STM8L152M8"  ,  Int24, eCoreSTM8 },
  { "STM8L152R8"  ,  Int24, eCoreSTM8 },
  { "STM8L162M8"  ,  Int24, eCoreSTM8 },
  { "STM8L162R8"  ,  Int24, eCoreSTM8 },
  { "STM8AF6366"  ,  Int16, eCoreSTM8 },
  { "STM8AF6388"  ,  Int24, eCoreSTM8 },
  { "STM8AF6213"  ,  Int16, eCoreSTM8 },
  { "STM8AF6223"  ,  Int16, eCoreSTM8 },
  { "STM8AF6226"  ,  Int16, eCoreSTM8 },
  { "STM8AF6246"  ,  Int16, eCoreSTM8 },
  { "STM8AF6248"  ,  Int16, eCoreSTM8 },
  { "STM8AF6266"  ,  Int16, eCoreSTM8 },
  { "STM8AF6268"  ,  Int16, eCoreSTM8 },
  { "STM8AF6269"  ,  Int16, eCoreSTM8 },
  { "STM8AF6286"  ,  Int24, eCoreSTM8 },
  { "STM8AF6288"  ,  Int24, eCoreSTM8 },
  { "STM8AF6289"  ,  Int24, eCoreSTM8 },
  { "STM8AF628A"  ,  Int24, eCoreSTM8 },
  { "STM8AF62A6"  ,  Int24, eCoreSTM8 },
  { "STM8AF62A8"  ,  Int24, eCoreSTM8 },
  { "STM8AF62A9"  ,  Int24, eCoreSTM8 },
  { "STM8AF62AA"  ,  Int24, eCoreSTM8 },
  { "STM8AF5268"  ,  Int16, eCoreSTM8 },
  { "STM8AF5269"  ,  Int16, eCoreSTM8 },
  { "STM8AF5286"  ,  Int24, eCoreSTM8 },
  { "STM8AF5288"  ,  Int24, eCoreSTM8 },
  { "STM8AF5289"  ,  Int24, eCoreSTM8 },
  { "STM8AF528A"  ,  Int24, eCoreSTM8 },
  { "STM8AF52A6"  ,  Int24, eCoreSTM8 },
  { "STM8AF52A8"  ,  Int24, eCoreSTM8 },
  { "STM8AF52A9"  ,  Int24, eCoreSTM8 },
  { "STM8AF52AA"  ,  Int24, eCoreSTM8 },
  { "STM8AL3136"  ,  Int16, eCoreSTM8 },
  { "STM8AL3138"  ,  Int16, eCoreSTM8 },
  { "STM8AL3146"  ,  Int16, eCoreSTM8 },
  { "STM8AL3148"  ,  Int16, eCoreSTM8 },
  { "STM8AL3166"  ,  Int16, eCoreSTM8 },
  { "STM8AL3168"  ,  Int16, eCoreSTM8 },
  { "STM8AL3L46"  ,  Int16, eCoreSTM8 },
  { "STM8AL3L48"  ,  Int16, eCoreSTM8 },
  { "STM8AL3L66"  ,  Int16, eCoreSTM8 },
  { "STM8AL3L68"  ,  Int16, eCoreSTM8 },
  { "STM8AL3188"  ,  Int24, eCoreSTM8 },
  { "STM8AL3189"  ,  Int24, eCoreSTM8 },
  { "STM8AL318A"  ,  Int24, eCoreSTM8 },
  { "STM8AL3L88"  ,  Int24, eCoreSTM8 },
  { "STM8AL3L89"  ,  Int24, eCoreSTM8 },
  { "STM8AL3L8A"  ,  Int24, eCoreSTM8 },
  { "STM8TL52F4"  ,  Int16, eCoreSTM8 },
  { "STM8TL52G4"  ,  Int16, eCoreSTM8 },
  { "STM8TL53C4"  ,  Int16, eCoreSTM8 },
  { "STM8TL53F4"  ,  Int16, eCoreSTM8 },
  { "STM8TL53G4"  ,  Int16, eCoreSTM8 },
  { NULL          ,  UInt1, eCoreST7  },
};

void codest7_init(void)
{
  const tCPUProps *pProp;

  for (pProp = CPUProps; pProp->pName; pProp++)
    (void)AddCPUUser(pProp->pName, SwitchTo_ST7, (void*)pProp, NULL);
}
