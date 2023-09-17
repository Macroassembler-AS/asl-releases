/* code6804.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegenerator Motorola/ST 6804                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code6804.h"

typedef struct
{
  char *Name;
  LongInt Code;
} BaseOrder;

enum
{
  ModNone = -1,
  ModInd = 0,
  ModDir = 1,
  ModImm = 2
};

#define MModInd (1 << ModInd)
#define MModDir (1 << ModDir)
#define MModImm (1 << ModImm)

static ShortInt AdrMode;
static Byte AdrVal;

static CPUVar CPU6804;

static BaseOrder *FixedOrders;

/*--------------------------------------------------------------------------*/

static void DecodeAdr(const tStrComp *pArg, Boolean MayImm)
{
  Boolean OK;

  AdrMode = ModNone;

  if (!as_strcasecmp(pArg->str.p_str, "(X)"))
  {
    AdrMode = ModInd;
    AdrVal = 0x00;
    goto chk;
  }
  if (!as_strcasecmp(pArg->str.p_str, "(Y)"))
  {
    AdrMode = ModInd;
    AdrVal = 0x10;
    goto chk;
  }

  if (*pArg->str.p_str == '#')
  {
    AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
    if (OK)
      AdrMode = ModImm;
    goto chk;
  }

  AdrVal = EvalStrIntExpression(pArg, Int8, &OK);
  if (OK)
    AdrMode = ModDir;

chk:
  if ((AdrMode == ModImm) && (!MayImm))
  {
    WrError(ErrNum_InvAddrMode); AdrMode = ModNone;
  }
}

static Boolean IsShort(Byte Adr)
{
  return ((Adr & 0xfc) == 0x80);
}

/*--------------------------------------------------------------------------*/

/* Anweisungen ohne Argument */

static void DecodeFixed(Word Index)
{
  const BaseOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0))
  {
    if ((pOrder->Code >> 16) != 0)
      CodeLen = 3;
    else
      CodeLen = 1 + Ord(Hi(pOrder->Code) != 0);
    if (CodeLen == 3)
      BAsmCode[0] = pOrder->Code >> 16;
    if (CodeLen >= 2)
      BAsmCode[CodeLen - 2] = Hi(pOrder->Code);
    BAsmCode[CodeLen - 1] = Lo(pOrder->Code);
  }
}

/* relative/absolute Spruenge */

static void DecodeRel(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Integer AdrInt = EvalStrIntExpressionWithResult(&ArgStr[1], Int16, &EvalResult) - (EProgCounter() + 1);

    if (EvalResult.OK)
    {
      if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -16) || (AdrInt > 15))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 1;
        BAsmCode[0] = Code + (AdrInt & 0x1f);
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      }
    }
  }
}

static void DecodeJSR_JMP(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word AdrInt = EvalStrIntExpressionWithResult(&ArgStr[1], UInt12, &EvalResult);

    if (EvalResult.OK)
    {
      CodeLen = 2;
      BAsmCode[1] = Lo(AdrInt);
      BAsmCode[0] = Code + (Hi(AdrInt) & 15);
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
    }
  }
}

/* AKKU-Operationen */

static void DecodeALU(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], True);
    switch (AdrMode)
    {
      case ModInd:
        CodeLen = 1;
        BAsmCode[0] = 0xe0 + AdrVal + Code;
        break;
      case ModDir:
        CodeLen = 2;
        BAsmCode[0] = 0xf8 + Code;
        BAsmCode[1] = AdrVal;
        break;
      case ModImm:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 + Code;
        BAsmCode[1] = AdrVal;
        break;
    }
  }
}

/* Datentransfer */

static void DecodeLDA_STA(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], !Code);
    switch (AdrMode)
    {
      case ModInd:
        CodeLen = 1;
        BAsmCode[0] = 0xe0 + Code + AdrVal;
        break;
      case ModDir:
        if (IsShort(AdrVal))
        {
          CodeLen = 1;
          BAsmCode[0] = 0xac + (Code << 4) + (AdrVal & 3);
        }
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xf8 + Code;
          BAsmCode[1] = AdrVal;
        }
        break;
      case ModImm:
        CodeLen = 2;
        BAsmCode[0] = 0xe8 + Code;
        BAsmCode[1] = AdrVal;
        break;
    }
  }
}

static void DecodeLDXI_LDYI(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    Boolean OK;

    BAsmCode[2] = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int8, &OK);
    if (OK)
    {
      CodeLen = 3;
      BAsmCode[0] = 0xb0;
      BAsmCode[1] = Code;
    }
  }
}

static void DecodeMVI(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(2, 2));
  else if (*ArgStr[2].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    tEvalResult EvalResult;

    BAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[1], Int8, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
      BAsmCode[2] = EvalStrIntExpressionOffsWithResult(&ArgStr[2], 1, Int8, &EvalResult);
      if (EvalResult.OK)
      {
        BAsmCode[0] = 0xb0;
        CodeLen = 3;
      }
    }
  }
}

/* Read/Modify/Write-Operationen */

