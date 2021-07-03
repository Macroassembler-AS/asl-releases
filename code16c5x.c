/* code16c5x.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS - Codegenerator fuer PIC16C5x                                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code16c5x.h"

static CPUVar CPU16C54, CPU16C55, CPU16C56, CPU16C57;

/*-------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    WAsmCode[0] = Code;
  }
}

static void DecodeLit(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      CodeLen = 1;
      WAsmCode[0] = Code + (AdrWord & 0xff);
    }
  }
}

static void DecodeAri(Word Code)
{
  Word DefaultDir = (Code >> 15) & 1, AdrWord;

  Code &= 0x7fff;
  if (ChkArgCnt(1, 2))
  {
    tEvalResult EvalResult;

    AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt5, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
      WAsmCode[0] = Code + (AdrWord & 0x1f);
      if (ArgCnt == 1)
      {
        CodeLen = 1;
        WAsmCode[0] += DefaultDir << 5;
      }
      else if (!as_strcasecmp(ArgStr[2].str.p_str, "W"))
        CodeLen = 1;
      else if (!as_strcasecmp(ArgStr[2].str.p_str, "F"))
      {
        CodeLen = 1;
        WAsmCode[0] += 0x20;
      }
      else
      {
        AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], UInt1, &EvalResult);
        if (EvalResult.OK)
        {
          CodeLen = 1;
          WAsmCode[0] += AdrWord << 5;
        }
      }
    }
  }
}

static void DecodeBit(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], UInt3, &EvalResult);

    if (EvalResult.OK)
    {
      WAsmCode[0] = EvalStrIntExpressionWithResult(&ArgStr[1], UInt5, &EvalResult);
      if (EvalResult.OK)
      {
        CodeLen = 1;
        WAsmCode[0] += Code + (AdrWord << 5);
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
      }
    }
  }
}

static void DecodeF(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt5, &EvalResult);

    if (EvalResult.OK)
    {
      CodeLen = 1;
      WAsmCode[0] = Code + AdrWord;
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeTRIS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);

    if (EvalResult.OK)
     if (ChkRange(AdrWord, 5, 7))
     {
       CodeLen = 1;
       WAsmCode[0] = 0x000 + AdrWord;
       ChkSpace(SegData, EvalResult.AddrSpaceMask);
     }
  }
}

static void DecodeCALL_GOTO(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);

    if (EvalResult.OK)
    {
      if (AdrWord > SegLimits[SegCode]) WrError(ErrNum_OverRange);
      else if ((Code & 0x100) && ((AdrWord & 0x100) != 0)) WrError(ErrNum_NotFromThisAddress);
      else
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        if (((ProgCounter() ^ AdrWord) & 0x200) != 0)
          WAsmCode[CodeLen++] = 0x4a3 + ((AdrWord & 0x200) >> 1); /* BCF/BSF 3,5 */
        if (((ProgCounter() ^ AdrWord) & 0x400) != 0)
          WAsmCode[CodeLen++] = 0x4c3 + ((AdrWord & 0x400) >> 2); /* BCF/BSF 3,6 */
        WAsmCode[CodeLen++] = Code + (AdrWord & (Code & 0x100 ? 0xff : 0x1ff));
      }
    }
  }
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, 0x1f);
}

static void DecodeDATA_16C5x(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int12, Int8);
}

static void DecodeZERO(Word Code)
{
  Word Size;
  Boolean ValOK;
  tSymbolFlags Flags;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &ValOK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (ValOK && !mFirstPassUnknown(Flags))
    {
      if (SetMaxCodeLen(Size << 1)) WrError(ErrNum_CodeOverflow);
      else
      {
        CodeLen = Size;
        memset(WAsmCode, 0, 2 * Size);
      }
    }
  }
}

/*-------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddLit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLit);
}

static void AddAri(const char *NName, Word NCode, Word NDef)
{
  AddInstTable(InstTable, NName, NCode | (NDef << 15), DecodeAri);
}

static void AddBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void AddF(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeF);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(103);
  AddInstTable(InstTable, "TRIS", 0, DecodeTRIS);
  AddInstTable(InstTable, "CALL", 0x0900, DecodeCALL_GOTO);
  AddInstTable(InstTable, "GOTO", 0x0a00, DecodeCALL_GOTO);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_16C5x);
  AddInstTable(InstTable, "ZERO", 0, DecodeZERO);

  AddFixed("CLRW"  , 0x040);
  AddFixed("NOP"   , 0x000);
  AddFixed("CLRWDT", 0x004);
  AddFixed("OPTION", 0x002);
  AddFixed("SLEEP" , 0x003);

  AddLit("ANDLW", 0xe00);
  AddLit("IORLW", 0xd00);
  AddLit("MOVLW", 0xc00);
  AddLit("RETLW", 0x800);
  AddLit("XORLW", 0xf00);

  AddAri("ADDWF" , 0x1c0, 0);
  AddAri("ANDWF" , 0x140, 0);
  AddAri("COMF"  , 0x240, 1);
  AddAri("DECF"  , 0x0c0, 1);
  AddAri("DECFSZ", 0x2c0, 1);
  AddAri("INCF"  , 0x280, 1);
  AddAri("INCFSZ", 0x3c0, 1);
  AddAri("IORWF" , 0x100, 0);
  AddAri("MOVF"  , 0x200, 0);
  AddAri("RLF"   , 0x340, 1);
  AddAri("RRF"   , 0x300, 1);
  AddAri("SUBWF" , 0x080, 0);
  AddAri("SWAPF" , 0x380, 1);
  AddAri("XORWF" , 0x180, 0);

  AddBit("BCF"  , 0x400);
  AddBit("BSF"  , 0x500);
  AddBit("BTFSC", 0x600);
  AddBit("BTFSS", 0x700);

  AddF("CLRF" , 0x060);
  AddF("MOVWF", 0x020);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_16C5X(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_16C5X(void)
{
  return Memo("SFR");
}

static void SwitchFrom_16C5X(void)
{
  DeinitFields();
}

static void SwitchTo_16C5X(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x71;
  NOPCode = 0x000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) + (1 << SegData);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  if (MomCPU == CPU16C56)
    SegLimits[SegCode] = 1023;
  else if (MomCPU == CPU16C57)
    SegLimits[SegCode] = 2047;
  else
    SegLimits[SegCode] = 511;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegCode] = 0;
  SegLimits[SegData] = 0x1f;

  MakeCode = MakeCode_16C5X;
  IsDef = IsDef_16C5X;
  SwitchFrom = SwitchFrom_16C5X;
  InitFields();
}

void code16c5x_init(void)
{
  CPU16C54 = AddCPU("16C54", SwitchTo_16C5X);
  CPU16C55 = AddCPU("16C55", SwitchTo_16C5X);
  CPU16C56 = AddCPU("16C56", SwitchTo_16C5X);
  CPU16C57 = AddCPU("16C57", SwitchTo_16C5X);
}
