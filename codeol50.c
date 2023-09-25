/* codeol50.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator OKI OLMS-50-Familie                                         */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmallg.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "codeol50.h"

typedef enum
{
  ModACC = 0,
  ModAP = 1,
  ModAX = 2,
  ModImm = 3,
  ModNone = 0x7f
} tAdrMode;

typedef struct
{
  Word ACCCode, ImmCode;
  Word CPUMask;
} tAriOrder;
static tAriOrder *AriOrders;

typedef struct
{
  Word Code, CPUMask;
} tAPOrder;
static tAPOrder *APOrders;

typedef struct
{
  Word APCode, AXCode, CPUMask;
} tAPAXOrder;
static tAPAXOrder *APAXOrders;

typedef struct
{
  Word Code, CPUMask;
} tFixedOrder;
static tFixedOrder *FixedOrders;

typedef struct
{
  Word Code, CPUMask;
} tImmOrder;
static tImmOrder *ImmOrders;

typedef struct
{
  Word Code, PlusCPUMask, MinusCPUMask;
} tRelOrder;
static tRelOrder *RelOrders;

typedef struct
{
  Word Code, CPUMask;
} tDSPOrder;
static tDSPOrder *DSPOrders;

typedef struct
{
  Word Code;
  Word Shift;
  Word CPUMask;
} tONOFFOrder;
static tONOFFOrder *ONOFFOrders;

typedef struct
{
  Word APCode, ImmCode;
  Boolean AllowAP;
  Word CPUMask;
} tCtrlOrder;
static tCtrlOrder *CtrlOrders;

typedef struct
{
  Word Code;
  Word CPUMask;
} tMemOrder;
static tMemOrder *MemOrders;

#define MModACC (1 << ModACC)
#define MModAP (1 << ModAP)
#define MModAX (1 << ModAX)
#define MModImm (1 << ModImm)

#define M_5054 (1 << 0)
#define M_5055 (1 << 1)
#define M_5056 (1 << 2)
#define M_6051 (1 << 3)
#define M_6052 (1 << 4)

static CPUVar CPU5054, CPU5055, CPU5056, CPU6051, CPU6052;
static tAdrMode AdrMode;
static Word AdrVal;
static LongInt PRegAssume;
static IntType CodeIntType, DataIntType;

/*-------------------------------------------------------------------------*/

static Boolean DecodeAdr(const tStrComp *pArg, Word Mask)
{
  Boolean OK;
  tSymbolFlags Flags;

  AdrMode = ModNone;

  if (!as_strcasecmp(pArg->str.p_str, "ACC"))
  {
    AdrMode = ModACC;
    goto AdrFound;
  }

  if (0[pArg->str.p_str] == '#')
  {
    AdrVal = EvalStrIntExpressionOffs(pArg, 1, Int4, &OK);
    if (OK)
    {
      AdrVal = (AdrVal & 15) << 4;
      AdrMode = ModImm;
    }
    goto AdrFound;
  }

  AdrVal = EvalStrIntExpressionWithFlags(pArg, DataIntType, &OK, &Flags);
  if (OK)
  {
    if (mFirstPassUnknown(Flags))
      AdrVal &= 15;

    if (AdrVal < 16)
      AdrMode = ModAP;
    else if (((AdrVal >> 4) & 15) == PRegAssume)
    {
      AdrVal = (AdrVal & 15) | 0x100;
      AdrMode = ModAP;
    }
    else if (Mask & MModAX)
      AdrMode = ModAX;
    else
      WrError(ErrNum_AddrOnDifferentPage);
  }

AdrFound:

  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone; AdrCnt = 0;
  }
  return (AdrMode != ModNone);
}

static Boolean CheckCPU(Byte Mask)
{
  if (MomCPU == CPU5054)
    Mask &= M_5054;
  else if (MomCPU == CPU5055)
    Mask &= M_5055;
  else if (MomCPU == CPU5056)
    Mask &= M_5056;
  else if (MomCPU == CPU6051)
    Mask &= M_6051;
  else if (MomCPU == CPU6052)
    Mask &= M_6052;
  return !!Mask;
}

