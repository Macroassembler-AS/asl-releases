/* code4500.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator MELPS-4500                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>

#include "chunks.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "errmsg.h"


typedef struct
{
  Word Code;
  IntType Max;
} ConstOrder;


static CPUVar CPU4500;

static ConstOrder *ConstOrders;

/*-------------------------------------------------------------------------*/

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, 415);
}

static void DecodeDATA_4500(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int10, Int4);
}

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1;
    WAsmCode[0] = Code;
  }
}

static void DecodeConst(Word Index)
{
  const ConstOrder *pOrder = ConstOrders + Index;

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], pOrder->Max, &OK);
    if (OK)
    {
      CodeLen = 1;
      WAsmCode[0] += pOrder->Code;
    }
  }
}

static void DecodeSZD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 0))
  {
    CodeLen = 2;
    WAsmCode[0] = 0x024;
    WAsmCode[1] = 0x02b;
  }
}

static void DecodeSEA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    WAsmCode[1] = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[1] += 0x070;
      WAsmCode[0] = 0x025;
    }
  }
}

static void DecodeB(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Word AdrWord;
    Boolean OK;
    tSymbolFlags Flags;

    AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt13, &OK, &Flags);
    if (OK && ChkSamePage(EProgCounter(), AdrWord, 7, Flags))
    {
      CodeLen = 1;
      WAsmCode[0] = 0x180 + (AdrWord & 0x7f);
    }
  }
}

static void DecodeBL_BML(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    Boolean OK;
    Word AdrWord;

    if (ArgCnt == 1)
      AdrWord = EvalStrIntExpression(&ArgStr[1], UInt13, &OK);
    else
    {
      AdrWord = EvalStrIntExpression(&ArgStr[1], UInt6, &OK) << 7;
      if (OK)
        AdrWord += EvalStrIntExpression(&ArgStr[2], UInt7, &OK);
    }
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[1] = 0x200 + (AdrWord & 0x7f) + ((AdrWord >> 12) << 7);
      WAsmCode[0] = Code + ((AdrWord >> 7) & 0x1f);
    }
  }
}

static void DecodeBLA_BMLA(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord;

    AdrWord = EvalStrIntExpression(&ArgStr[1], UInt6, &OK);
    if (OK)
    {
      CodeLen = 2;
      WAsmCode[1] = 0x200 + (AdrWord & 0x0f) + ((AdrWord & 0x30) << 2);
      WAsmCode[0] = Code;
    }
  }
}

static void DecodeBM(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word AdrWord;

    AdrWord = EvalStrIntExpression(&ArgStr[1], UInt13, &OK);
    if (OK)
    {
      if ((AdrWord >> 7) != 2) WrError(ErrNum_NotFromThisAddress);
      else
      {
        CodeLen = 1;
        WAsmCode[0] = 0x100 + (AdrWord & 0x7f);
      }
    }
  }
}

