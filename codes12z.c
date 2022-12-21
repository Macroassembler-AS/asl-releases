/* code68s12z.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS                                                                        */
/*                                                                           */
/* Code Generator NXP S12Z                                                   */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>

#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmallg.h"
#include "asmitree.h"
#include "asmstructs.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"
#include "headids.h"

#include "codes12z.h"

typedef enum
{
  AdrModeNone = -1,
  AdrModeReg = 0,
  AdrModeAReg = 1,
  AdrModeImm = 2,
  AdrModeMemReg = 3
} tAdrMode;

typedef enum
{
  eIndirModeNone,
  eIndirModePar,
  eIndirModeSquare
} tIndirMode;

typedef enum
{
  eIncModeNone,
  eIncModePreInc,
  eIncModePostInc,
  eIncModePreDec,
  eIncModePostDec
} tIncMode;

#define MModeReg (1 << AdrModeReg)
#define MModeAReg (1 << AdrModeAReg)
#define MModeImm (1 << AdrModeImm)
#define MModeMemReg (1 << AdrModeMemReg)

#define OpSizeBitPos8 ((tSymbolSize)16)
#define OpSizeBitPos16 ((tSymbolSize)17)
#define OpSizeBitPos32 ((tSymbolSize)18)
#define OpSizeShiftCount ((tSymbolSize)19)

typedef struct
{
  tAdrMode Mode;
  Byte Arg, Vals[4], ShiftLSB;
  unsigned ValCnt;
} tAdrVals;

static tSymbolSize OpSize, OpSize2;

static const tSymbolSize RegSizes[16] =
{
  eSymbolSize16Bit, eSymbolSize16Bit,   /* D2/D3 */
  eSymbolSize16Bit, eSymbolSize16Bit,   /* D4/D5 */
  eSymbolSize8Bit,  eSymbolSize8Bit,    /* D0/D1 */
  eSymbolSize32Bit, eSymbolSize32Bit,   /* D6/D7 */
  eSymbolSize24Bit, eSymbolSize24Bit,   /* X/Y */
  eSymbolSize24Bit, eSymbolSizeUnknown, /* S/- */
  eSymbolSize8Bit,  eSymbolSize8Bit,    /* CCH/CCL */
  eSymbolSize16Bit, eSymbolSizeUnknown, /* CCR/- */
};

/*--------------------------------------------------------------------------*/
/* Helper Functions */

static void PutCode(Word Code)
{
  if (Hi(Code))
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

static void AppendAdrVals(const tAdrVals *pVals)
{
  memcpy(BAsmCode + CodeLen, pVals->Vals, pVals->ValCnt);
  CodeLen += pVals->ValCnt;
}

static Boolean DecodeRegStr(const char *pArg, Byte *pRes)
{
  if ((strlen(pArg) == 2)
   && (toupper(*pArg) == 'D')
   && ((pArg[1] >= '0') && (pArg[1] <= '7')))
  {
    static const Byte RegCodes[8] = { 4, 5, 0, 1, 2, 3, 6, 7 };

    *pRes = RegCodes[pArg[1] - '0'];
    return True;
  }
  else
    return False;
}

static Boolean DecodeAdrRegStr(const char *pArg, Byte *pRes)
{
  static const char Regs[4][3] = { "X", "Y", "S", "PC" };
  unsigned z;

  for (z = 0; z < 4; z++)
    if (!as_strcasecmp(pArg, Regs[z]))
    {
      *pRes = z;
      return True;
    }
  return False;
}

static Boolean DecodeRegArg(int ArgNum, Byte *pRes, Byte Mask)
{
  Boolean Result = DecodeRegStr(ArgStr[ArgNum].str.p_str, pRes);

  if (!Result || !((Mask >> *pRes) & 1))
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[ArgNum]);
  return Result;
}

static Boolean DecodeAdrRegArg(int ArgNum, Byte *pRes, Byte Mask)
{
  Boolean Result = DecodeAdrRegStr(ArgStr[ArgNum].str.p_str, pRes);

  if (!Result || !((Mask >> *pRes) & 1))
    WrStrErrorPos(ErrNum_InvReg, &ArgStr[ArgNum]);
  return Result;
}

static Boolean DecodeGenRegArg(int ArgNum, Byte *pRes)
{
  if (DecodeRegStr(ArgStr[ArgNum].str.p_str, pRes))
    return True;
  else if (DecodeAdrRegStr(ArgStr[ArgNum].str.p_str, pRes) && (*pRes != 3))
  {
    *pRes += 8;
    return True;
  }
  else if (!as_strcasecmp(ArgStr[ArgNum].str.p_str, "CCH"))
  {
    *pRes = 12;
    return True;
  }
  else if (!as_strcasecmp(ArgStr[ArgNum].str.p_str, "CCL"))
  {
    *pRes = 13;
    return True;
  }
  else if (!as_strcasecmp(ArgStr[ArgNum].str.p_str, "CCW"))
  {
    *pRes = 14;
    return True;
  }
  else
    return False;
}

static Boolean ShortImm(LongInt Value, ShortInt OpSize, Byte *pShortValue, Byte *pShiftLSB)
{
  if (OpSize == OpSizeShiftCount)
  {
    if ((Value >= 0) && (Value <= 31))
    {
      *pShortValue = (Value >> 1 & 15);
      *pShiftLSB = Value & 1;
      return True;
    }
    else
      return False;
  }
  else if (OpSize < OpSizeBitPos8)
  {
    if (Value == -1)
    {
      *pShortValue = 0;
      return True;
    }
    else if ((Value >= 1) && (Value <= 15))
    {
      *pShortValue = Value;
      return True;
    }
    else if (((Value == (LongInt)0xff) && (OpSize == 0))
          || ((Value == (LongInt)0xffff) && (OpSize == 1))
          || ((Value == (LongInt)0xffffff) && (OpSize == eSymbolSize24Bit))
          || ((Value == (LongInt)0xffffffff) && (OpSize == 2)))
    {
      *pShortValue = 0;
      return True;
    }
    else
      return False;
  }
  else
    return False;
}

static unsigned OpSizeByteLen(ShortInt OpSize)
{
  switch (OpSize)
  {
    case -1: return 0;
    case 1: return 2;
    case 2: return 4;
    case eSymbolSize24Bit: return 3;
    default: return 1;
  }
}

static void ResetAdrVals(tAdrVals *pVals)
{
  pVals->Mode = AdrModeNone;
  pVals->Arg = 0;
  pVals->ValCnt = 0;
  pVals->ShiftLSB = 0;
}

static Boolean IsIncDec(char ch, char *pRes)
{
  *pRes = ((ch == '+') || (ch == '-')) ? ch : '\0';
  return !!*pRes;
}

static void CopyIndirect(tStrComp *pDest, const tStrComp *pSrc)
{
  pDest->Pos.Len = strmemcpy(pDest->str.p_str, STRINGSIZE, pSrc->str.p_str + 1, strlen(pSrc->str.p_str) - 2);
  pDest->Pos.StartCol = pSrc->Pos.StartCol + 1;
}

