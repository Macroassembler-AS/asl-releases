/* code56k.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* AS-Codegeneratormodul fuer die DSP56K-Familie                             */
/*                                                                           */
/*****************************************************************************/

#include "stdinc.h"
#include <string.h>
#include <ctype.h>

#include "strutil.h"
#include "chunks.h"
#include "errmsg.h"
#include "asmdef.h"
#include "asmsub.h"
#include "asmpars.h"
#include "asmitree.h"
#include "codepseudo.h"
#include "chartrans.h"
#include "codevars.h"
#include "onoff_common.h"

#include "code56k.h"

typedef struct
{
  LongWord Code;
  CPUVar MinCPU;
} FixedOrder;

typedef enum
{
  ParAB, ParABShl1, ParABShl2, ParXYAB, ParABXYnAB, ParABBA, ParXYnAB, ParMul, ParFixAB
} ParTyp;
typedef struct
{
  ParTyp Typ;
  Byte Code;
} ParOrder;

#define CondCount (sizeof(CondNames) / sizeof(*CondNames))
static const char CondNames[][3] =
{
  "CC", "GE", "NE", "PL", "NN", "EC", "LC", "GT", "CS", "LT", "EQ", "MI",
  "NR", "ES", "LS", "LE", "HS", "LO"
};

static const Byte MacTable[4][4] =
{
  { 0, 2, 5, 4 },
  { 2, 0xff, 6, 7 },
  { 5, 6, 1, 3 },
  { 4, 7, 3, 0xff }
};

static const Byte Mac4Table[4][4] =
{
  { 0, 13, 10, 4 },
  { 5, 1, 14, 11 },
  { 2, 6, 8, 15 },
  { 12, 3, 7, 9 }
};

static const Byte Mac2Table[4] =
{
  1, 3, 2, 0
};

enum
{
  ModNone = -1,
  ModImm = 0,
  ModAbs = 1,
  ModIReg = 2,
  ModPreDec = 3,
  ModPostDec = 4,
  ModPostInc = 5,
  ModIndex = 6,
  ModModDec = 7,
  ModModInc = 8,
  ModDisp = 9
};

#define MModImm (1 << ModImm)
#define MModAbs (1 << ModAbs)
#define MModIReg (1 << ModIReg)
#define MModPreDec (1 << ModPreDec)
#define MModPostDec (1 << ModPostDec)
#define MModPostInc (1 << ModPostInc)
#define MModIndex (1 << ModIndex)
#define MModModDec (1 << ModModDec)
#define MModModInc (1 << ModModInc)
#define MModDisp (1 << ModDisp)

#define MModNoExt (MModIReg | MModPreDec | MModPostDec | MModPostInc | MModIndex | MModModDec | MModModInc)
#define MModNoImm (MModAbs | MModNoExt)
#define MModAll (MModNoImm | MModImm)

#define SegLData SegBData /* SegYData + 1 */

#define MSegCode (1 << SegCode)
#define MSegXData (1 << SegXData)
#define MSegYData (1 << SegYData)
#define MSegLData (1 << SegLData)

typedef struct
{
  ShortInt Type;
  LongInt Mode, Val;
  Byte Seg, ShortMode;
  Boolean ForceImmLong;
  tSymbolFlags AbsSymFlags;
  int Cnt;
} tAdrResult;

static CPUVar CPU56000, CPU56002, CPU56300;
static IntType AdrInt;
static LargeInt MemLimit;

static tStrComp LeftComp, MidComp, RightComp, Left1Comp, Left2Comp, Right1Comp, Right2Comp;

static FixedOrder *FixedOrders;
static ParOrder *ParOrders;

/*----------------------------------------------------------------------------------------------*/

static void SplitArg(const tStrComp *pOrig, tStrComp *pLDest, tStrComp *pRDest)
{
  char *p;

  p = QuotPos(pOrig->str.p_str, ',');
  if (!p)
  {
    StrCompReset(pRDest);
    StrCompCopy(pLDest, pOrig);
  }
  else
    StrCompSplitCopy(pLDest, pRDest, pOrig, p);
}

static unsigned CutSize(const tStrComp *pArg, Byte *ShortMode)
{
  switch (*pArg->str.p_str)
  {
    case '>':
      *ShortMode = 2;
      return 1;
    case '<':
      *ShortMode = 1;
      return 1;
    default:
      *ShortMode = 0;
      return 0;
  }
}

static Boolean DecodeReg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][3] =
  {
    "X0", "X1", "Y0", "Y1", "A0", "B0", "A2", "B2", "A1", "B1", "A", "B"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      *Erg = z + 4;
      return True;
    }
  if ((strlen(Asc) == 2) && (Asc[1] >= '0') && (Asc[1] <= '7'))
    switch (as_toupper(*Asc))
    {
      case 'R':
        *Erg = 16 + Asc[1] - '0';
        return True;
      case 'N':
        *Erg = 24 + Asc[1] - '0';
        return True;
    }
  return False;
#undef RegCount
}

static Boolean DecodeALUReg(char *Asc, LongInt *Erg,
                            Boolean MayX, Boolean MayY, Boolean MayAcc)
{
  Boolean Result = False;

  if (!DecodeReg(Asc, Erg))
    return Result;

  switch (*Erg)
  {
    case 4:
    case 5:
      if (MayX)
      {
        Result = True;
        (*Erg) -= 4;
      }
      break;
    case 6:
    case 7:
      if (MayY)
      {
        Result = True;
        (*Erg) -= 6;
      }
      break;
    case 14:
    case 15:
      if (MayAcc)
      {
        Result = True;
        (*Erg) -= (MayX || MayY) ? 12 : 14;
      }
      break;
  }

  return Result;
}

static Boolean DecodeLReg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][4] =
  {
    "A10", "B10", "X", "Y", "A", "B", "AB", "BA"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      *Erg = z;
      return True;
    }

  return False;
#undef RegCount
}

static Boolean DecodeXYABReg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][3] =
  {
    "B", "A", "X", "Y", "X0", "Y0", "X1", "Y1"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      *Erg = z;
      return True;
    }

  return False;
#undef RegCount
}

static Boolean DecodeXYAB0Reg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][3] =
  {
    "A0", "B0", "X0", "Y0", "X1", "Y1"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      *Erg = z + 2;
      return True;
    }

  return False;
#undef RegCount
}

static Boolean DecodeXYAB1Reg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
  static const char RegNames[][3] =
  {
    "A1", "B1", "X0", "Y0", "X1", "Y1"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      *Erg = z + 2;
      return True;
    }

  return False;
#undef RegCount
}

static Boolean DecodePCReg(char *Asc, LongInt *Erg)
{
#define RegCount (sizeof(RegNames) / sizeof(*RegNames))
                                /** vvvv ab 56300 ? */
  static const char RegNames[][4] =
  {
    "SZ", "SR", "OMR", "SP", "SSH", "SSL", "LA", "LC"
  };
  Word z;

  for (z = 0; z < RegCount; z++)
    if (!as_strcasecmp(Asc, RegNames[z]))
    {
      (*Erg) = z;
      return True;
    }

  return False;
#undef RegCount
}

static Boolean DecodeAddReg(char *Asc, LongInt *Erg)
{
  if ((strlen(Asc) == 2) && (as_toupper(*Asc) == 'M') && (Asc[1] >= '0') && (Asc[1] <= '7'))
  {
    *Erg = Asc[1] - '0';
    return True;
  }
  /* >=56300 ? */
  if (!as_strcasecmp(Asc, "EP"))
  {
    *Erg = 0x0a;
    return True;
  }
  if (!as_strcasecmp(Asc, "VBA"))
  {
    *Erg = 0x10;
    return True;
  }
  if (!as_strcasecmp(Asc, "SC"))
  {
    *Erg = 0x11;
    return True;
  }

  return False;
}

static Boolean DecodeGeneralReg(char *Asc, LongInt *Erg)
{
  if (DecodeReg(Asc, Erg))
    return True;
  if (DecodePCReg(Asc, Erg))
  {
    (*Erg) += 0x38;
    return True;
  }
  if (DecodeAddReg(Asc, Erg))
  {
    (*Erg) += 0x20;
    return True;
  }
  return False;
}

static Boolean DecodeCtrlReg(char *Asc, LongInt *Erg)
{
  if (DecodeAddReg(Asc, Erg))
    return True;
  if (DecodePCReg(Asc, Erg))
  {
    (*Erg) += 0x18;
    return True;
  }
  return False;
}

static Boolean DecodeControlReg(char *Asc, LongInt *Erg)
{
  Boolean Result = True;

  if (!as_strcasecmp(Asc, "MR"))
    *Erg = 0;
  else if (!as_strcasecmp(Asc, "CCR"))
    *Erg = 1;
  else if ((!as_strcasecmp(Asc, "OMR")) || (!as_strcasecmp(Asc, "COM")))
    *Erg = 2;
  else if ((!as_strcasecmp(Asc, "EOM")) && (MomCPU >= CPU56000))
    *Erg = 3;
  else
    Result = False;

  return Result;
}

static void DecodeAdr(const tStrComp *pArg, Word Erl, Byte ErlSeg, tAdrResult *pResult)
{
  static const char ModMasks[ModModInc + 1][8] =
  {
    "","", "(Rx)", "-(Rx)", "(Rx)-", "(Rx)+", "(Rx+Nx)", "(Rx)-Nx", "(Rx)+Nx"
  };
  static const Byte ModCodes[ModModInc + 1] =
  {
    0, 0, 4, 7, 2, 3, 5, 0, 1
  };
#define Seg56KCount (sizeof(Seg56KNames) / sizeof(*Seg56KNames))
  static const char Seg56KNames[] =
  {
    'P', 'X', 'Y', 'L'
  };
  static Byte Seg56KVals[] =
  {
    SegCode, SegXData, SegYData, SegLData
  };
  int z, l, ArgLen;
  unsigned SegIndex, Offset;
  Boolean OK;
  tEvalResult EvalResult;
  Byte OrdVal;
  char *pp, *np, save;
  tStrComp Arg;

  pResult->Type = ModNone;
  pResult->Cnt = 0;
  pResult->ShortMode = 0;

  /* Adressierungsmodi vom 56300 abschneiden */

  if (MomCPU < CPU56300)
    Erl |= (~MModDisp);

  /* Defaultsegment herausfinden */

  if (ErlSeg & MSegXData)
    pResult->Seg = SegXData;
  else if (ErlSeg & MSegYData)
    pResult->Seg = SegYData;
  else if (ErlSeg & MSegCode)
    pResult->Seg = SegCode;
  else
    pResult->Seg = SegNone;

  /* Zielsegment vorgegeben ? */

  Offset = 0;
  for (SegIndex = 0; SegIndex < Seg56KCount; SegIndex++)
    if ((as_toupper(*pArg->str.p_str) == Seg56KNames[SegIndex]) && (pArg->str.p_str[1] == ':'))
    {
      pResult->Seg = Seg56KVals[SegIndex];
      Offset = 2;
    }
  StrCompRefRight(&Arg, pArg, Offset);

  /* Adressausdruecke abklopfen: dazu mit Referenzstring vergleichen */

  ArgLen = strlen(Arg.str.p_str);
  for (z = ModIReg; z <= ModModInc; z++)
    if (ArgLen == (int)strlen(ModMasks[z]))
    {
      pResult->Mode = 0xffff;
      for (l = 0; l <= ArgLen; l++)
        if (ModMasks[z][l] == 'x')
        {
          OrdVal = Arg.str.p_str[l] - '0';
          if (OrdVal > 7)
            break;
          else if (pResult->Mode == 0xffff)
            pResult->Mode = OrdVal;
          else if (pResult->Mode != OrdVal)
          {
            WrError(ErrNum_InvRegPair);
            goto chk;
          }
        }
        else if (ModMasks[z][l] != as_toupper(Arg.str.p_str[l]))
          break;
      if (l > ArgLen)
      {
        pResult->Type = z;
        pResult->Mode += ModCodes[z] << 3;
        goto chk;
      }
    }

  /* immediate ? */

  if (*Arg.str.p_str == '#')
  {
    if (Arg.str.p_str[1] == '>')
    {
      pResult->ForceImmLong = TRUE;
      pResult->Val = EvalStrIntExpressionOffs(&Arg, 2, Int24, &OK);
    }
    else
    {
      pResult->ForceImmLong = FALSE;
      pResult->Val = EvalStrIntExpressionOffs(&Arg, 1, Int24, &OK);
    }
    if (OK)
    {
      pResult->Type = ModImm; pResult->Cnt = 1; pResult->Mode = 0x34;
      goto chk;
    }
  }

  /* Register mit Displacement bei 56300 */

  if (IsIndirect(Arg.str.p_str))
  {
    tStrComp IArg;

    Arg.str.p_str[--ArgLen] = '\0'; Arg.Pos.Len--;
    StrCompRefRight(&IArg, &Arg, 1);
    pp = strchr(IArg.str.p_str, '+');
    np = strchr(IArg.str.p_str, '-');
    if ((!pp) || ((np) && (np < pp)))
      pp = np;
    if (pp)
    {
      save = *pp;
      *pp = '\0';
      if ((DecodeGeneralReg(IArg.str.p_str, &pResult->Mode)) && (pResult->Mode >= 16) && (pResult->Mode <= 23))
      {
        *pp = save;
        pResult->Mode -= 16;
        pResult->Val = EvalStrIntExpressionOffsWithResult(&IArg, pp - IArg.str.p_str, Int24, &EvalResult);
        if (EvalResult.OK)
        {
          if (mFirstPassUnknown(EvalResult.Flags))
            pResult->Val &= 63;
          pResult->Type = ModDisp;
        }
        goto chk;
      }
      *pp = save;
    }
  }

  /* dann absolut */

  Offset = CutSize(&Arg, &pResult->ShortMode);
  pResult->Val = EvalStrIntExpressionOffsWithResult(&Arg, Offset, AdrInt, &EvalResult);
  if (EvalResult.OK)
  {
    pResult->Type = ModAbs;
    pResult->AbsSymFlags = EvalResult.Flags;
    pResult->Mode = 0x30; pResult->Cnt = 1;
    if ((pResult->Seg & ((1 << SegCode) | (1 << SegXData) | (1 << SegYData))) != 0)
      ChkSpace(pResult->Seg, EvalResult.AddrSpaceMask);
    goto chk;
  }

chk:
  if ((pResult->Type != ModNone) && (!(Erl & (1 << pResult->Type))))
  {
    WrError(ErrNum_InvAddrMode);
    pResult->Cnt = 0;
    pResult->Type = ModNone;
  }
  if ((pResult->Seg != SegNone) && (!(ErlSeg & (1 << pResult->Seg))))
  {
    WrError(ErrNum_InvSegment);
    pResult->Cnt = 0;
    pResult->Type = ModNone;
  }
}

