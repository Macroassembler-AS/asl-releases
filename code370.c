/* code370.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 370-Familie                                                 */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"

#include <ctype.h>
#include <string.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "intformat.h"
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code370.h"

typedef struct
{
  char *Name;
  Word Code;
} FixedOrder;

enum
{
  ModNone = -1,
  ModAccA = 0,       /* A */
  ModAccB = 1,       /* B */
  ModReg = 2,        /* Rn */
  ModPort = 3,       /* Pn */
  ModAbs = 4,        /* nnnn */
  ModBRel = 5,       /* nnnn(B) */
  ModSPRel = 6,      /* nn(SP) */
  ModIReg = 7,       /* @Rn */
  ModRegRel = 8,     /* nn(Rn) */
  ModImm = 9,        /* #nn */
  ModImmBRel = 10,   /* #nnnn(B) */
  ModImmRegRel = 11  /* #nn(Rm) */
};

#define MModAccA (1 << ModAccA)
#define MModAccB (1 << ModAccB)
#define MModReg (1 << ModReg)
#define MModPort (1 << ModPort)
#define MModAbs (1 << ModAbs)
#define MModBRel (1 << ModBRel)
#define MModSPRel (1 << ModSPRel)
#define MModIReg (1 << ModIReg)
#define MModRegRel (1 << ModRegRel)
#define MModImm (1 << ModImm)
#define MModImmBRel (1 << ModImmBRel)
#define MModImmRegRel (1 << ModImmRegRel)

static CPUVar CPU37010, CPU37020, CPU37030, CPU37040, CPU37050;

static Byte OpSize;
static ShortInt AdrType;
static Byte AdrVals[2];

/****************************************************************************/

static char *HasDisp(char *Asc)
{
  char *p;
  int Lev;

  if ((*Asc) && (Asc[strlen(Asc) - 1] == ')'))
  {
    p = Asc + strlen(Asc) - 2;
    Lev = 0;
    while ((p >= Asc) && (Lev != -1))
    {
      switch (*p)
      {
        case '(': Lev--; break;
        case ')': Lev++; break;
      }
      if (Lev != -1)
        p--;
    }
    if (p < Asc)
    {
      WrXError(ErrNum_BrackErr, Asc);
      return NULL;
    }
  }
  else
    p = NULL;

  return p;
}