static Boolean DecodeAdr(int ArgIndex, unsigned ModeMask, tAdrVals *pVals)
{
  String CompStr;
  tStrComp Comp;
  int l;
  tIndirMode IndirMode;
  LargeWord Address;
  Boolean OK;

  ResetAdrVals(pVals);
  StrCompMkTemp(&Comp, CompStr, sizeof(CompStr));

  /* simple register: */

  if (DecodeRegStr(ArgStr[ArgIndex].str.p_str, &pVals->Arg))
  {
    if (ModeMask & MModeReg)
      pVals->Mode = AdrModeReg;
    else
    {
      pVals->Mode = AdrModeMemReg;
      pVals->Arg |= 0xb8;
    }
    goto done;
  }

  if (DecodeAdrRegStr(ArgStr[ArgIndex].str.p_str, &pVals->Arg))
  {
    pVals->Mode = AdrModeAReg;
    goto done;
  }

  /* immediate: */

  if (*ArgStr[ArgIndex].str.p_str == '#')
  {
    Boolean OK;
    LongInt Value;

    /* avoid returning AdrModeMemReg if immediate is forbidden */

    if (!(ModeMask &MModeImm))
      goto error;

    switch ((int)OpSize)
    {
      case eSymbolSize8Bit:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, Int8, &OK);
        break;
      case eSymbolSize16Bit:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, Int16, &OK);
        break;
      case eSymbolSize32Bit:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, Int32, &OK);
        break;
      case eSymbolSize24Bit:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, Int24, &OK);
        break;
      case OpSizeBitPos8:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, UInt3, &OK);
        break;
      case OpSizeBitPos16:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, UInt4, &OK);
        break;
      case OpSizeBitPos32:
      case OpSizeShiftCount:
        Value = EvalStrIntExpressionOffs(&ArgStr[ArgIndex], 1, UInt5, &OK);
        break;
      default:
        WrStrErrorPos(ErrNum_UndefOpSizes, &ArgStr[ArgIndex]);
        goto done;
    }

    if ((ModeMask & MModeMemReg) && (ShortImm(Value, OpSize, &pVals->Arg, &pVals->ShiftLSB)))
    {
      pVals->Mode = AdrModeMemReg;
      pVals->Arg |= 0x70;
    }
    else
    {
      pVals->Mode = AdrModeImm;
      if (OpSize == eSymbolSize32Bit)
        pVals->Vals[pVals->ValCnt++] = (Value >> 24) & 0xff;
      if ((OpSize == eSymbolSize32Bit) || (OpSize == eSymbolSize24Bit))
        pVals->Vals[pVals->ValCnt++] = (Value >> 16) & 0xff;
      if ((OpSize != eSymbolSize8Bit) && (OpSize < OpSizeBitPos8))
        pVals->Vals[pVals->ValCnt++] = (Value >> 8) & 0xff;
      pVals->Vals[pVals->ValCnt++] = Value & 0xff;
    }
    goto done;
  }

  /* indirect () []: */

  l = strlen(ArgStr[ArgIndex].str.p_str);
  if (IsIndirect(ArgStr[ArgIndex].str.p_str))
    IndirMode = eIndirModePar;
  else if ((l >= 2) && (ArgStr[ArgIndex].str.p_str[0] == '[') && (ArgStr[ArgIndex].str.p_str[l - 1] == ']'))
    IndirMode = eIndirModeSquare;
  else
    IndirMode = eIndirModeNone;

  if (IndirMode)
  {
    char *pSep, IncChar;
    LongInt DispAcc = 0;
    Byte DataReg = 0, AdrReg = 0, AdrIncReg = 0;
    Boolean AdrRegPresent = False, DataRegPresent = False, HasDisp = False;
    tIncMode IncMode = eIncModeNone;
    tStrComp Right, RunComp;

    CopyIndirect(&Comp, &ArgStr[ArgIndex]);
    StrCompRefRight(&RunComp, &Comp, 0);

    /* split into components */

    while (True)
    {
      pSep = QuotPos(RunComp.str.p_str, ',');
      if (pSep)
        StrCompSplitRef(&RunComp, &Right, &RunComp, pSep);

      /* remove leading/trailing spaces */

      KillPrefBlanksStrCompRef(&RunComp);
      KillPostBlanksStrComp(&RunComp);
      l = strlen(RunComp.str.p_str);

      if (DecodeRegStr(RunComp.str.p_str, &DataReg))
      {
        if (DataRegPresent)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &RunComp);
          goto done;
        }
        DataRegPresent = True;
      }
      else if (DecodeAdrRegStr(RunComp.str.p_str, &AdrReg))
      {
        if (AdrRegPresent)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &RunComp);
          goto done;
        }
        AdrRegPresent = True;
      }
      else if (IsIncDec(*RunComp.str.p_str, &IncChar) && DecodeAdrRegStr(RunComp.str.p_str + 1, &AdrIncReg))
      {
        if (IncMode)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &RunComp);
          goto done;
        }
        IncMode = (IncChar == '+') ? eIncModePreInc : eIncModePreDec;
      }
      else if (IsIncDec(Comp.str.p_str[l - 1], &IncChar))
      {
        RunComp.str.p_str[l - 1] = '\0';
        if (!DecodeAdrRegStr(RunComp.str.p_str, &AdrIncReg))
        {
          WrStrErrorPos(ErrNum_InvReg, &RunComp);
          goto done;
        }
        if (IncMode)
        {
          WrStrErrorPos(ErrNum_InvAddrMode, &RunComp);
          goto done;
        }
        IncMode = (IncChar == '+') ? eIncModePostInc : eIncModePostDec;
      }
      else
      {
        Boolean OK;
        LongInt Val = EvalStrIntExpression(&RunComp, Int24, &OK);

        if (!OK)
          goto done;
        DispAcc += Val;
        HasDisp = True;
      }

      if (pSep)
        RunComp = Right;
      else
        break;
    }

    /* pre/pos in/decrement */

    if ((IndirMode == eIndirModePar) && IncMode && !DispAcc && !AdrRegPresent && !DataRegPresent)
    {
      switch (AdrIncReg)
      {
        case 0:
        case 1:
          pVals->Arg = 0xc3 | (AdrIncReg << 4);
          if ((IncMode == eIncModePostInc) || (IncMode == eIncModePreInc))
            pVals->Arg |= 0x20;
          if ((IncMode == eIncModePostInc) || (IncMode == eIncModePostDec))
            pVals->Arg |= 0x04;
          pVals->Mode = AdrModeMemReg;
          break;
        case 2:
          if (IncMode == eIncModePreDec)
          {
            pVals->Arg = 0xfb;
            pVals->Mode = AdrModeMemReg;
          }
          else if (IncMode == eIncModePostInc)
          {
            pVals->Arg = 0xff;
            pVals->Mode = AdrModeMemReg;
          }
          else
            goto error;
          break;
        default:
          goto error;
      }
    }

    /* (disp,XYSP) */

    else if ((IndirMode == eIndirModePar) && AdrRegPresent && !DataRegPresent && !IncMode)
    {
      if ((AdrReg == 3) && (HasDisp))
        DispAcc -= EProgCounter();

      if ((DispAcc >= 0) && (DispAcc <= 15) && (AdrReg != 3))
      {
        pVals->Arg = 0x40 | (AdrReg << 4) | (DispAcc & 15);
        pVals->Mode = AdrModeMemReg;
      }
      else if (RangeCheck(DispAcc, SInt9))
      {
        pVals->Arg = 0xc0 | (AdrReg << 4) | ((DispAcc >> 8) & 1);
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
      else
      {
        pVals->Arg = 0xc2 | (AdrReg << 4);
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 16) & 0xff;
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 8) & 0xff;
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
    }

    /* (Dn,XYS) */

    else if ((IndirMode == eIndirModePar) && AdrRegPresent && DataRegPresent && !IncMode && !DispAcc)
    {
      if (AdrReg == 3)
        goto error;
      else
      {
        pVals->Arg = 0x88 | (AdrReg << 4) | DataReg;
        pVals->Mode = AdrModeMemReg;
      }
    }

    /* (disp,Dn) */

    else if ((IndirMode == eIndirModePar) && !AdrRegPresent && DataRegPresent && !IncMode)
    {
      if (RangeCheck(DispAcc, UInt18))
      {
        pVals->Arg = 0x80 | DataReg | ((DispAcc >> 12) & 0x30);
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 8) & 0xff;
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
      else
      {
        pVals->Arg = 0xe8 | DataReg;
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 16) & 0xff;
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 8) & 0xff;
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
    }

    /* [Dn,XY] */

    else if ((IndirMode == eIndirModeSquare) && AdrRegPresent && DataRegPresent && !IncMode && !DispAcc)
    {
      if (AdrReg >= 2)
        goto error;
      else
      {
        pVals->Arg = 0xc8 | (AdrReg << 4) | DataReg;
        pVals->Mode = AdrModeMemReg;
      }
    }

    /* [disp,XYSP] */

    else if ((IndirMode == eIndirModeSquare) && AdrRegPresent && !DataRegPresent && !IncMode)
    {
      if ((AdrReg == 3) && (HasDisp))
        DispAcc -= EProgCounter();

      if (RangeCheck(DispAcc, SInt9))
      {
        pVals->Arg = 0xc4 | (AdrReg << 4) | ((DispAcc >> 8) & 1);
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
      else
      {
        pVals->Arg = 0xc6 | (AdrReg << 4);
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 16) & 0xff;
        pVals->Vals[pVals->ValCnt++] = (DispAcc >> 8) & 0xff;
        pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
        pVals->Mode = AdrModeMemReg;
      }
    }

    /* [disp] */

    else if ((IndirMode == eIndirModeSquare) && !AdrRegPresent && !DataRegPresent && !IncMode)
    {
      pVals->Arg = 0xfe;
      pVals->Vals[pVals->ValCnt++] = (DispAcc >> 16) & 0xff;
      pVals->Vals[pVals->ValCnt++] = (DispAcc >> 8) & 0xff;
      pVals->Vals[pVals->ValCnt++] = DispAcc & 0xff;
      pVals->Mode = AdrModeMemReg;
    }

    else
      goto error;

    goto done;
  }

  /* absolute: */

  Address = EvalStrIntExpression(&ArgStr[ArgIndex], UInt24, &OK);
  if (OK)
  {
    if (RangeCheck(Address, UInt14))
    {
      pVals->Arg = 0x00 | ((Address >> 8) & 0x3f);
      pVals->Vals[pVals->ValCnt++] = Address & 0xff;
    }
    else if (RangeCheck(Address, UInt18))
    {
      pVals->Arg = 0xf8 | ((Address >> 16) & 1) | ((Address >> 15) & 4);
      pVals->Vals[pVals->ValCnt++] = (Address >> 8) & 0xff;
      pVals->Vals[pVals->ValCnt++] = Address & 0xff;
    }
    else
    {
      pVals->Arg = 0xfa;
      pVals->Vals[pVals->ValCnt++] = (Address >> 16) & 0xff;
      pVals->Vals[pVals->ValCnt++] = (Address >> 8) & 0xff;
      pVals->Vals[pVals->ValCnt++] = Address & 0xff;
    }
    pVals->Mode = AdrModeMemReg;
  }

done:
  if ((pVals->Mode != AdrModeNone) && !((ModeMask >> pVals->Mode) & 1))
  {
    ResetAdrVals(pVals);
    goto error;
  }
  return (pVals->Mode != AdrModeNone);

error:
  WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgIndex]);
  return False;
}

static Boolean IsImmediate(const tAdrVals *pVals, ShortInt OpSize, Byte *pImmVal)
{
  switch (pVals->Mode)
  {
    case AdrModeImm:
      *pImmVal = pVals->Vals[pVals->ValCnt - 1];
      return True;
    case AdrModeMemReg:
      if ((pVals->Arg & 0xf0) == 0x70)
      {
        *pImmVal = pVals->Arg & 15;
        if (OpSize == OpSizeShiftCount)
          *pImmVal = (*pImmVal << 1) | pVals->ShiftLSB;
        else if (!*pImmVal && (OpSize < OpSizeBitPos8))
          *pImmVal = 0xff;
        return True;
      }
      /* else fall-through */
    default:
      *pImmVal = 0;
      return False;
  }
}

static void ChangeImmediate(tAdrVals *pVals, ShortInt OpSize, Byte ImmVal)
{
  unsigned z;

  pVals->Mode = AdrModeImm;
  pVals->Arg = 0;
  pVals->ValCnt = OpSizeByteLen(OpSize);
  pVals->Vals[pVals->ValCnt - 1] = ImmVal;
  for (z = 1; z < pVals->ValCnt; z++)
    pVals->Vals[z] = (ImmVal & 0x80) ? 0xff : 0x00;
}

static Boolean IsReg(const tAdrVals *pVals, Byte *pReg)
{
  switch (pVals->Mode)
  {
    case AdrModeReg:
      *pReg = pVals->Arg;
      return True;
    case AdrModeMemReg:
      if ((pVals->Arg & 0xf8) == 0xb8)
      {
        *pReg = pVals->Arg & 7;
        return True;
      }
      /* else fall-through */
    default:
      *pReg = 0;
      return False;
  }
}

static Boolean SetOpSize(tSymbolSize NewOpSize)
{
  if ((OpSize == NewOpSize) || (OpSize == eSymbolSizeUnknown))
  {
    OpSize = NewOpSize;
    return True;
  }
  else
  {
    char Str[30];

    as_snprintf(Str, sizeof(Str), "%d -> %d", (int)OpSize, (int)NewOpSize);
    WrXError(ErrNum_ConfOpSizes, Str);
    return False;
  }
}

static Boolean SizeCode2(ShortInt ThisOpSize, Byte *pSizeCode)
{
  switch (ThisOpSize)
  {
    case eSymbolSize8Bit: *pSizeCode = 0; break;
    case eSymbolSize16Bit: *pSizeCode = 1; break;
    case eSymbolSize24Bit: *pSizeCode = 2; break;
    case eSymbolSize32Bit: *pSizeCode = 3; break;
    default: return False;
  }
  return True;
}

static Boolean DecodeImmBitField(tStrComp *pArg, Word *pResult)
{
  char *pSplit = strchr(pArg->str.p_str, ':'), Save;
  tStrComp Left, Right;
  Boolean OK;
  tSymbolFlags Flags;

  if (!pSplit)
  {
    WrError(ErrNum_InvBitPos);
    return False;
  }
  Save = StrCompSplitRef(&Left, &Right, pArg, pSplit);
  *pResult = EvalStrIntExpressionWithFlags(&Left, UInt6, &OK, &Flags);
  if (mFirstPassUnknown(Flags))
    *pResult &= 31;
  *pSplit = Save;
  if (!OK || !ChkRange(*pResult, 1, 32))
    return False;
  *pResult = (*pResult << 5) | (EvalStrIntExpression(&Right, UInt5, &OK) & 31);
  return OK;
}