static Boolean DecodeOpPair(const tStrComp *pLeft, const tStrComp *pRight, Byte WorkSeg,
                            LongInt *Dir, LongInt *Reg1, LongInt *Reg2, tAdrResult *pResult)
{
  Boolean Result = False;

  if (DecodeALUReg(pLeft->str.p_str, Reg1, WorkSeg == SegXData, WorkSeg == SegYData, True))
  {
    if (DecodeALUReg(pRight->str.p_str, Reg2, WorkSeg == SegXData, WorkSeg == SegYData, True))
    {
      *Dir = 2;
      pResult->Type = ModNone;
      pResult->Mode = 0;
      pResult->Cnt = 0;
      pResult->Val = 0;
      Result = True;
    }
    else
    {
      *Dir = 0;
      *Reg2 = -1;
      DecodeAdr(pRight, MModNoImm, 1 << WorkSeg, pResult);
      if (pResult->Type != ModNone)
        Result = True;
    }
  }
  else if (DecodeALUReg(pRight->str.p_str, Reg1, WorkSeg == SegXData, WorkSeg == SegYData, True))
  {
    *Dir = 1;
    *Reg2 = -1;
    DecodeAdr(pLeft, MModAll, 1 << WorkSeg, pResult);
    if (pResult->Type != ModNone)
      Result = True;
  }

  return Result;
}

static LongInt TurnXY(LongInt Inp)
{
  switch (Inp)
  {
    case 4:
    case 7:
      return Inp - 4;
    case 5:
    case 6:
      return 7 - Inp;
    default:  /* wird nie erreicht */
      return 0;
  }
}

static Boolean DecodeTFR(const tStrComp *pArg, LongInt *Erg)
{
  LongInt Part1, Part2;

  SplitArg(pArg, &LeftComp, &RightComp);
  if (!DecodeALUReg(RightComp.str.p_str, &Part2, False, False, True)) return False;
  if (!DecodeReg(LeftComp.str.p_str, &Part1)) return False;
  if ((Part1 < 4) || ((Part1 > 7) && (Part1 < 14)) || (Part1 > 15)) return False;
  if (Part1 > 13)
  {
    if (((Part1 ^ Part2) & 1) == 0) return False;
    else
      Part1 = 0;
  }
  else
    Part1 = TurnXY(Part1) + 4;
  *Erg = (Part1 << 1) + Part2;
  return True;
}

static Boolean DecodeRR(const tStrComp *pArg, LongInt *Erg)
{
  LongInt Part1, Part2;

  SplitArg(pArg, &LeftComp, &RightComp);
  if (!DecodeGeneralReg(RightComp.str.p_str, &Part2)) return False;
  if ((Part2 < 16) || (Part2 > 23)) return False;
  if (!DecodeGeneralReg(LeftComp.str.p_str, &Part1)) return False;
  if ((Part1 < 16) || (Part1 > 23)) return False;
  *Erg = (Part2 & 7) + ((Part1 & 7) << 8);
  return True;
}

static Boolean DecodeCondition(char *Asc, Word *Erg)
{
  Boolean Result;

  (*Erg) = 0;
  while ((*Erg < CondCount) && (as_strcasecmp(CondNames[*Erg], Asc)))
    (*Erg)++;
  if (*Erg == CondCount - 1)
    *Erg = 8;
  Result = (*Erg < CondCount);
  *Erg &= 15;
  return Result;
}

static Boolean DecodeMOVE_0(void)
{
  DAsmCode[0] = 0x200000;
  CodeLen = 1;
  return True;
}

static Boolean DecodeMOVE_1(int Start)
{
  LongInt RegErg, RegErg2, IsY, MixErg, l;
  char c;
  Word Condition;
  Boolean Result = False;
  Byte SegMask;

  if (!as_strncasecmp(ArgStr[Start].str.p_str, "IF", 2))
  {
    l = strlen(ArgStr[Start].str.p_str);
    if (!as_strcasecmp(ArgStr[Start].str.p_str + l - 2, ".U"))
    {
      RegErg = 0x1000;
      l -= 2;
    }
    else
      RegErg = 0;
    c = ArgStr[Start].str.p_str[l];
    ArgStr[Start].str.p_str[l] = '\0';
    if (DecodeCondition(ArgStr[Start].str.p_str + 2, &Condition))
    {
      if (ChkMinCPUExt(CPU56300, ErrNum_AddrModeNotSupported))
      {
        DAsmCode[0] = 0x202000 + (Condition << 8) + RegErg;
        CodeLen = 1;
        return True;
      }
    }
    ArgStr[Start].str.p_str[l] = c;
  }

  SplitArg(&ArgStr[Start], &LeftComp, &RightComp);

  /* 1. Register-Update */

  if (*RightComp.str.p_str == '\0')
  {
    tAdrResult AdrResult;

    DecodeAdr(&LeftComp, MModPostDec | MModPostInc | MModModDec | MModModInc, 0, &AdrResult);
    if (AdrResult.Type != ModNone)
    {
      Result = True;
      DAsmCode[0] = 0x204000 + (AdrResult.Mode << 8);
      CodeLen = 1;
    }
    return Result;
  }

  /* 2. Ziel ist Register */

  if (DecodeReg(RightComp.str.p_str, &RegErg))
  {
    tAdrResult AdrResult;

    AdrResult.Seg = SegNone;
    if (DecodeReg(LeftComp.str.p_str, &RegErg2))
    {
      Result = True;
      DAsmCode[0] = 0x200000 + (RegErg << 8) + (RegErg2 << 13);
      CodeLen = 1;
    }
    else
    {
      /* A und B gehen auch als L:..., in L-Zweig zwingen! */

      SegMask = MSegXData + MSegYData;
      if ((RegErg == 14) || (RegErg == 15))
        SegMask |= MSegLData;
      DecodeAdr(&LeftComp, MModAll | MModDisp, SegMask, &AdrResult);
      if (AdrResult.Seg != SegLData)
      {
        IsY = Ord(AdrResult.Seg == SegYData);
        MixErg = ((RegErg & 0x18) << 17) + (IsY << 19) + ((RegErg & 7) << 16);
        if (AdrResult.Type == ModDisp)
        {
          if ((AdrResult.Val < 63) && (AdrResult.Val > -64) && (RegErg <= 15))
          {
            DAsmCode[0] = 0x020090 + ((AdrResult.Val & 1) << 6) + ((AdrResult.Val & 0x7e) << 10)
                        + (AdrResult.Mode << 8) + (IsY << 5) + RegErg;
            CodeLen = 1;
          }
          else
          {
            DAsmCode[0] = 0x0a70c0 + (AdrResult.Mode << 8) + (IsY << 16) + RegErg;
            DAsmCode[1] = AdrResult.Val;
            CodeLen = 2;
          }
        }
        else if (!AdrResult.ForceImmLong && (AdrResult.Type == ModImm) && RangeCheck(AdrResult.Val, UInt8))
        {
          Result = True;
          DAsmCode[0] = 0x200000 + (RegErg << 16) + ((AdrResult.Val & 0xff) << 8);
          CodeLen = 1;
        }
        else if ((AdrResult.Type == ModAbs) && (AdrResult.Val <= 63) && (AdrResult.Val >= 0) && (AdrResult.ShortMode != 2))
        {
          Result = True;
          DAsmCode[0] = 0x408000 + MixErg + (AdrResult.Val << 8);
          CodeLen = 1;
        }
        else if (AdrResult.Type != ModNone)
        {
          Result = True;
          DAsmCode[0] = 0x40c000 + MixErg + (AdrResult.Mode << 8);
          DAsmCode[1] = AdrResult.Val;
          CodeLen = 1 + AdrResult.Cnt;
        }
      }
    }
    if (AdrResult.Seg != SegLData)
      return Result;
  }

  /* 3. Quelle ist Register */

  if (DecodeReg(LeftComp.str.p_str, &RegErg))
  {
    tAdrResult RightAdrResult;

    /* A und B gehen auch als L:..., in L-Zweig zwingen! */
    SegMask = MSegXData + MSegYData;
    if ((RegErg == 14) || (RegErg == 15))
      SegMask |= MSegLData;
    DecodeAdr(&RightComp, MModNoImm | MModDisp, SegMask, &RightAdrResult);
    if (RightAdrResult.Seg != SegLData)
    {
      IsY = Ord(RightAdrResult.Seg == SegYData);
      MixErg = ((RegErg & 0x18) << 17) + (IsY << 19) + ((RegErg & 7) << 16);
      if (RightAdrResult.Type == ModDisp)
      {
        if ((RightAdrResult.Val < 63) && (RightAdrResult.Val > -64) && (RegErg <= 15))
        {
          DAsmCode[0] = 0x020080 + ((RightAdrResult.Val & 1) << 6) + ((RightAdrResult.Val & 0x7e) << 10)
                      + (RightAdrResult.Mode << 8) + (IsY << 5) + RegErg;
          CodeLen = 1;
        }
        else
        {
          DAsmCode[0] = 0x0a7080 + (RightAdrResult.Mode << 8) + (IsY << 16) + RegErg;
          DAsmCode[1] = RightAdrResult.Val;
          CodeLen = 2;
        }
      }
      else if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val <= 63) && (RightAdrResult.Val >= 0) && (RightAdrResult.ShortMode != 2))
      {
        Result = True;
        DAsmCode[0] = 0x400000 + MixErg + (RightAdrResult.Val << 8);
        CodeLen = 1;
      }
      else if (RightAdrResult.Type != ModNone)
      {
        Result = True;
        DAsmCode[0] = 0x404000 + MixErg + (RightAdrResult.Mode << 8);
        DAsmCode[1] = RightAdrResult.Val;
        CodeLen = 1 + RightAdrResult.Cnt;
      }
      return Result;
    }
  }

  /* 4. Ziel ist langes Register */

  if (DecodeLReg(RightComp.str.p_str, &RegErg))
  {
    tAdrResult LeftAdrResult;

    DecodeAdr(&LeftComp, MModNoImm, MSegLData, &LeftAdrResult);
    MixErg = ((RegErg & 4) << 17) + ((RegErg & 3) << 16);
    if ((LeftAdrResult.Type == ModAbs) && (LeftAdrResult.Val <= 63) && (LeftAdrResult.Val >= 0) && (LeftAdrResult.ShortMode != 2))
    {
      Result = True;
      DAsmCode[0] = 0x408000 + MixErg + (LeftAdrResult.Val << 8);
      CodeLen = 1;
    }
    else
    {
      Result = True;
      DAsmCode[0] = 0x40c000 + MixErg + (LeftAdrResult.Mode << 8);
      DAsmCode[1] = LeftAdrResult.Val;
      CodeLen = 1 + LeftAdrResult.Cnt;
    }
    return Result;
  }

  /* 5. Quelle ist langes Register */

  if (DecodeLReg(LeftComp.str.p_str, &RegErg))
  {
    tAdrResult RightAdrResult;

    DecodeAdr(&RightComp, MModNoImm, MSegLData, &RightAdrResult);
    MixErg = ((RegErg & 4) << 17) + ((RegErg & 3) << 16);
    if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val <= 63) && (RightAdrResult.Val >= 0) && (RightAdrResult.ShortMode != 2))
    {
      Result = True;
      DAsmCode[0] = 0x400000 + MixErg + (RightAdrResult.Val << 8);
      CodeLen = 1;
    }
    else
    {
      Result = True;
      DAsmCode[0] = 0x404000 + MixErg + (RightAdrResult.Mode << 8);
      DAsmCode[1] = RightAdrResult.Val;
      CodeLen = 1 + RightAdrResult.Cnt;
    }
    return Result;
  }

  WrError(ErrNum_InvAddrMode);
  return Result;
}

