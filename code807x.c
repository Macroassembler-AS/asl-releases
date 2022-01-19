/* code807c.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator National INS807X.c                                          */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <ctype.h>
#include <string.h>
#include "bpemu.h"

#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "errmsg.h"
#include "asmpars.h"
#include "asmitree.h"
#include "headids.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code807x.h"

/*---------------------------------------------------------------------------*/

#define ShiftOrderCnt 5
#define BranchOrderCnt 5

#define ModNone (-1)
#define ModReg 0
#define MModReg (1 << ModReg)
#define ModImm 1
#define MModImm (1 << ModImm)
#define ModMem 2
#define MModMem (1 << ModMem)
#define ModAcc 3
#define MModAcc (1 << ModAcc)
#define ModE 4
#define MModE (1 << ModE)
#define ModS 5
#define MModS (1 << ModS)
#define ModT 6
#define MModT (1 << ModT)

typedef struct
{
  Byte Code, Code16;
} ShiftOrder;

static ShiftOrder *ShiftOrders;

static int OpSize, AdrMode;
static Byte AdrVals[2], AdrPart;

static CPUVar CPU8070;

/*---------------------------------------------------------------------------*/

static Boolean SetOpSize(int NewSize)
{
  Boolean Result = True;

  if (OpSize == -1)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    Result = False;
  }

  return Result;
}

static Boolean GetReg16(char *Asc, Byte *AdrPart)
{
  int z;
  static const char Reg16Names[4][3] = { "PC", "SP", "P2", "P3" };

  for (z = 0; z < 4; z++)
    if (!as_strcasecmp(Asc, Reg16Names[z]))
    {
      *AdrPart = z;
      return True;
    }
  return False;
}

static void DecodeAdr(int Index, Byte Mask, LongInt PCDelta)
{
  int Cnt;
  Word TmpVal;
  LongInt Addr;
  Boolean OK;

  AdrMode = ModNone;

  if (!ChkArgCnt(Index, ArgCntMax))
    return;

  Cnt = ArgCnt - Index + 1;

  /* accumulator ? */

  if (!as_strcasecmp(ArgStr[Index].str.p_str, "A"))
  {
    if (SetOpSize(0))
      AdrMode = ModAcc;
    goto AdrFound;
  }

  if (!as_strcasecmp(ArgStr[Index].str.p_str, "EA"))
  {
    if (SetOpSize(1))
      AdrMode = ModAcc;
    goto AdrFound;
  }

  if (!as_strcasecmp(ArgStr[Index].str.p_str, "E"))
  {
    if (SetOpSize(0))
      AdrMode = ModE;
    goto AdrFound;
  }

  if (!as_strcasecmp(ArgStr[Index].str.p_str, "S"))
  {
    if (SetOpSize(0))
      AdrMode = ModS;
    goto AdrFound;
  }

  if (!as_strcasecmp(ArgStr[Index].str.p_str, "T"))
  {
    if (SetOpSize(1))
      AdrMode = ModT;
    goto AdrFound;
  }

  /* register ? */

  if (GetReg16(ArgStr[Index].str.p_str, &AdrPart))
  {
    if (SetOpSize(1))
      AdrMode = ModReg;
    goto AdrFound;
  }

  /* immediate? */

  if ((*ArgStr[Index].str.p_str == '#') || (*ArgStr[Index].str.p_str == '='))
  {
    switch (OpSize)
    {
      case 0:
        AdrVals[0] = EvalStrIntExpressionOffs(&ArgStr[Index], 1, Int8, &OK);
        break;
      case 1:
        TmpVal = EvalStrIntExpressionOffs(&ArgStr[Index], 1, Int16, &OK);
        if (OK)
        {
          AdrVals[0] = Lo(TmpVal); AdrVals[1] = Hi(TmpVal);
        }
        break;
      default:
        WrError(ErrNum_UndefOpSizes);
        OK = False;
    }
    if (OK)
    {
      AdrCnt = OpSize + 1; AdrMode = ModImm; AdrPart = 4;
    }
    goto AdrFound;
  }

  if (Cnt == 1)
  {
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionWithFlags(&ArgStr[Index], UInt16, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      Addr &= 0xff;
    if (OK)
    {
      if ((Hi(Addr) != 0x00) && (Hi(Addr) != 0xff)) WrError(ErrNum_OverRange);
      else
      {
        AdrVals[0] = Lo(Addr);
        AdrMode = ModMem; AdrPart = 5; AdrCnt = 1;
      }
    }
    goto AdrFound;
  }

  else
  {
    Boolean Incr = 0;

    if (*ArgStr[Index].str.p_str == '@')
      Incr++;

    OK = GetReg16(ArgStr[Index + 1].str.p_str, &AdrPart);
    if (!OK) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Index + 1]);
    else if ((Incr) && (AdrPart < 2)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Index + 1]);
    else
    {
      tSymbolFlags Flags;

      if (Incr)
        AdrPart |= 4;
      Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[Index], Incr, Int16, &OK, &Flags);
      if (OK)
      {
        if (!AdrPart)
          Addr -= (EProgCounter() + PCDelta);
        if (!mFirstPassUnknownOrQuestionable(Flags) && ((Addr < - 128) || (Addr > 127))) WrError(ErrNum_OverRange);
        else
        {
          AdrMode = ModMem;
          AdrVals[0] = Addr & 0xff;
          AdrCnt = 1;
        }
      }
    }
  }

