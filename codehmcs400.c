/* codehmcs400.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator Hitachi HMCS400-Familie                                     */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "bpemu.h"
#include "strutil.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "fourpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "codehmcs400.h"

static CPUVar CPU614023, CPU614043, CPU614081;
static IntType CodeIntType, DataIntType, OpSizeType;
static ShortInt AdrMode;
static Word AdrVal;

enum
{
  ModA = 0,
  ModB = 1,
  ModX = 2,
  ModY = 3,
  ModW = 4,
  ModM = 5,
  ModMInc = 6,
  ModMDec = 7,
  ModSPX = 8,
  ModSPY = 9,
  ModImm = 10,
  ModDir = 11,
  ModMR = 12,
  ModNone = 0x7f
};

#define MModA (1 << ModA)
#define MModB (1 << ModB)
#define MModX (1 << ModX)
#define MModY (1 << ModY)
#define MModW (1 << ModW)
#define MModM (1 << ModM)
#define MModMInc (1 << ModMInc)
#define MModMDec (1 << ModMDec)
#define MModSPX (1 << ModSPX)
#define MModSPY (1 << ModSPY)
#define MModImm (1 << ModImm)
#define MModDir (1 << ModDir)
#define MModMR (1 << ModMR)

/*-------------------------------------------------------------------------*/

static void DecodeAdr(const tStrComp *pArg, Word Mask)
{
  tEvalResult EvalResult;
  int l;

  if (!as_strcasecmp(pArg->str.p_str, "A"))
  {
    AdrMode = ModA;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "B"))
  {
    AdrMode = ModB;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "X"))
  {
    AdrMode = ModX;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "Y"))
  {
    AdrMode = ModY;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "W"))
  {
    AdrMode = ModW;
    OpSizeType = UInt2;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "M"))
  {
    AdrMode = ModM;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "M+"))
  {
    AdrMode = ModMInc;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "M-"))
  {
    AdrMode = ModMDec;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "SPX"))
  {
    AdrMode = ModSPX;
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "SPY"))
  {
    AdrMode = ModSPY;
    goto AdrFound;
  }

  l = strlen(pArg->str.p_str);
  if ((l >= 3) && (l <= 4)
   && (as_toupper(pArg->str.p_str[0]) == 'M')
   && (as_toupper(pArg->str.p_str[1]) == 'R')
   && (isdigit(pArg->str.p_str[2]))
   && (isdigit(pArg->str.p_str[l - 1])))
  {
    AdrVal = pArg->str.p_str[2] - '0';
    if (l == 4)
      AdrVal = (AdrVal * 10) + (pArg->str.p_str[3] - '0');
    if (AdrVal < 16)
    {
      AdrMode = ModMR;
      goto AdrFound;
    }
  }

  if (0[pArg->str.p_str] == '#')
  {
    AdrVal = EvalStrIntExpressionOffsWithResult(pArg, 1, OpSizeType, &EvalResult);
    if (EvalResult.OK)
    {
      AdrMode = ModImm;
      AdrVal &= IntTypeDefs[OpSizeType].Mask;
    }
    goto AdrFound;
  }

  AdrVal = EvalStrIntExpressionWithResult(pArg, DataIntType, &EvalResult);
  if (EvalResult.OK)
  {
    if (!mFirstPassUnknown(EvalResult.Flags) && (AdrVal > SegLimits[SegData])) WrError(ErrNum_OverRange);
    else
    {
      AdrMode = ModDir;
      AdrVal &= IntTypeDefs[DataIntType].Mask;
      ChkSpace(SegData, EvalResult.AddrSpaceMask);
    }
    goto AdrFound;
  }


AdrFound:

  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone; AdrCnt = 0;
  }
}

/*-------------------------------------------------------------------------*/