static void DecodeAdrRel(const tStrComp *pArg, Word Mask, Boolean AddrRel)
{
  Integer HVal;
  char *p;
  Boolean OK;

  AdrType = ModNone;
  AdrCnt = 0;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    if (Mask & MModAccA)
      AdrType = ModAccA;
    else if (Mask & MModReg)
    {
      AdrCnt = 1;
      AdrVals[0] = 0;
      AdrType = ModReg;
    }
    else
    {
      AdrCnt = 2;
      AdrVals[0] = 0;
      AdrVals[1] = 0;
      AdrType = ModAbs;
    }
    goto chk;
  }

  if (!as_strcasecmp(pArg->str.p_str, "B"))
  {
    if (Mask & MModAccB)
      AdrType = ModAccB;
    else if (Mask & MModReg)
    {
      AdrCnt = 1;
      AdrVals[0] = 1;
      AdrType = ModReg;
    }
    else
    {
      AdrCnt = 2;
      AdrVals[0] = 0;
      AdrVals[1] = 1;
      AdrType = ModAbs;
    }
    goto chk;
  }

  if (*pArg->str.p_str == '#')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    p = HasDisp(Arg.str.p_str);
    if (!p)
    {
      switch (OpSize)
      {
        case 0:
          AdrVals[0] = EvalStrIntExpression(&Arg, Int8, &OK);
          break;
        case 1:
          HVal = EvalStrIntExpression(&Arg, Int16, &OK);
          AdrVals[0] = Hi(HVal); AdrVals[1] = Lo(HVal);
          break;
      }
      if (OK)
      {
        AdrCnt = 1 + OpSize;
        AdrType = ModImm;
      }
    }
    else
    {
      tStrComp Left, Right;
      tSymbolFlags Flags;

      StrCompSplitRef(&Left, &Right, &Arg, p);
      HVal = EvalStrIntExpressionWithFlags(&Left, Int16, &OK, &Flags);
      if (OK)
      {
        *p = '(';
        if (!as_strcasecmp(p, "(B)"))
        {
          AdrVals[0] = Hi(HVal);
          AdrVals[1] = Lo(HVal);
          AdrCnt = 2;
          AdrType = ModImmBRel;
        }
        else
        {
          if (mFirstPassUnknown(Flags))
            HVal &= 127;
          if (ChkRange(HVal, -128, 127))
          {
            AdrVals[0] = HVal & 0xff;
            AdrCnt = 1;
            AdrVals[1] = EvalStrIntExpression(&Arg, UInt8, &OK);
            if (OK)
            {
              AdrCnt = 2;
              AdrType = ModImmRegRel;
            }
          }
        }
      }
    }
    goto chk;
  }

  if (*pArg->str.p_str == '@')
  {
    AdrVals[0] = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
    if (OK)
    {
      AdrCnt = 1;
      AdrType = ModIReg;
    }
    goto chk;
  }

  p = HasDisp(pArg->str.p_str);

  if (!p)
  {
    HVal = EvalStrIntExpression(pArg, Int16, &OK);
    if (OK)
    {
      if (((Mask & MModReg) != 0) && (Hi(HVal) == 0))
      {
        AdrVals[0] = Lo(HVal);
        AdrCnt = 1;
        AdrType = ModReg;
      }
      else if (((Mask & MModPort) != 0) && (Hi(HVal) == 0x10))
      {
        AdrVals[0] = Lo(HVal);
        AdrCnt = 1;
        AdrType = ModPort;
      }
      else
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        AdrVals[0] = Hi(HVal);
        AdrVals[1] = Lo(HVal);
        AdrCnt = 2;
        AdrType = ModAbs;
      }
    }
    goto chk;
  }
  else
  {
    tStrComp Left, Right;
    tSymbolFlags Flags;

    StrCompSplitRef(&Left, &Right, pArg, p);
    HVal = EvalStrIntExpressionWithFlags(&Left, Int16, &OK, &Flags);
    if (mFirstPassUnknown(Flags))
      HVal &= 0x7f;
    if (OK)
    {
      StrCompShorten (&Right, 1);
      if (!as_strcasecmp(Right.str.p_str, "B"))
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        AdrVals[0] = Hi(HVal);
        AdrVals[1] = Lo(HVal);
        AdrCnt = 2;
        AdrType = ModBRel;
      }
      else if (!as_strcasecmp(Right.str.p_str, "SP"))
      {
        if (AddrRel)
          HVal -= EProgCounter() + 3;
        if (HVal > 127) WrError(ErrNum_OverRange);
        else if (HVal < -128) WrError(ErrNum_UnderRange);
        else
        {
          AdrVals[0] = HVal & 0xff;
          AdrCnt = 1;
          AdrType = ModSPRel;
        }
      }
      else
      {
        if (HVal > 127) WrError(ErrNum_OverRange);
        else if (HVal < -128) WrError(ErrNum_UnderRange);
        else
        {
          AdrVals[0] = HVal & 0xff;
          AdrVals[1] = EvalStrIntExpression(&Right, Int8, &OK);
          if (OK)
          {
            AdrCnt = 2;
            AdrType = ModRegRel;
          }
        }
      }
    }
    goto chk;
  }

chk:
  if ((AdrType != ModNone) && (!(Mask & (1 << AdrType))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrType = ModNone;
    AdrCnt = 0;
  }
}

static void DecodeAdr(const tStrComp *pArg, Word Mask)
{
  DecodeAdrRel(pArg, Mask, FALSE);
}

static void PutCode(Word Code)
{
  if (Hi(Code) == 0)
  {
    CodeLen = 1;
    BAsmCode[0] = Code;
  }
  else
  {
    CodeLen = 2;
    BAsmCode[0] = Hi(Code);
    BAsmCode[1] = Lo(Code);
  }
}

static void DissectBitValue(LargeWord Symbol, Word *pAddr, Byte *pBit)
{
  *pAddr = Symbol & 0xffff;
  *pBit = (Symbol >> 16) & 7;
}