AdrFound:
  if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
  }
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

static void DecodeLD(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 3))
  {
    DecodeAdr(1, MModAcc | MModReg | MModT | MModE | MModS, 0);
    switch (AdrMode)
    {
      case ModAcc:
        if (OpSize == 0)
        {
          DecodeAdr(2, MModMem | MModImm | MModS | MModE, 1);
          switch (AdrMode)
          {
            case ModMem:
            case ModImm:
              BAsmCode[0] = 0xc0 | AdrPart;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              CodeLen = 1 + AdrCnt;
              break;
            case ModS:
              BAsmCode[0] = 0x06;
              CodeLen = 1;
              break;
            case ModE:
              BAsmCode[0] = 0x40;
              CodeLen = 1;
              break;
          }
        }
        else
        {
          DecodeAdr(2, MModMem | MModImm | MModReg | MModT, 1);
          switch (AdrMode)
          {
            case ModMem:
            case ModImm:
              BAsmCode[0] = 0x80 | AdrPart;
              memcpy(BAsmCode + 1, AdrVals, AdrCnt);
              CodeLen = 1 + AdrCnt;
              break;
            case ModReg:
              BAsmCode[0] = 0x30 | AdrPart;
              CodeLen = 1;
              break;
            case ModT:
              BAsmCode[0] = 0x0b;
              CodeLen = 1;
              break;
          }
        }
        break;
      case ModReg:
        BAsmCode[0] = AdrPart;
        DecodeAdr(2, MModAcc | MModImm, 0);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] |= 0x44;
            CodeLen = 1;
            break;
          case ModImm:
            BAsmCode[0] |= 0x24;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
        }
        break;
      case ModT:
        DecodeAdr(2, MModAcc | MModMem | MModImm, 1);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x09;
            CodeLen = 1;
            break;
          case ModMem:
          case ModImm:
            BAsmCode[0] = 0xa0 | AdrPart;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
        }
        break;
      case ModE:
        DecodeAdr(2, MModAcc, 0);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x48;
            CodeLen = 1;
            break;
        }
        break;
      case ModS:
        DecodeAdr(2, MModAcc, 0);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x07;
            CodeLen = 1;
            break;
        }
        break;
    }
  }
}

static void DecodeST(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 3))
  {
    DecodeAdr(1, MModAcc, 0);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(2, MModMem, 1);
        switch (AdrMode)
        {
          case ModMem:
            BAsmCode[0] = 0x88 | ((1 - OpSize) << 6) | AdrPart;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
        }
        break;
    }
  }
}

static void DecodeXCH(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(1, MModE | MModAcc | MModReg, 0);
    switch (AdrMode)
    {
      case ModE:
        DecodeAdr(2, MModAcc, 0);
        if (AdrMode == ModAcc)
          BAsmCode[CodeLen++] = 0x01;
        break;
      case ModAcc:
        DecodeAdr(2, MModE | MModReg, 0);
        switch (AdrMode)
        {
          case ModE:
            BAsmCode[CodeLen++] = 0x01;
            break;
          case ModReg:
            BAsmCode[CodeLen++] = 0x4c | AdrPart;
            break;
        }
        break;
      case ModReg:
        BAsmCode[0] = 0x4c | AdrPart;
        DecodeAdr(2, MModAcc, 0);
        if (AdrMode == ModAcc)
          CodeLen = 1;
        break;
    }
  }
}

static void DecodePLI(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(1, MModReg, 0);
    if (AdrMode == ModReg)
    {
      if (AdrPart == 1) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x20 | AdrPart;
        DecodeAdr(2, MModImm, 0);
        if (AdrMode == ModImm)
        {
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          CodeLen = 1 + AdrCnt;
        }
      }
    }
  }
}

