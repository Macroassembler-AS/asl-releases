/* codefmc16.c */
/****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS, C-Version                                                            */
/*                                                                          */
/* Codegenerator fuer Fujitsu-F2MC16L-Prozessoren                           */
/*                                                                          */
/****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmpars.h"
#include "asmsub.h"
#include "asmallg.h"
#include "onoff_common.h"
#include "errmsg.h"
#include "codepseudo.h"
#include "intpseudo.h"
#include "asmitree.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "codefmc16.h"

/*--------------------------------------------------------------------------*/
/* Definitionen */

#define AccOrderCnt 2
#define MulDivOrderCnt 8

typedef struct
{
  Byte Code;
  Word AccCode;
} MulDivOrder;

#define ModNone (-1)
#define ModAcc 0
#define MModAcc (1 << ModAcc)
#define ModReg 1
#define MModReg (1 << ModReg)
#define ModMem 2
#define MModMem (1 << ModMem)
#define ModDir 3
#define MModDir (1 << ModDir)
#define ModImm 4
#define MModImm (1 << ModImm)
#define ModCCR 5
#define MModCCR (1 << ModCCR)
#define ModIO 6
#define MModIO (1 << ModIO)
#define ModSeg 7
#define MModSeg (1 << ModSeg)
#define ModIAcc 8
#define MModIAcc (1 << ModIAcc)
#define ModRDisp 9
#define MModRDisp (1 << ModRDisp)
#define ModSpec 10
#define MModSpec (1 << ModSpec)
#define ModRP 11
#define MModRP (1 << ModRP)
#define ModILM 12
#define MModILM (1 << ModILM)
#define ModSP 13
#define MModSP (1 << ModSP)

#define ABSMODE 0x1f

static CPUVar CPU90500;

static MulDivOrder *MulDivOrders;

static const char BankNames[4][4] =
{
  "PCB", "DTB", "ADB", "SPB"
};

static Byte AdrVals[5], AdrPart, NextDataSeg;
static LongWord CurrBank;
static ShortInt AdrMode, OpSize;

static LongInt Reg_PCB, Reg_DTB, Reg_ADB, Reg_USB, Reg_SSB, Reg_DPR;

#define ASSUMEF2MC16Count 6
static ASSUMERec ASSUMEF2MC16s[ASSUMEF2MC16Count] =
{
  { "PCB"    , &Reg_PCB,     0x00, 0xff, 0x100, NULL },
  { "DTB"    , &Reg_DTB,     0x00, 0xff, 0x100, NULL },
  { "ADB"    , &Reg_ADB,     0x00, 0xff, 0x100, NULL },
  { "USB"    , &Reg_USB,     0x00, 0xff, 0x100, NULL },
  { "SSB"    , &Reg_SSB,     0x00, 0xff, 0x100, NULL },
  { "DPR"    , &Reg_DPR,     0x00, 0xff, 0x100, NULL }
};

/*--------------------------------------------------------------------------*/
/* Adressdekoder */

static void SetOpSize(ShortInt NewSize)
{
  if (OpSize == -1)
    OpSize = NewSize;
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    AdrMode = ModNone; AdrCnt = 0;
  }
}