static void DecodeLD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA | MModB | MModX | MModY | MModW | MModM | MModMInc | MModMDec | MModDir);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModB | MModY | MModM | MModSPX | MModSPY | MModImm | MModDir | MModMR);
        switch (AdrMode)
        {
          case ModB:
            WAsmCode[CodeLen++] = 0x048;
            break;
          case ModY:
            WAsmCode[CodeLen++] = 0x0af;
            break;
          case ModM:
            WAsmCode[CodeLen++] = 0x090;
            break;
          case ModSPX:
            WAsmCode[CodeLen++] = 0x068;
            break;
          case ModSPY:
            WAsmCode[CodeLen++] = 0x058;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x190;
            WAsmCode[CodeLen++] = AdrVal;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x230 | AdrVal;
            break;
          case ModMR:
            WAsmCode[CodeLen++] = 0x270 | AdrVal;
            break;
        }
        break;
      case ModB:
        DecodeAdr(&ArgStr[1], MModA | MModImm | MModM);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x0c8;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x200 | AdrVal;
            break;
          case ModM:
            WAsmCode[CodeLen++] = 0x400;
            break;
        }
        break;
      case ModX:
        DecodeAdr(&ArgStr[1], MModA | MModImm);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x0e8;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x220 | AdrVal;
            break;
        }
        break;
      case ModY:
        DecodeAdr(&ArgStr[1], MModA | MModImm);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x0d8;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x210 | AdrVal;
            break;
        }
        break;
      case ModW:
        DecodeAdr(&ArgStr[1], MModImm);
        switch (AdrMode)
        {
          case ModImm:
            WAsmCode[CodeLen++] = 0x0f0 | AdrVal;
            break;
        }
        break;
      case ModM:
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x094;
            break;
        }
        break;
      case ModMInc:
        DecodeAdr(&ArgStr[1], MModA | MModImm);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x050;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x290 | AdrVal;
            break;
        }
        break;
      case ModMDec:
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x0d0;
            break;
        }
        break;
      case ModDir:
      {
        Word HVal = AdrVal;

        DecodeAdr(&ArgStr[1], MModA | MModImm);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x194;
            WAsmCode[CodeLen++] = HVal;
            break;
          case ModImm:
            WAsmCode[CodeLen++] = 0x1a0 | AdrVal;
            WAsmCode[CodeLen++] = HVal;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeXCH(Word Code)
{
  Word HVal;

  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA | MModB | MModX | MModY | MModSPX | MModSPY | MModM | MModDir | MModMR);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModMR | MModM | MModDir);
        switch (AdrMode)
        {
          case ModMR:
            WAsmCode[CodeLen++] = 0x2f0 | AdrVal;
            break;
          case ModM:
            WAsmCode[CodeLen++] = 0x080;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x180;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
      case ModB:
        DecodeAdr(&ArgStr[1], MModM);
        switch (AdrMode)
        {
          case ModM:
            WAsmCode[CodeLen++] = 0x0c0;
            break;
        }
        break;
      case ModX:
        DecodeAdr(&ArgStr[1], MModSPX);
        switch (AdrMode)
        {
          case ModSPX:
            WAsmCode[CodeLen++] = 0x001;
            break;
        }
        break;
      case ModY:
        DecodeAdr(&ArgStr[1], MModSPY);
        switch (AdrMode)
        {
          case ModSPY:
            WAsmCode[CodeLen++] = 0x002;
            break;
        }
        break;
      case ModSPX:
        DecodeAdr(&ArgStr[1], MModX);
        switch (AdrMode)
        {
          case ModX:
            WAsmCode[CodeLen++] = 0x001;
            break;
        }
        break;
      case ModSPY:
        DecodeAdr(&ArgStr[1], MModY);
        switch (AdrMode)
        {
          case ModY:
            WAsmCode[CodeLen++] = 0x002;
            break;
        }
        break;
      case ModMR:
        HVal = AdrVal;
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x2f0 | HVal;
            break;
        }
        break;
      case ModM:
        DecodeAdr(&ArgStr[1], MModA | MModB);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x080;
            break;
          case ModB:
            WAsmCode[CodeLen++] = 0x0c0;
            break;
        }
        break;
      case ModDir:
        HVal = AdrVal;
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x180;
            WAsmCode[CodeLen++] = HVal;
            break;
        }
        break;
    }
  }
}

static void DecodeINCDEC(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModY | MModB);
    switch (AdrMode)
    {
      case ModY:
        WAsmCode[CodeLen++] = Code | 0x10;
        break;
      case ModB:
        WAsmCode[CodeLen++] = Code;
        break;
    }
  }
}

