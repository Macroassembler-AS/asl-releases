/* code7700.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegeneratormodul MELPS-7700                                          */
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
#include "intpseudo.h"
#include "motpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code7700.h"

typedef struct
{
  Word Code;
  ShortInt Disp8, Disp16;
} RelOrder;

typedef struct
{
  Byte CodeImm, CodeAbs8, CodeAbs16, CodeIdxX8, CodeIdxX16,
       CodeIdxY8, CodeIdxY16;
} XYOrder;

enum
{
  ModNone =   (-1),
  ModImm =     0,
  ModAbs8 =    1,
  ModAbs16 =   2,
  ModAbs24 =   3,
  ModIdxX8 =   4,
  ModIdxX16 =  5,
  ModIdxX24 =  6,
  ModIdxY8 =   7,
  ModIdxY16 =  8,
  ModIdxY24 =  9,
  ModInd8 =   10,
  ModInd16 =  11,
  ModInd24 =  12,
  ModIndX8 =  13,
  ModIndX16 = 14,
  ModIndX24 = 15,
  ModIndY8 =  16,
  ModIndY16 = 17,
  ModIndY24 = 18,
  ModIdxS8 =  19,
  ModIndS8 =  20
};

#define MModImm      (1l << ModImm)
#define MModAbs8     (1l << ModAbs8)
#define MModAbs16    (1l << ModAbs16)
#define MModAbs24    (1l << ModAbs24)
#define MModIdxX8    (1l << ModIdxX8)
#define MModIdxX16   (1l << ModIdxX16)
#define MModIdxX24   (1l << ModIdxX24)
#define MModIdxY8    (1l << ModIdxY8)
#define MModIdxY16   (1l << ModIdxY16)
#define MModIdxY24   (1l << ModIdxY24)
#define MModInd8     (1l << ModInd8)
#define MModInd16    (1l << ModInd16)
#define MModInd24    (1l << ModInd24)
#define MModIndX8    (1l << ModIndX8)
#define MModIndX16   (1l << ModIndX16)
#define MModIndX24   (1l << ModIndX24)
#define MModIndY8    (1l << ModIndY8)
#define MModIndY16   (1l << ModIndY16)
#define MModIndY24   (1l << ModIndY24)
#define MModIdxS8    (1l << ModIdxS8)
#define MModIndS8    (1l << ModIndS8)

#define PushRegCnt 10
static const char PushRegNames[PushRegCnt][4] =
{
  "A", "B", "X", "Y", "DPR", "DT", "DBR", "PG", "PBR", "PS"
};
static const Byte PushRegCodes[PushRegCnt] =
{
  0  ,1  ,2  ,3  ,4    ,5   ,5    ,6   ,6    ,7
};

#define PrefAccB 0x42

static LongInt Reg_PG, Reg_DT, Reg_X, Reg_M, Reg_DPR, BankReg;

static Boolean WordSize;
static Byte AdrVals[3];
static ShortInt AdrType;

static RelOrder *RelOrders;
static XYOrder *XYOrders;

static CPUVar CPU65816, CPUM7700, CPUM7750, CPUM7751;

static ASSUMERec ASSUME7700s[] =
{
  { "PG" , &Reg_PG , 0,   0xff,   0x100, NULL },
  { "DT" , &Reg_DT , 0,   0xff,   0x100, NULL },
  { "PBR", &Reg_PG , 0,   0xff,   0x100, NULL },
  { "DBR", &Reg_DT , 0,   0xff,   0x100, NULL },
  { "X"  , &Reg_X  , 0,      1,      -1, NULL },
  { "M"  , &Reg_M  , 0,      1,      -1, NULL },
  { "DPR", &Reg_DPR, 0, 0xffff, 0x10000, NULL }
};

/*---------------------------------------------------------------------------*/
/* Address Parsing */

static void CodeDisp(const tStrComp *pArg, LongInt Start, LongWord Mask)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt Adr;
  ShortInt DType;
  int ArgLen = strlen(pArg->str.p_str);
  unsigned Offset = 0;

  if ((ArgLen > 1) && (pArg->str.p_str[Offset] == '<'))
  {
    Offset = 1;
    DType = 0;
  }
  else if ((ArgLen > 1) && (pArg->str.p_str[Offset] == '>'))
  {
    if ((ArgLen > 2) && (pArg->str.p_str[Offset + 1] == '>'))
    {
      Offset = 2;
      DType = 2;
    }
    else
    {
      Offset = 1;
      DType = 1;
    }
  }
  else
    DType = -1;

  Adr = EvalStrIntExpressionOffsWithFlags(pArg, Offset, UInt24, &OK, &Flags);

  if (!OK)
    return;

  if (DType == -1)
  {
    if ((Mask & (1l << Start))
     && (Adr >= Reg_DPR)
     && (Adr < Reg_DPR + 0x100))
      DType = 0;
    else if ((Mask & (2l << Start))
          && ((Adr >> 16) == BankReg))
      DType = 1;
    else
      DType = 2;
  }

  if (!(Mask & (1l << (Start + DType)))) WrError(ErrNum_InvAddrMode);
  else
  {
    switch (DType)
    {
      case 0:
        if (mFirstPassUnknown(Flags) || (ChkRange(Adr, Reg_DPR, Reg_DPR + 0xff)))
        {
          AdrCnt = 1;
          AdrType = Start;
          AdrVals[0] = Lo(Adr - Reg_DPR);
        }
        break;
      case 1:
        if (!mFirstPassUnknown(Flags) && ((Adr >> 16) != BankReg)) WrError(ErrNum_OverRange);
        else
        {
          AdrCnt = 2;
          AdrType = Start + 1;
          AdrVals[0] = Lo(Adr);
          AdrVals[1] = Hi(Adr);
        }
        break;
      case 2:
        AdrCnt = 3;
        AdrType = Start + 2;
        AdrVals[0] = Lo(Adr);
        AdrVals[1] = Hi(Adr);
        AdrVals[2] = Adr >> 16;
        break;
    }
  }
}