static Boolean DecodeAdr(const tStrComp *pArg, int Mask)
{
  Integer AdrVal;
  LongWord ImmVal;
  Boolean OK;
  unsigned Index;
  static const char SpecNames[7][4] = {"DTB", "ADB", "SSB", "USB", "DPR", "\a", "PCB"};

  AdrMode = ModNone; AdrCnt = 0;

  /* 1. Sonderregister: */

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    AdrMode = ModAcc;
    goto found;
  }

  if (!as_strcasecmp(pArg->str.p_str, "CCR"))
  {
    AdrMode = ModCCR;
    goto found;
  }

  if (!as_strcasecmp(pArg->str.p_str, "ILM"))
  {
    AdrMode = ModILM;
    goto found;
  }

  if (!as_strcasecmp(pArg->str.p_str, "RP"))
  {
    AdrMode = ModRP;
    goto found;
  }

  if (!as_strcasecmp(pArg->str.p_str, "SP"))
  {
    AdrMode = ModSP;
    goto found;
  }

  if (Mask & MModSeg)
  {
    for (Index = 0; Index < sizeof(BankNames) / sizeof(BankNames[0]); Index++)
      if (!as_strcasecmp(pArg->str.p_str, BankNames[Index]))
      {
        AdrMode = ModSeg;
        AdrPart = Index;
        goto found;
      }
  }

  if (Mask & MModSpec)
  {
    for (Index = 0; Index < sizeof(SpecNames) / sizeof(SpecNames[0]); Index++)
      if (as_strcasecmp(pArg->str.p_str, SpecNames[Index]) == 0)
      {
        AdrMode = ModSpec;
        AdrPart = Index;
        goto found;
      }
  }

  /* 2. Register: */

  if (as_toupper(*pArg->str.p_str) == 'R')
  {
    switch(as_toupper(pArg->str.p_str[1]))
    {
      case 'W':
        if ((pArg->str.p_str[3] == '\0') && (pArg->str.p_str[2] >= '0') && (pArg->str.p_str[2] <= '7'))
        {
          AdrPart = pArg->str.p_str[2] - '0';
          AdrMode = ModReg;
          SetOpSize(1);
          goto found;
        }
        break;
      case 'L':
        if ((pArg->str.p_str[3] == '\0') && (pArg->str.p_str[2] >= '0') && (pArg->str.p_str[2] <= '3'))
        {
          AdrPart = (pArg->str.p_str[2] - '0') << 1;
          AdrMode = ModReg;
          SetOpSize(2);
          goto found;
        }
        break;
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        if (pArg->str.p_str[2] == '\0')
        {
          AdrPart = pArg->str.p_str[1] - '0';
          AdrMode = ModReg;
          SetOpSize(0);
          goto found;
        }
    }
  }

  /* 3. 32-Bit-Register indirekt: */

  if ((*pArg->str.p_str == '(')
   && (as_toupper(pArg->str.p_str[1]) == 'R')
   && (as_toupper(pArg->str.p_str[2]) == 'L')
   && (pArg->str.p_str[3] >= '0') && (pArg->str.p_str[3] <= '3')
   && (pArg->str.p_str[4] == ')')
   && (pArg->str.p_str[5] == '\0'))
  {
    AdrPart = ((pArg->str.p_str[3] - '0') << 1) + 1;
    AdrMode = ModMem;
    goto found;
  }

  /* 4. immediate: */

  else if (*pArg->str.p_str == '#')
  {
    if (OpSize == -1) WrError(ErrNum_UndefOpSizes);
    else
    {
      ImmVal = EvalStrIntExpressionOffs(pArg, 1, (OpSize == 2) ? Int32 : ((OpSize == 1) ? Int16 : Int8), &OK);
      if (OK)
      {
        AdrMode = ModImm;
        AdrVals[AdrCnt++] = ImmVal & 0xff;
        if (OpSize >= 1)
          AdrVals[AdrCnt++] = (ImmVal >> 8) & 0xff;
        if (OpSize >= 2)
        {
          AdrVals[AdrCnt++] = (ImmVal >> 16) & 0xff;
          AdrVals[AdrCnt++] = (ImmVal >> 24) & 0xff;
        }
      }
    }

    goto found;
  }

  /* 5. indirekt: */

  if (*pArg->str.p_str == '@')
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);

    /* Akku-indirekt: */

    if (!as_strcasecmp(Arg.str.p_str, "A"))
    {
      AdrMode = ModIAcc;
    }

    /* PC-relativ: */

    else if (as_strncasecmp(Arg.str.p_str, "PC", 2) == 0)
    {
      tStrComp RegComp;

      StrCompRefRight(&RegComp, &Arg, 2);
      AdrPart = 0x1e;
      if ((*RegComp.str.p_str == '+') || (*RegComp.str.p_str == '-') || (as_isspace(*RegComp.str.p_str)))
      {
        AdrVal = EvalStrIntExpression(&RegComp, SInt16, &OK);
        if (OK)
        {
          AdrVals[0] = AdrVal & 0xff;
          AdrVals[1] = (AdrVal >> 8) & 0xff;
          AdrCnt = 2;
          AdrMode = ModMem;
        }
      }
      else if (*RegComp.str.p_str == '\0')
      {
        AdrVals[0] = AdrVals[1] = 0;
        AdrCnt = 2;
        AdrMode = ModMem;
      }
      else
        WrStrErrorPos(ErrNum_InvReg, &Arg);
    }

    /* base register, 32 bit: */

    else if ((as_toupper(*Arg.str.p_str) == 'R')
          && (as_toupper(Arg.str.p_str[1]) == 'L')
          && (Arg.str.p_str[2] >= '0') && (Arg.str.p_str[2] <= '3'))
    {
      AdrVal = EvalStrIntExpressionOffs(&Arg, 3, SInt8, &OK);
      if (OK)
      {
        AdrVals[0] = AdrVal & 0xff;
        AdrCnt = 1;
        AdrPart = Arg.str.p_str[2] - '0';
        AdrMode = ModRDisp;
      }
    }

    /* base register, 16 bit: */

    else if ((as_toupper(*Arg.str.p_str) == 'R') && (as_toupper(Arg.str.p_str[1]) == 'W') &&
             (Arg.str.p_str[2] >= '0') && (Arg.str.p_str[2] <= '7'))
    {
      tStrComp IComp;

      AdrPart = Arg.str.p_str[2] - '0';
      StrCompRefRight(&IComp, &Arg, 3);
      switch (*IComp.str.p_str)
      {
        case '\0':                          /* no displacement             */
          if (AdrPart < 4)                  /* RW0..RW3 directly available */
          {
            AdrPart += 8;
            AdrMode = ModMem;
          }
          else                              /* dummy disp for RW4..RW7     */
          {
            AdrPart += 0x10;
            AdrVals[0] = 0; AdrCnt = 1;
            AdrMode = ModMem;
          }
          break;
        case '+':
          if (IComp.str.p_str[1] == '\0')          /* postincrement               */
          {
            if (AdrPart > 3) WrError(ErrNum_InvReg);  /* only allowed for RW0..RW3   */
            else
            {
              AdrPart += 0x0c;
              AdrMode = ModMem;
            }
            break;
          }                               /* run into disp part otherwise*/
          /* else fall-through */
        case ' ':
        case '\t':
        case '-':
          while (as_isspace(*IComp.str.p_str))  /* skip leading spaces         */
            StrCompIncRefLeft(&IComp, 1);
          if (!as_strcasecmp(IComp.str.p_str, "+RW7"))  /* base + RW7 as index         */
          {
            if (AdrPart > 1) WrError(ErrNum_InvReg);
            else
            {
              AdrPart += 0x1c;
              AdrMode = ModMem;
            }
          }
          else                               /* numeric index               */
          {
            AdrVal =                         /* max length depends on base  */
              EvalStrIntExpression(&IComp, (AdrPart > 3) ? SInt8 : SInt16, &OK);
            if (OK)
            {                           /* optimize length             */
              AdrVals[0] = AdrVal & 0xff;
              AdrCnt = 1;
              AdrMode = ModMem;
              AdrPart |= 0x10;
              if ((AdrVal < -0x80) || (AdrVal > 0x7f))
              {
                AdrVals[AdrCnt++] = (AdrVal >> 8) & 0xff;
                AdrPart |= 0x08;
              }
            }
          }
          break;
        default:
          WrError(ErrNum_InvAddrMode);
      }
    }

    else
      WrStrErrorPos(ErrNum_InvReg, &Arg);

    goto found;
  }

  /* 6. dann direkt: */

  ImmVal = EvalStrIntExpression(pArg, UInt24, &OK);
  if (OK)
  {
    AdrVals[AdrCnt++] = ImmVal & 0xff;
    if (((ImmVal >> 8) == 0) && (Mask & MModIO))
      AdrMode = ModIO;
    else if ((Lo(ImmVal >> 8) == Reg_DPR) && ((ImmVal & 0xffff0000) == CurrBank) && (Mask & MModDir))
      AdrMode = ModDir;
    else
    {
      AdrVals[AdrCnt++] = (ImmVal >> 8) & 0xff;
      AdrPart = ABSMODE;
      AdrMode = ModMem;
      if ((ImmVal & 0xffff0000) != CurrBank) WrError(ErrNum_InAccPage);
    }
  }

found:
  if ((AdrMode != ModNone) && ((Mask & (1 << AdrMode)) == 0))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone; AdrCnt = 0;
  }

  return (AdrMode != ModNone);
}

static Boolean SplitBit(tStrComp *pArg, Byte *Result)
{
  char *pos;
  Boolean Res = FALSE;

  pos = RQuotPos(pArg->str.p_str, ':');
  if (pos == NULL)
  {
    *Result = 0;
    WrError(ErrNum_InvBitPos);
  }
  else
  {
    tStrComp BitArg;

    StrCompSplitRef(pArg, &BitArg, pArg, pos);
    *Result = EvalStrIntExpression(&BitArg, UInt3, &Res);
  }

  return Res;
}

static void CopyVals(int Offset)
{
  memcpy(BAsmCode + Offset, AdrVals, AdrCnt);
  CodeLen = Offset + AdrCnt;
}

/*--------------------------------------------------------------------------*/
/* Dekoder fuer einzelne Instruktionen */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeALU8(Word Code)
{
  int HCnt;

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(0);
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModReg | MModMem | MModImm | MModDir);
        switch (AdrMode)
        {
          case ModImm:
           BAsmCode[0] = 0x30 + Code;
           BAsmCode[1] = AdrVals[0];
           CodeLen = 2;
           break;
          case ModDir:
           BAsmCode[0] = 0x20 + Code;
           BAsmCode[1] = AdrVals[0];
           CodeLen = 2;
           break;
          case ModReg:
          case ModMem:
          {
            BAsmCode[0] = 0x74;
            BAsmCode[1] = (Code << 5) | AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
          }
        }
        break;
      case ModReg:
      case ModMem:
        HCnt = AdrCnt;
        BAsmCode[1] = (Code << 5) | AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        if (DecodeAdr(&ArgStr[2], MModAcc))
        {
          BAsmCode[0] = 0x75;
          CodeLen = 2 + HCnt;
        }
        break;
    }
  }
}

static void DecodeLog8(Word Code)
{
  int HCnt;

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(0);
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModMem | ((Code < 6) ? MModCCR : 0));
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModReg | MModMem | MModImm);
        switch (AdrMode)
        {
          case ModImm:
           BAsmCode[0] = 0x30 + Code;
           BAsmCode[1] = AdrVals[0];
           CodeLen = 2;
           break;
          case ModReg:
          case ModMem:
          {
            BAsmCode[0] = 0x74;
            BAsmCode[1] = (Code << 5) | AdrPart;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
          }
        }
        break;
      case ModReg:
      case ModMem:
        HCnt = AdrCnt;
        BAsmCode[1] = (Code << 5) | AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        if (DecodeAdr(&ArgStr[2], MModAcc))
        {
          BAsmCode[0] = 0x75;
          CodeLen = 2 + HCnt;
        }
        break;
      case ModCCR:
        if (DecodeAdr(&ArgStr[2], MModImm))
        {
          BAsmCode[0] = 0x20 + Code;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
        }
        break;
    }
  }
}

