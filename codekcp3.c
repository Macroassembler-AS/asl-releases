/* codekcp3.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Xilinx kcpsm3                                               */
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
#include "asmallg.h"
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"
#include "codepseudo.h"

#include "codekcp3.h"

typedef struct
{
  LongWord Code;
} FixedOrder;

static FixedOrder *RegOrders, *ALUOrders;

static CPUVar CPUKCPSM3;

/*--------------------------------------------------------------------------
 * Address Expression Parsing
 *--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     IsWRegCore(const char *pArg, LongWord *pResult)
 * \brief  is argument a built-in register?
 * \param  pArg argument
 * \param  pResult register number if yes
 * \return true if yes
 * ------------------------------------------------------------------------ */

static Boolean IsWRegCore(const char *pArg, LongWord *pResult)
{
  Boolean OK;

  if ((strlen(pArg) < 2) || (as_toupper(*pArg) != 'S'))
    return False;

  *pResult = ConstLongInt(pArg + 1, &OK, 16);
  if (!OK)
    return False;

  return (*pResult < 16);
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_KCPSM3(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - KCPSM3 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_KCPSM3(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
{
  switch (InpSize)
  {
    case eSymbolSize8Bit:
      as_snprintf(pDest, DestSize, "S%x", (unsigned)Value);
      pDest[1] = as_toupper(pDest[1]);
      break;
    default:
      as_snprintf(pDest, DestSize, "%d-%u", (int)InpSize, (unsigned)Value);
  }
}

/*!------------------------------------------------------------------------
 * \fn     IsWReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
 * \brief  is argument a built-in register or register alias?
 * \param  pArg argument
 * \param  pResult register number if yes
 * \param  MustBeReg expect register arg?
 * \return reg evel result
 * ------------------------------------------------------------------------ */

static tRegEvalResult IsWReg(const tStrComp *pArg, LongWord *pResult, Boolean MustBeReg)
{
  tRegDescr RegDescr;
  tEvalResult EvalResult;
  tRegEvalResult RegEvalResult;

  if (IsWRegCore(pArg->str.p_str, pResult))
    return eIsReg;

  RegEvalResult = EvalStrRegExpressionAsOperand(pArg, &RegDescr, &EvalResult, eSymbolSize8Bit, MustBeReg);
  *pResult = RegDescr.Reg;
  return RegEvalResult;
}

/*!------------------------------------------------------------------------
 * \fn     IsIWReg(const tStrComp *pArg, LongWord *pResult)
 * \brief  is argument an indirect register expression?
 * \param  pArg argument
 * \param  pResult register number if yes
 * \return true if yes
 * ------------------------------------------------------------------------ */

static tRegEvalResult IsIWReg(const tStrComp *pArg, LongWord *pResult)
{
  char Tmp[10];
  tStrComp TmpComp;
  int l = strlen(pArg->str.p_str);

  if ((l < 3) || (pArg->str.p_str[0] != '(') || (pArg->str.p_str[l - 1] != ')'))
    return eIsNoReg;

  StrCompMkTemp(&TmpComp, Tmp, sizeof(Tmp));
  StrCompCopySub(&TmpComp, pArg, 1, l - 2);
  return IsWReg(&TmpComp, pResult, False);
}

static Boolean IsCond(int OtherArgCnt, LongWord *pErg)
{
  static const char Conds[4][3] = { "Z", "NZ", "C", "NC" };

  if (ArgCnt <= OtherArgCnt)
  {
    *pErg = 0;
    return True;
  }

  for (*pErg = 0; *pErg < (sizeof(Conds) / sizeof(*Conds)); (*pErg)++)
    if (!as_strcasecmp(Conds[*pErg], ArgStr[1].str.p_str))
    {
      *pErg |= 4;
      return True;
    }

  return False;
}

/*--------------------------------------------------------------------------
 * Code Handlers
 *--------------------------------------------------------------------------*/

static void DecodePort(Word Index)
{
  UNUSED(Index);

  CodeEquate(SegIO, 0, SegLimits[SegIO]);
}

static void DecodeConstant(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    TempResult t;
    Boolean OK;

    as_tempres_ini(&t);
    as_tempres_set_int(&t, EvalStrIntExpressionWithFlags(&ArgStr[2], Int32, &OK, &t.Flags));
    if (OK && !mFirstPassUnknown(t.Flags))
    {
      SetListLineVal(&t);
      PushLocHandle(-1);
      EnterIntSymbol(&ArgStr[1], t.Contents.Int, SegNone, False);
      PopLocHandle();
    }
    as_tempres_free(&t);
  }
}

static void DecodeOneReg(Word Index)
{
  FixedOrder *pOrder = RegOrders + Index;
  LongWord Reg;

  if (ChkArgCnt(1, 1)
   && IsWReg(&ArgStr[1], &Reg, True))
  {
    DAsmCode[0] = pOrder->Code | (Reg << 8);
    CodeLen = 1;
  }
}

static void DecodeALU(Word Index)
{
  FixedOrder *pOrder = ALUOrders + Index;
  LongWord Src, DReg;
  Boolean OK;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &DReg, True))
    switch (IsWReg(&ArgStr[2], &Src, False))
    {
      case eIsReg:
        DAsmCode[0] = pOrder->Code | 0x1000 | (DReg << 8) | (Src << 4);
        CodeLen = 1;
        break;
      case eIsNoReg:
        Src = EvalStrIntExpression(&ArgStr[2], Int8, &OK);
        if (OK)
        {
          DAsmCode[0] = pOrder->Code | (DReg << 8) | (Src & 0xff);
          CodeLen = 1;
        }
        break;
      case eRegAbort:
        break;
    }
}

