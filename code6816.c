/* code6816.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegeneratormodul CPU16                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <string.h>
#include <ctype.h>

#include "nls.h"
#include "nlmessages.h"
#include "as.rsc"
#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "errmsg.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"

#include "code6816.h"

/*---------------------------------------------------------------------------*/

typedef struct
{
  tSymbolSize Size;
  Word Code, ExtCode;
  Byte AdrMask, ExtShift;
} GenOrder;

typedef struct
{
  Word Code1, Code2;
} EmuOrder;

#define RegCnt 7

enum
{
  ModNone = -1,
  ModDisp8 = 0,
  ModDisp16 = 1,
  ModDispE = 2,
  ModAbs = 3,
  ModImm = 4,
  ModImmExt = 5,
  ModDisp20 = ModDisp16,
  ModAbs20 = ModAbs
};

#define MModDisp8 (1 << ModDisp8)
#define MModDisp16 (1 << ModDisp16)
#define MModDispE (1 << ModDispE)
#define MModAbs (1 << ModAbs)
#define MModImm (1 << ModImm)
#define MModImmExt (1 << ModImmExt)
#define MModDisp20 MModDisp16
#define MModAbs20 MModAbs

static tSymbolSize OpSize;
static ShortInt AdrMode;
static Byte AdrPart;
static Byte AdrVals[4];

static LongInt Reg_EK;

static GenOrder *GenOrders;
static EmuOrder *EmuOrders;
static const char **Regs;

static CPUVar CPU6816;

/*-------------------------------------------------------------------------*/

typedef enum
{
  ShortDisp,
  LongDisp,
  NoDisp
} DispType;

static unsigned SplitSize(char *Asc, DispType *pErg)
{
  if (strlen(Asc) < 1)
  {
    *pErg = NoDisp;
    return 0;
  }
  else if (*Asc == '>')
  {
    *pErg = LongDisp;
    return 1;
  }
  else if (*Asc == '<')
  {
    *pErg = ShortDisp;
    return 1;
  }
  else
  {
    *pErg = NoDisp;
    return 0;
  }
}

static void DecodeAdr(int Start, int Stop, Boolean LongAdr, Byte Mask)
{
  Integer V16;
  LongInt V32;
  Boolean OK;
  unsigned Offset;
  DispType Size;

  AdrMode = ModNone;
  AdrCnt = 0;

  Stop -= Start - 1;
  if (Stop < 1)
  {
    char Str[100];

    as_snprintf(Str, sizeof(Str), getmessage(Num_ErrMsgAddrArgCnt), 1, 2, Stop - Start + 1);
    WrXError(ErrNum_WrongArgCnt, Str);
    return;
  }

  /* immediate ? */

  if (*ArgStr[Start].str.p_str == '#')
  {
    Offset = SplitSize(ArgStr[Start].str.p_str + 1, &Size);
    switch (OpSize)
    {
      case eSymbolSizeUnknown:
        WrError(ErrNum_UndefOpSizes);
        break;
      case eSymbolSize8Bit:
        AdrVals[0] = EvalStrIntExpressionOffs(&ArgStr[Start], 1 + Offset, Int8, &OK);
        if (OK)
        {
          AdrCnt = 1;
          AdrMode = ModImm;
        }
        break;
      case eSymbolSize16Bit:
        V16 = EvalStrIntExpressionOffs(&ArgStr[Start], 1 + Offset, (Size == ShortDisp) ? SInt8 : Int16, &OK);
        if ((Size == NoDisp) && (V16 >= -128) && (V16 <= 127) && ((Mask & MModImmExt) != 0))
          Size = ShortDisp;
        if (OK)
        {
          if (Size == ShortDisp)
          {
            AdrVals[0] = Lo(V16);
            AdrCnt = 1;
            AdrMode = ModImmExt;
          }
          else
          {
            AdrVals[0] = Hi(V16);
            AdrVals[1] = Lo(V16);
            AdrCnt = 2;
            AdrMode = ModImm;
          }
        }
        break;
      case eSymbolSize32Bit:
        V32 = EvalStrIntExpressionOffs(&ArgStr[Start], 1 + Offset, Int32, &OK);
        if (OK)
        {
          AdrVals[0] = (V32 >> 24) & 0xff;
          AdrVals[1] = (V32 >> 16) & 0xff;
          AdrVals[2] = (V32 >>  8) & 0xff;
          AdrVals[3] = V32 & 0xff;
          AdrCnt = 4;
          AdrMode = ModImm;
        }
        break;
      default:
        break;
    }
    goto chk;
  }

  /* zusammengesetzt ? */

  if (Stop == 2)
  {
    AdrPart = 0xff;
    if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "X"))
      AdrPart = 0x00;
    else if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "Y"))
      AdrPart = 0x10;
    else if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "Z"))
      AdrPart = 0x20;
    else
      WrStrErrorPos(ErrNum_InvReg, &ArgStr[Start + 1]);
    if (AdrPart != 0xff)
    {
      if (!as_strcasecmp(ArgStr[Start].str.p_str, "E"))
        AdrMode = ModDispE;
      else
      {
        Offset = SplitSize(ArgStr[Start].str.p_str, &Size);
        if (Size == ShortDisp)
          V32 = EvalStrIntExpressionOffs(&ArgStr[Start], Offset, UInt8, &OK);
        else if (LongAdr)
          V32 = EvalStrIntExpressionOffs(&ArgStr[Start], Offset, SInt20, &OK);
        else
          V32 = EvalStrIntExpressionOffs(&ArgStr[Start], Offset, SInt16, &OK);
        if (OK)
        {
          if (Size == NoDisp)
          {
            if ((V32 >= 0) && (V32 <= 255) && ((Mask & MModDisp8) != 0))
              Size = ShortDisp;
          }
          if (Size == ShortDisp)
          {
            AdrVals[0] = V32 & 0xff;
            AdrCnt = 1;
            AdrMode = ModDisp8;
          }
          else if (LongAdr)
          {
            AdrVals[0] = (V32 >> 16) & 0x0f;
            AdrVals[1] = (V32 >> 8) & 0xff;
            AdrVals[2] = V32 & 0xff;
            AdrCnt = 3;
            AdrMode = ModDisp16;
          }
          else
          {
            AdrVals[0] = (V32 >> 8) & 0xff;
            AdrVals[1] = V32 & 0xff;
            AdrCnt = 2;
            AdrMode = ModDisp16;
          }
        }
      }
    }
    goto chk;
  }

   /* absolut ? */

  else
  {
    Offset = SplitSize(ArgStr[Start].str.p_str, &Size);
    V32 = EvalStrIntExpressionOffs(&ArgStr[Start], Offset, UInt20, &OK);
    if (OK)
    {
      if (LongAdr)
      {
        AdrVals[0] = (V32 >> 16) & 0xff;
        AdrVals[1] = (V32 >> 8) & 0xff;
        AdrVals[2] = V32 & 0xff;
        AdrMode = ModAbs;
        AdrCnt = 3;
      }
      else
      {
        if ((V32 >> 16) != Reg_EK) WrError(ErrNum_InAccPage);
        AdrVals[0] = (V32 >> 8) & 0xff;
        AdrVals[1] = V32 & 0xff;
        AdrMode = ModAbs;
        AdrCnt = 2;
      }
    }
    goto chk;
  }