static void DecodeCarry8(Word Index)
{
  if (ChkArgCnt(1, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (ArgCnt == 1)
    {
      BAsmCode[0] = 0x22 + (Index << 4);
      CodeLen = 1;
    }
    else if (DecodeAdr(&ArgStr[2], MModReg | MModMem))
    {
      BAsmCode[0] = 0x74 + Index;
      BAsmCode[1] = AdrPart | 0x40;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeCarry16(Word Index)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc)
   && DecodeAdr(&ArgStr[2], MModReg | MModMem))
  {
    BAsmCode[0] = 0x76 + Index;
    BAsmCode[1] = AdrPart | 0x40;
    memcpy(BAsmCode + 2, AdrVals, AdrCnt);
    CodeLen = 2 + AdrCnt;
  }
}

static void DecodeAcc(Word Code)
{
  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    BAsmCode[0] = Code;
    CodeLen = 1;
  }
}

static void DecodeShift(Word Code)
{
  if (ChkArgCnt(Hi(Code) ? 1 : 2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (ArgCnt == 1)
    {
      BAsmCode[0] = Lo(Code);
      CodeLen = 1;
    }
    else
    {
      SetOpSize(0);
      if (DecodeAdr(&ArgStr[2], MModReg))
      {
        if (AdrPart != 0) WrStrErrorPos(ErrNum_InvReg, &ArgStr[2]);
        else
        {
          BAsmCode[0] = 0x6f;
          BAsmCode[1] = Lo(Code);
          CodeLen = 2;
        }
      }
    }
  }
}

static void DecodeAdd32(Word Index)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    SetOpSize(2);
    if (DecodeAdr(&ArgStr[2], MModImm | MModMem | MModReg))
    {
      switch (AdrMode)
      {
        case ModImm:
          BAsmCode[0] = 0x18 + Index;
          memcpy(BAsmCode + 1, AdrVals, AdrCnt);
          CodeLen = 1 + AdrCnt;
          break;
        case ModReg:
        case ModMem:
          BAsmCode[0] = 0x70;
          BAsmCode[1] = AdrPart | (Index << 5);
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          CodeLen = 2 + AdrCnt;
          break;
      }
    }
  }
}

static void DecodeLog32(Word Index)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (DecodeAdr(&ArgStr[2], MModMem | MModReg))
    {
      BAsmCode[0] = 0x70;
      BAsmCode[1] = AdrPart | (Index << 5);
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeADDSP(Word Index)
{
  Integer Val;
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    Val = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);
    if (OK)
    {
      BAsmCode[CodeLen++] = 0x17;
      BAsmCode[CodeLen++] = Val & 0xff;
      if ((Val > 127) || (Val < -128))
      {
        BAsmCode[CodeLen++] = (Val >> 8) & 0xff;
        BAsmCode[0] |= 8;
      }
    }
  }
}

static void DecodeAdd16(Word Index)
{
  int HCnt;

  if (!ChkArgCnt(1, 2));
  else if (ArgCnt == 1)
  {
    if (DecodeAdr(&ArgStr[1], MModAcc))
    {
      BAsmCode[0] = 0x28 | Index;
      CodeLen = 1;
    }
  }
  else
  {
    SetOpSize(1);
    DecodeAdr(&ArgStr[1], MModAcc | ((Index != 3) ? (MModMem | MModReg) : 0));
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModImm | MModMem | MModReg);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x38 | Index;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 1 + AdrCnt;
            break;
          case ModMem:
          case ModReg:
            BAsmCode[0] = 0x76;
            BAsmCode[1] = AdrPart | (Index << 5);
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      case ModMem:
      case ModReg:
        BAsmCode[0] = 0x77;
        BAsmCode[1] = AdrPart | (Index << 5);
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        HCnt = 2 + AdrCnt;
        if (DecodeAdr(&ArgStr[2], MModAcc))
          CodeLen = HCnt;
        break;
    }
  }
}

static void DecodeBBcc(Word Index)
{
  Byte BitPos, HLen;
  LongInt Addr;
  Boolean OK;

  if ((ChkArgCnt(2, 2))
   && (SplitBit(&ArgStr[1], &BitPos)))
  {
    HLen = 0;
    DecodeAdr(&ArgStr[1], MModMem | MModDir | MModIO);
    if (AdrMode != ModNone)
    {
      BAsmCode[HLen++] = 0x6c;
      switch (AdrMode)
      {
        case ModDir:
          BAsmCode[HLen++] = Index + 8 + BitPos;
          BAsmCode[HLen++] = AdrVals[0];
          break;
        case ModIO:
          BAsmCode[HLen++] = Index + BitPos;
          BAsmCode[HLen++] = AdrVals[0];
          break;
        case ModMem:
          if (AdrPart != ABSMODE) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[HLen++] = Index + 16 + BitPos;
            memcpy(BAsmCode + HLen, AdrVals, AdrCnt);
            HLen += AdrCnt;
          }
          break;
      }
      if (HLen > 1)
      {
        tSymbolFlags Flags;

        Addr = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt24, &OK, &Flags) - (EProgCounter() + HLen + 1);
        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[HLen++] = Addr & 0xff;
            CodeLen = HLen;
          }
        }
      }
    }
  }
}

static void DecodeBranch(Word Code)
{
  LongInt Addr;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = Addr & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeJmp16(Word Index)
{
  LongWord Addr;
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str == '@')
  {
    tStrComp Arg1;

    SetOpSize(1);
    StrCompRefRight(&Arg1, &ArgStr[1], 1);
    DecodeAdr(&Arg1, MModReg | ((Index == 0) ? MModAcc : 0) | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        BAsmCode[0] = 0x61;
        CodeLen = 1;
        break;
      case ModReg:
      case ModMem:
        BAsmCode[0] = 0x73;
        BAsmCode[1] = (Index << 4) | AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
        break;
    }
  }
  else
  {
    Addr = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x62 + Index;
      BAsmCode[1] = Addr & 0xff;
      BAsmCode[2] = (Addr >> 8) & 0xff;
      CodeLen = 3;
      if (((Addr >> 16) & 0xff) != (LongWord)Reg_PCB)
        WrError(ErrNum_InAccSegment);
    }
  }
}

static void DecodeJmp24(Word Index)
{
  LongWord Addr;
  Boolean OK;

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str == '@')
  {
    tStrComp Arg1;

    SetOpSize(2);
    StrCompRefRight(&Arg1, &ArgStr[1], 1);
    if (DecodeAdr(&Arg1, MModReg | MModMem))
    {
      BAsmCode[0] = 0x71;
      BAsmCode[1] = (Index << 4) | AdrPart;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
  else
  {
    Addr = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x63 + Index;
      BAsmCode[1] = Addr & 0xff;
      BAsmCode[2] = (Addr >> 8) & 0xff;
      BAsmCode[3] = (Addr >> 16) & 0xff;
      CodeLen = 4;
    }
  }
}

static void DecodeCALLV(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
  else
  {
    BAsmCode[0] = 0xe0 + EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt4, &OK);
    if (OK) CodeLen = 1;
  }
}