static Boolean DecodeMOVE_2(int Start)
{
  LongInt RegErg, Reg1L, Reg1R, Reg2L, Reg2R, Dir1, Dir2;
  tAdrResult AdrResult1, AdrResult2;
  Boolean Result = False;

  SplitArg(&ArgStr[Start], &Left1Comp, &Right1Comp);
  SplitArg(&ArgStr[Start + 1], &Left2Comp, &Right2Comp);

  /* 1. Spezialfall X auf rechter Seite ? */

  if (!as_strcasecmp(Left2Comp.str.p_str, "X0"))
  {
    if (!DecodeALUReg(Right2Comp.str.p_str, &RegErg, False, False, True)) WrError(ErrNum_InvAddrMode);
    else if (strcmp(Left1Comp.str.p_str, Right2Comp.str.p_str)) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&Right1Comp, MModNoImm, MSegXData, &AdrResult1);
      if (AdrResult1.Type != ModNone)
      {
        CodeLen = 1 + AdrResult1.Cnt;
        DAsmCode[0] = 0x080000 + (RegErg << 16) + (AdrResult1.Mode << 8);
        DAsmCode[1] = AdrResult1.Val;
        Result = True;
      }
    }
    return Result;
  }

  /* 2. Spezialfall Y auf linker Seite ? */

  if (!as_strcasecmp(Left1Comp.str.p_str, "Y0"))
  {
    if (!DecodeALUReg(Right1Comp.str.p_str, &RegErg, False, False, True)) WrError(ErrNum_InvAddrMode);
    else if (strcmp(Left2Comp.str.p_str, Right1Comp.str.p_str)) WrError(ErrNum_InvAddrMode);
    else
    {
      DecodeAdr(&Right2Comp, MModNoImm, MSegYData, &AdrResult2);
      if (AdrResult2.Type != ModNone)
      {
        CodeLen = 1 + AdrResult2.Cnt;
        DAsmCode[0] = 0x088000 + (RegErg << 16) + (AdrResult2.Mode << 8);
        DAsmCode[1] = AdrResult2.Val;
        Result = True;
      }
    }
    return Result;
  }

  /* der Rest..... */

  if ((DecodeOpPair(&Left1Comp, &Right1Comp, SegXData, &Dir1, &Reg1L, &Reg1R, &AdrResult1))
   && (DecodeOpPair(&Left2Comp, &Right2Comp, SegYData, &Dir2, &Reg2L, &Reg2R, &AdrResult2)))
  {
    if ((Reg1R == -1) && (Reg2R == -1))
    {
      if ((AdrResult1.Mode >> 3 < 1) || (AdrResult1.Mode >> 3 > 4) || (AdrResult2.Mode >> 3 < 1) || (AdrResult2.Mode >> 3 > 4)) WrError(ErrNum_InvAddrMode);
      else if (((AdrResult1.Mode ^ AdrResult2.Mode) & 4) == 0) WrError(ErrNum_InvRegPair);
      else
      {
        DAsmCode[0] = 0x800000 + (Dir2 << 22) + (Dir1 << 15)
                    + (Reg1L << 18) + (Reg2L << 16) + ((AdrResult1.Mode & 0x1f) << 8)
                    + ((AdrResult2.Mode & 3) << 13) + ((AdrResult2.Mode & 24) << 17);
        CodeLen = 1;
        Result = True;
      }
    }
    else if (Reg1R == -1)
    {
      if ((Reg2L < 2) || (Reg2R > 1)) WrError(ErrNum_InvAddrMode);
      else
      {
        DAsmCode[0] = 0x100000 + (Reg1L << 18) + ((Reg2L - 2) << 17) + (Reg2R << 16)
                    + (Dir1 << 15) + (AdrResult1.Mode << 8);
        DAsmCode[1] = AdrResult1.Val;
        CodeLen = 1 + AdrResult1.Cnt;
        Result = True;
      }
    }
    else if (Reg2R == -1)
    {
      if ((Reg1L < 2) || (Reg1R > 1)) WrError(ErrNum_InvAddrMode);
      else
      {
        DAsmCode[0] = 0x104000 + (Reg2L << 16) + ((Reg1L - 2) << 19) + (Reg1R << 18)
                    + (Dir2 << 15) + (AdrResult2.Mode << 8);
        DAsmCode[1] = AdrResult2.Val;
        CodeLen = 1 + AdrResult2.Cnt;
        Result = True;
      }
    }
    else
      WrError(ErrNum_InvAddrMode);
    return Result;
  }

  WrError(ErrNum_InvAddrMode);
  return Result;
}

static Boolean DecodeMOVE(int Start)
{
  switch (ArgCnt - Start + 1)
  {
    case 0:
      return DecodeMOVE_0();
    case 1:
      return DecodeMOVE_1(Start);
    case 2:
      return DecodeMOVE_2(Start);
    default:
      (void)ChkArgCnt(0, 2);
      return False;
  }
}

static tErrorNum ErrCode;
static const tStrComp *pErrComp;

static void SetError(tErrorNum Code)
{
  ErrCode = Code; pErrComp = NULL;
}

static void SetXError(tErrorNum Code, const tStrComp *pNewErrComp)
{
  ErrCode = Code; pErrComp = pNewErrComp;
}

static void PrError(void)
{
  if (pErrComp) WrStrErrorPos(ErrCode, pErrComp);
  else if (ErrCode != 0) WrError(ErrCode);
}

/*----------------------------------------------------------------------------------------------*/

static void DecodeSFR(Word space)
{
  CodeEquate((as_addrspace_t)space, 0, MemLimit);
}

static void DecodeDS(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    Boolean OK;
    tSymbolFlags Flags;
    Word AdrWord = EvalStrIntExpressionWithFlags(&ArgStr[1], AdrInt, &OK, &Flags);

    if (mFirstPassUnknown(Flags)) WrError(ErrNum_FirstPassCalc);
    if (OK && !mFirstPassUnknown(Flags))
    {
      if (!AdrWord) WrError(ErrNum_NullResMem);
      CodeLen = AdrWord; DontPrint = True;
      BookKeeping();
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
    tStrComp *pArg;

    forallargs(pArg, OK)
    {
      EvalStrExpression(pArg, &t);
      switch (t.Typ)
      {
        case TempString:
          if (MultiCharToInt(&t, 3))
            goto ToInt;

          if (as_chartrans_xlate_nonz_dynstr(CurrTransTable->p_table, &t.Contents.str, pArg))
            OK = False;
          else
            OK = !string_2_dasm_code(&t.Contents.str, Packing ? 3 : 1, True);
          break;
        ToInt:
        case TempInt:
          if (mFirstPassUnknown(t.Flags)) t.Contents.Int &= 0xffffff;
          if (!(OK = RangeCheck(t.Contents.Int, Int24))) WrStrErrorPos(ErrNum_OverRange, pArg);
          else
            DAsmCode[CodeLen++] = t.Contents.Int & 0xffffff;
          break;
        case TempFloat:
          WrStrErrorPos(ErrNum_StringOrIntButFloat, pArg);
          /* fall-through */
        default:
          OK = False;
      }
    }
    if (!OK) CodeLen = 0;
  }
  as_tempres_free(&t);
}

/* ohne Argument */

static void DecodeFixed(Word Index)
{
  const FixedOrder *pOrder = FixedOrders + Index;

  if (!ChkArgCnt(0, 0));
  else if (ChkMinCPU(pOrder->MinCPU))
  {
    CodeLen = 1;
    DAsmCode[0] = pOrder->Code;
  }
}

/* ALU */