static void DecodeSSM(Word Code)
{
  UNUSED(Code);

  switch (ArgCnt)
  {
    case 0:
      BAsmCode[0] = 0x2e;
      CodeLen = 1;
      break;
    case 1:
      if ((!GetReg16(ArgStr[1].str.p_str, BAsmCode + 0)) || (BAsmCode[0] < 2)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
      else
      {
        BAsmCode[0] |= 0x2c;
        CodeLen = 1;
      }
      break;
    default:
      (void)ChkArgCnt(0, 1);
  }
}

static void DecodeADDSUB(Word Index)
{
  if (ChkArgCnt(1, 3))
  {
    DecodeAdr(1, MModAcc, 0);
    switch (AdrMode)
    {
      case ModAcc:
        /* EA <-> E implicitly give operand size conflict, we don't have to check it here */

        DecodeAdr(2, MModMem | MModImm | MModE, 1);
        switch (AdrMode)
        {
          case ModMem:
          case ModImm:
            BAsmCode[0] = Index | ((1 - OpSize) << 6) | AdrPart;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
          case ModE:
            BAsmCode[0] = Index - 0x40;
            CodeLen = 1;
            break;
        }
    }
  }
}

static void DecodeMulDiv(Word Index)
{
  if (ChkArgCnt(2, 2))
  {
    OpSize = 1;
    DecodeAdr(1, MModAcc, 0);
    if (AdrMode == ModAcc)
    {
      DecodeAdr(2, MModT, 0);
      if (AdrMode == ModT)
      {
        BAsmCode[0] = Index;
        CodeLen = 1;
      }
    }
  }
}

static void DecodeLogic(Word Index)
{
  if (ChkArgCnt(1, 3))
  {
    OpSize = 0;
    DecodeAdr(1, MModAcc | MModS, 0);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(2, MModMem | MModImm | MModE, 1);
        switch (AdrMode)
        {
          case ModMem:
          case ModImm:
            BAsmCode[0] = Index | AdrPart;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
          case ModE:
            BAsmCode[0] = Index - 0x80;
            CodeLen = 1;
            break;
        }
        break;
      case ModS:
        if (Index == 0xe0) WrError(ErrNum_InvAddrMode);
        else
        {
          DecodeAdr(2, MModImm, 0);
          if (AdrMode == ModImm)
          {
            BAsmCode[0] = (Index == 0xd0) ? 0x39 : 0x3b;
            BAsmCode[1] = *AdrVals;
            CodeLen = 2;
          }
        }
        break;
    }
  }
}

static void DecodeShift(Word Index)
{
  ShiftOrder *POrder = ShiftOrders + Index;

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(1, MModAcc, 0);
    if (AdrMode == ModAcc)
    {
      if ((OpSize == 1) && (!POrder->Code16)) WrError(ErrNum_InvAddrMode);
      else
        BAsmCode[CodeLen++] = OpSize ? POrder->Code16 : POrder->Code;
    }
  }
}

static void DecodeStack(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(1, MModAcc | MModReg, 0);
    switch (AdrMode)
    {
      case ModAcc:
        BAsmCode[0] = 0x0a - (OpSize << 1);
        if (Index)
          BAsmCode[0] = (BAsmCode[0] | 0x30) ^ 2;
        CodeLen++;
        break;
      case ModReg:
        if (AdrPart == 1) WrError(ErrNum_InvAddrMode);
        else
          BAsmCode[CodeLen++] = 0x54 | Index | AdrPart;
        break;
    }
  }
}

static void DecodeIDLD(Word Index)
{
  if (ChkArgCnt(2, 3))
  {
    DecodeAdr(1, MModAcc, 0);
    if (AdrMode == ModAcc)
    {
      if (OpSize == 1) WrError(ErrNum_InvAddrMode);
      else
      {
        DecodeAdr(2, MModMem, 1);
        if (AdrMode == ModMem)
        {
          BAsmCode[0] = 0x90 | Index | AdrPart;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          CodeLen = 1 + AdrCnt;
        }
      }
    }
  }
}

static void DecodeJMPJSR(Word Index)
{
  Word Address;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    Address = (EvalStrIntExpression(&ArgStr[1], UInt16, &OK) - 1) & 0xffff;
    if (OK)
    {
      BAsmCode[CodeLen++] = Index;
      BAsmCode[CodeLen++] = Lo(Address);
      BAsmCode[CodeLen++] = Hi(Address);
    }
  }
}

static void DecodeCALL(Word Index)
{
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    BAsmCode[0] = 0x10 | EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
    if (OK)
      CodeLen = 1;
  }
}