static void DissectBit_370(char *pDest, size_t DestSize, LargeWord Symbol)
{
  Word Addr;
  Byte Bit;

  DissectBitValue(Symbol, &Addr, &Bit);

  if (Addr < 2)
    as_snprintf(pDest, DestSize, "%c", HexStartCharacter + Addr);
  else
    as_snprintf(pDest, DestSize, "%~0.*u%s",
                ListRadixBase, (unsigned)Addr, GetIntConstIntelSuffix(ListRadixBase));
  as_snprcatf(pDest, DestSize, ".%c", Bit + '0');
}

static Boolean DecodeBitExpr(int Start, int Stop, LongWord *pResult)
{
  Boolean OK;

  if (Start == Stop)
  {
    *pResult = EvalStrIntExpression(&ArgStr[Start], UInt19, &OK);
    return OK;
  }
  else
  {
    Byte Bit;
    Word Addr;

    Bit = EvalStrIntExpression(&ArgStr[Start], UInt3, &OK);
    if (!OK)
      return OK;

    if ((!as_strcasecmp(ArgStr[Stop].str.p_str, "A")) || (!as_strcasecmp(ArgStr[Stop].str.p_str, "B")))
    {
      Addr = toupper(*ArgStr[Stop].str.p_str) - 'A';
      OK = True;
    }
    else
    {
      tSymbolFlags Flags;

      Addr = EvalStrIntExpressionWithFlags(&ArgStr[Stop], UInt16, &OK, &Flags);
      if (!OK)
        return OK;
      if (mFirstPassUnknown(Flags))
        Addr &= 0xff;
      if (Addr & 0xef00) /* 00h...0ffh, 1000h...10ffh allowed */
      {
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[Stop]);
        return False;
      }
    }

    *pResult = (((LongWord)Bit) << 16) + Addr;
    return True;
  }
}

/****************************************************************************/

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    PutCode(Code);
}

static void DecodeDBIT(Word Code)
{
  LongWord Value;

  UNUSED(Code);

  if (ChkArgCnt(1, 2) && DecodeBitExpr(1, ArgCnt, &Value))
  {
    PushLocHandle(-1);
    EnterIntSymbol(&LabPart, Value, SegBData, False);
    *ListLine = '=';
    DissectBit_370(ListLine + 1, STRINGSIZE - 1, Value);
    PopLocHandle();
  }
}

static void DecodeMOV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModAccB + MModReg + MModPort + MModAbs + MModIReg + MModBRel
                       + MModSPRel + MModRegRel + MModAccA);
    switch (AdrType)
    {
      case ModAccA:
        DecodeAdr(&ArgStr[1], MModReg + MModAbs + MModIReg + MModBRel + MModRegRel
                           + MModSPRel + MModAccB + MModPort + MModImm);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x12;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModAbs:
            BAsmCode[0] = 0x8a;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModIReg:
            BAsmCode[0] = 0x9a;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModBRel:
            BAsmCode[0] = 0xaa;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModRegRel:
            BAsmCode[0] = 0xf4;
            BAsmCode[1] = 0xea;
            memcpy(BAsmCode + 2, AdrVals, 2);
            CodeLen = 4;
            break;
          case ModSPRel:
            BAsmCode[0] = 0xf1;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModAccB:
            BAsmCode[0] = 0x62;
            CodeLen = 1;
            break;
          case ModPort:
            BAsmCode[0] = 0x80;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x22;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModAccB:
        DecodeAdr(&ArgStr[1], MModAccA + MModReg + MModPort + MModImm);
        switch (AdrType)
        {
          case ModAccA:
            BAsmCode[0] = 0xc0;
            CodeLen = 1;
            break;
          case ModReg:
            BAsmCode[0] = 0x32;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModPort:
            BAsmCode[0] = 0x91;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x52;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModReg:
        BAsmCode[1] = BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModAccA + MModAccB + MModReg + MModPort + MModImm);
        switch (AdrType)
        {
          case ModAccA:
            BAsmCode[0] = 0xd0;
            CodeLen = 2;
            break;
          case ModAccB:
            BAsmCode[0] = 0xd1;
            CodeLen = 2;
            break;
          case ModReg:
            BAsmCode[0] = 0x42;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModPort:
            BAsmCode[0] = 0xa2;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = 0x72;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
      case ModPort:
        BAsmCode[1] = BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModAccA + MModAccB + MModReg + MModImm);
        switch (AdrType)
        {
          case ModAccA:
            BAsmCode[0] = 0x21;
            CodeLen = 2;
            break;
          case ModAccB:
            BAsmCode[0] = 0x51;
            CodeLen = 2;
            break;
          case ModReg:
            BAsmCode[0] = 0x71;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = 0xf7;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
      case ModAbs:
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[1], MModAccA);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0x8b;
          CodeLen = 3;
        }
        break;
      case ModIReg:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModAccA);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0x9b;
          CodeLen = 2;
        }
        break;
      case ModBRel:
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[1], MModAccA);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xab;
          CodeLen = 3;
        }
        break;
      case ModSPRel:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModAccA);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xf2;
          CodeLen = 2;
        }
        break;
      case ModRegRel:
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[1], MModAccA);
        if (AdrType != ModNone)
        {
          BAsmCode[0] = 0xf4;
          BAsmCode[1] = 0xeb;
          CodeLen = 4;
        }
        break;
    }
  }
}