static void DecodePar(Word Index)
{
  const ParOrder *pOrder = ParOrders + Index;
  Boolean OK, DontAdd;
  tSymbolFlags Flags;
  tStrComp LeftRegComp;
  LargeInt LAddVal;
  LongInt AddVal, h = 0, Reg1, Reg2, Reg3;

  if (DecodeMOVE(2))
  {
    SetError((tErrorNum)0);
    DontAdd = False;
    switch (pOrder->Typ)
    {
      case ParAB:
        if (!DecodeALUReg(ArgStr[1].str.p_str, &Reg1, False, False, True)) SetXError(ErrNum_InvReg, &ArgStr[1]);
        else
          h = Reg1 << 3;
        break;
      case ParFixAB:
        if (as_strcasecmp(ArgStr[1].str.p_str, "A,B")) SetError(ErrNum_InvRegPair);
        else
          h = 0;
        break;
      case ParABShl1:
        if (!strchr(ArgStr[1].str.p_str, ','))
        {
          if (!DecodeALUReg(ArgStr[1].str.p_str, &Reg1, False, False, True)) SetXError(ErrNum_InvReg, &ArgStr[1]);
          else
            h = Reg1 << 3;
        }
        else if (ArgCnt != 1) SetError(ErrNum_ParNotPossible);
        else if (MomCPU < CPU56300) SetError(ErrNum_InstructionNotSupported);
        else
        {
          SplitArg(&ArgStr[1], &LeftComp, &RightComp);
          if (!strchr(RightComp.str.p_str, ','))
            StrCompCopy(&MidComp, &RightComp);
          else
            SplitArg(&RightComp, &MidComp, &RightComp);
          if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
          else if (!DecodeALUReg(MidComp.str.p_str, &Reg2, False, False, True)) SetXError(ErrNum_InvReg, &MidComp);
          else if (*LeftComp.str.p_str == '#')
          {
            AddVal = EvalStrIntExpressionOffs(&LeftComp, 1, UInt6, &OK);
            if (OK)
            {
              DAsmCode[0] = 0x0c1c00 + ((pOrder->Code & 0x10) << 4) + (Reg2 << 7)
                          + (AddVal << 1) + Reg1;
              CodeLen = 1;
              DontAdd = True;
            }
          }
          else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg3)) SetXError(ErrNum_InvReg, &LeftComp);
          else
          {
            DAsmCode[0] = 0x0c1e60 - ((pOrder->Code & 0x10) << 2) + (Reg2 << 4)
                        + (Reg3 << 1) + Reg1;
            CodeLen = 1;
            DontAdd = True;
          }
        }
        break;
      case ParABShl2:
        if (!strchr(ArgStr[1].str.p_str, ','))
        {
          if (!DecodeALUReg(ArgStr[1].str.p_str, &Reg1, False, False, True)) SetXError(ErrNum_InvReg, &ArgStr[1]);
          else
            h = Reg1 << 3;
        }
        else if (ArgCnt != 1) SetError(ErrNum_ParNotPossible);
        else if (MomCPU < CPU56300) SetError(ErrNum_InstructionNotSupported);
        else
        {
          SplitArg(&ArgStr[1], &LeftComp, &RightComp);
          if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
          else if (*LeftComp.str.p_str == '#')
          {
            AddVal = EvalStrIntExpressionOffs(&LeftComp, 1, UInt5, &OK);
            if (OK)
            {
              DAsmCode[0] = 0x0c1e80 + ((0x33 - pOrder->Code) << 2)
                          + (AddVal << 1) + Reg1;
              CodeLen = 1;
              DontAdd = True;
            }
          }
          else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg3)) SetXError(ErrNum_InvReg, &LeftComp);
          else
          {
            DAsmCode[0] = 0x0c1e10 + ((0x33 - pOrder->Code) << 1)
                        + (Reg3 << 1) + Reg1;
            CodeLen = 1;
            DontAdd = True;
          }
        }
        break;
      case ParXYAB:
        SplitArg(&ArgStr[1], &LeftComp, &RightComp);
        if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
        else if (!DecodeLReg(LeftComp.str.p_str, &Reg1)) SetXError(ErrNum_InvReg, &LeftComp);
        else if ((Reg1 < 2) || (Reg1 > 3)) SetXError(ErrNum_InvReg, &LeftComp);
        else
           h = (Reg2 << 3) + ((Reg1 - 2) << 4);
        break;
      case ParABXYnAB:
        SplitArg(&ArgStr[1], &LeftComp, &RightComp);
        if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
        else if (*LeftComp.str.p_str == '#')
        {
          if (Memo("CMPM")) SetError(ErrNum_InvAddrMode);
          else if (MomCPU < CPU56300) SetError(ErrNum_InstructionNotSupported);
          else if (ArgCnt != 1) SetError(ErrNum_ParNotPossible);
          else
          {
            AddVal = EvalStrIntExpressionOffs(&LeftComp, 1, Int24, &OK);
            if (!OK) SetError((tErrorNum)-1);
            else if ((AddVal >= 0) && (AddVal <= 63))
            {
              DAsmCode[0] = 0x014000 + (AddVal << 8);
              h = 0x80 + (Reg2 << 3);
            }
            else
            {
              DAsmCode[0] = 0x014000; h = 0xc0 + (Reg2 << 3);
              DAsmCode[1] = AddVal & 0xffffff; CodeLen = 2;
            }
          }
        }
        else
        {
          if (!DecodeXYABReg(LeftComp.str.p_str, &Reg1)) SetXError(ErrNum_InvReg, &LeftComp);
          else if ((Reg1 ^ Reg2) == 1) SetError(ErrNum_InvRegPair);
          else if ((Memo("CMPM")) && ((Reg1 &6) == 2)) SetXError(ErrNum_InvReg, &LeftComp);
          else
          {
            if (Reg1 < 2)
              Reg1 = Ord(!Memo("CMPM"));
            h = (Reg2 << 3) + (Reg1 << 4);
          }
        }
        break;
      case ParABBA:
        if  (!as_strcasecmp(ArgStr[1].str.p_str, "B,A"))
          h = 0;
        else if (!as_strcasecmp(ArgStr[1].str.p_str, "A,B"))
          h = 8;
        else
          SetXError(ErrNum_InvRegPair, &ArgStr[1]);
        break;
      case ParXYnAB:
        SplitArg(&ArgStr[1], &LeftComp, &RightComp);
        if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
        else if (*LeftComp.str.p_str == '#')
        {
          if (MomCPU < CPU56300) SetError(ErrNum_InstructionNotSupported);
          else if (ArgCnt != 1) SetError(ErrNum_ParNotPossible);
          else
          {
            AddVal = EvalStrIntExpressionOffs(&LeftComp, 1, Int24, &OK);
            if (!OK) SetError((tErrorNum)-1);
            else if ((AddVal >= 0) && (AddVal <= 63))
            {
              DAsmCode[0] = 0x014080 + (AddVal << 8) + (Reg2 << 3) + (pOrder->Code & 7);
              CodeLen = 1;
              DontAdd = True;
            }
            else
            {
              DAsmCode[0] = 0x0140c0 + (Reg2 << 3) + (pOrder->Code & 7);
              DAsmCode[1] = AddVal & 0xffffff;
              CodeLen = 2;
              DontAdd = True;
            }
          }
        }
        else
        {
          if (!DecodeReg(LeftComp.str.p_str, &Reg1)) SetXError(ErrNum_InvReg, &LeftComp);
          else if ((Reg1 < 4) || (Reg1 > 7)) SetXError(ErrNum_InvReg, &LeftComp);
          else
            h = (Reg2 << 3) + (TurnXY(Reg1) << 4);
        }
        break;
      case ParMul:
        SplitArg(&ArgStr[1], &LeftComp, &MidComp);
        SplitArg(&MidComp, &MidComp, &RightComp);
        h = 0;
        if (*LeftComp.str.p_str == '-')
        {
          StrCompRefRight(&LeftRegComp, &LeftComp, 1);
          h += 4;
        }
        else if (*LeftComp.str.p_str == '+')
          StrCompRefRight(&LeftRegComp, &LeftComp, 1);
        else
          StrCompRefRight(&LeftRegComp, &LeftComp, 0);
        if (!DecodeALUReg(RightComp.str.p_str, &Reg3, False, False, True)) SetXError(ErrNum_InvReg, &RightComp);
        else if (!DecodeReg(LeftRegComp.str.p_str, &Reg1)) SetXError(ErrNum_InvReg, &LeftRegComp);
        else if ((Reg1 < 4) || (Reg1 > 7)) SetXError(ErrNum_InvReg, &LeftRegComp);
        else if (*MidComp.str.p_str == '#')
        {
          if (!ChkArgCnt(1, 1));
          else if (ChkMinCPU(CPU56300))
          {
            AddVal = EvalStrIntExpressionOffsWithFlags(&MidComp, 1, UInt24, &OK, &Flags);
            if (mFirstPassUnknown(Flags))
              AddVal = 1;
            if ((!(SingleBit(AddVal, &LAddVal))) || (LAddVal > 22)) WrError(ErrNum_NotOneBit);
            else
            {
              LAddVal = 23 - LAddVal;
              DAsmCode[0] = 0x010040 + (LAddVal << 8) + (Mac2Table[Reg1 & 3] << 4)
                          + (Reg3 << 3);
              CodeLen = 1;
            }
          }
        }

        else if (!DecodeReg(MidComp.str.p_str, &Reg2)) SetXError(ErrNum_InvReg, &MidComp);
        else if ((Reg2 < 4) || (Reg2 > 7)) SetXError(ErrNum_InvReg, &MidComp);
        else if (MacTable[Reg1 - 4][Reg2 - 4] == 0xff) SetError(ErrNum_InvRegPair);
        else
          h += (Reg3 << 3) + (MacTable[Reg1 - 4][Reg2 - 4] << 4);
        break;
    }
    if (ErrCode == 0)
    {
      if (!DontAdd)
        DAsmCode[0] += pOrder->Code + h;
    }
    else
    {
      if (ErrCode > 0)
        PrError();
      CodeLen = 0;
    }
  }
}

static void DecodeDIV(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    LongInt Reg2, Reg1;

    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) WrError(ErrNum_InvAddrMode);
    else if (!DecodeReg(LeftComp.str.p_str, &Reg1)) WrError(ErrNum_InvAddrMode);
    else if ((Reg1 < 4) || (Reg1 > 7)) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 1;
      DAsmCode[0] = 0x018040 + (Reg2 << 3) + (TurnXY(Reg1) << 4);
    }
  }
}