static void DecodeJmp(Word Index)
{
  LongWord Cond;

  if (ChkArgCnt(1, 2)
   && IsCond(1, &Cond))
  {
    tEvalResult EvalResult;
    LongWord Addr = EvalStrIntExpressionWithResult(&ArgStr[ArgCnt], UInt10, &EvalResult);

    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      DAsmCode[0] = 0x30000 | Index | (Cond << 10) | (Addr & 0x3ff);
      CodeLen = 1;
    }
  }
}

static void DecodeRet(Word Index)
{
  LongWord Cond;

  UNUSED(Index);

  if (ChkArgCnt(0, 1)
   && IsCond(0, &Cond))
  {
    DAsmCode[0] = 0x2a000 | (Cond << 10);
    CodeLen = 1;
  }
}

static void DecodeReti(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "DISABLE"))
    {
      DAsmCode[0] = 0x38000;
      CodeLen = 1;
    }
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "ENABLE"))
    {
      DAsmCode[0] = 0x38001;
      CodeLen = 1;
    }
    else
      WrError(ErrNum_InvAddrMode);
  }
}

static void DecodeInt(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    if (as_strcasecmp(ArgStr[1].str.p_str, "INTERRUPT")) WrError(ErrNum_InvAddrMode);
    else
    {
      DAsmCode[0] = 0x3c000 | Index;
      CodeLen = 1;
    }
  }
}

static void DecodeMem(Word Index)
{
  LongWord Reg, Addr;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &Reg, True))
  {
    DAsmCode[0] = (((LongWord)Index) << 13) | (Reg << 8);
    switch (IsIWReg(&ArgStr[2], &Addr))
    {
      case eIsReg:
        DAsmCode[0] |= 0x01000 | (Addr << 4);
        CodeLen = 1;
        break;
      case eIsNoReg:
      {
        tEvalResult EvalResult;

        Addr = EvalStrIntExpressionWithResult(&ArgStr[2], UInt6, &EvalResult);
        if (EvalResult.OK)
        {
          ChkSpace(SegData, EvalResult.AddrSpaceMask);
          DAsmCode[0] |= Addr & 0x3f;
          CodeLen = 1;
        }
        break;
      }
      case eRegAbort:
        break;
    }
  }
}

static void DecodeIO(Word Index)
{
  LongWord Reg, Addr;

  if (ChkArgCnt(2, 2)
   && IsWReg(&ArgStr[1], &Reg, True))
  {
    DAsmCode[0] = (((LongWord)Index) << 13) | (Reg << 8);
    switch (IsIWReg(&ArgStr[2], &Addr))
    {
      case eIsReg:
        DAsmCode[0] |= 0x01000 | (Addr << 4);
        CodeLen = 1;
        break;
      case eIsNoReg:
      {
        tEvalResult EvalResult;

        Addr = EvalStrIntExpressionWithResult(&ArgStr[2], UInt8, &EvalResult);
        if (EvalResult.OK)
        {
          ChkSpace(SegIO, EvalResult.AddrSpaceMask);
          DAsmCode[0] |= Addr & 0xff;
          CodeLen = 1;
        }
        break;
      }
      case eRegAbort:
        break;
    }
  }
}

static void DecodeNop(Word Index)
{
  UNUSED (Index);

  if (ChkArgCnt(0, 0))
  {
    DAsmCode[0] = NOPCode;
    CodeLen = 1;
  }
}

/*--------------------------------------------------------------------------
 * Instruction Table Handling
 *--------------------------------------------------------------------------*/