static Integer SplitArg(const tStrComp *pSrc, tStrComp *pDest)
{
  tStrComp IArg;
  char *p;

  StrCompRefRight(&IArg, pSrc, 1);
  StrCompShorten(&IArg, 1);
  p = QuotPos(IArg.str.p_str, ',');
  if (!p)
  {
    StrCompCopy(&pDest[0], &IArg);
    return 1;
  }
  else
  {
    StrCompSplitCopy(&pDest[0], &pDest[1], &IArg, p);
    return 2;
  }
}

static void DecodeAdr(Integer Start, LongWord Mask)
{
  Word AdrWord;
  Boolean OK;
  Integer HCnt;
  String HStr[2];
  tStrComp HArg[2];

  StrCompMkTemp(&HArg[0], HStr[0], sizeof(HStr[0]));
  StrCompMkTemp(&HArg[1], HStr[1], sizeof(HStr[1]));
  AdrType = ModNone;
  AdrCnt = 0;

  /* I. 1 Parameter */

  if (Start == ArgCnt)
  {
    /* I.1. immediate */

    if (*ArgStr[Start].str.p_str == '#')
    {
      if (WordSize)
      {
        AdrWord = EvalStrIntExpressionOffs(&ArgStr[Start], 1, Int16, &OK);
        AdrVals[0] = Lo(AdrWord);
        AdrVals[1] = Hi(AdrWord);
      }
      else
        AdrVals[0] = EvalStrIntExpressionOffs(&ArgStr[Start], 1, Int8, &OK);
      if (OK)
      {
        AdrCnt = 1 + Ord(WordSize);
        AdrType = ModImm;
      }
      goto chk;
    }

    /* I.2. indirekt */

    if (IsIndirect(ArgStr[Start].str.p_str))
    {
      HCnt = SplitArg(&ArgStr[Start], HArg);

      /* I.2.i. einfach indirekt */

      if (HCnt == 1)
      {
        CodeDisp(&HArg[0], ModInd8, Mask);
        goto chk;
      }

      /* I.2.ii indirekt mit Vorindizierung */

      else if (!as_strcasecmp(HArg[1].str.p_str, "X"))
      {
        CodeDisp(&HArg[0], ModIndX8, Mask);
        goto chk;
      }

      else
      {
        WrError(ErrNum_InvAddrMode);
        goto chk;
      }
    }

    /* I.3. absolut */

    else
    {
      CodeDisp(&ArgStr[Start], ModAbs8, Mask);
      goto chk;
    }
  }

  /* II. 2 Parameter */

  else if (Start + 1 == ArgCnt)
  {
    /* II.1 indirekt mit Nachindizierung */

    if (IsIndirect(ArgStr[Start].str.p_str))
    {
      if (as_strcasecmp(ArgStr[Start + 1].str.p_str, "Y")) WrError(ErrNum_InvAddrMode);
      else
      {
        HCnt = SplitArg(&ArgStr[Start], HArg);

        /* II.1.i. (d),Y */

        if (HCnt == 1)
        {
          CodeDisp(&HArg[0], ModIndY8, Mask);
          goto chk;
        }

        /* II.1.ii. (d,S),Y */

        else if (!as_strcasecmp(HArg[1].str.p_str, "S"))
        {
          AdrVals[0] = EvalStrIntExpression(&HArg[0], Int8, &OK);
          if (OK)
          {
            AdrType = ModIndS8;
            AdrCnt = 1;
          }
          goto chk;
        }

        else WrError(ErrNum_InvAddrMode);
      }
      goto chk;
    }

    /* II.2. einfach indiziert */

    else
    {
      /* II.2.i. d,X */

      if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "X"))
      {
        CodeDisp(&ArgStr[Start], ModIdxX8, Mask);
        goto chk;
      }

      /* II.2.ii. d,Y */

      else if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "Y"))
      {
        CodeDisp(&ArgStr[Start], ModIdxY8, Mask);
        goto chk;
      }

      /* II.2.iii. d,S */

      else if (!as_strcasecmp(ArgStr[Start + 1].str.p_str, "S"))
      {
        AdrVals[0] = EvalStrIntExpression(&ArgStr[Start], Int8, &OK);
        if (OK)
        {
          AdrType = ModIdxS8;
          AdrCnt = 1;
        }
        goto chk;
      }

      else WrError(ErrNum_InvAddrMode);
    }
  }

  else
    (void)ChkArgCnt(Start, Start + 1);

chk:
  if ((AdrType != ModNone)
   && (!(Mask & (1l << ((LongWord)AdrType)))))
   {
     AdrType = ModNone;
     AdrCnt = 0;
     WrError(ErrNum_InvAddrMode);
   }
}

static Boolean CPUMatch(Byte Mask)
{
  return ((Mask >> (MomCPU - CPU65816)) & 1);
}

/*---------------------------------------------------------------------------*/

/* see my rant about BRK in code65.c */