static int ImmPtr(const char *pArg)
{
  return !!(*pArg == '#');
}

/*-------------------------------------------------------------------------*/

static void DecodeAri(Word Index)
{
  const tAriOrder *pOrder = AriOrders + Index;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    if (DecodeAdr(&ArgStr[2], MModAP))
    {
      WAsmCode[0] = AdrVal;
      DecodeAdr(&ArgStr[1], MModACC | MModImm);
      switch (AdrMode)
      {
        case ModACC:
          WAsmCode[0] |= pOrder->ACCCode;
          CodeLen = 1;
          break;
        case ModImm:
          WAsmCode[0] |= pOrder->ImmCode | AdrVal;
          CodeLen = 1;
          break;
        default:
          break;
      }
    }
  }
}

static void DecodeADJUST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_5054 | M_5055, CPU5054) >= 0))
  {
    if (DecodeAdr(&ArgStr[2], MModAP))
    {
      Boolean OK;
      Word N = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

      if (OK)
      {
        N = ((~N) + 1) & 15;
        WAsmCode[0] = 0x3000 | AdrVal | (N << 4);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeAP(Word Index)
{
  const tAPOrder *pOrder = APOrders + Index;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    if (DecodeAdr(&ArgStr[1], MModAP))
    {
      WAsmCode[0] = pOrder->Code | AdrVal;
      CodeLen = 1;
    }
  }
}

static void DecodeFixed(Word Index)
{
  const tFixedOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    WAsmCode[0] = pOrder->Code;
    CodeLen = 1;
  }
}

static void DecodeMOV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_5054 | M_5055 | M_5056 | M_6051 | M_6052, CPU5054) >= 0))
  {
    DecodeAdr(&ArgStr[2], MModACC | MModAP | MModAX);
    switch (AdrMode)
    {
      case ModACC:
        DecodeAdr(&ArgStr[1], MModAP | MModAX);
        switch (AdrMode)
        {
          case ModAP:
          case ModAX:
            WAsmCode[0] = 0x3e00 | AdrVal;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case ModAP:
        WAsmCode[0] = AdrVal;
        DecodeAdr(&ArgStr[1], MModACC | MModImm);
        switch (AdrMode)
        {
          case ModACC:
            WAsmCode[0] |= 0x3c00;
            CodeLen = 1;
            break;
          case ModImm:
            WAsmCode[0] |= 0x1c00 | AdrVal;
            CodeLen = 1;
            break;
          default:
            break;
        }
        break;
      case ModAX:
        WAsmCode[0] = AdrVal;
        if (DecodeAdr(&ArgStr[1], MModACC))
        {
          WAsmCode[0] |= 0x3c00;
          CodeLen = 1;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeAPAX(Word Index)
{
  const tAPAXOrder *pOrder = APAXOrders + Index;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    DecodeAdr(&ArgStr[1], MModAP | MModAX);
    switch (AdrMode)
    {
      case ModAP:
        WAsmCode[0] = pOrder->APCode | AdrVal;
        CodeLen = 1;
        break;
      case ModAX:
        WAsmCode[0] = pOrder->AXCode | AdrVal;
        CodeLen = 1;
        break;
      default:
        break;
    }
  }
}

static void DecodeJMPIO(Word Code)
{
  if (ChkArgCnt(1, 1));
  else if (ChkExactCPUMask(M_5054 | M_5055 | M_5056 | M_6051 | M_6052, CPU5054) < 0);
  else if (*ArgStr[1].str.p_str != '@') WrError(ErrNum_InvAddrMode);
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, &ArgStr[1], 1);
    if (DecodeAdr(&Arg, MModAP))
    {
      WAsmCode[0] = Code | AdrVal;
      CodeLen = 1;
    }
  }
}

static void DecodeJMP(Word Code)
{
  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (ChkExactCPUMask(M_5054 | M_5055 | M_5056 | M_6051 | M_6052, CPU5054) < 0);
  else if (*ArgStr[1].str.p_str == '@')
    DecodeJMPIO(0x00d0);
  else
  {
    Boolean OK;
    tSymbolFlags Flags;

    WAsmCode[0] = EvalStrIntExpressionWithFlags(&ArgStr[1], CodeIntType, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags) && (WAsmCode[0] >= SegLimits[SegCode]))
        WAsmCode[0] = SegLimits[SegCode];
      if (ChkRange(WAsmCode[0], 0, SegLimits[SegCode]))
      {
        WAsmCode[0] |= 0x2000;
        CodeLen = 1;
      }
    }
  }
}