chk:
  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
     AdrCnt = 0;
  }
}

/*-------------------------------------------------------------------------*/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
    CodeLen = 2;
  }
}

static void DecodeEmu(Word Index)
{
  EmuOrder *pOrder = EmuOrders + Index;

  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Hi(pOrder->Code1);
    BAsmCode[1] = Lo(pOrder->Code1);
    BAsmCode[2] = Hi(pOrder->Code2);
    BAsmCode[3] = Lo(pOrder->Code2);
    CodeLen = 4;
  }
}

static void DecodeRel(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrLong;

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags) - EProgCounter() - 6;
    if (AdrLong & 1) WrError(ErrNum_NotAligned);
    else if (Code & 0x8000)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrLong > 0x7fffl) || (AdrLong < -0x8000l))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = 0x37;
        BAsmCode[1] = Lo(Code) + 0x80;
        BAsmCode[2] = (AdrLong >> 8) & 0xff;
        BAsmCode[3] = AdrLong & 0xff;
        CodeLen = 4;
      }
    }
    else
    {
      if (!mSymbolQuestionable(Flags) && ((AdrLong > 0x7fl) || (AdrLong < -0x80l))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = 0xb0 + Lo(Code);
        BAsmCode[1] = AdrLong & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeLRel(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrLong;

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags) - EProgCounter() - 6;
    if (AdrLong & 1) WrError(ErrNum_NotAligned);
    else if (!mSymbolQuestionable(Flags) && ((AdrLong > 0x7fffl) || (AdrLong < -0x8000l))) WrError(ErrNum_JmpDistTooBig);
    else
    {
      BAsmCode[0] = Hi(Code);
      BAsmCode[1] = Lo(Code);
      BAsmCode[2] = (AdrLong >> 8) & 0xff;
      BAsmCode[3] = AdrLong & 0xff;
      CodeLen = 4;
    }
  }
}