static void DecodeADD(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA | MModY);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModImm | MModM | MModDir);
        switch (AdrMode)
        {
          case ModImm:
            WAsmCode[CodeLen++] = 0x280 | AdrVal;
            break;
          case ModM:
            WAsmCode[CodeLen++] = 0x008;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x108;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
      case ModY:
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x054;
            break;
        }
        break;
    }
  }
}

static void DecodeADC(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModM | MModDir);
        switch (AdrMode)
        {
          case ModM:
            WAsmCode[CodeLen++] = 0x018;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x118;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
    }
  }
}

static void DecodeSUB(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModY);
    switch (AdrMode)
    {
      case ModY:
        DecodeAdr(&ArgStr[1], MModA);
        switch (AdrMode)
        {
          case ModA:
            WAsmCode[CodeLen++] = 0x0d4;
            break;
        }
        break;
    }
  }
}

static void DecodeSBC(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModM | MModDir);
        switch (AdrMode)
        {
          case ModM:
            WAsmCode[CodeLen++] = 0x098;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x198;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
    }
  }
}

static void DecodeOR(Word Code)
{
  UNUSED(Code);

  if (ArgCnt == 0)
    WAsmCode[CodeLen++] = 0x144;
  else if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModB | MModM | MModDir);
        switch (AdrMode)
        {
          case ModB:
            WAsmCode[CodeLen++] = 0x144;
            break;
          case ModM:
            WAsmCode[CodeLen++] = 0x00c;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x10c;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
    }
  }
}

static void DecodeAND(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModM | MModDir);
        switch (AdrMode)
        {
          case ModM:
            WAsmCode[CodeLen++] = 0x09c;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x19c;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
    }
  }
}

static void DecodeEOR(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[2], MModA);
    switch (AdrMode)
    {
      case ModA:
        DecodeAdr(&ArgStr[1], MModM | MModDir);
        switch (AdrMode)
        {
          case ModM:
            WAsmCode[CodeLen++] = 0x01c;
            break;
          case ModDir:
            WAsmCode[CodeLen++] = 0x11c;
            WAsmCode[CodeLen++] = AdrVal;
            break;
        }
        break;
    }
  }
}

static void DecodeCP(Word Code)
{
  Boolean IsLE;
  Word HVal;

  UNUSED(Code);

  if (!ChkArgCnt(3, 3))
    return;

  if (!as_strcasecmp(ArgStr[1].str.p_str, "NE"))
    IsLE = False;
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "LE"))
    IsLE = True;
  else
  {
    WrStrErrorPos(ErrNum_UndefCond, &ArgStr[1]);
    return;
  }
  DecodeAdr(&ArgStr[3], MModA | MModB | (IsLE ? 0 : MModM | MModDir | MModY) | MModImm);
  switch (AdrMode)
  {
    case ModA:
      DecodeAdr(&ArgStr[2], MModM | MModDir | (IsLE ? MModImm : 0));
      switch (AdrMode)
      {
        case ModM:
          WAsmCode[CodeLen++] = IsLE ? 0x014 : 0x004;
          break;
        case ModDir:
          WAsmCode[CodeLen++] = IsLE ? 0x114 : 0x104;
          WAsmCode[CodeLen++] = AdrVal;
          break;
        case ModImm:
          WAsmCode[CodeLen++] = 0x2b0 | AdrVal;
          break;
      }
      break;
    case ModB:
      DecodeAdr(&ArgStr[2], MModM);
      switch (AdrMode)
      {
        case ModM:
          WAsmCode[CodeLen++] = IsLE ? 0x0c4 : 0x044;
          break;
      }
      break;
    case ModY:
      DecodeAdr(&ArgStr[2], MModImm);
      switch (AdrMode)
      {
        case ModImm:
          WAsmCode[CodeLen++] = 0x070 | AdrVal;
          break;
      }
      break;
    case ModM:
      DecodeAdr(&ArgStr[2], MModImm | MModA | MModB);
      switch (AdrMode)
      {
        case ModImm:
          WAsmCode[CodeLen++] = 0x020 | AdrVal;
          break;
        case ModA:
          WAsmCode[CodeLen++] = 0x004;
          break;
        case ModB:
          WAsmCode[CodeLen++] = 0x044;
          break;
      }
      break;
    case ModDir:
      HVal = AdrVal;
      DecodeAdr(&ArgStr[2], MModImm | MModA);
      switch (AdrMode)
      {
        case ModImm:
          WAsmCode[CodeLen++] = 0x120 | AdrVal;
          WAsmCode[CodeLen++] = HVal;
          break;
        case ModA:
          WAsmCode[CodeLen++] = 0x104;
          WAsmCode[CodeLen++] = HVal;
          break;
      }
      break;
    case ModImm:
      HVal = AdrVal;
      DecodeAdr(&ArgStr[2], MModM | MModDir | (IsLE ? 0 : MModY));
      switch (AdrMode)
      {
        case ModM:
          WAsmCode[CodeLen++] = (IsLE ? 0x030 : 0x020) | HVal;
          break;
        case ModDir:
          WAsmCode[CodeLen++] = (IsLE ? 0x130 : 0x120) | HVal;
          WAsmCode[CodeLen++] = AdrVal;
          break;
        case ModY:
          WAsmCode[CodeLen++] = 0x070 | HVal;
          break;
      }
      break;
  }
}