static void DecodeCALL(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(M_6051 | M_6052, CPU5054) >= 0))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], (MomCPU == CPU6052) ? UInt11 : UInt10, &OK);
    if (OK)
    {
      WAsmCode[0] |= (MomCPU == CPU6052) ? 0x2800 : 0x2c00;
      CodeLen = 1;
    }
  }
}

static void DecodeMSA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(M_6051, CPU5054) >= 0))
  {
    Boolean OK;

    WAsmCode[0] = EvalStrIntExpression(&ArgStr[1], UInt9, &OK);
    if (OK)
    {
      WAsmCode[0] |= 0x2a00;
      CodeLen = 1;
    }
  }
}

static void DecodeRel(Word Index)
{
  const tRelOrder *pOrder = RelOrders + Index;
  Boolean AllowMinus = CheckCPU(pOrder->MinusCPUMask),
       AllowPlus = CheckCPU(pOrder->PlusCPUMask);

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->MinusCPUMask | pOrder->PlusCPUMask, CPU5054) >= 0))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Integer Distance = EvalStrIntExpressionWithFlags(&ArgStr[1], CodeIntType, &OK, &Flags) - (EProgCounter() + 1);
    Integer MinDist = AllowMinus ? -32 : 0,
            MaxDist = AllowPlus ? 31 : -1;

    if (((Distance < MinDist) || (Distance > MaxDist)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
    else
    {
      if (Distance >= 0)
        WAsmCode[0] = 0;
      else
      {
        WAsmCode[0] = 0x0100;
        Distance = -1 - Distance;
      }
      WAsmCode[0] |= pOrder->Code | (Distance & 0x1f);
      CodeLen = 1;
    }
  }
}

static void DecodeCtrl(Word Index)
{
  const tCtrlOrder *pOrder = CtrlOrders + Index;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    DecodeAdr(&ArgStr[1], MModImm | MModAP);
    switch (AdrMode)
    {
      case ModImm:
        WAsmCode[0] = pOrder->ImmCode | (AdrVal >> 4);
        CodeLen = 1;
        break;
      case ModAP:
        if (!pOrder->AllowAP && (AdrVal & 0x100)) WrError(ErrNum_InvAddrMode);
        else
        {
          WAsmCode[0] = pOrder->APCode | AdrVal;
          CodeLen = 1;
        }
        break;
      default:
        break;
    }
  }
}

static void DecodeDSP(Word Index)
{
  const tDSPOrder *pOrder = DSPOrders + Index;

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0)
   && DecodeAdr(&ArgStr[2], MModAP))
  {
    Boolean OK;
    Word Digit;

    Digit = EvalStrIntExpressionOffs(&ArgStr[1], ImmPtr(ArgStr[1].str.p_str), UInt4, &OK);
    if (OK)
    {
      WAsmCode[0] = pOrder->Code | AdrVal | (Digit << 4);
      CodeLen = 1;
    }
  }
}

static void DecodeINTENDSAB(Word Code)
{
  if (ChkArgCnt(0, 0)
   && (ChkExactCPUMask(M_5054 | M_5055 | M_6051, CPU5054) >= 0))
  {
    WAsmCode[0] = 0x04b0 | (Code & 0x0f);
    WAsmCode[1] = 0x0440 | ((Code >> 4) & 0x0f);
    CodeLen = 2;
  }
}