static void DecodeImmMac(Word Code)
{
  tStrComp LeftArg;
  Boolean OK;
  LongInt h = 0, Reg1, Reg2;

  if (!ChkArgCnt(1, 1));
  else if (ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    h = 0;
    StrCompRefRight(&LeftArg, &LeftComp, 0);
    switch (*LeftComp.str.p_str)
    {
      case '-':
        h = 4;
        /* fall-through */
      case '+':
        StrCompRefRight(&LeftArg, &LeftComp, 1);
    }
    if ((*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYABReg(MidComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if ((Reg2 < 4) || (Reg2 > 7)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (*LeftArg.str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      DAsmCode[1] = EvalStrIntExpressionOffs(&LeftArg, 1, Int24, &OK);
      if (OK)
      {
        DAsmCode[0] = 0x0141c0 + Code + h + (Reg1 << 3) + ((Reg2 & 3) << 4);
        CodeLen = 2;
      }
    }
  }
}

static void DecodeDMAC(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (ChkMinCPU(CPU56300))
  {
    LongInt Reg1, Reg2, Reg3;
    tStrComp LeftReg;

    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    if (*LeftComp.str.p_str == '-')
    {
      StrCompRefRight(&LeftReg, &LeftComp, 1);
      Code += 16;
    }
    else if (*LeftComp.str.p_str == '+')
      StrCompRefRight(&LeftReg, &LeftComp, 1);
    else
      StrCompRefRight(&LeftReg, &LeftComp, 0);
    if ((*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYAB1Reg(MidComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (Reg2 < 4) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (!DecodeXYAB1Reg(LeftReg.str.p_str, &Reg3)) WrStrErrorPos(ErrNum_InvReg, &LeftReg);
    else if (Reg3 < 4) WrStrErrorPos(ErrNum_InvReg, &LeftReg);
    else
    {
      DAsmCode[0] = 0x012480 + Code + (Reg1 << 5) + Mac4Table[Reg3 - 4][Reg2 - 4];
      CodeLen = 1;
    }
  }
}

static void DecodeMAC_MPY(Word Code)
{
  if (!ChkArgCnt(1, 1));
  else if (ChkMinCPU(CPU56300))
  {
    tStrComp LeftReg;
    LongInt Reg1, Reg2, Reg3;

    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    if (*LeftComp.str.p_str == '-')
    {
      StrCompRefRight(&LeftReg, &LeftComp, 1);
      Code += 16;
    }
    else if (*LeftComp.str.p_str == '+')
      StrCompRefRight(&LeftReg, &LeftComp, 1);
    else
      StrCompRefRight(&LeftReg, &LeftComp, 0);
    if ((*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYAB1Reg(MidComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (Reg2 < 4) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (!DecodeXYAB1Reg(LeftReg.str.p_str, &Reg3)) WrStrErrorPos(ErrNum_InvReg, &LeftReg);
    else if (Reg3 < 4) WrStrErrorPos(ErrNum_InvReg, &LeftReg);
    else
    {
      DAsmCode[0] = 0x012680 + Code + (Reg1 << 5) + Mac4Table[Reg3 - 4][Reg2 - 4];
      CodeLen = 1;
    }
  }
}

static void DecodeINC_DEC(Word Code)
{
  LongInt Reg1;

  if (!ChkArgCnt(1, 1));
  else if (!ChkMinCPU(CPU56002));
  else if (!DecodeALUReg(ArgStr[1].str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
  else
  {
    DAsmCode[0] = (LongWord)Code + Reg1;
    CodeLen = 1;
  }
}

static void DecodeANDI_ORI(Word Code)
{
  LongInt Reg1, h = 0;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeControlReg(RightComp.str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvAddrMode, &RightComp);
    else if (*LeftComp.str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      h = EvalStrIntExpressionOffs(&LeftComp, 1, Int8, &OK);
      if (OK)
      {
        CodeLen = 1;
        DAsmCode[0] = (LongWord)Code + ((h & 0xff) << 8) + Reg1;
      }
    }
  }
}

static void DecodeNORM(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) WrError(ErrNum_InvAddrMode);
    else if (!DecodeReg(LeftComp.str.p_str, &Reg1)) WrError(ErrNum_InvAddrMode);
    else if ((Reg1 < 16) || (Reg1 > 23)) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 1;
      DAsmCode[0] = 0x01d815 + ((Reg1 & 7) << 8) + (Reg2 << 3);
    }
  }
}

static void DecodeNORMF(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else
    {
      CodeLen = 1;
      DAsmCode[0] = 0x0c1e20 + Reg2 + (Reg1 << 1);
    }
  }
}

static void DecodeBit(Word Code)
{
  LongInt Reg1, Reg2, Reg3, h = 0;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    Reg2 = ((Code & 1) << 5) + (((LongInt) Code >> 1) << 16);
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (*LeftComp.str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      h = EvalStrIntExpressionOffsWithFlags(&LeftComp, 1, Int8, &OK, &Flags);
      if (mFirstPassUnknown(Flags)) h &= 15;
      if (OK)
      {
        if ((h < 0) || (h > 23)) WrError(ErrNum_OverRange);
        else if (DecodeGeneralReg(RightComp.str.p_str, &Reg1))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x0ac040 + h + (Reg1 << 8) + Reg2;
        }
        else
        {
          tAdrResult RightAdrResult;

          DecodeAdr(&RightComp, MModNoImm, MSegXData + MSegYData, &RightAdrResult);
          Reg3 = Ord(RightAdrResult.Seg == SegYData) << 6;
          if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val <= 63) && (RightAdrResult.Val >= 0) && (RightAdrResult.ShortMode != 2))
          {
            CodeLen = 1;
            DAsmCode[0] = 0x0a0000 + h + (RightAdrResult.Val << 8) + Reg3 + Reg2;
          }
          else if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val >= MemLimit - 0x3f) && (RightAdrResult.Val <= MemLimit) && (RightAdrResult.ShortMode != 2))
          {
            CodeLen = 1;
            DAsmCode[0] = 0x0a8000 + h + ((RightAdrResult.Val & 0x3f) << 8) + Reg3 + Reg2;
          }
          else if ((RightAdrResult.Type == ModAbs) && (MomCPU >= CPU56300) && (RightAdrResult.Val >= MemLimit - 0x7f) && (RightAdrResult.Val <= MemLimit - 0x40) && (RightAdrResult.ShortMode != 2))
          {
            Reg2 = ((Code & 1) << 5) + (((LongInt) Code >> 1) << 14);
            CodeLen = 1;
            DAsmCode[0] = 0x010000 + h + ((RightAdrResult.Val & 0x3f) << 8) + Reg3 + Reg2;
          }
          else if (RightAdrResult.Type != ModNone)
          {
            CodeLen = 1 + RightAdrResult.Cnt;
            DAsmCode[0] = 0x0a4000 + h + (RightAdrResult.Mode << 8) + Reg3 + Reg2;
            DAsmCode[1] = RightAdrResult.Val;
          }
        }
      }
    }
  }
}

static void DecodeEXTRACT_EXTRACTU(Word Code)
{
  LongInt Reg1, Reg2, Reg3;
  Boolean OK;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    if ((*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeALUReg(MidComp.str.p_str, &Reg2, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (*LeftComp.str.p_str == '#')
    {
      DAsmCode[1] = EvalStrIntExpressionOffs(&LeftComp, 1, Int24, &OK);
      if (OK)
      {
        DAsmCode[0] = 0x0c1800 + Code + Reg1 + (Reg2 << 4);
        CodeLen = 2;
      }
    }
    else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg3)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else
    {
      DAsmCode[0] = 0x0c1a00 + Code + Reg1 + (Reg2 << 4) + (Reg3 << 1);
      CodeLen = 1;
    }
  }
}

static void DecodeINSERT(Word Code)
{
  LongInt Reg1, Reg2, Reg3;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    if ((*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYAB0Reg(MidComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &MidComp);
    else if (*LeftComp.str.p_str == '#')
    {
      DAsmCode[1] = EvalStrIntExpressionOffs(&LeftComp, 1, Int24, &OK);
      if (OK)
      {
        DAsmCode[0] = 0x0c1900 + Reg1 + (Reg2 << 4);
        CodeLen = 2;
      }
    }
    else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg3)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else
    {
      DAsmCode[0] = 0x0c1b00 + Reg1 + (Reg2 << 4) + (Reg3 << 1);
      CodeLen = 1;
    }
  }
}

static void DecodeMERGE(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYAB1Reg(LeftComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else
    {
      DAsmCode[0] = 0x0c1b80 + Reg1 + (Reg2 << 1);
      CodeLen = 1;
    }
  }
}

static void DecodeCLB(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(LeftComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg2, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else
    {
      DAsmCode[0] = 0x0c1e00 + Reg2 + (Reg1 << 1);
      CodeLen = 1;
    }
  }
}

static void DecodeCMPU(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (!DecodeALUReg(RightComp.str.p_str, &Reg1, False, False, True)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (!DecodeXYABReg(LeftComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else if ((Reg1 ^ Reg2) == 1) WrError(ErrNum_InvRegPair);
    else if ((Reg2 & 6) == 2) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
    else
    {
      if (Reg2 < 2)
        Reg2 = 0;
      DAsmCode[0] = 0x0c1ff0 + (Reg2 << 1) + Reg1;
      CodeLen = 1;
    }
  }
}

/* Datentransfer */

static void DecodePlainMOVE(Word Code)
{
  UNUSED(Code);

  DecodeMOVE(1);
}

static void DecodeMOVEC(Word Code)
{
  LongInt Reg1, Reg2, Reg3;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (DecodeCtrlReg(LeftComp.str.p_str, &Reg1))
    {
      if (DecodeGeneralReg(RightComp.str.p_str, &Reg2))
      {
        DAsmCode[0] = 0x0440a0 + (Reg2 << 8) + Reg1;
        CodeLen = 1;
      }
      else
      {
        tAdrResult RightAdrResult;

        DecodeAdr(&RightComp, MModNoImm, MSegXData + MSegYData, &RightAdrResult);
        Reg3 = (Ord(RightAdrResult.Seg == SegYData)) << 6;
        if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val <= 63) && (RightAdrResult.ShortMode != 2))
        {
          DAsmCode[0] = 0x050020 + (RightAdrResult.Val << 8) + Reg3 + Reg1;
          CodeLen = 1;
        }
        else
        {
          DAsmCode[0] = 0x054020 + (RightAdrResult.Mode << 8) + Reg3 + Reg1;
          DAsmCode[1] = RightAdrResult.Val; CodeLen = 1 + RightAdrResult.Cnt;
        }
      }
    }
    else if (!DecodeCtrlReg(RightComp.str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvCtrlReg, &RightComp);
    else
    {
      if (DecodeGeneralReg(LeftComp.str.p_str, &Reg2))
      {
        DAsmCode[0] = 0x04c0a0 + (Reg2 << 8) + Reg1;
        CodeLen = 1;
      }
      else
      {
        tAdrResult LeftAdrResult;

        DecodeAdr(&LeftComp, MModAll, MSegXData + MSegYData, &LeftAdrResult);
        Reg3 = (Ord(LeftAdrResult.Seg == SegYData)) << 6;
        if ((LeftAdrResult.Type == ModAbs) && (LeftAdrResult.Val <= 63) && (LeftAdrResult.ShortMode != 2))
        {
          DAsmCode[0] = 0x058020 + (LeftAdrResult.Val << 8) + Reg3 + Reg1;
          CodeLen = 1;
        }
        else if (!LeftAdrResult.ForceImmLong && (LeftAdrResult.Type == ModImm) && (LeftAdrResult.Val <= 255))
        {
          DAsmCode[0] = 0x0500a0 + (LeftAdrResult.Val << 8) + Reg1;
          CodeLen = 1;
        }
        else
        {
          DAsmCode[0] = 0x05c020 + (LeftAdrResult.Mode << 8) + Reg3 + Reg1;
          DAsmCode[1] = LeftAdrResult.Val; CodeLen = 1 + LeftAdrResult.Cnt;
        }
      }
    }
  }
}

static void DecodeMOVEM(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (DecodeGeneralReg(LeftComp.str.p_str, &Reg1))
    {
      tAdrResult RightAdrResult;

      DecodeAdr(&RightComp, MModNoImm, MSegCode, &RightAdrResult);
      if ((RightAdrResult.Type == ModAbs) && (RightAdrResult.Val >= 0) && (RightAdrResult.Val <= 63) && (RightAdrResult.ShortMode != 2))
      {
        CodeLen = 1;
        DAsmCode[0] = 0x070000 + Reg1 + (RightAdrResult.Val << 8);
      }
      else if (RightAdrResult.Type != ModNone)
      {
        CodeLen = 1 + RightAdrResult.Cnt;
        DAsmCode[1] = RightAdrResult.Val;
        DAsmCode[0] = 0x074080 + Reg1 + (RightAdrResult.Mode << 8);
      }
    }
    else if (!DecodeGeneralReg(RightComp.str.p_str, &Reg2)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else
    {
      tAdrResult LeftAdrResult;

      DecodeAdr(&LeftComp, MModNoImm, MSegCode, &LeftAdrResult);
      if ((LeftAdrResult.Type == ModAbs) && (LeftAdrResult.Val >= 0) && (LeftAdrResult.Val <= 63) && (LeftAdrResult.ShortMode != 2))
      {
        CodeLen = 1;
        DAsmCode[0] = 0x078000 + Reg2 + (LeftAdrResult.Val << 8);
      }
      else if (LeftAdrResult.Type != ModNone)
      {
        CodeLen = 1 + LeftAdrResult.Cnt;
        DAsmCode[1] = LeftAdrResult.Val;
        DAsmCode[0] = 0x07c080 + Reg2 + (LeftAdrResult.Mode << 8);
      }
    }
  }
}

static void DecodeMOVEP(Word Code)
{
  LongInt Reg1, Reg2;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (DecodeGeneralReg(LeftComp.str.p_str, &Reg1))
    {
      tAdrResult RightAdrResult;

      DecodeAdr(&RightComp, MModAbs, MSegXData + MSegYData, &RightAdrResult);
      if (RightAdrResult.Type != ModNone)
      {
        if ((RightAdrResult.Val <= MemLimit) && (RightAdrResult.Val >= MemLimit - 0x3f))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x08c000 + (Ord(RightAdrResult.Seg == SegYData) << 16)
                      + (RightAdrResult.Val & 0x3f) + (Reg1 << 8);
        }
        else if ((MomCPU >= CPU56300) && (RightAdrResult.Val <= MemLimit - 0x40) && (RightAdrResult.Val >= MemLimit - 0x7f))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x04c000 + (Ord(RightAdrResult.Seg == SegYData) << 5)
                      + (Ord(RightAdrResult.Seg == SegXData) << 7)
                      + (RightAdrResult.Val & 0x1f) + ((RightAdrResult.Val & 0x20) << 1) + (Reg1 << 8);
        }
        else
          WrError(ErrNum_UnderRange);
      }
    }
    else if (DecodeGeneralReg(RightComp.str.p_str, &Reg2))
    {
      tAdrResult LeftAdrResult;

      DecodeAdr(&LeftComp, MModAbs, MSegXData + MSegYData, &LeftAdrResult);
      if (LeftAdrResult.Type != ModNone)
      {
        if ((LeftAdrResult.Val <= MemLimit) && (LeftAdrResult.Val >= MemLimit - 0x3f))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x084000 + (Ord(LeftAdrResult.Seg == SegYData) << 16)
                      + (LeftAdrResult.Val & 0x3f) + (Reg2 << 8);
        }
        else if ((MomCPU >= CPU56300) && (LeftAdrResult.Val <= MemLimit - 0x40) && (LeftAdrResult.Val >= MemLimit - 0x7f))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x044000 + (Ord(LeftAdrResult.Seg == SegYData) << 5)
                      + (Ord(LeftAdrResult.Seg == SegXData) << 7)
                      + (LeftAdrResult.Val & 0x1f) + ((LeftAdrResult.Val & 0x20) << 1) + (Reg2 << 8);
        }
        else
          WrError(ErrNum_UnderRange);
      }
    }
    else
    {
      tAdrResult LeftAdrResult;

      DecodeAdr(&LeftComp, MModAll, MSegXData + MSegYData + MSegCode, &LeftAdrResult);
      if ((LeftAdrResult.Type == ModAbs) && (LeftAdrResult.Seg != SegCode) && (LeftAdrResult.Val >= MemLimit - 0x3f) && (LeftAdrResult.Val <= MemLimit))
      {
        LongInt HVal = LeftAdrResult.Val & 0x3f, HSeg = LeftAdrResult.Seg;
        tAdrResult RightAdrResult;

        DecodeAdr(&RightComp, MModNoImm, MSegXData + MSegYData + MSegCode, &RightAdrResult);
        if (RightAdrResult.Type != ModNone)
        {
          if (RightAdrResult.Seg == SegCode)
          {
            CodeLen = 1 + RightAdrResult.Cnt;
            DAsmCode[1] = RightAdrResult.Val;
            DAsmCode[0] = 0x084040 + HVal + (RightAdrResult.Mode << 8)
                        + (Ord(HSeg == SegYData) << 16);
          }
          else
          {
            CodeLen = 1 + RightAdrResult.Cnt;
            DAsmCode[1] = RightAdrResult.Val;
            DAsmCode[0] = 0x084080 + HVal + (RightAdrResult.Mode << 8)
                        + (Ord(HSeg == SegYData) << 16)
                        + (Ord(RightAdrResult.Seg == SegYData) << 6);
          }
        }
      }
      else if ((LeftAdrResult.Type == ModAbs) && (MomCPU >= CPU56300) && (LeftAdrResult.Seg != SegCode) && (LeftAdrResult.Val >= MemLimit - 0x7f) && (LeftAdrResult.Val <= MemLimit - 0x40) && (LeftAdrResult.ShortMode != 2))
      {
        LongInt HVal = LeftAdrResult.Val & 0x3f, HSeg = LeftAdrResult.Seg;
        tAdrResult RightAdrResult;

        DecodeAdr(&RightComp, MModNoImm, MSegXData + MSegYData + MSegCode, &RightAdrResult);
        if (RightAdrResult.Type != ModNone)
        {
          if (RightAdrResult.Seg == SegCode)
          {
            CodeLen = 1 + RightAdrResult.Cnt;
            DAsmCode[1] = RightAdrResult.Val;
            DAsmCode[0] = 0x008000 + HVal + (RightAdrResult.Mode << 8)
                        + (Ord(HSeg == SegYData) << 6);
          }
          else
          {
            CodeLen = 1 + RightAdrResult.Cnt;
            DAsmCode[1] = RightAdrResult.Val;
            DAsmCode[0] = 0x070000 + HVal + (RightAdrResult.Mode << 8)
                        + (Ord(HSeg == SegYData) << 7)
                        + (Ord(HSeg == SegXData) << 14)
                        + (Ord(RightAdrResult.Seg == SegYData) << 6);
          }
        }
      }
      else if (LeftAdrResult.Type != ModNone)
      {
        LongInt HVal = LeftAdrResult.Val,
                HCnt = LeftAdrResult.Cnt,
                HMode = LeftAdrResult.Mode,
                HSeg = LeftAdrResult.Seg;
        tAdrResult RightAdrResult;

        DecodeAdr(&RightComp, MModAbs, MSegXData + MSegYData, &RightAdrResult);
        if (RightAdrResult.Type != ModNone)
        {
          if ((RightAdrResult.Val >= MemLimit - 0x3f) && (RightAdrResult.Val <= MemLimit))
          {
            if (HSeg == SegCode)
            {
              CodeLen = 1 + HCnt;
              DAsmCode[1] = HVal;
              DAsmCode[0] = 0x08c040 + (RightAdrResult.Val & 0x3f) + (HMode << 8)
                          + (Ord(RightAdrResult.Seg == SegYData) << 16);
            }
            else
            {
              CodeLen = 1 + HCnt;
              DAsmCode[1] = HVal;
              DAsmCode[0] = 0x08c080 + (((Word)RightAdrResult.Val) & 0x3f) + (HMode << 8)
                          + (Ord(RightAdrResult.Seg == SegYData) << 16)
                          + (Ord(HSeg == SegYData) << 6);
            }
          }
          else if ((MomCPU >= CPU56300) && (RightAdrResult.Val >= MemLimit - 0x7f) && (RightAdrResult.Val <= MemLimit - 0x40))
          {
            if (HSeg == SegCode)
            {
              CodeLen = 1 + HCnt;
              DAsmCode[1] = HVal;
              DAsmCode[0] = 0x00c000 + (RightAdrResult.Val & 0x3f) + (HMode << 8)
                          + (Ord(RightAdrResult.Seg == SegYData) << 6);
            }
            else
            {
              CodeLen = 1 + HCnt;
              DAsmCode[1] = HVal;
              DAsmCode[0] = 0x078000 + (((Word)RightAdrResult.Val) & 0x3f) + (HMode << 8)
                          + (Ord(RightAdrResult.Seg == SegYData) << 7)
                          + (Ord(RightAdrResult.Seg == SegXData) << 14)
                          + (Ord(HSeg == SegYData) << 6);
            }
          }
          else
            WrError(ErrNum_UnderRange);
        }
      }
    }
  }
}