static void DecodeGen(Word Index)
{
  GenOrder *pOrder = GenOrders + Index;

  if (ChkArgCnt(1, 2))
  {
    OpSize = pOrder->Size;
    DecodeAdr(1, ArgCnt, False, pOrder->AdrMask);
    switch (AdrMode)
    {
      case ModDisp8:
        BAsmCode[0] = pOrder->Code + AdrPart;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModDisp16:
        BAsmCode[0] = 0x17 + pOrder->ExtShift;
        BAsmCode[1] = pOrder->Code + (OpSize << 6) + AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
      case ModDispE:
        BAsmCode[0] = 0x27;
        BAsmCode[1] = pOrder->Code + AdrPart;
        CodeLen = 2;
        break;
      case ModAbs:
        BAsmCode[0] = 0x17 + pOrder->ExtShift;
        BAsmCode[1] = pOrder->Code + (OpSize << 6) + 0x30;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
      case ModImm:
        if (OpSize == eSymbolSize8Bit)
        {
          BAsmCode[0] = pOrder->Code + 0x30;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
        }
        else
        {
          BAsmCode[0] = 0x37;
          BAsmCode[1] = pOrder->Code + 0x30;
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          CodeLen = 2 + AdrCnt;
        }
        break;
      case ModImmExt:
        BAsmCode[0] = pOrder->ExtCode;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeAux(Word Code)
{
  if (ChkArgCnt(1, 2))
  {
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, ArgCnt, False, (*OpPart.str.p_str == 'S' ? 0 : MModImm) | MModDisp8 | MModDisp16 | MModAbs);
    switch (AdrMode)
    {
      case ModDisp8:
        BAsmCode[0] = Code + AdrPart;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
      case ModDisp16:
        BAsmCode[0] = 0x17;
        BAsmCode[1] = Code + AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
      case ModAbs:
        BAsmCode[0] = 0x17;
        BAsmCode[1] = Code + 0x30;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
      case ModImm:
        BAsmCode[0] = 0x37;
        BAsmCode[1] = Code + 0x30;
        if (*OpPart.str.p_str == 'L') BAsmCode[1] -= 0x40;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
       break;
    }
  }
}

static void DecodeImm(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, 1, False, MModImm | MModImmExt);
    switch (AdrMode)
    {
      case ModImm:
        BAsmCode[0] = 0x37;
        BAsmCode[1] = Code;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
      case ModImmExt:
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeExt(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, 1, False, MModAbs);
    switch (AdrMode)
    {
      case ModAbs:
       BAsmCode[0] = Hi(Code);
       BAsmCode[1] = Lo(Code);
       memcpy(BAsmCode + 2, AdrVals, AdrCnt);
       CodeLen = 2 + AdrCnt;
       break;
    }
  }
}

static void DecodeStkMult(Word Index)
{
  int z, z2;
  Boolean OK;

  if (ChkArgCnt(1, ArgCntMax))
  {
    OK = True;
    BAsmCode[1] = 0;
    for (z = 1; z <= ArgCnt; z++)
      if (OK)
      {
        z2 = 0;
        NLS_UpString(ArgStr[z].str.p_str);
        while ((z2 < RegCnt) && (strcmp(ArgStr[z].str.p_str, Regs[z2])))
          z2++;
        if (z2 >= RegCnt)
        {
          WrStrErrorPos(ErrNum_InvReg, &ArgStr[z]);
          OK = False;
        }
        else
          BAsmCode[1] |= Index ? (1 << z2) : (1 << (RegCnt - 1 - z2));
      }
    if (OK)
    {
      BAsmCode[0] = 0x34 | (Index ^ 1);
      CodeLen = 2;
    }
  }
}

static void DecodeMov(Word Index)
{
  Boolean OK;

  if (ArgCnt == 2)
  {
    DecodeAdr(1, 1, False, MModAbs);
    if (AdrMode == ModAbs)
    {
      memcpy(BAsmCode + 2, AdrVals, 2);
      DecodeAdr(2, 2, False, MModAbs);
      if (AdrMode == ModAbs)
      {
        memcpy(BAsmCode + 4, AdrVals, 2);
        BAsmCode[0] = 0x37;
        BAsmCode[1] = 0xfe | Index;
        CodeLen = 6;
      }
    }
  }
  else if (!ChkArgCnt(3, 3));
  else if (!as_strcasecmp(ArgStr[2].str.p_str, "X"))
  {
    BAsmCode[1] = EvalStrIntExpression(&ArgStr[1], SInt8, &OK);
    if (OK)
    {
      DecodeAdr(3, 3, False, MModAbs);
      if (AdrMode == ModAbs)
      {
        memcpy(BAsmCode + 2, AdrVals, 2);
        BAsmCode[0] = 0x30 | Index;
        CodeLen = 4;
      }
    }
  }
  else if (!as_strcasecmp(ArgStr[3].str.p_str, "X"))
  {
    BAsmCode[3] = EvalStrIntExpression(&ArgStr[2], SInt8, &OK);
    if (OK)
    {
      DecodeAdr(1, 1, False, MModAbs);
      if (AdrMode == ModAbs)
      {
        memcpy(BAsmCode + 1, AdrVals, 2);
        BAsmCode[0] = 0x32 | Index;
        CodeLen = 4;
      }
    }
  }
  else WrError(ErrNum_InvAddrMode);
}

static void DecodeLogp(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    OpSize = eSymbolSize16Bit; DecodeAdr(1, 1, False, MModImm);
    switch (AdrMode)
    {
      case ModImm:
        BAsmCode[0] = 0x37;
        BAsmCode[1] = 0x3a | Index;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
    }
  }
}