static void DecodeCmpBranch(Word Index)
{
  LongInt Addr;
  int HCnt;
  Boolean OK;

  if (ChkArgCnt(3, 3))
  {
    SetOpSize(Index); HCnt = 0;
    if (DecodeAdr(&ArgStr[1], MModMem | MModReg | MModAcc))
    {
      OK = TRUE;
      switch (AdrMode)
      {
        case ModAcc:
          BAsmCode[HCnt++] = 0x2a + (Index << 4);
          break;
        case ModReg:
        case ModMem:
          if ((AdrPart >= 0x0c) && (AdrPart <= 0x0f))
          {
            WrError(ErrNum_InvAddrMode); OK = FALSE;
          }
          BAsmCode[HCnt++] = 0x70;
          BAsmCode[HCnt++] = 0xe0 - (Index * 0xa0) + AdrPart;
          memcpy(BAsmCode + HCnt, AdrVals, AdrCnt);
          HCnt += AdrCnt;
          break;
      }
      if ((OK) && (DecodeAdr(&ArgStr[2], MModImm)))
      {
        tSymbolFlags Flags;

        memcpy(BAsmCode + HCnt, AdrVals, AdrCnt);
        HCnt += AdrCnt;
        Addr = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt24, &OK, &Flags) - (EProgCounter() + HCnt + 1);
        if (OK)
        {
          if (!mSymbolQuestionable(Flags) && ((Addr > 127) || (Addr < -128))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[HCnt++] = Addr & 0xff;
            CodeLen = HCnt;
          }
        }
      }
    }
  }
}

static void DecodeBit(Word Index)
{
  Byte BitPos;

  if ((ChkArgCnt(1, 1))
   && (SplitBit(&ArgStr[1], &BitPos)))
  {
    DecodeAdr(&ArgStr[1], MModMem | MModDir | MModIO);
    if (AdrMode != ModNone)
    {
      BAsmCode[CodeLen++] = 0x6c;
      switch (AdrMode)
      {
        case ModDir:
          BAsmCode[CodeLen++] = Index + 8 + BitPos;
          BAsmCode[CodeLen++] = AdrVals[0];
          break;
        case ModIO:
          BAsmCode[CodeLen++] = Index + BitPos;
          BAsmCode[CodeLen++] = AdrVals[0];
          break;
        case ModMem:
          if (AdrPart != ABSMODE) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[CodeLen++] = Index + 16 + BitPos;
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
          }
          break;
      }
    }
  }
}

static void DecodeCMP(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (ArgCnt == 1)
    {
      BAsmCode[0] = 0x23;
      CodeLen = 1;
    }
    else
    {
      SetOpSize(0);
      DecodeAdr(&ArgStr[2], MModMem | MModReg | MModImm);
      switch (AdrMode)
      {
        case ModMem:
        case ModReg:
          BAsmCode[0] = 0x74;
          BAsmCode[1] = AdrPart | 0x40;
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          CodeLen = 2 + AdrCnt;
          break;
        case ModImm:
          BAsmCode[0] = 0x33;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
          break;
      }
    }
  }
}

static void DecodeDBNZ(Word Index)
{
  LongInt Addr;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(Index);
    if (DecodeAdr(&ArgStr[1], MModReg | MModMem))
    {
      tSymbolFlags Flags;

      Addr = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt16, &OK, &Flags) - (EProgCounter() + 3 + AdrCnt);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0x74 + (Index << 1);
          BAsmCode[1] = 0xe0 | AdrPart;
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          BAsmCode[2 + AdrCnt] = Addr & 0xff;
          CodeLen = 3 + AdrCnt;
        }
      }
    }
  }
}

static void DecodeIncDec(Word Code)
{
  static Byte Sizes[3] = {2, 0, 1};

  if (ChkArgCnt(1, 1))
  {
    SetOpSize(Sizes[(Code & 3) - 1]);
    if (DecodeAdr(&ArgStr[1], MModMem | MModReg))
    {
      BAsmCode[0] = 0x70 | (Code & 15);
      BAsmCode[1] = (Code & 0xf0) | AdrPart;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeMulDiv(Word Index)
{
  MulDivOrder *POrder = MulDivOrders + Index;

  if (ChkArgCnt(1, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (ArgCnt == 1)
    {
      if (POrder->AccCode == 0xfff) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[CodeLen++] = Lo(POrder->AccCode);
        if (Hi(POrder->AccCode) != 0)
         BAsmCode[CodeLen++] = Hi(POrder->AccCode);
      }
    }
    else
    {
      SetOpSize((POrder->Code >> 5) & 1);
      if (DecodeAdr(&ArgStr[2], MModMem | MModReg))
      {
        BAsmCode[0] = 0x78;
        BAsmCode[1] = POrder->Code | AdrPart;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
      }
    }
  }
}

static void DecodeSeg(Word Code)
{
  if (ChkArgCnt(1, 1)
   && DecodeAdr(&ArgStr[1], MModSeg))
  {
    BAsmCode[0] = 0x6e;
    BAsmCode[1] = Code + AdrPart;
    CodeLen = 2;
  }
}

static void DecodeString(Word Code)
{
  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModSeg))
  {
    BAsmCode[1] = AdrPart << 2;
    if (DecodeAdr(&ArgStr[2], MModSeg))
    {
      BAsmCode[1] += Code + AdrPart;
      BAsmCode[0] = 0x6e;
      CodeLen = 2;
    }
  }
}

static void DecodeINT(Word Index)
{
  Boolean OK;
  LongWord Addr;
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str == '#')
  {
    BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], 1, UInt8, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x68;
      CodeLen = 2;
    }
  }
  else
  {
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((Addr & 0xff0000) != 0xff0000))
        WrError(ErrNum_InAccPage);
      BAsmCode[0] = 0x69;
      BAsmCode[1] = Addr & 0xff;
      BAsmCode[2] = (Addr >> 8) & 0xff;
      CodeLen = 3;
    }
  }
}

static void DecodeINTP(Word Index)
{
  Boolean OK;
  LongWord Addr;
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    Addr = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x6a;
      BAsmCode[1] = Addr & 0xff;
      BAsmCode[2] = (Addr >> 8) & 0xff;
      BAsmCode[3] = (Addr >> 16) & 0xff;
      CodeLen = 4;
    }
  }
}

static void DecodeJCTX(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (*ArgStr[1].str.p_str != '@') WrError(ErrNum_InvAddrMode);
  else
  {
    tStrComp Arg1;

    StrCompRefRight(&Arg1, &ArgStr[1], 1);
    if (DecodeAdr(&Arg1, MModAcc))
    {
      BAsmCode[0] = 0x13;
      CodeLen = 1;
    }
  }
}

static void DecodeLINK(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    SetOpSize(0);
    if (DecodeAdr(&ArgStr[1], MModImm))
    {
      BAsmCode[0] = 0x08;
      BAsmCode[1] = AdrVals[0];
      CodeLen = 2;
    }
  }
}