static void DecodePlainTFR(Word Code)
{
  LongInt Reg1;

  UNUSED(Code);

  if (ChkArgCnt(1, ArgCntMax))
  if (DecodeMOVE(2))
  {
    if (DecodeTFR(&ArgStr[1], &Reg1))
    {
      DAsmCode[0] += 0x01 + (Reg1 << 3);
    }
    else
    {
      WrError(ErrNum_InvAddrMode);
      CodeLen = 0;
    }
  }
}

static void DecodeTcc(Word Condition)
{
  LongInt Reg1, Reg2;

  if (!ChkArgCnt(1, 2));
  else if (DecodeTFR(&ArgStr[1], &Reg1))
  {
    if (ArgCnt == 1)
    {
      CodeLen = 1;
      DAsmCode[0] = 0x020000 + (Condition << 12) + (Reg1 << 3);
    }
    else if (!DecodeRR(&ArgStr[2], &Reg2)) WrError(ErrNum_InvAddrMode);
    else
    {
      CodeLen = 1;
      DAsmCode[0] = 0x030000 + (Condition << 12) + (Reg1 << 3) + Reg2;
    }
  }
  else if (!ChkArgCnt(1, 1));
  else if (!DecodeRR(&ArgStr[1], &Reg1)) WrError(ErrNum_InvAddrMode);
  else
  {
    DAsmCode[0] = 0x020800 + (Condition << 12) + Reg1;
    CodeLen = 1;
  }
}

static void DecodeBitBr(Word Code)
{
  LongInt Reg1, Reg3, h = 0, h2, AddVal;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    h = (Code & 1) << 5;
    h2 = (((LongInt) Code) & 2) << 15;
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    SplitArg(&RightComp, &MidComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0') || (*MidComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (*LeftComp.str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      AddVal = EvalStrIntExpressionOffsWithFlags(&LeftComp, 1, Int8, &OK, &Flags);
      if (mFirstPassUnknown(Flags)) AddVal &= 15;
      if (OK)
      {
        if ((AddVal < 0) || (AddVal > 23)) WrError(ErrNum_OverRange);
        else if (DecodeGeneralReg(MidComp.str.p_str, &Reg1))
        {
          CodeLen = 1;
          DAsmCode[0] = 0x0cc080 + AddVal + (Reg1 << 8) + h + h2;
        }
        else
        {
          tAdrResult AdrResult;

          DecodeAdr(&MidComp, MModNoImm, MSegXData + MSegYData, &AdrResult);
          Reg3 = Ord(AdrResult.Seg == SegYData) << 6;
          if ((AdrResult.Type == ModAbs) && (mFirstPassUnknown(AdrResult.AbsSymFlags))) AdrResult.Val &= 0x3f;
          if ((AdrResult.Type == ModAbs) && (AdrResult.Val <= 63) && (AdrResult.Val >= 0) && (AdrResult.ShortMode != 2))
          {
            CodeLen = 1;
            DAsmCode[0] = 0x0c8080 + AddVal + (AdrResult.Val << 8) + Reg3 + h + h2;
          }
          else if ((AdrResult.Type == ModAbs) && (AdrResult.Val >= MemLimit - 0x3f) && (AdrResult.Val <= MemLimit))
          {
            CodeLen = 1;
            DAsmCode[0] = 0x0cc000 + AddVal + ((AdrResult.Val & 0x3f) << 8) + Reg3 + h + h2;
          }
          else if ((AdrResult.Type == ModAbs) && (AdrResult.Val >= MemLimit - 0x7f) && (AdrResult.Val <= MemLimit - 0x40))
          {
            CodeLen = 1;
            DAsmCode[0] = 0x048000 + AddVal + ((AdrResult.Val & 0x3f) << 8) + Reg3 + h + (h2 >> 9);
          }
          else if (AdrResult.Type == ModAbs) WrError(ErrNum_InvAddrMode);
          else if (AdrResult.Type != ModNone)
          {
            CodeLen = 1;
            DAsmCode[0] = 0x0c8000 + AddVal + (AdrResult.Mode << 8) + Reg3 + h + h2;
          }
        }
      }
    }
    if (CodeLen == 1)
    {
      LongInt Dist = EvalStrIntExpression(&RightComp, AdrInt, &OK) - EProgCounter();

      if (OK)
      {
        DAsmCode[1] = Dist & 0xffffff;
        CodeLen = 2;
      }
      else
        CodeLen = 0;
    }
  }
}

static void DecodeBRA_BSR(Word Code)
{
  LongInt Reg1, Dist;
  Byte Size;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300)
   && DecodeReg(ArgStr[1].str.p_str, &Reg1))
  {
    if ((Reg1 < 16) || (Reg1 > 23)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
    {
      Reg1 -= 16;
      DAsmCode[0] = 0x0d1880 + (Reg1 << 8) + Code;
      CodeLen = 1;
    }
  }
  else
  {
    unsigned Offset = CutSize(&ArgStr[1], &Size);

    Dist = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], Offset, AdrInt, &OK, &Flags) - EProgCounter();
    if (Size == 0)
      Size = ((Dist> - 256) && (Dist < 255)) ? 1 : 2;
    switch (Size)
    {
      case 1:
        if (!mSymbolQuestionable(Flags) && ((Dist < -256) || (Dist > 255))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          Dist &= 0x1ff;
          DAsmCode[0] = 0x050800 + (Code << 4) + ((Dist & 0x1e0) << 1) + (Dist & 0x1f);
          CodeLen = 1;
        }
        break;
      case 2:
        DAsmCode[0] = 0x0d1080 + Code;
        DAsmCode[1] = Dist & 0xffffff;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeBcc(Word Condition)
{
  LongInt Dist, Reg1;
  Boolean OK;
  tSymbolFlags Flags;
  Byte Size;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300)
   && DecodeReg(ArgStr[1].str.p_str, &Reg1))
  {
    if ((Reg1 < 16) || (Reg1 > 23)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
    {
      Reg1 -= 16;
      DAsmCode[0] = 0x0d1840 + (Reg1 << 8) + Condition;
      CodeLen = 1;
    }
  }
  else
  {
    unsigned Offset = CutSize(&ArgStr[1], &Size);

    Dist = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], Offset, AdrInt, &OK, &Flags) - EProgCounter();
    if (Size == 0)
      Size = ((Dist > -256) && (Dist < 255)) ? 1 : 2;
    switch (Size)
    {
      case 1:
        if (!mSymbolQuestionable(Flags) && ((Dist < -256) || (Dist > 255))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          Dist &= 0x1ff;
          DAsmCode[0] = 0x050400 + (Condition << 12) + ((Dist & 0x1e0) << 1) + (Dist & 0x1f);
          CodeLen = 1;
        }
        break;
      case 2:
        DAsmCode[0] = 0x0d1040 + Condition;
        DAsmCode[1] = Dist & 0xffffff;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeBScc(Word Condition)
{
  LongInt Reg1, Dist;
  Byte Size;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300)
   && DecodeReg(ArgStr[1].str.p_str, &Reg1))
  {
    if ((Reg1 < 16) || (Reg1 > 23)) WrStrErrorPos(ErrNum_InvReg, &ArgStr[1]);
    else
    {
      Reg1 -= 16;
      DAsmCode[0] = 0x0d1800 + (Reg1 << 8) + Condition;
      CodeLen = 1;
    }
  }
  else
  {
    unsigned Offset;

    Offset = CutSize(&ArgStr[1], &Size);
    Dist = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], Offset, AdrInt, &OK, &Flags) - EProgCounter();
    if (Size == 0)
      Size = ((Dist > -256) && (Dist < 255)) ? 1 : 2;
    switch (Size)
    {
      case 1:
        if (!mSymbolQuestionable(Flags) && ((Dist < -256) || (Dist > 255))) WrError(ErrNum_JmpDistTooBig);
        else
        {
          Dist &= 0x1ff;
          DAsmCode[0] = 0x050000 + (Condition << 12) + ((Dist & 0x1e0) << 1) + (Dist & 0x1f);
          CodeLen = 1;
        }
        break;
      case 2:
        DAsmCode[0] = 0x0d1000 + Condition;
        DAsmCode[1] = Dist & 0xffffff;
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeLUA_LEA(Word Code)
{
  LongInt Reg1;

  UNUSED(Code);

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (!DecodeReg(RightComp.str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (Reg1 > 31) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else
    {
      tAdrResult AdrResult;

      DecodeAdr(&LeftComp, MModModInc | MModModDec | MModPostInc | MModPostDec | MModDisp, MSegXData, &AdrResult);
      if (AdrResult.Type == ModDisp)
      {
        if (ChkRange(AdrResult.Val, -64, 63))
        {
          AdrResult.Val &= 0x7f;
          DAsmCode[0] = 0x040000 + (Reg1 - 16) + (AdrResult.Mode << 8)
                      + ((AdrResult.Val & 0x0f) << 4)
                      + ((AdrResult.Val & 0x70) << 7);
           CodeLen = 1;
        }
      }
      else if (AdrResult.Type != ModNone)
      {
        CodeLen = 1;
        DAsmCode[0] = 0x044000 + (AdrResult.Mode << 8) + Reg1;
      }
    }
  }
}

static void DecodeLRA(Word Code)
{
  LongInt Reg1, Reg2;
  Boolean OK;

  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if (*RightComp.str.p_str == '\0') WrError(ErrNum_CannotSplitArg);
    else if (!DecodeGeneralReg(RightComp.str.p_str, &Reg1)) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (Reg1 > 0x1f) WrStrErrorPos(ErrNum_InvReg, &RightComp);
    else if (DecodeGeneralReg(LeftComp.str.p_str, &Reg2))
    {
      if ((Reg2 < 16) || (Reg2 > 23)) WrStrErrorPos(ErrNum_InvReg, &LeftComp);
      else
      {
        DAsmCode[0] = 0x04c000 + ((Reg2 & 7) << 8) + Reg1;
        CodeLen = 1;
      }
    }
    else
    {
      DAsmCode[1] = EvalStrIntExpression(&LeftComp, AdrInt, &OK) - EProgCounter();
      if (OK)
      {
        DAsmCode[0] = 0x044040 + Reg1;
        CodeLen = 2;
      }
    }
  }
}

static void DecodePLOCK(Word Code)
{
  UNUSED(Code);

  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    tAdrResult AdrResult;

    DecodeAdr(&ArgStr[1], MModNoImm, MSegCode, &AdrResult);
    if (AdrResult.Type != ModNone)
    {
      DAsmCode[0] = 0x0ac081 + (AdrResult.Mode << 8); DAsmCode[1] = AdrResult.Val;
      CodeLen = 2;
    }
  }
}

