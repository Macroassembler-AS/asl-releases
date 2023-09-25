/* codekcpsm.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Xilinx kcpsm                                                */
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

#include "codekcpsm.h"

#undef DEBUG_PRINTF

typedef struct
{
  const char *Name;
  Word Code;
} Condition;


#define WorkOfs 0xe0

#define COND_CODE_TRUE 0

#define IOopCnt 2

#define ModNone  (-1)
#define ModWReg   0
#define MModWReg   (1 << ModWReg)
#define ModAbs    1
#define MModAbs    (1 << ModAbs)
#define ModImm    4
#define MModImm    (1 << ModImm)
#define ModIRReg  6
#define MModIRReg  (1 << ModIRReg)
#define ModInd    7
#define MModInd    (1 << ModInd)

static ShortInt AdrType;
static Word AdrMode,AdrIndex;

static Condition *Conditions;

static CPUVar CPUKCPSM;

/*--------------------------------------------------------------------------*/
/* Code Helpers */

/*!------------------------------------------------------------------------
 * \fn     IsWRegCore(const char *pArg, Word *pResult)
 * \brief  check whether argument is CPU register
 * \param  pArg argument
 * \param  pResult register number if it is
 * \return True if it is
 * ------------------------------------------------------------------------ */

static Boolean IsWRegCore(const char *pArg, Word *pResult)
{
  Boolean retValue;

  if ((strlen(pArg) < 2) || (as_toupper(*pArg) != 'S'))
    retValue = False;
  else
  {
    Boolean OK;

    *pResult = ConstLongInt(pArg + 1, &OK, 10);
    if (!OK)
      retValue = False;
    else
      retValue = (*pResult <= 15);
  }
#ifdef DEBUG_PRINTF
  fprintf( stderr, "IsWRegCore: %s %d\n", Asc, retValue );
#endif
  return retValue;
}

/*!------------------------------------------------------------------------
 * \fn     DissectReg_KCPSM(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
 * \brief  dissect register symbols - KCPSM3 variant
 * \param  pDest destination buffer
 * \param  DestSize destination buffer size
 * \param  Value numeric register value
 * \param  InpSize register size
 * ------------------------------------------------------------------------ */

static void DissectReg_KCPSM(char *pDest, size_t DestSize, tRegInt Value, tSymbolSize InpSize)
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
 * \fn     IsWReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
 * \brief  check whether argument is CPU register, including register aliases
 * \param  pArg argument
 * \param  pResult register number if it is
 * \param  MustBeReg expecting register as arg?
 * \return reg eval result
 * ------------------------------------------------------------------------ */

static tRegEvalResult IsWReg(const tStrComp *pArg, Word *pResult, Boolean MustBeReg)
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

static void DecodeAdr(const tStrComp *pArg, Byte Mask, int Segment)
{
  tEvalResult EvalResult;
  char *p;
  int ArgLen;

  AdrType = ModNone;

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    AdrMode = EvalStrIntExpressionOffsWithResult(pArg, 1, UInt8, &EvalResult);
    if (EvalResult.OK)
      AdrType = ModImm;
    goto chk;
  }

  /* Register ? */

  switch (IsWReg(pArg, &AdrMode, False))
  {
    case eIsReg:
      AdrType = ModWReg;
      goto chk;
    case eIsNoReg:
      break;
    case eRegAbort:
      return;
  }

  /* indiziert ? */

  ArgLen = strlen(pArg->str.p_str);
  if ((ArgLen >= 4) && (pArg->str.p_str[ArgLen - 1] == ')'))
  {
    p = pArg->str.p_str + ArgLen - 1;
    while ((p >= pArg->str.p_str) && (*p != '('))
      p--;
    if (*p != '(') WrError(ErrNum_BrackErr);
    else
    {
      tStrComp RegComp, DispComp;

      StrCompSplitRef(&DispComp, &RegComp, pArg, p);
      StrCompShorten(&RegComp, 1);
      if (IsWReg(&RegComp, &AdrMode, True) == eIsReg)
      {
        AdrIndex = EvalStrIntExpressionWithResult(&DispComp, UInt8, &EvalResult);
        if (EvalResult.OK)
        {
          AdrType = ModInd;
          ChkSpace(SegData, EvalResult.AddrSpaceMask);
        }
        goto chk;
      }
    }
  }

  /* einfache direkte Adresse ? */

  AdrMode = EvalStrIntExpressionWithResult(pArg, UInt8, &EvalResult);
  if (EvalResult.OK)
  {
    AdrType = ModAbs;
    if (Segment != SegNone)
      ChkSpace(Segment, EvalResult.AddrSpaceMask);
    goto chk;
  }

chk:
  if ((AdrType != ModNone) && ((Mask & (1 << AdrType)) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrType = ModNone;
  }
}