static void DecodeMOV(Word Index)
{
  Byte HPart, HCnt;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (((!as_strcasecmp(ArgStr[1].str.p_str, "@AL")) || (!as_strcasecmp(ArgStr[1].str.p_str, "@A")))
       && ((!as_strcasecmp(ArgStr[2].str.p_str, "AH" )) || (!as_strcasecmp(ArgStr[2].str.p_str, "T" ))))
  {
    BAsmCode[0] = 0x6f; BAsmCode[1] = 0x15;
    CodeLen = 2;
  }
  else
  {
    SetOpSize(0);
    DecodeAdr(&ArgStr[1], MModRP | MModILM | MModAcc | MModDir | MModRDisp | MModIO | MModSpec | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModRP:
      {
        if (DecodeAdr(&ArgStr[2], MModImm))
        {
          BAsmCode[0] = 0x0a;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
        }
        break;
      } /* 1 = ModRP */
      case ModILM:
      {
        if (DecodeAdr(&ArgStr[2], MModImm))
        {
          BAsmCode[0] = 0x1a;
          BAsmCode[1] = AdrVals[0];
          CodeLen = 2;
        }
        break;
      } /* 1 = ModILM */
      case ModAcc:
      {
        DecodeAdr(&ArgStr[2], MModImm | MModIAcc | MModRDisp | MModIO | MModMem | MModReg | MModDir | MModSpec);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x42;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModIAcc:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x05;
            CodeLen = 2;
            break;
          case ModRDisp:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x40 | (AdrPart << 1);
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModIO:
            BAsmCode[0] = 0x50;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModMem:
            if (AdrPart == ABSMODE) /* 16 bit absolute */
            {
              BAsmCode[0] = 0x52;
              CodeLen = 1;
            }
            else
            {
              BAsmCode[0] = 0x72;
              BAsmCode[1] = 0x80 + AdrPart;
              CodeLen = 2;
            }
            memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
            CodeLen += AdrCnt;
            break;
          case ModReg:
            BAsmCode[0] = 0x80 | AdrPart;
            CodeLen = 1;
            break;
          case ModDir:
            BAsmCode[0] = 0x40;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModSpec:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x00 + AdrPart;
            CodeLen = 2;
            break;
        }
        break;
      } /* 1 = ModAcc */
      case ModDir:
      {
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x41;
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x44;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModReg:           /* extend to 16 bits */
            BAsmCode[0] = 0x7c;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = (AdrPart << 5) | ABSMODE;
            BAsmCode[3] = Reg_DPR;
            CodeLen = 4;
            break;
        }
        break;
      } /* 1 = ModDir */
      case ModRDisp:
      {
        BAsmCode[2] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModAcc);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x30 | (AdrPart << 1);
            CodeLen = 3;
            break;
        }
        break;
      } /* 1 = ModRDisp */
      case ModIO:
      {
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x51;
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x54;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
          case ModReg:           /* extend to 16 bits - will only work when in Bank 0 */
            BAsmCode[0] = 0x7c;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = (AdrPart << 5) | ABSMODE;
            BAsmCode[3] = 0;
            if (CurrBank != 0) WrError(ErrNum_InAccPage);
            CodeLen = 4;
            break;
        }
        break;
      } /* 1 = ModIO */
      case ModSpec:
        if (AdrPart == 6) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[1] = 0x10 + AdrPart;
          DecodeAdr(&ArgStr[2], MModAcc);
          switch (AdrMode)
          {
            case ModAcc:
              BAsmCode[0] = 0x6f;
              CodeLen = 2;
              break;
          }
        } /* 1 = ModSpec */
        break;
      case ModReg:
      {
        BAsmCode[0] = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg | MModMem);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] += 0x90;
            CodeLen = 1;
            break;
          case ModImm:
            BAsmCode[0] += 0xa0;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModReg:
          case ModMem:
            BAsmCode[1] = (BAsmCode[0] << 5) | AdrPart;
            BAsmCode[0] = 0x7a;
            memcpy(BAsmCode + 2, AdrVals, AdrCnt);
            CodeLen = 2 + AdrCnt;
            break;
        }
        break;
      } /* 1 = ModReg */
      case ModMem:
      {
        HPart = AdrPart; HCnt = AdrCnt;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            if (HPart == ABSMODE)
            {
              memmove(BAsmCode + 1, BAsmCode + 2, 2);
              BAsmCode[0] = 0x53;
              CodeLen = 3;
            }
            else
            {
              BAsmCode[0] = 0x72;
              BAsmCode[1] = 0xa0 | HPart;
              CodeLen = 2 + HCnt;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0x71;
            BAsmCode[1] = 0xc0 | HPart;
            BAsmCode[2 + HCnt] = AdrVals[0];
            CodeLen = 3 + HCnt;
            break;
          case ModReg:
            BAsmCode[0] = 0x7c;
            BAsmCode[1] = (AdrPart << 5) | HPart;
            CodeLen = 2 + HCnt;
        }
        break;
      } /* 1 = ModMem */
    }
  }
}

static void DecodeMOVB(Word Index)
{
  if (ChkArgCnt(2, 2))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      Index = 2;
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "A"))
      Index = 1;
    else
      WrError(ErrNum_InvAddrMode);
    if ((Index > 0)
     && SplitBit(&ArgStr[Index], BAsmCode + 1)
     && DecodeAdr(&ArgStr[Index], MModDir | MModIO | MModMem))
    {
      BAsmCode[0] = 0x6c;
      BAsmCode[1] += (2 - Index) << 5;
      switch (AdrMode)
      {
        case ModDir:
          BAsmCode[1] += 0x08;
          break;
        case ModMem:
          BAsmCode[1] += 0x18;
          if (AdrPart != ABSMODE)
          {
            WrError(ErrNum_InvAddrMode);
            AdrCnt = 0;
          }
          break;
      }
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeMOVEA(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(1);
    DecodeAdr(&ArgStr[1], MModAcc | MModReg);
    switch (AdrMode)
    {
      case ModAcc:
        if (DecodeAdr(&ArgStr[2], MModMem | MModReg))
        {
          BAsmCode[0] = 0x71;
          BAsmCode[1] = 0xe0 | AdrPart;
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          CodeLen = 2 + AdrCnt;
        }
        break;
      case ModReg:
        BAsmCode[1] = AdrPart << 5;
        if (DecodeAdr(&ArgStr[2], MModMem | MModReg))
        {
          BAsmCode[0] = 0x79;
          BAsmCode[1] += AdrPart;
          memcpy(BAsmCode + 2, AdrVals, AdrCnt);
          CodeLen = 2 + AdrCnt;
        }
        break;
    }
  }
}

static void DecodeMOVL(Word Index)
{
  Byte HCnt;
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(2);
    DecodeAdr(&ArgStr[1], MModAcc | MModMem | MModReg);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModMem | MModReg | MModImm);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x4b;
            CopyVals(1);
            break;
          case ModReg:
          case ModMem:
            BAsmCode[0] = 0x71;
            BAsmCode[1] = 0x80 | AdrPart;
            CopyVals(2);
        }
        break;
      case ModReg:
      case ModMem:
        BAsmCode[1] = 0xa0 | AdrPart;
        HCnt = AdrCnt;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        if (DecodeAdr(&ArgStr[2], MModAcc))
        {
          BAsmCode[0] = 0x71;
          CodeLen = 2 + HCnt;
        }
        break;
    }
  }
}

static void DecodeMOVN(Word Index)
{
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    if (*ArgStr[2].str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      BAsmCode[0] = 0xd0 + EvalStrIntExpressionOffs(&ArgStr[2], 1, UInt4, &OK);
      if (OK)
       CodeLen = 1;
    }
  }
}

