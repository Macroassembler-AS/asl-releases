/* code6805.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 68(HC)05/08                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"

#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmitree.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code6805.h"

typedef struct
{
  CPUVar MinCPU;
  Byte Code;
} BaseOrder;

typedef struct
{
  CPUVar MinCPU;
  Byte Code;
  Word Mask;
  tSymbolSize Size;
} ALUOrder;

typedef struct
{
  CPUVar MinCPU;
  Byte Code;
  Word Mask;
} RMWOrder;

enum
{
  ModNone = -1,
  ModImm = 0,
  ModDir = 1,
  ModExt = 2,
  ModIx2 = 3,
  ModIx1 = 4,
  ModIx = 5,
  ModSP2 = 6,
  ModSP1 = 7,
  ModIxP = 8
};

#define MModImm (1 << ModImm)
#define MModDir (1 << ModDir)
#define MModExt (1 << ModExt)
#define MModIx2 (1 << ModIx2)
#define MModIx1 (1 << ModIx1)
#define MModIx  (1 << ModIx)
#define MModSP2 (1 << ModSP2)
#define MModSP1 (1 << ModSP1)
#define MModIxP (1 << ModIxP)
#define MMod05 (MModImm | MModDir | MModExt | MModIx2 | MModIx1 | MModIx)
#define MMod08 (MModSP2 | MModSP1 | MModIxP)

static ShortInt AdrMode;
static tSymbolSize OpSize;
static Byte AdrVals[2];

static IntType AdrIntType;

static CPUVar CPU6805, CPU68HC05, CPU68HC08, CPU68HCS08;

static BaseOrder *FixedOrders;
static BaseOrder *RelOrders;
static RMWOrder *RMWOrders;
static ALUOrder *ALUOrders;

/*--------------------------------------------------------------------------*/
/* address parser */


static unsigned ChkZero(char *s, Byte *pErg)
{
  if (*s == '<')
  {
    *pErg = 2;
    return 1;
  }
  else if (*s == '>')
  {
    *pErg = 1;
    return 1;
  }
  else
  {
    *pErg = 0;
    return 0;
  }
}

