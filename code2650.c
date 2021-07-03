/* code2650.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Signetics 2650                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "chunks.h"
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

#include "code2650.h"

/*--------------------------------------------------------------------------*/
/* Local Variables */

static CPUVar CPU2650;

/*--------------------------------------------------------------------------*/
/* Expression Parsers */

static Boolean DecodeReg(const char *pAsc, Byte *pRes)
{
  Boolean Result;

  Result = ((strlen(pAsc) == 2) && (as_toupper(pAsc[0]) == 'R') && (pAsc[1] >= '0') && (pAsc[1] <= '3'));
  if (Result)
    *pRes = pAsc[1] - '0';
  return Result;
}

static Boolean DecodeCondition(const char *pAsc, Byte *pRes)
{
  Boolean Result = TRUE;

  if (!as_strcasecmp(pAsc, "EQ"))
    *pRes = 0;
  else if (!as_strcasecmp(pAsc, "GT"))
    *pRes = 1;
  else if (!as_strcasecmp(pAsc, "LT"))
    *pRes = 2;
  else if ((!as_strcasecmp(pAsc, "ALWAYS")) || (!as_strcasecmp(pAsc, "UN")))
    *pRes = 3;
  else
    Result = FALSE;

  return Result;
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Index; CodeLen = 1;
  }
}

static void DecodeOneReg(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Reg; CodeLen = 1;
  }
}

static void DecodeImm(Word Index)
{
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = Index; CodeLen = 2;
    }
  }
}

static void DecodeRegImm(Word Index)
{
  Byte Reg;
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = Index | Reg; CodeLen = 2;
    }
  }
}