static void DecodeMOVW(Word Index)
{
  Byte HPart, HCnt;
  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (((!as_strcasecmp(ArgStr[1].str.p_str, "@AL")) || (!as_strcasecmp(ArgStr[1].str.p_str, "@A")))
       && ((!as_strcasecmp(ArgStr[2].str.p_str, "AH" )) || (!as_strcasecmp(ArgStr[2].str.p_str, "T" ))))
  {
    BAsmCode[0] = 0x6f; BAsmCode[1] = 0x1d;
    CodeLen = 2;
  }
  else
  {
    SetOpSize(1);
    DecodeAdr(&ArgStr[1], MModAcc | MModRDisp | MModSP | MModDir | MModIO | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        DecodeAdr(&ArgStr[2], MModImm | MModIAcc | MModRDisp | MModSP | MModIO | MModMem | MModReg | MModDir);
        switch (AdrMode)
        {
          case ModImm:
            BAsmCode[0] = 0x4a;
            CopyVals(1);
            break;
          case ModIAcc:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x0d;
            CodeLen = 2;
            break;
          case ModRDisp:
            BAsmCode[0] = 0x6f;
            BAsmCode[1] = 0x48 + (AdrPart << 1);
            CopyVals(2);
            break;
          case ModSP:
            BAsmCode[0] = 0x46;
            CodeLen = 1;
            break;
          case ModIO:
            BAsmCode[0] = 0x58;
            CopyVals(1);
            break;
          case ModDir:
            BAsmCode[0] = 0x48;
            CopyVals(1);
            break;
          case ModReg:
            BAsmCode[0] = 0x88 + AdrPart;
            CodeLen = 1;
            break;
          case ModMem:
            if (AdrPart == ABSMODE)
            {
              BAsmCode[0] = 0x5a;
              CopyVals(1);
            }
            else if ((AdrPart >= 0x10) && (AdrPart <= 0x17))
            {
              BAsmCode[0] = 0xa8 + AdrPart;
              CopyVals(1);
            }
            else
            {
              BAsmCode[0] = 0x73;
              BAsmCode[1] = 0x80 + AdrPart;
              CopyVals(2);
            }
            break;
        }
        break; /* 1 = ModAcc */
      case ModRDisp:
        BAsmCode[1] = 0x38 + (AdrPart << 1);
        BAsmCode[2] = AdrVals[0];
        if (DecodeAdr(&ArgStr[2], MModAcc))
        {
          BAsmCode[0] = 0x6f;
          CodeLen = 3;
        }
        break; /* 1 = ModRDisp */
      case ModSP:
        if (DecodeAdr(&ArgStr[2], MModAcc))
        {
          BAsmCode[0] = 0x47;
          CodeLen = 1;
        }
        break; /* 1 = ModSP */
      case ModDir:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x49;
            CodeLen = 2;
            break;
          case ModImm:           /* extend to 16 bits */
            BAsmCode[0] = 0x73;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = 0xc0 | ABSMODE;
            BAsmCode[3] = Reg_DPR;
            CopyVals(4);
            break;
          case ModReg:           /* extend to 16 bits */
            BAsmCode[0] = 0x7d;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = (AdrPart << 5) | ABSMODE;
            BAsmCode[3] = Reg_DPR;
            CodeLen = 4;
            break;
        }
        break; /* 1 = ModDir */
      case ModIO:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x59;
            CodeLen = 2;
            break;
          case ModImm:
            BAsmCode[0] = 0x56;
            CopyVals(1);
            break;
          case ModReg:           /* extend to 16 bits - will only work when in Bank 0 */
            BAsmCode[0] = 0x7d;
            BAsmCode[2] = BAsmCode[1];
            BAsmCode[1] = (AdrPart << 5) | ABSMODE;
            BAsmCode[3] = 0;
            if (CurrBank != 0) WrError(ErrNum_InAccPage);
            CodeLen = 4;
            break;
        }
        break; /* 1 = ModIO */
      case ModReg:
        HPart = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg | MModMem);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x98 | HPart;
            CodeLen = 1;
            break;
          case ModImm:
            BAsmCode[0] = 0x98 | HPart;
            CopyVals(1);
            break;
          case ModReg:
          case ModMem:
            BAsmCode[0] = 0x7b;
            BAsmCode[1] = (HPart << 5) | AdrPart;
            CopyVals(2);
            break;
        }
        break; /* 1 = ModReg */
      case ModMem:
        HPart = AdrPart; HCnt = AdrCnt;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModAcc | MModImm | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            if (HPart == ABSMODE)
            {
              BAsmCode[0] = 0x5b;
              memmove(BAsmCode + 1, BAsmCode + 2, HCnt);
              CodeLen = 1 + HCnt;
            }
            else if ((HPart >= 0x10) && (HPart <= 0x17))
            {
              BAsmCode[0] = 0xb8 + AdrPart;
              memmove(BAsmCode + 1, BAsmCode + 2, HCnt);
              CodeLen = 1 + HCnt;
            }
            else
            {
              BAsmCode[0] = 0x73;
              BAsmCode[1] = 0xa0 | AdrPart;
              CodeLen = 2 + HCnt;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0x73;
            BAsmCode[1] = 0xc0 | HPart;
            CopyVals(2 + HCnt);
            break;
          case ModReg:
            BAsmCode[0] = 0x7d;
            BAsmCode[1] = (AdrPart << 5) | HPart;
            CodeLen = 2 + HCnt;
        }
        break; /* 1 = ModMem */
    }
  }
}

static void DecodeMOVX(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    SetOpSize(0);
    DecodeAdr(&ArgStr[2], MModImm | MModIAcc | MModRDisp | MModDir | MModIO | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModImm:
        BAsmCode[0] = 0x43;
        CopyVals(1);
        break;
      case ModIAcc:
        BAsmCode[0] = 0x6f;
        BAsmCode[1] = 0x16;
        CodeLen = 2;
        break;
      case ModRDisp:
        BAsmCode[0] = 0x6f;
        BAsmCode[1] = 0x20 | (AdrPart << 1);
        CopyVals(2);
        break;
      case ModDir:
        BAsmCode[0] = 0x45;
        CopyVals(1);
        break;
      case ModIO:
        BAsmCode[0] = 0x55;
        CopyVals(1);
        break;
      case ModReg:
        BAsmCode[0] = 0xb0 + AdrPart;
        CodeLen = 1;
        break;
      case ModMem:
        if (AdrPart == ABSMODE)
        {
          BAsmCode[0] = 0x57;
          CopyVals(1);
        }
        else if ((AdrPart >= 0x10) && (AdrPart <= 0x17))
        {
          BAsmCode[0] = 0xb0 + AdrPart;
          CopyVals(1);
        }
        else
        {
          BAsmCode[0] = 0x72;
          BAsmCode[1] = 0xc0 | AdrPart;
          CopyVals(2);
        }
        break;
    }
  }
}

static void DecodeNEGNOT(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    SetOpSize((OpPart.str.p_str[3] == 'W') ? 1 : 0);
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        BAsmCode[0] = Lo(Index);
        CodeLen = 1;
        break;
      case ModReg:
      case ModMem:
        BAsmCode[0] = 0x75 + (OpSize << 1);
        BAsmCode[1] = Hi(Index) + AdrPart;
        CopyVals(2);
        break;
    }
  }
}

static void DecodeNRML(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeAdr(&ArgStr[1], MModAcc))
  {
    SetOpSize(0);
    if (DecodeAdr(&ArgStr[2], MModReg))
    {
      if (AdrPart != 0) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x6f;
        BAsmCode[1] = 0x2d;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeStack(Word Index)
{
  int z, z2;
  Byte HReg;
  char *p;

  if (!ChkArgCnt(1, ArgCntMax));
  else if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "A")))
  {
    BAsmCode[0] = Index;
    CodeLen = 1;
  }
  else if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "AH")))
  {
    BAsmCode[0] = Index + 1;
    CodeLen = 1;
  }
  else if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "PS")))
  {
    BAsmCode[0] = Index + 2;
    CodeLen = 1;
  }
  else
  {
    tStrComp From, To;

    BAsmCode[1] = 0; SetOpSize(1);
    for (z = 1; z <= ArgCnt; z++)
     if ((p = strchr(ArgStr[z].str.p_str, '-')) != NULL)
     {
       StrCompSplitRef(&From, &To, &ArgStr[z], p);
       if (!DecodeAdr(&From, MModReg)) break;
       HReg = AdrPart;
       if (!DecodeAdr(&To, MModReg)) break;
       if (AdrPart >= HReg)
       {
         for (z2 = HReg; z2 <= AdrPart; z2++)
          BAsmCode[1] |= (1 << z2);
       }
       else
       {
         for (z2 = HReg; z2 <= 7; z2++)
          BAsmCode[1] |= (1 << z2);
         for (z2 = 0; z2 <= AdrPart; z2++)
          BAsmCode[1] |= (1 << z2);
       }
     }
     else
     {
       if (!DecodeAdr(&ArgStr[z], MModReg)) break;
       BAsmCode[1] |= (1 << AdrPart);
     }
    if (z > ArgCnt)
    {
      BAsmCode[0] = Index + 3;
      CodeLen = 2;
    }
  }
}

