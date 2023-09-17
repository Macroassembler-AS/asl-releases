/* code6812.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegeneratormodul CPU12                                                  */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>

#include "strutil.h"
#include "bpemu.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmallg.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code6812.h"

typedef struct
{
  Word Code;
  CPUVar MinCPU;
} FixedOrder;

typedef struct
{
  Word Code;
  CPUVar MinCPU;
  Boolean MayImm, MayDir, MayExt;
  tSymbolSize ThisOpSize;
} GenOrder;

typedef struct
{
  Word Code;
  Boolean MayDir;
} JmpOrder;

typedef struct
{
  Word Code;
  Byte OpSize;
  CPUVar MinCPU;
} Reg;

enum
{
  eShortModeAuto = 0,
  eShortModeNo = 1,
  eShortModeYes = 2,
  eShortModeExtreme = 3
};

enum
{
  ModNone = -1,
  ModImm = 0,
  ModDir = 1,
  ModExt = 2,
  ModIdx = 3,
  ModIdx1 = 4,
  ModIdx2 = 5,
  ModDIdx = 6,
  ModIIdx2 = 7,
  ModExtPg = 8
};

#define MModImm (1 << ModImm)
#define MModDir (1 << ModDir)
#define MModExt (1 << ModExt)
#define MModIdx (1 << ModIdx)
#define MModIdx1 (1 << ModIdx1)
#define MModIdx2 (1 << ModIdx2)
#define MModDIdx (1 << ModDIdx)
#define MModIIdx2 (1 << ModIIdx2)
#define MModExtPg (1 << ModExtPg)

#define MModAllIdx (MModIdx | MModIdx1 | MModIdx2 | MModDIdx | MModIIdx2)

static tSymbolSize OpSize;
static ShortInt AdrMode;
static ShortInt ExPos;
static Byte AdrVals[4];
static Byte ActReg, ActRegSize;
static CPUVar CPU6812, CPU6812X;
static IntType AddrInt;

static LongInt Reg_Direct, Reg_GPage;

static FixedOrder *FixedOrders;
static FixedOrder *BranchOrders;
static GenOrder *GenOrders;
static FixedOrder *LoopOrders;
static FixedOrder *LEAOrders;
static JmpOrder *JmpOrders;
static Reg *Regs;

static PInstTable RegTable;

/*---------------------------------------------------------------------------*/
/* Address Expression Decoder */

enum
{
  eRegA = 0,
  eRegB = 1,
  eRegCCRL = 2,
  eRegD = 4,
  eRegX = 5,
  eRegY = 6,
  eRegSP = 7,
  eRegPC = 8,
  eRegHalf = 0x40,
  eRegUpper = 0x80,
  eRegWord = 0xc0,
  eRegXL = eRegHalf | eRegX,
  eRegXH = eRegHalf | eRegUpper | eRegX,
  eRegYL = eRegHalf | eRegY,
  eRegYH = eRegHalf | eRegUpper | eRegY,
  eRegSPL = eRegHalf | eRegSP,
  eRegSPH = eRegHalf | eRegUpper | eRegSP,
  eRegCCRH = eRegCCRL | eRegUpper,
  eRegCCRW = eRegCCRL | eRegWord,
  eNoReg = 0xff
};

static Boolean DecodeReg(const char *pAsc, Byte *pErg)
{
  Boolean Result;
  String Reg;

  strmaxcpy(Reg, pAsc, STRINGSIZE);
  UpString(Reg);
  Result = LookupInstTable(RegTable, Reg) && (ActReg != eNoReg);

  if (Result)
    *pErg = ActReg;

  return Result;
}

enum
{
  eBaseRegX = 0,
  eBaseRegY = 1,
  eBaseRegSP = 2,
  eBaseRegPC = 3
};

static Boolean DecodeBaseReg(const char *pAsc, Byte *pErg)
{
  Boolean Result = DecodeReg(pAsc, pErg);

  if (Result)
  {
    switch (*pErg)
    {
      case eRegX:
        *pErg = eBaseRegX;
        break;
      case eRegY:
        *pErg = eBaseRegY;
        break;
      case eRegSP:
        *pErg = eBaseRegSP;
        break;
      case eRegPC:
        *pErg = eBaseRegPC;
        break;
      default:
        Result = FALSE;
    }
  }

  return Result;
}

static Boolean ValidReg(const char *Asc_o)
{
  Byte Dummy;
  String Asc;
  int l = strlen(Asc_o);

  if ((*Asc_o == '-') || (*Asc_o == '+'))
    strcpy(Asc, Asc_o + 1);
  else
  {
    strcpy(Asc, Asc_o);
    if ((l > 0) && ((Asc_o[l - 1] == '-') || (Asc_o[l - 1] == '+')))
      Asc[l - 1] = '\0';
  }
  return DecodeBaseReg(Asc, &Dummy);
}

enum
{
  eIdxRegA = 0,
  eIdxRegB = 1,
  eIdxRegD = 2
};

static Boolean DecodeIdxReg(const char *pAsc, Byte *pErg)
{
  Boolean Result = DecodeReg(pAsc, pErg);

  if (Result)
  {
    switch (*pErg)
    {
      case eRegA:
        *pErg = eIdxRegA;
        break;
      case eRegB:
        *pErg = eIdxRegB;
        break;
      case eRegD:
        *pErg = eIdxRegD;
        break;
      default:
        Result = FALSE;
    }
  }

  return Result;
}

static Boolean ChkRegPair(Byte SrcReg, Byte DestReg, Byte *pExtMask)
{
  Boolean Result = TRUE;

  switch (SrcReg)
  {
    case eRegA:
      if ((DestReg <= eRegCCRL) || ((DestReg >= eRegD) && (DestReg <= eRegSP)))
        *pExtMask = 0;
      else if ((DestReg == eRegCCRH) || (DestReg == eRegXH) || (DestReg == eRegYH) || (DestReg == eRegSPH))
        *pExtMask = 8;
      else
        Result = False;
      break;

    case eRegB:
      if ((DestReg <= eRegCCRL) || ((DestReg >= eRegD) && (DestReg <= eRegSP)))
        *pExtMask = 0;
      else if ((DestReg == eRegXL) || (DestReg == eRegYL) || (DestReg == eRegSPL))
        *pExtMask = 8;
      else
        Result = False;
      break;

    case eRegCCRL:
      if ((DestReg <= eRegCCRL) || ((DestReg >= eRegD) && (DestReg <= eRegSP)))
        *pExtMask = 0;
      else
        Result = False;
      break;

    case eRegD:
    case eRegX:
    case eRegY:
    case eRegSP:
      if ((DestReg <= eRegCCRL) || ((DestReg >= eRegD) && (DestReg <= eRegSP)))
        *pExtMask = 0;
      else if (DestReg == eRegCCRW)
        *pExtMask = 8;
      else
        Result = False;
      break;

    case eRegXL:
    case eRegYL:
    case eRegSPL:
      if (DestReg <= eRegCCRL)
        *pExtMask = 0;
      else
        Result = False;
      break;

    case eRegXH:
    case eRegYH:
    case eRegSPH:
    case eRegCCRH:
      if (DestReg == eRegA)
        *pExtMask = 8;
      else
        Result = False;
      break;

    case eRegCCRW:
      if ((DestReg == eRegCCRW) || ((DestReg >= eRegD) && (DestReg <= eRegSP)))
        *pExtMask = 8;
      else
        Result = False;
      break;

    default:
      Result = False;
  }

  if ((Result) && (*pExtMask) && (MomCPU < CPU6812X))
    Result = FALSE;

  return Result;
}