/*--------------------------------------------------------------------------*/
/* Bit Symbol Handling */

/*
 * Compact representation of bits and bit fields in symbol table:
 * bits 0..2/3/4: (start) bit position
 * bits 3/4/5...14/15/16: register address in I/O space (first 4K)
 * bits 20/21: register size (0/1/2/3 for 8/16/32/24 bits)
 * bits 24..28: length of bit field minus one (0 for individual bit)
 */

/*!------------------------------------------------------------------------
 * \fn     AssembleBitfieldSymbol(Byte BitPos, Byte Width, ShortInt OpSize, Word Address)
 * \brief  build the compact internal representation of a bit field symbol
 * \param  BitPos bit position in word
 * \param  Width width of bit field
 * \param  OpSize operand size (0..2)
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitfieldSymbol(Byte BitPos, Byte Width, ShortInt OpSize, Word Address)
{
  LongWord CodeOpSize = (OpSize == eSymbolSize24Bit) ? 3 : OpSize;
  int AddrShift = (OpSize == eSymbolSize24Bit) ? 5 : (3 + OpSize);

  return BitPos
       | (((LongWord)Address & 0xfff) << AddrShift)
       | (CodeOpSize << 20)
       | (((LongWord)(Width - 1) & 31) << 24);
}

/*!------------------------------------------------------------------------
 * \fn     AssembleBitSymbol(Byte BitPos, ShortInt OpSize, Word Address)
 * \brief  build the compact internal representation of a bit symbol
 * \param  BitPos bit position in word
 * \param  OpSize operand size (0..2)
 * \param  Address register address
 * \return compact representation
 * ------------------------------------------------------------------------ */

static LongWord AssembleBitSymbol(Byte BitPos, ShortInt OpSize, Word Address)
{
  return AssembleBitfieldSymbol(BitPos, 1, OpSize, Address);
}

/*!------------------------------------------------------------------------
 * \fn     EvalBitPosition(const char *pBitArg, Boolean *pOK, ShortInt OpSize)
 * \brief  evaluate constant bit position, with bit range depending on operand size
 * \param  pBitArg bit position argument
 * \param  pOK returns True if OK
 * \param  OpSize operand size (0,1,2 -> 8,16,32 bits)
 * \return bit position as number
 * ------------------------------------------------------------------------ */

