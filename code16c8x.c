/* code16c8x.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegenerator PIC16C8x                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "chunks.h"
#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "headids.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code16c8x.h"

/*---------------------------------------------------------------------------*/

#define AddCodeSpace 0x300

static CPUVar CPU16C64, CPU16C84, CPU16C873, CPU16C874, CPU16C876, CPU16C877;

/*--------------------------------------------------------------------------*/
/* helper functions */

static Word EvalFExpression(tStrComp *pArg, tEvalResult *pEvalResult)
{
  LongInt h;

  h = EvalStrIntExpressionWithResult(pArg, UInt9, pEvalResult);
  if (pEvalResult->OK)
  {
    ChkSpace(SegData, pEvalResult->AddrSpaceMask);
    return (h & 0x7f);
  }
  else
    return 0;
}

/*--------------------------------------------------------------------------*/
/* instruction decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    WAsmCode[CodeLen++] = Code;
    if (Memo("OPTION"))
      WrError(ErrNum_Obsolete);
  }
}

static void DecodeLit(Word Code)
{
  Word AdrWord;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    AdrWord = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
      WAsmCode[CodeLen++] = Code | Lo(AdrWord);
  }
}

static void DecodeAri(Word Code)
{
  Word DefaultDir = (Code >> 8) & 0x80;
  tEvalResult EvalResult;
  Word AdrWord;

  Code &= 0x7fff;

  if (ChkArgCnt(1, 2))
  {
    AdrWord = EvalFExpression(&ArgStr[1], &EvalResult);
    if (EvalResult.OK)
    {
      WAsmCode[0] = Code | AdrWord;
      if (1 == ArgCnt)
      {
        WAsmCode[0] |= DefaultDir;
        CodeLen = 1;
      }
      else if (!as_strcasecmp(ArgStr[2].str.p_str, "W"))
        CodeLen = 1;
      else if (!as_strcasecmp(ArgStr[2].str.p_str, "F"))
      {
         WAsmCode[0] |= 0x80;
        CodeLen = 1;
      }
      else
      {
        AdrWord = EvalStrIntExpressionWithResult(&ArgStr[2], UInt1, &EvalResult);
        if (EvalResult.OK)
        {
          WAsmCode[0] |= AdrWord << 7;
          CodeLen = 1;
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
      WAsmCode[0] = EvalFExpression(&ArgStr[1], &EvalResult);
      if (EvalResult.OK)
      {
        WAsmCode[0] |= Code | (AdrWord << 7);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeF(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalFExpression(&ArgStr[1], &EvalResult);

    if (EvalResult.OK)
      WAsmCode[CodeLen++] = Code | AdrWord;
  }
}

static void DecodeTRIS(Word Index)
{
  Word AdrWord;
  tEvalResult EvalResult;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);
    if (mFirstPassUnknown(EvalResult.Flags))
      AdrWord = 5;
    if (EvalResult.OK)
      if (ChkRange(AdrWord, 5, 6))
      {
        WAsmCode[CodeLen++] = 0x0060 | AdrWord;
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        WrError(ErrNum_Obsolete);
      }
  }
}

static void DecodeJump(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &EvalResult);
    if (EvalResult.OK)
    {
      if (AdrWord > (SegLimits[SegCode] - AddCodeSpace)) WrError(ErrNum_OverRange);
      else
      {
        Word XORVal, Mask, RegBit;

        ChkSpace(SegCode, EvalResult.AddrSpaceMask);

        XORVal = (ProgCounter() ^ AdrWord) & ~0x7ff;

        /* add BCF/BSF instruction for non-matching upper address bits
           - we might need to extend this for the PICs with more than
             8K of program space */

        for (RegBit = 3, Mask = 0x800; RegBit <= 4; RegBit++, Mask <<= 1)
          if (XORVal & Mask)
            WAsmCode[CodeLen++] = 0x100a
                                | (RegBit << 7)
                                | ((AdrWord & Mask) >> (RegBit - 2));

        WAsmCode[CodeLen++] = Index | (AdrWord & 0x7ff);
      }
    }
  }
}

static void DecodeSFR(Word Index)
{
  UNUSED(Index);

  CodeEquate(SegData, 0, 511);
}

static void DecodeDATA_16C8x(Word Index)
{
  UNUSED(Index);

  DecodeDATA(Int14, Int8);
}

static void DecodeZERO(Word Index)
{
  Word Size, Shift = (ActPC == SegCode) ? 1 : 0;
  Boolean ValOK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    Size = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &ValOK, &Flags);
    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (ValOK && !mFirstPassUnknown(Flags))
    {
      if (SetMaxCodeLen(Size << Shift)) WrError(ErrNum_CodeOverflow);
      else
      {
        CodeLen = Size;
        memset(WAsmCode, 0, Size << Shift);
      }
    }
  }
}

static void DecodeBANKSEL(Word Index)
{
  Word Adr;
  Boolean ValOK;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    Adr = EvalStrIntExpression(&ArgStr[1], UInt9, &ValOK);
    if (ValOK)
    {
      WAsmCode[0] = 0x1283 | ((Adr &  0x80) << 3); /* BxF Status, 5 */
      WAsmCode[1] = 0x1303 | ((Adr & 0x100) << 2); /* BxF Status, 6 */
      CodeLen = 2;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* dynamic code table handling */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddLit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLit);
}