static void DecodeONOFF(Word Index)
{
  const tONOFFOrder *pOrder = ONOFFOrders + Index;
  Boolean IsON;

  if (!ChkArgCnt(1, 1));
  else if (ChkExactCPUMask(pOrder->CPUMask, CPU5054) < 0);
  else if (CheckONOFFArg(&ArgStr[1], &IsON))
  {
    WAsmCode[0] = pOrder->Code | ((IsON ? 3 : 2) << pOrder->Shift);
    CodeLen = 1;
  }
}

static void DecodeImm(Word Index)
{
  const tImmOrder *pOrder = ImmOrders + Index;

  if (ChkArgCnt(1, 1)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    Boolean OK;
    Word Freq = EvalStrIntExpressionOffs(&ArgStr[1], ImmPtr(ArgStr[1].str.p_str), UInt4, &OK);

    if (OK)
    {
      WAsmCode[0] = pOrder->Code | Freq;
      CodeLen = 1;
    }
  }
}

static void DecodeBUZZER(Word Code)
{
  int ReqArgCnt = (MomCPU == CPU5055) ? 1 : 2;

  UNUSED(Code);

  if (ChkArgCnt(ReqArgCnt, ReqArgCnt)
   && (ChkExactCPUMask(M_5054 | M_5055, CPU5054) >= 0))
  {
    Boolean OK = True, OK2;
    Word Freq, Sound;

    Freq =  (ReqArgCnt >= 2) ? EvalStrIntExpressionOffs(&ArgStr[1], ImmPtr(ArgStr[1].str.p_str), UInt2, &OK) : 2;
    Sound = EvalStrIntExpressionOffs(&ArgStr[ArgCnt], ImmPtr(ArgStr[ArgCnt].str.p_str), UInt2, &OK2);
    if (OK && OK2)
    {
      WAsmCode[0] = 0x04c0 | Freq | (Sound << 2);
      CodeLen = 1;
    }
  }
}

static void DecodeINP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_5056, CPU5054) >= 0))
  {
    Boolean OK;
    Word Port = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK && DecodeAdr(&ArgStr[2], MModAP))
    {
      WAsmCode[0] = 0x3400 | AdrVal | (Port << 4);
      CodeLen = 1;
    }
  }
}

static void DecodeIN(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_6052, CPU5054) >= 0))
  {
    Boolean OK;
    Word Port = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);

    if (OK && DecodeAdr(&ArgStr[2], MModAP))
    {
      WAsmCode[0] = 0x0400 | AdrVal | (Port << 4);
      CodeLen = 1;
    }
  }
}

static void DecodeOUT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2)
   && (ChkExactCPUMask(M_5056 | M_6052, CPU5054) >= 0))
  {
    Boolean OK;
    Word Port = EvalStrIntExpression(&ArgStr[2], (MomCPU == CPU6052) ? UInt5 : UInt4, &OK);

    if (OK)
    {
      if (MomCPU == CPU6052)
        Port = (Port & 15) | ((Port & 16) << 1);
      DecodeAdr(&ArgStr[1], MModAP | MModImm);
      switch (AdrMode)
      {
        case ModAP:
          WAsmCode[0] = ((MomCPU == CPU6052) ? 0x0800 : 0x3600) | AdrVal | (Port << 4);
          CodeLen = 1;
          break;
        case ModImm:
          WAsmCode[0] = ((MomCPU == CPU6052) ? 0x0c00 : 0x0400) | (AdrVal >> 4) | (Port << 4);
          CodeLen = 1;
          break;
        default:
          break;
      }
    }
  }
}