static void DecodeBRK(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(0, 1))
  {
    BAsmCode[0] = 0x00;
    if (ArgCnt > 0)
    {
      Boolean OK;

      BAsmCode[1] = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), Int8, &OK);
      if (OK)
        CodeLen = 2;
    }
    else
      CodeLen = 1;
  }
}

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    CodeLen = 1 + Ord(Hi(Code) != 0);
    if (CodeLen == 2)
      BAsmCode[0] = Hi(Code);
    BAsmCode[CodeLen - 1] = Lo(Code);
  }
}

static void DecodePHB_PLB(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    if (MomCPU >= CPUM7700)
    {
      CodeLen = 2;
      BAsmCode[0] = PrefAccB;
      BAsmCode[1] = 0x48;
    }
    else
    {
      CodeLen = 1;
      BAsmCode[0] = 0x8b;
    }
    BAsmCode[CodeLen - 1] += Code;
  }
}

static void DecodeRel(Word Index)
{
  const RelOrder *pOrder = RelOrders + Index;
  LongInt AdrLong;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    AdrLong = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '#'), Int32, &OK, &Flags);
    if (OK)
    {
      OK = pOrder->Disp8 == -1;
      if (OK)
        AdrLong -= EProgCounter() + pOrder->Disp16;
      else
      {
        AdrLong -= EProgCounter() + pOrder->Disp8;
        if (((AdrLong > 127) || (AdrLong < -128)) && !mSymbolQuestionable(Flags) && (pOrder->Disp16 != -1))
        {
          OK = True;
          AdrLong -= pOrder->Disp16 - pOrder->Disp8;
        }
      }
      if (OK)            /* d16 */
      {
        if (((AdrLong < -32768) || (AdrLong > 32767)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_DistTooBig);
        else
        {
          CodeLen = 3;
          BAsmCode[0] = Hi(pOrder->Code);
          BAsmCode[1] = Lo(AdrLong);
          BAsmCode[2] = Hi(AdrLong);
        }
      }
      else               /* d8 */
      {
        if (((AdrLong < -128) || (AdrLong > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 2;
          BAsmCode[0] = Lo(pOrder->Code);
          BAsmCode[1] = Lo(AdrLong);
        }
      }
    }
  }
}

static void DecodeAcc(Word Code)
{
  int Start;
  Boolean LFlag = Hi(Code);
  Code = Lo(Code);

  if (ChkArgCnt(1, 3))
  {
    WordSize = (Reg_M == 0);
    if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      Start = 2;
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "B"))
    {
      Start = 2;
      BAsmCode[0] = PrefAccB;
      CodeLen++;
      if (!ChkExcludeCPUExt(CPU65816, ErrNum_AddrModeNotSupported))
        return;
    }
    else
      Start = 1;
    DecodeAdr(Start,
              MModAbs8 | MModAbs16 | MModAbs24
            | MModIdxX8 | MModIdxX16 | MModIdxX24
            | MModIdxY16
            | MModInd8 | MModIndX8 | MModIndY8
            | MModIdxS8 | MModIndS8
            | ((Code == 0x80) ? 0 : MModImm)); /* STA */
    if (AdrType != ModNone)
    {
      if ((LFlag) && (AdrType != ModInd8) && (AdrType != ModIndY8)) WrError(ErrNum_InvAddrMode);
      else
      {
        switch (AdrType)
        {
          case ModImm:
            BAsmCode[CodeLen] = Code + 0x09;
            break;
          case ModAbs8:
            BAsmCode[CodeLen] = Code + 0x05;
            break;
          case ModAbs16:
            BAsmCode[CodeLen] = Code + 0x0d;
            break;
          case ModAbs24:
            BAsmCode[CodeLen] = Code + 0x0f;
            break;
          case ModIdxX8:
            BAsmCode[CodeLen] = Code + 0x15;
            break;
          case ModIdxX16:
            BAsmCode[CodeLen] = Code + 0x1d;
            break;
          case ModIdxX24:
            BAsmCode[CodeLen] = Code + 0x1f;
            break;
          case ModIdxY16:
            BAsmCode[CodeLen] = Code + 0x19;
            break;
          case ModInd8:
            BAsmCode[CodeLen] = LFlag ? Code + 0x07 : Code + 0x12;
            break;
          case ModIndX8:
            BAsmCode[CodeLen] = Code + 0x01;
            break;
          case ModIndY8:
            BAsmCode[CodeLen] = (LFlag) ? Code + 0x17 : Code + 0x11;
            break;
          case ModIdxS8:
            BAsmCode[CodeLen] = Code + 0x03;
            break;
          case ModIndS8:
            BAsmCode[CodeLen] = Code + 0x13;
            break;
        }
        memcpy(BAsmCode + CodeLen + 1, AdrVals, AdrCnt);
        CodeLen += 1 + AdrCnt;
      }
    }
  }
}

static void DecodeEXTS_EXTZ(Word Code)
{
  if (ArgCnt == 0)
  {
    const char AccArg[] = "A";

    AppendArg(strlen(AccArg));
    strmaxcpy(ArgStr[ArgCnt].str.p_str, AccArg, STRINGSIZE);
  }

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPUM7750))
  {
    BAsmCode[1] = Code;
    BAsmCode[0] = 0;
    if (!as_strcasecmp(ArgStr[1].str.p_str, "A"))
      BAsmCode[0] = 0x89;
    else if (!as_strcasecmp(ArgStr[1].str.p_str, "B"))
      BAsmCode[0] = 0x42;
    else WrError(ErrNum_InvAddrMode);
    if (BAsmCode[0] != 0)
      CodeLen = 2;
  }
}