static void DecodeLXY(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2))
  {
    Boolean OK;
    Word AdrWord;

    if (ArgCnt == 1)
      AdrWord = EvalStrIntExpression(&ArgStr[1], Int8, &OK);
    else
    {
      AdrWord = EvalStrIntExpression(&ArgStr[1], Int4, &OK) << 4;
      if (OK)
        AdrWord += EvalStrIntExpression(&ArgStr[2], Int4, &OK);
    }
    if (OK)
    {
      CodeLen = 1;
      WAsmCode[0] = 0x300 + AdrWord;
    }
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddConst(const char *NName, Word NCode, IntType NMax)
{
  order_array_rsv_end(ConstOrders, ConstOrder);
  ConstOrders[InstrZ].Code = NCode;
  ConstOrders[InstrZ].Max = NMax;
  AddInstTable(InstTable, NName, InstrZ++, DecodeConst);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  AddInstTable(InstTable, "SZD", 0, DecodeSZD);
  AddInstTable(InstTable, "SEA", 0, DecodeSEA);
  AddInstTable(InstTable, "B", 0, DecodeB);
  AddInstTable(InstTable, "BL", 0x0e0, DecodeBL_BML);
  AddInstTable(InstTable, "BML", 0x0c0, DecodeBL_BML);
  AddInstTable(InstTable, "BLA", 0x010, DecodeBLA_BMLA);
  AddInstTable(InstTable, "BMLA", 0x030, DecodeBLA_BMLA);
  AddInstTable(InstTable, "BM", 0, DecodeBM);
  AddInstTable(InstTable, "LXY", 0, DecodeLXY);

  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_4500);

  AddFixed("AM"   , 0x00a);  AddFixed("AMC"  , 0x00b);  AddFixed("AND"  , 0x018);
  AddFixed("CLD"  , 0x011);  AddFixed("CMA"  , 0x01c);  AddFixed("DEY"  , 0x017);
  AddFixed("DI"   , 0x004);  AddFixed("EI"   , 0x005);  AddFixed("IAP0" , 0x260);
  AddFixed("IAP1" , 0x261);  AddFixed("IAP2" , 0x262);  AddFixed("IAP3" , 0x263);
  AddFixed("IAP4" , 0x264);  AddFixed("INY"  , 0x013);  AddFixed("NOP"  , 0x000);
  AddFixed("OR"   , 0x019);  AddFixed("OP0A" , 0x220);  AddFixed("OP1A" , 0x221);
  AddFixed("POF"  , 0x002);  AddFixed("POF2" , 0x008);  AddFixed("RAR"  , 0x01d);
  AddFixed("RC"   , 0x006);  AddFixed("RC3"  , 0x2ac);  AddFixed("RC4"  , 0x2ae);
  AddFixed("RD"   , 0x014);  AddFixed("RT"   , 0x044);  AddFixed("RTI"  , 0x046);
  AddFixed("RTS"  , 0x045);  AddFixed("SC"   , 0x007);  AddFixed("SC3"  , 0x2ad);
  AddFixed("SC4"  , 0x2af);  AddFixed("SD"   , 0x015);  AddFixed("SEAM" , 0x026);
  AddFixed("SNZ0" , 0x038);  AddFixed("SNZP" , 0x003);  AddFixed("SNZT1", 0x280);
  AddFixed("SNZT2", 0x281);  AddFixed("SNZT3", 0x282);  AddFixed("SPCR" , 0x299);
  AddFixed("STCR" , 0x298);  AddFixed("SZC"  , 0x02f);  AddFixed("T1R1" , 0x2ab);
  AddFixed("T3AB" , 0x232);  AddFixed("TAB"  , 0x01e);  AddFixed("TAB3" , 0x272);
  AddFixed("TABE" , 0x02a);  AddFixed("TAD"  , 0x051);  AddFixed("TAI1" , 0x253);
  AddFixed("TAL1" , 0x24a);  AddFixed("TAMR" , 0x252);  AddFixed("TASP" , 0x050);
  AddFixed("TAV1" , 0x054);  AddFixed("TAW1" , 0x24b);  AddFixed("TAW2" , 0x24c);
  AddFixed("TAW3" , 0x24d);  AddFixed("TAX"  , 0x052);  AddFixed("TAY"  , 0x01f);
  AddFixed("TAZ"  , 0x053);  AddFixed("TBA"  , 0x00e);  AddFixed("TC1A" , 0x2a8);
  AddFixed("TC2A" , 0x2a9);  AddFixed("TDA"  , 0x029);  AddFixed("TEAB" , 0x01a);
  AddFixed("TI1A" , 0x217);  AddFixed("TL1A" , 0x20a);  AddFixed("TL2A" , 0x20b);
  AddFixed("TL3A" , 0x20c);  AddFixed("TLCA" , 0x20d);  AddFixed("TMRA" , 0x216);
  AddFixed("TPTA" , 0x2a5);  AddFixed("TPAA" , 0x2aa);  AddFixed("TR1A" , 0x2a6);
  AddFixed("TR1AB", 0x23f);  AddFixed("TV1A" , 0x03f);  AddFixed("TW1A" , 0x20e);
  AddFixed("TW2A" , 0x20f);  AddFixed("TW3A" , 0x210);  AddFixed("TYA"  , 0x00c);
  AddFixed("WRST" , 0x2a0);

  InstrZ = 0;
  AddConst("A"   , 0x060, UInt4);  AddConst("LA"  , 0x070, UInt4);
  AddConst("LZ"  , 0x048, UInt2);  AddConst("RB"  , 0x04c, UInt2);
  AddConst("SB"  , 0x05c, UInt2);  AddConst("SZB" , 0x020, UInt2);
  AddConst("TABP", 0x080, UInt6);  AddConst("TAM" , 0x2c0, UInt4);
  AddConst("TMA" , 0x2b0, UInt4);  AddConst("XAM" , 0x2d0, UInt4);
  AddConst("XAMD", 0x2f0, UInt4);  AddConst("XAMI", 0x2e0, UInt4);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(ConstOrders);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_4500(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_4500(void)
{
  return (Memo("SFR"));
}

static void SwitchFrom_4500(void)
{
  DeinitFields();
}

static void SwitchTo_4500(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x12;
  NOPCode = 0x000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode ] = 2; ListGrans[SegCode ] = 2; SegInits[SegCode ] = 0;
  SegLimits[SegCode] = 0x1fff;
  Grans[SegData ] = 1; ListGrans[SegData ] = 1; SegInits[SegData ] = 0;
  SegLimits[SegData] = 415;

  MakeCode = MakeCode_4500;
  IsDef = IsDef_4500;
  SwitchFrom = SwitchFrom_4500;

  InitFields();
}

void code4500_init(void)
{
  CPU4500 = AddCPU("MELPS4500", SwitchTo_4500);
}
