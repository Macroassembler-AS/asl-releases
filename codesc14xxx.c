/* codesc14xxx.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator SC14xxx                                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>

#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "codepseudo.h"
#include "headids.h"
#include "asmitree.h"
#include "codevars.h"
#include "errmsg.h"
#include "chartrans.h"

#include "codesc14xxx.h"

/*---------------------------------------------------------------------------*/

#define M_14400 (1 << 0)
#define M_14401 (1 << 1)
#define M_14402 (1 << 2)
#define M_14404 (1 << 3)
#define M_14405 (1 << 4)
#define M_14420 (1 << 5)
#define M_14421 (1 << 6)
#define M_14422 (1 << 7)
#define M_14424 (1 << 8)

typedef struct
{
  Word CPUMask;
  Byte Code, MinArg, MaxArg;
} FixedOrder;

static CPUVar CPU14400, CPU14401, CPU14402, CPU14404, CPU14405,
              CPU14420, CPU14421, CPU14422, CPU14424;
static FixedOrder *FixedOrders;
static Word CurrMask;

/*---------------------------------------------------------------------------*/

static void DecodeFixed(Word Index)
{
  FixedOrder *POp = FixedOrders + Index;
  Boolean OK;
  Byte Value;
  tSymbolFlags Flags;

  if (ChkArgCnt((POp->MinArg != POp->MaxArg) ? 1 : 0, (POp->MinArg != POp->MaxArg) ? 1 : 0)
   && (ChkExactCPUMask(POp->CPUMask, CPU14400) >= 0))
  {
    OK = True;
    if (ArgCnt == 0) Value = 0;
    else
    {
      Value = EvalStrIntExpressionWithFlags(&ArgStr[1], Int8, &OK, &Flags);
      if (mFirstPassUnknown(Flags)) Value = POp->MinArg;
      if (OK) OK = ChkRange(Value, POp->MinArg, POp->MaxArg);
    }
    if (OK)
    {
      WAsmCode[0] = (((Word) POp->Code) << 8) + (Value & 0xff);
      CodeLen = 1;
    }
  }
}

/*---------------------------------------------------------------------------*/

static Boolean Toggle;

static void PutByte(Byte Val)
{
  if (Toggle)
    WAsmCode[CodeLen++] |= ((Word) Val) << 8;
  else
    WAsmCode[CodeLen] = Val & 0xff;
  Toggle = !Toggle;
}

static void DecodeDS(Word Code)
{
  int cnt;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    cnt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
      else
      {
        if (!cnt) WrError(ErrNum_NullResMem);
        CodeLen = (cnt + 1) >> 1;
        DontPrint = True;
        BookKeeping();
      }
    }
  }
}

static void DecodeDS16(Word Code)
{
  int cnt;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    cnt = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt16, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
      else
      {
        if (!cnt) WrError(ErrNum_NullResMem);
        CodeLen = cnt;
        DontPrint = True;
        BookKeeping();
      }
    }
  }
}

static void DecodeDC(Word Code)
{
  TempResult t;

  UNUSED(Code);

  as_tempres_ini(&t);
  if (ChkArgCnt(1, ArgCntMax))
  {
    Boolean OK = True;
    char *p, *pEnd;
    tStrComp *pArg;

    Toggle = FALSE;
    forallargs(pArg, OK)
    {
      EvalStrExpression(pArg, &t);
      switch (t.Typ)
      {
        case TempInt:
          if (mFirstPassUnknown(t.Flags))
            t.Contents.Int &= 127;
          if (ChkRange(t.Contents.Int, -128, 255))
            PutByte(t.Contents.Int);
          break;
        case TempString:
          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else
            for (p = t.Contents.str.p_str, pEnd = p + t.Contents.str.len; p < pEnd; p++)
              PutByte(((usint) *p) & 0xff);
          break;
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          /* fall-through */
        default:
          OK = False;
      }
    }
    if (!OK)
      CodeLen = 0;
    else if (Toggle)
      CodeLen++;
  }
  as_tempres_free(&t);
}