static void DecodeMac(Word Index)
{
  Byte Val;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    Val = EvalStrIntExpression(&ArgStr[1], UInt4, &OK);
    if (OK)
    {
      BAsmCode[1] = EvalStrIntExpression(&ArgStr[2], UInt4, &OK);
      if (OK)
      {
        BAsmCode[1] |= (Val << 4);
        BAsmCode[0] = 0x7b | Index;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeBit8(Word Index)
{
  Byte Mask;

  if (ChkArgCnt(2, 3))
  {
    OpSize = eSymbolSize8Bit;
    DecodeAdr(ArgCnt, ArgCnt, False, MModImm);
    switch (AdrMode)
    {
      case ModImm:
        Mask = AdrVals[0];
        DecodeAdr(1, ArgCnt - 1, False, MModDisp8 | MModDisp16 | MModAbs);
        switch (AdrMode)
        {
          case ModDisp8:
            BAsmCode[0] = 0x17;
            BAsmCode[1] = 0x08 | Index | AdrPart;
            BAsmCode[2] = Mask;
            BAsmCode[3] = AdrVals[0];
            CodeLen = 4;
            break;
          case ModDisp16:
            BAsmCode[0] = 0x08 | Index | AdrPart;
            BAsmCode[1] = Mask;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
          case ModAbs:
            BAsmCode[0] = 0x38 + Index;
            BAsmCode[1] = Mask;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
    }
  }
}

static void DecodeBit16(Word Index)
{
  if (ChkArgCnt(2, 3))
  {
    OpSize = eSymbolSize16Bit;
    DecodeAdr(ArgCnt, ArgCnt, False, MModImm);
    switch (AdrMode)
    {
      case ModImm:
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        DecodeAdr(1, ArgCnt - 1, False, MModDisp16 | MModAbs);
        switch (AdrMode)
        {
          case ModDisp16:
            BAsmCode[0] = 0x27;
            BAsmCode[1] = 0x08 | Index | AdrPart;
            memcpy(BAsmCode + 4, AdrVals, AdrCnt);
            CodeLen = 4 + AdrCnt;
            break;
          case ModAbs:
            BAsmCode[0] = 0x27;
            BAsmCode[1] = 0x38 | Index;
            memcpy(BAsmCode + 4, AdrVals, AdrCnt);
            CodeLen = 4 + AdrCnt;
            break;
        }
    }
  }
}

static void DecodeBrBit(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrLong;

  if (ChkArgCnt(3, 4))
  {
    OpSize = eSymbolSize8Bit; DecodeAdr(ArgCnt - 1, ArgCnt - 1, False, MModImm);
    if (AdrMode == ModImm)
    {
      BAsmCode[1] = AdrVals[0];
      AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], UInt20, &OK, &Flags) - EProgCounter() - 6;
      if (OK)
      {
        DecodeAdr(1, ArgCnt - 2, False, MModDisp8 | MModDisp16 | MModAbs);
        switch (AdrMode)
        {
          case ModDisp8:
            if ((AdrLong >= -128) && (AdrLong < 127))
            {
              BAsmCode[0] = 0xcb - (Index << 6) + AdrPart;
              BAsmCode[2] = AdrVals[0];
              BAsmCode[3] = AdrLong & 0xff;
              CodeLen = 4;
            }
            else if (!mSymbolQuestionable(Flags) && ((AdrLong <- 0x8000l) || (AdrLong > 0x7fffl))) WrError(ErrNum_JmpDistTooBig);
            else
            {
              BAsmCode[0] = 0x0a + AdrPart + Index;
              BAsmCode[2] = 0;
              BAsmCode[3] = AdrVals[0];
              BAsmCode[4] = (AdrLong >> 8) & 0xff;
              BAsmCode[5] = AdrLong & 0xff;
              CodeLen = 6;
            }
            break;
          case ModDisp16:
            if (!mSymbolQuestionable(Flags) && ((AdrLong < -0x8000l) || (AdrLong > 0x7fffl))) WrError(ErrNum_JmpDistTooBig);
            else
            {
              BAsmCode[0] = 0x0a + AdrPart + Index;
              memcpy(BAsmCode + 2,AdrVals, 2);
              BAsmCode[4] = (AdrLong >> 8) & 0xff;
              BAsmCode[5] = AdrLong & 0xff;
              CodeLen = 6;
            }
            break;
          case ModAbs:
            if (!mSymbolQuestionable(Flags) && ((AdrLong < -0x8000l) || (AdrLong > 0x7fffl))) WrError(ErrNum_JmpDistTooBig);
            else
            {
              BAsmCode[0] = 0x3a | Index;
              memcpy(BAsmCode + 2, AdrVals, 2);
              BAsmCode[4] = (AdrLong >> 8) & 0xff;
              BAsmCode[5] = AdrLong & 0xff;
              CodeLen = 6;
            }
            break;
        }
      }
    }
  }
}

static void DecodeJmpJsr(Word Index)
{
  if (ChkArgCnt(1, 2))
  {
    OpSize = eSymbolSize16Bit;
    DecodeAdr(1, ArgCnt, True, MModAbs20 | MModDisp20);
    switch (AdrMode)
    {
      case ModAbs20:
        BAsmCode[0] = 0x7a + (Index << 7);
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
        break;
      case ModDisp20:
        BAsmCode[0] = Index ? 0x89 : 0x4b;
        BAsmCode[0] += AdrPart;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
        break;
    }
  }
}

static void DecodeBsr(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt AdrLong;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags) - EProgCounter() - 6;
    if (AdrLong & 1) WrError(ErrNum_NotAligned);
    else if (!mSymbolQuestionable(Flags) & ((AdrLong > 0x7fl) || (AdrLong < -0x80l))) WrError(ErrNum_JmpDistTooBig);
    else
    {
      BAsmCode[0] = 0x36;
      BAsmCode[1] = AdrLong & 0xff;
      CodeLen = 2;
    }
  }
}