static void DecodeMOVW(Word Code)
{
  UNUSED(Code);

  OpSize = 1;
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrType != ModNone)
    {
      Byte AdrVal = AdrVals[0];

      DecodeAdr(&ArgStr[1], MModReg + MModImm + MModImmBRel + MModImmRegRel);
      switch (AdrType)
      {
        case ModReg:
          BAsmCode[0] = 0x98;
          BAsmCode[1] = AdrVals[0];
          BAsmCode[2] = AdrVal;
          CodeLen = 3;
          break;
        case ModImm:
          BAsmCode[0] = 0x88;
          memcpy(BAsmCode + 1, AdrVals, 2);
          BAsmCode[3] = AdrVal;
          CodeLen = 4;
          break;
        case ModImmBRel:
          BAsmCode[0] = 0xa8;
          memcpy(BAsmCode + 1, AdrVals, 2);
          BAsmCode[3] = AdrVal;
          CodeLen = 4;
          break;
        case ModImmRegRel:
          BAsmCode[0] = 0xf4;
          BAsmCode[1] = 0xe8;
          memcpy(BAsmCode + 2, AdrVals, 2);
          BAsmCode[4] = AdrVal;
          CodeLen = 5;
          break;
      }
    }
  }
}

static void DecodeRel8(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[1], Int16, &OK, &Flags) - (EProgCounter() + 2);

    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        CodeLen = 2;
        BAsmCode[0] = Code;
        BAsmCode[1] = AdrInt & 0xff;
      }
    }
  }
}

static void DecodeCMP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModAccA + MModAccB + MModReg);
    switch (AdrType)
    {
      case ModAccA:
        DecodeAdr(&ArgStr[1], MModAbs + MModIReg + MModBRel + MModRegRel + MModSPRel + MModAccB + MModReg + MModImm);
        switch (AdrType)
        {
          case ModAbs:
            BAsmCode[0] = 0x8d;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModIReg:
            BAsmCode[0] = 0x9d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModBRel:
            BAsmCode[0] = 0xad;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModRegRel:
            BAsmCode[0] = 0xf4;
            BAsmCode[1] = 0xed;
            memcpy(BAsmCode + 2, AdrVals, 2);
            CodeLen = 4;
            break;
          case ModSPRel:
            BAsmCode[0] = 0xf3;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModAccB:
            BAsmCode[0] = 0x6d;
            CodeLen = 1;
            break;
          case ModReg:
            BAsmCode[0] = 0x1d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x2d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModAccB:
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x3d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x5d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModReg:
        BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x4d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = 0x7d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
    }
  }
}

static void DecodeALU1(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModAccA + MModAccB + MModReg);
    switch (AdrType)
    {
      case ModAccA:
        DecodeAdr(&ArgStr[1], MModAccB + MModReg + MModImm);
        switch (AdrType)
        {
          case ModAccB:
            CodeLen = 1;
            BAsmCode[0] = 0x60 + Code;
            break;
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0x10 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0x20 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;
      case ModAccB:
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 2;
            BAsmCode[0] = 0x30 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0x50 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;
      case ModReg:
        BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            CodeLen = 3;
            BAsmCode[0] = 0x40 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModImm:
            CodeLen = 3;
            BAsmCode[0] = 0x70 + Code;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;
    }
  }
}

