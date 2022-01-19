/* codef8.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Code Generator Fairchild F8                                               */
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
#include "asmitree.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "codef8.h"

#define Flg_IO (1 << 8)
#define Flg_Code (1 << 9)
#define Flg_CMOS (1 << 10)

static IntType CodeIntType;
static Word MomCPUFlags;

/*---------------------------------------------------------------------------*/

static Boolean DecodeReg(const tStrComp *pArg, Byte *pResult)
{
  /* KU/KL/QU/QL are addressed via dedicated LR instructions! */
  static const char RegNames[][3] = { "J", "HU", "HL", "S", "I", "D", "" };
  int z;
  Boolean OK;

  for (z = 0; *RegNames[z]; z++)
    if (!as_strcasecmp(RegNames[z], pArg->str.p_str))
    {
      *pResult = z + 9;
      return True;
    }
  *pResult = EvalStrIntExpression(pArg, UInt4, &OK);
  return OK;
}

static Boolean DecodeAux(const char *pArg, Byte *pResult)
{
  if (strlen(pArg) != 2)
    return False;
  *pResult = 0;

  if (toupper(pArg[0]) == 'Q')
    *pResult |= 2;
  else if (toupper(pArg[0]) != 'K')
    return False;
  if (toupper(pArg[1]) == 'L')
    *pResult |= 1;
  else if (toupper(pArg[1]) != 'U')
    return False;
  return True;
}

static Boolean ArgPair(const char *pArg1, const char *pArg2)
{
  return !as_strcasecmp(ArgStr[1].str.p_str, pArg1) && !as_strcasecmp(ArgStr[2].str.p_str, pArg2);
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (!ChkArgCnt(0, 0));
  else if ((Code & Flg_CMOS) && (!(MomCPUFlags & Flg_CMOS))) WrError(ErrNum_InstructionNotSupported);
  else
  {
    BAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeImm8(Word Code)
{
  Boolean IsIO = !!(Code & Flg_IO);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;

    BAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[1], IsIO ? UInt8 : Int8, &EvalResult);
    if (EvalResult.OK)
    {
      if (IsIO)
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      BAsmCode[0] = Lo(Code);
      CodeLen = 2;
    }
  }
}

static void DecodeImm4(Word Code)
{
  Boolean IsIO = !!(Code & Flg_IO);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;

    BAsmCode[0] = Lo(Code) | EvalStrIntExpressionWithResult(&ArgStr[1], UInt4, &EvalResult);
    if (EvalResult.OK)
    {
      CodeLen = 1;
      if (IsIO)
        ChkSpace(SegIO, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeImm3(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[0] = Code | EvalStrIntExpression(&ArgStr[1], UInt3, &OK);
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeImm16(Word Code)
{
  Boolean IsCode = !!(Code & Flg_Code);

  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word Arg = EvalStrIntExpressionWithResult(&ArgStr[1], IsCode ? CodeIntType : Int16, &EvalResult);

    if (EvalResult.OK)
    {
      BAsmCode[0] = Code;
      BAsmCode[1] = Hi(Arg);
      BAsmCode[2] = Lo(Arg);
      CodeLen = 3;
      if (IsCode)
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

static void DecodeOneReg(Word Code)
{
  if (ChkArgCnt(1, 1) && DecodeReg(&ArgStr[1], &BAsmCode[0]))
  {
    BAsmCode[0] |= Code;
    CodeLen = 1;
  }
}

static void DecodeBranchCore(const tStrComp *pArg, Word Code)
{
  Boolean OK;
  LongInt Dist;
  tSymbolFlags Flags;

  Dist = EvalStrIntExpressionWithFlags(pArg, UInt16, &OK, &Flags) - (EProgCounter() + 1);
  if (OK)
  {
    if (!mSymbolQuestionable(Flags) && ((Dist < -128) || (Dist > 127))) WrStrErrorPos(ErrNum_JmpDistTooBig, pArg);
    else
    {
      BAsmCode[0] = Code;
      BAsmCode[1] = Dist & 0xff;
      CodeLen = 2;
    }
  }
}

static void DecodeBranch(Word Code)
{
  if (ChkArgCnt(1, 1))
    DecodeBranchCore(&ArgStr[1], Code);
}

static void DecodeGenBranch(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Boolean OK;

    /* BT does not include OVF flag in its condition mask, only values 0..7! */

    Code |= EvalStrIntExpression(&ArgStr[1], (Code == 0x90) ? UInt4 : UInt3, &OK);
    if (OK)
      DecodeBranchCore(&ArgStr[2], Code);
  }
}

static void DecodeLR(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2,2))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
  {
    if (DecodeAux(ArgStr[2].str.p_str, &BAsmCode[0]))
    {
      CodeLen = 1;
    }
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "IS"))
    {
      BAsmCode[0] = 0x0a;
      CodeLen = 1;
    }
    else if (DecodeReg(&ArgStr[2], &BAsmCode[0]))
    {
      BAsmCode[0] |= 0x40;
      CodeLen = 1;
    }
  }
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
  {
    if (DecodeAux(ArgStr[1].str.p_str, &BAsmCode[0]))
    {
      BAsmCode[0] |= 4;
      CodeLen = 1;
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "IS"))
      BAsmCode[CodeLen++] = 0x0b;
    else if (DecodeReg(&ArgStr[1], &BAsmCode[0]))
    {
      BAsmCode[0] |= 0x50;
      CodeLen = 1;
    }
  }
  else if (ArgPair("DC", "Q"))
    BAsmCode[CodeLen++] = 0x0f;
  else if (ArgPair("DC", "H"))
    BAsmCode[CodeLen++] = 0x10;
  else if (ArgPair("Q", "DC"))
    BAsmCode[CodeLen++] = 0x0e;
  else if (ArgPair("H", "DC"))
    BAsmCode[CodeLen++] = 0x11;
  else if (ArgPair("K", "P"))
    BAsmCode[CodeLen++] = 0x08;
  else if (ArgPair("P", "K"))
    BAsmCode[CodeLen++] = 0x09;
  else if (ArgPair("P0", "Q"))
    BAsmCode[CodeLen++] = 0x0d;
  else if (ArgPair("J", "W"))
    BAsmCode[CodeLen++] = 0x1e;
  else if (ArgPair("W", "J"))
    BAsmCode[CodeLen++] = 0x1d;
  else
    WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[1]);
}