/*-------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddRel(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel);
}

static void AddLRel(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLRel);
}

static void AddGen(const char *NName, tSymbolSize NSize, Word NCode, Word NExtCode, Byte NShift, Byte NMask)
{
  order_array_rsv_end(GenOrders, GenOrder);
  GenOrders[InstrZ].Code = NCode;
  GenOrders[InstrZ].ExtCode = NExtCode;
  GenOrders[InstrZ].Size = NSize;
  GenOrders[InstrZ].AdrMask = NMask;
  GenOrders[InstrZ].ExtShift = NShift;
  AddInstTable(InstTable, NName, InstrZ++, DecodeGen);
}

static void AddAux(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAux);
}

static void AddImm(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeImm);
}

static void AddExt(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeExt);
}

static void AddEmu(const char *NName, Word NCode1, Word NCode2)
{
  order_array_rsv_end(EmuOrders, EmuOrder);
  EmuOrders[InstrZ].Code1 = NCode1;
  EmuOrders[InstrZ].Code2 = NCode2;
  AddInstTable(InstTable, NName, InstrZ++, DecodeEmu);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(405);

  AddFixed("ABA"   , 0x370b); AddFixed("ABX"   , 0x374f);
  AddFixed("ABY"   , 0x375f); AddFixed("ABZ"   , 0x376f);
  AddFixed("ACE"   , 0x3722); AddFixed("ACED"  , 0x3723);
  AddFixed("ADE"   , 0x2778); AddFixed("ADX"   , 0x37cd);
  AddFixed("ADY"   , 0x37dd); AddFixed("ADZ"   , 0x37ed);
  AddFixed("AEX"   , 0x374d); AddFixed("AEY"   , 0x375d);
  AddFixed("AEZ"   , 0x376d); AddFixed("ASLA"  , 0x3704);
  AddFixed("ASLB"  , 0x3714); AddFixed("ASLD"  , 0x27f4);
  AddFixed("ASLE"  , 0x2774); AddFixed("ASLM"  , 0x27b6);
  AddFixed("LSLB"  , 0x3714); AddFixed("LSLD"  , 0x27f4);
  AddFixed("LSLE"  , 0x2774); AddFixed("LSLA"  , 0x3704);
  AddFixed("ASRA"  , 0x370d); AddFixed("ASRB"  , 0x371d);
  AddFixed("ASRD"  , 0x27fd); AddFixed("ASRE"  , 0x277d);
  AddFixed("ASRM"  , 0x27ba); AddFixed("BGND"  , 0x37a6);
  AddFixed("CBA"   , 0x371b); AddFixed("CLRA"  , 0x3705);
  AddFixed("CLRB"  , 0x3715); AddFixed("CLRD"  , 0x27f5);
  AddFixed("CLRE"  , 0x2775); AddFixed("CLRM"  , 0x27b7);
  AddFixed("COMA"  , 0x3700); AddFixed("COMB"  , 0x3710);
  AddFixed("COMD"  , 0x27f0); AddFixed("COME"  , 0x2770);
  AddFixed("DAA"   , 0x3721); AddFixed("DECA"  , 0x3701);
  AddFixed("DECB"  , 0x3711); AddFixed("EDIV"  , 0x3728);
  AddFixed("EDIVS" , 0x3729); AddFixed("EMUL"  , 0x3725);
  AddFixed("EMULS" , 0x3726); AddFixed("FDIV"  , 0x372b);
  AddFixed("FMULS" , 0x3727); AddFixed("IDIV"  , 0x372a);
  AddFixed("INCA"  , 0x3703); AddFixed("INCB"  , 0x3713);
  AddFixed("LPSTOP", 0x27f1); AddFixed("LSRA"  , 0x370f);
  AddFixed("LSRB"  , 0x371f); AddFixed("LSRD"  , 0x27ff);
  AddFixed("LSRE"  , 0x277f); AddFixed("MUL"   , 0x3724);
  AddFixed("NEGA"  , 0x3702); AddFixed("NEGB"  , 0x3712);
  AddFixed("NEGD"  , 0x27f2); AddFixed("NEGE"  , 0x2772);
  AddFixed("NOP"   , 0x274c); AddFixed("PSHA"  , 0x3708);
  AddFixed("PSHB"  , 0x3718); AddFixed("PSHMAC", 0x27b8);
  AddFixed("PULA"  , 0x3709); AddFixed("PULB"  , 0x3719);
  AddFixed("PULMAC", 0x27b9); AddFixed("ROLA"  , 0x370c);
  AddFixed("ROLB"  , 0x371c); AddFixed("ROLD"  , 0x27fc);
  AddFixed("ROLE"  , 0x277c); AddFixed("RORA"  , 0x370e);
  AddFixed("RORB"  , 0x371e); AddFixed("RORD"  , 0x27fe);
  AddFixed("RORE"  , 0x277e); AddFixed("RTI"   , 0x2777);
  AddFixed("RTS"   , 0x27f7); AddFixed("SBA"   , 0x370a);
  AddFixed("SDE"   , 0x2779); AddFixed("SWI"   , 0x3720);
  AddFixed("SXT"   , 0x27f8); AddFixed("TAB"   , 0x3717);
  AddFixed("TAP"   , 0x37fd); AddFixed("TBA"   , 0x3707);
  AddFixed("TBEK"  , 0x27fa); AddFixed("TBSK"  , 0x379f);
  AddFixed("TBXK"  , 0x379c); AddFixed("TBYK"  , 0x379d);
  AddFixed("TBZK"  , 0x379e); AddFixed("TDE"   , 0x277b);
  AddFixed("TDMSK" , 0x372f); AddFixed("TDP"   , 0x372d);
  AddFixed("TED"   , 0x27fb); AddFixed("TEDM"  , 0x27b1);
  AddFixed("TEKB"  , 0x27bb); AddFixed("TEM"   , 0x27b2);
  AddFixed("TMER"  , 0x27b4); AddFixed("TMET"  , 0x27b5);
  AddFixed("TMXED" , 0x27b3); AddFixed("TPA"   , 0x37fc);
  AddFixed("TPD"   , 0x372c); AddFixed("TSKB"  , 0x37af);
  AddFixed("TSTA"  , 0x3706); AddFixed("TSTB"  , 0x3716);
  AddFixed("TSTD"  , 0x27f6); AddFixed("TSTE"  , 0x2776);
  AddFixed("TSX"   , 0x274f); AddFixed("TSY"   , 0x275f);
  AddFixed("TSZ"   , 0x276f); AddFixed("TXKB"  , 0x37ac);
  AddFixed("TXS"   , 0x374e); AddFixed("TXY"   , 0x275c);
  AddFixed("TXZ"   , 0x276c); AddFixed("TYKB"  , 0x37ad);
  AddFixed("TYS"   , 0x375e); AddFixed("TYX"   , 0x274d);
  AddFixed("TYZ"   , 0x276d); AddFixed("TZKB"  , 0x37ae);
  AddFixed("TZS"   , 0x376e); AddFixed("TZX"   , 0x274e);
  AddFixed("TZY"   , 0x275e); AddFixed("WAI"   , 0x27f3);
  AddFixed("XGAB"  , 0x371a); AddFixed("XGDE"  , 0x277a);
  AddFixed("XGDX"  , 0x37cc); AddFixed("XGDY"  , 0x37dc);
  AddFixed("XGDZ"  , 0x37ec); AddFixed("XGEX"  , 0x374c);
  AddFixed("XGEY"  , 0x375c); AddFixed("XGEZ"  , 0x376c);
  AddFixed("DES"   , 0x3fff); AddFixed("INS"   , 0x3f01);
  AddFixed("DEX"   , 0x3cff); AddFixed("INX"   , 0x3c01);
  AddFixed("DEY"   , 0x3dff); AddFixed("INY"   , 0x3d01);
  AddFixed("PSHX"  , 0x3404); AddFixed("PULX"  , 0x3510);
  AddFixed("PSHY"  , 0x3408); AddFixed("PULY"  , 0x3508);

  AddRel("BCC" ,           4); AddRel("BCS",            5); AddRel("BEQ",            7);
  AddRel("BGE" ,          12); AddRel("BGT",           14); AddRel("BHI",            2);
  AddRel("BLE" ,          15); AddRel("BLS",            3); AddRel("BLT",           13);
  AddRel("BMI" ,          11); AddRel("BNE",            6); AddRel("BPL",           10);
  AddRel("BRA" ,           0); AddRel("BRN",            1); AddRel("BVC",            8);
  AddRel("BVS" ,           9); AddRel("BHS",            4); AddRel("BLO",            5);
  AddRel("LBCC", 0x8000 |  4); AddRel("LBCS", 0x8000 |  5); AddRel("LBEQ", 0x8000 |  7);
  AddRel("LBGE", 0x8000 | 12); AddRel("LBGT", 0x8000 | 14); AddRel("LBHI", 0x8000 |  2);
  AddRel("LBLE", 0x8000 | 15); AddRel("LBLS", 0x8000 |  3); AddRel("LBLT", 0x8000 | 13);
  AddRel("LBMI", 0x8000 | 11); AddRel("LBNE", 0x8000 |  6); AddRel("LBPL", 0x8000 | 10);
  AddRel("LBRA", 0x8000 |  0); AddRel("LBRN", 0x8000 |  1); AddRel("LBVC", 0x8000 |  8);
  AddRel("LBVS", 0x8000 |  9); AddRel("LBHS", 0x8000 |  4); AddRel("LBLO", 0x8000 |  5);

  AddLRel("LBEV", 0x3791); AddLRel("LBMV", 0x3790); AddLRel("LBSR", 0x27f9);

  InstrZ = 0;
  AddGen("ADCA", eSymbolSize8Bit , 0x43, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ADCB", eSymbolSize8Bit , 0xc3, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ADCD", eSymbolSize16Bit, 0x83, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ADCE", eSymbolSize16Bit, 0x03, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("ADDA", eSymbolSize8Bit , 0x41, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ADDB", eSymbolSize8Bit , 0xc1, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ADDD", eSymbolSize16Bit, 0x81,   0xfc, 0x20, MModDisp8 | MModImm | MModImmExt | MModDisp16 | MModAbs | MModDispE);
  AddGen("ADDE", eSymbolSize16Bit, 0x01,   0x7c, 0x20,             MModImm | MModImmExt | MModDisp16 | MModAbs            );
  AddGen("ANDA", eSymbolSize8Bit , 0x46, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ANDB", eSymbolSize8Bit , 0xc6, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ANDD", eSymbolSize16Bit, 0x86, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ANDE", eSymbolSize16Bit, 0x06, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("ASL" , eSymbolSize8Bit , 0x04, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("ASLW", eSymbolSize8Bit , 0x04, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("LSL" , eSymbolSize8Bit , 0x04, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("LSLW", eSymbolSize8Bit , 0x04, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("ASR" , eSymbolSize8Bit , 0x0d, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("ASRW", eSymbolSize8Bit , 0x0d, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("BITA", eSymbolSize8Bit , 0x49, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("BITB", eSymbolSize8Bit , 0xc9, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("CLR" , eSymbolSize8Bit , 0x05, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("CLRW", eSymbolSize8Bit , 0x05, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("CMPA", eSymbolSize8Bit , 0x48, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("CMPB", eSymbolSize8Bit , 0xc8, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("COM" , eSymbolSize8Bit , 0x00, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("COMW", eSymbolSize8Bit , 0x00, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("CPD" , eSymbolSize16Bit, 0x88, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("CPE" , eSymbolSize16Bit, 0x08, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("DEC" , eSymbolSize8Bit , 0x01, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("DECW", eSymbolSize8Bit , 0x01, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("EORA", eSymbolSize8Bit , 0x44, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("EORB", eSymbolSize8Bit , 0xc4, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("EORD", eSymbolSize16Bit, 0x84, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("EORE", eSymbolSize16Bit, 0x04, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("INC" , eSymbolSize8Bit , 0x03, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("INCW", eSymbolSize8Bit , 0x03, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("LDAA", eSymbolSize8Bit , 0x45, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("LDAB", eSymbolSize8Bit , 0xc5, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("LDD" , eSymbolSize16Bit, 0x85, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("LDE" , eSymbolSize16Bit, 0x05, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("LSR" , eSymbolSize8Bit , 0x0f, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("LSRW", eSymbolSize8Bit , 0x0f, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("NEG" , eSymbolSize8Bit , 0x02, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("NEGW", eSymbolSize8Bit , 0x02, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("ORAA", eSymbolSize8Bit , 0x47, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ORAB", eSymbolSize8Bit , 0xc7, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ORD" , eSymbolSize16Bit, 0x87, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("ORE" , eSymbolSize16Bit, 0x07, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("ROL" , eSymbolSize8Bit , 0x0c, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("ROLW", eSymbolSize8Bit , 0x0c, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("ROR" , eSymbolSize8Bit , 0x0e, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("RORW", eSymbolSize8Bit , 0x0e, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );
  AddGen("SBCA", eSymbolSize8Bit , 0x42, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SBCB", eSymbolSize8Bit , 0xc2, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SBCD", eSymbolSize16Bit, 0x82, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SBCE", eSymbolSize16Bit, 0x02, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("STAA", eSymbolSize8Bit , 0x4a, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs | MModDispE);
  AddGen("STAB", eSymbolSize8Bit , 0xca, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs | MModDispE);
  AddGen("STD" , eSymbolSize16Bit, 0x8a, 0xffff, 0x20, MModDisp8 |                        MModDisp16 | MModAbs | MModDispE);
  AddGen("STE" , eSymbolSize16Bit, 0x0a, 0xffff, 0x20,                                    MModDisp16 | MModAbs            );
  AddGen("SUBA", eSymbolSize8Bit , 0x40, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SUBB", eSymbolSize8Bit , 0xc0, 0xffff, 0x00, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SUBD", eSymbolSize16Bit, 0x80, 0xffff, 0x20, MModDisp8 | MModImm |              MModDisp16 | MModAbs | MModDispE);
  AddGen("SUBE", eSymbolSize16Bit, 0x00, 0xffff, 0x20,             MModImm |              MModDisp16 | MModAbs            );
  AddGen("TST" , eSymbolSize8Bit , 0x06, 0xffff, 0x00, MModDisp8 |                        MModDisp16 | MModAbs            );
  AddGen("TSTW", eSymbolSize8Bit , 0x06, 0xffff, 0x10,                                    MModDisp16 | MModAbs            );

  AddAux("CPS", 0x4f); AddAux("CPX", 0x4c); AddAux("CPY", 0x4d); AddAux("CPZ", 0x4e);
  AddAux("LDS", 0xcf); AddAux("LDX", 0xcc); AddAux("LDY", 0xcd); AddAux("LDZ", 0xce);
  AddAux("STS", 0x8f); AddAux("STX", 0x8c); AddAux("STY", 0x8d); AddAux("STZ", 0x8e);

  AddImm("AIS", 0x3f); AddImm("AIX", 0x3c); AddImm("AIY", 0x3d); AddImm("AIZ", 0x3e);

  AddExt("LDED", 0x2771); AddExt("LDHI", 0x27b0); AddExt("STED", 0x2773);

  InstrZ = 0;
  AddEmu("CLC", 0x373a, 0xfeff); AddEmu("CLI", 0x373a, 0xff1f); AddEmu("CLV", 0x373a, 0xfdff);
  AddEmu("SEC", 0x373b, 0x0100); AddEmu("SEI", 0x373b, 0x00e0); AddEmu("SEV", 0x373b, 0x0200);

  AddInstTable(InstTable, "PSHM", 1, DecodeStkMult);
  AddInstTable(InstTable, "PULM", 0, DecodeStkMult);

  AddInstTable(InstTable, "MOVB", 0, DecodeMov);
  AddInstTable(InstTable, "MOVW", 1, DecodeMov);

  AddInstTable(InstTable, "ANDP", 0, DecodeLogp);
  AddInstTable(InstTable, "ORP" , 1, DecodeLogp);

  AddInstTable(InstTable, "MAC" , 0 << 7, DecodeMac);
  AddInstTable(InstTable, "RMAC", 1 << 7, DecodeMac);

  AddInstTable(InstTable, "BCLR", 0, DecodeBit8);
  AddInstTable(InstTable, "BSET", 1, DecodeBit8);

  AddInstTable(InstTable, "BCLRW", 0, DecodeBit16);
  AddInstTable(InstTable, "BSETW", 1, DecodeBit16);

  AddInstTable(InstTable, "BRCLR", 0, DecodeBrBit);
  AddInstTable(InstTable, "BRSET", 1, DecodeBrBit);

  AddInstTable(InstTable, "JMP", 0, DecodeJmpJsr);
  AddInstTable(InstTable, "JSR", 1, DecodeJmpJsr);
  AddInstTable(InstTable, "BSR", 0, DecodeBsr);

  AddInstTable(InstTable, "DB", 0, DecodeMotoBYT);
  AddInstTable(InstTable, "DW", 0, DecodeMotoADR);

  Regs = (const char **) malloc(sizeof(char *) * RegCnt);
  Regs[0] = "D"; Regs[1] = "E"; Regs[2] = "X"; Regs[3] = "Y";
  Regs[4] = "Z"; Regs[5] = "K"; Regs[6] = "CCR";

  init_moto8_pseudo(InstTable, e_moto_8_be);
}

static void DeinitFields(void)
{
  order_array_free(GenOrders);
  order_array_free(EmuOrders);
  DestroyInstTable(InstTable);
  free(Regs);
}

/*-------------------------------------------------------------------------*/