static void DecodeALU2(Word Code)
{
  Boolean Rela = Hi(Code) != 0;
  Code &= 0xff;

  if (ChkArgCnt(Rela ? 3 : 2, Rela ? 3 : 2))
  {
    DecodeAdr(&ArgStr[2], MModAccA + MModAccB + MModReg + MModPort);
    switch (AdrType)
    {
      case ModAccA:
        DecodeAdr(&ArgStr[1], MModAccB + MModReg + MModImm);
        switch (AdrType)
        {
          case ModAccB:
            BAsmCode[0] = 0x60 + Code;
            CodeLen = 1;
            break;
          case ModReg:
            BAsmCode[0] = 0x10 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x20 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModAccB:
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x30 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x50 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;
      case ModReg:
        BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModReg + MModImm);
        switch (AdrType)
        {
          case ModReg:
            BAsmCode[0] = 0x40 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModImm:
            BAsmCode[0] = 0x70 + Code;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
      case ModPort:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[1], MModAccA + MModAccB + MModImm);
        switch (AdrType)
        {
          case ModAccA:
            BAsmCode[0] = 0x80 + Code;
            CodeLen = 2;
            break;
          case ModAccB:
            BAsmCode[0] = 0x90 + Code;
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0xa0 + Code;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;
    }
    if ((CodeLen != 0) && (Rela))
    {
      Boolean OK;
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrError(ErrNum_JmpDistTooBig);
        CodeLen = 0;
      }
      else
        BAsmCode[CodeLen++] = AdrInt & 0xff;
    }
  }
}

static void DecodeJmp(Word Code)
{
  Boolean AddrRel = Hi(Code) != 0;
  Code &= 0xff;

  if (ChkArgCnt(1, 1))
  {
    DecodeAdrRel(&ArgStr[1], MModAbs + MModIReg + MModBRel + MModRegRel, AddrRel);
    switch (AdrType)
    {
      case ModAbs:
        CodeLen = 3;
        BAsmCode[0] = 0x80 + Code;
        memcpy(BAsmCode + 1, AdrVals, 2);
        break;
      case ModIReg:
        CodeLen = 2;
        BAsmCode[0] = 0x90 + Code;
        BAsmCode[1] = AdrVals[0];
        break;
      case ModBRel:
        CodeLen = 3;
        BAsmCode[0] = 0xa0 + Code;
        memcpy(BAsmCode + 1, AdrVals, 2);
        break;
      case ModRegRel:
        CodeLen = 4;
        BAsmCode[0] = 0xf4;
        BAsmCode[1] = 0xe0 + Code;
        memcpy(BAsmCode + 2, AdrVals, 2);
        break;
    }
  }
}

static void DecodeABReg(Word Code)
{
  int IsDJNZ = Hi(Code) & 1;
  Boolean IsStack = (Code & 0x200) || False;

  Code &= 0xff;

  if (!ChkArgCnt(1 + IsDJNZ, 1 + IsDJNZ));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "ST"))
  {
    if (IsStack)
    {
      BAsmCode[0] = 0xf3 + Code;
      CodeLen = 1;
    }
    else
      WrError(ErrNum_InvAddrMode);
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModAccA + MModAccB + MModReg);
    switch (AdrType)
    {
      case ModAccA:
        BAsmCode[0] = 0xb0 + Code;
        CodeLen = 1;
        break;
      case ModAccB:
        BAsmCode[0] = 0xc0 + Code;
        CodeLen = 1;
        break;
      case ModReg:
        BAsmCode[0] = 0xd0 + Code;
        BAsmCode[CodeLen + 1] = AdrVals[0];
        CodeLen = 2;
        break;
    }
    if ((IsDJNZ) && (CodeLen != 0))
    {
      Boolean OK;
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[2], Int16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrError(ErrNum_JmpDistTooBig);
        CodeLen = 0;
      }
      else
        BAsmCode[CodeLen++] = AdrInt & 0xff;
    }
  }
}