static void DecodeRMW(Word Code)
{
  if ((ArgCnt == 0) || ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "A"))))
  {
    CodeLen = 1;
    BAsmCode[0] = Hi(Code);
  }
  else if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "B")))
  {
    CodeLen = 2;
    BAsmCode[0] = PrefAccB;
    BAsmCode[1] = Hi(Code);
    if (!ChkExcludeCPUExt(CPU65816, ErrNum_AddrModeNotSupported))
      return;
  }
  else if (!ChkArgCnt(0, 2));
  else
  {
    DecodeAdr(1, MModAbs8 | MModAbs16 | MModIdxX8 | MModIdxX16);
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModAbs8:
          BAsmCode[0] = Lo(Code);
          break;
        case ModAbs16:
          BAsmCode[0] = Lo(Code) + 8;
          break;
        case ModIdxX8:
          BAsmCode[0] = Lo(Code) + 16;
          break;
        case ModIdxX16:
          BAsmCode[0] = Lo(Code) + 24;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeASR(Word Code)
{
  UNUSED(Code);

  if (!ChkMinCPU(CPUM7750));
  else if ((ArgCnt == 0) || ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "A"))))
  {
    BAsmCode[0] = 0x89;
    BAsmCode[1] = 0x08;
    CodeLen = 2;
  }
  else if ((ArgCnt == 1) && (!as_strcasecmp(ArgStr[1].str.p_str, "B")))
  {
    BAsmCode[0] = 0x42;
    BAsmCode[1] = 0x08;
    CodeLen = 2;
  }
  else if (ChkArgCnt(1, 2))
  {
    DecodeAdr(1, MModAbs8 | MModIdxX8 | MModAbs16 | MModIdxX16);
    if (AdrType != ModNone)
    {
      BAsmCode[0] = 0x89;
      switch (AdrType)
      {
        case ModAbs8:
          BAsmCode[1] = 0x06;
          break;
        case ModIdxX8:
          BAsmCode[1] = 0x16;
          break;
        case ModAbs16:
          BAsmCode[1] = 0x0e;
          break;
        case ModIdxX16:
          BAsmCode[1] = 0x1e;
          break;
      }
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeBBC_BBS(Word Code)
{
  if (ChkArgCnt(3, 3)
   && ChkMinCPU(CPUM7700))
  {
    WordSize = (Reg_M == 0);
    ArgCnt = 2;
    DecodeAdr(2, MModAbs8 + MModAbs16);
    if (AdrType != ModNone)
    {
      BAsmCode[0] = Code + ((AdrType == ModAbs16) ? 8 : 0);
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
      ArgCnt = 1;
      DecodeAdr(1, MModImm);
      if (AdrType == ModNone)
        CodeLen = 0;
      else
      {
        LongInt AdrLong;
        Boolean OK;
        tSymbolFlags Flags;

        memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
        CodeLen += AdrCnt;
        AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[3], UInt24, &OK, &Flags) - (EProgCounter() + CodeLen + 1);
        if (!OK)
          CodeLen = 0;
        else if (!mSymbolQuestionable(Flags) && ((AdrLong < -128) || (AdrLong > 127)))
        {
          WrError(ErrNum_JmpDistTooBig);
          CodeLen = 0;
        }
        else
        {
          BAsmCode[CodeLen] = Lo(AdrLong);
          CodeLen++;
        }
      }
    }
  }
}

static void DecodeBIT(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2)
   && ChkExactCPU(CPU65816))
  {
    WordSize = (Reg_M == 0);
    DecodeAdr(1, MModAbs8 | MModAbs16 | MModIdxX8 | MModIdxX16 | MModImm);
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModAbs8:
          BAsmCode[0] = 0x24;
          break;
        case ModAbs16:
          BAsmCode[0] = 0x2c;
          break;
        case ModIdxX8:
          BAsmCode[0] = 0x34;
          break;
        case ModIdxX16:
          BAsmCode[0] = 0x3c;
          break;
        case ModImm:
          BAsmCode[0] = 0x89;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeCLB_SEB(Word Code)
{
  if (ChkArgCnt(2, 2)
   && ChkMinCPU(CPUM7700))
  {
    WordSize = (Reg_M == 0);
    DecodeAdr(2, MModAbs8 | MModAbs16);
    if (AdrType != ModNone)
    {
      BAsmCode[0] = Code + ((AdrType == ModAbs16) ? 8 : 0);
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
      ArgCnt = 1;
      DecodeAdr(1, MModImm);
      if (AdrType == ModNone)
        CodeLen = 0;
      else
      {
        memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
        CodeLen += AdrCnt;
      }
    }
  }
}

static void DecodeTSB_TRB(Word Code)
{
  if (MomCPU == CPU65816)
  {
    if (ChkArgCnt(1, 1))
    {
      DecodeAdr(1, MModAbs8 + MModAbs16);
      if (AdrType != ModNone)
      {
        BAsmCode[0] = Code + ((AdrType == ModAbs16) ? 8 : 0);
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        CodeLen = 1 + AdrCnt;
      }
    }
  }
  else if (Code == 0x14) (void)ChkExactCPU(CPU65816); /* TRB */
  else if (ChkArgCnt(0, 0))
  {
    CodeLen = 2;
    BAsmCode[0] = 0x42;
    BAsmCode[1] = 0x3b;
  }
}

static void DecodeImm8(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    WordSize = False;
    DecodeAdr(1, MModImm);
    if (AdrType == ModImm)
    {
      CodeLen = 1 + Ord(Hi(Code) != 0);
      if (CodeLen == 2)
        BAsmCode[0] = Hi(Code);
      BAsmCode[CodeLen-1] = Lo(Code);
      memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
      CodeLen += AdrCnt;
    }
  }
}