static void DecodeShift(Word Code)
{
  if (!ArgCnt)
    BAsmCode[CodeLen++] = Code;
  else if (ChkArgCnt(1,1))
  {
    Boolean OK;
    Byte Cnt;
    tSymbolFlags Flags;

    Cnt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt3, &OK, &Flags);
    if (OK)
    {
      if (Cnt == 4)
        BAsmCode[CodeLen++] = Code + 2;
      else if (mFirstPassUnknown(Flags) || (Cnt == 1))
        BAsmCode[CodeLen++] = Code;
      else
        WrStrErrorPos(ErrNum_OverRange, &ArgStr[1]);
    }
  }
}

static void DecodePORT(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegIO, 0, 0xff);
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddImm8(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm8);
}

static void AddImm4(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm4);
}

static void AddImm3(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm3);
}

static void AddImm16(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm16);
}

static void AddOneReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeOneReg);
}

static void AddBranch(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBranch);
}

static void AddGenBranch(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeGenBranch);
}

static void AddShift(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeShift);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(101);

  AddFixed("ADC", 0x8e);
  AddFixed("AM" , 0x88);
  AddFixed("AMD", 0x89);
  AddFixed("CLR", 0x70);
  AddFixed("CM" , 0x8d);
  AddFixed("COM", 0x18);
  AddFixed("DI" , 0x1a);
  AddFixed("EI" , 0x1b);
  AddFixed("HET", Flg_CMOS | 0x2e); /* guess */
  AddFixed("HAL", Flg_CMOS | 0x2f); /* guess */
  AddFixed("INC", 0x1f);
  AddFixed("LM" , 0x16);
  AddFixed("LNK", 0x19);
  AddFixed("NM" , 0x8a);
  AddFixed("NOP", NOPCode);
  AddFixed("OM" , 0x8b);
  AddFixed("PK" , 0x0c);
  AddFixed("POP", 0x1c);
  AddFixed("ST" , 0x17);
  AddFixed("XDC", 0x2c);
  AddFixed("XM" , 0x8c);

  AddImm8("AI" , 0x24);
  AddImm8("CI" , 0x25);
  AddImm8("LI" , 0x20);
  AddImm8("NI" , 0x21);
  AddImm8("IN" , Flg_IO | 0x26);
  AddImm8("OI" , 0x22);
  AddImm8("OUT", Flg_IO | 0x27);
  AddImm8("XI" , 0x23);

  AddImm4("LIS" , 0x70);
  AddImm4("INS" , Flg_IO | 0xa0);
  AddImm4("OUTS", Flg_IO | 0xb0);

  AddImm3("LISL", 0x68);
  AddImm3("LISU", 0x60);

  AddImm16("DCI", 0x2a);
  AddImm16("JMP", Flg_Code | 0x29);
  AddImm16("PI" , Flg_Code | 0x28);

  AddOneReg("AS"  , 0xc0);
  AddOneReg("ASD" , 0xd0);
  AddOneReg("DS"  , 0x30);
  AddOneReg("NS"  , 0xf0);
  AddOneReg("XS"  , 0xe0);

  AddBranch("BC"  , 0x82);
  AddBranch("BM"  , 0x91);
  AddBranch("BNC" , 0x92);
  AddBranch("BNO" , 0x98);
  AddBranch("BNZ" , 0x94);
  AddBranch("BP"  , 0x81);
  AddBranch("BR"  , 0x90);
  AddBranch("BR7" , 0x8f);
  AddBranch("BZ"  , 0x84);
  AddGenBranch("BF", 0x90);
  AddGenBranch("BT", 0x80);

  AddShift("SL", 0x13);
  AddShift("SR", 0x12);

  AddInstTable(InstTable, "LR", 0, DecodeLR);
  AddInstTable(InstTable, "PORT", 0, DecodePORT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_F8(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen - DS is a machine instruction on F8 */

  if (!Memo("DS"))
  {
    if (DecodeIntelPseudo(True)) return;
  }

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void SwitchFrom_F8(void)
{
  DeinitFields();
}

static Boolean IsDef_F8(void)
{
  return Memo("PORT");
}

static void SwitchTo_F8(void)
{
  const TFamilyDescr *Descr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  Descr = FindFamilyByName("F8");
  PCSymbol = "$";
  HeaderID = Descr->Id;
  NOPCode = 0x2b;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegLimits[SegCode] = IntTypeDefs[CodeIntType].Max;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegLimits[SegData] = 0x3f;
  Grans[SegIO  ] = 1; ListGrans[SegIO  ] = 1; SegLimits[SegIO  ] = 0xff;

  MakeCode = MakeCode_F8;
  SwitchFrom = SwitchFrom_F8;
  IsDef = IsDef_F8;
  InitFields();
}

static void SwitchTo_F8_12(void)
{
  CodeIntType = UInt12;
  MomCPUFlags = 0;
  SwitchTo_F8();
}

static void SwitchTo_F8_16(void)
{
  CodeIntType = UInt16;
  MomCPUFlags = 0;
  SwitchTo_F8();
}

static void SwitchTo_F8_12_CMOS(void)
{
  CodeIntType = UInt12;
  MomCPUFlags = Flg_CMOS;
  SwitchTo_F8();
}

void codef8_init(void)
{
  (void)AddCPU("F3850"    , SwitchTo_F8_16);
  (void)AddCPU("MK3850"   , SwitchTo_F8_16);
  (void)AddCPU("MK3870"   , SwitchTo_F8_12);
  (void)AddCPU("MK3870/10", SwitchTo_F8_12);
  (void)AddCPU("MK3870/12", SwitchTo_F8_12);
  (void)AddCPU("MK3870/20", SwitchTo_F8_12); /* == MK3870 */
  (void)AddCPU("MK3870/22", SwitchTo_F8_12); /* == MK3876 */
  (void)AddCPU("MK3870/30", SwitchTo_F8_12);
  (void)AddCPU("MK3870/32", SwitchTo_F8_12);
  (void)AddCPU("MK3870/40", SwitchTo_F8_12);
  (void)AddCPU("MK3870/42", SwitchTo_F8_12); /* == MK3872 */
  (void)AddCPU("MK3872"   , SwitchTo_F8_12);
  (void)AddCPU("MK3873"   , SwitchTo_F8_12);
  (void)AddCPU("MK3873/10", SwitchTo_F8_12);
  (void)AddCPU("MK3873/12", SwitchTo_F8_12);
  (void)AddCPU("MK3873/20", SwitchTo_F8_12); /* == MK3873 */
  (void)AddCPU("MK3873/22", SwitchTo_F8_12);
  (void)AddCPU("MK3874"   , SwitchTo_F8_12);
  (void)AddCPU("MK3875"   , SwitchTo_F8_12);
  (void)AddCPU("MK3875/22", SwitchTo_F8_12); /* == MK3876 with standby */
  (void)AddCPU("MK3875/42", SwitchTo_F8_12); /* == MK3872 with standby */
  (void)AddCPU("MK3876"   , SwitchTo_F8_12);
  (void)AddCPU("MK38P70/02",SwitchTo_F8_12); /* == MK3874, MK974xx */
  (void)AddCPU("MK38CP70/02", SwitchTo_F8_12_CMOS);
  (void)AddCPU("MK38C70"  , SwitchTo_F8_12_CMOS);
  (void)AddCPU("MK38C70/10",SwitchTo_F8_12);
  (void)AddCPU("MK38C70/20",SwitchTo_F8_12); /* == MK38C70 */
  (void)AddCPU("MK97400"  , SwitchTo_F8_12);
  (void)AddCPU("MK97410"  , SwitchTo_F8_12);
  (void)AddCPU("MK97500"  , SwitchTo_F8_16);
  (void)AddCPU("MK97501"  , SwitchTo_F8_16);
  (void)AddCPU("MK97503"  , SwitchTo_F8_16);
}
