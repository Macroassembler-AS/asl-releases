/* codecop8.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegeneratormodul COP8-Familie                                           */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "natpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codecop8.h"

#define ModNone (-1)
#define ModAcc 0
#define MModAcc (1 << ModAcc)
#define ModBInd 1
#define MModBInd (1 << ModBInd)
#define ModBInc 2
#define MModBInc (1 << ModBInc)
#define ModBDec 3
#define MModBDec (1 << ModBDec)
#define ModXInd 4
#define MModXInd (1 << ModXInd)
#define ModXInc 5
#define MModXInc (1 << ModXInc)
#define ModXDec 6
#define MModXDec (1 << ModXDec)
#define ModDir 7
#define MModDir (1 << ModDir)
#define ModImm 8
#define MModImm (1 << ModImm)

#define DirPrefix 0xbd
#define BReg 0xfe

#define BitOrderCnt 3

static CPUVar CPUCOP87L84;

typedef struct
{
  ShortInt Mode;
  Byte Val;
  tSymbolFlags ValSymFlags;
} tAdrResult;

/*---------------------------------------------------------------------------*/

static ShortInt DecodeAdr(const tStrComp *pArg, Word Mask, tAdrResult *pResult)
{
  static const char ModStrings[ModXDec + 1][5] =
  {
    "A", "[B]", "[B+]", "[B-]",
    "[X]", "[X+]", "[X-]"
  };

  int z;
  tEvalResult EvalResult;

  pResult->Mode = ModNone;

   /* indirekt/Akku */

  for (z = ModAcc; z <= ModXDec; z++)
    if (!as_strcasecmp(pArg->str.p_str, ModStrings[z]))
    {
      pResult->Mode = z;
      goto chk;
    }

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    pResult->Val = EvalStrIntExpressionOffsWithResult(pArg, 1, Int8, &EvalResult);
    if (EvalResult.OK)
    {
      pResult->Mode = ModImm;
      pResult->ValSymFlags = EvalResult.Flags;
    }
    goto chk;
  }

  /* direkt */

  pResult->Val = EvalStrIntExpressionWithResult(pArg, Int8, &EvalResult);
  if (EvalResult.OK)
  {
    pResult->Mode = ModDir;
    pResult->ValSymFlags = EvalResult.Flags;
    ChkSpace(SegData, EvalResult.AddrSpaceMask);
  }

chk:
  if ((pResult->Mode != ModNone) && !(Mask & (1 << pResult->Mode)))
  {
    pResult->Mode = ModNone; WrError(ErrNum_InvAddrMode);
  }
  return pResult->Mode;
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeLD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModAcc | MModDir | MModBInd | MModBInc | MModBDec, &DestResult))
    {
      case ModAcc:
      {
        tAdrResult SrcResult;

        switch(DecodeAdr(&ArgStr[2], MModDir | MModImm | MModBInd | MModXInd | MModBInc | MModXInc | MModBDec | MModXDec, &SrcResult))
        {
          case ModDir:
            BAsmCode[0] = 0x9d;
            BAsmCode[1] = SrcResult.Val;
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x98;
            BAsmCode[1] = SrcResult.Val;
            CodeLen = 2;
            break;
          case ModBInd:
            BAsmCode[0] = 0xae;
            CodeLen = 1;
            break;
          case ModXInd:
            BAsmCode[0] = 0xbe;
            CodeLen = 1;
            break;
          case ModBInc:
            BAsmCode[0] = 0xaa;
            CodeLen = 1;
            break;
          case ModXInc:
            BAsmCode[0] = 0xba;
            CodeLen = 1;
            break;
          case ModBDec:
            BAsmCode[0] = 0xab;
            CodeLen = 1;
            break;
          case ModXDec:
            BAsmCode[0] = 0xbb;
            CodeLen = 1;
            break;
        }
        break;
      }
      case ModDir:
      {
        tAdrResult SrcResult;

        if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) == ModImm)
        {
          if (DestResult.Val == BReg)
          {
            if (SrcResult.Val <= 15)
            {
              BAsmCode[0] = 0x5f - SrcResult.Val;
              CodeLen = 1;
            }
            else
            {
              BAsmCode[0] = 0x9f;
              BAsmCode[1] = SrcResult.Val;
              CodeLen = 2;
            }
          }
          else if (DestResult.Val >= 0xf0)
          {
            BAsmCode[0] = DestResult.Val - 0x20;
            BAsmCode[1] = SrcResult.Val;
            CodeLen = 2;
          }
          else
          {
            BAsmCode[0] = 0xbc;
            BAsmCode[1] = DestResult.Val;
            BAsmCode[2] = SrcResult.Val;
            CodeLen = 3;
          }
        }
        break;
      }
      case ModBInd:
      {
        tAdrResult SrcResult;

        if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) != ModNone)
        {
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = SrcResult.Val;
          CodeLen = 2;
        }
        break;
      }
      case ModBInc:
      {
        tAdrResult SrcResult;

        if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) != ModNone)
        {
          BAsmCode[0] = 0x9a;
          BAsmCode[1] = SrcResult.Val;
          CodeLen = 2;
        }
        break;
      }
      case ModBDec:
      {
        tAdrResult SrcResult;

        if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) != ModNone)
        {
          BAsmCode[0] = 0x9b;
          BAsmCode[1] = SrcResult.Val;
          CodeLen = 2;
        }
        break;
      }
    }
  }
}