static void DecodeRegAbs(Word Index)
{
  Byte DReg, IReg;
  Word AbsVal;
  Boolean OK, IndFlag;

  if (!ChkArgCnt(2, 4));
  else if (!DecodeReg(ArgStr[1].str.p_str, &DReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    IndFlag = *ArgStr[2].str.p_str == '*';
    AbsVal = EvalStrIntExpressionOffs(&ArgStr[2], IndFlag, UInt13, &OK);
    if (OK)
    {
      BAsmCode[0] = Index;
      BAsmCode[1] = Hi(AbsVal);
      BAsmCode[2] = Lo(AbsVal);
      if (IndFlag)
        BAsmCode[1] |= 0x80;
      if (ArgCnt == 2)
      {
        BAsmCode[0] |= DReg;
        CodeLen = 3;
      }
      else
      {
        if (!DecodeReg(ArgStr[3].str.p_str, &IReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
        else if (DReg != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] |= IReg;
          if (ArgCnt == 3)
          {
            BAsmCode[1] |= 0x60;
            CodeLen = 3;
          }
          else if (!strcmp(ArgStr[4].str.p_str, "-"))
          {
            BAsmCode[1] |= 0x40;
            CodeLen = 3;
          }
          else if (!strcmp(ArgStr[4].str.p_str, "+"))
          {
            BAsmCode[1] |= 0x20;
            CodeLen = 3;
          }
          else
            WrError(ErrNum_InvAddrMode);
        }
      }
    }
  }
}

static void DecodeRegRel(Word Index)
{
  Byte Reg;
  Boolean IndFlag, OK;
  int Dist;
  tSymbolFlags Flags;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Reg;
    IndFlag = *ArgStr[2].str.p_str == '*';
    Dist = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], IndFlag, UInt13, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (((Dist < - 64) || (Dist > 63)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[1] = Dist & 0x7f;
        if (IndFlag)
          BAsmCode[1] |= 0x80;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeCondAbs(Word Index)
{
  Byte Cond;
  Word Address;
  Boolean OK, IndFlag;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeCondition(ArgStr[1].str.p_str, &Cond)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
  else
  {
    IndFlag = *ArgStr[2].str.p_str == '*';
    Address = EvalStrIntExpressionOffs(&ArgStr[2], IndFlag, UInt13, &OK);
    if (OK)
    {
      BAsmCode[0] = Index | Cond;
      BAsmCode[1] = Hi(Address);
      if (IndFlag)
        BAsmCode[1] |= 0x80;
      BAsmCode[2] = Lo(Address);
      CodeLen = 3;
    }
  }
}

static void DecodeCondRel(Word Index)
{
  Byte Cond;
  Boolean IndFlag, OK;
  tSymbolFlags Flags;
  int Dist;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeCondition(ArgStr[1].str.p_str, &Cond)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Cond;
    IndFlag = *ArgStr[2].str.p_str == '*';
    Dist = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], IndFlag, UInt13, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (((Dist < - 64) || (Dist > 63)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[1] = Dist & 0x7f;
        if (IndFlag)
          BAsmCode[1] |= 0x80;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeRegAbs2(Word Index)
{
  Byte Reg;
  Word AbsVal;
  Boolean IndFlag, OK;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Reg;
    IndFlag = *ArgStr[2].str.p_str == '*';
    AbsVal = EvalStrIntExpressionOffs(&ArgStr[2], IndFlag, UInt13, &OK);
    if (OK)
    {
      BAsmCode[1] = Hi(AbsVal);
      if (IndFlag)
        BAsmCode[1] |= 0x80;
      BAsmCode[2] = Lo(AbsVal);
      CodeLen = 3;
    }
  }
}

static void DecodeBrAbs(Word Index)
{
  Byte Reg = 3;
  Word AbsVal;
  Boolean IndFlag, OK;

  if (!ChkArgCnt(1, 2));
  else if ((ArgCnt == 2) && (!DecodeReg(ArgStr[2].str.p_str, &Reg))) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (Reg != 3) WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[0] = Index | Reg;
    IndFlag = *ArgStr[1].str.p_str == '*';
    AbsVal = EvalStrIntExpressionOffs(&ArgStr[1], IndFlag, UInt13, &OK);
    if (OK)
    {
      BAsmCode[1] = Hi(AbsVal);
      if (IndFlag)
        BAsmCode[1] |= 0x80;
      BAsmCode[2] = Lo(AbsVal);
      CodeLen = 3;
    }
  }
}

static void DecodeCond(Word Index)
{
  Byte Cond;

  if (!ChkArgCnt(1, 1));
  else if (!DecodeCondition(ArgStr[1].str.p_str, &Cond)) WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Cond;
    CodeLen = 1;
  }
}

static void DecodeZero(Word Index)
{
  Boolean IndFlag, OK;

  if (ChkArgCnt(1, 1))
  {
    BAsmCode[0] = Index;
    IndFlag = *ArgStr[1].str.p_str == '*';
    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], IndFlag, UInt7, &OK);
    if (OK)
    {
      if (IndFlag)
        BAsmCode[1] |= 0x80;
      CodeLen = 2;
    }
  }
}

/*--------------------------------------------------------------------------*/
/* Code Table Handling */

static void AddFixed(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeFixed);
}

static void AddOneReg(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeOneReg);
}

static void AddImm(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeImm);
}

static void AddRegImm(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeRegImm);
}

static void AddRegAbs(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeRegAbs);
}

static void AddRegRel(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeRegRel);
}

static void AddCondAbs(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeCondAbs);
}

static void AddCondRel(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeCondRel);
}

static void AddRegAbs2(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeRegAbs2);
}

static void AddBrAbs(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeBrAbs);
}

static void AddCond(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeCond);
}