static void DecodeBit(Word Code)
{
  int Rela = Hi(Code);
  LongWord BitExpr;

  Code &= 0xff;

  if (ChkArgCnt(1 + Rela, 2 + Rela)
   && DecodeBitExpr(1, ArgCnt - Rela, &BitExpr))
  {
    Boolean OK;
    Word Addr;
    Byte Bit;

    DissectBitValue(BitExpr, &Addr, &Bit);

    BAsmCode[1] = 1 << Bit;
    BAsmCode[2] = Lo(Addr);
    switch (Hi(Addr))
    {
      case 0x00:
        BAsmCode[0] = 0x70 + Code;
        CodeLen = 3;
        break;
      case 0x10:
        BAsmCode[0] = 0xa0 + Code;
        CodeLen = 3;
        break;
      default:
        WrStrErrorPos(ErrNum_InvAddrMode, &ArgStr[ArgCnt - 1]);
    }
    if ((CodeLen != 0) && Rela)
    {
      tSymbolFlags Flags;
      Integer AdrInt = EvalStrIntExpressionWithFlags(&ArgStr[ArgCnt], Int16, &OK, &Flags) - (EProgCounter() + CodeLen + 1);

      if (!OK)
        CodeLen = 0;
      else if (!mSymbolQuestionable(Flags) && ((AdrInt > 127) || (AdrInt < -128)))
      {
        WrError(ErrNum_JmpDistTooBig);
        CodeLen = 0;
      }
      else
        BAsmCode[CodeLen++] = AdrInt & 0xff;
    }
  }
}

static void DecodeDIV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModAccA);
    if (AdrType != ModNone)
    {
      DecodeAdr(&ArgStr[1], MModReg);
      if (AdrType != ModNone)
      {
        BAsmCode[0] = 0xf4;
        BAsmCode[1] = 0xf8;
        BAsmCode[2] = AdrVals[0];
        CodeLen = 3;
      }
    }
  }
}

static void DecodeINCW(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModReg);
    if (AdrType != ModNone)
    {
      BAsmCode[2] = AdrVals[0];
      DecodeAdr(&ArgStr[1], MModImm);
      if (AdrType != ModNone)
      {
        BAsmCode[0] = 0x70;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 3;
      }
    }
  }
}

static void DecodeLDST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModImm);
    if (AdrType != ModNone)
    {
      BAsmCode[0] = 0xf0;
      BAsmCode[1] = AdrVals[0];
      CodeLen = 2;
    }
  }
}

static void DecodeTRAP(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;

    BAsmCode[0] = EvalStrIntExpression(&ArgStr[1], Int4, &OK);
    if (OK)
    {
      BAsmCode[0] = 0xef - BAsmCode[0];
      CodeLen = 1;
    }
  }
}

static void DecodeTST(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAccA + MModAccB);
    switch (AdrType)
    {
      case ModAccA:
        BAsmCode[0] = 0xb0;
        CodeLen = 1;
        break;
      case ModAccB:
        BAsmCode[0] = 0xc6;
        CodeLen = 1;
        break;
    }
  }
}

/****************************************************************************/

static void InitFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void InitRel8(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeRel8);
}

static void InitALU1(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU1);
}

static void InitALU2(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU2);
}

static void InitJmp(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeJmp);
}

static void InitABReg(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeABReg);
}