static void DecodeX(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tStrComp *pAccArg, *pMemArg;
    tAdrResult DestResult;

    if (as_strcasecmp(ArgStr[1].str.p_str, "A"))
    {
      pAccArg = &ArgStr[2];
      pMemArg = &ArgStr[1];
    }
    else
    {
      pAccArg = &ArgStr[1];
      pMemArg = &ArgStr[2];
    }
    if (DecodeAdr(pAccArg, MModAcc, &DestResult) != ModNone)
    {
      tAdrResult SrcResult;

      switch (DecodeAdr(pMemArg, MModDir | MModBInd | MModXInd | MModBInc | MModXInc | MModBDec | MModXDec, &SrcResult))
      {
        case ModDir:
          BAsmCode[0] = 0x9c;
          BAsmCode[1] = SrcResult.Val;
          CodeLen = 2;
          break;
        case ModBInd:
          BAsmCode[0] = 0xa6;
          CodeLen = 1;
          break;
        case ModBInc:
          BAsmCode[0] = 0xa2;
          CodeLen = 1;
          break;
        case ModBDec:
          BAsmCode[0] = 0xa3;
          CodeLen = 1;
          break;
        case ModXInd:
          BAsmCode[0] = 0xb6;
          CodeLen = 1;
          break;
        case ModXInc:
          BAsmCode[0] = 0xb2;
          CodeLen = 1;
          break;
        case ModXDec:
          BAsmCode[0] = 0xb3;
          CodeLen = 1;
          break;
      }
    }
  }
}

static void DecodeAcc(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult DestResult;

    if (DecodeAdr(&ArgStr[1], MModAcc, &DestResult) == ModAcc)
    {
      BAsmCode[0] = Code;
      CodeLen = 1;
    }
  }
}

static void DecodeAccMem(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    if (DecodeAdr(&ArgStr[1], MModAcc, &DestResult) != ModNone)
    {
      tAdrResult SrcResult;

      switch (DecodeAdr(&ArgStr[2], MModDir | MModImm | MModBInd, &SrcResult))
      {
        case ModBInd:
          BAsmCode[0] = Code;
          CodeLen = 1;
          break;
        case ModImm:
          BAsmCode[0] = Code + 0x10;
          BAsmCode[1] = SrcResult.Val;
          CodeLen = 2;
          break;
        case ModDir:
          BAsmCode[0] = DirPrefix;
          BAsmCode[1] = SrcResult.Val;
          BAsmCode[2] = Code;
          CodeLen = 3;
          break;
      }
    }
  }
}

static void DecodeANDSZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    if (DecodeAdr(&ArgStr[1], MModAcc, &DestResult) != ModNone)
    {
      tAdrResult SrcResult;

      if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) == ModImm)
      {
        BAsmCode[0] = 0x60;
        BAsmCode[1] = SrcResult.Val;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeIFEQ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModAcc | MModDir, &DestResult))
    {
      case ModAcc:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModDir | MModBInd | MModImm, &SrcResult))
        {
          case ModDir:
            BAsmCode[0] = DirPrefix;
            BAsmCode[1] = SrcResult.Val;
            BAsmCode[2] = 0x82;
            CodeLen = 3;
            break;
          case ModBInd:
            BAsmCode[0] = 0x82;
            CodeLen = 1;
            break;
          case ModImm:
            BAsmCode[0] = 0x92;
            BAsmCode[1] = SrcResult.Val;
            CodeLen = 2;
            break;
        }
        break;
      }
      case ModDir:
      {
        tAdrResult SrcResult;

        BAsmCode[1] = DestResult.Val;
        if (DecodeAdr(&ArgStr[2], MModImm, &SrcResult) == ModImm)
        {
          BAsmCode[0] = 0xa9;
          BAsmCode[2] = SrcResult.Val;
          CodeLen = 3;
        }
        break;
      }
    }
  }
}

static void DecodeIFNE(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModAcc, &DestResult))
    {
      case ModAcc:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModDir | MModBInd | MModImm, &SrcResult))
        {
          case ModDir:
            BAsmCode[0] = DirPrefix;
            BAsmCode[1] = SrcResult.Val;
            BAsmCode[2] = 0xb9;
            CodeLen = 3;
            break;
          case ModBInd:
            BAsmCode[0] = 0xb9;
            CodeLen = 1;
            break;
          case ModImm:
            BAsmCode[0] = 0x99;
            BAsmCode[1] = SrcResult.Val;
            CodeLen = 2;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeIFBNE(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[0] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt4, &OK);
    if (OK)
    {
      BAsmCode[0] += 0x40;
      CodeLen = 1;
    }
  }
}