static void DecodeAdr(Byte Start, Byte Stop, Word Mask)
{
  Boolean OK;
  tSymbolFlags Flags;
  Word AdrWord, Mask08;
  Byte ZeroMode;
  unsigned Offset;
  ShortInt tmode1, tmode2;

  AdrMode = ModNone;
  AdrCnt = 0;

  Mask08 = Mask & MMod08;
  if (MomCPU == CPU6805)
    Mask &= MMod05;

  if (Stop - Start == 1)
  {
    if (!as_strcasecmp(ArgStr[Stop].str.p_str, "X"))
    {
      tmode1 = ModIx1;
      tmode2 = ModIx2;
    }
    else if (!as_strcasecmp(ArgStr[Stop].str.p_str,"SP"))
    {
      tmode1 = ModSP1;
      tmode2 = ModSP2;
      if (MomCPU < CPU68HC08)
      {
        WrStrErrorPos(ErrNum_InvReg, &ArgStr[Stop]);
        goto chk;
      }
    }
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[Stop]);
      goto chk;
    }

    Offset = ChkZero(ArgStr[Start].str.p_str, &ZeroMode);
    AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[Start], Offset, (ZeroMode == 2) ? Int8 : Int16, &OK, &Flags);

    if (OK)
    {
      if ((ZeroMode == 0) && (AdrWord == 0) && (Mask & MModIx) && (tmode1 == ModIx1))
        AdrMode = ModIx;

      else if (((Mask & (1 << tmode2)) == 0) || (ZeroMode == 2) || ((ZeroMode == 0) && (Hi(AdrWord) == 0)))
      {
        if (mFirstPassUnknown(Flags))
          AdrWord &= 0xff;
        if (Hi(AdrWord) != 0) WrError(ErrNum_NoShortAddr);
        else
        {
          AdrCnt = 1;
          AdrVals[0] = Lo(AdrWord);
          AdrMode = tmode1;
        }
      }

      else
      {
        AdrVals[0] = Hi(AdrWord);
        AdrVals[1] = Lo(AdrWord);
        AdrCnt = 2;
        AdrMode = tmode2;
      }
    }
  }

  else if (Stop == Start)
  {
    /* Postinkrement */

    if (!as_strcasecmp(ArgStr[Start].str.p_str, "X+"))
    {
      AdrMode = ModIxP;
      goto chk;
    }

    /* X-indirekt */

    if (!as_strcasecmp(ArgStr[Start].str.p_str, "X"))
    {
      WrError(ErrNum_ConvIndX);
      AdrMode = ModIx;
      goto chk;
    }

    /* immediate */

    if (*ArgStr[Start].str.p_str == '#')
    {
      switch (OpSize)
      {
        case eSymbolSizeUnknown:
          WrError(ErrNum_UndefOpSizes);
          break;
        case eSymbolSize8Bit:
          AdrVals[0] = EvalStrIntExpressionOffs(&ArgStr[Start], 1, Int8, &OK);
          if (OK)
          {
            AdrCnt = 1;
            AdrMode = ModImm;
          }
          break;
        case eSymbolSize16Bit:
          AdrWord = EvalStrIntExpressionOffs(&ArgStr[Start], 1, Int16, &OK);
          if (OK)
          {
            AdrVals[0] = Hi(AdrWord);
            AdrVals[1] = Lo(AdrWord);
            AdrCnt = 2;
            AdrMode = ModImm;
          }
          break;
        default:
          break;
      }
      goto chk;
    }

    /* absolut */

    Offset = ChkZero(ArgStr[Start].str.p_str, &ZeroMode);
    AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[Start], Offset, (ZeroMode == 2) ? UInt8 : AdrIntType, &OK, &Flags);

    if (OK)
    {
      if (((Mask & MModExt) == 0) || (ZeroMode == 2) || ((ZeroMode == 0) && (Hi(AdrWord) == 0)))
      {
        if (mFirstPassUnknown(Flags))
          AdrWord &= 0xff;
        if (Hi(AdrWord) != 0) WrError(ErrNum_NoShortAddr);
        else
        {
          AdrCnt = 1;
          AdrVals[0] = Lo(AdrWord);
          AdrMode = ModDir;
        }
      }
      else
      {
        AdrVals[0] = Hi(AdrWord);
        AdrVals[1] = Lo(AdrWord);
        AdrCnt = 2;
        AdrMode = ModExt;
      }
      goto chk;
    }
  }

  else
    (void)ChkArgCnt(Start, Start + 1);

chk:
  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    if ((1 << AdrMode) & Mask08)
      (void)ChkMinCPUExt(CPU68HC08, ErrNum_AddrModeNotSupported);
    else
      WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
    AdrCnt = 0;
  }
}

/*--------------------------------------------------------------------------*/
/* instruction parsers */

static void DecodeFixed(Word Index)
{
  const BaseOrder *pOrder = FixedOrders + Index;

  if (ChkArgCnt(0, 0)
   && ChkMinCPU(pOrder->MinCPU))
  {
    CodeLen = 1;
    BAsmCode[0] = pOrder->Code;
  }
}

static void DecodeMOV(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU68HC08))
  {
    OpSize = eSymbolSize8Bit;
    DecodeAdr(1, 1, MModImm | MModDir | MModIxP);
    switch (AdrMode)
    {
      case ModImm:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(2, 2, MModDir);
        if (AdrMode == ModDir)
        {
          BAsmCode[0] = 0x6e;
          BAsmCode[2] = AdrVals[0];
          CodeLen = 3;
        }
        break;
      case ModDir:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(2, 2, MModDir | MModIxP);
        switch (AdrMode)
        {
          case ModDir:
            BAsmCode[0] = 0x4e;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIxP:
            BAsmCode[0] = 0x5e;
            CodeLen = 2;
            break;
        }
        break;
      case ModIxP:
        DecodeAdr(2, 2, MModDir);
        if (AdrMode == ModDir)
        {
          BAsmCode[0] = 0x7e;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
        }
        break;
    }
  }
}

static void DecodeRel(Word Index)
{
  BaseOrder *pOrder = RelOrders + Index;
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrInt;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(pOrder->MinCPU))
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], AdrIntType, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = pOrder->Code;
        BAsmCode[1] = Lo(AdrInt);
      }
    }
  }
}