static void CutShort(char *Asc, Integer *ShortMode)
{
  if (*Asc == '>')
  {
    *ShortMode = eShortModeNo;
    strmov(Asc, Asc + 1);
  }
  else if (*Asc == '<')
  {
    if (Asc[1] == '<')
    {
      *ShortMode = eShortModeExtreme;
      strmov(Asc, Asc + 2);
    }
    else
    {
      *ShortMode = eShortModeYes;
      strmov(Asc,Asc + 1);
    }
  }
  else
    *ShortMode = eShortModeAuto;
}

static Boolean DistFits(Byte Reg, Integer Dist, Integer Offs, LongInt Min, LongInt Max, tSymbolFlags Flags)
{
  if (Reg == eBaseRegPC)
    Dist -= Offs;
  return (((Dist >= Min) && (Dist <= Max)) || ((Reg == eBaseRegPC) && mSymbolQuestionable(Flags)));
}

static void DecodeAdr(int Start, int Stop, Word Mask)
{
  Integer ShortMode;
  LongInt AdrWord;
  int l;
  char *p;
  Boolean OK;
  tSymbolFlags Flags;
  Boolean DecFlag, AutoFlag, PostFlag;

  AdrMode = ModNone;
  AdrCnt = 0;

  /* one argument? */

  if (Stop - Start == 0)
  {
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
            AdrVals[0] = AdrWord >> 8;
            AdrVals[1] = AdrWord & 0xff;
            AdrCnt = 2;
            AdrMode = ModImm;
          }
          break;
        default:
          break;
      }
      goto chk;
    }

    /* indirekt */

    if ((*ArgStr[Start].str.p_str == '[') && (ArgStr[Start].str.p_str[strlen(ArgStr[Start].str.p_str) - 1] == ']'))
    {
      strmov(ArgStr[Start].str.p_str, ArgStr[Start].str.p_str + 1);
      ArgStr[Start].str.p_str[strlen(ArgStr[Start].str.p_str) - 1] = '\0';
      p = QuotPos(ArgStr[Start].str.p_str, ',');
      if (p)
        *p = '\0';
      if (!p) WrError(ErrNum_InvAddrMode);
      else if (!DecodeBaseReg(p + 1, AdrVals))
        WrXError(ErrNum_InvReg, p + 1);
      else if (!as_strcasecmp(ArgStr[Start].str.p_str, "D"))
      {
        AdrVals[0] = (AdrVals[0] << 3) | 0xe7;
        AdrCnt = 1;
        AdrMode = ModDIdx;
      }
      else
      {
        AdrWord = EvalStrIntExpression(&ArgStr[Start], Int16, &OK);
        if (OK)
        {
          if (AdrVals[0] == eBaseRegPC)
            AdrWord -= EProgCounter() + ExPos + 3;
          AdrVals[0] = (AdrVals[0] << 3) | 0xe3;
          AdrVals[1] = AdrWord >> 8;
          AdrVals[2] = AdrWord & 0xff;
          AdrCnt = 3;
          AdrMode = ModIIdx2;
        }
      }
      goto chk;
    }

    /* dann absolut */

    CutShort(ArgStr[Start].str.p_str, &ShortMode);
    AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[Start], AddrInt, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
    {
      if ((!(Mask & (MModExt | MModExtPg))) || (ShortMode == eShortModeYes))
        AdrWord = (Reg_Direct << 8) | Lo(AdrWord);
    }

    if (OK)
    {
      if ((ShortMode != eShortModeNo)
       && (Hi(AdrWord) == Reg_Direct)
       && ((Mask & MModDir) != 0))
      {
        AdrMode = ModDir;
        AdrVals[0] = AdrWord & 0xff;
        AdrCnt = 1;
      }
      else
      {
        AdrMode = ModExt;
        AdrVals[0] = Hi(AdrWord);
        AdrVals[1] = Lo(AdrWord);
        if (Mask & MModExtPg)
        {
          Mask |= MModExt;
          if ((HiWord(EProgCounter()) != HiWord(AdrWord))
           && ((AdrWord & 0xc000) == 0x8000)
           && ((EProgCounter() & 0xc000) == 0x8000)
           && !mFirstPassUnknown(Flags))
            WrError(ErrNum_PageCrossing);
        }
        AdrCnt = 2;
      }
    }
    goto chk;
  }

  /* two arguments? */

  else if (Stop - Start == 1)
  {
    /* Autoin/-dekrement abspalten */

    l = strlen(ArgStr[Stop].str.p_str);
    if ((*ArgStr[Stop].str.p_str == '-') || (*ArgStr[Stop].str.p_str == '+'))
    {
      DecFlag = (*ArgStr[Stop].str.p_str == '-');
      AutoFlag = True;
      PostFlag = False;
      strmov(ArgStr[Stop].str.p_str, ArgStr[Stop].str.p_str + 1);
    }
    else if ((ArgStr[Stop].str.p_str[l - 1] == '-') || (ArgStr[Stop].str.p_str[l - 1] == '+'))
    {
      DecFlag = (ArgStr[Stop].str.p_str[l - 1] == '-');
      AutoFlag = True;
      PostFlag = True;
      ArgStr[Stop].str.p_str[l - 1] = '\0';
    }
    else
      AutoFlag = DecFlag = PostFlag = False;

    if (AutoFlag)
    {
      if (!DecodeBaseReg(ArgStr[Stop].str.p_str, AdrVals)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Stop]);
      else if (AdrVals[0] == eBaseRegPC) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Stop]);
      else
      {
        AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[Start], SInt8, &OK, &Flags);
        if (mFirstPassUnknown(Flags))
          AdrWord = 1;

        /* no increment/decrement degenerates to register indirect with zero displacement! */

        if (AdrWord == 0)
        {
          AdrVals[0] = (AdrVals[0] << 6);
          AdrCnt = 1;
          AdrMode = ModIdx;
        }
        else if (AdrWord > 8) WrError(ErrNum_OverRange);
        else if (AdrWord < -8) WrError(ErrNum_UnderRange);
        else
        {
          if (AdrWord < 0)
          {
            DecFlag = !DecFlag;
            AdrWord = (-AdrWord);
          }
          AdrWord = DecFlag ? 8 - AdrWord : AdrWord - 1;
          AdrVals[0] = (AdrVals[0] << 6) | 0x20 | (Ord(PostFlag) << 4) | (Ord(DecFlag) << 3) | (AdrWord & 7);
          AdrCnt = 1;
          AdrMode = ModIdx;
        }
      }
      goto chk;
    }

    else
    {
      if (!DecodeBaseReg(ArgStr[Stop].str.p_str, AdrVals)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[Stop]);
      else if (DecodeIdxReg(ArgStr[Start].str.p_str, AdrVals + 1))
      {
        AdrVals[0] = (AdrVals[0] << 3) | AdrVals[1] | 0xe4;
        AdrCnt = 1;
        AdrMode = ModIdx;
      }
      else
      {
        CutShort(ArgStr[Start].str.p_str, &ShortMode);
        AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[Start], Int16, &OK, &Flags);
        if (AdrVals[0] == eBaseRegPC)
          AdrWord -= EProgCounter() + ExPos;
        if (OK)
        {
          if ((ShortMode != eShortModeNo) && (ShortMode != eShortModeYes) && ((Mask & MModIdx) != 0) && (DistFits(AdrVals[0], AdrWord, 1, -16, 15, Flags)))
          {
            if (AdrVals[0] == eBaseRegPC)
              AdrWord--;
            AdrVals[0] = (AdrVals[0] << 6) | (AdrWord & 0x1f);
            AdrCnt = 1;
            AdrMode = ModIdx;
          }
          else if ((ShortMode != eShortModeNo) && (ShortMode != eShortModeExtreme) && ((Mask & MModIdx1) != 0) && (DistFits(AdrVals[0], AdrWord, 2, -256, 255, Flags)))
          {
            if (AdrVals[0] == eBaseRegPC)
              AdrWord -= 2;
            AdrVals[0] = 0xe0 | (AdrVals[0] << 3) | (Hi(AdrWord) & 1);
            AdrVals[1] = Lo(AdrWord);
            AdrCnt = 2;
            AdrMode = ModIdx1;
          }
          else
          {
            if (AdrVals[0] == eBaseRegPC)
              AdrWord -= 3;
            AdrVals[0] = 0xe2 | (AdrVals[0] << 3);
            AdrVals[1] = Hi(AdrWord);
            AdrVals[2] = Lo(AdrWord);
            AdrCnt = 3;
            AdrMode = ModIdx2;
          }
        }
      }
      goto chk;
    }
  }

  else WrError(ErrNum_InvAddrMode);