static void DecodeBit(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Boolean OK;

    Byte HReg = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
    {
      tAdrResult DestResult;

      switch (DecodeAdr(&ArgStr[2], MModDir | MModBInd, &DestResult))
      {
        case ModDir:
          BAsmCode[0] = DirPrefix;
          BAsmCode[1] = DestResult.Val;
          BAsmCode[2] = Code + HReg;
          CodeLen = 3;
          break;
        case ModBInd:
          BAsmCode[0] = Code + HReg;
          CodeLen = 1;
          break;
      }
    }
  }
}

static void DecodeJMP_JSR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);

    if (EvalResult.OK && ChkSamePage(EProgCounter() + 2, AdrWord, 12, EvalResult.Flags))
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[0] = 0x20 + Code + ((AdrWord >> 8) & 15);
      BAsmCode[1] = Lo(AdrWord);
      CodeLen = 2;
    }
  }
}

static void DecodeJMPL_JSRL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrWord = EvalStrIntExpressionWithResult(&ArgStr[1], UInt16, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[0] = Code;
      BAsmCode[1] = Hi(AdrWord);
      BAsmCode[2] = Lo(AdrWord);
      CodeLen = 3;
    }
  }
}

static void DecodeJP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;

    Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags) - (EProgCounter() + 1);
    if (OK)
    {
      if (AdrInt == 0)
      {
        BAsmCode[0] = NOPCode;
        CodeLen = 1;
        WrError(ErrNum_DistNull);
      }
      else if (((AdrInt > 31) || (AdrInt < -32)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = AdrInt & 0xff;
        CodeLen = 1;
      }
    }
  }
  return;
}

static void DecodeDRSZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult DestResult;

    if (DecodeAdr(&ArgStr[1], MModDir, &DestResult) == ModDir)
    {
      if (mFirstPassUnknown(DestResult.ValSymFlags))
        DestResult.Val |= 0xf0;
      if (DestResult.Val < 0xf0) WrError(ErrNum_UnderRange);
      else
      {
        BAsmCode[0] = DestResult.Val - 0x30;
        CodeLen = 1;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddAcc(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAcc);
}

static void AddAccMem(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAccMem);
}

static void AddBit(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(103);
  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "X", 0, DecodeX);
  AddInstTable(InstTable, "ANDSZ", 0, DecodeANDSZ);
  AddInstTable(InstTable, "IFEQ", 0, DecodeIFEQ);
  AddInstTable(InstTable, "IFNE", 0, DecodeIFNE);
  AddInstTable(InstTable, "IFBNE", 0, DecodeIFBNE);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 0x10, DecodeJMP_JSR);
  AddInstTable(InstTable, "JMPL", 0xac, DecodeJMPL_JSRL);
  AddInstTable(InstTable, "JSRL", 0xad, DecodeJMPL_JSRL);
  AddInstTable(InstTable, "JP", 0, DecodeJP);
  AddInstTable(InstTable, "DRSZ", 0, DecodeDRSZ);

  AddFixed("LAID" , 0xa4);  AddFixed("SC"   , 0xa1);  AddFixed("RC"   , 0xa0);
  AddFixed("IFC"  , 0x88);  AddFixed("IFNC" , 0x89);  AddFixed("VIS"  , 0xb4);
  AddFixed("JID"  , 0xa5);  AddFixed("RET"  , 0x8e);  AddFixed("RETSK", 0x8d);
  AddFixed("RETI" , 0x8f);  AddFixed("INTR" , 0x00);  AddFixed("NOP"  , 0xb8);
  AddFixed("RPND" , 0xb5);

  AddAcc("CLR"  , 0x64);  AddAcc("INC"  , 0x8a);  AddAcc("DEC"  , 0x8b);
  AddAcc("DCOR" , 0x66);  AddAcc("RRC"  , 0xb0);  AddAcc("RLC"  , 0xa8);
  AddAcc("SWAP" , 0x65);  AddAcc("POP"  , 0x8c);  AddAcc("PUSH" , 0x67);

  AddAccMem("ADD"  , 0x84);  AddAccMem("ADC"  , 0x80);  AddAccMem("SUBC" , 0x81);
  AddAccMem("AND"  , 0x85);  AddAccMem("OR"   , 0x87);  AddAccMem("XOR"  , 0x86);
  AddAccMem("IFGT" , 0x83);

  AddBit("IFBIT", 0x70); AddBit("SBIT", 0x78); AddBit("RBIT", 0x68);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_COP8(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeNatPseudo()) return;
  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_COP8(void)
{
  return (Memo("SFR"));
}

static void SwitchFrom_COP8(void)
{
  DeinitFields();
}

static void SwitchTo_COP8(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = ".";
  HeaderID = 0x6f;
  NOPCode = 0xb8;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode]  =0;
  SegLimits[SegCode] = 0x1fff;
  Grans[SegData] = 1;
  ListGrans[SegData] = 1;
  SegInits[SegData] = 0;
  SegLimits[SegData] = 0xff;

  MakeCode = MakeCode_COP8;
  IsDef = IsDef_COP8;
  SwitchFrom = SwitchFrom_COP8;
  InitFields();
}

void codecop8_init(void)
{
  CPUCOP87L84 = AddCPU("COP87L84", SwitchTo_COP8);
}