static Byte EvalBitPosition(const tStrComp *pBitArg, Boolean *pOK, ShortInt OpSize)
{
  switch (OpSize)
  {
    case eSymbolSize8Bit:
      return EvalStrIntExpression(pBitArg, UInt3, pOK);
    case eSymbolSize16Bit:
      return EvalStrIntExpression(pBitArg, UInt4, pOK);
    case eSymbolSize24Bit:
    {
      Byte Result;
      tSymbolFlags Flags;

      Result = EvalStrIntExpressionWithFlags(pBitArg, UInt5, pOK, &Flags);
      if (!*pOK)
        return Result;
      if (mFirstPassUnknown(Flags))
        Result &= 15;
      *pOK = ChkRange(Result, 0, 23);
      return Result;
    }
    case eSymbolSize32Bit:
      return EvalStrIntExpression(pBitArg, UInt5, pOK);
    default:
      WrError(ErrNum_InvOpSize);
      *pOK = False;
      return 0;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, ShortInt OpSize)
 * \brief  encode a bit symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \param  OpSize register size (0/1/2 = 8/16/32 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg2(LongWord *pResult, const tStrComp *pRegArg, const tStrComp *pBitArg, ShortInt OpSize)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongWord Addr;
  Byte BitPos;

  BitPos = EvalBitPosition(pBitArg, &OK, OpSize);
  if (!OK)
    return False;

  /* all I/O registers reside in the first 4K of the address space */

  Addr = EvalStrIntExpressionWithFlags(pRegArg, UInt12, &OK, &Flags);
  if (!OK)
    return False;

  *pResult = AssembleBitSymbol(BitPos, OpSize, Addr);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitfieldArg2(LongWord *pResult, const tStrComp *pRegArg, tStrComp *pBitArg, ShortInt OpSize)
 * \brief  encode a bit field symbol, address & bit position separated
 * \param  pResult resulting encoded bit
 * \param  pRegArg register argument
 * \param  pBitArg bit argument
 * \param  OpSize register size (0/1/2 = 8/16/32 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitfieldArg2(LongWord *pResult, const tStrComp *pRegArg, tStrComp *pBitArg, ShortInt OpSize)
{
  Boolean OK;
  LongWord Addr;
  Word BitSpec;

  if (!DecodeImmBitField(pBitArg, &BitSpec))
    return False;

  /* all I/O registers reside in the first 4K of the address space */

  Addr = EvalStrIntExpression(pRegArg, UInt12, &OK);
  if (!OK)
    return False;

  *pResult = AssembleBitfieldSymbol(BitSpec & 31, (BitSpec >> 5) & 31, OpSize, Addr);

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DecodeBitArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \param  OpSize register size (0/1/2 = 8/16/32 bit)
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
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
 * \fn     DecodeBitfieldArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
 * \brief  encode a bit symbol from instruction argument(s)
 * \param  pResult resulting encoded bit
 * \param  Start first argument
 * \param  Stop last argument
 * \return True if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeBitfieldArg(LongWord *pResult, int Start, int Stop, ShortInt OpSize)
{
  *pResult = 0;

  /* Just one argument -> parse as bit field argument */

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
    return DecodeBitfieldArg2(pResult, &ArgStr[Start], &ArgStr[Stop], OpSize);

  /* other # of arguments not allowed */

  else
  {
    WrError(ErrNum_WrongArgCnt);
    return False;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, Byte *pWidth, tSymbolSize *pOpSize)
 * \brief  transform compact represenation of bit (field) symbol into components
 * \param  BitSymbol compact storage
 * \param  pAddress (I/O) register address
 * \param  pBitPos (start) bit position
 * \param  pWidth pWidth width of bit field, always one for individual bit
 * \param  pOpSize returns register size (0/1/2 for 8/16/32 bits)
 * \return constant True
 * ------------------------------------------------------------------------ */

static Boolean DissectBitSymbol(LongWord BitSymbol, Word *pAddress, Byte *pBitPos, Byte *pWidth, tSymbolSize *pOpSize)
{
  *pOpSize = (tSymbolSize)((BitSymbol >> 20) & 3);
  switch (*pOpSize)
  {
    case eSymbolSize8Bit:
      *pAddress = (BitSymbol >> 3) & 0xfff;
      *pBitPos = BitSymbol & 7;
      break;
    case eSymbolSize16Bit:
      *pAddress = (BitSymbol >> 4) & 0xfff;
      *pBitPos = BitSymbol & 15;
      break;
    case 3:
      *pOpSize = eSymbolSize24Bit;
      /* fall-through */
    case eSymbolSize32Bit:
      *pAddress = (BitSymbol >> 5) & 0xfff;
      *pBitPos = BitSymbol & 31;
    default:
      break;
  }
  *pWidth = 1 + ((BitSymbol >> 24) & 31);
  return True;
}

/*!------------------------------------------------------------------------
 * \fn     DissectBit_S12Z(char *pDest, size_t DestSize, LargeWord Inp)
 * \brief  dissect compact storage of bit (field) into readable form for listing
 * \param  pDest destination for ASCII representation
 * \param  DestSize destination buffer size
 * \param  Inp compact storage
 * ------------------------------------------------------------------------ */

static void DissectBit_S12Z(char *pDest, size_t DestSize, LargeWord Inp)
{
  Byte BitPos, BitWidth;
  Word Address;
  tSymbolSize OpSize;
  char Attribute;

  DissectBitSymbol(Inp, &Address, &BitPos, &BitWidth, &OpSize);
  Attribute = (OpSize == eSymbolSize24Bit) ? 'p' : "bwl"[OpSize];

  if (BitWidth > 1)
    as_snprintf(pDest, DestSize, "$%x(%c).%u:%u", (unsigned)Address, Attribute, (unsigned)BitWidth, (unsigned)BitPos);
  else
    as_snprintf(pDest, DestSize, "$%x(%c).%u", (unsigned)Address, Attribute, (unsigned)BitPos);
}

/*!------------------------------------------------------------------------
 * \fn     ExpandS12ZBit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandS12ZBit(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    ShortInt OpSize = (pStructElem->OpSize < 0) ? 0 : pStructElem->OpSize;

    if (!ChkRange(Address, 0, 0xfff)
     || !ChkRange(pStructElem->BitPos, 0, (8 << OpSize) - 1))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitSymbol(pStructElem->BitPos, OpSize, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*!------------------------------------------------------------------------
 * \fn     ExpandS12ZBitfield(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
 * \brief  expands bit field definition when a structure is instantiated
 * \param  pVarName desired symbol name
 * \param  pStructElem element definition
 * \param  Base base address of instantiated structure
 * ------------------------------------------------------------------------ */

static void ExpandS12ZBitfield(const tStrComp *pVarName, const struct sStructElem *pStructElem, LargeWord Base)
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
    ShortInt OpSize = (pStructElem->OpSize < 0) ? 0 : pStructElem->OpSize;

    if (!ChkRange(Address, 0, 0xfff)
     || !ChkRange(pStructElem->BitPos, 0, (8 << OpSize) - 1)
     || !ChkRange(pStructElem->BitPos + pStructElem->BitWidthM1, 0, (8 << OpSize) - 1))
      return;

    PushLocHandle(-1);
    EnterIntSymbol(pVarName, AssembleBitfieldSymbol(pStructElem->BitPos, pStructElem->BitWidthM1 + 1, OpSize, Address), SegBData, False);
    PopLocHandle();
    /* TODO: MakeUseList? */
  }
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Code)
{
  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static Boolean DecodeBranchCore(int ArgIndex)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt ShortDist, LongDist;

  /* manual says distance is relative to start of next instruction */

  ShortDist = EvalStrIntExpressionWithFlags(&ArgStr[ArgIndex], UInt24, &OK, &Flags) - (EProgCounter() + CodeLen + 1);
  if (!OK)
    return False;
  LongDist = ShortDist - 1;

  if (OpSize == eSymbolSizeUnknown)
    OpSize = ((ShortDist <= 63) && (ShortDist >= -64)) ? eSymbolSizeFloat32Bit : eSymbolSize32Bit;
  switch (OpSize)
  {
    case eSymbolSize32Bit:
      if (!mSymbolQuestionable(Flags) && !RangeCheck(LongDist, SInt15))
      {
        WrError(ErrNum_JmpDistTooBig);
        return False;
      }
      else
      {
        BAsmCode[CodeLen++] = 0x80 | ((LongDist >> 7) & 0x7f);
        BAsmCode[CodeLen++] = LongDist & 0xff;
      }
      break;
    case eSymbolSizeFloat32Bit:
      if (!mSymbolQuestionable(Flags) && !RangeCheck(ShortDist, SInt7))
      {
        WrError(ErrNum_JmpDistTooBig);
        return False;
      }
      else
      {
        BAsmCode[CodeLen++] = ShortDist & 0x7f;
      }
      break;
    default:
      WrStrErrorPos(ErrNum_InvOpSize, &AttrPart);
      return False;
  }
  return True;
}

static void DecodeBranch(Word Code)
{
  if (!ChkArgCnt(1, 1))
    return;

  PutCode(Code);
  if (!DecodeBranchCore(1))
    CodeLen = 0;
}

static void DecodeReg(Word Code)
{
  Byte Reg;

  if (ChkArgCnt(1, 1)
   && DecodeRegArg(1, &Reg, 0xff)
   && SetOpSize(RegSizes[Reg]))
    PutCode(Code | Reg);
}

static void DecodeTwoReg(Word Code)
{
  Byte SrcReg, DestReg;

  /* TODO: what is the operand order (source/dest)? The manual is
     unclear about this.  Assuming source is first argument, similar to TFR: */

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2)
        && DecodeRegArg(2, &DestReg, 0xff)
        && DecodeRegArg(1, &SrcReg, 0xff))
  {
    PutCode(Code);
    BAsmCode[CodeLen++] = DestReg | (SrcReg << 4);
  }
}

static void DecodeRegMemImm(Word Code)
{
  Byte Reg;
  tAdrVals AdrVals;

  if (ChkArgCnt(2, 2)
   && DecodeRegArg(1, &Reg, 0xff)
   && SetOpSize(RegSizes[Reg])
   && DecodeAdr(2, MModeImm | MModeMemReg, &AdrVals))
  {
    if (AdrVals.Mode == AdrModeImm)
      PutCode(Code | Reg);
    else
    {
      PutCode((Code + 0x10) | Reg);
      BAsmCode[CodeLen++] = AdrVals.Arg;
    }
    AppendAdrVals(&AdrVals);
  }
}

static void DecodeSUB(Word Code)
{
  Byte Reg;
  tAdrVals AdrVals;

  if (ArgCnt == 3)
  {
    if (DecodeRegArg(1, &Reg, 1 << 6)
     && DecodeAdrRegArg(2, &Reg, 3)
     && DecodeAdrRegArg(3, &Reg, 1 << (1 - Reg)))
    {
      BAsmCode[CodeLen++] = 0xfe - Reg;
    }
  }
  else if (ChkArgCnt(2, 3)
   && DecodeRegArg(1, &Reg, 0xff)
   && SetOpSize(RegSizes[Reg])
   && DecodeAdr(2, MModeImm | MModeMemReg, &AdrVals))
  {
    if (AdrVals.Mode == AdrModeImm)
      PutCode(Code | Reg);
    else
    {
      PutCode((Code + 0x10) | Reg);
      BAsmCode[CodeLen++] = AdrVals.Arg;
    }
    AppendAdrVals(&AdrVals);
  }
}

static void DecodeCMP(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Byte Reg;
    tAdrVals AdrVals;

    DecodeAdr(1, MModeReg | MModeAReg, &AdrVals);
    Reg = AdrVals.Arg;
    switch (AdrVals.Mode)
    {
      case AdrModeReg:
        if (SetOpSize(RegSizes[Reg])
         && DecodeAdr(2, MModeImm | MModeMemReg, &AdrVals))
        {
          if (AdrVals.Mode == AdrModeImm)
            PutCode(Code | Reg);
          else
          {
            PutCode((Code + 0x10) | Reg);
            BAsmCode[CodeLen++] = AdrVals.Arg;
          }
          AppendAdrVals(&AdrVals);
        }
        break;
      case AdrModeAReg:
        if (Reg == 3) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        else if (SetOpSize(eSymbolSize24Bit))
        {
          DecodeAdr(2, MModeImm | MModeMemReg | MModeAReg, &AdrVals);
          switch (AdrVals.Mode)
          {
            case AdrModeImm:
              PutCode((Reg == 2) ? 0x1b04 : (0xe8 | Reg));
              AppendAdrVals(&AdrVals);
              break;
            case AdrModeMemReg:
              PutCode((Reg == 2) ? 0x1b02 : (0xf8 | Reg));
              BAsmCode[CodeLen++] = AdrVals.Arg;
              AppendAdrVals(&AdrVals);
              break;
            case AdrModeAReg:
              if (Reg != 0) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
              else if (AdrVals.Arg != 1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
              else
                PutCode(0xfc);
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
}

static void DecodeImm8(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1) && SetOpSize(eSymbolSize8Bit) && DecodeAdr(1, MModeImm, &AdrVals))
  {
    PutCode(Code);
    AppendAdrVals(&AdrVals);
  }
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(2,3))
  {
    tAdrVals CntAdrVals, OpAdrVals;
    tSymbolSize SaveOpSize;
    Boolean IsASL = (Code == 0xc0),
            IsASR = (Code == 0x80);
    Boolean ImmediateCnt, DestIsReg;
    Byte ImmCnt, SizeCode, OpReg, DestReg;

    /* force operand size to 5 bits for count */

    SaveOpSize = OpSize;
    OpSize = OpSizeShiftCount;
    if (!DecodeAdr(ArgCnt, MModeImm | MModeMemReg | MModeReg, &CntAdrVals))
      return;
    ImmediateCnt = IsImmediate(&CntAdrVals, OpSize, &ImmCnt);
    OpSize = SaveOpSize;

    /* source or source-and-dest operand */

    if (!DecodeAdr(ArgCnt - 1, MModeMemReg | MModeReg, &OpAdrVals))
      return;

    /* operand size not yet set - then set from source */

    if (IsReg(&OpAdrVals, &OpReg))
      SetOpSize(RegSizes[OpReg]);
    if (OpSize < 0)
    {
      WrError(ErrNum_UndefOpSizes);
      return;
    }
    else if (!SizeCode2(OpSize, &SizeCode))
    {
      WrError(ErrNum_InvOpSize);
      return;
    }

    /* for three args, destination is always a register */

    if (ArgCnt == 3)
    {
      /* dest reg does not set operand size - opsize is from src
         operand which may be memory */

      if (!DecodeRegArg(1, &DestReg, 0xff))
        return;
      DestIsReg = True;
    }
    else
      DestIsReg = IsReg(&OpAdrVals, &DestReg);

    /* REG-REG-OPR1/2/3 only allowed with ASL: convert to (REG-)OPR1/2/3-OPR1/2/3 for other instructions,
       unless count is immediate: */

    if (!IsASL && DestIsReg && (OpAdrVals.Mode == AdrModeReg) && !ImmediateCnt && (CntAdrVals.Mode == AdrModeMemReg))
    {
      OpAdrVals.Mode = AdrModeMemReg;
      OpAdrVals.Arg |= 0xb8;
    }

    /* REG-REG */

    if (DestIsReg && (OpAdrVals.Mode == AdrModeReg) && (CntAdrVals.Mode == AdrModeReg))
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | (IsASL ? 0x10 : 0x20) | OpAdrVals.Arg;
      BAsmCode[CodeLen++] = 0xb8 | CntAdrVals.Arg;
    }

    /* REG-IMM with n=1..2 */

    else if (DestIsReg && (OpAdrVals.Mode == AdrModeReg) && ImmediateCnt && (ImmCnt >= 1) && (ImmCnt <= 2))
    {
      BAsmCode[CodeLen++] = (IsASR ? 0x00 : 0x10) | DestReg;
      BAsmCode[CodeLen++] = Code | (IsASR ? 0x10 : 0x00) | ((ImmCnt - 1) << 3) | OpAdrVals.Arg;
    }

    /* REG-IMM with arbitrary n */

    else if (DestIsReg && (OpAdrVals.Mode == AdrModeReg) && ImmediateCnt)
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | (IsASL ? 0x00 : 0x10) | ((ImmCnt & 1) << 3) | OpAdrVals.Arg;
      BAsmCode[CodeLen++] = 0x70 | ((ImmCnt >> 1) & 7);
    }

    /* REG-OPR1/2/3 - ASL only */

    else if (IsASL && DestIsReg && (OpAdrVals.Mode == AdrModeReg) && (CntAdrVals.Mode == AdrModeMemReg))
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | (CntAdrVals.ShiftLSB << 3) | OpAdrVals.Arg;
      BAsmCode[CodeLen++] = CntAdrVals.Arg;
      AppendAdrVals(&CntAdrVals);
    }

    /* (REG-)OPR1/2/3-IMM with n=1..2 */

    else if (DestIsReg && ImmediateCnt && (ImmCnt >= 1) && (ImmCnt <= 2))
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | 0x20 | ((ImmCnt - 1) << 3) | SizeCode;
      BAsmCode[CodeLen++] = OpAdrVals.Arg;
      AppendAdrVals(&OpAdrVals);
    }

    /* (REG-)OPR1/2/3-IMM with arbitrary n */

    else if (DestIsReg && ImmediateCnt && (OpAdrVals.Mode == AdrModeMemReg))
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | 0x30 | ((ImmCnt & 1) << 3) | SizeCode;
      BAsmCode[CodeLen++] = OpAdrVals.Arg;
      AppendAdrVals(&OpAdrVals);
      BAsmCode[CodeLen++] = 0x70 | ((ImmCnt >> 1) & 0x0f);
    }

    /* (REG-)OPR1/2/3-OPR1/2/3 */

    else if (DestIsReg && (OpAdrVals.Mode == AdrModeMemReg) && (CntAdrVals.Mode == AdrModeMemReg))
    {
      BAsmCode[CodeLen++] = 0x10 | DestReg;
      BAsmCode[CodeLen++] = Code | 0x30 | (CntAdrVals.ShiftLSB << 3) | SizeCode;
      BAsmCode[CodeLen++] = OpAdrVals.Arg;
      AppendAdrVals(&OpAdrVals);
      BAsmCode[CodeLen++] = CntAdrVals.Arg;
      AppendAdrVals(&CntAdrVals);
    }

    /* (src-)OPR/1/2/3-IMM (n=1..2) */

    else if ((OpAdrVals.Mode == AdrModeMemReg) && ImmediateCnt && (ImmCnt >= 1) && (ImmCnt <= 2))
    {
      BAsmCode[CodeLen++] = 0x10;
      BAsmCode[CodeLen++] = Code | 0x34 | ((ImmCnt - 1) << 3) | SizeCode;
      BAsmCode[CodeLen++] = OpAdrVals.Arg;
      AppendAdrVals(&OpAdrVals);
    }

    else
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeBit(Word Code)
{
  tSymbolSize SaveOpSize;
  tAdrVals PosAdrVals, OpAdrVals;
  Byte ImmPos, ImmWidth, SizeCode, OpReg;
  Boolean ImmediatePos;

  if (!ChkArgCnt(1, 2))
    return;

  /* bit operand: */

  if (1 == ArgCnt)
  {
    LongWord BitArg;
    tSymbolSize ThisOpSize;
    Word Address;

    if (DecodeBitArg(&BitArg, 1, 1, OpSize)
     && DissectBitSymbol(BitArg, &Address, &ImmPos, &ImmWidth, &ThisOpSize)
     && SetOpSize(ThisOpSize))
    {
      /* TODO: warn if ImmWidth != 1 */
      if (!SizeCode2(OpSize, &SizeCode) || (SizeCode == 2))
      {
        WrError(ErrNum_InvOpSize);
        return;
      }
      ImmediatePos = True;
      OpAdrVals.Mode = AdrModeMemReg;
      OpAdrVals.Arg = Hi(Address);
      OpAdrVals.Vals[0] = Lo(Address);
      OpAdrVals.ValCnt = 1;
    }
    else
      return;
    OpReg = 0;
  }

  /* other operand */

  else
  {
    if (!DecodeAdr(1, MModeMemReg | MModeReg, &OpAdrVals))
      return;

    /* operand size not yet set - then set from source */

    if (IsReg(&OpAdrVals, &OpReg))
      SetOpSize(RegSizes[OpReg]);
    if (OpSize < 0)
    {
      WrError(ErrNum_UndefOpSizes);
      return;
    }

    else if (!SizeCode2(OpSize, &SizeCode) || (SizeCode == 2))
    {
      WrError(ErrNum_InvOpSize);
      return;
    }

    /* force operand size to 3/4/5 bits for bit position */

    SaveOpSize = OpSize;
    OpSize = OpSizeBitPos8 + OpSize;
    if (!DecodeAdr(2, MModeImm | MModeReg, &PosAdrVals))
      return;
    ImmediatePos = IsImmediate(&PosAdrVals, OpSize, &ImmPos);
    OpSize = SaveOpSize;
  }

  switch (OpAdrVals.Mode)
  {
    case AdrModeReg:
      BAsmCode[CodeLen++] = Code;
      if (ImmediatePos)
        BAsmCode[CodeLen++] = (ImmPos << 3) | OpReg;
      else
      {
        BAsmCode[CodeLen++] = 0x81 | (PosAdrVals.Arg << 4);
        BAsmCode[CodeLen++] = 0xb8 | OpReg;
      }
      break;
    case AdrModeMemReg:
      BAsmCode[CodeLen++] = Code;
      if (ImmediatePos)
      {
        BAsmCode[CodeLen] = 0x80 | ((ImmPos & 7) << 4);
        if (OpSize >= eSymbolSize16Bit)
          BAsmCode[CodeLen] |= (1 << SizeCode) | ((ImmPos >> 3) & SizeCode);
        CodeLen++;
      }
      else
        BAsmCode[CodeLen++] = 0x81 | (PosAdrVals.Arg << 4) | (SizeCode << 2);
      BAsmCode[CodeLen++] = OpAdrVals.Arg;
      AppendAdrVals(&OpAdrVals);
      break;
    default:
      break;
  }
}