static void DecodeRLA(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    WordSize = (Reg_M == 0);
    DecodeAdr(1, MModImm);
    if (AdrType != ModNone)
    {
      CodeLen = 2 + AdrCnt;
      BAsmCode[0] = 0x89;
      BAsmCode[1] = 0x49;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
    }
  }
}

static void DecodeXY(Word Index)
{
  const XYOrder *pOrder = XYOrders + Index;

  if (ChkArgCnt(1, 2))
  {
    WordSize = (Reg_X == 0);
    DecodeAdr(1, ((pOrder->CodeImm    != 0xff) ? MModImm : 0)
               | ((pOrder->CodeAbs8   != 0xff) ? MModAbs8 : 0)
               | ((pOrder->CodeAbs16  != 0xff) ? MModAbs16 : 0)
               | ((pOrder->CodeIdxX8  != 0xff) ? MModIdxX8 : 0)
               | ((pOrder->CodeIdxX16 != 0xff) ? MModIdxX16 : 0)
               | ((pOrder->CodeIdxY8  != 0xff) ? MModIdxY8 : 0)
               | ((pOrder->CodeIdxY16 != 0xff) ? MModIdxY16 : 0));
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModImm:
          BAsmCode[0] = pOrder->CodeImm;
          break;
        case ModAbs8:
          BAsmCode[0] = pOrder->CodeAbs8;
          break;
        case ModAbs16:
          BAsmCode[0] = pOrder->CodeAbs16;
          break;
        case ModIdxX8:
          BAsmCode[0] = pOrder->CodeIdxX8;
          break;
        case ModIdxY8:
          BAsmCode[0] = pOrder->CodeIdxY8;
          break;
        case ModIdxX16:
          BAsmCode[0] = pOrder->CodeIdxX16;
          break;
        case ModIdxY16:
          BAsmCode[0] = pOrder->CodeIdxY16;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeMulDiv(Word Code)
{
  Byte LFlag = Hi(Code);
  Code = Lo(Code);

  if (ChkArgCnt(1, 2))
  {
    WordSize = (Reg_M == 0);
    DecodeAdr(1, MModImm | MModAbs8 | MModAbs16 | MModAbs24 | MModIdxX8 | MModIdxX16
               | MModIdxX24 | MModIdxY16 | MModInd8 | MModIndX8 | MModIndY8
               | MModIdxS8 | MModIndS8);
    if (AdrType != ModNone)
    {
      if ((LFlag) && (AdrType != ModInd8) && (AdrType != ModIndY8)) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x89;
        switch (AdrType)
        {
          case ModImm:
            BAsmCode[1] = 0x09;
            break;
          case ModAbs8:
            BAsmCode[1] = 0x05;
            break;
          case ModAbs16:
            BAsmCode[1] = 0x0d;
            break;
          case ModAbs24:
            BAsmCode[1] = 0x0f;
            break;
          case ModIdxX8:
            BAsmCode[1] = 0x15;
            break;
          case ModIdxX16:
            BAsmCode[1] = 0x1d;
            break;
          case ModIdxX24:
            BAsmCode[1] = 0x1f;
            break;
          case ModIdxY16:
            BAsmCode[1] = 0x19;
            break;
          case ModInd8:
            BAsmCode[1] = (LFlag) ? 0x07 : 0x12;
            break;
          case ModIndX8:
            BAsmCode[1] = 0x01;
            break;
          case ModIndY8:
            BAsmCode[1] = (LFlag) ? 0x17 : 0x11;
            break;
          case ModIdxS8:
            BAsmCode[1] = 0x03;
            break;
          case ModIndS8:
            BAsmCode[1] = 0x13;
            break;
        }
        BAsmCode[1] += Code;
        memcpy(BAsmCode + 2, AdrVals, AdrCnt);
        CodeLen = 2 + AdrCnt;
      }
    }
  }
}

static void DecodeJML_JSL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    LongInt AdrLong = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);

    if (OK)
    {
      CodeLen = 4;
      BAsmCode[0] = Code;
      BAsmCode[3] = AdrLong >> 16;
      BAsmCode[2] = Hi(AdrLong);
      BAsmCode[1] = Lo(AdrLong);
    }
  }
}

static void DecodeJMP_JSR(Word IsJSR)
{
  Byte LFlag = Hi(IsJSR);
  IsJSR = Lo(IsJSR);

  if (ChkArgCnt(1, 1))
  {
    BankReg = Reg_PG;
    DecodeAdr(1, MModAbs24 | MModIndX16
               | (LFlag ? 0: MModAbs16)
               | (IsJSR ? 0 : MModInd16));
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModAbs16:
          BAsmCode[0] = IsJSR ? 0x20 : 0x4c;
          break;
        case ModAbs24:
          BAsmCode[0] = IsJSR ? 0x22 : 0x5c;
          break;
        case ModIndX16:
          BAsmCode[0] = IsJSR ? 0xfc : 0x7c;
          break;
        case ModInd16:
          BAsmCode[0] = LFlag ? 0xdc : 0x6c;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeLDM(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 3)
   && ChkMinCPU(CPUM7700))
  {
    DecodeAdr(2, MModAbs8 | MModAbs16 | MModIdxX8 | MModIdxX16);
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModAbs8:
          BAsmCode[0] = 0x64;
          break;
        case ModAbs16:
          BAsmCode[0] = 0x9c;
          break;
        case ModIdxX8:
          BAsmCode[0] = 0x74;
          break;
        case ModIdxX16:
          BAsmCode[0] = 0x9e;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
      WordSize = (Reg_M == 0);
      ArgCnt = 1;
      DecodeAdr(1, MModImm);
      if (AdrType == ModNone)
        CodeLen = 0;
      else
      {
        memcpy(BAsmCode + CodeLen, AdrVals, AdrCnt);
        CodeLen += AdrCnt;
      }
    }
  }
}