static void DecodeBit(Word Code)
{
  if (ArgCnt == 1)
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "CA"))
      WAsmCode[CodeLen++] = Hi(Code);
    else
      WrError(ErrNum_InvAddrMode);
  }
  else if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    unsigned BitNo = EvalStrIntExpression(&ArgStr[1], UInt2, &OK);

    if (OK)
    {
      DecodeAdr(&ArgStr[2], MModDir | MModM);
      switch (AdrMode)
      {
        case ModDir:
          WAsmCode[CodeLen++] = Lo(Code) | 0x100 | BitNo;
          WAsmCode[CodeLen++] = AdrVal;
          break;
        case ModM:
          WAsmCode[CodeLen++] = Lo(Code) | BitNo;
          break;
      }
    }
  }
}

static void CodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
    WAsmCode[CodeLen++] = Code;
}

static void CodeImm(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    Word ImmVal = EvalStrIntExpression(&ArgStr[1], (Code & 0x8000) ? UInt2 : Int4, &OK);

    if (OK)
      WAsmCode[CodeLen++] = (Code & 0x3ff) | (ImmVal & 15);
  }
}

static void CodeDir(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModDir);
    if (AdrMode == ModDir)
    {
      WAsmCode[CodeLen++] = Code;
      WAsmCode[CodeLen++] = AdrVal;
    }
  }
}

static void CodeImmDir(Word Code)
{
  if (ChkArgCnt(2, 2))
  {
    Boolean OK;
    Word ImmVal = EvalStrIntExpression(&ArgStr[1], (Code & 0x8000) ? UInt2 : Int4, &OK);

    if (OK)
    {
      DecodeAdr(&ArgStr[2], MModDir);
      if (AdrMode == ModDir)
      {
        WAsmCode[CodeLen++] = (Code & 0x3ff) | (ImmVal & 15);
        WAsmCode[CodeLen++] = AdrVal;
      }
    }
  }
}

static void CodeIO(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word DirVal;

    DirVal = EvalStrIntExpressionWithResult(&ArgStr[1], UInt4, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegIO, EvalResult.AddrSpaceMask);
      WAsmCode[CodeLen++] = Code | (DirVal & 15);
    }
  }
}

static void CodeLong(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word DirVal;

    DirVal = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      WAsmCode[CodeLen++] = Code | ((DirVal >> 10) & 15);
      WAsmCode[CodeLen++] = DirVal & 0x3ff;
    }
  }
}

static void CodeBR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word DirVal;

    DirVal = EvalStrIntExpressionWithResult(&ArgStr[1], CodeIntType, &EvalResult);
    if (EvalResult.OK && ChkSamePage(EProgCounter() + 1, DirVal, 8, EvalResult.Flags))
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      WAsmCode[CodeLen++] = Code | (DirVal & 0xff);
    }
  }
}