static void AddReg(const char *NName, LongWord NCode)
{
  order_array_rsv_end(RegOrders, FixedOrder);
  RegOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeOneReg);
}

static void AddALU(const char *NName, LongWord NCode)
{
  order_array_rsv_end(ALUOrders, FixedOrder);
  ALUOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(97);

  InstrZ = 0;
  AddReg("RL" , 0x20002);
  AddReg("RR" , 0x2000c);
  AddReg("SL0", 0x20006);
  AddReg("SL1", 0x20007);
  AddReg("SLA", 0x20000);
  AddReg("SLX", 0x20004);
  AddReg("SR0", 0x2000e);
  AddReg("SR1", 0x2000f);
  AddReg("SRA", 0x20008);
  AddReg("SRX", 0x2000a);

  InstrZ = 0;
  AddALU("ADD"    , 0x18000);
  AddALU("ADDCY"  , 0x1a000);
  AddALU("AND"    , 0x0a000);
  AddALU("COMPARE", 0x14000);
  AddALU("LOAD"   , 0x00000);
  AddALU("OR"     , 0x0c000);
  AddALU("SUB"    , 0x1c000);
  AddALU("SUBCY"  , 0x1e000);
  AddALU("TEST"   , 0x12000);
  AddALU("XOR"    , 0x0e000);

  AddInstTable(InstTable, "CALL", 0x0000, DecodeJmp);
  AddInstTable(InstTable, "JUMP", 0x4000, DecodeJmp);
  AddInstTable(InstTable, "RETURN", 0, DecodeRet);
  AddInstTable(InstTable, "RETURNI", 0, DecodeReti);
  AddInstTable(InstTable, "ENABLE", 1, DecodeInt);
  AddInstTable(InstTable, "DISABLE", 0, DecodeInt);
  AddInstTable(InstTable, "FETCH", 0x03, DecodeMem);
  AddInstTable(InstTable, "STORE", 0x17, DecodeMem);
  AddInstTable(InstTable, "INPUT", 0x02, DecodeIO);
  AddInstTable(InstTable, "OUTPUT", 0x16, DecodeIO);

  AddInstTable(InstTable, "PORT", 0, DecodePort);
  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "NAMEREG", 0, CodeNAMEREG);
  AddInstTable(InstTable, "CONSTANT", 0, DecodeConstant);

  AddInstTable(InstTable, "NOP", 0, DecodeNop);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(RegOrders);
  order_array_free(ALUOrders);
}

/*--------------------------------------------------------------------------
 * Semipublic Functions
 *--------------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_KCPSM3(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for KCPSM3
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_KCPSM3(char *pArg, TempResult *pResult)
{
  LongWord RegNum;

  if (IsWRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_KCPSM3;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static Boolean IsDef_KCPSM3(void)
{
   return (Memo("REG")) || (Memo("PORT"));
}

static void SwitchFrom_KCPSM3(void)
{
   DeinitFields();
}

static void MakeCode_KCPSM3(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

   if (Memo("")) return;

   /* Pseudoanweisungen */

   if (DecodeIntelPseudo(True)) return;

   if (!LookupInstTable(InstTable, OpPart.str.p_str))
     WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void SwitchTo_KCPSM3(void)
{
   const TFamilyDescr *FoundDescr;

   FoundDescr = FindFamilyByName("KCPSM3");

   TurnWords = True;
   SetIntConstMode(eIntConstModeIntel);

   PCSymbol = "$"; HeaderID = FoundDescr->Id;

   /* NOP = load s0,s0 */

   NOPCode = 0x01000;
   DivideChars = ","; HasAttrs = False;

   ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
   Grans[SegCode] = 4; ListGrans[SegCode] = 4; SegInits[SegCode] = 0;
   SegLimits[SegCode] = 0x3ff;
   Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
   SegLimits[SegData] = 0x3f;
   Grans[SegIO] = 1; ListGrans[SegIO] = 1; SegInits[SegIO] = 0;
   SegLimits[SegIO] = 0xff;

   MakeCode = MakeCode_KCPSM3;
   IsDef = IsDef_KCPSM3;
   InternSymbol = InternSymbol_KCPSM3;
   DissectReg = DissectReg_KCPSM3;
   SwitchFrom = SwitchFrom_KCPSM3; InitFields();
}

/*--------------------------------------------------------------------------
 * Initialization
 *--------------------------------------------------------------------------*/

void codekcpsm3_init(void)
{
   CPUKCPSM3 = AddCPU("KCPSM3", SwitchTo_KCPSM3);
}
