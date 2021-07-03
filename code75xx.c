/* code75xx.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator NEC 75xx                                                    */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "headids.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code75xx.h"

static CPUVar CPU7566, CPU7508;
static IntType CodeIntType, DataIntType;

/*-------------------------------------------------------------------------*/
/* code generation */

static void PutCode(Word Code)
{
  CodeLen = 0;
  if (Hi(Code))
    BAsmCode[CodeLen++] = Hi(Code);
  BAsmCode[CodeLen++] = Lo(Code);
}

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
    PutCode(Index);
}

static void DecodeImm4(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Value = EvalStrIntExpressionWithResult(&ArgStr[1], Int4, &EvalResult);

    if (EvalResult.OK)
    {
      PutCode(Index | (Value & 15));
      if (Memo("OP"))
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeImm5(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Value;

    Value = EvalStrIntExpression(&ArgStr[1], Int5, &OK);
    if (OK)
      PutCode(Index | (Value & 31));
  }
}

static void DecodeImm8(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Value;

    Value = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      PutCode(Index);
      BAsmCode[CodeLen++] = Value;
    }
  }
}

static void DecodeImm2(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word Value;

    Value = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);
    if (OK)
      PutCode(Index | (Value & 3));
  }
}

static void DecodeImm3(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Value = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);
    if (EvalResult.OK)
    {
      PutCode(Index | (Value & 7));
      if (Memo("OP"))
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodePR(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "HL-"))
    PutCode(Index | 0x10);
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "HL+"))
    PutCode(Index | 0x11);
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "HL"))
    PutCode(Index | 0x12);
  else if ((MomCPU == CPU7508) && (!as_strcasecmp(ArgStr[1].str.p_str, "DL")))
    PutCode(Index | 0x00);
  else if ((MomCPU == CPU7508) && (!as_strcasecmp(ArgStr[1].str.p_str, "DE")))
    PutCode(Index | 0x01);
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static void DecodeDataMem(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Value = EvalStrIntExpressionWithResult(&ArgStr[1], DataIntType, &EvalResult);
    if (EvalResult.OK)
    {
      PutCode(Index);
      BAsmCode[CodeLen++] = Value & SegLimits[SegData];
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeAbs(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address;
    IntType Type = CodeIntType;
    Word SegLimit = SegLimits[SegCode];

    /* CALL can only address first 2K on 7508! */

    if ((Memo("CALL")) && (MomCPU == CPU7508))
    {
      Type = UInt11;
      SegLimit = 0x1fff;
    }

    Address = EvalStrIntExpressionWithResult(&ArgStr[1], Type, &EvalResult);
    if (EvalResult.OK)
    {
      PutCode(Index | (Address & SegLimit));
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeJCP(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);

    if (EvalResult.OK && ChkSamePage(Address, EProgCounter() + 1, 6, EvalResult.Flags))
    {
      PutCode(Index | (Address & 0x3f));
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeCAL(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);

    if (mFirstPassUnknown(EvalResult.Flags))
      Address = (Address & 0x1c7) | 0x100;
    if (EvalResult.OK)
    {
      if ((Address & 0x338) != 0x100) WrError(ErrNum_OverRange);
      else
        PutCode(Index | ((Address & 0x0c0) >> 3) | (Address & 7));
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeLHLT(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);

    if (mFirstPassUnknown(EvalResult.Flags))
      Address = (Address & 0x00cf) | 0x00c0;
    if (EvalResult.OK)
    {
      if ((Address < 0xc0) || (Address > 0xcf)) WrError(ErrNum_OverRange);
      else
        PutCode(Index | (Address & 0xf));
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeCALT(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Address = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);

    if (mFirstPassUnknown(EvalResult.Flags))
      Address = (Address & 0x00ff) | 0x00c0;
    if (EvalResult.OK)
    {
      if ((Address < 0xd0) || (Address > 0xff)) WrError(ErrNum_OverRange);
      else
        PutCode(Index | (Address & 0x3f));
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeLogPort(Word Index)
{
  if (ChkArgCnt(2, 2))
  {
    tEvalResult EvalResult;
    Word Port, Mask;

    Port = EvalStrIntExpressionWithResult(&ArgStr[1], UInt4, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      Mask = EvalStrIntExpressionWithResult(&ArgStr[2], UInt4, &EvalResult);
      if (EvalResult.OK)
        PutCode(Index | ((Mask & 15) << 4) | (Port & 15));
    }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Codetabellenverwaltung */

static void AddFixed(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeFixed);
}

static void AddImm4(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeImm4);
}

static void AddImm2(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeImm2);
}

static void AddImm5(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeImm5);
}

static void AddImm8(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeImm8);
}

static void AddImm3(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeImm3);
}

static void AddPR(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodePR);
}

static void AddDataMem(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeDataMem);
}

static void AddAbs(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeAbs);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(103);
  AddFixed("ST"    , 0x57);
  AddFixed("XAL"   , 0x7b);
  AddFixed("ASC"   , 0x7d);
  AddFixed("ACSC"  , 0x7c);
  AddFixed("EXL"   , 0x7e);
  AddFixed("CMA"   , 0x7f);
  AddFixed("RC"    , 0x78);
  AddFixed("SC"    , 0x79);
  AddFixed("ILS"   , 0x59);
  AddFixed("DLS"   , 0x58);
  AddFixed("RT"    , 0x53);
  AddFixed("RTS"   , 0x5b);
  AddFixed("TAMSP" , 0x3f31);
  AddFixed("SKC"   , 0x5a);
  AddFixed("SKAEM" , 0x5f);
  AddFixed("TAMSIO", 0x3f3e);
  AddFixed("TSIOAM", 0x3f3a);
  AddFixed("SIO"   , 0x3f33);
  AddFixed("TIMER" , 0x3f32);
  AddFixed("TCNTAM", 0x3f3b);
  AddFixed("IPL"   , 0x70);
  AddFixed("OPL"   , 0x72);
  AddFixed("HALT"  , 0x3f36);
  AddFixed("STOP"  , 0x3f37);
  AddFixed("NOP"  , 0x00);
  if (MomCPU == CPU7566)
  {
    AddFixed("RPBL"  , 0x5c);
    AddFixed("SPBL"  , 0x5d);
  }
  if (MomCPU == CPU7508)
  {
    AddFixed("LAMTL", 0x3f34);
    AddFixed("TAD", 0x3eaa);
    AddFixed("TAE", 0x3e8a);
    AddFixed("TAH", 0x3eba);
    AddFixed("TAL", 0x3e9a);
    AddFixed("TDA", 0x3eab);
    AddFixed("TEA", 0x3e8b);
    AddFixed("THA", 0x3ebb);
    AddFixed("TLA", 0x3e9b);
    AddFixed("XAD", 0x4a);
    AddFixed("XAE", 0x4b);
    AddFixed("XAH", 0x7a);
    AddFixed("ANL", 0x3fb2);
    AddFixed("ORL", 0x3fb6);
    AddFixed("RAR", 0x3fb3);
    AddFixed("IES", 0x49);
    AddFixed("DES", 0x48);
    AddFixed("RTPSW", 0x43);
    AddFixed("PSHDE", 0x3e8e);
    AddFixed("PSHHL", 0x3e9e);
    AddFixed("POPDE", 0x3e8f);
    AddFixed("POPHL", 0x3e9f);
    AddFixed("TSPAM", 0x3f35);
    AddFixed("TAMMOD", 0x3f3f);
    AddFixed("IP1", 0x71);
    AddFixed("IP54", 0x3f38);
    AddFixed("OP3", 0x73);
    AddFixed("OP54", 0x3f3c);
  }

  AddImm4("LAI", 0x10);
  AddImm4("AISC", 0x00);
  AddImm4("SKAEI", 0x3f60);
  if (MomCPU == CPU7566)
  {
    AddImm4("STII", 0x40);
  }
  if (MomCPU == CPU7508)
  {
    AddImm4("LDI", 0x3e20);
    AddImm4("LEI", 0x3e00);
    AddImm4("LHI", 0x3e30);
    AddImm4("LLI", 0x3e10);
    AddImm4("JAM", 0x3f10);
    AddImm4("SKDEI", 0x3e60);
    AddImm4("SKEEI", 0x3e40);
    AddImm4("SKHEI", 0x3e70);
    AddImm4("SKLEI", 0x3e50);
    AddImm4("OP", 0x3fe0);
  }

  AddImm2("RMB", 0x68);
  AddImm2("SMB", 0x6c);
  AddImm2("SKABT", 0x74);
  AddImm2("SKMBT", 0x64);
  AddImm2("SKMBF", 0x60);
  if (MomCPU == CPU7566)
  {
    AddImm2("SKI", 0x3d40);
    AddImm2("LHI", 0x28);
  }

  if (MomCPU == CPU7508)
  {
    AddImm3("EI", 0x3f90);
    AddImm3("DI", 0x3f80);
    AddImm3("SKI", 0x3f40);
    AddImm3("IP", 0x3fc0);
  }

  if (MomCPU == CPU7566)
  {
    AddImm5("LHLI", 0xc0);
  }

  if (MomCPU == CPU7508)
  {
    AddImm8("LDEI", 0x4f);
    AddImm8("LHLI", 0x4e);
  }

  AddPR("LAM", 0x40);
  AddPR("XAM", 0x44);

  AddDataMem("IDRS", 0x3d);
  AddDataMem("DDRS", 0x3c);
  if (MomCPU == CPU7508)
  {
    AddDataMem("LADR", 0x38);
    AddDataMem("XADR", 0x39);
    AddDataMem("XHDR", 0x3a);
    AddDataMem("XLDR", 0x3b);
  }

  AddAbs("JMP" , 0x2000);
  AddAbs("CALL", 0x3000);
  AddInstTable(InstTable, "JCP", 0x80, DecodeJCP);
  if (MomCPU == CPU7566)
  {
    AddInstTable(InstTable, "CAL", 0xe0, DecodeCAL);
  }
  if (MomCPU == CPU7508)
  {
    AddInstTable(InstTable, "LHLT", 0xc0, DecodeLHLT);
    AddInstTable(InstTable, "CALT", 0xc0, DecodeCALT);
    AddInstTable(InstTable, "ANP", 0x4c00, DecodeLogPort);
    AddInstTable(InstTable, "ORP", 0x4d00, DecodeLogPort);
  }
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_75xx(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_75xx(void)
{
  return False;
}

static void SwitchFrom_75xx(void)
{
  DeinitFields();
}

static void SwitchTo_75xx(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC"; HeaderID = FindFamilyByName("75xx")->Id; NOPCode = 0x00;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  if (MomCPU == CPU7508)
  {
    ValidSegs |= (1 << SegIO);
    Grans[SegIO] = 1; ListGrans[SegIO] = 1; SegInits[SegIO] = 0; SegLimits[SegIO] = 15;

    SegLimits[SegData] = 0xff;
    DataIntType = UInt8;
    SegLimits[SegCode] = 0xfff;
    CodeIntType = UInt12;
  }
  else
  {
    SegLimits[SegData] = 0x3f;
    DataIntType = UInt6;
    SegLimits[SegCode] = 0x3ff;
    CodeIntType = UInt10;
  }

  MakeCode = MakeCode_75xx; IsDef = IsDef_75xx;
  SwitchFrom = SwitchFrom_75xx; InitFields();
}

void code75xx_init(void)
{
  CPU7566 = AddCPU("7566", SwitchTo_75xx);
  CPU7508 = AddCPU("7508", SwitchTo_75xx);
}