static void DecodeCBEQx(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrInt;

  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPU68HC08))
  {
    OpSize = eSymbolSize8Bit; DecodeAdr(1, 1, MModImm);
    if (AdrMode == ModImm)
    {
      BAsmCode[1] = AdrVals[0];
      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], AdrIntType, &OK, &Flags) - (EProgCounter() + 3);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0x41 | Index;
          BAsmCode[2] = AdrInt & 0xff;
          CodeLen = 3;
        }
      }
    }
  }
}

static void DecodeCBEQ(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrInt;
  Byte Disp = 0;

  UNUSED(Index);

  if (!ChkMinCPU(CPU68HC08));
  else if (ArgCnt == 2)
  {
    DecodeAdr(1, 1, MModDir | MModIxP);
    switch (AdrMode)
    {
      case ModDir:
        BAsmCode[0] = 0x31;
        BAsmCode[1] = AdrVals[0];
        Disp = 3;
        break;
      case ModIxP:
        BAsmCode[0]=0x71;
        Disp = 2;
        break;
    }
    if (AdrMode != ModNone)
    {
      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], AdrIntType, &OK, &Flags) - (EProgCounter() + Disp);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[Disp - 1] = AdrInt & 0xff;
          CodeLen = Disp;
        }
      }
    }
  }
  else if (ArgCnt == 3)
  {
    OK = True;
    if (!as_strcasecmp(ArgStr[2].str.p_str, "X+")) Disp = 3;
    else if (!as_strcasecmp(ArgStr[2].str.p_str,"SP"))
    {
      BAsmCode[0] = 0x9e;
      Disp = 4;
    }
    else
    {
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
      OK = False;
    }
    if (OK)
    {
      BAsmCode[Disp - 3] = 0x61;
      BAsmCode[Disp - 2] = EvalStrIntExpression(&ArgStr[1], UInt8, &OK);
      if (OK)
      {
        AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], AdrIntType, &OK, &Flags) - (EProgCounter() + Disp);
        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[Disp - 1] = AdrInt & 0xff;
            CodeLen = Disp;
          }
        }
      }
    }
  }
  else
    (void)ChkArgCnt(2, 3);
}

static void DecodeDBNZx(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrInt;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU68HC08))
  {
    AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], AdrIntType, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = 0x4b | Index;
        BAsmCode[1] = AdrInt & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeDBNZ(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrInt;
  Byte Disp = 0;

  UNUSED(Index);

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPU68HC08))
  {
    DecodeAdr(1, ArgCnt - 1, MModDir | MModIx | MModIx1 | MModSP1);
    switch (AdrMode)
    {
      case ModDir:
        BAsmCode[0] = 0x3b;
        BAsmCode[1] = AdrVals[0];
        Disp = 3;
        break;
      case ModIx:
        BAsmCode[0] = 0x7b;
        Disp = 2;
        break;
      case ModIx1:
        BAsmCode[0] = 0x6b;
        BAsmCode[1] = AdrVals[0];
        Disp = 3;
        break;
      case ModSP1:
        BAsmCode[0] = 0x9e;
        BAsmCode[1] = 0x6b;
        BAsmCode[2] = AdrVals[0];
        Disp = 4;
        break;
    }
    if (AdrMode != ModNone)
    {
      AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], AdrIntType, &OK, &Flags) - (EProgCounter() + Disp);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && ((AdrInt < -128) || (AdrInt > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[Disp - 1] = AdrInt & 0xff;
          CodeLen = Disp;
        }
      }
    }
  }
}