chk:
  if ((AdrMode != ModNone) && (((1 << AdrMode) & Mask) == 0))
  {
    AdrMode = ModNone;
    AdrCnt = 0;
    WrError(ErrNum_InvAddrMode);
  }
}

static void Try2Split(int Src)
{
  char *p;
  int z;
  size_t SrcLen;

  KillPrefBlanksStrComp(&ArgStr[Src]);
  KillPostBlanksStrComp(&ArgStr[Src]);
  SrcLen = strlen(ArgStr[Src].str.p_str);
  p = ArgStr[Src].str.p_str + SrcLen - 1;
  while ((p >= ArgStr[Src].str.p_str) && !as_isspace(*p))
    p--;
  if (p >= ArgStr[Src].str.p_str)
  {
    InsertArg(Src + 1, SrcLen);
    for (z = ArgCnt - 1; z >= Src + 1; z--)
      StrCompCopy(&ArgStr[z + 1], &ArgStr[z]);
    StrCompSplitRight(&ArgStr[Src], &ArgStr[Src + 1], p);
    KillPostBlanksStrComp(&ArgStr[Src]);
    KillPrefBlanksStrComp(&ArgStr[Src + 1]);
  }
}

/*---------------------------------------------------------------------------*/
/* Instruction Decoders */

static void DecodeFixed(Word Index)
{
  FixedOrder *pOrder = FixedOrders + Index;

  if (!ChkArgCnt(0, 0));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!ChkMinCPU(pOrder->MinCPU));
  else
  {
    if (Hi(pOrder->Code))
      BAsmCode[CodeLen++] = Hi(pOrder->Code);
    BAsmCode[CodeLen++] = Lo(pOrder->Code);
  }
}

