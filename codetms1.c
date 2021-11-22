/* codetms1.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator TMS1000-Familie                                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codevars.h"
#include "headids.h"
#include "intpseudo.h"
#include "errmsg.h"

#include "codetms1.h"

static CPUVar CPU1000, CPU1100, CPU1200, CPU1300;
static IntType CodeAdrIntType, DataAdrIntType;

/* 2/3/4-bit-operand in instruction is bit-mirrored */

static const Byte BitMirr[16] =
{
   0,  8,  4, 12,  2, 10,  6, 14,
   1,  9,  5, 13,  3, 11,  7, 15
};

/*---------------------------------------------------------------------------*/
/* Decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Lo(Code);
    CodeLen = 1;
  }
}

static void DecodeConst4(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], Int4, &OK);
    if (OK)
    {
      BAsmCode[0] = BitMirr[BAsmCode[0] & 0x0f] | (Code & 0xf0);
      CodeLen = 1;
    }
  }
}

static void DecodeConst3(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
    {
      BAsmCode[0] = (BitMirr[BAsmCode[0] & 0x07] >> 1) | (Code & 0xf8);
      CodeLen = 1;
    }
  }
}

static void DecodeConst2(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
    {
      BAsmCode[0] = (BitMirr[BAsmCode[0] & 0x03] >> 2) | (Code & 0xfc);
      CodeLen = 1;
    }
  }
}

static void DecodeJmp(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Addr = EvalStrIntExpressionWithResult(&ArgStr[1], CodeAdrIntType, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      CodeLen = 1;
      BAsmCode[0] = (Code & 0xc0) | (Addr & 0x3f);
    }
  }
}

static void DecodeJmpL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Addr = EvalStrIntExpressionWithResult(&ArgStr[1], CodeAdrIntType, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      BAsmCode[0] = 0x10 | ((Addr >> 6) & 15); /* LDP... */
      BAsmCode[1] = (Code & 0xc0) | (Addr & 0x3f);
      CodeLen = 2;
    }
  }
}

/*---------------------------------------------------------------------------*/
/* Dynamic Instruction Table Handling */