static void DecodePLOCKR_PUNLOCKR(Word Code)
{
  if (ChkArgCnt(1, 1)
   && ChkMinCPU(CPU56300))
  {
    Boolean OK;

    DAsmCode[1] = (EvalStrIntExpression(&ArgStr[1],  AdrInt,  &OK) - EProgCounter()) & 0xffffff;
    if (OK)
    {
      DAsmCode[0] = Code;
      CodeLen = 2;
    }
  }
}

/* Spruenge */

static void DecodeJMP_JSR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    LongWord AddVal = (LongWord)Code << 16;
    tAdrResult AdrResult;

    DecodeAdr(&ArgStr[1], MModNoImm, MSegCode, &AdrResult);
    if (AdrResult.Type == ModAbs)
     if (((AdrResult.Val & 0xfff000) == 0) && (AdrResult.ShortMode != 2))
     {
       CodeLen = 1;
       DAsmCode[0] = 0x0c0000 + AddVal + (AdrResult.Val & 0xfff);
     }
     else
     {
       CodeLen = 2;
       DAsmCode[0] = 0x0af080 + AddVal;
       DAsmCode[1] = AdrResult.Val;
     }
    else if (AdrResult.Type != ModNone)
    {
      CodeLen = 1;
      DAsmCode[0] = 0x0ac080 + AddVal + (AdrResult.Mode << 8);
    }
  }
}

static void DecodeJcc(Word Condition)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    DecodeAdr(&ArgStr[1], MModNoImm, MSegCode, &AdrResult);
    if (AdrResult.Type == ModAbs)
    {
      if (((AdrResult.Val & 0xfff000) == 0) && (AdrResult.ShortMode != 2))
      {
        CodeLen = 1;
        DAsmCode[0] = 0x0e0000 + (Condition << 12) + (AdrResult.Val & 0xfff);
      }
      else
      {
        CodeLen = 2;
        DAsmCode[0] = 0x0af0a0 + Condition;
        DAsmCode[1] = AdrResult.Val;
      }
    }
    else if (AdrResult.Type != ModNone)
    {
      CodeLen = 1;
      DAsmCode[0] = 0x0ac0a0 + Condition + (AdrResult.Mode << 8);
    }
  }
}

static void DecodeJScc(Word Condition)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult AdrResult;

    DecodeAdr(&ArgStr[1], MModNoImm, MSegCode, &AdrResult);
    if (AdrResult.Type == ModAbs)
    {
      if (((AdrResult.Val & 0xfff000) == 0) && (AdrResult.ShortMode != 2))
      {
        CodeLen = 1;
        DAsmCode[0] = 0x0f0000 + (Condition << 12) + (AdrResult.Val & 0xfff);
      }
      else
      {
        CodeLen = 2;
        DAsmCode[0] = 0x0bf0a0 + Condition;
        DAsmCode[1] = AdrResult.Val;
      }
    }
    else if (AdrResult.Type != ModNone)
    {
      CodeLen = 1;
      DAsmCode[0] = 0x0bc0a0 + Condition + (AdrResult.Mode << 8);
    }
  }
}

static void DecodeBitJmp(Word Code)
{
  Boolean OK;
  tSymbolFlags Flags;
  LongInt h, Reg1, Reg2, Reg3;

  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &MidComp);
    SplitArg(&MidComp, &MidComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*MidComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else if (*LeftComp.str.p_str != '#') WrError(ErrNum_OnlyImmAddr);
    else
    {
      DAsmCode[1] = EvalStrIntExpression(&RightComp, AdrInt, &OK);
      if (OK)
      {
        h = EvalStrIntExpressionOffsWithFlags(&LeftComp, 1, Int8, &OK, &Flags);
        if (mFirstPassUnknown(Flags))
          h &= 15;
        if (OK)
        {
          if ((h < 0) || (h > 23)) WrError(ErrNum_OverRange);
          else
          {
            Reg2 = ((Code & 1) << 5) + (((LongInt)(Code >> 1)) << 16);
            if (DecodeGeneralReg(MidComp.str.p_str, &Reg1))
            {
              CodeLen = 2;
              DAsmCode[0] = 0x0ac000 + h + Reg2 + (Reg1 << 8);
            }
            else
            {
              tAdrResult AdrResult;

              DecodeAdr(&MidComp, MModNoImm, MSegXData + MSegYData, &AdrResult);
              Reg3 = Ord(AdrResult.Seg == SegYData) << 6;
              if (AdrResult.Type == ModAbs)
              {
                if ((AdrResult.Val >= 0) && (AdrResult.Val <= 63))
                {
                  CodeLen = 2;
                  DAsmCode[0] = 0x0a0080 + h + Reg2 + Reg3 + (AdrResult.Val << 8);
                }
                else if ((AdrResult.Val >= MemLimit - 0x3f) && (AdrResult.Val <= MemLimit))
                {
                  CodeLen = 2;
                  DAsmCode[0] = 0x0a8080 + h + Reg2 + Reg3 + ((AdrResult.Val & 0x3f) << 8);
                }
                else if ((MomCPU >= CPU56300) && (AdrResult.Val >= MemLimit - 0x7f) && (AdrResult.Val <= MemLimit - 0x40))
                {
                  CodeLen = 2;
                  Reg2 = ((Code & 1) << 5) + (((LongInt)(Code >> 1)) << 14);
                  DAsmCode[0] = 0x018080 + h + Reg2 + Reg3 + ((AdrResult.Val & 0x3f) << 8);
                }
                else WrError(ErrNum_OverRange);
              }
              else if (AdrResult.Type != ModNone)
              {
                CodeLen = 2;
                DAsmCode[0] = 0x0a4080 + h + Reg2 + Reg3 + (AdrResult.Mode << 8);
              }
            }
          }
        }
      }
    }
  }
}

static void DecodeDO_DOR(Word Code)
{
  if (ChkArgCnt(1, 1))
  {
    SplitArg(&ArgStr[1], &LeftComp, &RightComp);
    if ((*LeftComp.str.p_str == '\0') || (*RightComp.str.p_str == '\0')) WrError(ErrNum_CannotSplitArg);
    else
    {
      LongInt Reg1;
      tEvalResult EvalResult;

      DAsmCode[1] = EvalStrIntExpressionWithResult(&RightComp, AdrInt, &EvalResult) - 1;
      if (EvalResult.OK)
      {
        ChkSpace(SegCode, EvalResult.AddrSpaceMask);
        if (!as_strcasecmp(LeftComp.str.p_str, "FOREVER"))
        {
          if (ChkMinCPU(CPU56300))
          {
            DAsmCode[0] = 0x000203 - Code;
            CodeLen = 2;
          }
        }
        else if (DecodeGeneralReg(LeftComp.str.p_str, &Reg1))
        {
          if (Reg1 == 0x3c) WrXError(ErrNum_InvReg, LeftComp.str.p_str); /* kein SSH!! */
          else
          {
            CodeLen = 2;
            DAsmCode[0] = 0x06c000 + (Reg1 << 8) + (Code << 4);
          }
        }
        else if (*LeftComp.str.p_str == '#')
        {
          Reg1 = EvalStrIntExpressionOffsWithResult(&LeftComp, 1, UInt12, &EvalResult);
          if (EvalResult.OK)
          {
            CodeLen = 2;
            DAsmCode[0] = 0x060080 + (Reg1 >> 8) + ((Reg1 & 0xff) << 8) + (Code << 4);
          }
        }
        else
        {
          tAdrResult AdrResult;

          DecodeAdr(&LeftComp, MModNoImm, MSegXData + MSegYData, &AdrResult);
          if (AdrResult.Type == ModAbs)
           if ((AdrResult.Val < 0) || (AdrResult.Val > 63)) WrError(ErrNum_OverRange);
           else
           {
             CodeLen = 2;
             DAsmCode[0] = 0x060000 + (AdrResult.Val << 8) + (Ord(AdrResult.Seg == SegYData) << 6) + (Code << 4);
           }
          else
          {
            CodeLen = 2;
            DAsmCode[0] = 0x064000 + (AdrResult.Mode << 8) + (Ord(AdrResult.Seg == SegYData) << 6) + (Code << 4);
          }
        }
      }
    }
  }
}

static void DecodeBRKcc(Word Condition)
{
  if (ChkArgCnt(0, 0)
   && ChkMinCPU(CPU56300))
  {
    DAsmCode[0] = 0x00000210 + Condition;
    CodeLen = 1;
  }
}

static void DecodeTRAPcc(Word Condition)
{
  if (ChkArgCnt(0, 0)
   && ChkMinCPU(CPU56300))
  {
    DAsmCode[0] = 0x000010 + Condition;
    CodeLen = 1;
  }
}

static void DecodeDEBUGcc(Word Condition)
{
  if (ChkArgCnt(0, 0)
   && ChkMinCPU(CPU56300))
  {
    DAsmCode[0] = 0x00000300 + Condition;
    CodeLen = 1;
  }
}

static void DecodeREP(Word Code)
{
  LongInt Reg1;

  UNUSED(Code);

  if (!ChkArgCnt(1, 1));
  else if (DecodeGeneralReg(ArgStr[1].str.p_str, &Reg1))
  {
    CodeLen = 1;
    DAsmCode[0] = 0x06c020 + (Reg1 << 8);
  }
  else
  {
    tAdrResult AdrResult;

    DecodeAdr(&ArgStr[1], MModAll, MSegXData + MSegYData, &AdrResult);
    if (AdrResult.Type == ModImm)
    {
      if ((AdrResult.Val < 0) || (AdrResult.Val > 0xfff)) WrError(ErrNum_OverRange);
      else
      {
        CodeLen = 1;
        DAsmCode[0] = 0x0600a0 + (AdrResult.Val >> 8) + ((AdrResult.Val & 0xff) << 8);
      }
    }
    else if (AdrResult.Type == ModAbs)
    {
      if ((AdrResult.Val < 0) || (AdrResult.Val > 63)) WrError(ErrNum_OverRange);
      else
      {
        CodeLen = 1;
        DAsmCode[0] = 0x060020 + (AdrResult.Val << 8) + (Ord(AdrResult.Seg == SegYData) << 6);
      }
    }
    else
    {
      CodeLen = 1 + AdrResult.Cnt;
      DAsmCode[1] = AdrResult.Val;
      DAsmCode[0] = 0x064020 + (AdrResult.Mode << 8) + (Ord(AdrResult.Seg == SegYData) << 6);
    }
  }
}