static void AddZero(const char *pName, Word Code)
{
  AddInstTable(InstTable, pName, Code, DecodeZero);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);

  AddFixed("NOP", 0xc0);
  AddFixed("HALT", 0x40);
  AddFixed("LPSL", 0x93);
  AddFixed("LPSU", 0x92);
  AddFixed("SPSL", 0x13);
  AddFixed("SPSU", 0x12);

  AddOneReg("ADDZ", 0x80);
  AddOneReg("ANDZ", 0x40);
  AddOneReg("COMZ", 0xe0);
  AddOneReg("DAR", 0x94);
  AddOneReg("EORZ", 0x20);
  AddOneReg("IORZ", 0x60);
  AddOneReg("LODZ", 0x00);
  AddOneReg("REDC", 0x30);
  AddOneReg("REDD", 0x70);
  AddOneReg("RRL", 0xd0);
  AddOneReg("RRR", 0x50);
  AddOneReg("STRZ", 0xc0);
  AddOneReg("SUBZ", 0xa0);
  AddOneReg("WRTC", 0xb0);
  AddOneReg("WRTD", 0xf0);

  AddImm("CPSL", 0x75);
  AddImm("CPSU", 0x74);
  AddImm("PPSL", 0x77);
  AddImm("PPSU", 0x76);
  AddImm("TPSL", 0xb5);
  AddImm("TPSU", 0xb4);

  AddRegImm("ADDI", 0x84);
  AddRegImm("ANDI", 0x44);
  AddRegImm("COMI", 0xe4);
  AddRegImm("EORI", 0x24);
  AddRegImm("IORI", 0x64);
  AddRegImm("LODI", 0x04);
  AddRegImm("REDE", 0x54);
  AddRegImm("SUBI", 0xa4);
  AddRegImm("TMI", 0xf4);
  AddRegImm("WRTE", 0xd4);

  AddRegAbs("ADDA", 0x8c);
  AddRegAbs("ANDA", 0x4c);
  AddRegAbs("COMA", 0xec);
  AddRegAbs("EORA", 0x2c);
  AddRegAbs("IORA", 0x6c);
  AddRegAbs("LODA", 0x0c);
  AddRegAbs("STRA", 0xcc);
  AddRegAbs("SUBA", 0xac);

  AddRegRel("ADDR", 0x88);
  AddRegRel("ANDR", 0x48);
  AddRegRel("BDRR", 0xf8);
  AddRegRel("BIRR", 0xd8);
  AddRegRel("BRNR", 0x58);
  AddRegRel("BSNR", 0x78);
  AddRegRel("COMR", 0xe8);
  AddRegRel("EORR", 0x28);
  AddRegRel("IORR", 0x68);
  AddRegRel("LODR", 0x08);
  AddRegRel("STRR", 0xc8);
  AddRegRel("SUBR", 0xa8);

  AddCondAbs("BCFA", 0x9c);
  AddCondAbs("BCTA", 0x1c);
  AddCondAbs("BSFA", 0xbc);
  AddCondAbs("BSTA", 0x3c);

  AddCondRel("BCFR", 0x98);
  AddCondRel("BCTR", 0x18);
  AddCondRel("BSFR", 0xb8);
  AddCondRel("BSTR", 0x38);

  AddRegAbs2("BDRA", 0xfc);
  AddRegAbs2("BIRA", 0xdc);
  AddRegAbs2("BRNA", 0x5c);
  AddRegAbs2("BSNA", 0x7c);

  AddBrAbs("BSXA", 0xbf);
  AddBrAbs("BXA", 0x9f);

  AddCond("RETC", 0x14);
  AddCond("RETE", 0x34);

  AddZero("ZBRR", 0x9b);
  AddZero("ZBSR", 0xbb);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------*/
/* Callbacks */

static void MakeCode_2650(void)
{
  char *pPos;

  CodeLen = 0;

  DontPrint = False;

  /* Nullanweisung */

  if ((*OpPart.str.p_str == '\0') && (ArgCnt == 0))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  /* try to split off first (register) operand from instruction */

  pPos = strchr(OpPart.str.p_str, ',');
  if (pPos)
  {
    InsertArg(1, strlen(OpPart.str.p_str));
    StrCompSplitRight(&OpPart, &ArgStr[1], pPos);
  }

  /* alles aus der Tabelle */

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_2650(void)
{
  return FALSE;
}

static void SwitchFrom_2650(void)
{
  DeinitFields();
}

static void SwitchTo_2650(void)
{
  PFamilyDescr pDescr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  pDescr = FindFamilyByName("2650");
  PCSymbol = "$"; HeaderID = pDescr->Id; NOPCode = 0xc0;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0x1fffl;

  MakeCode = MakeCode_2650; IsDef = IsDef_2650;

  SwitchFrom = SwitchFrom_2650; InitFields();
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

void code2650_init(void)
{
  CPU2650 = AddCPU("2650", SwitchTo_2650);
}