static void DecodeRotate(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        BAsmCode[0] = 0x6f;
        BAsmCode[1] = 0x07 | (Index << 4);
        CodeLen = 2;
        break;
      case ModReg:
      case ModMem:
        BAsmCode[0] = 0x72;
        BAsmCode[1] = (Index << 5) | AdrPart;
        CopyVals(2);
        break;
    }
  }
}

static void DecodeSBBS(Word Index)
{
  LongInt Adr;
  Boolean OK;
  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && SplitBit(&ArgStr[1], BAsmCode + 1)
   && DecodeAdr(&ArgStr[1], MModMem))
  {
    if (AdrPart != ABSMODE) WrError(ErrNum_InvAddrMode);
    else
    {
      tSymbolFlags Flags;

      Adr = EvalStrIntExpressionWithFlags(&ArgStr[2], UInt24, &OK, &Flags) - (EProgCounter() + 5);
      if (OK)
      {
        if (!mSymbolQuestionable(Flags) && ((Adr < -128) || (Adr > 127))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[0] = 0x6c;
          BAsmCode[1] += 0xf8;
          CopyVals(2);
          BAsmCode[CodeLen++] = Adr & 0xff;
        }
      }
    }
  }
}

static void DecodeWBit(Word Index)
{
  if (ChkArgCnt(1, 1)
   && SplitBit(&ArgStr[1], BAsmCode + 1)
   && DecodeAdr(&ArgStr[1], MModIO))
  {
    BAsmCode[0] = 0x6c;
    BAsmCode[1] += Index;
    CopyVals(2);
  }
}

static void DecodeExchange(Word Index)
{
  Byte HPart, HCnt;

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(Index);
    DecodeAdr(&ArgStr[1], MModAcc | MModReg | MModMem);
    switch (AdrMode)
    {
      case ModAcc:
        if (DecodeAdr(&ArgStr[2], MModReg | MModMem))
        {
          BAsmCode[0] = 0x72 + Index;
          BAsmCode[1] = 0xe0 | AdrPart;
          CopyVals(2);
        }
        break;
      case ModReg:
        HPart = AdrPart;
        DecodeAdr(&ArgStr[2], MModAcc | MModReg | MModMem);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x72 + Index;
            BAsmCode[1] = 0xe0 | HPart;
            CodeLen = 2;
            break;
          case ModReg:
          case ModMem:
            BAsmCode[0] = 0x7e + Index;
            BAsmCode[1] = (HPart << 5) | AdrPart;
            CopyVals(2);
            break;
        }
        break;
      case ModMem:
        HPart = AdrPart; HCnt = AdrCnt;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModAcc | MModReg);
        switch (AdrMode)
        {
          case ModAcc:
            BAsmCode[0] = 0x72 + Index;
            BAsmCode[1] = 0xe0 | HPart;
            CodeLen = 2 + HCnt;
            break;
         case ModReg:
            BAsmCode[0] = 0x7e + Index;
            BAsmCode[1] = (AdrPart << 5) | HPart;
            CodeLen = 2 + HCnt;
            break;
        }
        break;
    }
  }
}

static void DecodeBank(Word Index)
{
  if (ChkArgCnt(0, 0))
  {
    BAsmCode[0] = Index + 4;
    CodeLen = 1;
    NextDataSeg = Index;
  }
}

/*--------------------------------------------------------------------------*/
/* Codetabellen */

static void AddFixed(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddALU8(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeALU8);
}

static void AddLog8(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeLog8);
}

static void AddAcc(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeAcc);
}

static void AddShift(const char *NName, Word NCode, Word NMay)
{
  AddInstTable(InstTable, NName, NCode | (NMay << 8), DecodeShift);
}

static void AddBranch(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeBranch);
}

static void AddIncDec(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeIncDec);
}

static void AddMulDiv(const char *NName, Byte NCode, Word NAccCode)
{
  MulDivOrders[InstrZ].Code = NCode;
  MulDivOrders[InstrZ].AccCode = NAccCode;
  AddInstTable(InstTable, NName, InstrZ++, DecodeMulDiv);
}

static void AddSeg(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeSeg);
}

static void AddString(const char *NName, Byte NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeString);
}

