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
#include "onoff_common.h"

#include "code2650.h"

#define ADDR_INT UInt15

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

/*!------------------------------------------------------------------------
 * \fn     DecodeCondition(const tStrComp *p_arg, Byte *p_ret)
 * \brief  decode condition code
 * \param  p_arg source argument
 * \param  p_ret binary encoded argument
 * \return true if success
 * ------------------------------------------------------------------------ */

static Boolean DecodeCondition(const tStrComp *p_arg, Byte *p_ret)
{
  const char *p_asc = p_arg->str.p_str;

  if (!as_strcasecmp(p_asc, "EQ") || !as_strcasecmp(p_asc, "Z"))
  {
    *p_ret = 0;
    return True;
  }
  else if (!as_strcasecmp(p_asc, "GT") || !as_strcasecmp(p_asc, "P"))
  {
    *p_ret = 1;
    return True;
  }
  else if (!as_strcasecmp(p_asc, "LT") || !as_strcasecmp(p_asc, "N"))
  {
    *p_ret = 2;
    return True;
  }
  else if ((!as_strcasecmp(p_asc, "ALWAYS")) || (!as_strcasecmp(p_asc, "UN")))
  {
    *p_ret = 3;
    return True;
  }
  else
  {
    Boolean ok;

    *p_ret = EvalStrIntExpression(p_arg, UInt2, &ok);
    return ok;
  }
}

/*!------------------------------------------------------------------------
 * \fn     page_rel_ok(Word dest, Word src, tSymbolFlags flags, Word *p_dist, Boolean is_branch)
 * \brief  check whether relative addressing in same 8K page is possible
 * \param  dest target address
 * \param  src current (source) address
 * \param  flags expression evaluation flags
 * \param  p_dist resulting distance
 * \param  is_branch code or data access?
 * \return True if distance is in range
 * ------------------------------------------------------------------------ */

#define PAGE_MASK 0x1fff

static Boolean page_rel_ok(Word dest, Word src, tSymbolFlags flags, Word *p_dist, Boolean is_branch)
{
  if (((src & ~PAGE_MASK) != (dest & ~PAGE_MASK)) && !mSymbolQuestionable(flags))
  {
    WrError(is_branch ? ErrNum_JmpTargOnDiffPage : ErrNum_TargOnDiffPage);
    return False;
  }

  *p_dist = (dest - src) & PAGE_MASK;
  if (((*p_dist < 0x1fc0) && (*p_dist > 0x3f)) && !mSymbolQuestionable(flags))
  {
    WrError(is_branch ? ErrNum_JmpDistTooBig : ErrNum_DistTooBig);
    return False;
  }

  return True;
}

/*!------------------------------------------------------------------------
 * \fn     Boolean page_abs_ok(Word dest, Word src, tSymbolFlags flags, Word *p_dest, Boolean is_branch)
 * \brief  check whether absolute address is in same 8K page
 * \param  dest target address
 * \param  src current (source) address
 * \param  flags expression evaluation flags
 * \param  p_dest resulting address in machine instruction
 * \param  is_branch code or data access?
 * \return True if distance is in range
 * ------------------------------------------------------------------------ */