static void CodeCAL(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    tEvalResult EvalResult;
    Word DirVal;

    DirVal = EvalStrIntExpressionWithResult(&ArgStr[1], UInt6, &EvalResult);
    if (EvalResult.OK)
    {
      ChkSpace(SegCode, EvalResult.AddrSpaceMask);
      WAsmCode[CodeLen++] = Code | (DirVal & 0x3f);
    }
  }
}

static void DecodeDATA_HMCS400(Word Code)
{
  UNUSED(Code);

  DecodeDATA(Int10, Int4);
}

static void DecodeSFR(Word Code)
{
  UNUSED(Code);

  CodeEquate(SegData, 0, SegLimits[SegData]);
}

/*-------------------------------------------------------------------------*/

static void AddX(const char *pOpPart, Word Code, InstProc Proc)
{
  char OpPart[30];

  AddInstTable(InstTable, pOpPart, Code, Proc);
  as_snprintf(OpPart, sizeof(OpPart), "%sX", pOpPart);
  AddInstTable(InstTable, OpPart, Code | 1, Proc);
}

static void AddXY(const char *pOpPart, Word Code, InstProc Proc)
{
  char OpPart[30];

  AddInstTable(InstTable, pOpPart, Code, Proc);
  as_snprintf(OpPart, sizeof(OpPart), "%sX", pOpPart);
  AddInstTable(InstTable, OpPart, Code | 1, Proc);
  as_snprintf(OpPart, sizeof(OpPart), "%sY", pOpPart);
  AddInstTable(InstTable, OpPart, Code | 2, Proc);
  as_snprintf(OpPart, sizeof(OpPart), "%sXY", pOpPart);
  AddInstTable(InstTable, OpPart, Code | 3, Proc);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);
  SetDynamicInstTable(InstTable);

  AddInstTable(InstTable, "LD", 0, DecodeLD);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "INC", 0x04c, DecodeINCDEC);
  AddInstTable(InstTable, "DEC", 0x0cf, DecodeINCDEC);
  AddInstTable(InstTable, "ADD", 0, DecodeADD);
  AddInstTable(InstTable, "ADC", 0, DecodeADC);
  AddInstTable(InstTable, "SUB", 0, DecodeSUB);
  AddInstTable(InstTable, "SBC", 0, DecodeSBC);
  AddInstTable(InstTable, "OR", 0, DecodeOR);
  AddInstTable(InstTable, "AND", 0, DecodeAND);
  AddInstTable(InstTable, "EOR", 0, DecodeEOR);
  AddInstTable(InstTable, "CP", 0, DecodeCP);
  AddInstTable(InstTable, "BSET", 0xef84, DecodeBit);
  AddInstTable(InstTable, "BCLR", 0xec88, DecodeBit);
  AddInstTable(InstTable, "BTST", 0x6f8c, DecodeBit);

  AddInstTable(InstTable, "LAB",    0x048, CodeFixed);
  AddInstTable(InstTable, "LBA",    0x0c8, CodeFixed);
  AddInstTable(InstTable, "LAY",    0x0af, CodeFixed);
  AddInstTable(InstTable, "LASPX",  0x068, CodeFixed);
  AddInstTable(InstTable, "LASPY",  0x058, CodeFixed);
  AddInstTable(InstTable, "LXA",    0x0e8, CodeFixed);
  AddInstTable(InstTable, "LYA",    0x0d8, CodeFixed);
  AddInstTable(InstTable, "IY",     0x05c, CodeFixed);
  AddInstTable(InstTable, "DY",     0x0df, CodeFixed);
  AddInstTable(InstTable, "AYY",    0x054, CodeFixed);
  AddInstTable(InstTable, "SYY",    0x0d4, CodeFixed);
  AddInstTable(InstTable, "XSPX",   0x001, CodeFixed);
  AddInstTable(InstTable, "XSPY",   0x002, CodeFixed);
  AddInstTable(InstTable, "XSPXY",  0x003, CodeFixed);
  AddXY("LAM", 0x090, CodeFixed);
  AddXY("LBM", 0x040, CodeFixed);
  AddXY("LMA", 0x094, CodeFixed);
  AddX("LMAIY", 0x050, CodeFixed);
  AddX("LMADY", 0x0d0, CodeFixed);
  AddXY("XMA", 0x080, CodeFixed);
  AddXY("XMB", 0x0c0, CodeFixed);
  AddInstTable(InstTable, "IB",     0x04c, CodeFixed);
  AddInstTable(InstTable, "DB",     0x0cf, CodeFixed);
  AddInstTable(InstTable, "DAA",    0x0a6, CodeFixed);
  AddInstTable(InstTable, "DAS",    0x0aa, CodeFixed);
  AddInstTable(InstTable, "NEGA",   0x060, CodeFixed);
  AddInstTable(InstTable, "COMB",   0x140, CodeFixed);
  AddInstTable(InstTable, "ROTR",   0x0a0, CodeFixed);
  AddInstTable(InstTable, "ROTL",   0x0a1, CodeFixed);
  AddInstTable(InstTable, "SEC",    0x0ef, CodeFixed);
  AddInstTable(InstTable, "REC",    0x0ec, CodeFixed);
  AddInstTable(InstTable, "TC",     0x06f, CodeFixed);
  AddInstTable(InstTable, "AM",     0x008, CodeFixed);
  AddInstTable(InstTable, "AMC",    0x018, CodeFixed);
  AddInstTable(InstTable, "SMC",    0x098, CodeFixed);
  AddInstTable(InstTable, "ANM",    0x09c, CodeFixed);
  AddInstTable(InstTable, "ORM",    0x00c, CodeFixed);
  AddInstTable(InstTable, "EORM",   0x01c, CodeFixed);
  AddInstTable(InstTable, "ANEM",   0x004, CodeFixed);
  AddInstTable(InstTable, "BNEM",   0x044, CodeFixed);
  AddInstTable(InstTable, "ALEM",   0x014, CodeFixed);
  AddInstTable(InstTable, "BLEM",   0x0c4, CodeFixed);
  AddInstTable(InstTable, "RTN",    0x010, CodeFixed);
  AddInstTable(InstTable, "RTNI",   0x011, CodeFixed);
  AddInstTable(InstTable, "SED",    0x0e4, CodeFixed);
  AddInstTable(InstTable, "RED",    0x064, CodeFixed);
  AddInstTable(InstTable, "TD",     0x0e0, CodeFixed);
  AddInstTable(InstTable, "NOP",    0x000, CodeFixed);
  AddInstTable(InstTable, "SBY",    0x14c, CodeFixed);
  AddInstTable(InstTable, "STOP",   0x14d, CodeFixed);

  AddInstTable(InstTable, "LAI",    0x230, CodeImm);
  AddInstTable(InstTable, "LBI",    0x200, CodeImm);
  AddInstTable(InstTable, "LMIIY",  0x290, CodeImm);
  AddInstTable(InstTable, "LWI",   0x80f0, CodeImm);
  AddInstTable(InstTable, "LXI",    0x220, CodeImm);
  AddInstTable(InstTable, "LYI",    0x210, CodeImm);
  AddInstTable(InstTable, "AI",     0x280, CodeImm);
  AddInstTable(InstTable, "INEM",   0x020, CodeImm);
  AddInstTable(InstTable, "YNEI",   0x070, CodeImm);
  AddInstTable(InstTable, "ILEM",   0x030, CodeImm);
  AddInstTable(InstTable, "ALEI",   0x2b0, CodeImm);
  AddInstTable(InstTable, "SEM",   0x8084, CodeImm);
  AddInstTable(InstTable, "REM",   0x8088, CodeImm);
  AddInstTable(InstTable, "TM",    0x808c, CodeImm);
  AddInstTable(InstTable, "P",      0x1b0, CodeImm);
  AddInstTable(InstTable, "TBR",    0x0b0, CodeImm);

  AddInstTable(InstTable, "LAMD",   0x190, CodeDir);
  AddInstTable(InstTable, "LMAD",   0x194, CodeDir);
  AddInstTable(InstTable, "XMAD",   0x180, CodeDir);
  AddInstTable(InstTable, "AMD",    0x108, CodeDir);
  AddInstTable(InstTable, "AMCD",   0x118, CodeDir);
  AddInstTable(InstTable, "SMCD",   0x198, CodeDir);
  AddInstTable(InstTable, "ANMD",   0x19c, CodeDir);
  AddInstTable(InstTable, "ORMD",   0x10c, CodeDir);
  AddInstTable(InstTable, "EORMD",  0x11c, CodeDir);
  AddInstTable(InstTable, "ANEMD",  0x104, CodeDir);
  AddInstTable(InstTable, "ALEMD",  0x114, CodeDir);

  AddInstTable(InstTable, "LMID",   0x1a0, CodeImmDir);
  AddInstTable(InstTable, "INEMD",  0x120, CodeImmDir);
  AddInstTable(InstTable, "ILEMD",  0x130, CodeImmDir);
  AddInstTable(InstTable, "SEMD",  0x8184, CodeImmDir);
  AddInstTable(InstTable, "REMD",  0x8188, CodeImmDir);
  AddInstTable(InstTable, "TMD",   0x818c, CodeImmDir);

  AddInstTable(InstTable, "LAMR",   0x270, CodeIO);
  AddInstTable(InstTable, "XMRA",   0x2f0, CodeIO);
  AddInstTable(InstTable, "SEDD",   0x2e0, CodeIO);
  AddInstTable(InstTable, "REDD",   0x260, CodeIO);
  AddInstTable(InstTable, "TDD",    0x2a0, CodeIO);
  AddInstTable(InstTable, "LAR",    0x250, CodeIO);
  AddInstTable(InstTable, "LBR",    0x240, CodeIO);
  AddInstTable(InstTable, "LRA",    0x2d0, CodeIO);
  AddInstTable(InstTable, "LRB",    0x2c0, CodeIO);

  AddInstTable(InstTable, "BRL",    0x270, CodeLong);
  AddInstTable(InstTable, "JMPL",   0x250, CodeLong);
  AddInstTable(InstTable, "CALL",   0x260, CodeLong);
  AddInstTable(InstTable, "BR",     0x300, CodeBR);
  AddInstTable(InstTable, "CAL",    0x1c0, CodeCAL);

  AddInstTable(InstTable, "RES", 0, DecodeRES);
  AddInstTable(InstTable, "DATA", 0, DecodeDATA_HMCS400);
  AddInstTable(InstTable, "SFR", 0, DecodeSFR);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void  MakeCode_HMCS400(void)
{
  CodeLen = 0; DontPrint = False; OpSizeType = Int4;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}


