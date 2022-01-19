/* codevector.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Atari Asteroids Vector Processor                            */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"
#include "codepseudo.h"

#include "codevector.h"

static CPUVar CPUVector;

/*--------------------------------------------------------------------------
 * Operand Parsers
 *--------------------------------------------------------------------------*/

static Boolean Is4(const char *pAsc, Word *pResult)
{
  *pResult = 0;

  for (; *pAsc; pAsc++)
  {
    if (!isdigit(*pAsc))
      return False;
    *pResult = (*pResult * 10) + (*pAsc - '0');
  }
  return (*pResult <= 15);
}

static Boolean DecodeScale(tStrComp *pArg, Word *pResult)
{
  Boolean OK;

  KillPrefBlanksStrComp(pArg);
  KillPostBlanksStrComp(pArg);

  if ((toupper(*pArg->str.p_str) == 'S') && (Is4(pArg->str.p_str + 1, pResult)))
    return True;

  *pResult = EvalStrIntExpression(pArg, UInt4, &OK);
  return OK;
}

static Boolean DecodeBright(tStrComp *pArg, Word *pResult)
{
  Boolean OK;

  KillPrefBlanksStrComp(pArg);
  KillPostBlanksStrComp(pArg);

  if ((toupper(*pArg->str.p_str) == 'Z') && (Is4(pArg->str.p_str + 1, pResult)))
    return True;

  *pResult = EvalStrIntExpression(pArg, UInt4, &OK);
  return OK;
}

static Boolean DecodeSign(tStrComp *pArg, Word *pResult, Boolean Signed)
{
  LongInt Val;
  Boolean OK;
  tSymbolFlags Flags;

  Val = EvalStrIntExpressionWithFlags(pArg, SInt16, &OK, &Flags);
  if (!OK)
    return False;

  if (mFirstPassUnknown(Flags))
    Val = 0;

  if (!ChkRange(Val, Signed ? -1023 : 0, 1023))
    return False;

  if ((Signed) && (Val < 0))
    *pResult = (-Val) | (1 << 10);
  else
    *pResult = Val;
  return True;
}

static Boolean DecodeXY(tStrComp *pArg, Word *pX, Word *pY, Boolean Signed)
{
  tStrComp Tot, Left, Right;
  char *pEnd, *pPos;

  KillPrefBlanksStrComp(pArg);
  KillPostBlanksStrComp(pArg);

  if (*pArg->str.p_str != '(')
  {
    WrError(ErrNum_BrackErr);
    return False;
  }
  StrCompRefRight(&Tot, pArg, 1);

  pEnd = Tot.str.p_str + strlen(Tot.str.p_str) - 1;
  if (*pEnd != ')')
  {
    WrError(ErrNum_BrackErr);
    return False;
  }
  *pEnd = '\0';
  Tot.Pos.Len--;

  pPos = strchr(Tot.str.p_str, ',');
  if (!pPos)
  {
    WrError(ErrNum_UseLessAttr);
    return False;
  }
  StrCompSplitRef(&Left, &Right, &Tot, pPos);

  if (!DecodeSign(&Left, pX, Signed))
    return False;
  if (!DecodeSign(&Right, pY, Signed))
    return False;

  return True;
}

/*--------------------------------------------------------------------------
 * Code Handlers
 *--------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
  {
    WAsmCode[0] = Index;
    CodeLen = 1;
  }
}

static void DecodeJmp(Word Index)
{
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    WAsmCode[0] = Index | EvalStrIntExpression(&ArgStr[1], UInt12, &OK);
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeLAbs(Word Index)
{
  Word X, Y, Scale;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (!DecodeXY(&ArgStr[1], &X, &Y, False));
  else if (!DecodeScale(&ArgStr[2], &Scale));
  else
  {
    WAsmCode[0] = 0xa000 | Y;
    WAsmCode[1] = (Scale << 12) | X;
    CodeLen = 2;
  }
}

static void DecodeVctr(Word Index)
{
  Word X, Y, Scale, Bright;

  UNUSED(Index);

  if (!ChkArgCnt(3, 3));
  else if (!DecodeXY(&ArgStr[1], &X, &Y, True));
  else if (!DecodeScale(&ArgStr[2], &Scale));
  else if (!DecodeBright(&ArgStr[3], &Bright));
  else
  {
    WAsmCode[0] = (Scale << 12) | Y;
    WAsmCode[1] = (Bright << 12) | X;
    CodeLen = 2;
  }
}

static void DecodeSVec(Word Index)
{
  Word X, Y, Scale, Bright;

  UNUSED(Index);

  if (!ChkArgCnt(3, 3));
  else if (!DecodeXY(&ArgStr[1], &X, &Y, True));
  else if ((X & 0xff) || (Y & 0xff)) WrError(ErrNum_NotAligned);
  else if (!DecodeScale(&ArgStr[2], &Scale));
  else if (Scale > 3) WrError(ErrNum_OverRange);
  else if (!DecodeBright(&ArgStr[3], &Bright));
  else
  {
    WAsmCode[0] = 0xf000
                | (Bright << 4)
                | ((X >> 8) & 3)
                | (Y & 0x300)
                | ((Scale & 1) << 11)
                | ((Scale & 2) << 2);
    CodeLen = 1;
  }
}

/*--------------------------------------------------------------------------
 * Instruction Table Handling
 *--------------------------------------------------------------------------*/

static void InitFields(void)
{
  InstTable = CreateInstTable(17);

  AddInstTable(InstTable, "RTSL", 0xd000, DecodeFixed);
  AddInstTable(InstTable, "HALT", 0xb0b0, DecodeFixed);

  AddInstTable(InstTable, "JMPL", 0xe000, DecodeJmp);
  AddInstTable(InstTable, "JSRL", 0xc000, DecodeJmp);

  AddInstTable(InstTable, "LABS", 0x0000, DecodeLAbs);
  AddInstTable(InstTable, "VCTR", 0x0000, DecodeVctr);
  AddInstTable(InstTable, "SVEC", 0x0000, DecodeSVec);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*--------------------------------------------------------------------------
 * Semipublic Functions
 *--------------------------------------------------------------------------*/

static Boolean IsDef_Vector(void)
{
  return FALSE;
}

static void SwitchFrom_Vector(void)
{
  DeinitFields();
}

static void MakeCode_Vector(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void SwitchTo_Vector(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("ATARI_VECTOR");

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "$"; HeaderID = FoundDescr->Id;

  /* NOP = ??? */

  NOPCode = 0x00000;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfff;

  MakeCode = MakeCode_Vector; IsDef = IsDef_Vector;
  SwitchFrom = SwitchFrom_Vector; InitFields();
}

/*--------------------------------------------------------------------------
 * Initialization
 *--------------------------------------------------------------------------*/

void codevector_init(void)
{
  CPUVector = AddCPU("ATARI_VECTOR", SwitchTo_Vector);
}