static void DecodeBitField(Word Code)
{
  if (!ChkArgCnt(2, 3))
    return;

  /* if two arguments, bit field is symbolic and is
     - the destination (first arg) for BFINS
     - the source (second arg) for BFEXT */

  if (2 == ArgCnt)
  {
    LongWord BitfieldArg;
    Byte Reg, ImmPos, ImmWidth, SizeCode;
    Word FieldSpec, Address;
    tSymbolSize ThisOpSize;
    int RegArg = (Code == 0x80) ? 2 : 1;

    if (DecodeRegArg(RegArg, &Reg, 0xff)
     && DecodeBitfieldArg(&BitfieldArg, 3 - RegArg, 3 - RegArg, OpSize)
     && DissectBitSymbol(BitfieldArg, &Address, &ImmPos, &ImmWidth, &ThisOpSize)
     && SetOpSize(ThisOpSize)
     && SizeCode2(OpSize, &SizeCode))
    {
      FieldSpec = ImmPos | ((ImmWidth < 32) ? ((Word)ImmWidth << 5) : 0);
      BAsmCode[CodeLen++] = 0x1b;
      BAsmCode[CodeLen++] = 0x08 | Reg;
      BAsmCode[CodeLen++] = (Code ? 0xf0: 0x60) | (SizeCode << 2) | Hi(FieldSpec);
      BAsmCode[CodeLen++] = Lo(FieldSpec);
      BAsmCode[CodeLen++] = Hi(Address);
      BAsmCode[CodeLen++] = Lo(Address);
    }
  }
  else
  {
    Byte ParamReg, SizeCode;
    Word ParamImm;
    tAdrVals SrcAdrVals, DestAdrVals;

    if (*ArgStr[3].str.p_str == '#')
    {
      tStrComp Field;

      StrCompRefRight(&Field, &ArgStr[3], 1);
      if (!DecodeImmBitField(&Field, &ParamImm))
        return;
      ParamReg = 16; /* immediate flag */
    }

    /* only D2...D5 allowed as parameter */

    else if (!DecodeRegStr(ArgStr[3].str.p_str, &ParamReg) || (ParamReg >= 4))
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
      return;
    }

    DecodeAdr(2, MModeReg | MModeImm | MModeMemReg, &SrcAdrVals);
    switch (SrcAdrVals.Mode)
    {
      case AdrModeReg:
        DecodeAdr(1, MModeReg | MModeMemReg, &DestAdrVals);
        switch (DestAdrVals.Mode)
        {
          case AdrModeReg:
            BAsmCode[CodeLen++] = 0x1b;
            BAsmCode[CodeLen++] = 0x08 | DestAdrVals.Arg;
            if (16 == ParamReg)
            {
              BAsmCode[CodeLen++] = Code | 0x20 | (SrcAdrVals.Arg << 2) | Hi(ParamImm);
              BAsmCode[CodeLen++] = Lo(ParamImm);
            }
            else
              BAsmCode[CodeLen++] = Code | (SrcAdrVals.Arg << 2) | ParamReg;
            break;
          case AdrModeMemReg:
            if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else if (!SizeCode2(OpSize, &SizeCode)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0x1b;
              BAsmCode[CodeLen++] = 0x08 | SrcAdrVals.Arg;
              if (16 == ParamReg)
              {
                BAsmCode[CodeLen++] = Code | 0x70 | (SizeCode << 2) | Hi(ParamImm);
                BAsmCode[CodeLen++] = Lo(ParamImm);
              }
              else
                BAsmCode[CodeLen++] = Code | 0x50 | (SizeCode << 2) | ParamReg;
              BAsmCode[CodeLen++] = DestAdrVals.Arg;
              AppendAdrVals(&DestAdrVals);
            }
            break;
          default:
            break;
        }
        break;
      case AdrModeMemReg:
        DecodeAdr(1, MModeReg, &DestAdrVals);
        switch (DestAdrVals.Mode)
        {
          case AdrModeReg:
            if (OpSize  == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
            else if (!SizeCode2(OpSize, &SizeCode)) WrError(ErrNum_InvOpSize);
            else
            {
              BAsmCode[CodeLen++] = 0x1b;
              BAsmCode[CodeLen++] = 0x08 | DestAdrVals.Arg;
              if (16 == ParamReg)
              {
                BAsmCode[CodeLen++] = Code | 0x60 | (SizeCode << 2) | Hi(ParamImm);
                BAsmCode[CodeLen++] = Lo(ParamImm);
              }
              else
                BAsmCode[CodeLen++] = Code | 0x40 | (SizeCode << 2) | ParamReg;
              BAsmCode[CodeLen++] = SrcAdrVals.Arg;
              AppendAdrVals(&SrcAdrVals);
            }
            break;
          default:
            break;
        }
        break;
      case AdrModeImm: /* immediate only allowed for short immediate in MemReg op */
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        break;
      default:
        break;
    }
  }
}

static void DecodeBitRel(Word Code)
{
  if (!ChkArgCnt(3, 3))
    return;

  ArgCnt--;
  DecodeBit(Code);
  if (!CodeLen)
    return;

  /* operand size attribute is consumed by bit operand */

  OpSize = eSymbolSizeUnknown;
  if (!DecodeBranchCore(3))
    CodeLen = 0;
}

static void DecodeCLR(Word Code)
{
  tAdrVals AdrVals;

  UNUSED(Code);

  if (ChkArgCnt(1, 1) && DecodeAdr(1, MModeReg | MModeAReg | MModeMemReg, &AdrVals))
  {
    switch (AdrVals.Mode)
    {
      case AdrModeReg:
        if (SetOpSize(RegSizes[AdrVals.Arg]))
          PutCode(0x0038 | AdrVals.Arg);
        break;
      case AdrModeAReg:
        if (!SetOpSize(eSymbolSize24Bit));
        else if (AdrVals.Arg > 1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        else
          PutCode(0x009a | AdrVals.Arg);
        break;
      case AdrModeMemReg:
      {
        Byte SizeCode;

        if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
        else if (!SizeCode2(OpSize, &SizeCode)) WrError(ErrNum_InvOpSize);
        else
        {
          BAsmCode[CodeLen++] = 0xbc | SizeCode;
          BAsmCode[CodeLen++] = AdrVals.Arg;
          AppendAdrVals(&AdrVals);
        }
        break;
      }
      default:
        break;
    }
  }
}

static void DecodeCOM_NEG(Word Code)
{
  tAdrVals AdrVals;
  Byte OpReg, SizeCode;

  if (!ChkArgCnt(1, 1))
    return;
  DecodeAdr(1, MModeMemReg, &AdrVals);

  /* operand size not yet set - then set from (register) op */

  if (IsReg(&AdrVals, &OpReg))
  {
    if (!SetOpSize(RegSizes[OpReg]))
      return;
  }
  if (OpSize == eSymbolSizeUnknown)
  {
    WrError(ErrNum_UndefOpSizes);
    return;
  }
  if (!SizeCode2(OpSize, &SizeCode) || (OpSize == eSymbolSize24Bit))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }
  PutCode(Code | SizeCode);
  BAsmCode[CodeLen++] = AdrVals.Arg;
  AppendAdrVals(&AdrVals);
}