static void InitBit(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBit);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(203);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVW", 0, DecodeMOVW);
  AddInstTable(InstTable, "CMP", 0, DecodeCMP);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "INCW", 0, DecodeINCW);
  AddInstTable(InstTable, "LDST", 0, DecodeLDST);
  AddInstTable(InstTable, "TRAP", 0, DecodeTRAP);
  AddInstTable(InstTable, "TST", 0, DecodeTST);
  AddInstTable(InstTable, "DBIT", 0, DecodeDBIT);

  InitFixed("CLRC" , 0x00b0); InitFixed("DINT" , 0xf000);
  InitFixed("EINT" , 0xf00c); InitFixed("EINTH", 0xf004);
  InitFixed("EINTL", 0xf008); InitFixed("IDLE" , 0x00f6);
  InitFixed("LDSP" , 0x00fd); InitFixed("NOP"  , 0x00ff);
  InitFixed("RTI"  , 0x00fa); InitFixed("RTS"  , 0x00f9);
  InitFixed("SETC" , 0x00f8); InitFixed("STSP" , 0x00fe);

  InitRel8("JMP", 0x00); InitRel8("JC" , 0x03); InitRel8("JEQ", 0x02);
  InitRel8("JG" , 0x0e); InitRel8("JGE", 0x0d); InitRel8("JHS", 0x0b);
  InitRel8("JL" , 0x09); InitRel8("JLE", 0x0a); InitRel8("JLO", 0x0f);
  InitRel8("JN" , 0x01); InitRel8("JNC", 0x07); InitRel8("JNE", 0x06);
  InitRel8("JNV", 0x0c); InitRel8("JNZ", 0x06); InitRel8("JP" , 0x04);
  InitRel8("JPZ", 0x05); InitRel8("JV" , 0x08); InitRel8("JZ" , 0x02);

  InitALU1("ADC",  9); InitALU1("ADD",  8);
  InitALU1("DAC", 14); InitALU1("DSB", 15);
  InitALU1("SBB", 11); InitALU1("SUB", 10); InitALU1("MPY", 12);

  InitALU2("AND" ,  3); InitALU2("BTJO",  0x0106);
  InitALU2("BTJZ",  0x0107); InitALU2("OR"  ,  4); InitALU2("XOR",  5);

  InitJmp("BR"  , 12); InitJmp("CALL" , 14);
  InitJmp("JMPL", 0x0109); InitJmp("CALLR", 0x010f);

  InitABReg("CLR"  ,  5); InitABReg("COMPL", 11); InitABReg("DEC"  ,  2);
  InitABReg("INC"  ,  3); InitABReg("INV"  ,  4); InitABReg("POP"  , 0x0209);
  InitABReg("PUSH" , 0x0208); InitABReg("RL"   , 14); InitABReg("RLC"  , 15);
  InitABReg("RR"   , 12); InitABReg("RRC"  , 13); InitABReg("SWAP" ,  7);
  InitABReg("XCHB" ,  6); InitABReg("DJNZ" , 0x010a);

  InitBit("CMPBIT",  5); InitBit("JBIT0" ,  0x0107); InitBit("JBIT1" ,  0x0106);
  InitBit("SBIT0" ,  3); InitBit("SBIT1" ,  4);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/****************************************************************************/

static void MakeCode_370(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = 0;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(True))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_370(void)
{
  return (Memo("DBIT"));
}

static void InternSymbol_370(char *Asc,  TempResult *Erg)
{
  Boolean OK;
  String h;
  LargeInt Num;

  as_tempres_set_none(Erg);
  if ((strlen(Asc) < 2) || ((as_toupper(*Asc) != 'R') && (as_toupper(*Asc) != 'P')))
    return;

  strcpy(h, Asc + 1);
  if ((*h == '0') && (strlen(h) > 1))
    *h = '$';
  Num = ConstLongInt(h, &OK, 10);
  if (!OK || (Num < 0) || (Num > 255))
    return;

  if (as_toupper(*Asc) == 'P')
    Num += 0x1000;
  as_tempres_set_int(Erg, Num);
}

static void SwitchFrom_370(void)
{
  DeinitFields();
}

static void SwitchTo_370(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = 0x49;
  NOPCode = 0xff;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode ] = 1;
  ListGrans[SegCode ] = 1;
  SegInits[SegCode ] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_370;
  IsDef = IsDef_370;
  SwitchFrom = SwitchFrom_370;
  InternSymbol = InternSymbol_370;
  DissectBit = DissectBit_370;

  InitFields();
}

void code370_init(void)
{
  CPU37010 = AddCPU("370C010" , SwitchTo_370);
  CPU37020 = AddCPU("370C020" , SwitchTo_370);
  CPU37030 = AddCPU("370C030" , SwitchTo_370);
  CPU37040 = AddCPU("370C040" , SwitchTo_370);
  CPU37050 = AddCPU("370C050" , SwitchTo_370);
}