static void DecodeALU(Word Index)
{
  const ALUOrder *pOrder = ALUOrders + Index;

  if (ChkMinCPU(pOrder->MinCPU))
  {
    OpSize = pOrder->Size;
    DecodeAdr(1, ArgCnt, pOrder->Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModImm:
          BAsmCode[0] = 0xa0 + pOrder->Code;
          CodeLen = 1; /* leave for CPHX */
          break;
        case ModDir:
          BAsmCode[0] = 0xb0 + pOrder->Code;
          CodeLen = 1;
          break;
        case ModExt:
          BAsmCode[0] = 0xc0 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModIx:
          BAsmCode[0] = 0xf0 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModIx1:
          BAsmCode[0] = 0xe0 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModIx2:
          BAsmCode[0] = 0xd0 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModSP1:
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xe0| pOrder->Code;
          CodeLen = 2;
          break;
        case ModSP2:
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xd0 | pOrder->Code;
          CodeLen = 2;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeCPHX(Word Index)
{
  UNUSED(Index);

  if (ChkMinCPU(CPU68HC08))
  {
    Word Mask = MModImm | MModDir;

    if (MomCPU >= CPU68HCS08)
      Mask |= MModExt| MModSP1;
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, ArgCnt, Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModImm:
          BAsmCode[0] = 0x65;
          CodeLen = 1;
          break;
        case ModDir:
          BAsmCode[0] = 0x75;
          CodeLen = 1;
          break;
        case ModExt:
          BAsmCode[0] = 0x3e;
          CodeLen = 1;
          break;
        case ModSP1:
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xf3;
          CodeLen = 2;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeSTHX(Word Index)
{
  UNUSED(Index);

  if (ChkMinCPU(CPU68HC08))
  {
    Word Mask = MModDir;

    if (MomCPU >= CPU68HCS08)
      Mask |= MModExt| MModSP1;
    DecodeAdr(1, ArgCnt, Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModDir:
          BAsmCode[0] = 0x35;
          CodeLen = 1;
          break;
        case ModExt:
          BAsmCode[0] = 0x96;
          CodeLen = 1;
          break;
        case ModSP1:
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xff;
          CodeLen = 2;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeLDHX(Word Index)
{
  UNUSED(Index);

  if (ChkMinCPU(CPU68HC08))
  {
    Word Mask = MModImm | MModDir;

    if (MomCPU >= CPU68HCS08)
      Mask |= MModExt| MModIx | MModIx1 | MModIx2 | MModSP1;
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, ArgCnt, Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModImm:
          BAsmCode[0] = 0x45;
          CodeLen = 1;
          break;
        case ModDir:
          BAsmCode[0] = 0x55;
          CodeLen = 1;
          break;
        case ModExt:
          BAsmCode[0] = 0x32;
          CodeLen = 1;
          break;
        case ModIx  :
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xae;
          CodeLen = 2;
          break;
        case ModIx1 :
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xce;
          CodeLen = 2;
          break;
        case ModIx2 :
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xbe;
          CodeLen = 2;
          break;
        case ModSP1:
          BAsmCode[0] = 0x9e;
          BAsmCode[1] = 0xfe;
          CodeLen = 2;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeAIx(Word Index)
{
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU68HC08));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, SInt8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0xa7 | Index;
      CodeLen = 2;
    }
  }
}

static void DecodeRMW(Word Index)
{
  const RMWOrder *pOrder = RMWOrders + Index;

  if (ChkMinCPU(pOrder->MinCPU))
  {
    DecodeAdr(1, ArgCnt, pOrder->Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModDir :
          BAsmCode[0] = 0x30 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModIx  :
          BAsmCode[0] = 0x70 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModIx1 :
          BAsmCode[0] = 0x60 | pOrder->Code;
          CodeLen = 1;
          break;
        case ModSP1 :
          BAsmCode[0] = 0x9e; BAsmCode[1] = 0x60 | pOrder->Code;
          CodeLen = 2;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void decode_bset_bclr_core(Word code, int arg_index)
{
  Boolean ok;

  BAsmCode[1] = EvalStrIntExpression(&ArgStr[arg_index], Int8, &ok);
  if (ok)
  {
    BAsmCode[0] = 0x10 | code;
    CodeLen = 2;
  }
}

static void decode_bset_bclr_1(Word code)
{
  if (ChkArgCnt(1, 1))
    decode_bset_bclr_core(code, 1);
}

static void decode_bset_bclr_2(Word code)
{
  if (ChkArgCnt(2, 2))
  {
    Boolean ok;

    code |= EvalStrIntExpression(&ArgStr[1], UInt3, &ok) << 1;
    if (ok)
      decode_bset_bclr_core(code, 2);
  }
}

static void decode_brset_brclr_core(Word code, int arg_index)
{
  Boolean ok;

  BAsmCode[1] = EvalStrIntExpression(&ArgStr[arg_index], Int8, &ok);
  if (ok)
  {
    tSymbolFlags flags;
    LongInt address;

    address = EvalStrIntExpressionWithFlags(&ArgStr[arg_index + 1], AdrIntType, &ok, &flags) - (EProgCounter() + 3);
    if (ok)
    {
      if (!mSymbolQuestionable(flags) && ((address < -128) || (address > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 3;
        BAsmCode[0] = code;
        BAsmCode[2] = Lo(address);
      }
    }
  }
}

static void decode_brset_brclr_2(Word code)
{
  if (ChkArgCnt(2, 2))
    decode_brset_brclr_core(code, 1);
}

static void decode_brset_brclr_3(Word code)
{
  if (ChkArgCnt(3, 3))
  {
    Boolean ok;

    code |= EvalStrIntExpression(&ArgStr[1], UInt3, &ok) << 1;
    if (ok)
      decode_brset_brclr_core(code, 2);
  }
}

/*--------------------------------------------------------------------------*/
/* dynamic code table handling */

static void AddFixed(const char *NName, CPUVar NMin, Byte NCode)
{
  order_array_rsv_end(FixedOrders, BaseOrder);
  FixedOrders[InstrZ].MinCPU = NMin;
  FixedOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddRel(const char *NName, CPUVar NMin, Byte NCode)
{
  order_array_rsv_end(RelOrders, BaseOrder);
  RelOrders[InstrZ].MinCPU = NMin;
  RelOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRel);
}

static void AddALU(const char *NName, CPUVar NMin, Byte NCode, Word NMask, tSymbolSize NSize)
{
  order_array_rsv_end(ALUOrders, ALUOrder);
  ALUOrders[InstrZ].MinCPU = NMin;
  ALUOrders[InstrZ].Code = NCode;
  ALUOrders[InstrZ].Mask = NMask;
  ALUOrders[InstrZ].Size = NSize;
  AddInstTable(InstTable, NName, InstrZ++, DecodeALU);
}

static void AddRMW(const char *NName, CPUVar NMin, Byte NCode ,Word NMask)
{
  order_array_rsv_end(RMWOrders, RMWOrder);
  RMWOrders[InstrZ].MinCPU = NMin;
  RMWOrders[InstrZ].Code = NCode;
  RMWOrders[InstrZ].Mask = NMask;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRMW);
}

static void add_bset_bclr(const char *p_name, Word code)
{
  char name[10];
  unsigned bit;

  AddInstTable(InstTable, p_name, code, decode_bset_bclr_2);
  for (bit = 0; bit < 8; bit++)
  {
    as_snprintf(name, sizeof(name), "%s%c", p_name, bit + '0');
    AddInstTable(InstTable, name, code | (bit << 1), decode_bset_bclr_1);
  }
}

static void add_brset_brclr(const char *p_name, Word code)
{
  char name[10];
  unsigned bit;

  AddInstTable(InstTable, p_name, code, decode_brset_brclr_3);
  for (bit = 0; bit < 8; bit++)
  {
    as_snprintf(name, sizeof(name), "%s%c", p_name, bit + '0');
    AddInstTable(InstTable, name, code | (bit << 1), decode_brset_brclr_2);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(247);
  SetDynamicInstTable(InstTable);

  InstrZ = 0;
  AddFixed("RTI" , CPU6805   , 0x80); AddFixed("RTS" , CPU6805   , 0x81);
  AddFixed("SWI" , CPU6805   , 0x83); AddFixed("TAX" , CPU6805   , 0x97);
  AddFixed("CLC" , CPU6805   , 0x98); AddFixed("SEC" , CPU6805   , 0x99);
  AddFixed("CLI" , CPU6805   , 0x9a); AddFixed("SEI" , CPU6805   , 0x9b);
  AddFixed("RSP" , CPU6805   , 0x9c); AddFixed("NOP" , CPU6805   , 0x9d);
  AddFixed("TXA" , CPU6805   , 0x9f); AddFixed("NEGA", CPU6805   , 0x40);
  AddFixed("NEGX", CPU6805   , 0x50); AddFixed("COMA", CPU6805   , 0x43);
  AddFixed("COMX", CPU6805   , 0x53); AddFixed("LSRA", CPU6805   , 0x44);
  AddFixed("LSRX", CPU6805   , 0x54); AddFixed("RORA", CPU6805   , 0x46);
  AddFixed("RORX", CPU6805   , 0x56); AddFixed("ASRA", CPU6805   , 0x47);
  AddFixed("ASRX", CPU6805   , 0x57); AddFixed("ASLA", CPU6805   , 0x48);
  AddFixed("ASLX", CPU6805   , 0x58); AddFixed("LSLA", CPU6805   , 0x48);
  AddFixed("LSLX", CPU6805   , 0x58); AddFixed("ROLA", CPU6805   , 0x49);
  AddFixed("ROLX", CPU6805   , 0x59); AddFixed("DECA", CPU6805   , 0x4a);
  AddFixed("DECX", CPU6805   , 0x5a); AddFixed("INCA", CPU6805   , 0x4c);
  AddFixed("INCX", CPU6805   , 0x5c); AddFixed("TSTA", CPU6805   , 0x4d);
  AddFixed("TSTX", CPU6805   , 0x5d); AddFixed("CLRA", CPU6805   , 0x4f);
  AddFixed("CLRX", CPU6805   , 0x5f); AddFixed("CLRH", CPU68HC08 , 0x8c);
  AddFixed("DAA" , CPU68HC08 , 0x72); AddFixed("DIV" , CPU68HC08 , 0x52);
  AddFixed("MUL" , CPU68HC05 , 0x42); AddFixed("NSA" , CPU68HC08 , 0x62);
  AddFixed("PSHA", CPU68HC08 , 0x87); AddFixed("PSHH", CPU68HC08 , 0x8b);
  AddFixed("PSHX", CPU68HC08 , 0x89); AddFixed("PULA", CPU68HC08 , 0x86);
  AddFixed("PULH", CPU68HC08 , 0x8a); AddFixed("PULX", CPU68HC08 , 0x88);
  AddFixed("STOP", CPU68HC05 , 0x8e); AddFixed("TAP" , CPU68HC08 , 0x84);
  AddFixed("TPA" , CPU68HC08 , 0x85); AddFixed("TSX" , CPU68HC08 , 0x95);
  AddFixed("TXS" , CPU68HC08 , 0x94); AddFixed("WAIT", CPU68HC05 , 0x8f);
  AddFixed("INX" , CPU6805   , 0x5c); AddFixed("BGND", CPU68HCS08, 0x82);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);

  InstrZ = 0;
  AddRel("BRA" , CPU6805   , 0x20); AddRel("BRN" , CPU6805   , 0x21);
  AddRel("BHI" , CPU6805   , 0x22); AddRel("BLS" , CPU6805   , 0x23);
  AddRel("BCC" , CPU6805   , 0x24); AddRel("BCS" , CPU6805   , 0x25);
  AddRel("BNE" , CPU6805   , 0x26); AddRel("BEQ" , CPU6805   , 0x27);
  AddRel("BHCC", CPU6805   , 0x28); AddRel("BHCS", CPU6805   , 0x29);
  AddRel("BPL" , CPU6805   , 0x2a); AddRel("BMI" , CPU6805   , 0x2b);
  AddRel("BMC" , CPU6805   , 0x2c); AddRel("BMS" , CPU6805   , 0x2d);
  AddRel("BIL" , CPU6805   , 0x2e); AddRel("BIH" , CPU6805   , 0x2f);
  AddRel("BSR" , CPU6805   , 0xad); AddRel("BGE" , CPU68HC08 , 0x90);
  AddRel("BGT" , CPU68HC08 , 0x92); AddRel("BHS" , CPU6805   , 0x24);
  AddRel("BLE" , CPU68HC08 , 0x93); AddRel("BLO" , CPU6805   , 0x25);
  AddRel("BLT" , CPU68HC08 , 0x91);

  AddInstTable(InstTable, "CBEQA", 0x00, DecodeCBEQx);
  AddInstTable(InstTable, "CBEQX", 0x10, DecodeCBEQx);
  AddInstTable(InstTable, "CBEQ", 0, DecodeCBEQ);
  AddInstTable(InstTable, "DBNZA", 0x00, DecodeDBNZx);
  AddInstTable(InstTable, "DBNZX", 0x10, DecodeDBNZx);
  AddInstTable(InstTable, "DBNZ", 0, DecodeDBNZ);

  InstrZ = 0;
  AddALU("SUB" , CPU6805   , 0x00, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("CMP" , CPU6805   , 0x01, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("SBC" , CPU6805   , 0x02, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("CPX" , CPU6805   , 0x03, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("AND" , CPU6805   , 0x04, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("BIT" , CPU6805   , 0x05, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("LDA" , CPU6805   , 0x06, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("STA" , CPU6805   , 0x07,           MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("EOR" , CPU6805   , 0x08, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("ADC" , CPU6805   , 0x09, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("ORA" , CPU6805   , 0x0a, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("ADD" , CPU6805   , 0x0b, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("JMP" , CPU6805   , 0x0c,           MModDir | MModExt | MModIx | MModIx1 | MModIx2                    , eSymbolSizeUnknown);
  AddALU("JSR" , CPU6805   , 0x0d,           MModDir | MModExt | MModIx | MModIx1 | MModIx2                    , eSymbolSizeUnknown);
  AddALU("LDX" , CPU6805   , 0x0e, MModImm | MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);
  AddALU("STX" , CPU6805   , 0x0f,           MModDir | MModExt | MModIx | MModIx1 | MModIx2 | MModSP1 | MModSP2, eSymbolSize8Bit);

  AddInstTable(InstTable, "CPHX", 0, DecodeCPHX);
  AddInstTable(InstTable, "LDHX", 0, DecodeLDHX);
  AddInstTable(InstTable, "STHX", 0, DecodeSTHX);

  AddInstTable(InstTable, "AIS", 0x00, DecodeAIx);
  AddInstTable(InstTable, "AIX", 0x08, DecodeAIx);

  InstrZ = 0;
  AddRMW("NEG", CPU6805, 0x00, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("COM", CPU6805, 0x03, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("LSR", CPU6805, 0x04, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("ROR", CPU6805, 0x06, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("ASR", CPU6805, 0x07, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("ASL", CPU6805, 0x08, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("LSL", CPU6805, 0x08, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("ROL", CPU6805, 0x09, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("DEC", CPU6805, 0x0a, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("INC", CPU6805, 0x0c, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("TST", CPU6805, 0x0d, MModDir |        MModIx | MModIx1 |         MModSP1        );
  AddRMW("CLR", CPU6805, 0x0f, MModDir |        MModIx | MModIx1 |         MModSP1        );

  add_bset_bclr("BCLR", 0x01);
  add_bset_bclr("BSET", 0x00);
  add_brset_brclr("BRCLR", 0x01);
  add_brset_brclr("BRSET", 0x00);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(RelOrders);
  order_array_free(ALUOrders);
  order_array_free(RMWOrders);
}

/*--------------------------------------------------------------------------*/
/* Main Functions */

static Boolean DecodeAttrPart_6805(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  /* deduce operand size - no size is zero-length string -> '\0' */

  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_6805(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = AttrPartOpSize[0];

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize, True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_6805(void)
{
  return False;
}

static void SwitchFrom_6805(void)
{
  DeinitFields();
}

static void SwitchTo_6805(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x62;
  NOPCode = 0x9d;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = (MomCPU >= CPU68HC08) ? 0xffff : 0x1fff;
  AdrIntType = (MomCPU >= CPU68HC08) ? UInt16 : UInt13;

  DecodeAttrPart = DecodeAttrPart_6805;
  MakeCode = MakeCode_6805;
  IsDef = IsDef_6805;
  SwitchFrom = SwitchFrom_6805;
  InitFields();
  AddMoto16PseudoONOFF(False);
}

void code6805_init(void)
{
  CPU6805    = AddCPU("6805",    SwitchTo_6805);
  CPU68HC05  = AddCPU("68HC05",  SwitchTo_6805);
  CPU68HC08  = AddCPU("68HC08",  SwitchTo_6805);
  CPU68HCS08 = AddCPU("68HCS08", SwitchTo_6805);
}