static void DecodeDBcc(Word Code)
{
  tAdrVals AdrVals;
  Byte OpReg, SizeCode;

  if (!ChkArgCnt(2, 2))
    return;
  DecodeAdr(1, MModeReg | MModeAReg | MModeMemReg, &AdrVals);

  if (IsReg(&AdrVals, &OpReg))
  {
    if (!SetOpSize(RegSizes[OpReg]))
      return;
  }
  else if (AdrVals.Mode == AdrModeAReg)
  {
    if (!SetOpSize(eSymbolSize24Bit))
      return;
  }
  if (OpSize == eSymbolSizeUnknown)
  {
    WrError(ErrNum_UndefOpSizes);
    return;
  }
  if (!SizeCode2(OpSize, &SizeCode))
  {
    WrError(ErrNum_InvOpSize);
    return;
  }
  switch (AdrVals.Mode)
  {
    case AdrModeReg:
      PutCode(Code | AdrVals.Arg);
      break;
    case AdrModeAReg:
      if (AdrVals.Arg > 1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
        PutCode(Code | 0x0008 | AdrVals.Arg);
      break;
    case AdrModeMemReg:
      PutCode(Code | 0x000c | SizeCode);
      BAsmCode[CodeLen++] = AdrVals.Arg;
      AppendAdrVals(&AdrVals);
      break;
    default:
      break;
  }

  if (CodeLen > 0)
  {
    /* operand size attribute consumed by operand */

    OpSize = eSymbolSizeUnknown;

    if (!DecodeBranchCore(2))
      CodeLen = 0;
  }
}

static void DecodeINC_DEC(Word Code)
{
  tAdrVals AdrVals;

  if (ChkArgCnt(1, 1) && DecodeAdr(1, MModeReg | MModeMemReg, &AdrVals))
  {
    switch (AdrVals.Mode)
    {
      case AdrModeReg:
        if (SetOpSize(RegSizes[AdrVals.Arg]))
          PutCode(Code + AdrVals.Arg);
        break;
      case AdrModeMemReg:
      {
        Byte SizeCode;

        if (OpSize == eSymbolSizeUnknown) WrError(ErrNum_UndefOpSizes);
        else if (!SizeCode2(OpSize, &SizeCode) || (OpSize == eSymbolSize24Bit)) WrError(ErrNum_UndefOpSizes);
        else
        {
          BAsmCode[CodeLen++] = (Code + 0x6c) | SizeCode;
          BAsmCode[CodeLen++] = AdrVals.Arg;
          AppendAdrVals(&AdrVals);
        }
        break;
      }
      default:
        break;
    }
  }
}

static void DecodeDIV_MOD(Word Code)
{
  tAdrVals DividentAdrVals, DivisorAdrVals;
  Byte DividentSizeCode, DivisorSizeCode, DestReg, DivisorReg;
  Word EffCode, LoCode;

  EffCode = Hi(Code) | ((Lo(Code) & 0x01) ? 0x1b00 : 0x0000);
  LoCode = Lo(Code) & 0x80;

  /* destination is always a register */

  if (!ChkArgCnt(3, 3) || !DecodeRegArg(1, &DestReg, 0xff))
    return;

  DecodeAdr(2, MModeImm | MModeReg | MModeMemReg, &DividentAdrVals);
  switch (DividentAdrVals.Mode)
  {
    case AdrModeReg:
      DecodeAdr(3, MModeImm | MModeReg | MModeMemReg, &DivisorAdrVals);
      switch (DivisorAdrVals.Mode)
      {
        case AdrModeReg:
          PutCode(EffCode | DestReg);
          BAsmCode[CodeLen++] = LoCode | (DividentAdrVals.Arg << 3) | DivisorAdrVals.Arg;
          break;
        case AdrModeImm:
          if (!SizeCode2(OpSize, &DivisorSizeCode) || (OpSize == eSymbolSize24Bit)) WrError(ErrNum_UndefOpSizes);
          else
          {
            PutCode(EffCode | DestReg);
            BAsmCode[CodeLen++] = LoCode | 0x44 | (DividentAdrVals.Arg << 3) | DivisorSizeCode;
            AppendAdrVals(&DivisorAdrVals);
          }
          break;
        case AdrModeMemReg:
          if (!SizeCode2(OpSize, &DivisorSizeCode) || (OpSize == eSymbolSize24Bit)) WrError(ErrNum_UndefOpSizes);
          else
          {
            PutCode(EffCode | DestReg);
            BAsmCode[CodeLen++] = LoCode | 0x40 | (DividentAdrVals.Arg << 3) | DivisorSizeCode;
            BAsmCode[CodeLen++] = DivisorAdrVals.Arg;
            AppendAdrVals(&DivisorAdrVals);
          }
          break;
        default:
          break;
      }
      break;
    case AdrModeMemReg:
      /* divident==register is filtered out before, so divident size cannot be set from register */
      if (!SizeCode2(OpSize, &DividentSizeCode) || (OpSize == eSymbolSize24Bit)) WrError(ErrNum_UndefOpSizes);
      else
      {
        OpSize = OpSize2;
        DecodeAdr(3, MModeImm | MModeMemReg, &DivisorAdrVals);
        switch (DivisorAdrVals.Mode)
        {
          case AdrModeMemReg:
            if ((OpSize == eSymbolSizeUnknown) && IsReg(&DivisorAdrVals, &DivisorReg))
              SetOpSize(RegSizes[DivisorReg]);
            if (!SizeCode2(OpSize, &DivisorSizeCode) || (OpSize == eSymbolSize24Bit)) WrError(ErrNum_UndefOpSizes);
            else
            {
              PutCode(EffCode | DestReg);
              BAsmCode[CodeLen++] = LoCode | 0x42 | (DividentSizeCode << 4) | (DivisorSizeCode << 2);
              BAsmCode[CodeLen++] = DividentAdrVals.Arg;
              AppendAdrVals(&DividentAdrVals);
              BAsmCode[CodeLen++] = DivisorAdrVals.Arg;
              AppendAdrVals(&DivisorAdrVals);
            }
            break;
          case AdrModeImm: /* was only allowed for short imm in MemReg */
            WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[3]);
            break;
          default:
            break;
        }
      }
      break;
    case AdrModeImm: /* was only allowed for short imm in MemReg */
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
      break;
    default:
      break;
  }
}

static void DecodeEXG_TFR(Word Code)
{
  Byte SrcReg, DestReg;

  if (ChkArgCnt(2, 2)
   && DecodeGenRegArg(1, &SrcReg)
   && DecodeGenRegArg(2, &DestReg))
  {
    if ((OpSizeByteLen(RegSizes[SrcReg]) >= OpSizeByteLen(RegSizes[DestReg])) && Hi(Code))
      WrError(ErrNum_SrcLEThanDest);
    BAsmCode[CodeLen++] = Lo(Code);
    BAsmCode[CodeLen++] = (SrcReg << 4) | DestReg;
  }
}

static void DecodeJMP_JSR(Word Code)
{
  tAdrVals AdrVals;

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(1, 1) && DecodeAdr(1, MModeMemReg, &AdrVals))
  {
    Byte Dummy;

    if (IsReg(&AdrVals, &Dummy)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
    else
    {
      if (AdrVals.Arg == 0xfa)
        PutCode(Code | 0x10);
      else
      {
        PutCode(Code);
        BAsmCode[CodeLen++] = AdrVals.Arg;
      }
      AppendAdrVals(&AdrVals);
    }
  }
}