static Boolean DecodeCond(char *Asc, Word *p_code)
{
  int Cond;

   for (Cond = 0; Conditions[Cond].Name; Cond++)
     if (!strcmp(Conditions[Cond].Name, Asc))
     {
       *p_code = Conditions[Cond].Code;
       return True;
     }
  return False;
}

/*--------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    WAsmCode[0] = Code;
  }
}

static void DecodeLOAD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModWReg, SegNone);
    switch (AdrType)
    {
      case ModWReg:
      {
        Word Save = AdrMode;
        DecodeAdr(&ArgStr[2], MModWReg | MModAbs | MModImm, SegNone);
        switch (AdrType)
        {
          case ModWReg:
#ifdef DEBUG_PRINTF
            fprintf( stderr, "LOAD-->ModWReg %d %d\n", AdrMode, Save );
#endif
            WAsmCode[0] = 0xc000 | (Save << 8) | ( AdrMode << 4 );
            CodeLen = 1;
            break;
          case ModAbs:
#ifdef DEBUG_PRINTF
            fprintf( stderr, "LOAD-->ModAbs %d %d\n", AdrMode, Save );
#endif
            WAsmCode[0] = 0xc000 | (Save << 8) | ( AdrMode << 4 );
            CodeLen = 1;
            break;
          case ModImm:
#ifdef DEBUG_PRINTF
            fprintf( stderr, "LOAD-->ModImm %d %d\n", AdrMode, Save );
#endif
            WAsmCode[0] = (Save << 8) | AdrMode;
            CodeLen = 1;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeALU2(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModWReg, SegNone);
    switch (AdrType)
    {
      case ModWReg:
      {
        Word Save = AdrMode;
        DecodeAdr(&ArgStr[2], MModAbs | MModWReg | MModImm, SegNone);
        switch (AdrType)
        {
          case ModWReg:
            WAsmCode[0] = 0xc000 | (Save << 8) | ( AdrMode << 4 ) | Code;
            CodeLen = 1;
            break;
          case ModImm:
          case ModAbs:
            WAsmCode[0] = (Code << 12 ) | (Save << 8) | AdrMode;
            CodeLen = 1;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeALU1(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModWReg, SegNone);
    switch (AdrType)
    {
      case ModWReg:
        WAsmCode[0] = 0xd000 | (AdrMode << 8) | Code;
        CodeLen = 1;
        break;
    }
  }
}

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Word cond_code;

    if (ArgCnt == 1)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCond(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    DecodeAdr(&ArgStr[ArgCnt], MModAbs | ModImm, SegCode);
    switch (AdrType)
    {
      case ModAbs:
      case ModImm:
        WAsmCode[0] = 0x8300 | (cond_code << 10) | Lo(AdrMode);
        CodeLen = 1;
        break;
    }
  }
}

static void DecodeJUMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Word cond_code;

    if (ArgCnt == 1)
       cond_code= COND_CODE_TRUE;
    else if (!DecodeCond(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return;
    }

    DecodeAdr(&ArgStr[ArgCnt], MModAbs | MModImm, SegCode);
    switch (AdrType)
    {
      case ModAbs:
      case ModImm:
        WAsmCode[0] = 0x8100 | (cond_code << 10) | Lo(AdrMode);
        CodeLen = 1;
        break;
    }
  }
}

static void DecodeRETURN(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 1))
  {
    Word cond_code;

    if (ArgCnt == 0)
      cond_code = COND_CODE_TRUE;
    else if (!DecodeCond(ArgStr[1].str.p_str, &cond_code))
    {
      WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
      return ;
    }

    WAsmCode[0] = 0x8080 | (cond_code << 10);
    CodeLen = 1;
  }
}

static void DecodeIOop(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModWReg, SegNone);
    switch (AdrType)
    {
      case ModWReg:
      {
        Word Save = AdrMode;
        DecodeAdr(&ArgStr[2], MModInd | MModImm | MModAbs, SegData);
        switch (AdrType)
        {
          case ModInd:
            WAsmCode[0] = 0x1000 | ((Code | Save) << 8) | ( AdrMode << 4);
            CodeLen = 1;
            break;
          case ModImm:
          case ModAbs:
            WAsmCode[0] = ((Code | Save) << 8) | AdrMode;
            CodeLen = 1;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeRETURNI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    NLS_UpString(ArgStr[1].str.p_str);
    if (!strcmp(ArgStr[1].str.p_str, "ENABLE"))
    {
      WAsmCode[0] = 0x80f0;
      CodeLen = 1;
    }
    else if (!strcmp(ArgStr[1].str.p_str, "DISABLE"))
    {
      WAsmCode[0] =  0x80d0;
      CodeLen = 1;
    }
  }
}

static void DecodeENABLE_DISABLE(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    NLS_UpString(ArgStr[1].str.p_str);
    if (!as_strcasecmp(ArgStr[1].str.p_str, "INTERRUPT"))
    {
      WAsmCode[0] = Code;
      CodeLen = 1;
    }
  }
}

static void DecodeCONSTANT(Word Code)
{
  UNUSED(Code);

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

/*--------------------------------------------------------------------------*/
/* code table handling */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddALU2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU2);
}