static void DecodeMem(Word Index)
{
  const tMemOrder *pOrder = MemOrders + Index;

  if (ChkArgCnt(0, 3)
   && (ChkExactCPUMask(pOrder->CPUMask, CPU5054) >= 0))
  {
    WAsmCode[0] = pOrder->Code;
    if (ArgCnt < 1)
      CodeLen = 1;
    else
    {
      if (!strcmp(ArgStr[1].str.p_str, "-"))
        WAsmCode[0] |= 0x0010;
      else if (strcmp(ArgStr[1].str.p_str, "+"))
      {
        WrError(ErrNum_InvAddrMode);
        return;
      }
      if (ArgCnt < 2)
      {
        WAsmCode[0] |= 0x0020;
        CodeLen = 1;
      }
      else
      {
        if (!as_strcasecmp(ArgStr[2].str.p_str, "Z"))
          WAsmCode[0] |= 0x0040;
        else if (!as_strcasecmp(ArgStr[2].str.p_str, "N"))
          WAsmCode[0] |= 0x0080;
        else if (!as_strcasecmp(ArgStr[2].str.p_str, "L"))
          WAsmCode[0] |= 0x0200;
        else
        {
          WrError(ErrNum_InvAddrMode);
          return;
        }
        if (ArgCnt < 3)
          CodeLen = 1;
        else if (as_strcasecmp(ArgStr[3].str.p_str, "L"))
        {
          WrError(ErrNum_InvAddrMode);
          return;
        }
        else
        {
          WAsmCode[0] |= 0x0200;
          CodeLen = 1;
        }
      }
    }
  }
}

/*-------------------------------------------------------------------------*/

static void DecodeDATA_OLMS50(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int14, Int4);
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, SegLimits[SegData]);
}

/*-------------------------------------------------------------------------*/

static void AddAri(const char *pName, Word ACCCode, Word ImmCode, Word CPUMask)
{
  order_array_rsv_end(AriOrders, tAriOrder);
  AriOrders[InstrZ].ACCCode = ACCCode;
  AriOrders[InstrZ].ImmCode = ImmCode;
  AriOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeAri);
}

static void AddAP(const char *pName, Word Code, Word CPUMask)
{
  order_array_rsv_end(APOrders, tAPOrder);
  APOrders[InstrZ].Code = Code;
  APOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeAP);
}

static void AddImm(const char *pName, Word Code, Word CPUMask)
{
  order_array_rsv_end(ImmOrders, tImmOrder);
  ImmOrders[InstrZ].Code = Code;
  ImmOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeImm);
}

static void AddAPAX(const char *pName, Word APCode, Word AXCode, Word CPUMask)
{
  order_array_rsv_end(APAXOrders, tAPAXOrder);
  APAXOrders[InstrZ].APCode = APCode;
  APAXOrders[InstrZ].AXCode = AXCode;
  APAXOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeAPAX);
}

static void AddFixed(const char *pName, Word Code, Word CPUMask)
{
  order_array_rsv_end(FixedOrders, tFixedOrder);
  FixedOrders[InstrZ].Code = Code;
  FixedOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeFixed);
}

static void AddRel(const char *pName, Word Code, Word PlusCPUMask, Word MinusCPUMask)
{
  order_array_rsv_end(RelOrders, tRelOrder);
  RelOrders[InstrZ].Code = Code;
  RelOrders[InstrZ].PlusCPUMask = PlusCPUMask;
  RelOrders[InstrZ].MinusCPUMask = MinusCPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeRel);
}

static void AddCtrl(const char *pName, Word APCode, Word ImmCode, Boolean AllowAP, Word CPUMask)
{
  order_array_rsv_end(CtrlOrders, tCtrlOrder);
  CtrlOrders[InstrZ].APCode = APCode;
  CtrlOrders[InstrZ].ImmCode = ImmCode;
  CtrlOrders[InstrZ].AllowAP = AllowAP;
  CtrlOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeCtrl);
}

static void AddDSP(const char *pName, Word Code, Word CPUMask)
{
  order_array_rsv_end(DSPOrders, tDSPOrder);
  DSPOrders[InstrZ].Code = Code;
  DSPOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeDSP);
}

static void AddONOFFInstr(const char *pName, Word Code, Word Shift, Word CPUMask)
{
  order_array_rsv_end(ONOFFOrders, tONOFFOrder);
  ONOFFOrders[InstrZ].Code = Code;
  ONOFFOrders[InstrZ].CPUMask = CPUMask;
  ONOFFOrders[InstrZ].Shift = Shift;
  AddInstTable(InstTable, pName, InstrZ++, DecodeONOFF);
}