static void DecodeLD_ST(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;

  if (ChkArgCnt(2, 2) && DecodeAdr(1, MModeReg | MModeAReg, &DestAdrVals))
  {
    switch (DestAdrVals.Mode)
    {
      case AdrModeReg:
        if (!SetOpSize(RegSizes[DestAdrVals.Arg]))
          return;
        DecodeAdr(2, (Code ? 0 : MModeImm) | MModeMemReg, &SrcAdrVals);
        switch (SrcAdrVals.Mode)
        {
          case AdrModeMemReg:
          {
            Byte ImmVal;

            if ((OpSize == eSymbolSize8Bit) && IsImmediate(&SrcAdrVals, OpSize, &ImmVal)) /* same instr length for byte, but what people expect? */
            {
              ChangeImmediate(&SrcAdrVals, OpSize, ImmVal);
              goto immediate;
            }
            if (SrcAdrVals.Arg == 0xfa)
              BAsmCode[CodeLen++] = (0xb0 + Code) | DestAdrVals.Arg;
            else
            {
              BAsmCode[CodeLen++] = (0xa0 + Code) | DestAdrVals.Arg;
              BAsmCode[CodeLen++] = SrcAdrVals.Arg;
            }
            AppendAdrVals(&SrcAdrVals);
            break;
          }
          case AdrModeImm:
          immediate:
            BAsmCode[CodeLen++] = 0x90 | DestAdrVals.Arg;
            AppendAdrVals(&SrcAdrVals);
            break;
          default:
            break;
        }
        break;
      case AdrModeAReg:
        if (DestAdrVals.Arg > 2)
        {
          WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        }
        if (!SetOpSize(eSymbolSize24Bit))
          return;
        DecodeAdr(2, (Code ? 0 : MModeImm) | MModeMemReg, &SrcAdrVals);
        switch (SrcAdrVals.Mode)
        {
          case AdrModeMemReg:
            if ((DestAdrVals.Arg < 2) && (SrcAdrVals.Arg == 0xfa))
              BAsmCode[CodeLen++] = (0xb8 + Code) | DestAdrVals.Arg;
            else if (2 == DestAdrVals.Arg)
            {
              BAsmCode[CodeLen++] = 0x1b;
              BAsmCode[CodeLen++] = 0x00 + !!Code;
              BAsmCode[CodeLen++] = SrcAdrVals.Arg;
            }
            else
            {
              BAsmCode[CodeLen++] = (0xa8 + Code) | DestAdrVals.Arg;
              BAsmCode[CodeLen++] = SrcAdrVals.Arg;
            }
            AppendAdrVals(&SrcAdrVals);
            break;
          case AdrModeImm:
            /* SrcAdrVals.Cnt must be 3 */
            if ((DestAdrVals.Arg < 2) && (SrcAdrVals.Vals[0] < 0x04))
            {
              BAsmCode[CodeLen++] = 0xca | DestAdrVals.Arg | (SrcAdrVals.Vals[0] << 4);
              BAsmCode[CodeLen++] = SrcAdrVals.Vals[1];
              BAsmCode[CodeLen++] = SrcAdrVals.Vals[2];
            }
            else
            {
              PutCode((DestAdrVals.Arg == 2) ? 0x1b03 : (0x98 | DestAdrVals.Arg));
              AppendAdrVals(&SrcAdrVals);
            }
            break;
          default:
            break;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeLEA(Word Code)
{
  tAdrVals DestAdrVals, SrcAdrVals;
  Byte Reg;

  UNUSED(Code);

  if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (ChkArgCnt(2, 2) && DecodeAdr(1, MModeReg | MModeAReg, &DestAdrVals))
  {
    switch (DestAdrVals.Mode)
    {
      case AdrModeReg:
        if (DestAdrVals.Arg < 6) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        else if (DecodeAdr(2, MModeMemReg, &SrcAdrVals))
        {
          if (IsReg(&SrcAdrVals, &Reg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
          {
            BAsmCode[CodeLen++] = 0x00 | DestAdrVals.Arg;
            BAsmCode[CodeLen++] = SrcAdrVals.Arg;
            AppendAdrVals(&SrcAdrVals);
          }
        }
        break;
      case AdrModeAReg:
        if (DestAdrVals.Arg > 2) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        else if (DecodeAdr(2, MModeMemReg, &SrcAdrVals))
        {
          if (IsReg(&SrcAdrVals, &Reg)) WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
          else
          {
            /* XYS,(i8,XYS) */

            if (((SrcAdrVals.Arg & 0xce) == 0xc0) /* ...,(XYS,i9) */
             && ((SrcAdrVals.Arg & 0x01) == ((SrcAdrVals.Vals[0] >> 7) & 1)) /* i9 is i8 */
             && (DestAdrVals.Arg == ((SrcAdrVals.Arg >> 4) & 3))) /* destreg==srcreg */
            {
              BAsmCode[CodeLen++] = 0x18 | DestAdrVals.Arg;
              BAsmCode[CodeLen++] = SrcAdrVals.Vals[0];
            }
            else
            {
              BAsmCode[CodeLen++] = 0x08 | DestAdrVals.Arg;
              BAsmCode[CodeLen++] = SrcAdrVals.Arg;
              AppendAdrVals(&SrcAdrVals);
            }
          }
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeMIN_MAX(Word Code)
{
  tAdrVals AdrVals;
  Byte Reg;

  if (ChkArgCnt(2, 2)
   && DecodeRegArg(1, &Reg, 0xff)
   && SetOpSize(RegSizes[Reg])
   && DecodeAdr(2, MModeMemReg | MModeImm, &AdrVals))
  {
    switch (AdrVals.Mode)
    {
      case AdrModeMemReg:
        PutCode(Code | Reg);
        BAsmCode[CodeLen++] = AdrVals.Arg;
        AppendAdrVals(&AdrVals);
        break;
      case AdrModeImm: /* was only allowed for short immediate in MemReg */
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[2]);
        break;
      default:
        break;
    }
  }
}

static void DecodeMOV(Word Code)
{
  tAdrVals SrcAdrVals, DestAdrVals;
  Byte Reg, SizeCode;

  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(2, MModeMemReg, &DestAdrVals))
  {
    /* prefer attribute to destination... */

    if (IsReg(&DestAdrVals, &Reg) && (OpSize == eSymbolSizeUnknown))
      SetOpSize(RegSizes[Reg]);

    if (!DecodeAdr(1, MModeMemReg | MModeImm, &SrcAdrVals))
      return;

    /* ...to source operand size */

    if (IsReg(&SrcAdrVals, &Reg) && (OpSize == eSymbolSizeUnknown))
      SetOpSize(RegSizes[Reg]);

    if (!SizeCode2(OpSize, &SizeCode))
    {
      WrError(ErrNum_InvOpSize);
      return;
    }

    switch (SrcAdrVals.Mode)
    {
      case AdrModeImm:
        PutCode(0x0c + SizeCode);
        AppendAdrVals(&SrcAdrVals);
        BAsmCode[CodeLen++] = DestAdrVals.Arg;
        AppendAdrVals(&DestAdrVals);
        break;
      case AdrModeMemReg:
        PutCode(0x1c | SizeCode);
        BAsmCode[CodeLen++] = SrcAdrVals.Arg;
        AppendAdrVals(&SrcAdrVals);
        BAsmCode[CodeLen++] = DestAdrVals.Arg;
        AppendAdrVals(&DestAdrVals);
        break;
      default:
        break;
    }
  }
}

static void DecodePSH_PUL(Word Code)
{
  Word RegMask = 0, ThisRegMask;
  int z;
  Byte Reg;
  static const Word RegMasks[8] = { 0x0002, 0x0001, 0x2000, 0x1000, 0x0008, 0x004, 0x0800, 0x0400 };

  if (!ChkArgCnt(1, ArgCntMax))
    return;
  for (z = 1; z <= ArgCnt; z++)
  {
    if (!as_strcasecmp(ArgStr[z].str.p_str, "ALL"))
      ThisRegMask = 0x3f3f;
    else if (!as_strcasecmp(ArgStr[z].str.p_str, "ALL16b"))
      ThisRegMask = 0x3003;
    else if (DecodeRegStr(ArgStr[z].str.p_str, &Reg))
      ThisRegMask = RegMasks[Reg];
    else if (!as_strcasecmp(ArgStr[z].str.p_str, "CCH"))
      ThisRegMask = 0x0020;
    else if (!as_strcasecmp(ArgStr[z].str.p_str, "CCL"))
      ThisRegMask = 0x0010;
    else if (DecodeAdrRegStr(ArgStr[z].str.p_str, &Reg) && (Reg < 2))
      ThisRegMask = 0x0200 >> Reg;
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[z]);
      return;
    }
    if (ThisRegMask & RegMask)
    {
      WrStrErrorPos(ErrNum_DoubleReg, &ArgStr[z]);
      return;
    }
    RegMask |= ThisRegMask;
  }
  if (RegMask == 0x3f3f)
    PutCode(Code | 0x00);
  else if (RegMask == 0x3003)
    PutCode(Code | 0x40);
  else if (Hi(RegMask) && !Lo(RegMask))
    PutCode(Code | 0x40 | Hi(RegMask));
  else if (Lo(RegMask) && !Hi(RegMask))
    PutCode(Code | 0x00 | Lo(RegMask));
  else
    WrError(ErrNum_InvRegList);
}

static void DecodeROL_ROR(Word Code)
{
  tAdrVals AdrVals;
  Byte Reg, SizeCode;

  if (ChkArgCnt(1, 1) && DecodeAdr(1, MModeMemReg, &AdrVals))
  {
    if (IsReg(&AdrVals, &Reg) && !SetOpSize(RegSizes[Reg]))
      return;
    if (OpSize == eSymbolSizeUnknown)
    {
      WrError(ErrNum_UndefOpSizes);
      return;
    }
    if (!SizeCode2(OpSize, &SizeCode))
    {
      WrError(ErrNum_InvOpSize);
      return;
    }
    PutCode(Code | SizeCode);
    BAsmCode[CodeLen++] = AdrVals.Arg;
    AppendAdrVals(&AdrVals);
  }
}

static void DecodeTBcc(Word Code)
{
  tAdrVals AdrVals;

  if (!ChkArgCnt(2, 2) || !DecodeAdr(1, MModeReg | MModeAReg | MModeMemReg | MModeImm, &AdrVals))
    return;

  switch (AdrVals.Mode)
  {
    case AdrModeReg:
      if (!SetOpSize(RegSizes[AdrVals.Arg]))
        return;
      PutCode(Code | AdrVals.Arg);
      break;
    case AdrModeAReg:
      if (AdrVals.Arg >= 2)
      {
        WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
        return;
      }
      if (!SetOpSize(eSymbolSize24Bit))
        return;
      PutCode(Code | 0x0008 | AdrVals.Arg);
      break;
    case AdrModeMemReg:
    {
      Byte SizeCode;

      if (OpSize == eSymbolSizeUnknown)
      {
        WrError(ErrNum_UndefOpSizes);
        return;
      }
      if (!SizeCode2(OpSize, &SizeCode))
      {
        WrError(ErrNum_InvOpSize);
        return;
      }
      PutCode(Code | 0x000c | SizeCode);
      BAsmCode[CodeLen++] = AdrVals.Arg;
      AppendAdrVals(&AdrVals);
      break;
    }
    case AdrModeImm: /* was only allowed for short immediate */
      WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
      return;
    default:
      return;
  }

  OpSize = OpSize2;
  if (!DecodeBranchCore(2))
    CodeLen = 0;
}

static void DecodeTRAP(Word Code)
{
  tAdrVals AdrVals;

  UNUSED(Code);

  if (ChkArgCnt(1, 1) && SetOpSize(eSymbolSize8Bit) && DecodeAdr(1, MModeImm, &AdrVals))
  {
    BAsmCode[CodeLen++] = 0x1b;
    BAsmCode[CodeLen++] = AdrVals.Vals[0];
    switch ((AdrVals.Vals[0] >> 4) & 0x0f)
    {
      case 12: case 13: case 14: case 15:
        break;
      case 10: case 11:
        if ((AdrVals.Vals[0] & 0x0f) >= 8)
          break;
        else
          goto warn;
      case 9:
        if ((AdrVals.Vals[0] & 0x0f) >= 2)
          break;
        /* else fall-through */
      default:
      warn:
        WrError(ErrNum_TrapValidInstruction);
    }
  }
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
    BitPos = EvalBitPosition(&ArgStr[2], &OK, (OpSize == eSymbolSizeUnknown) ? eSymbolSize32Bit : OpSize);
    if (!OK)
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = OpSize;
    pElement->BitPos = BitPos;
    pElement->ExpandFnc = ExpandS12ZBit;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize8Bit;
    if (OpSize > eSymbolSize32Bit)
    {
      WrError(ErrNum_InvOpSize);
      return;
    }

    if (DecodeBitArg(&BitSpec, 1, ArgCnt, OpSize))
    {
      *ListLine = '=';
      DissectBit_S12Z(ListLine + 1, STRINGSIZE - 3, BitSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

static void DecodeDEFBITFIELD(Word Code)
{
  UNUSED(Code);

  /* if in structure definition, add special element to structure */

  if (ActPC == StructSeg)
  {
    Word BitField;
    PStructElem pElement;

    if (!ChkArgCnt(2, 2))
      return;
    if (!DecodeImmBitField(&ArgStr[2], &BitField))
      return;
    pElement = CreateStructElem(&LabPart);
    if (!pElement)
      return;
    pElement->pRefElemName = as_strdup(ArgStr[1].str.p_str);
    pElement->OpSize = OpSize;
    pElement->BitPos = BitField & 31;
    pElement->BitWidthM1 = (BitField >> 5) - 1;
    pElement->ExpandFnc = ExpandS12ZBitfield;
    AddStructElem(pInnermostNamedStruct->StructRec, pElement);
  }
  else
  {
    LongWord BitfieldSpec;

    /* opposed to bit operations, bit field operations also work
        24 bit operands: */

    if (OpSize == eSymbolSizeUnknown)
      OpSize = eSymbolSize8Bit;
    if ((OpSize > eSymbolSize32Bit) && (OpSize != eSymbolSize24Bit))
    {
      WrError(ErrNum_InvOpSize);
      return;
    }

    if (DecodeBitfieldArg(&BitfieldSpec, 1, ArgCnt, OpSize))
    {
      *ListLine = '=';
      DissectBit_S12Z(ListLine + 1, STRINGSIZE - 3, BitfieldSpec);
      PushLocHandle(-1);
      EnterIntSymbol(&LabPart, BitfieldSpec, SegBData, False);
      PopLocHandle();
      /* TODO: MakeUseList? */
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Code Table Handling */

static void AddFixed(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeFixed);
}

static void AddBranch(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeBranch);
}

static void AddReg(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeReg);
}

static void AddRegMemImm(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeRegMemImm);
}

static void AddCondition(const char *pName, Word Code, InstProc Proc)
{
  char InstrName[20];

  as_snprintf(InstrName, sizeof(InstrName), pName, "NE"); AddInstTable(InstTable, InstrName, Code | (0 << 4), Proc);
  as_snprintf(InstrName, sizeof(InstrName), pName, "EQ"); AddInstTable(InstTable, InstrName, Code | (1 << 4), Proc);
  as_snprintf(InstrName, sizeof(InstrName), pName, "PL"); AddInstTable(InstTable, InstrName, Code | (2 << 4), Proc);
  as_snprintf(InstrName, sizeof(InstrName), pName, "MI"); AddInstTable(InstTable, InstrName, Code | (3 << 4), Proc);
  as_snprintf(InstrName, sizeof(InstrName), pName, "GT"); AddInstTable(InstTable, InstrName, Code | (4 << 4), Proc);
  as_snprintf(InstrName, sizeof(InstrName), pName, "LE"); AddInstTable(InstTable, InstrName, Code | (5 << 4), Proc);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(405);
  SetDynamicInstTable(InstTable);

  AddFixed("NOP",  NOPCode);
  AddFixed("BGND", 0x0000);
  AddFixed("CLC",  0xcefe);
  AddFixed("CLI",  0xceef);
  AddFixed("CLV",  0xcefd);
  AddFixed("RTI",  0x1b90);
  AddFixed("RTS",  0x0005);
  AddFixed("SEC",  0xde01);
  AddFixed("SEI",  0xde10);
  AddFixed("SEV",  0xde02);
  AddFixed("STOP", 0x1b05);
  AddFixed("SWI",  0x00ff);
  AddFixed("SYS",  0x1b07);
  AddFixed("WAI",  0x1b06);
  AddFixed("SPARE", 0x00ef);

  AddBranch("BCC", 0x0024);
  AddBranch("BCS", 0x0025);
  AddBranch("BEQ", 0x0027);
  AddBranch("BGE", 0x002c);
  AddBranch("BGT", 0x002e);
  AddBranch("BHI", 0x0022);
  AddBranch("BHS", 0x0024);
  AddBranch("BLE", 0x002f);
  AddBranch("BLO", 0x0025);
  AddBranch("BLS", 0x0023);
  AddBranch("BLT", 0x002d);
  AddBranch("BMI", 0x002b);
  AddBranch("BNE", 0x0026);
  AddBranch("BPL", 0x002a);
  AddBranch("BRA", 0x0020);
  AddBranch("BSR", 0x0021);
  AddBranch("BVC", 0x0028);
  AddBranch("BVS", 0x0029);

  AddReg("ABS", 0x1b40);
  AddReg("SAT", 0x1ba0);

  AddRegMemImm("ADC",  0x1b50);
  AddRegMemImm("ADD",  0x0050);
  AddRegMemImm("AND",  0x0058);
  AddRegMemImm("BIT",  0x1b58);
  AddRegMemImm("EOR",  0x1b78);
  AddRegMemImm("OR",   0x0078);
  AddRegMemImm("SBC",  0x1b70);

  AddInstTable(InstTable, "SUB", 0x0070, DecodeSUB);
  AddInstTable(InstTable, "CMP", 0x00e0, DecodeCMP);

  AddInstTable(InstTable, "ANDCC", 0x00ce, DecodeImm8);
  AddInstTable(InstTable, "ORCC" , 0x00de, DecodeImm8);
  AddInstTable(InstTable, "TRAP" , 0, DecodeTRAP);

  AddInstTable(InstTable, "ASL", 0xc0, DecodeShift);
  AddInstTable(InstTable, "ASR", 0x80, DecodeShift);
  AddInstTable(InstTable, "LSL", 0x40, DecodeShift);
  AddInstTable(InstTable, "LSR", 0x00, DecodeShift);

  AddInstTable(InstTable, "BCLR", 0xec, DecodeBit);
  AddInstTable(InstTable, "BSET", 0xed, DecodeBit);
  AddInstTable(InstTable, "BTGL", 0xee, DecodeBit);

  AddInstTable(InstTable, "BFEXT", 0x00, DecodeBitField);
  AddInstTable(InstTable, "BFINS", 0x80, DecodeBitField);

  AddInstTable(InstTable, "BRCLR", 0x02, DecodeBitRel);
  AddInstTable(InstTable, "BRSET", 0x03, DecodeBitRel);

  AddInstTable(InstTable, "CLB", 0x1b91, DecodeTwoReg);

  AddInstTable(InstTable, "CLR", 0x0000, DecodeCLR);
  AddInstTable(InstTable, "COM", 0x00cc, DecodeCOM_NEG);
  AddInstTable(InstTable, "NEG", 0x00dc, DecodeCOM_NEG);
  AddCondition("DB%s", 0x0d80, DecodeDBcc);
  AddInstTable(InstTable, "DEC", 0x0040, DecodeINC_DEC);
  AddInstTable(InstTable, "INC", 0x0030, DecodeINC_DEC);

  AddInstTable(InstTable, "DIVS", 0x3081, DecodeDIV_MOD);
  AddInstTable(InstTable, "DIVU", 0x3001, DecodeDIV_MOD);
  AddInstTable(InstTable, "MODS", 0x3881, DecodeDIV_MOD);
  AddInstTable(InstTable, "MODU", 0x3801, DecodeDIV_MOD);
  AddInstTable(InstTable, "MACS", 0x4881, DecodeDIV_MOD);
  AddInstTable(InstTable, "MACU", 0x4801, DecodeDIV_MOD);
  AddInstTable(InstTable, "MULS", 0x4880, DecodeDIV_MOD);
  AddInstTable(InstTable, "MULU", 0x4800, DecodeDIV_MOD);
  AddInstTable(InstTable,"QMULS", 0xb081, DecodeDIV_MOD);
  AddInstTable(InstTable,"QMULU", 0xb001, DecodeDIV_MOD);

  AddInstTable(InstTable, "EXG", 0x00ae, DecodeEXG_TFR);
  AddInstTable(InstTable, "TFR", 0x009e, DecodeEXG_TFR);
  AddInstTable(InstTable, "SEX", 0x01ae, DecodeEXG_TFR);
  AddInstTable(InstTable, "ZEX", 0x019e, DecodeEXG_TFR);

  AddInstTable(InstTable, "JMP", 0x00aa, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 0x00ab, DecodeJMP_JSR);

  AddInstTable(InstTable, "LD" , 0x0000, DecodeLD_ST);
  AddInstTable(InstTable, "ST" , 0x0020, DecodeLD_ST);
  AddInstTable(InstTable, "MOV" , 0x0000, DecodeMOV);
  AddInstTable(InstTable, "LEA" , 0x0000, DecodeLEA);

  AddInstTable(InstTable, "MAXS", 0x1b28, DecodeMIN_MAX);
  AddInstTable(InstTable, "MAXU", 0x1b18, DecodeMIN_MAX);
  AddInstTable(InstTable, "MINS", 0x1b20, DecodeMIN_MAX);
  AddInstTable(InstTable, "MINU", 0x1b10, DecodeMIN_MAX);

  AddInstTable(InstTable, "PSH", 0x0400, DecodePSH_PUL);
  AddInstTable(InstTable, "PUL", 0x0480, DecodePSH_PUL);

  AddInstTable(InstTable, "ROL", 0x1064, DecodeROL_ROR);
  AddInstTable(InstTable, "ROR", 0x1024, DecodeROL_ROR);

  AddCondition("TB%s", 0x0b00, DecodeTBcc);

  AddInstTable(InstTable, "DEFBIT", 0, DecodeDEFBIT);
  AddInstTable(InstTable, "DEFBITFIELD", 0, DecodeDEFBITFIELD);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Semiglobal Functions */

static Boolean DecodeAttrPart_S12Z(void)
{
  int z;

  if (strlen(AttrPart.str.p_str) > 2)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  for (z = 0; z < 2; z++)
  {
    if (AttrPart.str.p_str[z] == '\0')
      break;
    if (!DecodeMoto16AttrSize(AttrPart.str.p_str[z], &AttrPartOpSize[z], True))
      return False;
  }
  return True;
}

static void MakeCode_S12Z(void)
{
  CodeLen = 0;
  DontPrint = False;

  OpSize = (AttrPartOpSize[0] != eSymbolSizeUnknown) ? AttrPartOpSize[0] : eSymbolSizeUnknown;
  OpSize2 = (AttrPartOpSize[1] != eSymbolSizeUnknown) ? AttrPartOpSize[1] : eSymbolSizeUnknown;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  /* TODO: handle eSymbolSize24Bit in DC/DS */

  if (DecodeMoto16Pseudo(OpSize, True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_S12Z(void)
{
  return Memo("DEFBIT") || Memo("DEFBITFIELD");
}

static void SwitchTo_S12Z(void)
{
  const TFamilyDescr *pDescr = FindFamilyByName("S12Z");
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = pDescr->Id;
  NOPCode = 0x01;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffffff;
  DecodeAttrPart = DecodeAttrPart_S12Z;
  MakeCode = MakeCode_S12Z;
  IsDef = IsDef_S12Z;
  SwitchFrom = DeinitFields;
  DissectBit = DissectBit_S12Z;
  InitFields();
  AddMoto16PseudoONOFF(False);
}

void codes12z_init(void)
{
  (void)AddCPU("S912ZVC19F0MKH" , SwitchTo_S12Z);
  (void)AddCPU("S912ZVC19F0MLF" , SwitchTo_S12Z);
  (void)AddCPU("S912ZVCA19F0MKH", SwitchTo_S12Z);
  (void)AddCPU("S912ZVCA19F0MLF", SwitchTo_S12Z);
  (void)AddCPU("S912ZVCA19F0WKH", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH128F2CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH128F2CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH64F2CLQ" , SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY64F1CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY32F1CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY64F1CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY32F1CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHL64F1CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHL32F1CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHL64F1CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHL32F1CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVFP64F1CLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVFP64F1CLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH128F2VLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH128F2VLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVH64F2VLQ" , SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY64F1VLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY32F1VLQ", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY64F1VL" , SwitchTo_S12Z);
  (void)AddCPU("S912ZVHY32F1VLL", SwitchTo_S12Z);
  (void)AddCPU("S912ZVHL64F1VLQ", SwitchTo_S12Z);
}