static void InitFields(void)
{
  Boolean Is1100 = (MomCPU == CPU1100) || (MomCPU == CPU1300);

  InstTable = CreateInstTable(107);

  AddInstTable(InstTable, "TAY", Is1100 ? 0x20 : 0x24, DecodeFixed);
  AddInstTable(InstTable, "TYA", 0x23, DecodeFixed);
  AddInstTable(InstTable, "CLA", Is1100 ? 0x7f : 0x2f, DecodeFixed);

  AddInstTable(InstTable, "TAM", Is1100 ? 0x27 : 0x03, DecodeFixed);
  if (Is1100)
  {
    AddInstTable(InstTable, "TAMIYC", 0x25, DecodeFixed);
    AddInstTable(InstTable, "TAMDYN", 0x24, DecodeFixed);
  }
  else
  {
    AddInstTable(InstTable, "TAMIY", 0x20, DecodeFixed);
  }
  AddInstTable(InstTable, "TAMZA", Is1100 ? 0x26 : 0x04, DecodeFixed);
  AddInstTable(InstTable, "TMY", 0x22, DecodeFixed);
  AddInstTable(InstTable, "TMA", 0x21, DecodeFixed);
  AddInstTable(InstTable, "XMA", Is1100 ? 0x03 : 0x2e, DecodeFixed);

  AddInstTable(InstTable, "AMAAC", Is1100 ? 0x06 : 0x25, DecodeFixed);
  AddInstTable(InstTable, "SAMAN", Is1100 ? 0x3c : 0x27, DecodeFixed);
  AddInstTable(InstTable, "IMAC", Is1100 ? 0x3e : 0x28, DecodeFixed);
  AddInstTable(InstTable, "DMAN", Is1100 ? 0x07 : 0x2a, DecodeFixed);
  if (Is1100)
    AddInstTable(InstTable, "IAC", 0x70, DecodeFixed);
  else
    AddInstTable(InstTable, "IA", 0x0e, DecodeFixed);
  AddInstTable(InstTable, "IYC", Is1100 ? 0x05 : 0x2b, DecodeFixed);
  AddInstTable(InstTable, "DAN", Is1100 ? 0x77 : 0x07, DecodeFixed);
  AddInstTable(InstTable, "DYN", Is1100 ? 0x04 : 0x2c, DecodeFixed);
  AddInstTable(InstTable, "CPAIZ", Is1100 ? 0x3d : 0x2d, DecodeFixed);
  AddInstTable(InstTable, "A6AAC", Is1100 ? 0x7a : 0x06, DecodeFixed);
  AddInstTable(InstTable, "A8AAC", Is1100 ? 0x7e : 0x01, DecodeFixed);
  AddInstTable(InstTable, "A10AAC", Is1100 ? 0x79 : 0x05, DecodeFixed);
  if (Is1100)
  {
    AddInstTable(InstTable, "A2AAC", 0x78, DecodeFixed);
    AddInstTable(InstTable, "A3AAC", 0x74, DecodeFixed);
    AddInstTable(InstTable, "A4AAC", 0x7c, DecodeFixed);
    AddInstTable(InstTable, "A5AAC", 0x72, DecodeFixed);
    AddInstTable(InstTable, "A7AAC", 0x76, DecodeFixed);
    AddInstTable(InstTable, "A9AAC", 0x71, DecodeFixed);
    AddInstTable(InstTable, "A11AAC", 0x75, DecodeFixed);
    AddInstTable(InstTable, "A12AAC", 0x7d, DecodeFixed);
    AddInstTable(InstTable, "A13AAC", 0x73, DecodeFixed);
    AddInstTable(InstTable, "A14AAC", 0x7b, DecodeFixed);
  }

  AddInstTable(InstTable, "ALEM", Is1100 ? 0x01: 0x29, DecodeFixed);
  if (!Is1100)
  {
    AddInstTable(InstTable, "ALEC", 0x70, DecodeConst4);
  }

  if (Is1100)
  {
    AddInstTable(InstTable, "MNEA", 0x00, DecodeFixed);
  }
  AddInstTable(InstTable, "MNEZ", Is1100 ? 0x3f : 0x26, DecodeFixed);
  AddInstTable(InstTable, "YNEA", Is1100 ? 0x02 : 0x02, DecodeFixed);
  AddInstTable(InstTable, "YNEC", 0x50, DecodeConst4);

  AddInstTable(InstTable, "SBIT", 0x30, DecodeConst2);
  AddInstTable(InstTable, "RBIT", 0x34, DecodeConst2);
  AddInstTable(InstTable, "TBIT1", 0x38, DecodeConst2);

  AddInstTable(InstTable, "TCY", 0x40, DecodeConst4);
  AddInstTable(InstTable, "TCMIY", 0x60, DecodeConst4);

  AddInstTable(InstTable, "KNEZ", Is1100 ? 0x0e : 0x09, DecodeFixed);
  AddInstTable(InstTable, "TKA", 0x08, DecodeFixed);

  AddInstTable(InstTable, "SETR", 0x0d, DecodeFixed);
  AddInstTable(InstTable, "RSTR", 0x0c, DecodeFixed);
  AddInstTable(InstTable, "TDO", 0x0a, DecodeFixed);
  if (!Is1100)
  {
    AddInstTable(InstTable, "CLO", 0x0b, DecodeFixed);
  }

  if (Is1100)
  {
    AddInstTable(InstTable, "LDX", 0x28, DecodeConst3);
  }
  else
  {
    AddInstTable(InstTable, "LDX", 0x3c, DecodeConst2);
  }
  AddInstTable(InstTable, "COMX", Is1100 ? 0x09 : 0x00, DecodeFixed);

  AddInstTable(InstTable, "BR", 0x80, DecodeJmp);
  AddInstTable(InstTable, "CALL", 0xc0, DecodeJmp);
  AddInstTable(InstTable, "BL", 0x80, DecodeJmpL);
  AddInstTable(InstTable, "CALLL", 0xc0, DecodeJmpL);
  AddInstTable(InstTable, "RETN", 0x0f, DecodeFixed);
  AddInstTable(InstTable, "LDP", 0x10, DecodeConst4);
  if (Is1100)
  {
    AddInstTable(InstTable, "COMC", 0x0b, DecodeFixed);
  }
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/
/* Interface */

static void MakeCode_TMS1(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True))
    return;

  /* remainder */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_TMS1(void)
{
  return False;
}

static void SwitchFrom_TMS1(void)
{
  DeinitFields();
}

static void SwitchTo_TMS1(void)
{
  const TFamilyDescr *pFoundDescr = FindFamilyByName("TMS1000");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = pFoundDescr->Id;
  NOPCode = 0x100;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  if ((MomCPU == CPU1000) || (MomCPU == CPU1200))
  {
    CodeAdrIntType = UInt10;
    DataAdrIntType = UInt6;
  }
  else if ((MomCPU == CPU1100) || (MomCPU == CPU1300))
  {
    CodeAdrIntType = UInt11;
    DataAdrIntType = UInt7;
  }
  SegLimits[SegCode] = IntTypeDefs[CodeAdrIntType].Max;
  SegLimits[SegData] = IntTypeDefs[DataAdrIntType].Max;

  MakeCode = MakeCode_TMS1;
  IsDef = IsDef_TMS1;
  SwitchFrom = SwitchFrom_TMS1;

  InitFields();
}

void codetms1_init(void)
{
  CPU1000 = AddCPU("TMS1000", SwitchTo_TMS1);
  CPU1100 = AddCPU("TMS1100", SwitchTo_TMS1);
  CPU1200 = AddCPU("TMS1200", SwitchTo_TMS1);
  CPU1300 = AddCPU("TMS1300", SwitchTo_TMS1);
}