static void AddAri(const char *NName, Word NCode, Word NDir)
{
  AddInstTable(InstTable, NName, NCode | (NDir << 15), DecodeAri);
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
  InstTable = CreateInstTable(201);

  AddFixed("CLRW"  , 0x0100);
  AddFixed("NOP"   , 0x0000);
  AddFixed("CLRWDT", 0x0064);
  AddFixed("OPTION", 0x0062);
  AddFixed("SLEEP" , 0x0063);
  AddFixed("RETFIE", 0x0009);
  AddFixed("RETURN", 0x0008);

  AddLit("ADDLW", 0x3e00);
  AddLit("ANDLW", 0x3900);
  AddLit("IORLW", 0x3800);
  AddLit("MOVLW", 0x3000);
  AddLit("RETLW", 0x3400);
  AddLit("SUBLW", 0x3c00);
  AddLit("XORLW", 0x3a00);

  AddAri("ADDWF" , 0x0700, 0);
  AddAri("ANDWF" , 0x0500, 0);
  AddAri("COMF"  , 0x0900, 1);
  AddAri("DECF"  , 0x0300, 1);
  AddAri("DECFSZ", 0x0b00, 1);
  AddAri("INCF"  , 0x0a00, 1);
  AddAri("INCFSZ", 0x0f00, 1);
  AddAri("IORWF" , 0x0400, 0);
  AddAri("MOVF"  , 0x0800, 0);
  AddAri("RLF"   , 0x0d00, 1);
  AddAri("RRF"   , 0x0c00, 1);
  AddAri("SUBWF" , 0x0200, 0);
  AddAri("SWAPF" , 0x0e00, 1);
  AddAri("XORWF" , 0x0600, 0);

  AddBit("BCF"  , 0x1000);
  AddBit("BSF"  , 0x1400);
  AddBit("BTFSC", 0x1800);
  AddBit("BTFSS", 0x1c00);

  AddF("CLRF" , 0x0180);
  AddF("MOVWF", 0x0080);

  AddInstTable(InstTable, "TRIS", 0, DecodeTRIS);
  AddInstTable(InstTable, "GOTO", 0x2800, DecodeJump);
  AddInstTable(InstTable, "CALL", 0x2000, DecodeJump);
  AddInstTable(InstTable, "SFR" , 0, DecodeSFR);
  AddInstTable(InstTable, "RES" , 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_16C8x);
  AddInstTable(InstTable, "ZERO", 0, DecodeZERO);

  AddInstTable(InstTable, "BANKSEL", 0, DecodeBANKSEL);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/

static void MakeCode_16c8x(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* seek instruction */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_16c8x(void)
{
  return (Memo("SFR"));
}

static Boolean ChkPC_16c8x(LargeWord Addr)
{
  if ((ActPC == SegCode) && (Addr > (LargeWord)SegLimits[SegCode]))
    return ((Addr >= 0x2000) && (Addr <= 0x2007));
  else
    return (Addr <= (LargeWord)SegLimits[ActPC]);
}

static void SwitchFrom_16c8x(void)
{
   DeinitFields();
}

static void SwitchTo_16c8x(void)
{
  const TFamilyDescr *pDescr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  pDescr = FindFamilyByName("16C8x");
  PCSymbol = "*";
  HeaderID = pDescr->Id;
  NOPCode = 0x0000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  if (MomCPU == CPU16C64)
    SegLimits[SegCode] = 0x7ff;
  else if (MomCPU == CPU16C873)
    SegLimits[SegCode] = 0x0fff;
  else if (MomCPU == CPU16C874)
    SegLimits[SegCode] = 0x0fff;
  else if (MomCPU == CPU16C876)
    SegLimits[SegCode] = 0x1fff;
  else if (MomCPU == CPU16C877)
    SegLimits[SegCode] = 0x1fff;
  else
    SegLimits[SegCode] = 0x3ff;

  SegLimits[SegCode] += AddCodeSpace;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = 0x1ff;
  Grans[SegEEData] = 1; ListGrans[SegEEData] = 1; SegInits[SegEEData] = 0;
  if ((MomCPU == CPU16C877) || (MomCPU == CPU16C876))
    SegLimits[SegEEData] = 0xff;
  else if ((MomCPU == CPU16C874) || (MomCPU == CPU16C873))
    SegLimits[SegEEData] = 0x7f;
  else
    SegLimits[SegEEData] = 0x3f;
  ChkPC = ChkPC_16c8x;

  MakeCode = MakeCode_16c8x;
  IsDef = IsDef_16c8x;
  SwitchFrom = SwitchFrom_16c8x;
  InitFields();
}

void code16c8x_init(void)
{
  CPU16C64  = AddCPU("16C64",  SwitchTo_16c8x);
  CPU16C84  = AddCPU("16C84",  SwitchTo_16c8x);
  CPU16C873 = AddCPU("16C873", SwitchTo_16c8x);
  CPU16C874 = AddCPU("16C874", SwitchTo_16c8x);
  CPU16C876 = AddCPU("16C876", SwitchTo_16c8x);
  CPU16C877 = AddCPU("16C877", SwitchTo_16c8x);
}