static void DecodeINC_DEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], False);
    switch (AdrMode)
    {
      case ModInd:
        CodeLen = 1;
        BAsmCode[0] = 0xe6 + Code + AdrVal;
        break;
      case ModDir:
        if (IsShort(AdrVal))
        {
          CodeLen = 1;
          BAsmCode[0] = 0xa8 + (Code << 4) + (AdrVal & 3);
        }
        else
        {
          CodeLen = 2;
          BAsmCode[0] = 0xfe + Code;
          BAsmCode[1] = AdrVal;
        }
        break;
    }
  }
}

/* Bitbefehle */

static void DecodeBSET_BCLR(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    tEvalResult EvalResult;
    Byte Bit = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);
    if (EvalResult.OK)
    {
      BAsmCode[0] = Code + Bit;
      BAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[2], Int8, &EvalResult);
      if (EvalResult.OK)
      {
        CodeLen = 2;
        ChkSpace(SegData, EvalResult.AddrSpaceMask);
      }
    }
  }
}

static void DecodeBRSET_BRCLR(Word Code)
{
  if (ChkArgCnt(3, 3))
  {
    tEvalResult EvalResult;
    Byte Bit = EvalStrIntExpressionWithResult(&ArgStr[1], UInt3, &EvalResult);

    if (EvalResult.OK)
    {
      BAsmCode[0] = Code + Bit;
      BAsmCode[1] = EvalStrIntExpressionWithResult(&ArgStr[2], Int8, &EvalResult);
      if (EvalResult.OK)
      {
        Integer AdrInt;

        ChkSpace(SegData, EvalResult.AddrSpaceMask);
        AdrInt = EvalStrIntExpressionWithResult(&ArgStr[3], Int16, &EvalResult) - (EProgCounter() + 3);
        if (EvalResult.OK)
        {
          if (!mSymbolQuestionable(EvalResult.Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            ChkSpace(SegCode, EvalResult.AddrSpaceMask);
            BAsmCode[2] = AdrInt & 0xff;
            CodeLen = 3;
          }
        }
      }
    }
  }
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, 0xff);
}

/*--------------------------------------------------------------------------*/

static void AddFixed(const char *NName, LongInt NCode)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddRel(const char *NName, LongInt NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void AddALU(const char *NName, LongInt NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "JMP", 0x90, DecodeJSR_JMP);
  AddInstTable(InstTable, "JSR", 0x80, DecodeJSR_JMP);
  AddInstTable(InstTable, "LDA", 0, DecodeLDA_STA);
  AddInstTable(InstTable, "STA", 1, DecodeLDA_STA);
  AddInstTable(InstTable, "LDXI", 0x80, DecodeLDXI_LDYI);
  AddInstTable(InstTable, "LDYI", 0x81, DecodeLDXI_LDYI);
  AddInstTable(InstTable, "MVI", 0, DecodeMVI);
  AddInstTable(InstTable, "INC", 0, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINC_DEC);
  AddInstTable(InstTable, "BSET", 0xd8, DecodeBSET_BCLR);
  AddInstTable(InstTable, "BCLR", 0xd0, DecodeBSET_BCLR);
  AddInstTable(InstTable, "BRSET", 0xc8, DecodeBRSET_BRCLR);
  AddInstTable(InstTable, "BRCLR", 0xc0, DecodeBRSET_BRCLR);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);

  InstrZ = 0;
  AddFixed("CLRA", 0x00fbff);
  AddFixed("CLRX", 0xb08000);
  AddFixed("CLRY", 0xb08100);
  AddFixed("COMA", 0x0000b4);
  AddFixed("ROLA", 0x0000b5);
  AddFixed("ASLA", 0x00faff);
  AddFixed("INCA", 0x00feff);
  AddFixed("INCX", 0x0000a8);
  AddFixed("INCY", 0x0000a9);
  AddFixed("DECA", 0x00ffff);
  AddFixed("DECX", 0x0000b8);
  AddFixed("DECY", 0x0000b9);
  AddFixed("TAX" , 0x0000bc);
  AddFixed("TAY" , 0x0000bd);
  AddFixed("TXA" , 0x0000ac);
  AddFixed("TYA" , 0x0000ad);
  AddFixed("RTS" , 0x0000b3);
  AddFixed("RTI" , 0x0000b2);
  AddFixed("NOP" , 0x000020);

  AddRel("BCC", 0x40);
  AddRel("BHS", 0x40);
  AddRel("BCS", 0x60);
  AddRel("BLO", 0x60);
  AddRel("BNE", 0x00);
  AddRel("BEQ", 0x20);

  AddALU("ADD", 0x02);
  AddALU("SUB", 0x03);
  AddALU("CMP", 0x04);
  AddALU("AND", 0x05);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw | e_moto_8_ds);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(FixedOrders);
}

/*--------------------------------------------------------------------------*/

static void MakeCode_6804(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_6804(void)
{
  return (Memo("SFR"));
}

static void SwitchFrom_6804(void)
{
  DeinitFields();
}

static void SwitchTo_6804(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "PC";
  HeaderID = 0x64;
  NOPCode = 0x20;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfff;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegData] = 0;
  SegLimits[SegData] = 0xff;

  MakeCode = MakeCode_6804;
  IsDef = IsDef_6804;
  SwitchFrom = SwitchFrom_6804;
  InitFields();
}

void code6804_init(void)
{
  CPU6804 = AddCPU("6804", SwitchTo_6804);
}