static Boolean page_abs_ok(Word dest, Word src, tSymbolFlags flags, Word *p_dest, Boolean is_branch)
{
  if (((src & ~PAGE_MASK) != (dest & ~PAGE_MASK)) && !mSymbolQuestionable(flags))
  {
    WrError(is_branch ? ErrNum_JmpTargOnDiffPage : ErrNum_TargOnDiffPage);
    return False;
  }

  *p_dest = dest & PAGE_MASK;
  return True;
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

/*!------------------------------------------------------------------------
 * \fn     DecodeOneReg(Word Index)
 * \brief  decode instructions taking a single register as argument
 * \param  Index machine code of instruction when register 0 is used
 * ------------------------------------------------------------------------ */

static void DecodeOneReg(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Reg;
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeOneReg_NoZero(Word Index)
 * \brief  decode instructions taking a single register except R0 as argument
 * \param  Index machine code of instruction register # is added to
 * ------------------------------------------------------------------------ */


static void DecodeOneReg_NoZero(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (!Reg) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    BAsmCode[0] = Index | Reg;
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeLODZ(Word Index)
 * \brief  decode LODZ instruction
 * \param  Index machine code of instruction when register 0 is used
 * ------------------------------------------------------------------------ */

static void DecodeLODZ(Word Index)
{
  Byte Reg;

  if (!ChkArgCnt(1, 1));
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    /* LODZ R0 shall be encoded as IORZ R0 */
    BAsmCode[0] = Reg ? (Index | Reg) : 0x60;
    CodeLen = 1;
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

/*!------------------------------------------------------------------------
 * \fn     void DecodeRegAbs(Word code)
 * \brief  handle instruction with register & absolute address
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void DecodeRegAbs(Word code)
{
  Byte dest_reg;

  if (!ChkArgCnt(2, 4));
  else if (!DecodeReg(ArgStr[1].str.p_str, &dest_reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Boolean ok, ind_flag = *ArgStr[2].str.p_str == '*';
    Word abs_val;
    tSymbolFlags flags;
    Byte index_reg;

    abs_val = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], ind_flag, ADDR_INT, &ok, &flags);
    if (ok && page_abs_ok(abs_val, EProgCounter() + 3, flags, &abs_val, False))
    {
      BAsmCode[0] = code;
      BAsmCode[1] = Hi(abs_val);
      BAsmCode[2] = Lo(abs_val);
      if (ind_flag)
        BAsmCode[1] |= 0x80;
      if (ArgCnt == 2)
      {
        BAsmCode[0] |= dest_reg;
        CodeLen = 3;
      }
      else
      {
        if (!DecodeReg(ArgStr[3].str.p_str, &index_reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[3]);
        else if (dest_reg != 0) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] |= index_reg;
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

/*!------------------------------------------------------------------------
 * \fn     DecodeRegRel(Word code)
 * \brief  handle instructions with register & relative argument
 * \param  code instruction machine code
 * ------------------------------------------------------------------------ */

static void DecodeRegRel(Word code)
{
  Byte reg;

  if (!ChkArgCnt(2, 2));
  else if (!DecodeReg(ArgStr[1].str.p_str, &reg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    Word dest, dist;
    tSymbolFlags flags;
    Boolean ind_flag, ok;

    BAsmCode[0] = code | reg;
    ind_flag = *ArgStr[2].str.p_str == '*';
    dest = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], ind_flag, ADDR_INT, &ok, &flags);
    if (ok && page_rel_ok(dest, EProgCounter() + 2, flags, &dist, False))
    {
      BAsmCode[1] = dist & 0x7f;
      if (ind_flag)
        BAsmCode[1] |= 0x80;
      CodeLen = 2;
    }
  }
}

static void DecodeCondAbs(Word Index)
{
  Byte Cond;
  Word Address;
  Boolean OK, IndFlag;

  if (ChkArgCnt(2, 2)
   && DecodeCondition(&ArgStr[1], &Cond))
  {
    IndFlag = *ArgStr[2].str.p_str == '*';
    Address = EvalStrIntExpressionOffs(&ArgStr[2], IndFlag, ADDR_INT, &OK);
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

/*!------------------------------------------------------------------------
 * \fn     DecodeCondRel(Word code)
 * \brief  decode relative branches
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeCondRel(Word code)
{
  Byte cond;

  if (ChkArgCnt(2, 2)
   && DecodeCondition(&ArgStr[1], &cond))
  {
    Boolean ind_flag, ok;
    tSymbolFlags flags;
    Word dist, dest;

    BAsmCode[0] = code | cond;
    ind_flag = *ArgStr[2].str.p_str == '*';
    dest = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], ind_flag, ADDR_INT, &ok, &flags);
    if (ok && page_rel_ok(dest, EProgCounter() + 2, flags, &dist, True))
    {
      BAsmCode[1] = dist & 0x7f;
      if (ind_flag)
        BAsmCode[1] |= 0x80;
      CodeLen = 2;
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
    AbsVal = EvalStrIntExpressionOffs(&ArgStr[2], IndFlag, ADDR_INT, &OK);
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
    AbsVal = EvalStrIntExpressionOffs(&ArgStr[1], IndFlag, ADDR_INT, &OK);
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

  if (ChkArgCnt(1, 1)
   && DecodeCondition(&ArgStr[1], &Cond))
  {
    BAsmCode[0] = Index | Cond;
    CodeLen = 1;
  }
}

/*!------------------------------------------------------------------------
 * \fn     DecodeZero(Word code)
 * \brief  decode zero page branch instructions
 * \param  code machine code
 * ------------------------------------------------------------------------ */

static void DecodeZero(Word code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean ind_flag, ok;
    tSymbolFlags flags;
    Word dest, dist;

    BAsmCode[0] = code;
    ind_flag = *ArgStr[1].str.p_str == '*';
    dest = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], ind_flag, ADDR_INT, &ok, &flags);
    if (ok && page_rel_ok(dest, 0x0000, flags, &dist, True))
    {
      BAsmCode[1] = dist & 0x7f;
      if (ind_flag)
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
  /* ANDZ R0 is not allowed and decodes as HALT */
  AddInstTable(InstTable, "ANDZ", 0x40, DecodeOneReg_NoZero);
  AddOneReg("COMZ", 0xe0);
  AddOneReg("DAR", 0x94);
  AddOneReg("EORZ", 0x20);
  AddOneReg("IORZ", 0x60);
  AddInstTable(InstTable, "LODZ", 0x00, DecodeLODZ);
  AddOneReg("REDC", 0x30);
  AddOneReg("REDD", 0x70);
  AddOneReg("RRL", 0xd0);
  AddOneReg("RRR", 0x50);
  /* STRZ R0 is not allowed and decodes as NOP */
  AddInstTable(InstTable, "STRZ", 0xc0, DecodeOneReg_NoZero);
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

  AddInstTable(InstTable, "RES", 1, DecodeIntelDS);
  AddInstTable(InstTable, "ACON", eIntPseudoFlag_BigEndian | eIntPseudoFlag_AllowInt | eIntPseudoFlag_AllowString, DecodeIntelDW);
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

  if (DecodeIntelPseudo(TargetBigEndian)) return;

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
  const TFamilyDescr *pDescr;

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  pDescr = FindFamilyByName("2650");
  PCSymbol = "$"; HeaderID = pDescr->Id; NOPCode = 0xc0;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = IntTypeDefs[ADDR_INT].Max;

  MakeCode = MakeCode_2650; IsDef = IsDef_2650;
  SwitchFrom = SwitchFrom_2650; InitFields();

  onoff_bigendian_add();
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

void code2650_init(void)
{
  CPU2650 = AddCPU("2650", SwitchTo_2650);
}