static void AddALU1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU1);
}

static void AddIOop(const Char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeIOop);
}

static void AddCondition(const char *NName, Word NCode)
{
  order_array_rsv_end(Conditions, Condition);
  Conditions[InstrZ].Name = NName;
  Conditions[InstrZ++].Code = NCode;
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  AddInstTable(InstTable, "LOAD", 0, DecodeLOAD);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "JUMP", 0, DecodeJUMP);
  AddInstTable(InstTable, "RETURN", 0, DecodeRETURN);
  AddInstTable(InstTable, "RETURNI", 0, DecodeRETURNI);
  AddInstTable(InstTable, "ENABLE", 0x8030, DecodeENABLE_DISABLE);
  AddInstTable(InstTable, "DISABLE", 0x8010, DecodeENABLE_DISABLE);
  AddInstTable(InstTable, "REG", 0, CodeREG);
  AddInstTable(InstTable, "NAMEREG", 0, CodeNAMEREG);
  AddInstTable(InstTable, "CONSTANT", 0, DecodeCONSTANT);

  AddFixed("EI"     , 0x8030);  AddFixed("DI"     , 0x8010);
  AddFixed("RETIE"  , 0x80f0);  AddFixed("RETID"  , 0x80d0);
  AddFixed("NOP"    , 0xc000); /* fake */

  AddALU2("ADD"   , 0x04);
  AddALU2("ADDCY" , 0x05);
  AddALU2("SUB"   , 0x06);
  AddALU2("SUBCY" , 0x07);
  AddALU2("OR"    , 0x02);
  AddALU2("AND"   , 0x01);
  AddALU2("XOR"   , 0x03);

  AddALU1("SR0" , 0x0e);
  AddALU1("SR1" , 0x0f);
  AddALU1("SRX" , 0x0a);
  AddALU1("SRA" , 0x08);
  AddALU1("RR"  , 0x0c);
  AddALU1("SL0" , 0x06);
  AddALU1("SL1" , 0x07);
  AddALU1("SLX" , 0x04);
  AddALU1("SLA" , 0x00);
  AddALU1("RL"  , 0x02);

  AddIOop("INPUT"  , 0xa0);
  AddIOop("OUTPUT" , 0xe0);

  InstrZ = 0;
  AddCondition("T"  , COND_CODE_TRUE);
  AddCondition("C"  , 6); AddCondition("NC" , 7);
  AddCondition("Z"  , 4); AddCondition("NZ" , 5);
  AddCondition(NULL , 0);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(Conditions);
}

/*---------------------------------------------------------------------*/

/*!------------------------------------------------------------------------
 * \fn     InternSymbol_KCPSM(char *pArg, TempResult *pResult)
 * \brief  handle built-in (register) symbols for KCPSM
 * \param  pArg source argument
 * \param  pResult result buffer
 * ------------------------------------------------------------------------ */

static void InternSymbol_KCPSM(char *pArg, TempResult *pResult)
{
  Word RegNum;

  if (IsWRegCore(pArg, &RegNum))
  {
    pResult->Typ = TempReg;
    pResult->DataSize = eSymbolSize8Bit;
    pResult->Contents.RegDescr.Reg = RegNum;
    pResult->Contents.RegDescr.Dissect = DissectReg_KCPSM;
    pResult->Contents.RegDescr.compare = NULL;
  }
}

static void MakeCode_KCPSM(void)
{
  CodeLen = 0; DontPrint = False;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_KCPSM(void)
{
  return (Memo("REG"));
}

static void SwitchFrom_KCPSM(void)
{
  DeinitFields();
}

static void SwitchTo_KCPSM(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("KCPSM");

  TurnWords = True;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = FoundDescr->Id;
  NOPCode = 0xc0; /* nop = load s0,s0 */
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0; SegLimits[SegCode] = 0xff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0; SegLimits[SegData] = 0xff;

  MakeCode = MakeCode_KCPSM;
  IsDef = IsDef_KCPSM;
  InternSymbol = InternSymbol_KCPSM;
  DissectReg = DissectReg_KCPSM;
  SwitchFrom = SwitchFrom_KCPSM;
  InitFields();
}

void codekcpsm_init(void)
{
  CPUKCPSM = AddCPU("KCPSM", SwitchTo_KCPSM);

  AddCopyright("XILINX KCPSM(Picoblaze)-Generator (C) 2003 Andreas Wassatsch");
}