static void DecodeGen(Word Index)
{
  GenOrder *pOrder = GenOrders + Index;

  if (!ChkArgCnt(1, 2));
  else if (!ChkMinCPU(pOrder->MinCPU));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    ExPos = 1 + Ord(Hi(pOrder->Code) != 0);
    OpSize = pOrder->ThisOpSize;
    DecodeAdr(1, ArgCnt, MModAllIdx | (pOrder->MayImm ? MModImm : 0) | (pOrder->MayDir ? MModDir : 0) | (pOrder->MayExt ? MModExt : 0));
    if (AdrMode != ModNone)
    {
      if (Hi(pOrder->Code) == 0)
      {
        BAsmCode[0] = pOrder->Code;
        CodeLen = 1;
      }
      else
      {
        BAsmCode[0] = Hi(pOrder->Code);
        BAsmCode[1] = Lo(pOrder->Code);
        CodeLen = 2;
      }
    }
    switch (AdrMode)
    {
      case ModImm:
        break;
      case ModDir:
        BAsmCode[CodeLen - 1] += 0x10;
        break;
      case ModIdx:
      case ModIdx1:
      case ModIdx2:
      case ModDIdx:
      case ModIIdx2:
        BAsmCode[CodeLen - 1] += 0x20;
        break;
      case ModExt:
        BAsmCode[CodeLen - 1] += 0x30;
        break;
    }
    if (AdrMode != ModNone)
    {
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeLEA(Word Index)
{
  FixedOrder *pOrder = LEAOrders + Index;

  if (!ChkArgCnt(1, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    ExPos = 1;
    DecodeAdr(1, ArgCnt, MModIdx | MModIdx1 | MModIdx2);
    if (AdrMode != ModNone)
    {
      BAsmCode[0] = pOrder->Code;
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeBranch(Word Index)
{
  FixedOrder *pOrder = BranchOrders + Index;
  LongInt Address;
  Boolean OK;
  tSymbolFlags Flags;

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Address = EvalStrIntExpressionWithFlags(&ArgStr[1], AddrInt, &OK, &Flags) - EProgCounter() - 2;
    if (OK)
    {
      if (((Address < -128) || (Address > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = pOrder->Code;
        BAsmCode[1] = Lo(Address);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeLBranch(Word Index)
{
  FixedOrder *pOrder = BranchOrders + Index;
  LongInt Address;
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Address = EvalStrIntExpression(&ArgStr[1], AddrInt, &OK) - EProgCounter() - 4;
    if (OK)
    {
      BAsmCode[0] = 0x18;
      BAsmCode[1] = pOrder->Code;
      BAsmCode[2] = Hi(Address);
      BAsmCode[3] = Lo(Address);
      CodeLen = 4;
    }
  }
}

static void DecodeJmp(Word Index)
{
  JmpOrder *pOrder = JmpOrders + Index;
  Word Mask;

  if (!ChkArgCnt(1, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Mask = MModAllIdx | MModExtPg;
    if (pOrder->MayDir)
      Mask |= MModDir;
    ExPos = 1;
    DecodeAdr(1, ArgCnt, Mask);
    if (AdrMode != ModNone)
    {
      switch (AdrMode)
      {
        case ModExt:
          BAsmCode[0] = pOrder->Code;
          break;
        case ModDir:
          BAsmCode[0] = pOrder->Code + 1;
          break;
        case ModIdx:
        case ModIdx1:
        case ModIdx2:
        case ModDIdx:
        case ModIIdx2:
          BAsmCode[0] = pOrder->Code - 1;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeLoop(Word Index)
{
  FixedOrder *pOrder = LoopOrders + Index;
  Byte HReg;
  LongInt Address;
  Boolean OK;
  tSymbolFlags Flags;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!DecodeReg(ArgStr[1].str.p_str, &HReg)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    OK = (HReg <= eRegB) || ((HReg >= eRegD) && (HReg <= eRegSP));
    if (!OK) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
    {
      Address = EvalStrIntExpressionWithFlags(&ArgStr[2], AddrInt, &OK, &Flags) - (EProgCounter() + 3);
      if (OK)
      {
        if (((Address < -256) || (Address > 255)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0x04;
          BAsmCode[1] = pOrder->Code | HReg | ((Address >> 4) & 0x10);
          BAsmCode[2] = Address & 0xff;
          CodeLen = 3;
        }
      }
    }
  }
}

static void DecodeETBL(Word Index)
{
  if (!ChkArgCnt(1, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    ExPos = 2;
    DecodeAdr(1, ArgCnt, MModIdx);
    if (AdrMode == ModIdx)
    {
      BAsmCode[0] = 0x18;
      BAsmCode[1] = Index;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeEMACS(Word Index)
{
  LongInt Address;
  Boolean OK;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    Address = EvalStrIntExpression(&ArgStr[1], UInt16, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x18;
      BAsmCode[1] = 0x12;
      BAsmCode[2] = Hi(Address) & 0xff;
      BAsmCode[3] = Lo(Address);
      CodeLen = 4;
    }
  }
}

static void DecodeTransfer(Word Index)
{
  Byte Reg1, Reg2;
  Byte ExtMask;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!DecodeReg(ArgStr[2].str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (eRegPC == Reg2) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (eRegPC == Reg1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (!ChkRegPair(Reg1, Reg2, &ExtMask)) WrError(ErrNum_InvRegPair);
  else
  {
    BAsmCode[0] = 0xb7;
    Reg1 &= 7;
    Reg2 &= 7;
    BAsmCode[1] = Index | (Reg1 << 4) | Reg2 | ExtMask;
    CodeLen = 2;
  }
}

static void DecodeSEX(Word Index)
{
  Byte Reg1, Reg2;
  Byte ExtMask;

  if (!ChkArgCnt(2, 2));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (!DecodeReg(ArgStr[2].str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (ActRegSize != 1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (eRegPC == Reg2) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
  else if (!DecodeReg(ArgStr[1].str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (ActRegSize != 0) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (eRegPC == Reg1) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else if (!ChkRegPair(Reg1, Reg2, &ExtMask)) WrError(ErrNum_InvRegPair);
  else
  {
    BAsmCode[0] = 0xb7;
    BAsmCode[1] = Index | (Reg1 << 4) | Reg2 | ExtMask;
    CodeLen = 2;
  }
}

static void DecodeMOV(Word Index)
{
  Byte Arg2Start, HCnt = 0, HAdrVals[4];
  Word Mask;

  switch (ArgCnt)
  {
    case 1:
      Try2Split(1);
      break;
    case 2:
      Try2Split(1);
      if (ArgCnt == 2)
        Try2Split(2);
      break;
    case 3:
      Try2Split(2);
      break;
  }

  if (!ChkArgCnt(2, 4));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    if (ArgCnt == 2)
      Arg2Start = 2;
    else if (ArgCnt == 4)
      Arg2Start = 3;
    else if (ValidReg(ArgStr[2].str.p_str))
      Arg2Start = 3;
    else
      Arg2Start = 2;
    OpSize = (tSymbolSize)Index;
    ExPos = 2;
    BAsmCode[0] = 0x18;
    BAsmCode[1] = (1 - Index) << 3;

    Mask = MModImm | MModExt | MModIdx;
    if (MomCPU >= CPU6812X)
      Mask |= MModIdx1 | MModIdx2 | MModDIdx | MModIIdx2;

    /* decode & save source operand */

    DecodeAdr(1, Arg2Start - 1, Mask);
    if (AdrMode != ModNone)
    {
      memcpy(HAdrVals, AdrVals, AdrCnt);
      HCnt = AdrCnt;
    }

    /* dispatch source address mode */

    switch (AdrMode)
    {
      case ModImm:
        ExPos = 4 + 2 * OpSize;
        DecodeAdr(Arg2Start, ArgCnt, MModExt | MModIdx | MModIdx1 | MModIdx2 | MModDIdx | MModIIdx2);
        switch (AdrMode)
        {
          case ModExt:
            BAsmCode[1] |= 3;
            memcpy(BAsmCode + 2 , HAdrVals, HCnt);
            memcpy(BAsmCode + 2 + HCnt, AdrVals, AdrCnt);
            break;
          case ModIdx:
          case ModIdx1:
          case ModIdx2:
          case ModDIdx:
          case ModIIdx2:
            memcpy(BAsmCode + 2 , AdrVals, AdrCnt);
            memcpy(BAsmCode + 2 + AdrCnt, HAdrVals, HCnt);
            break;
        }
        break;
      case ModExt:
        ExPos = 6;
        DecodeAdr(Arg2Start, ArgCnt, MModExt | MModIdx | MModIdx1 | MModIdx2 | MModDIdx | MModIIdx2);
        switch (AdrMode)
        {
          case ModExt:
            BAsmCode[1] |= 4;
            memcpy(BAsmCode + 2, HAdrVals, HCnt);
            memcpy(BAsmCode + 2 + HCnt, AdrVals, AdrCnt);
            break;
          case ModIdx:
          case ModIdx1:
          case ModIdx2:
          case ModDIdx:
          case ModIIdx2:
            BAsmCode[1] |= 1;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            memcpy(BAsmCode + 2 + AdrCnt, HAdrVals, HCnt);
            break;
        }
        break;
      case ModIdx:
      case ModIdx1:
      case ModIdx2:
      case ModDIdx:
      case ModIIdx2:
        ExPos = 4;
        DecodeAdr(Arg2Start, ArgCnt, MModExt | MModIdx| MModIdx1 | MModIdx2 | MModDIdx | MModIIdx2);
        if (AdrMode != ModNone)
        {
          BAsmCode[1] |= (AdrMode == ModExt) ? 5 : 2;
          memcpy(BAsmCode + 2, HAdrVals, HCnt);
          memcpy(BAsmCode + 2 + HCnt, AdrVals, AdrCnt);
        }
        break;
    }
    if (AdrMode != ModNone)
      CodeLen = 2 + AdrCnt + HCnt;
  }
}

static void DecodeLogic(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    OpSize = eSymbolSize8Bit; DecodeAdr(1, 1, MModImm);
    if (AdrMode == ModImm)
    {
      BAsmCode[0] = 0x10 | Index;
      BAsmCode[1] = AdrVals[0];
      CodeLen = 2;
    }
  }
}

static void DecodeBit(Word Index)
{
  Byte HReg;
  Boolean OK;

  if ((ArgCnt == 1) || (ArgCnt == 2))
    Try2Split(ArgCnt);

  if (!ChkArgCnt(2, 3));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    HReg = EvalStrIntExpressionOffs(&ArgStr[ArgCnt], !!(*ArgStr[ArgCnt].str.p_str  == '#'), UInt8, &OK);
    if (OK)
    {
      ExPos = 2; /* wg. Masken-Postbyte */
      DecodeAdr(1, ArgCnt - 1, MModDir | MModExt | MModIdx | MModIdx1 | MModIdx2);
      if (AdrMode != ModNone)
      {
        BAsmCode[0] = Index;
        switch (AdrMode)
        {
          case ModDir:
            BAsmCode[0] += 0x40;
            break;
          case ModExt:
            BAsmCode[0] += 0x10;
            break;
        }
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        BAsmCode[1 + AdrCnt] = HReg;
        CodeLen = 2 + AdrCnt;
      }
    }
  }
}

static void DecodeCALL(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 3));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else if (*ArgStr[1].str.p_str == '[')
  {
    if (ChkArgCnt(1, 1))
    {
      ExPos = 1; DecodeAdr(1, 1, MModDIdx | MModIIdx2);
      if (AdrMode != ModNone)
      {
        BAsmCode[0] = 0x4b;
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
      }
    }
  }
  else
  {
    if (ChkArgCnt(2, 3))
    {
      Boolean OK;
      Byte Page = EvalStrIntExpression(&ArgStr[ArgCnt], UInt8, &OK);

      if (OK)
      {
        ExPos = 2; /* wg. Seiten-Byte eins mehr */
        DecodeAdr(1, ArgCnt - 1, MModExt | MModIdx | MModIdx1 | MModIdx2);
        if (AdrMode != ModNone)
        {
          BAsmCode[0] = 0x4a | Ord(AdrMode != ModExt);
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          BAsmCode[1 + AdrCnt] = Page;
          CodeLen = 2 + AdrCnt;
        }
      }
    }
  }
}

static void DecodePCALL(Word Index)
{
  Boolean OK;
  LongWord Addr;

  UNUSED(Index);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU6812X))
  {
    Addr = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x4a;
      BAsmCode[1] = Hi(Addr);
      BAsmCode[2] = Lo(Addr);
      BAsmCode[3] = (Addr >> 16) & 0xff;
      CodeLen = 4;
    }
  }
}


static void DecodeBrBit(Word Index)
{
  Byte HReg;
  Boolean OK;
  tSymbolFlags Flags;
  LongInt Address;

  if (ArgCnt == 1)
  {
    Try2Split(1);
    Try2Split(1);
  }
  else if (ArgCnt == 2)
  {
    Try2Split(ArgCnt);
    Try2Split(2);
  }

  if (!ChkArgCnt(3, 4));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    HReg = EvalStrIntExpressionOffs(&ArgStr[ArgCnt - 1], !!(*ArgStr[ArgCnt - 1].str.p_str == '#'), UInt8, &OK);
    if (OK)
    {
      Address = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], AddrInt, &OK, &Flags) - EProgCounter();
      if (OK)
      {
        ExPos = 3; /* Opcode, Maske+Distanz */
        DecodeAdr(1, ArgCnt - 2, MModDir | MModExt | MModIdx | MModIdx1 | MModIdx2);
        if (AdrMode != ModNone)
        {
          BAsmCode[0] = 0x0e | Index;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          switch (AdrMode)
          {
            case ModDir:
              BAsmCode[0] += 0x40;
              break;
            case ModExt:
              BAsmCode[0] += 0x10;
              break;
          }
          BAsmCode[1 + AdrCnt] = HReg;
          Address -= 3 + AdrCnt;
          if (((Address < -128) || (Address > 127)) & !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[2 + AdrCnt] = Lo(Address);
            CodeLen = 3 + AdrCnt;
          }
        }
      }
    }
  }
}

static void DecodeTRAP(Word Index)
{
  Boolean OK;
  tSymbolFlags Flags;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    BAsmCode[1] = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), UInt8, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      BAsmCode[1] = 0x30;
    if (OK)
    {
      if ((BAsmCode[1] < 0x30) || ((BAsmCode[1] > 0x39) && (BAsmCode[1] < 0x40))) WrError(ErrNum_OverRange);
      else
      {
        BAsmCode[0] = 0x18;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeBTAS(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(2, 3));
  else if (!ChkMinCPU(CPU6812X));
  else if (*AttrPart.str.p_str) WrError(ErrNum_UseLessAttr);
  else
  {
    ExPos = 2;
    OpSize = eSymbolSize8Bit;
    DecodeAdr(1, ArgCnt - 1, MModDir | MModExt | MModIdx | MModIdx1 | ModIdx2 | ModDIdx);
    if (AdrMode != ModNone)
    {
      BAsmCode[CodeLen++] = 0x18;
      switch (AdrMode)
      {
        case ModImm:
          break;
        case ModDir:
          BAsmCode[CodeLen++] = 0x35;
          break;
        case ModIdx:
        case ModIdx1:
        case ModIdx2:
        case ModDIdx:
          BAsmCode[CodeLen++] = 0x37;
          break;
        case ModExt:
          BAsmCode[CodeLen++] = 0x36;
          break;
      }
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
      DecodeAdr(ArgCnt, ArgCnt, MModImm);
      if (AdrMode  == ModImm)
        BAsmCode[CodeLen++] = *AdrVals;
      else
        CodeLen = 0;
    }
  }
}

static void LookupReg(Word Index)
{
  Reg *pReg = Regs + Index;

  ActReg = (MomCPU >= pReg->MinCPU) ? pReg->Code : (Byte)eNoReg;
  ActRegSize = pReg->OpSize;
}

/*---------------------------------------------------------------------------*/
/* Dynamic Code Table Handling */

static void AddFixed(const char *NName, Word NCode, CPUVar NMin)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = NCode;
  FixedOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeFixed);
}

static void AddBranch(const char *NName, Word NCode)
{
  order_array_rsv_end(BranchOrders, FixedOrder);
  BranchOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName + 1, InstrZ  , DecodeBranch);
  AddInstTable(InstTable, NName    , InstrZ++, DecodeLBranch);
}

static void AddGen(const char *NName, Word NCode,
                   Boolean NMayI, Boolean NMayD, Boolean NMayE,
                   tSymbolSize NSize, CPUVar NMin)
{
  order_array_rsv_end(GenOrders, GenOrder);
  GenOrders[InstrZ].Code = NCode;
  GenOrders[InstrZ].MayImm = NMayI;
  GenOrders[InstrZ].MayDir = NMayD;
  GenOrders[InstrZ].MayExt = NMayE;
  GenOrders[InstrZ].ThisOpSize = NSize;
  GenOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, NName, InstrZ++, DecodeGen);
}

static void AddLoop(const char *NName, Word NCode)
{
  order_array_rsv_end(LoopOrders, FixedOrder);
  LoopOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLoop);
}

static void AddLEA(const char *NName, Word NCode)
{
  order_array_rsv_end(LEAOrders, FixedOrder);
  LEAOrders[InstrZ].Code = NCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeLEA);
}

static void AddJmp(const char *NName, Word NCode, Boolean NDir)
{
  order_array_rsv_end(JmpOrders, JmpOrder);
  JmpOrders[InstrZ].Code = NCode;
  JmpOrders[InstrZ].MayDir = NDir;
  AddInstTable(InstTable, NName, InstrZ++, DecodeJmp);
}

static void AddReg(const char *NName, Word NCode, Word NSize, CPUVar NMin)
{
  order_array_rsv_end(Regs, Reg);
  Regs[InstrZ].Code = NCode;
  Regs[InstrZ].OpSize = NSize;
  Regs[InstrZ].MinCPU = NMin;
  AddInstTable(RegTable, NName, InstrZ++, LookupReg);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(405);

  InstrZ = 0;
  AddFixed("ABA"  , 0x1806, CPU6812 ); AddFixed("ABX"  , 0x1ae5, CPU6812 );
  AddFixed("ABY"  , 0x19ed, CPU6812 ); AddFixed("ASLA" , 0x0048, CPU6812 );
  AddFixed("ASLB" , 0x0058, CPU6812 ); AddFixed("ASLD" , 0x0059, CPU6812 );
  AddFixed("ASLX" , 0x1848, CPU6812X); AddFixed("ASLY" , 0x1858, CPU6812X);
  AddFixed("ASRA" , 0x0047, CPU6812 ); AddFixed("ASRB" , 0x0057, CPU6812 );
  AddFixed("ASRX" , 0x1847, CPU6812X); AddFixed("ASRY" , 0x1857, CPU6812X);
  AddFixed("BGND" , 0x0000, CPU6812 ); AddFixed("CBA"  , 0x1817, CPU6812 );
  AddFixed("CLC"  , 0x10fe, CPU6812 ); AddFixed("CLI"  , 0x10ef, CPU6812 );
  AddFixed("CLRA" , 0x0087, CPU6812 ); AddFixed("CLRB" , 0x00c7, CPU6812 );
  AddFixed("CLRX" , 0x1887, CPU6812X); AddFixed("CLRY" , 0x18c7, CPU6812X);
  AddFixed("CLV"  , 0x10fd, CPU6812 ); AddFixed("COMA" , 0x0041, CPU6812 );
  AddFixed("COMB" , 0x0051, CPU6812 ); AddFixed("COMX" , 0x1841, CPU6812X);
  AddFixed("COMY" , 0x1851, CPU6812X); AddFixed("DAA"  , 0x1807, CPU6812 );
  AddFixed("DECA" , 0x0043, CPU6812 ); AddFixed("DECB" , 0x0053, CPU6812 );
  AddFixed("DECX" , 0x1843, CPU6812X); AddFixed("DECY" , 0x1853, CPU6812X);
  AddFixed("DES"  , 0x1b9f, CPU6812 ); AddFixed("DEX"  , 0x0009, CPU6812 );
  AddFixed("DEY"  , 0x0003, CPU6812 ); AddFixed("EDIV" , 0x0011, CPU6812 );
  AddFixed("EDIVS", 0x1814, CPU6812 ); AddFixed("EMUL" , 0x0013, CPU6812 );
  AddFixed("EMULS", 0x1813, CPU6812 ); AddFixed("FDIV" , 0x1811, CPU6812 );
  AddFixed("IDIV" , 0x1810, CPU6812 ); AddFixed("IDIVS", 0x1815, CPU6812 );
  AddFixed("INCA" , 0x0042, CPU6812 ); AddFixed("INCB" , 0x0052, CPU6812 );
  AddFixed("INCX" , 0x1842, CPU6812X); AddFixed("INCY" , 0x1852, CPU6812X);
  AddFixed("INS"  , 0x1b81, CPU6812 ); AddFixed("INX"  , 0x0008, CPU6812 );
  AddFixed("INY"  , 0x0002, CPU6812 ); AddFixed("LSLA" , 0x0048, CPU6812 );
  AddFixed("LSLB" , 0x0058, CPU6812 ); AddFixed("LSLX" , 0x1848, CPU6812X);
  AddFixed("LSLY" , 0x1858, CPU6812X); AddFixed("LSLD" , 0x0059, CPU6812 );
  AddFixed("LSRA" , 0x0044, CPU6812 ); AddFixed("LSRB" , 0x0054, CPU6812 );
  AddFixed("LSRX" , 0x1844, CPU6812X); AddFixed("LSRY" , 0x1854, CPU6812X);
  AddFixed("LSRD" , 0x0049, CPU6812 ); AddFixed("MEM"  , 0x0001, CPU6812 );
  AddFixed("MUL"  , 0x0012, CPU6812 ); AddFixed("NEGA" , 0x0040, CPU6812 );
  AddFixed("NEGB" , 0x0050, CPU6812 ); AddFixed("NEGX" , 0x1840, CPU6812X);
  AddFixed("NEGY" , 0x1850, CPU6812X); AddFixed("NOP"  , 0x00a7, CPU6812 );
  AddFixed("PSHA" , 0x0036, CPU6812 ); AddFixed("PSHB" , 0x0037, CPU6812 );
  AddFixed("PSHC" , 0x0039, CPU6812 ); AddFixed("PSHCW", 0x1839, CPU6812X);
  AddFixed("PSHD" , 0x003b, CPU6812 ); AddFixed("PSHX" , 0x0034, CPU6812 );
  AddFixed("PSHY" , 0x0035, CPU6812 ); AddFixed("PULA" , 0x0032, CPU6812 );
  AddFixed("PULB" , 0x0033, CPU6812 ); AddFixed("PULC" , 0x0038, CPU6812 );
  AddFixed("PULCW", 0x1838, CPU6812X); AddFixed("PULD" , 0x003a, CPU6812 );
  AddFixed("PULX" , 0x0030, CPU6812 ); AddFixed("PULY" , 0x0031, CPU6812 );
  AddFixed("REV"  , 0x183a, CPU6812 ); AddFixed("REVW" , 0x183b, CPU6812 );
  AddFixed("ROLA" , 0x0045, CPU6812 ); AddFixed("ROLB" , 0x0055, CPU6812 );
  AddFixed("ROLX" , 0x1845, CPU6812X); AddFixed("ROLY" , 0x1855, CPU6812X);
  AddFixed("RORA" , 0x0046, CPU6812 ); AddFixed("RORB" , 0x0056, CPU6812 );
  AddFixed("RORX" , 0x1846, CPU6812X); AddFixed("RORY" , 0x1856, CPU6812X);
  AddFixed("RTC"  , 0x000a, CPU6812 ); AddFixed("RTI"  , 0x000b, CPU6812 );
  AddFixed("RTS"  , 0x003d, CPU6812 ); AddFixed("SBA"  , 0x1816, CPU6812 );
  AddFixed("SEC"  , 0x1401, CPU6812 ); AddFixed("SEI"  , 0x1410, CPU6812 );
  AddFixed("SEV"  , 0x1402, CPU6812 ); AddFixed("STOP" , 0x183e, CPU6812 );
  AddFixed("SWI"  , 0x003f, CPU6812 ); AddFixed("TAB"  , 0x180e, CPU6812 );
  AddFixed("TAP"  , 0xb702, CPU6812 ); AddFixed("TBA"  , 0x180f, CPU6812 );
  AddFixed("TPA"  , 0xb720, CPU6812 ); AddFixed("TSTA" , 0x0097, CPU6812 );
  AddFixed("TSTB" , 0x00d7, CPU6812 ); AddFixed("TSTX" , 0x1897, CPU6812X);
  AddFixed("TSTY" , 0x18d7, CPU6812X); AddFixed("TSX"  , 0xb775, CPU6812 );
  AddFixed("TSY"  , 0xb776, CPU6812 ); AddFixed("TXS"  , 0xb757, CPU6812 );
  AddFixed("TYS"  , 0xb767, CPU6812 ); AddFixed("WAI"  , 0x003e, CPU6812 );
  AddFixed("WAV"  , 0x183c, CPU6812 ); AddFixed("XGDX" , 0xb7c5, CPU6812 );
  AddFixed("XGDY" , 0xb7c6, CPU6812 );

  InstrZ = 0;
  AddBranch("LBGT", 0x2e);    AddBranch("LBGE", 0x2c);
  AddBranch("LBEQ", 0x27);    AddBranch("LBLE", 0x2f);
  AddBranch("LBLT", 0x2d);    AddBranch("LBHI", 0x22);
  AddBranch("LBHS", 0x24);    AddBranch("LBCC", 0x24);
  AddBranch("LBNE", 0x26);    AddBranch("LBLS", 0x23);
  AddBranch("LBLO", 0x25);    AddBranch("LBCS", 0x25);
  AddBranch("LBMI", 0x2b);    AddBranch("LBVS", 0x29);
  AddBranch("LBRA", 0x20);    AddBranch("LBPL", 0x2a);
  AddBranch("LBRN", 0x21);    AddBranch("LBVC", 0x28);
  AddBranch("LBSR", 0x07);

  InstrZ = 0;
  AddGen("ADCA" , 0x0089, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ADCB" , 0x00c9, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ADDA" , 0x008b, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ADDB" , 0x00cb, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ADDD" , 0x00c3, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("ADDX" , 0x188b, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ADDY" , 0x18cb, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ADED" , 0x18c3, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ADEX" , 0x1889, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ADEY" , 0x18c9, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ANDA" , 0x0084, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ANDB" , 0x00c4, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ANDX" , 0x1884, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ANDY" , 0x18c4, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ASL"  , 0x0048, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("ASLW" , 0x1848, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("ASR"  , 0x0047, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("ASRW" , 0x1847, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("BITA" , 0x0085, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("BITB" , 0x00c5, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("BITX" , 0x1885, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("BITY" , 0x18c5, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("CLR"  , 0x0049, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("CLRW" , 0x1849, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("CMPA" , 0x0081, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("CMPB" , 0x00c1, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("COM"  , 0x0041, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("COMW" , 0x1841, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("CPED" , 0x188c, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("CPES" , 0x188f, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("CPEX" , 0x188e, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("CPEY" , 0x188d, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("CPD"  , 0x008c, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("CPS"  , 0x008f, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("CPX"  , 0x008e, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("CPY"  , 0x008d, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("DEC"  , 0x0043, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("DECW" , 0x1843, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("EMAXD", 0x18fa, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("EMAXM", 0x18fe, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("EMIND", 0x18fb, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("EMINM", 0x18ff, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("EORA" , 0x0088, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("EORB" , 0x00c8, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("EORX" , 0x1888, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("EORY" , 0x18c8, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("GLDAA", 0x1886, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GLDAB", 0x18c6, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GLDD" , 0x18cc, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GLDS" , 0x18cf, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GLDX" , 0x18ce, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GLDY" , 0x18cd, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTAA", 0x184a, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTAB", 0x184b, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTD" , 0x184c, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTS" , 0x184f, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTX" , 0x184e, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("GSTY" , 0x184d, False, True , True , eSymbolSizeUnknown, CPU6812X);
  AddGen("INC"  , 0x0042, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("INCW" , 0x1842, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("LDAA" , 0x0086, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("LDAB" , 0x00c6, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("LDD"  , 0x00cc, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("LDS"  , 0x00cf, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("LDX"  , 0x00ce, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("LDY"  , 0x00cd, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("LSL"  , 0x0048, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("LSLW" , 0x1848, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("LSR"  , 0x0044, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("LSRW" , 0x1844, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("MAXA" , 0x18f8, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("MAXM" , 0x18fc, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("MINA" , 0x18f9, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("MINM" , 0x18fd, False, False, False, eSymbolSizeUnknown, CPU6812 );
  AddGen("NEG"  , 0x0040, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("NEGW" , 0x1840, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("ORAA" , 0x008a, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ORAB" , 0x00ca, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("ORX"  , 0x188a, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ORY"  , 0x18ca, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("ROL"  , 0x0045, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("ROLW" , 0x1845, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("ROR"  , 0x0046, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("RORW" , 0x1846, False, False, True , eSymbolSizeUnknown, CPU6812X);
  AddGen("SBCA" , 0x0082, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("SBCB" , 0x00c2, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("SBED" , 0x1883, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("SBEX" , 0x1882, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("SBEY" , 0x18c2, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("STAA" , 0x004a, False, True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("STAB" , 0x004b, False, True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("STD"  , 0x004c, False, True , True , eSymbolSizeUnknown, CPU6812 );
  AddGen("STS"  , 0x004f, False, True , True , eSymbolSizeUnknown, CPU6812 );
  AddGen("STX"  , 0x004e, False, True , True , eSymbolSizeUnknown, CPU6812 );
  AddGen("STY"  , 0x004d, False, True , True , eSymbolSizeUnknown, CPU6812 );
  AddGen("SUBA" , 0x0080, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("SUBB" , 0x00c0, True , True , True , eSymbolSize8Bit   , CPU6812 );
  AddGen("SUBX" , 0x1880, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("SUBY" , 0x18c0, True , True , True , eSymbolSize16Bit  , CPU6812X);
  AddGen("SUBD" , 0x0083, True , True , True , eSymbolSize16Bit  , CPU6812 );
  AddGen("TST"  , 0x00c7, False, False, True , eSymbolSizeUnknown, CPU6812 );
  AddGen("TSTW" , 0x18c7, False, False, True , eSymbolSizeUnknown, CPU6812X);

  InstrZ = 0;
  AddLoop("DBEQ", 0x00); AddLoop("DBNE", 0x20);
  AddLoop("IBEQ", 0x80); AddLoop("IBNE", 0xa0);
  AddLoop("TBEQ", 0x40); AddLoop("TBNE", 0x60);

  InstrZ = 0;
  AddLEA("LEAS", 0x1b);
  AddLEA("LEAX", 0x1a);
  AddLEA("LEAY", 0x19);

  InstrZ = 0;
  AddJmp("JMP", 0x06, False);
  AddJmp("JSR", 0x16, True );

  AddInstTable(InstTable, "TBL"   , 0x3d, DecodeETBL);
  AddInstTable(InstTable, "ETBL"  , 0x3f, DecodeETBL);
  AddInstTable(InstTable, "EMACS" , 0   , DecodeEMACS);
  AddInstTable(InstTable, "TFR"   , 0x00, DecodeTransfer);
  AddInstTable(InstTable, "EXG"   , 0x80, DecodeTransfer);
  AddInstTable(InstTable, "SEX"   , 0   , DecodeSEX);
  AddInstTable(InstTable, "MOVB"  , eSymbolSize8Bit, DecodeMOV);
  AddInstTable(InstTable, "MOVW"  , eSymbolSize16Bit, DecodeMOV);
  AddInstTable(InstTable, "ANDCC" , 0x00, DecodeLogic);
  AddInstTable(InstTable, "ORCC"  , 0x04, DecodeLogic);
  AddInstTable(InstTable, "BSET"  , 0x0c, DecodeBit);
  AddInstTable(InstTable, "BCLR"  , 0x0d, DecodeBit);
  AddInstTable(InstTable, "CALL"  , 0   , DecodeCALL);
  AddInstTable(InstTable, "PCALL" , 0   , DecodePCALL);
  AddInstTable(InstTable, "BRSET" , 0x00, DecodeBrBit);
  AddInstTable(InstTable, "BRCLR" , 0x01, DecodeBrBit);
  AddInstTable(InstTable, "TRAP"  , 0   , DecodeTRAP);
  AddInstTable(InstTable, "BTAS"  , 0   , DecodeBTAS);

  RegTable = CreateInstTable(31);
  InstrZ = 0;
  AddReg("A"   , eRegA    , 0, CPU6812 );
  AddReg("B"   , eRegB    , 0, CPU6812 );
  AddReg("CCR" , eRegCCRL , 0, CPU6812 );
  AddReg("CCRL", eRegCCRL , 0, CPU6812 );
  AddReg("D"   , eRegD    , 1, CPU6812 );
  AddReg("X"   , eRegX    , 1, CPU6812 );
  AddReg("Y"   , eRegY    , 1, CPU6812 );
  AddReg("SP"  , eRegSP   , 1, CPU6812 );
  AddReg("PC"  , eRegPC   , 1, CPU6812 );
  AddReg("XL"  , eRegXL   , 0, CPU6812 );
  AddReg("XH"  , eRegXH   , 0, CPU6812X);
  AddReg("YL"  , eRegYL   , 0, CPU6812 );
  AddReg("YH"  , eRegYH   , 0, CPU6812X);
  AddReg("SPL" , eRegSPL  , 0, CPU6812 );
  AddReg("SPH" , eRegSPH  , 0, CPU6812X);
  AddReg("CCRH", eRegCCRH , 0, CPU6812X);
  AddReg("CCRW", eRegCCRW , 1, CPU6812X);

  init_moto8_pseudo(InstTable, e_moto_8_be | e_moto_8_db | e_moto_8_dw);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(BranchOrders);
  order_array_free(GenOrders);
  order_array_free(LoopOrders);
  order_array_free(LEAOrders);
  order_array_free(JmpOrders);

  DestroyInstTable(RegTable);
  order_array_free(Regs);
}

/*--------------------------------------------------------------------------*/
/* Main Functions */

static Boolean DecodeAttrPart_6812(void)
{
  if (strlen(AttrPart.str.p_str) > 1)
  {
    WrStrErrorPos(ErrNum_UndefAttr, &AttrPart);
    return False;
  }

  /* Operandengroesse festlegen */

  return DecodeMoto16AttrSize(*AttrPart.str.p_str, &AttrPartOpSize[0], False);
}

static void MakeCode_6812(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = AttrPartOpSize[0];

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeMoto16Pseudo(OpSize,True)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_6812(void)
{
  Reg_Direct = 0;
  Reg_GPage = 0;
}

static Boolean IsDef_6812(void)
{
  return False;
}

static Boolean ChkPC_6812X(LargeWord Addr)
{
  Byte Page = (Addr >> 16) & 0xff;

  if (ActPC != SegCode)
    return False;
  else if ((Addr & 0xc000) == 0x8000)
    return ((Page == 0) || ((Page >= 0x30) && (Page <= 0x3f)));
  else
    return (Page == 0);
}

static void SwitchTo_6812(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x66;
  NOPCode = 0xa7;
  DivideChars = ",";
  HasAttrs = True;
  AttrChars = ".";

  ValidSegs = (1 << SegCode);
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  if (MomCPU == CPU6812X)
  {
    SegLimits[SegCode] = 0x3bfff;
    ChkPC = ChkPC_6812X;
    AddrInt = UInt22;
  }
  else
  {
    SegLimits[SegCode] = 0xffff;
    AddrInt = UInt16;
  }

  DecodeAttrPart = DecodeAttrPart_6812;
  MakeCode = MakeCode_6812;
  IsDef = IsDef_6812;
  SwitchFrom = DeinitFields;
  InitFields();
  AddMoto16PseudoONOFF(False);

  if (MomCPU >= CPU6812X)
  {
#define ASSUME6812Count (sizeof(ASSUME6812s) / sizeof(*ASSUME6812s))
   static const ASSUMERec ASSUME6812s[] =
   {
     { "DIRECT" , &Reg_Direct , 0,  0xff,  0x100, NULL },
     { "GPAGE"  , &Reg_GPage  , 0,  0x7f,   0x80, NULL }
   };

   pASSUMERecs = ASSUME6812s;
   ASSUMERecCnt = ASSUME6812Count;
  }
}

void code6812_init(void)
{
  CPU6812  = AddCPU("68HC12", SwitchTo_6812);
  CPU6812X = AddCPU("68HC12X", SwitchTo_6812);

  AddInitPassProc(InitCode_6812);
}