static Boolean IsDef_HMCS400(void)
{
  return False;
}

static void SwitchFrom_HMCS400(void)
{
  DeinitFields();
}

static void SwitchTo_HMCS400(void)
{
  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("HMCS400");

  TurnWords = False;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = pDescr->Id;
  NOPCode = 0x0000;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = (1 << SegCode) | (1 << SegData) | (1 << SegIO);
  Grans[SegCode] = 2; ListGrans[SegCode] = 2; SegInits[SegCode] = 0;
  Grans[SegData] = 1; ListGrans[SegData] = 1; SegInits[SegCode] = 0;
  Grans[SegIO]   = 1; ListGrans[SegIO]   = 1; SegInits[SegIO]   = 0;
  if (MomCPU == CPU614023)
  {
    CodeIntType = UInt11;
    DataIntType = UInt8;
    SegLimits[SegData] = 159;
  }
  else if (MomCPU == CPU614043)
  {
    CodeIntType = UInt12;
    DataIntType = UInt8;
    SegLimits[SegData] = 255;
  }
  else if (MomCPU == CPU614081)
  {
    CodeIntType = UInt13;
    DataIntType = UInt9;
    SegLimits[SegData] = 511;
  }
  SegLimits[SegIO] = 15;
  SegLimits[SegCode] = IntTypeDefs[CodeIntType].Max;

  MakeCode = MakeCode_HMCS400;
  IsDef = IsDef_HMCS400;
  SwitchFrom = SwitchFrom_HMCS400; InitFields();

}

void codehmcs400_init(void)
{
  CPU614023 = AddCPU("HD614023", SwitchTo_HMCS400);
  CPU614043 = AddCPU("HD614043", SwitchTo_HMCS400);
  CPU614081 = AddCPU("HD614081", SwitchTo_HMCS400);
}