static void DecodeBranch(Word Code)
{
  /* allow both syntaxes for PC-relative addressing */

  if (ArgCnt == 1)
  {
    const char PCArg[] = "PC";

    AppendArg(strlen(PCArg));
    strcpy(ArgStr[ArgCnt].str.p_str, PCArg);
  }

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(1, MModMem, 2); /* !! regard pre-increment after branch */
    if (AdrMode == ModMem)
    {
      if ((AdrPart == 1) || (AdrPart > 3)) WrError(ErrNum_InvAddrMode);
      else if ((Code < 0x60) && (AdrPart != 0))  WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = Code | AdrPart;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddBranch(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBranch);
}

static void AddShift(const char *NName, Byte NCode, Byte NCode16)
{
  if (InstrZ >= ShiftOrderCnt)
    exit(0);
  ShiftOrders[InstrZ].Code = NCode;
  ShiftOrders[InstrZ].Code16 = NCode16;
  AddInstTable(InstTable, NName, InstrZ++, DecodeShift);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(53);

  AddFixed("RET", 0x5c);
  AddFixed("NOP", 0x00);

  ShiftOrders = (ShiftOrder*) malloc(sizeof(ShiftOrder) * ShiftOrderCnt); InstrZ = 0;
  AddShift("SR" , 0x3c, 0x0c);
  AddShift("SRL", 0x3d, 0x00);
  AddShift("RR" , 0x3e, 0x00);
  AddShift("RRL", 0x3f, 0x00);
  AddShift("SL" , 0x0e, 0x0f);

  AddInstTable(InstTable, "LD",     0, DecodeLD);
  AddInstTable(InstTable, "ST",     0, DecodeST);
  AddInstTable(InstTable, "XCH",    0, DecodeXCH);
  AddInstTable(InstTable, "PLI",    0, DecodePLI);
  AddInstTable(InstTable, "SSM",    0, DecodeSSM);

  AddInstTable(InstTable, "ADD", 0xb0, DecodeADDSUB);
  AddInstTable(InstTable, "SUB", 0xb8, DecodeADDSUB);

  AddInstTable(InstTable, "MPY", 0x2c, DecodeMulDiv);
  AddInstTable(InstTable, "DIV", 0x0d, DecodeMulDiv);

  AddInstTable(InstTable, "AND", 0xd0, DecodeLogic);
  AddInstTable(InstTable, "OR",  0xd8, DecodeLogic);
  AddInstTable(InstTable, "XOR", 0xe0, DecodeLogic);

  AddInstTable(InstTable, "PUSH",0x00, DecodeStack);
  AddInstTable(InstTable, "POP", 0x08, DecodeStack);

  AddInstTable(InstTable, "ILD" ,0x00, DecodeIDLD);
  AddInstTable(InstTable, "DLD", 0x08, DecodeIDLD);

  AddInstTable(InstTable, "JMP" ,0x24, DecodeJMPJSR);
  AddInstTable(InstTable, "JSR", 0x20, DecodeJMPJSR);
  AddInstTable(InstTable, "CALL",   0, DecodeCALL);

  AddBranch("BND", 0x2d);
  AddBranch("BRA", 0x74);
  AddBranch("BP" , 0x64);
  AddBranch("BZ" , 0x6c);
  AddBranch("BNZ", 0x7c);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(ShiftOrders);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_807x(void)
{
   CodeLen=0; DontPrint=False; OpSize = -1;

   /* zu ignorierendes */

   if (Memo("")) return;

   if (DecodeIntelPseudo(False)) return;

   if (!LookupInstTable(InstTable, OpPart.str.p_str))
     WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_807x(void)
{
   return False;
}

static void SwitchFrom_807x(void)
{
   DeinitFields();
}

static void SwitchTo_807x(void)
{
   const TFamilyDescr *FoundDescr;

   FoundDescr = FindFamilyByName("807x");

   TurnWords = False;
   SetIntConstMode(eIntConstModeC);

   PCSymbol="$"; HeaderID = FoundDescr->Id; NOPCode = 0x00;
   DivideChars = ","; HasAttrs = False;

   ValidSegs = 1 << SegCode;
   Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
   SegLimits[SegCode] = 0xffff;

   MakeCode = MakeCode_807x; IsDef = IsDef_807x;
   SwitchFrom = SwitchFrom_807x; InitFields();
}

/*---------------------------------------------------------------------------*/

void code807x_init(void)
{
  CPU8070 = AddCPU("8070", SwitchTo_807x);
}