static void DecodeSTZ(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 2)
   && ChkMinCPU(CPU65816))
  {
    DecodeAdr(1, MModAbs8 | MModAbs16 | MModIdxX8 | MModIdxX16);
    if (AdrType != ModNone)
    {
      switch (AdrType)
      {
        case ModAbs8:
          BAsmCode[0] = 0x64;
          break;
        case ModAbs16:
          BAsmCode[0] = 0x9c;
          break;
        case ModIdxX8:
          BAsmCode[0] = 0x74;
          break;
        case ModIdxX16:
          BAsmCode[0] = 0x9e;
          break;
      }
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
      CodeLen = 1 + AdrCnt;
    }
  }
}

static void DecodeMVN_MVP(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    LongInt Src = EvalStrIntExpression(&ArgStr[1], UInt24, &OK);

    if (OK)
    {
      LongInt Dest = EvalStrIntExpression(&ArgStr[2], UInt24, &OK);

      if (OK)
      {
        BAsmCode[0] = Code;
        BAsmCode[1] = Dest >> 16;
        BAsmCode[2] = Src >> 16;
        CodeLen = 3;
      }
    }
  }
}

static void DecodePSH_PUL(Word Code)
{
  if (ChkArgCnt(0, 0)
   && ChkMinCPU(CPUM7700))
  {
    Boolean OK;
    int z, Start;

    BAsmCode[0] = Code;
    BAsmCode[1] = 0;
    OK = True;
    z = 1;
    while ((z <= ArgCnt) && (OK))
    {
      Boolean OK;

      if (*ArgStr[z].str.p_str == '#')
        BAsmCode[1] |= EvalStrIntExpressionOffs(&ArgStr[z], 1, Int8, &OK);
      else
      {
        Start = 0;
        while ((Start < PushRegCnt) && (as_strcasecmp(PushRegNames[Start], ArgStr[z].str.p_str)))
          Start++;
        OK = (Start < PushRegCnt);
        if (OK)
          BAsmCode[1] |= 1l << PushRegCodes[Start];
        else
          WrStrErrorPos(ErrNum_InvRegName, &ArgStr[z]);
      }
      z++;
    }
    if (OK)
      CodeLen = 2;
  }
}

static void DecodePEA(Word Code)
{
  UNUSED(Code);

  WordSize = True;
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(1, MModAbs16);
    if (AdrType != ModNone)
    {
      CodeLen = 1 + AdrCnt;
      BAsmCode[0] = 0xf4;
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
    }
  }
}

static void DecodePEI(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(1, MModInd8);
    if (AdrType != ModNone)
    {
      CodeLen = 1 + AdrCnt; BAsmCode[0] = 0xd4;
      memcpy(BAsmCode + 1, AdrVals, AdrCnt);
    }
  }
}

static void DecodePER(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK, Rel = !(*ArgStr[1].str.p_str == '#');
    tSymbolFlags Flags;

    BAsmCode[0] = 0x62;
    if (Rel)
    {
      LongInt AdrLong = EvalStrIntExpressionWithFlags(&ArgStr[1], UInt24, &OK, &Flags) - (EProgCounter() + 3);

      if (OK)
      {
        if (((AdrLong < -32768) || (AdrLong > 32767)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          CodeLen = 3;
          BAsmCode[1] = AdrLong & 0xff;
          BAsmCode[2] = (AdrLong >> 8) & 0xff;
        }
      }
    }
    else
    {
      Word AdrWord = EvalStrIntExpressionOffs(&ArgStr[1], 1, Int16, &OK);

      if (OK)
      {
        CodeLen = 3;
        BAsmCode[1] = Lo(AdrWord);
        BAsmCode[2] = Hi(AdrWord);
      }
    }
  }
}

/*---------------------------------------------------------------------------*/