static void AddMem(const char *pName, Word Code, Word CPUMask)
{
  order_array_rsv_end(MemOrders, tMemOrder);
  MemOrders[InstrZ].Code = Code;
  MemOrders[InstrZ].CPUMask = CPUMask;
  AddInstTable(InstTable, pName, InstrZ++, DecodeMem);
}

static void InitFields(void)
{
  int N;
  char Op[20];

  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  InstrZ = 0;
  AddAri("ADD", 0x0040, 0x1800, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("SUB", 0x0240, 0x1a00, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("CMP", 0x02e0, 0x1600, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("XOR", 0x0070, 0x1e00, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("BIT", 0x00e0, 0x1400, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("BIS", 0x0060, 0x1000, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAri("BIC", 0x0260, 0x1200, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);

  InstrZ = 0;
  if (CheckCPU(M_6052))
  {
    AddRel("BEQ", 0x3a40, M_6052, M_6052);
    AddRel("BZE", 0x3a40, M_6052, M_6052);
    AddRel("BNE", 0x3ac0, M_6052, M_6052);
    AddRel("BNZ", 0x3ac0, M_6052, M_6052);
    AddRel("BCS", 0x3a00, M_6052, M_6052);
    AddRel("BCC", 0x3a80, M_6052, M_6052);
    AddRel("BGT", 0x3a20, M_6052, M_6052);
    AddRel("BLE", 0x3aa0, M_6052, M_6052);
    AddRel("BGE", 0x3a60, M_6052, M_6052);
    AddRel("BLT", 0x3ae0, M_6052, M_6052);
  }
  else
  {
    AddRel("BEQ", 0x0640, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BZE", 0x0640, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BNE", 0x06c0, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BNZ", 0x06c0, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BCS", (MomCPU == CPU5056) ? 0x0620 : 0x0600, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BLT", (MomCPU == CPU6051) ? 0x06e0 : (MomCPU == CPU5056 ? 0x0620 : 0x0600), M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BCC", (MomCPU == CPU5056) ? 0x06a0 : 0x0680, M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BGE", (MomCPU == CPU6051) ? 0x0660 : (MomCPU == CPU5056 ? 0x06a0 : 0x0680), M_5054 | M_5055 | M_5056 | M_6051, M_6051);
    AddRel("BGT", (MomCPU == CPU5056) ? 0x06e0 : 0x0620, M_5056 | M_6051, M_6051);
    AddRel("BLE", (MomCPU == CPU5056) ? 0x0660 : 0x06a0, M_5056 | M_6051, M_6051);
  }

  InstrZ = 0;
  AddCtrl("MATRIX", 0x3620, 0x0420, True,  M_5054 | M_5055 | M_6051);
  AddCtrl("FORMAT", 0x3630, 0x0430, True,  M_5054 | M_5055 | M_6051);
  AddCtrl("PAGE",   0x3650, 0x0450, False, M_5054 | M_5055 | M_6051);
  AddCtrl("ADRS",   0x3660, 0x0460, True,  M_5055 | M_6051);

  InstrZ = 0;
  AddDSP("DSP",   0x0800,  M_5054 | M_5055 | M_5056 | M_6051);
  AddDSP("DSPH",  0x0a00,  M_5055 | M_6051);
  AddDSP("DSPF",  0x0c00,  M_5054 | M_5055 | M_5056 | M_6051);
  AddDSP("DSPFH", 0x0e00,  M_5055 | M_6051);

  InstrZ = 0;
  AddONOFFInstr("LAMP",   0x0410, 0, M_5054 | M_5055 | M_6051);
  AddONOFFInstr("BACKUP", 0x0410, 2, M_5054 | M_5055 | M_6051);
  AddONOFFInstr("XTCP",   0x0480, 0, M_5055 | M_6051);

  InstrZ = 0;
  if (CheckCPU(M_5054 | M_5055 | M_5056))
  {
    AddAP("INC"     , 0x1810, M_5054 | M_5055 | M_5056);
    AddAP("DEC"     , 0x1a10, M_5054 | M_5055 | M_5056);
  }
  AddAP("ASR"     , 0x0030, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAP("ASL"     , 0x0230, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddAP("ROR"     , 0x0020, M_6051 | M_6052);
  AddAP("ROL"     , 0x0220, M_6051 | M_6052);
  AddAP("SWITCH"  , 0x3410, M_5054 | M_5055 | M_6051);
  AddAP("KSWITCH" , 0x3420, M_5054 | M_5055 | M_6051);
  AddAP("INTMODE" , 0x3440, M_5054 | M_5055 | M_6051);
  AddAP("RATE"    , 0x3490, M_5054 | M_5055 | M_6051);
  AddAP("ADC"     , 0x0050, M_5056 | M_6051 | M_6052);
  AddAP("SBC"     , 0x0250, M_5056 | M_6051 | M_6052);
  AddAP("STATUS"  , 0x34a0, M_6051);
  AddAP("FLAGIN"  , 0x34e0, M_6051);
  AddAP("S1RATE"  , 0x34c0, M_6051);
  AddAP("S2RATE"  , 0x34d0, M_6051);
  for (N = 0; N < 16; N++)
  {
    as_snprintf(Op, sizeof(Op), "ADC%d", N);
    AddAP(Op, 0x3000 | (N << 4), M_6051);
    as_snprintf(Op, sizeof(Op), "SBC%d", N);
    AddAP(Op, 0x3200 | (N << 4), M_6051);
  }

  InstrZ = 0;
  AddAPAX("CHG", 0x3800, 0x3800, M_5056 | M_6052);
  if (CheckCPU(M_6051))
  {
    AddAPAX("INC" , 0x3800, 0x3800, M_6051);
    AddAPAX("DEC" , 0x3a00, 0x3a00, M_6051);
  }

  InstrZ = 0;
  AddFixed("CLZ"     , 0x00a0, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("CLC"     , 0x0090, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("CLG"     , 0x0080, M_6051 | M_6052);
  AddFixed("CLA"     , 0x00b0, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("SEZ"     , 0x02a0, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("SEC"     , 0x0290, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("SEG"     , 0x0280, M_6051 | M_6052);
  AddFixed("SEA"     , 0x02b0, M_5054 | M_5055 | M_5056 | M_6052);
  AddFixed("BSO"     , 0x04c0, M_5054 | M_5055);
  AddFixed("HALT"    , (MomCPU == CPU6052) ? 0x0e10 : 0x0400, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("RSTRATE" , 0x0488, M_5054 | M_5055 | M_6051);
  AddFixed("NOP"     , 0x0000, M_5054 | M_5055 | M_5056 | M_6051 | M_6052);
  AddFixed("RET"     , 0x00c0, M_6051 | M_6052);
  AddFixed("RTI"     , 0x02c0, M_6051 | M_6052);
  AddFixed("MSO"     , 0x04a0, M_6051);
  AddFixed("ACTIVATE", 0x0490, M_6051);
  AddFixed("KENAB"   , 0x04e8, M_6051);
  AddFixed("KDSAB"   , 0x04e4, M_6051);
  AddFixed("STOP"    , 0x0e00, M_6052);
  AddFixed("ACT"     , 0x0e20, M_6052);
  AddFixed("EI"      , 0x0e68, M_6052);
  AddFixed("DI"      , 0x0e64, M_6052);
  AddFixed("ET"      , 0x0e62, M_6052);
  AddFixed("DT"      , 0x0e61, M_6052);
  AddFixed("EC"      , 0x0e78, M_6052);
  AddFixed("DC"      , 0x0e74, M_6052);
  AddFixed("OM"      , 0x0e72, M_6052);
  AddFixed("IM"      , 0x0e71, M_6052);
  AddFixed("RST"     , 0x0e90, M_6052);

  InstrZ = 0;
  AddImm("FREQ",  0x04d0, M_5055);
  AddImm("PITCH", 0x04c0, M_6051);

  InstrZ = 0;
  AddMem("RDAR",  0x3000, M_6052);
  AddMem("MVAR",  0x3400, M_6052);

  AddInstTable(InstTable, "ADJUST", 0, DecodeADJUST);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP);
  AddInstTable(InstTable, "CALL", 0, DecodeCALL);
  AddInstTable(InstTable, "MSA", 0, DecodeMSA);
  AddInstTable(InstTable, "JMPIO", 0x02d0, DecodeJMPIO);

  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_OLMS50);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
  AddInstTable(InstTable, "INTENAB", 0x0028, DecodeINTENDSAB);
  AddInstTable(InstTable, "INTDSAB", 0x0014, DecodeINTENDSAB);
  AddInstTable(InstTable, "INP", 0, DecodeINP);
  AddInstTable(InstTable, "IN" , 0, DecodeIN);
  AddInstTable(InstTable, "OUT", 0, DecodeOUT);

  AddInstTable(InstTable, "BUZZER", 0, DecodeBUZZER);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(AriOrders);
  order_array_free(APOrders);
  order_array_free(APAXOrders);
  order_array_free(FixedOrders);
  order_array_free(RelOrders);
  order_array_free(CtrlOrders);
  order_array_free(DSPOrders);
  order_array_free(ONOFFOrders);
  order_array_free(ImmOrders);
  order_array_free(MemOrders);
}

/*-------------------------------------------------------------------------*/

static void  MakeCode_OLMS50(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}


static Boolean IsDef_OLMS50(void)
{
  return False;
}

static void SwitchFrom_OLMS50(void)
{
  DeinitFields();
}

static void SwitchTo_OLMS50(void)
{
  static ASSUMERec ASSUMEs[] =
{
  { "P", &PRegAssume, 0, 0x3, 0x8, NULL }
};

  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("OLMS-50");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);
  SwitchIsOccupied = True;
  PageIsOccupied = True;

  PCSymbol = "$";
  HeaderID = pDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegCode] = 0;
  if (MomCPU == CPU5054)
  {
    CodeIntType = UInt10;
    DataIntType = UInt6;
    SegLimits[SegData] = 61;
    SegLimits[SegCode] = 1023;
  }
  else if (MomCPU == CPU5055)
  {
    CodeIntType = UInt11;
    DataIntType = UInt7;
    SegLimits[SegData] = 95;
    SegLimits[SegCode] = 1791;
  }
  else if (MomCPU == CPU5056)
  {
    CodeIntType = UInt11;
    DataIntType = UInt7;
    SegLimits[SegData] = 89;
    SegLimits[SegCode] = 1791;
  }
  else if (MomCPU == CPU6051)
  {
    CodeIntType = UInt12;
    DataIntType = UInt7;
    SegLimits[SegData] = 119;
    SegLimits[SegCode] = 2559;
  }
  else if (MomCPU == CPU6052)
  {
    CodeIntType = UInt11;
    DataIntType = UInt10;
    SegLimits[SegData] = 639;
    SegLimits[SegCode] = 2047;
  }

  MakeCode = MakeCode_OLMS50;
  IsDef = IsDef_OLMS50;
  SwitchFrom = SwitchFrom_OLMS50; InitFields();

  pASSUMERecs = ASSUMEs;
  ASSUMERecCnt = sizeof(ASSUMEs) / sizeof(*ASSUMEs);
}

void codeolms50_init(void)
{
  CPU5054 = AddCPU("MSM5054" , SwitchTo_OLMS50);
  CPU5055 = AddCPU("MSM5055" , SwitchTo_OLMS50);
  CPU5056 = AddCPU("MSM5056" , SwitchTo_OLMS50);
  CPU6051 = AddCPU("MSM6051" , SwitchTo_OLMS50);
  CPU6052 = AddCPU("MSM6052" , SwitchTo_OLMS50);
}