static void InitFields(void)
{
  unsigned z;

  InstTable = CreateInstTable(201);

  AddFixed("EXT"    , 0x14); AddFixed("EXTW"   , 0x1c);
  AddFixed("INT9"   , 0x01); AddFixed("NOP"    , 0x00);
  AddFixed("RET"    , 0x67); AddFixed("RETI"   , 0x6b);
  AddFixed("RETP"   , 0x66); AddFixed("SWAP"   , 0x16);
  AddFixed("SWAPW"  , 0x1e); AddFixed("UNLINK" , 0x09);
  AddFixed("ZEXT"   , 0x15); AddFixed("ZEXTW"  , 0x1d);
  AddFixed("CMR"    , 0x10); AddFixed("NCC"    , 0x11);

  AddALU8("ADD"  , 0); AddALU8("SUB"  , 1);

  AddLog8("AND"  , 4); AddLog8("OR"   , 5);
  AddLog8("XOR"  , 6);

  AddAcc("ADDDC"  , 0x02);
  AddAcc("SUBDC"  , 0x12);

  AddShift("ASR"  , 0x2e, FALSE); AddShift("ASRL" , 0x1e, FALSE);
  AddShift("ASRW" , 0x0e, TRUE ); AddShift("LSLW" , 0x0c, TRUE );
  AddShift("LSRW" , 0x0f, TRUE ); AddShift("LSL"  , 0x2c, FALSE);
  AddShift("LSR"  , 0x2f, FALSE); AddShift("LSLL" , 0x1c, FALSE);
  AddShift("LSRL" , 0x1f, FALSE); AddShift("SHLW" , 0x0c, TRUE );
  AddShift("SHRW" , 0x0f, TRUE );

  AddBranch("BZ"  , 0xf0); AddBranch("BEQ" , 0xf0); AddBranch("BNZ" , 0xf1);
  AddBranch("BNE" , 0xf1); AddBranch("BC"  , 0xf2); AddBranch("BLO" , 0xf2);
  AddBranch("BNC" , 0xf3); AddBranch("BHS" , 0xf3); AddBranch("BN"  , 0xf4);
  AddBranch("BP"  , 0xf5); AddBranch("BV"  , 0xf6); AddBranch("BNV" , 0xf7);
  AddBranch("BT"  , 0xf8); AddBranch("BNT" , 0xf9); AddBranch("BLT" , 0xfa);
  AddBranch("BGE" , 0xfb); AddBranch("BLE" , 0xfc); AddBranch("BGT" , 0xfd);
  AddBranch("BLS" , 0xfe); AddBranch("BHI" , 0xff); AddBranch("BRA" , 0x60);

  AddIncDec("INC"  , 0x42); AddIncDec("INCW" , 0x43); AddIncDec("INCL" , 0x41);
  AddIncDec("DEC"  , 0x62); AddIncDec("DECW" , 0x63); AddIncDec("DECL" , 0x61);

  MulDivOrders = (MulDivOrder*) malloc(sizeof(MulDivOrder) * MulDivOrderCnt);
  InstrZ = 0;
  AddMulDiv("MULU", 0x00, 0x0027); AddMulDiv("MULUW", 0x20, 0x002f);
  AddMulDiv("MUL" , 0x40, 0x786f); AddMulDiv("MULW" , 0x60, 0x796f);
  AddMulDiv("DIVU", 0x80, 0x0026); AddMulDiv("DIVUW", 0xa0, 0xffff);
  AddMulDiv("DIV" , 0xc0, 0x7a6f); AddMulDiv("DIVW" , 0xe0, 0xffff);

  AddSeg("SCEQI" , 0x80); AddSeg("SCEQD" , 0x90);
  AddSeg("SCWEQI", 0xa0); AddSeg("SCWEQD", 0xb0);
  AddSeg("FILSI" , 0xc0); AddSeg("FILS"  , 0xc0);
  AddSeg("FILSWI", 0xe0); AddSeg("FILSW" , 0xe0);
  AddSeg("SCEQ"  , 0x80); AddSeg("SCWEQ" , 0xa0);

  AddString("MOVS" , 0x00); AddString("MOVSI" , 0x00); AddString("MOVSD" , 0x10);
  AddString("MOVSW", 0x20); AddString("MOVSWI", 0x20); AddString("MOVSWD", 0x30);

  for (z = 0; z < (int)sizeof(BankNames) / sizeof(*BankNames); z++)
    AddInstTable(InstTable, BankNames[z], z, DecodeBank);

  AddInstTable(InstTable, "ADDC", 0, DecodeCarry8);
  AddInstTable(InstTable, "SUBC", 1, DecodeCarry8);
  AddInstTable(InstTable, "ADDCW",0, DecodeCarry16);
  AddInstTable(InstTable, "SUBCW",1, DecodeCarry16);
  AddInstTable(InstTable, "ADDL", 0, DecodeAdd32);
  AddInstTable(InstTable, "SUBL", 1, DecodeAdd32);
  AddInstTable(InstTable, "CMPL", 3, DecodeAdd32);
  AddInstTable(InstTable, "ADDSP",0, DecodeADDSP);
  AddInstTable(InstTable, "ADDW", 0, DecodeAdd16);
  AddInstTable(InstTable, "SUBW", 1, DecodeAdd16);
  AddInstTable(InstTable, "CMPW", 3, DecodeAdd16);
  AddInstTable(InstTable, "ANDW", 4, DecodeAdd16);
  AddInstTable(InstTable, "ORW" , 5, DecodeAdd16);
  AddInstTable(InstTable, "XORW", 6, DecodeAdd16);
  AddInstTable(InstTable, "ANDL", 4, DecodeLog32);
  AddInstTable(InstTable, "ORL",  5, DecodeLog32);
  AddInstTable(InstTable, "XORL", 6, DecodeLog32);
  AddInstTable(InstTable, "BBC", 0x80, DecodeBBcc);
  AddInstTable(InstTable, "BBS", 0xa0, DecodeBBcc);
  AddInstTable(InstTable, "CALL", 2, DecodeJmp16);
  AddInstTable(InstTable, "JMP", 0, DecodeJmp16);
  AddInstTable(InstTable, "CALLP", 2, DecodeJmp24);
  AddInstTable(InstTable, "JMPP", 0, DecodeJmp24);
  AddInstTable(InstTable, "CALLV", 0, DecodeCALLV);
  AddInstTable(InstTable, "CBNE", 0, DecodeCmpBranch);
  AddInstTable(InstTable, "CWBNE", 1, DecodeCmpBranch);
  AddInstTable(InstTable, "CLRB", 0x40, DecodeBit);
  AddInstTable(InstTable, "SETB", 0x60, DecodeBit);
  AddInstTable(InstTable, "CMP" , 0, DecodeCMP);
  AddInstTable(InstTable, "DBNZ" , 0, DecodeDBNZ);
  AddInstTable(InstTable, "DWBNZ" , 1, DecodeDBNZ);
  AddInstTable(InstTable, "INT", 0, DecodeINT);
  AddInstTable(InstTable, "INTP", 0, DecodeINTP);
  AddInstTable(InstTable, "JCTX", 0, DecodeJCTX);
  AddInstTable(InstTable, "LINK", 0, DecodeLINK);
  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "MOVB", 0, DecodeMOVB);
  AddInstTable(InstTable, "MOVEA", 0, DecodeMOVEA);
  AddInstTable(InstTable, "MOVL", 0, DecodeMOVL);
  AddInstTable(InstTable, "MOVN", 0, DecodeMOVN);
  AddInstTable(InstTable, "MOVW", 0, DecodeMOVW);
  AddInstTable(InstTable, "MOVX", 0, DecodeMOVX);
  AddInstTable(InstTable, "NOT" , 0xe037, DecodeNEGNOT);
  AddInstTable(InstTable, "NEG" , 0x6003, DecodeNEGNOT);
  AddInstTable(InstTable, "NOTW", 0xe03f, DecodeNEGNOT);
  AddInstTable(InstTable, "NEGW", 0x600b, DecodeNEGNOT);
  AddInstTable(InstTable, "NRML", 0, DecodeNRML);
  AddInstTable(InstTable, "PUSHW", 0x40, DecodeStack);
  AddInstTable(InstTable, "POPW", 0x50, DecodeStack);
  AddInstTable(InstTable, "ROLC", 0, DecodeRotate);
  AddInstTable(InstTable, "RORC", 1, DecodeRotate);
  AddInstTable(InstTable, "SBBS", 1, DecodeSBBS);
  AddInstTable(InstTable, "WBTS", 0xc0, DecodeWBit);
  AddInstTable(InstTable, "WBTC", 0xe0, DecodeWBit);
  AddInstTable(InstTable, "XCH", 0, DecodeExchange);
  AddInstTable(InstTable, "XCHW", 1, DecodeExchange);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  free(MulDivOrders);
}

/*--------------------------------------------------------------------------*/
/* Interface zu AS */

static void MakeCode_F2MC16(void)
{
  /* Leeranweisung ignorieren */

  if (Memo(""))
    return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  /* akt. Datensegment */

  switch (NextDataSeg)
  {
    case 0:
      CurrBank = Reg_PCB;
      break;
    case 1:
      CurrBank = Reg_DTB;
      break;
    case 2:
      CurrBank = Reg_ADB;
      break;
    case 3:
      CurrBank = SupAllowed ? Reg_SSB : Reg_USB;
      break;
  }
  CurrBank <<= 16;
  NextDataSeg = 1; /* = DTB */

  OpSize = -1;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_F2MC16(void)
{
  return FALSE;
}

static void InitCode_F2MC16(void)
{
  Reg_PCB = 0xff;
  Reg_DTB = 0x00;
  Reg_USB = 0x00;
  Reg_SSB = 0x00;
  Reg_ADB = 0x00;
  Reg_DPR = 0x01;
}

static void SwitchTo_F2MC16(void)
{
  const TFamilyDescr *FoundDescr;

  FoundDescr = FindFamilyByName("F2MC16");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "$";
  HeaderID = FoundDescr->Id;
  NOPCode=0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1;
  ListGrans[SegCode] = 1;
  SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffffff;

  MakeCode = MakeCode_F2MC16;
  IsDef = IsDef_F2MC16;
  SwitchFrom = DeinitFields;
  InitFields();

  onoff_supmode_add();

  pASSUMERecs = ASSUMEF2MC16s;
  ASSUMERecCnt = ASSUMEF2MC16Count;

  NextDataSeg = 1; /* DTB */
}

/*--------------------------------------------------------------------------*/
/* Initialisierung */

void codef2mc16_init(void)
{
  CPU90500 = AddCPU("MB90500", SwitchTo_F2MC16);

  AddInitPassProc(InitCode_F2MC16);
}