/*----------------------------------------------------------------------------------------------*/

static void AddFixed(const char *Name, LongWord Code, CPUVar NMin)
{
  order_array_rsv_end(FixedOrders, FixedOrder);
  FixedOrders[InstrZ].Code = Code;
  FixedOrders[InstrZ].MinCPU = NMin;
  AddInstTable(InstTable, Name, InstrZ++, DecodeFixed);
}

static void AddPar(const char *Name, ParTyp Typ, LongWord Code)
{
  order_array_rsv_end(ParOrders, ParOrder);
  ParOrders[InstrZ].Typ = Typ;
  ParOrders[InstrZ].Code = Code;
  AddInstTable(InstTable, Name, InstrZ++, DecodePar);
}

static void AddMix(const char *pName, Word Code, InstProc Proc, unsigned Mask)
{
  char TmpName[30];

  if (Mask & 1)
  {
    as_snprintf(TmpName, sizeof(TmpName), "%sSS", pName);
    AddInstTable(InstTable, TmpName, Code + 0x0000, Proc);
  }
  if (Mask & 2)
  {
    as_snprintf(TmpName, sizeof(TmpName), "%sSU", pName);
    AddInstTable(InstTable, TmpName, Code + 0x0100, Proc);
  }
  if (Mask & 4)
  {
    as_snprintf(TmpName, sizeof(TmpName), "%sUU", pName);
    AddInstTable(InstTable, TmpName, Code + 0x0140, Proc);
  }
}

static void AddCondition(const char *pName, InstProc Proc)
{
  unsigned z;
  char TmpName[30];
  Word Code;

  for (z = 0; z < CondCount; z++)
  {
    as_snprintf(TmpName, sizeof(TmpName), "%s%s", pName, CondNames[z]);
    Code = (z == CondCount - 1) ? 8 : z & 15;
    AddInstTable(InstTable, TmpName, Code, Proc);
  }
}

static void InitFields(void)
{
  InstTable = CreateInstTable(307);
  SetDynamicInstTable(InstTable);
  AddInstTable(InstTable, "DIV", 0, DecodeDIV);
  AddInstTable(InstTable, "INC", 0x0008, DecodeINC_DEC);
  AddInstTable(InstTable, "DEC", 0x000a, DecodeINC_DEC);
  AddInstTable(InstTable, "ANDI", 0x00b8, DecodeANDI_ORI);
  AddInstTable(InstTable, "ORI", 0x00f8, DecodeANDI_ORI);
  AddInstTable(InstTable, "NORM", 0, DecodeNORM);
  AddInstTable(InstTable, "NORMF", 0, DecodeNORMF);
  AddInstTable(InstTable, "EXTRACT", 0, DecodeEXTRACT_EXTRACTU);
  AddInstTable(InstTable, "EXTRACTU", 128, DecodeEXTRACT_EXTRACTU);
  AddInstTable(InstTable, "INSERT", 0, DecodeINSERT);
  AddInstTable(InstTable, "MERGE", 0, DecodeMERGE);
  AddInstTable(InstTable, "CLB", 0, DecodeCLB);
  AddInstTable(InstTable, "CMPU", 0, DecodeCMPU);
  AddInstTable(InstTable, "MOVE", 0, DecodePlainMOVE);
  AddInstTable(InstTable, "MOVEC", 0, DecodeMOVEC);
  AddInstTable(InstTable, "MOVEM", 0, DecodeMOVEM);
  AddInstTable(InstTable, "MOVEP", 0, DecodeMOVEP);
  AddInstTable(InstTable, "TFR", 0, DecodePlainTFR);
  AddInstTable(InstTable, "BRA", 0x40, DecodeBRA_BSR);
  AddInstTable(InstTable, "BSR", 0x00, DecodeBRA_BSR);
  AddInstTable(InstTable, "LUA", 0, DecodeLUA_LEA);
  AddInstTable(InstTable, "LEA", 0, DecodeLUA_LEA);
  AddInstTable(InstTable, "LRA", 0, DecodeLRA);
  AddInstTable(InstTable, "PLOCK", 0, DecodePLOCK);
  AddInstTable(InstTable, "PLOCKR", 0x00000e, DecodePLOCKR_PUNLOCKR);
  AddInstTable(InstTable, "PUNLOCKR", 0x00000f, DecodePLOCKR_PUNLOCKR);
  AddInstTable(InstTable, "JMP", 0, DecodeJMP_JSR);
  AddInstTable(InstTable, "JSR", 1, DecodeJMP_JSR);
  AddInstTable(InstTable, "DO", 0, DecodeDO_DOR);
  AddInstTable(InstTable, "DOR", 1, DecodeDO_DOR);
  AddInstTable(InstTable, "REP", 0, DecodeREP);

  InstrZ = 0;
  AddFixed("NOP"    , 0x000000, CPU56000);
  AddFixed("ENDDO"  , 0x00008c, CPU56000);
  AddFixed("ILLEGAL", 0x000005, CPU56000);
  AddFixed("RESET"  , 0x000084, CPU56000);
  AddFixed("RTI"    , 0x000004, CPU56000);
  AddFixed("RTS"    , 0x00000c, CPU56000);
  AddFixed("STOP"   , 0x000087, CPU56000);
  AddFixed("SWI"    , 0x000006, CPU56000);
  AddFixed("WAIT"   , 0x000086, CPU56000);
  AddFixed("DEBUG"  , 0x000200, CPU56300);
  AddFixed("PFLUSH" , 0x000003, CPU56300);
  AddFixed("PFLUSHUN",0x000001, CPU56300);
  AddFixed("PFREE"  , 0x000002, CPU56300);
  AddFixed("TRAP"   , 0x000006, CPU56300);

  InstrZ = 0;
  AddPar("ABS" , ParAB,     0x26);
  AddPar("ASL" , ParABShl1, 0x32);
  AddPar("ASR" , ParABShl1, 0x22);
  AddPar("CLR" , ParAB,     0x13);
  AddPar("LSL" , ParABShl2, 0x33);
  AddPar("LSR" , ParABShl2, 0x23);
  AddPar("NEG" , ParAB,     0x36);
  AddPar("NOT" , ParAB,     0x17);
  AddPar("RND" , ParAB,     0x11);
  AddPar("ROL" , ParAB,     0x37);
  AddPar("ROR" , ParAB,     0x27);
  AddPar("TST" , ParAB,     0x03);
  AddPar("ADC" , ParXYAB,   0x21);
  AddPar("SBC" , ParXYAB,   0x25);
  AddPar("ADD" , ParABXYnAB,0x00);
  AddPar("CMP" , ParABXYnAB,0x05);
  AddPar("CMPM", ParABXYnAB,0x07);
  AddPar("SUB" , ParABXYnAB,0x04);
  AddPar("ADDL", ParABBA,   0x12);
  AddPar("ADDR", ParABBA,   0x02);
  AddPar("SUBL", ParABBA,   0x16);
  AddPar("SUBR", ParABBA,   0x06);
  AddPar("AND" , ParXYnAB,  0x46);
  AddPar("EOR" , ParXYnAB,  0x43);
  AddPar("OR"  , ParXYnAB,  0x42);
  AddPar("MAC" , ParMul,    0x82);
  AddPar("MACR", ParMul,    0x83);
  AddPar("MPY" , ParMul,    0x80);
  AddPar("MPYR", ParMul,    0x81);
  AddPar("MAX" , ParFixAB,  0x1d);
  AddPar("MAXM", ParFixAB,  0x15);

  InstrZ = 0;
  AddInstTable(InstTable, "MPYI", InstrZ++, DecodeImmMac);
  AddInstTable(InstTable, "MPYRI", InstrZ++, DecodeImmMac);
  AddInstTable(InstTable, "MACI", InstrZ++, DecodeImmMac);
  AddInstTable(InstTable, "MACRI", InstrZ++, DecodeImmMac);

  InstrZ = 0;
  AddInstTable(InstTable, "BCLR", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BSET", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BCHG", InstrZ++, DecodeBit);
  AddInstTable(InstTable, "BTST", InstrZ++, DecodeBit);

  InstrZ = 0;
  AddInstTable(InstTable, "BRCLR", InstrZ++, DecodeBitBr);
  AddInstTable(InstTable, "BRSET", InstrZ++, DecodeBitBr);
  AddInstTable(InstTable, "BSCLR", InstrZ++, DecodeBitBr);
  AddInstTable(InstTable, "BSSET", InstrZ++, DecodeBitBr);

  InstrZ = 0;
  AddInstTable(InstTable, "JCLR", InstrZ++, DecodeBitJmp);
  AddInstTable(InstTable, "JSET", InstrZ++, DecodeBitJmp);
  AddInstTable(InstTable, "JSCLR", InstrZ++, DecodeBitJmp);
  AddInstTable(InstTable, "JSSET", InstrZ++, DecodeBitJmp);

  AddMix("DMAC", 0, DecodeDMAC, 7);
  AddMix("MAC", 0xff00, DecodeMAC_MPY, 6);
  AddMix("MPY", 0, DecodeMAC_MPY, 6);

  AddCondition("T", DecodeTcc);
  AddCondition("B", DecodeBcc);
  AddCondition("BS", DecodeBScc);
  AddCondition("J", DecodeJcc);
  AddCondition("JS", DecodeJScc);
  AddCondition("BRK", DecodeBRKcc);
  AddCondition("TRAP", DecodeTRAPcc);
  AddCondition("DEBUG", DecodeDEBUGcc);

  AddInstTable(InstTable, "XSFR", SegXData, DecodeSFR);
  AddInstTable(InstTable, "YSFR", SegYData, DecodeSFR);
  AddInstTable(InstTable, "DS", 0, DecodeDS);
  AddInstTable(InstTable, "DC", 0, DecodeDC);

  StrCompAlloc(&LeftComp, STRINGSIZE);
  StrCompAlloc(&MidComp, STRINGSIZE);
  StrCompAlloc(&RightComp, STRINGSIZE);
  StrCompAlloc(&Left1Comp, STRINGSIZE);
  StrCompAlloc(&Left2Comp, STRINGSIZE);
  StrCompAlloc(&Right1Comp, STRINGSIZE);
  StrCompAlloc(&Right2Comp, STRINGSIZE);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
  order_array_free(FixedOrders);
  order_array_free(ParOrders);

  StrCompFree(&LeftComp);
  StrCompFree(&MidComp);
  StrCompFree(&RightComp);
  StrCompFree(&Left1Comp);
  StrCompFree(&Left2Comp);
  StrCompFree(&Right1Comp);
  StrCompFree(&Right2Comp);
}

static void MakeCode_56K(void)
{
  CodeLen = 0;
  DontPrint = False;

  /* zu ignorierendes */

  if (Memo(""))
    return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_56K(void)
{
  return ((Memo("XSFR")) || (Memo("YSFR")));
}

static void SwitchFrom_56K(void)
{
  DeinitFields();
}

static void SwitchTo_56K(void)
{
  TurnWords = True;
  SetIntConstMode(eIntConstModeMoto);

  PCSymbol = "*";
  HeaderID = 0x09;
  NOPCode = 0x000000;
  DivideChars = " \t";
  HasAttrs = False;

  if (MomCPU == CPU56300)
  {
    AdrInt = UInt24;
    MemLimit = 0xffffffl;
  }
  else
  {
    AdrInt = UInt16;
    MemLimit = 0xffff;
  }

  ValidSegs = (1 << SegCode) | (1 << SegXData) | (1 << SegYData);
  Grans[SegCode ] = 4; ListGrans[SegCode ] = 4; SegInits[SegCode ] = 0;
  SegLimits[SegCode ]  =  MemLimit;
  Grans[SegXData] = 4; ListGrans[SegXData] = 4; SegInits[SegXData] = 0;
  SegLimits[SegXData]  =  MemLimit;
  Grans[SegYData] = 4; ListGrans[SegYData] = 4; SegInits[SegYData] = 0;
  SegLimits[SegYData] = MemLimit;

  onoff_packing_add(True);

  MakeCode = MakeCode_56K;
  IsDef = IsDef_56K;
  SwitchFrom = SwitchFrom_56K;
  InitFields();
}

void code56k_init(void)
{
  CPU56000 = AddCPU("56000", SwitchTo_56K);
  CPU56002 = AddCPU("56002", SwitchTo_56K);
  CPU56300 = AddCPU("56300", SwitchTo_56K);
}