static void DecodeDW(Word Code)
{
  int z;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  {
    z = 1; OK = TRUE;
    while ((OK) && (z <= ArgCnt))
    {
      WAsmCode[z - 1] = EvalStrIntExpression(&ArgStr[z], Int16, &OK);
      z++;
    }
    if (OK)
      CodeLen = ArgCnt;
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Byte NCode, Word NMask, Byte NMin, Byte NMax)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].CPUMask = NMask;
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MinArg = NMin;
  FixedOrders[InstrZ].MaxArg = NMax;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(301);

  InstrZ = 0;

  AddInstTable(InstTable, "DC", 0, DecodeDC);
  AddInstTable(InstTable, "DC8", 0, DecodeDC);
  AddInstTable(InstTable, "DW", 0, DecodeDW);
  AddInstTable(InstTable, "DW16", 0, DecodeDW);
  AddInstTable(InstTable, "DS", 0, DecodeDS);
  AddInstTable(InstTable, "DS8", 0, DecodeDS);
  AddInstTable(InstTable, "DS16", 0, DecodeDS16);

  /* standard set */

  AddFixed("BR"       , 0x01, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x01, 0xff);
  AddFixed("BRK"      , 0x6e,                     M_14402 | M_14404 | M_14405                     | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("JMP"      , 0x02, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x01, 0xff);
  AddFixed("JMP1"     , 0x03, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x01, 0xff);
  AddFixed("RTN"      , 0x04, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("WNT"      , 0x08, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x01, 0xff);
  AddFixed("WT"       , 0x09, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x01, 0xff);
  AddFixed("WSC"      , 0x48,                               M_14404 | M_14405           | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("RFEN"     , 0x0b, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("RFDIS"    , 0x0a, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("BK_A"     , 0x0e, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("BK_A1"    , 0x05,                               M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("BK_C"     , 0x0f, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("SLOTZERO" , 0x0d, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("EN_SL_ADJ", 0x2c, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("WNTP1"    , 0x07, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("WNTM1"    , 0x06, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("LD_PTR"   , 0x0c,                     M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("UNLCK"    , 0x28, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RX"     , 0x49,                               M_14404 | M_14405                     | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_TX"     , 0x4a,                               M_14404 | M_14405                     | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_MUTE"   , 0xc1, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_MTOFF"  , 0xc9,                               M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_MUTE1"  , 0xca,                                                   M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_MTOFF1" , 0xcb,                                                   M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_STOFF"  , 0xc2, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_STON"   , 0xcc,                     M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV0"   , 0x80, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV36"  , 0x82, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV30"  , 0x83, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV24"  , 0x84, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV18"  , 0x85, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV12"  , 0x86, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV6"   , 0x87, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV33"  , 0x8a, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV27"  , 0x8b, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV21"  , 0x8c, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV15"  , 0x8d, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV9"   , 0x8e, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RCV3"   , 0x8f, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_NORM"   , 0xc5, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_RST"    , 0xc0, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_LDR"    , 0xc6, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_LDW"    , 0xc7, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_RST1"   , 0xeb,                                                   M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("A_LDR1"   , 0xce,                               M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_LDW1"   , 0xcf,                               M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("A_ST18"   , 0xe1,                     M_14402                                                            , 0x00, 0x00);
  AddFixed("B_ST"     , 0x31, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_ST2"    , 0x21,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_PPT"    , 0x22,                               M_14404 | M_14405                               | M_14424, 0x00, 0x00);
  AddFixed("B_ZT"     , 0x22, M_14400                                                                                , 0x00, 0x00);
  AddFixed("B_ZR"     , 0x2a, M_14400                                                                                , 0x00, 0x00);
  AddFixed("B_AT"     , 0x32, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_AT2"    , 0x37,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BT"     , 0x34, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BTFM"   , 0x23,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_BTFU"   , 0x25,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BTFP"   , 0x35,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BTDU"   , 0x71,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_BTDP"   , 0x72,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_XON"    , 0x27, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_XOFF"   , 0x26, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_SR"     , 0x29, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_AR"     , 0x3a, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_AR2"    , 0x3f,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_RON"    , 0x2f, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_RINV"   , 0x2e, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_BR"     , 0x3c, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BRFU"   , 0x2d,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BRFP"   , 0x3d,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BRFD"   , 0x2a,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_BRDU"   , 0x79,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_BRDP"   , 0x7a,                               M_14404 | M_14405                               | M_14424, 0x00, 0xff);
  AddFixed("B_XR"     , 0x2b, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_XT"     , 0x24, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_WB_ON"  , 0x65,                     M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_WB_OFF" , 0x64,                     M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_WRS"    , 0x39, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_RC"     , 0x33, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_RST"    , 0x20, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("B_DIV1"   , 0x4f,                                         M_14405                                        , 0x00, 0x00);
  AddFixed("B_DIV2"   , 0x4e,                                         M_14405                                        , 0x00, 0x00);
  AddFixed("B_DIV4"   , 0x4d,                                         M_14405                                        , 0x00, 0x00);
  AddFixed("C_LD"     , 0xfa,           M_14401 | M_14402                     | M_14420 | M_14421 | M_14422          , 0x00, 0xff);
  AddFixed("C_ON"     , 0xee,                                                   M_14420 | M_14421 | M_14422          , 0x00, 0x00);
  AddFixed("C_OFF"    , 0xef,                                                   M_14420 | M_14421 | M_14422          , 0x00, 0x00);
  AddFixed("C_LD2"    , 0xba,                                                                                 M_14424, 0x00, 0xff);
  AddFixed("C_ON2"    , 0xae,                                                                                 M_14424, 0x00, 0x00);
  AddFixed("C_OFF2"   , 0xaf,                                                                                 M_14424, 0x00, 0x00);
  AddFixed("D_LDK"    , 0x50, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("D_PREP"   , 0x44, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("D_WRS"    , 0x5f, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("D_LDS"    , 0x57, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("D_RST"    , 0x40, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("M_WR"     , 0xb9, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("M_RST"    , 0xa9, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("M_INI0"   , 0xa0,                               M_14404 | M_14405                               | M_14424, 0x00, 0x00);
  AddFixed("M_INI1"   , 0xa1,                               M_14404 | M_14405                               | M_14424, 0x00, 0x00);
  AddFixed("MEN1N"    , 0xa4, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("MEN1"     , 0xa5, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("P_EN"     , 0xe9,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("P_LDH"    , 0xed,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("P_LDL"    , 0xec,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("P_LD"     , 0xe8,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("P_SC"     , 0xea,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("U_INT0"   , 0x61, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("U_INT1"   , 0x6b, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("U_INT2"   , 0x6d, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("U_INT3"   , 0x6f, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("U_PSC"    , 0x60, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
  AddFixed("U_VINT"   , 0x63,                               M_14404 | M_14405                     | M_14422 | M_14424, 0x00, 0xff);

  /* obsolete stuff - argument range may be incorrect */

  AddFixed("D_ON"     , 0x42, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("D_OFF"    , 0x43, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("RCK_INT"  , 0x62,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("CLK1"     , 0x66,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("CLK3"     , 0x67,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("U_CK8"    , 0x68, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("U_CK4"    , 0x69, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("U_CK2"    , 0x6a, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("U_CK1"    , 0x6c, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("MEN3N"    , 0xa2, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("MEN3"     , 0xa3, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("MEN2N"    , 0xa6, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("MEN2"     , 0xa7, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("M_RD"     , 0xa8, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0xff);
  AddFixed("M_WRS"    , 0xb8, M_14400                                         | M_14420 | M_14421                    , 0x00, 0xff);
  AddFixed("A_ALAW"   , 0xc3, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("A_DT"     , 0xc4, M_14400 | M_14401                               | M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("A_LIN"    , 0xc8,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("A_DT1"    , 0xcd,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("A_STRN"   , 0xe0,                                                   M_14420 | M_14421                    , 0x00, 0x00);
  AddFixed("RCK_EXT"  , 0x63,                                                   M_14420 | M_14421                    , 0x00, 0xff);
  AddFixed("P_SPD0"   , 0xe8, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD1"   , 0xe9, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD2"   , 0xea, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD3"   , 0xeb, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD4"   , 0xec, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD5"   , 0xed, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD6"   , 0xee, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_SPD7"   , 0xef, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD0"   , 0xe0, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD1"   , 0xe1, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD2"   , 0xe2, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD3"   , 0xe3, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD4"   , 0xe4, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD5"   , 0xe5, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD6"   , 0xe6, M_14400                                                                                , 0x00, 0xff);
  AddFixed("P_RPD7"   , 0xe7, M_14400                                                                                , 0x00, 0xff);

  /* aliases */

  AddFixed("B_TX"     , 0x31, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BT2"    , 0x25,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BR2"    , 0x2d,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BTP"    , 0x35,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_BRP"    , 0x3d,           M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0xff);
  AddFixed("B_ON"     , 0x27, M_14400 | M_14401 | M_14402 | M_14404 | M_14405 | M_14420 | M_14421 | M_14422 | M_14424, 0x00, 0x00);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
}

/*---------------------------------------------------------------------------*/

static void MakeCode_sc14xxx(void)
{
  /* Leeranweisung ignorieren */

  if (Memo("")) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_sc14xxx(void)
{
  return FALSE;
}

static void SwitchFrom_sc14xxx(void)
{
  DeinitFields();
}

static void SwitchTo_sc14xxx(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("SC14XXX");

  TurnWords = False; SetIntConstMode(eIntConstModeC);
  PCSymbol = "$"; HeaderID = FoundDescr->Id; NOPCode = 0x0000;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 1;
  SegLimits[SegCode] = 0xff;

  MakeCode=MakeCode_sc14xxx; IsDef = IsDef_sc14xxx;
  SwitchFrom = SwitchFrom_sc14xxx;

  InitFields();
  CurrMask = 1 << (MomCPU - CPU14400);
}

/*---------------------------------------------------------------------------*/

void codesc14xxx_init(void)
{
  CPU14400 = AddCPU("SC14400", SwitchTo_sc14xxx);
  CPU14401 = AddCPU("SC14401", SwitchTo_sc14xxx);
  CPU14402 = AddCPU("SC14402", SwitchTo_sc14xxx);
  CPU14404 = AddCPU("SC14404", SwitchTo_sc14xxx);
  CPU14405 = AddCPU("SC14405", SwitchTo_sc14xxx);
  CPU14420 = AddCPU("SC14420", SwitchTo_sc14xxx);
  CPU14421 = AddCPU("SC14421", SwitchTo_sc14xxx);
  CPU14422 = AddCPU("SC14422", SwitchTo_sc14xxx);
  CPU14424 = AddCPU("SC14424", SwitchTo_sc14xxx);
}