static Boolean DecodeAttrPart_6816(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }
  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_6816(void)
{
  CodeLen = 0;
  DontPrint = False;
  AdrCnt = 0;

  /* Operandengroesse festlegen. ACHTUNG! Das gilt nur fuer die folgenden
     Pseudobefehle! Die Maschinenbefehle ueberschreiben diesen Wert! */

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

static void InitCode_6816(void)
{
  Reg_EK = 0;
}

static Boolean IsDef_6816(void)
{
  return False;
}

static void SwitchFrom_6816(void)
{
  DeinitFields();
}

static void SwitchTo_6816(void)
{
#define ASSUME6816Count (sizeof(ASSUME6816s) / sizeof(*ASSUME6816s))
  static ASSUMERec ASSUME6816s[11] =
  {
    { "EK" , &Reg_EK , 0 , 0xff , 0x100, NULL }
  };

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x65;
  NOPCode = 0x274c;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfffffl;

  DecodeAttrPart = DecodeAttrPart_6816;
  MakeCode = MakeCode_6816;
  IsDef = IsDef_6816;
  SwitchFrom = SwitchFrom_6816;
  AddMoto16PseudoONOFF(False);

  pASSUMERecs = ASSUME6816s;
  ASSUMERecCnt = ASSUME6816Count;

  InitFields();
}

void code6816_init(void)
{
  CPU6816 = AddCPU("68HC16", SwitchTo_6816);

  AddInitPassProc(InitCode_6816);
}
