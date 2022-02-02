/* codescmp.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator National SC/MP                                              */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>

#include "nls.h"
#include "strutil.h"
#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "intpseudo.h"
#include "codevars.h"
#include "codepseudo.h"
#include "errmsg.h"

#include "codescmp.h"

/*---------------------------------------------------------------------------*/

static CPUVar CPUSCMP;

/*---------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     DecodeReg(const char *pArg, Byte *pResult)
 * \brief  check whether argument is a CPU register
 * \param  pArg source argument
 * \param  pResult resulting register # if yes
 * \return True if argument is a register
 * ------------------------------------------------------------------------ */

static Boolean DecodeReg(const char *pArg, Byte *pResult)
{
  int l = strlen(pArg);

  switch (l)
  {
    case 2:
      if (as_toupper(*pArg) != 'P')
        return False;
      pArg++;
      /* fall-through */
    case 1:
      switch (as_toupper(*pArg))
      {
        case '0':
        case '1':
        case '2':
        case '3':
          *pResult = *pArg - '0';
          return True;
        case 'C':
          *pResult = 0;
          return (l == 2);
      }
  }
  return False;
}

static Boolean DecodeAdr(const tStrComp *pArg, Boolean MayInc, Byte PCDisp, Byte *Arg)
{
  Word Target;
  Boolean OK;
  int l, SplitPos;
  tSymbolFlags Flags;

  if (((SplitPos = FindDispBaseSplit(pArg->str.p_str, &l)) >= 0) && (l >= 4))
  {
    tStrComp Left, Right;

    StrCompSplitRef(&Left, &Right, pArg, pArg->str.p_str + SplitPos);
    StrCompShorten(&Right, 1);
    if (DecodeReg(Right.str.p_str, Arg))
    {
      if (*Left.str.p_str == '@')
      {
        if (!MayInc)
        {
          WrError(ErrNum_InvAddrMode);
          return False;
        }
        StrCompIncRefLeft(&Left, 1);
        *Arg += 4;
      }
      if (!as_strcasecmp(Left.str.p_str, "E"))
        BAsmCode[1] = 0x80;
      else if (*Arg == 0)
      {
        WrStrErrorPos(ErrNum_InvReg, &Right);
        return False;
      }
      else
      {
        BAsmCode[1] = EvalStrIntExpression(&Left, SInt8, &OK);
        if (!OK)
          return False;
      }
      return True;
    }
    else pArg->str.p_str[l - 1] = ')';
  }

  /* no carry in PC from bit 11 to 12; additionally handle preincrement */

  Target = EvalStrIntExpressionWithFlags(pArg, UInt16, &OK, &Flags);
  if (OK)
  {
    Word PCVal = (EProgCounter() & 0xf000) + ((EProgCounter() + 1 + PCDisp) & 0xfff);
    Word Disp = (Target - PCVal) & 0xfff;

    if (mSymbolQuestionable(Flags))
      Target = PCVal;

    if (!ChkSamePage(Target, PCVal, 12, Flags));
    else if ((Disp > 0x7f) && (Disp < 0xf80)) WrError(ErrNum_JmpDistTooBig);
    else
    {
      BAsmCode[1] = Disp & 0xff;
      *Arg = 0;
      return True;
    }
  }
  return False;
}

static void ChkPage(void)
{
  if (((EProgCounter()) & 0xf000) != ((EProgCounter() + CodeLen) & 0xf000))
    WrError(ErrNum_PageCrossing);
}

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Index; CodeLen = 1;
  }
}

static void DecodeImm(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    if (OK)
    {
      BAsmCode[0] = Index; CodeLen = 2; ChkPage();
    }
  }
}

static void DecodeRegOrder(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (!DecodeReg(ArgStr[1].str.p_str, BAsmCode + 0)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] |= Index; CodeLen = 1;
  }
}

static void DecodeMem(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], True, 0, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

static void DecodeJmp(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], False, 1, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

static void DecodeLD(Word Index)
{
  if (ChkArgCnt(1, 1))
  if (DecodeAdr(&ArgStr[1], False, 0, BAsmCode + 0))
  {
    BAsmCode[0] |= Index; CodeLen = 2; ChkPage();
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddImm(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm);
}

static void AddReg(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRegOrder);
}

static void AddMem(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeMem);
}

static void AddJmp(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddFixed("LDE" ,0x40); AddFixed("XAE" ,0x01); AddFixed("ANE" ,0x50);
  AddFixed("ORE" ,0x58); AddFixed("XRE" ,0x60); AddFixed("DAE" ,0x68);
  AddFixed("ADE" ,0x70); AddFixed("CAE" ,0x78); AddFixed("SIO" ,0x19);
  AddFixed("SR"  ,0x1c); AddFixed("SRL" ,0x1d); AddFixed("RR"  ,0x1e);
  AddFixed("RRL" ,0x1f); AddFixed("HALT",0x00); AddFixed("CCL" ,0x02);
  AddFixed("SCL" ,0x03); AddFixed("DINT",0x04); AddFixed("IEN" ,0x05);
  AddFixed("CSA" ,0x06); AddFixed("CAS" ,0x07); AddFixed("NOP" ,0x08);

  AddImm("LDI" , 0xc4); AddImm("ANI" , 0xd4); AddImm("ORI" , 0xdc);
  AddImm("XRI" , 0xe4); AddImm("DAI" , 0xec); AddImm("ADI" , 0xf4);
  AddImm("CAI" , 0xfc); AddImm("DLY" , 0x8f);

  AddReg("XPAL", 0x30); AddReg("XPAH", 0x34); AddReg("XPPC", 0x3c);

  AddMem("LD"  , 0xc0); AddMem("ST"  , 0xc8); AddMem("AND" , 0xd0);
  AddMem("OR"  , 0xd8); AddMem("XOR" , 0xe0); AddMem("DAD" , 0xe8);
  AddMem("ADD" , 0xf0); AddMem("CAD" , 0xf8);

  AddJmp("JMP" , 0x90); AddJmp("JP"  , 0x94); AddJmp("JZ"  , 0x98);
  AddJmp("JNZ" , 0x9c);

  AddInstTable(InstTable, "ILD", 0xa8, DecodeLD);
  AddInstTable(InstTable, "DLD", 0xb8, DecodeLD);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_SCMP(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(TargetBigEndian)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_SCMP(void)
{
  return False;
}

static void SwitchTo_SCMP(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeC);

  PCSymbol = "$"; HeaderID = 0x6e; NOPCode = 0x08;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_SCMP; IsDef = IsDef_SCMP;
  SwitchFrom = DeinitFields; InitFields();

  onoff_bigendian_add();

  QualifyQuote = QualifyQuote_SingleQuoteConstant;
  IntConstModeIBMNoTerm = True;
}

void codescmp_init(void)
{
  CPUSCMP = AddCPU("SC/MP", SwitchTo_SCMP);
}