static void AddFixed(const char *NName, Word NCode, Byte NAllowed)
{
  if (CPUMatch(NAllowed))
    AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void AddRel(const char *NName, Word NCode, ShortInt NDisp8, ShortInt NDisp16)
{
  order_array_rsv_end(RelOrders, RelOrder);
  RelOrders[InstrZ].Code = NCode;
  RelOrders[InstrZ].Disp8 = NDisp8;
  RelOrders[InstrZ].Disp16 = NDisp16;
  AddInstTable(InstTable, NName, InstrZ++, DecodeRel);
}

static void AddAcc(const char *NName, Byte NCode)
{
  char Tmp[30];

  AddInstTable(InstTable, NName, NCode, DecodeAcc);
  as_snprintf(Tmp, sizeof(Tmp), "%sL", NName);
  AddInstTable(InstTable, Tmp, 0x0100 | NCode, DecodeAcc);
}

static void AddRMW(const char *NName, Word NACode, Word NMCode)
{
  AddInstTable(InstTable, NName, NACode << 8 | NMCode, DecodeRMW);
}

static void AddImm8(const char *NName, Word NCode, Byte NAllowed)
{
  if (CPUMatch(NAllowed))
    AddInstTable(InstTable, NName, NCode, DecodeImm8);
}

static void AddXY(const char *NName, Byte NCodeImm, Byte NCodeAbs8, Byte NCodeAbs16,
                  Byte NCodeIdxX8, Byte NCodeIdxX16, Byte NCodeIdxY8,
                  Byte NCodeIdxY16)
{
  order_array_rsv_end(XYOrders, XYOrder);
  XYOrders[InstrZ].CodeImm = NCodeImm;
  XYOrders[InstrZ].CodeAbs8 = NCodeAbs8;
  XYOrders[InstrZ].CodeAbs16 = NCodeAbs16;
  XYOrders[InstrZ].CodeIdxX8 = NCodeIdxX8;
  XYOrders[InstrZ].CodeIdxX16 = NCodeIdxX16;
  XYOrders[InstrZ].CodeIdxY8 = NCodeIdxY8;
  XYOrders[InstrZ].CodeIdxY16 = NCodeIdxY16;
  AddInstTable(InstTable, NName, InstrZ++, DecodeXY);
}

static void AddMulDiv(const char *NName, Word NCode, Byte NAllowed)
{
  if (CPUMatch(NAllowed))
  {
    char Tmp[30];

    AddInstTable(InstTable, NName, NCode, DecodeMulDiv);
    as_snprintf(Tmp, sizeof(Tmp), "%sL", NName);
    AddInstTable(InstTable, Tmp, 0x0100 | NCode, DecodeMulDiv);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(403);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "BRK", 0, DecodeBRK);
  AddInstTable(InstTable, "PHB", 0, DecodePHB_PLB);
  AddInstTable(InstTable, "PLB", 0x20, DecodePHB_PLB);
  AddInstTable(InstTable, "EXTS", 0x8b, DecodeEXTS_EXTZ);
  AddInstTable(InstTable, "EXTZ", 0xab, DecodeEXTS_EXTZ);
  AddInstTable(InstTable, "ASR", 0, DecodeASR);
  AddInstTable(InstTable, "BBC", 0x34, DecodeBBC_BBS);
  AddInstTable(InstTable, "BBS", 0x24, DecodeBBC_BBS);
  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
  AddInstTable(InstTable, "CLB", 0x14, DecodeCLB_SEB);
  AddInstTable(InstTable, "SEB", 0x04, DecodeCLB_SEB);
  AddInstTable(InstTable, "TSB", 0x04, DecodeTSB_TRB);
  AddInstTable(InstTable, "TRB", 0x14, DecodeTSB_TRB);
  AddInstTable(InstTable, "RLA", 0, DecodeRLA);
  AddInstTable(InstTable, "JML", 0x5c, DecodeJML_JSL);
  AddInstTable(InstTable, "JSL", 0x22, DecodeJML_JSL);
  AddInstTable(InstTable, "JMP", 0x0000, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 0x0001, DecodeJMP_JSR);
  AddInstTable(InstTable, "JMPL", 0x0100, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSRL", 0x0101, DecodeJMP_JSR);
  AddInstTable(InstTable, "LDM", 0, DecodeLDM);
  AddInstTable(InstTable, "STZ", 0, DecodeSTZ);
  AddInstTable(InstTable, "MVN", 0x54, DecodeMVN_MVP);
  AddInstTable(InstTable, "MVP", 0x44, DecodeMVN_MVP);
  AddInstTable(InstTable, "PSH", 0xeb, DecodePSH_PUL);
  AddInstTable(InstTable, "PUL", 0xfb, DecodePSH_PUL);
  AddInstTable(InstTable, "PEA", 0, DecodePEA);
  AddInstTable(InstTable, "PEI", 0, DecodePEI);
  AddInstTable(InstTable, "PER", 0, DecodePER);

  AddFixed("CLC", 0x0018, 15); AddFixed("CLI", 0x0058, 15);
  AddFixed("CLM", 0x00d8, 14); AddFixed("CLV", 0x00b8, 15);
  AddFixed("DEX", 0x00ca, 15); AddFixed("DEY", 0x0088, 15);
  AddFixed("INX", 0x00e8, 15); AddFixed("INY", 0x00c8, 15);
  AddFixed("NOP", 0x00ea, 15); AddFixed("PHA", 0x0048, 15);
  AddFixed("PHD", 0x000b, 15); AddFixed("PHG", 0x004b, 14);
  AddFixed("PHP", 0x0008, 15); AddFixed("PHT", 0x008b, 14);
  AddFixed("PHX", 0x00da, 15); AddFixed("PHY", 0x005a, 15);
  AddFixed("PLA", 0x0068, 15); AddFixed("PLD", 0x002b, 15);
  AddFixed("PLP", 0x0028, 15); AddFixed("PLT", 0x00ab, 14);
  AddFixed("PLX", 0x00fa, 15); AddFixed("PLY", 0x007a, 15);
  AddFixed("RTI", 0x0040, 15); AddFixed("RTL", 0x006b, 15);
  AddFixed("RTS", 0x0060, 15); AddFixed("SEC", 0x0038, 15);
  AddFixed("SEI", 0x0078, 15); AddFixed("SEM", 0x00f8, 14);
  AddFixed("STP", 0x00db, 15); AddFixed("TAD", 0x005b, 15);
  AddFixed("TAS", 0x001b, 15); AddFixed("TAX", 0x00aa, 15);
  AddFixed("TAY", 0x00a8, 15); AddFixed("TBD", 0x425b, 14);
  AddFixed("TBS", 0x421b, 14); AddFixed("TBX", 0x42aa, 14);
  AddFixed("TBY", 0x42a8, 14); AddFixed("TDA", 0x007b, 15);
  AddFixed("TDB", 0x427b, 14); AddFixed("TSA", 0x003b, 15);
  AddFixed("TSX", 0x00ba, 15); AddFixed("TXA", 0x008a, 15);
  AddFixed("TXB", 0x428a, 14); AddFixed("TXS", 0x009a, 15);
  AddFixed("TXY", 0x009b, 15); AddFixed("TYA", 0x0098, 15);
  AddFixed("TYB", 0x4298, 15); AddFixed("TYX", 0x00bb, 15);
  AddFixed("WIT", 0x00cb, 14); AddFixed("XAB", 0x8928, 14);
  AddFixed("CLD", 0x00d8,  1); AddFixed("SED", 0x00f8,  1);
  AddFixed("TCS", 0x001b, 15); AddFixed("TSC", 0x003b, 15);
  AddFixed("TCD", 0x005b, 15); AddFixed("TDC", 0x007b, 15);
  AddFixed("PHK", 0x004b,  1); AddFixed("WAI", 0x00cb,  1);
  AddFixed("XBA", 0x00eb,  1); AddFixed("SWA", 0x00eb,  1);
  AddFixed("XCE", 0x00fb,  1);
  AddFixed("DEA", (MomCPU >= CPUM7700) ? 0x001a : 0x003a, 15);
  AddFixed("INA", (MomCPU >= CPUM7700) ? 0x003a : 0x001a, 15);

  InstrZ = 0;
  AddRel("BCC" , 0x0090,  2, -1);
  AddRel("BLT" , 0x0090,  2, -1);
  AddRel("BCS" , 0x00b0,  2, -1);
  AddRel("BGE" , 0x00b0,  2, -1);
  AddRel("BEQ" , 0x00f0,  2, -1);
  AddRel("BMI" , 0x0030,  2, -1);
  AddRel("BNE" , 0x00d0,  2, -1);
  AddRel("BPL" , 0x0010,  2, -1);
  AddRel("BRA" , 0x8280,  2,  3);
  AddRel("BVC" , 0x0050,  2, -1);
  AddRel("BVS" , 0x0070,  2, -1);
  AddRel("BRL" , 0x8200, -1,  3);
  AddRel("BRAL", 0x8200, -1,  3);

  AddAcc("ADC", 0x60);
  AddAcc("AND", 0x20);
  AddAcc("CMP", 0xc0);
  AddAcc("CPA", 0xc0);
  AddAcc("EOR", 0x40);
  AddAcc("LDA", 0xa0);
  AddAcc("ORA", 0x00);
  AddAcc("SBC", 0xe0);
  AddAcc("STA", 0x80);

  AddRMW("ASL", 0x0a, 0x06);
  AddRMW("DEC", (MomCPU >= CPUM7700) ? 0x1a : 0x3a, 0xc6);
  AddRMW("ROL", 0x2a, 0x26);
  AddRMW("INC", (MomCPU >= CPUM7700) ? 0x3a : 0x1a, 0xe6);
  AddRMW("LSR", 0x4a, 0x46);
  AddRMW("ROR", 0x6a, 0x66);

  AddImm8("CLP", 0x00c2, 15);
  AddImm8("REP", 0x00c2, 15);
  AddImm8("LDT", 0x89c2, 14);
  AddImm8("SEP", 0x00e2, 15);
  AddImm8("RMPA", 0x89e2, 8);
  AddImm8("COP", 0x0002, 1);

  InstrZ = 0;
  AddXY("CPX", 0xe0, 0xe4, 0xec, 0xff, 0xff, 0xff, 0xff);
  AddXY("CPY", 0xc0, 0xc4, 0xcc, 0xff, 0xff, 0xff, 0xff);
  AddXY("LDX", 0xa2, 0xa6, 0xae, 0xff, 0xff, 0xb6, 0xbe);
  AddXY("LDY", 0xa0, 0xa4, 0xac, 0xb4, 0xbc, 0xff, 0xff);
  AddXY("STX", 0xff, 0x86, 0x8e, 0xff, 0xff, 0x96, 0xff);
  AddXY("STY", 0xff, 0x84, 0x8c, 0x94, 0xff, 0xff, 0xff);

  AddMulDiv("MPY", 0x0000, 14); AddMulDiv("MPYS", 0x0080, 12);
  AddMulDiv("DIV", 0x0020, 14); AddMulDiv("DIVS", 0x00a0, 12); /*???*/

  init_moto8_pseudo(InstTable, e_moto_8_le | e_moto_8_ddb);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);

  order_array_free(RelOrders);
  order_array_free(XYOrders);
}

static void MakeCode_7700(void)
{
  CodeLen = 0;
  DontPrint = False;
  BankReg = Reg_DT;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static void InitCode_7700(void)
{
  Reg_PG = 0;
  Reg_DT = 0;
  Reg_X = 0;
  Reg_M = 0;
  Reg_DPR = 0;
}

static Boolean IsDef_7700(void)
{
  return False;
}

static void SwitchFrom_7700(void)
{
  DeinitFields();
}

static void SwitchTo_7700(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*"; HeaderID = 0x19; NOPCode = 0xea;
  DivideChars = ","; HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffffffl;

  pASSUMERecs = ASSUME7700s;
  ASSUMERecCnt = sizeof(ASSUME7700s) / sizeof(*ASSUME7700s);

  MakeCode = MakeCode_7700; IsDef = IsDef_7700;
  SwitchFrom = SwitchFrom_7700; InitFields();
}

void code7700_init(void)
{
  CPU65816 = AddCPU("65816"    , SwitchTo_7700);
  CPUM7700 = AddCPU("MELPS7700", SwitchTo_7700);
  CPUM7750 = AddCPU("MELPS7750", SwitchTo_7700);
  CPUM7751 = AddCPU("MELPS7751", SwitchTo_7700);

  AddInitPassProc(InitCode_7700);
}
