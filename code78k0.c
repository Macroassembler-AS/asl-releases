/* code78k0.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 78K0-Familie                                                */
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
#include "intpseudo.h"
#include "codevars.h"
#include "errmsg.h"

#include "code78k0.h"

enum
{
  ModNone = -1,
  ModReg8 = 0,
  ModReg16 = 1,
  ModImm = 2,
  ModShort = 3,
  ModSFR = 4,
  ModAbs = 5,
  ModIReg = 6,
  ModIndex = 7,
  ModDisp = 8
};

#define MModReg8 (1 << ModReg8)
#define MModReg16 (1 << ModReg16)
#define MModImm (1 << ModImm)
#define MModShort (1 << ModShort)
#define MModSFR (1 << ModSFR)
#define MModAbs (1 << ModAbs)
#define MModIReg (1 << ModIReg)
#define MModIndex (1 << ModIndex)
#define MModDisp (1 << ModDisp)

#define AccReg 1
#define AccReg16 0

static Byte OpSize,AdrPart;
static Byte AdrVals[2];
static ShortInt AdrMode;

static CPUVar CPU78070;

/*-------------------------------------------------------------------------*/
/* Adressausdruck parsen */

static void DecodeAdr(const tStrComp *pArg, Word Mask)
{
  static const char RegNames[8][2] =
  {
    "X","A","C","B","E","D","L","H"
  };

  Word AdrWord;
  int z;
  Boolean OK, LongFlag;
  int ArgLen = strlen(pArg->str.p_str);
  tSymbolFlags Flags;

  AdrMode = ModNone;
  AdrCnt = 0;

  /* Register */

  for (z = 0; z < 8; z++)
    if (!as_strcasecmp(pArg->str.p_str, RegNames[z]))
    {
      AdrMode = ModReg8;
      AdrPart = z;
      goto chk;
    }

  if (as_toupper(*pArg->str.p_str) == 'R')
  {
    if ((strlen(pArg->str.p_str) == 2) && (pArg->str.p_str[1] >= '0') && (pArg->str.p_str[1] <= '7'))
    {
      AdrMode = ModReg8;
      AdrPart = pArg->str.p_str[1] - '0';
      goto chk;
    }
    else if ((ArgLen == 3) && (as_toupper(pArg->str.p_str[1]) == 'P') && (pArg->str.p_str[2] >= '0') && (pArg->str.p_str[2] <= '3'))
    {
      AdrMode = ModReg16;
      AdrPart = pArg->str.p_str[2] - '0';
      goto chk;
    }
  }

  if (ArgLen == 2)
  {
    for (z = 0; z < 4; z++)
      if ((as_toupper(*pArg->str.p_str) == *RegNames[(z << 1) + 1])
       && (as_toupper(pArg->str.p_str[1]) == *RegNames[z << 1]))
      {
        AdrMode = ModReg16;
        AdrPart = z;
        goto chk;
      }
  }

  /* immediate */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case 0:
        AdrVals[0] = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        break;
      case 1:
        AdrWord = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        if (OK)
        {
          AdrVals[0] = Lo(AdrWord);
          AdrVals[1] = Hi(AdrWord);
        }
        break;
    }
    if (OK)
    {
      AdrMode = ModImm;
      AdrCnt = OpSize + 1;
    }
    goto chk;
  }

  /* indirekt */

  if ((*pArg->str.p_str == '[') && (pArg->str.p_str[ArgLen - 1] == ']'))
  {
    tStrComp Arg;

    StrCompRefRight(&Arg, pArg, 1);
    StrCompShorten(&Arg, 1);

    if ((!as_strcasecmp(Arg.str.p_str, "DE")) || (!as_strcasecmp(Arg.str.p_str, "RP2")))
    {
      AdrMode = ModIReg;
      AdrPart = 0;
    }
    else if ((!as_strncasecmp(Arg.str.p_str, "HL", 2)) && (!as_strncasecmp(Arg.str.p_str, "RP3", 3))) WrStrErrorPos(ErrNum_InvReg, &Arg);
    else
    {
      StrCompIncRefLeft(&Arg, 2);
      if (*Arg.str.p_str == '3')
        StrCompIncRefLeft(&Arg, 1);
      if ((!as_strcasecmp(Arg.str.p_str, "+B")) || (!as_strcasecmp(Arg.str.p_str, "+R3")))
      {
        AdrMode = ModIndex;
        AdrPart = 1;
      }
      else if ((!as_strcasecmp(Arg.str.p_str, "+C")) || (!as_strcasecmp(Arg.str.p_str, "+R2")))
      {
        AdrMode = ModIndex;
        AdrPart = 0;
      }
      else
      {
        AdrVals[0] = EvalStrIntExpression(&Arg, UInt8, &OK);
        if (OK)
        {
          if (AdrVals[0] == 0)
          {
            AdrMode = ModIReg;
            AdrPart = 1;
          }
          else
          {
            AdrMode = ModDisp;
            AdrCnt = 1;
          }
        }
      }
    }

    goto chk;
  }

  /* erzwungen lang ? */

  LongFlag = !!(*pArg->str.p_str == '!');

  /* -->absolut */

  AdrWord = EvalStrIntExpressionOffsWithFlags(pArg, LongFlag, UInt16, &OK, &Flags);
  if (mFirstPassUnknown(Flags))
  {
    AdrWord &= 0xffffe;
    if (!(Mask & MModAbs))
      AdrWord = (AdrWord | 0xff00) & 0xff1f;
  }
  if (OK)
  {
    if ((!LongFlag) && (Mask & MModShort) && (AdrWord >= 0xfe20) && (AdrWord <= 0xff1f))
    {
      AdrMode = ModShort;
      AdrCnt = 1;
      AdrVals[0] = Lo(AdrWord);
    }
    else if ((!LongFlag) && (Mask & MModSFR) && (((AdrWord >= 0xff00) && (AdrWord <= 0xffcf)) || (AdrWord >= 0xffe0)))
    {
      AdrMode = ModSFR;
      AdrCnt = 1;
      AdrVals[0] = Lo(AdrWord);
    }
    else
    {
      AdrMode = ModAbs;
      AdrCnt = 2;
      AdrVals[0] = Lo(AdrWord);
      AdrVals[1] = Hi(AdrWord);
    }
  }

chk:
  if ((AdrMode != ModNone) && (!(Mask & (1 << AdrMode))))
  {
    WrError(ErrNum_InvAddrMode);
    AdrMode = ModNone;
    AdrCnt = 0;
  }
}

static void ChkEven(void)
{
  if ((AdrMode==ModAbs) || (AdrMode==ModShort) || (AdrMode==ModSFR))
    if ((AdrVals[0]&1)==1) WrError(ErrNum_AddrNotAligned);
}

static Boolean DecodeBitAdr(const tStrComp *pArg, Byte *Erg)
{
  char *p;
  Boolean OK;
  tStrComp Reg, Bit;

  p = RQuotPos(pArg->str.p_str, '.');
  if (!p)
  {
    WrError(ErrNum_InvBitPos);
    return False;
  }

  StrCompSplitRef(&Reg, &Bit, pArg, p);
  *Erg = EvalStrIntExpression(&Bit, UInt3, &OK) << 4;
  if (!OK)
    return False;

  DecodeAdr(&Reg, MModShort | MModSFR | MModIReg | MModReg8);
  switch (AdrMode)
  {
    case ModReg8:
      if (AdrPart != AccReg)
      {
        WrError(ErrNum_InvAddrMode);
        return False;
      }
      else
      {
        *Erg += 0x88;
        return True;
      }
    case ModShort:
      return True;
    case ModSFR:
      *Erg += 0x08;
      return True;
    case ModIReg:
      if (AdrPart == 0)
      {
        WrError(ErrNum_InvAddrMode);
        return False;
      }
      else
      {
        *Erg += 0x80;
        return True;
      }
    default:
      return False;
  }
}

/*-------------------------------------------------------------------------*/
/* Instruction Decoders */

/* ohne Argument */

static void DecodeFixed(Word Code)
{
  if (ChkArgCnt(0, 0))
  {
    if (Hi(Code))
      BAsmCode[CodeLen++] = Hi(Code);
    BAsmCode[CodeLen++] = Lo(Code);
  }
}

/* Datentransfer */

static void DecodeMOV(Word Index)
{
  Byte HReg;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModShort | MModSFR | MModAbs
                       | MModIReg | MModIndex | MModDisp);
    switch (AdrMode)
    {
      case ModReg8:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModImm | MModReg8
                           | ((HReg == AccReg) ? MModShort | MModSFR | MModAbs | MModIReg | MModIndex | MModDisp : 0));
        switch (AdrMode)
        {
          case ModReg8:
            if ((HReg == AccReg) == (AdrPart == AccReg)) WrError(ErrNum_InvAddrMode);
            else if (HReg == AccReg)
            {
              CodeLen = 1;
              BAsmCode[0] = 0x60 + AdrPart;
            }
            else
            {
              CodeLen = 1;
              BAsmCode[0] = 0x70 + HReg;
            }
            break;
          case ModImm:
            CodeLen = 2;
            BAsmCode[0] = 0xa0 + HReg;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModShort:
            CodeLen = 2;
            BAsmCode[0] = 0xf0;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModSFR:
            CodeLen = 2;
            BAsmCode[0] = 0xf4;
            BAsmCode[1] = AdrVals[0];
            break;
          case ModAbs:
            CodeLen = 3;
            BAsmCode[0] = 0x8e;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            break;
          case ModIReg:
            CodeLen = 1;
            BAsmCode[0] = 0x85 + (AdrPart << 1);
            break;
          case ModIndex:
            CodeLen = 1;
            BAsmCode[0] = 0xaa + AdrPart;
            break;
          case ModDisp:
            CodeLen = 2;
            BAsmCode[0] = 0xae;
            BAsmCode[1] = AdrVals[0];
            break;
        }
        break;

      case ModShort:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg8 | MModImm);
        switch (AdrMode)
        {
          case ModReg8:
            if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xf2;
              CodeLen = 2;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0x11;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;

      case ModSFR:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg8 | MModImm);
        switch (AdrMode)
        {
          case ModReg8:
            if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xf6;
              CodeLen = 2;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0x13;
            BAsmCode[2] = AdrVals[0];
            CodeLen = 3;
            break;
        }
        break;

      case ModAbs:
        memcpy(BAsmCode + 1, AdrVals, 2);
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrMode == ModReg8)
        {
          if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0x9e;
            CodeLen = 3;
          }
        }
        break;

      case ModIReg:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrMode == ModReg8)
        {
          if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0x95 | (HReg << 1);
            CodeLen = 1;
          }
        }
        break;

      case ModIndex:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrMode == ModReg8)
        {
          if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0xba + HReg;
            CodeLen = 1;
          }
        }
        break;

      case ModDisp:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg8);
        if (AdrMode == ModReg8)
        {
          if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0xbe;
            CodeLen = 2;
          }
        }
        break;
    }
  }
}

static void DecodeXCH(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    Boolean Swap = (!as_strcasecmp(ArgStr[2].str.p_str, "A")) || (!as_strcasecmp(ArgStr[2].str.p_str, "RP1"));
    tStrComp *pArg1 = Swap ? &ArgStr[2] : &ArgStr[1],
             *pArg2 = Swap ? &ArgStr[1] : &ArgStr[2];

    DecodeAdr(pArg1, MModReg8);
    if (AdrMode != ModNone)
    {
      if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
      else
      {
        DecodeAdr(pArg2, MModReg8 | MModShort | MModSFR | MModAbs
                       | MModIReg | MModIndex | MModDisp);
        switch (AdrMode)
        {
          case ModReg8:
            if (AdrPart == AccReg) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x30 + AdrPart;
              CodeLen = 1;
            }
            break;
          case ModShort:
            BAsmCode[0] = 0x83;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModSFR:
            BAsmCode[0] = 0x93;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModAbs:
            BAsmCode[0] = 0xce;
            memcpy(BAsmCode + 1, AdrVals, AdrCnt);
            CodeLen = 3;
            break;
          case ModIReg:
            BAsmCode[0] = 0x05 + (AdrPart << 1);
            CodeLen = 1;
            break;
          case ModIndex:
            BAsmCode[0] = 0x31;
            BAsmCode[1] = 0x8a + AdrPart;
            CodeLen = 2;
            break;
          case ModDisp:
            BAsmCode[0] = 0xde;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
      }
    }
  }
}

static void DecodeMOVW(Word Index)
{
  Byte HReg;

  UNUSED(Index);

  OpSize = 1;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg16 | MModShort | MModSFR | MModAbs);
    switch (AdrMode)
    {
      case ModReg16:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg16 | MModImm
                           | ((HReg == AccReg16) ? MModShort | MModSFR | MModAbs : 0));
        switch (AdrMode)
        {
          case ModReg16:
            if ((HReg == AccReg16) == (AdrPart == AccReg16)) WrError(ErrNum_InvAddrMode);
            else if (HReg == AccReg16)
            {
              BAsmCode[0] = 0xc0 + (AdrPart << 1);
              CodeLen = 1;
            }
            else
            {
              BAsmCode[0] = 0xd0 + (HReg << 1);
              CodeLen = 1;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0x10 + (HReg << 1);
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModShort:
            BAsmCode[0] = 0x89;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            ChkEven();
            break;
          case ModSFR:
            BAsmCode[0] = 0xa9;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            ChkEven();
            break;
          case ModAbs:
            BAsmCode[0] = 0x02;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            ChkEven();
            break;
        }
        break;

      case ModShort:
        ChkEven();
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg16 | MModImm);
        switch (AdrMode)
        {
          case ModReg16:
            if (AdrPart != AccReg16) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0x99;
              CodeLen = 2;
            }
            break;
          case ModImm:
            BAsmCode[0] = 0xee;
            memcpy(BAsmCode + 2, AdrVals, 2);
            CodeLen = 4;
            break;
        }
        break;

      case ModSFR:
        ChkEven();
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModReg16 | MModImm);
        switch (AdrMode)
        {
          case ModReg16:
            if (AdrPart != AccReg16) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = 0xb9;
              CodeLen = 2;
            }
           break;
          case ModImm:
            BAsmCode[0] = 0xfe;
            memcpy(BAsmCode + 2,AdrVals, 2);
            CodeLen = 4;
            break;
        }
        break;

      case ModAbs:
        ChkEven();
        memcpy(BAsmCode + 1, AdrVals, AdrCnt);
        DecodeAdr(&ArgStr[2], MModReg16);
        if (AdrMode == ModReg16)
        {
          if (AdrPart != AccReg16) WrError(ErrNum_InvAddrMode);
          else
          {
            BAsmCode[0] = 0x03;
            CodeLen = 3;
          }
        }
        break;
    }
  }
}

static void DecodeXCHW(Word Index)
{
  Byte HReg;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrMode == ModReg16)
    {
      HReg = AdrPart;
      DecodeAdr(&ArgStr[2], MModReg16);
      if (AdrMode == ModReg16)
      {
        if ((HReg == AccReg16) == (AdrPart == AccReg16)) WrError(ErrNum_InvAddrMode);
        else
        {
          BAsmCode[0] = (HReg == AccReg16) ? 0xe0 + (AdrPart << 1) : 0xe0 + (HReg << 1);
          CodeLen = 1;
        }
      }
    }
  }
}

static void DecodeStack(Word Index)
{
  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "PSW"))
  {
    BAsmCode[0] = 0x22 + Index;
    CodeLen = 1;
  }
  else
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrMode == ModReg16)
    {
      BAsmCode[0] = 0xb1 - Index + (AdrPart << 1);
      CodeLen = 1;
    }
  }
}

/* Arithmetik */

static void DecodeAri(Word Index)
{
  Byte HReg;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModShort);
    switch (AdrMode)
    {
      case ModReg8:
        HReg = AdrPart;
        DecodeAdr(&ArgStr[2], MModReg8 | ((HReg == AccReg) ? (MModImm | MModShort | MModAbs | MModIReg | MModIndex | MModDisp) : 0));
        switch (AdrMode)
        {
          case ModReg8:
            if (AdrPart == AccReg)
            {
              BAsmCode[0] = 0x61;
              BAsmCode[1] = (Index << 4) + HReg;
              CodeLen = 2;
            }
            else if (HReg == AccReg)
            {
              BAsmCode[0] = 0x61;
              BAsmCode[1] = 0x08 + (Index << 4) + AdrPart;
              CodeLen = 2;
            }
            else WrError(ErrNum_InvAddrMode);
            break;
          case ModImm:
            BAsmCode[0] = (Index << 4) + 0x0d;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModShort:
            BAsmCode[0] = (Index << 4) + 0x0e;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
          case ModAbs:
            BAsmCode[0] = (Index << 4) + 0x08;
            memcpy(BAsmCode + 1, AdrVals, 2);
            CodeLen = 3;
            break;
          case ModIReg:
            if (AdrPart == 0) WrError(ErrNum_InvAddrMode);
            else
            {
              BAsmCode[0] = (Index << 4) + 0x0f;
              CodeLen = 1;
            }
            break;
          case ModIndex:
            BAsmCode[0] = 0x31;
            BAsmCode[1] = (Index << 4) + 0x0a + AdrPart;
            CodeLen = 2;
            break;
          case ModDisp:
            BAsmCode[0] = (Index << 4) + 0x09;
            BAsmCode[1] = AdrVals[0];
            CodeLen = 2;
            break;
        }
        break;

      case ModShort:
        BAsmCode[1] = AdrVals[0];
        DecodeAdr(&ArgStr[2], MModImm);
        if (AdrMode == ModImm)
        {
          BAsmCode[0] = (Index << 4) + 0x88;
          BAsmCode[2] = AdrVals[0];
          CodeLen = 3;
        }
        break;
    }
  }
}

static void DecodeAri16(Word Index)
{
  if (ChkArgCnt(2, 2))
  {
    OpSize = 1;
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrMode == ModReg16)
    {
      DecodeAdr(&ArgStr[2], MModImm);
      if (AdrMode == ModImm)
      {
        BAsmCode[0] = 0xca + (Index << 4);
        memcpy(BAsmCode + 1, AdrVals, 2);
        CodeLen = 3;
      }
    }
  }
}

static void DecodeMULU(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8);
    if (AdrMode == ModReg8)
    {
      if (AdrPart != 0) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x31;
        BAsmCode[1] = 0x88;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeDIVUW(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8);
    if (AdrMode == ModReg8)
    {
      if (AdrPart != 2) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x31;
        BAsmCode[1] = 0x82;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeINCDEC(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg8 | MModShort);
    switch (AdrMode)
    {
      case ModReg8:
        BAsmCode[0] = 0x40 + AdrPart + Index;
        CodeLen = 1;
        break;
      case ModShort:
        BAsmCode[0] = 0x81 + Index;
        BAsmCode[1] = AdrVals[0];
        CodeLen = 2;
        break;
    }
  }
}

static void DecodeINCDECW(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModReg16);
    if (AdrMode == ModReg16)
    {
      BAsmCode[0] = 0x80 + Index + (AdrPart << 1);
      CodeLen = 1;
    }
  }
}

static void DecodeShift(Word Index)
{
  Byte HReg;
  Boolean OK;

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8);
    if (AdrMode == ModReg8)
    {
      if (AdrPart != AccReg) WrError(ErrNum_InvAddrMode);
      else
      {
        HReg = EvalStrIntExpression(&ArgStr[2], UInt1, &OK);
        if (OK)
        {
          if (HReg != 1) WrError(ErrNum_UnderRange);
          else
          {
            BAsmCode[0] = 0x24 + Index;
            CodeLen = 1;
          }
        }
      }
    }
  }
}

static void DecodeRot4(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModIReg);
    if (AdrMode == ModIReg)
    {
      if (AdrPart == 0) WrError(ErrNum_InvAddrMode);
      else
      {
        BAsmCode[0] = 0x31;
        BAsmCode[1] = 0x80 + Index;
        CodeLen = 2;
      }
    }
  }
}

/* Bitoperationen */

static void DecodeMOV1(Word Index)
{
  Byte HReg;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    Boolean Swap = !as_strcasecmp(ArgStr[2].str.p_str, "CY");
    tStrComp *pArg1 = Swap ? &ArgStr[2] : &ArgStr[1],
             *pArg2 = Swap ? &ArgStr[1] : &ArgStr[2];
    int z = Swap ? 1 : 4;

    if (as_strcasecmp(pArg1->str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
    else if (DecodeBitAdr(pArg2, &HReg))
    {
      BAsmCode[0] = 0x61 + (Ord((HReg & 0x88) != 0x88) << 4);
      BAsmCode[1] = z + HReg;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen = 2 + AdrCnt;
    }
  }
}

static void DecodeBit2(Word Index)
{
  Byte HReg;

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else if (DecodeBitAdr(&ArgStr[2], &HReg))
  {
    BAsmCode[0] = 0x61 + (Ord((HReg & 0x88) != 0x88) << 4);
    BAsmCode[1] = Index + 5 + HReg;
    memcpy(BAsmCode + 2, AdrVals, AdrCnt);
    CodeLen = 2 + AdrCnt;
  }
}

static void DecodeSETCLR1(Word Index)
{
  Byte HReg;

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    BAsmCode[0] = 0x20 + Index;
    CodeLen = 1;
  }
  else if (DecodeBitAdr(&ArgStr[1], &HReg))
  {
    if ((HReg & 0x88) == 0)
    {
      BAsmCode[0] = 0x0a + Index + (HReg & 0x70);
      BAsmCode[1] = AdrVals[0];
      CodeLen = 2;
    }
    else
    {
      BAsmCode[0] =0x61 + (Ord((HReg & 0x88) != 0x88) << 4);
      BAsmCode[1] =HReg + 2 + Index;
      memcpy(BAsmCode + 2, AdrVals, AdrCnt);
      CodeLen=2 + AdrCnt;
    }
  }
}

static void DecodeNOT1(Word Index)
{
  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else
  {
    BAsmCode[0] = 0x01;
    CodeLen = 1;
  }
}

/* Spruenge */

static void DecodeCALL(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    DecodeAdr(&ArgStr[1], MModAbs);
    if (AdrMode == ModAbs)
    {
      BAsmCode[0] = 0x9a;
      memcpy(BAsmCode + 1, AdrVals, 2);
      CodeLen = 3;
    }
  }
}

static void DecodeCALLF(Word Index)
{
  Word AdrWord;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    AdrWord = EvalStrIntExpressionOffs(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt11, &OK);
    if (OK)
    {
      BAsmCode[0] = 0x0c | (Hi(AdrWord) << 4);
      BAsmCode[1] = Lo(AdrWord);
      CodeLen = 2;
    }
  }
}

static void DecodeCALLT(Word Index)
{
  Word AdrWord;
  Boolean OK;
  int l = strlen(ArgStr[1].str.p_str);

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if ((*ArgStr[1].str.p_str != '[') || (ArgStr[1].str.p_str[l - 1] != ']')) WrError(ErrNum_InvAddrMode);
  else
  {
    tSymbolFlags Flags;

    ArgStr[1].str.p_str[l - 1] = '\0';
    AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], 1, UInt6, &OK, &Flags);
    if (mFirstPassUnknown(Flags)) AdrWord &= 0xfffe;
    if (OK)
    {
      if (Odd(AdrWord)) WrError(ErrNum_NotAligned);
      else
      {
        BAsmCode[0] = 0xc1 + (AdrWord & 0x3e);
        CodeLen = 1;
      }
    }
  }
}

static void DecodeBR(Word Index)
{
  Word AdrWord;
  Integer AdrInt;
  Boolean OK;
  Byte HReg;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if ((!as_strcasecmp(ArgStr[1].str.p_str, "AX")) || (!as_strcasecmp(ArgStr[1].str.p_str, "RP0")))
  {
    BAsmCode[0] = 0x31;
    BAsmCode[1] = 0x98;
    CodeLen = 2;
  }
  else
  {
    unsigned Offset = 0;
    tSymbolFlags Flags;

    if (*ArgStr[1].str.p_str == '!')
    {
      Offset++;
      HReg = 1;
    }
    else if (*ArgStr[1].str.p_str == '$')
    {
      Offset++;
      HReg = 2;
    }
    else HReg = 0;
    AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], Offset, UInt16, &OK, &Flags);
    if (OK)
    {
      if (HReg == 0)
      {
        AdrInt = AdrWord - (EProgCounter() - 2);
        HReg = ((AdrInt >= -128) && (AdrInt < 127)) ? 2 : 1;
      }
      switch (HReg)
      {
        case 1:
          BAsmCode[0] = 0x9b;
          BAsmCode[1] = Lo(AdrWord);
          BAsmCode[2] = Hi(AdrWord);
          CodeLen = 3;
          break;
        case 2:
          AdrInt = AdrWord - (EProgCounter() + 2);
          if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
          else
          {
            BAsmCode[0] = 0xfa;
            BAsmCode[1] = AdrInt & 0xff;
            CodeLen = 2;
          }
          break;
      }
    }
  }
}

static void DecodeRel(Word Index)
{
  Integer AdrInt;
  Boolean OK;
  tSymbolFlags Flags;

  if (ChkArgCnt(1, 1))
  {
    AdrInt = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], ('$' == *ArgStr[1].str.p_str), UInt16, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[0] = 0x8d + (Index << 4);
        BAsmCode[1] = AdrInt & 0xff;
        CodeLen = 2;
      }
    }
  }
}

static void DecodeBRel(Word Index)
{
  Integer AdrInt;
  tSymbolFlags Flags;
  Byte HReg;
  Boolean OK;

  if (!ChkArgCnt(2, 2));
  else if (DecodeBitAdr(&ArgStr[1], &HReg))
  {
    if ((Index == 1) && ((HReg & 0x88) == 0))
    {
      BAsmCode[0] = 0x8c + HReg;
      BAsmCode[1] = AdrVals[0];
      HReg = 2;
    }
    else
    {
      BAsmCode[0] = 0x31;
      switch (HReg & 0x88)
      {
        case 0x00:
          BAsmCode[1] = 0x00;
          break;
        case 0x08:
          BAsmCode[1] = 0x04;
          break;
        case 0x80:
          BAsmCode[1] = 0x84;
          break;
        case 0x88:
          BAsmCode[1] = 0x0c;
          break;
      }
      BAsmCode[1] += (HReg & 0x70) + Index + 1;
      BAsmCode[2] = AdrVals[0];
      HReg = 2 + AdrCnt;
    }
    AdrInt = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], !!(*ArgStr[2].str.p_str == '$'), UInt16, &OK, &Flags) - (EProgCounter() + HReg + 1);
    if (OK)
    {
      if (((AdrInt < -128) || (AdrInt > 127)) & !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
      else
      {
        BAsmCode[HReg] = AdrInt & 0xff;
        CodeLen = HReg + 1;
      }
    }
  }
}

static void DecodeDBNZ(Word Index)
{
  Integer AdrInt;
  tSymbolFlags Flags;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    DecodeAdr(&ArgStr[1], MModReg8 + MModShort);
    if ((AdrMode == ModReg8) && ((AdrPart & 6) != 2)) WrError(ErrNum_InvAddrMode);
    else if (AdrMode != ModNone)
    {
      BAsmCode[0] = (AdrMode == ModReg8) ? 0x88 + AdrPart : 0x04;
      BAsmCode[1] = AdrVals[0];
      AdrInt = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], !!(*ArgStr[2].str.p_str == '$'), UInt16, &OK, &Flags) - (EProgCounter() + AdrCnt + 2);
      if (OK)
      {
        if (((AdrInt < -128) || (AdrInt > 127)) && !mSymbolQuestionable(Flags)) WrError(ErrNum_JmpDistTooBig);
        else
        {
          BAsmCode[AdrCnt + 1] = AdrInt & 0xff;
          CodeLen = AdrCnt + 2;
        }
      }
    }
  }
}

/* Steueranweisungen */

static void DecodeSEL(Word Index)
{
  Byte HReg;

  UNUSED(Index);

  if (ArgCnt != 1) WrError(ErrNum_InvAddrMode);
  else if ((strlen(ArgStr[1].str.p_str) != 3) || (as_strncasecmp(ArgStr[1].str.p_str, "RB", 2) != 0)) WrError(ErrNum_InvAddrMode);
  else
  {
    HReg = ArgStr[1].str.p_str[2] - '0';
    if (ChkRange(HReg, 0, 3))
    {
      BAsmCode[0] = 0x61;
      BAsmCode[1] = 0xd0 + ((HReg & 1) << 3) + ((HReg & 2) << 4);
      CodeLen = 2;
    }
  }
}

/*-------------------------------------------------------------------------*/
/* dynamische Codetabellenverwaltung */

static void AddFixed(const char *NewName, Word NewCode)
{
  AddInstTable(InstTable, NewName, NewCode, DecodeFixed);
}

static void AddAri(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeAri);
}

static void AddAri16(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeAri16);
}

static void AddShift(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeShift);
}

static void AddBit2(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeBit2);
}

static void AddRel(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeRel);
}

static void AddBRel(const char *NewName)
{
  AddInstTable(InstTable, NewName, InstrZ++, DecodeBRel);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "MOV"  , 0, DecodeMOV);
  AddInstTable(InstTable, "XCH"  , 0, DecodeXCH);
  AddInstTable(InstTable, "MOVW" , 0, DecodeMOVW);
  AddInstTable(InstTable, "XCHW" , 0, DecodeXCHW);
  AddInstTable(InstTable, "PUSH" , 0, DecodeStack);
  AddInstTable(InstTable, "POP"  , 1, DecodeStack);
  AddInstTable(InstTable, "MULU" , 0, DecodeMULU);
  AddInstTable(InstTable, "DIVUW", 0, DecodeDIVUW);
  AddInstTable(InstTable, "INC"  , 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC"  ,16, DecodeINCDEC);
  AddInstTable(InstTable, "INCW" , 0, DecodeINCDECW);
  AddInstTable(InstTable, "DECW" ,16, DecodeINCDECW);
  AddInstTable(InstTable, "ROL4" , 0, DecodeRot4);
  AddInstTable(InstTable, "ROR4" ,16, DecodeRot4);
  AddInstTable(InstTable, "MOV1" , 0, DecodeMOV1);
  AddInstTable(InstTable, "SET1" , 0, DecodeSETCLR1);
  AddInstTable(InstTable, "CLR1" , 1, DecodeSETCLR1);
  AddInstTable(InstTable, "NOT1" , 1, DecodeNOT1);
  AddInstTable(InstTable, "CALL" , 0, DecodeCALL);
  AddInstTable(InstTable, "CALLF", 0, DecodeCALLF);
  AddInstTable(InstTable, "CALLT", 0, DecodeCALLT);
  AddInstTable(InstTable, "BR"   , 0, DecodeBR);
  AddInstTable(InstTable, "DBNZ" , 0, DecodeDBNZ);
  AddInstTable(InstTable, "SEL"  , 0, DecodeSEL);

  AddFixed("BRK"  , 0x00bf); AddFixed("RET"  , 0x00af);
  AddFixed("RETB" , 0x009f); AddFixed("RETI" , 0x008f);
  AddFixed("HALT" , 0x7110); AddFixed("STOP" , 0x7100);
  AddFixed("NOP"  , 0x0000); AddFixed("EI"   , 0x7a1e);
  AddFixed("DI"   , 0x7b1e); AddFixed("ADJBA", 0x6180);
  AddFixed("ADJBS", 0x6190);

  InstrZ = 0;
  AddAri("ADD" ); AddAri("SUB" ); AddAri("ADDC"); AddAri("SUBC");
  AddAri("CMP" ); AddAri("AND" ); AddAri("OR"  ); AddAri("XOR" );

  InstrZ = 0;
  AddAri16("ADDW"); AddAri16("SUBW"); AddAri16("CMPW");

  InstrZ = 0;
  AddShift("ROR"); AddShift("RORC"); AddShift("ROL"); AddShift("ROLC");

  InstrZ = 0;
  AddBit2("AND1"); AddBit2("OR1"); AddBit2("XOR1");

  InstrZ = 0;
  AddRel("BC"); AddRel("BNC"); AddRel("BZ"); AddRel("BNZ");

  InstrZ = 0;
  AddBRel("BTCLR"); AddBRel("BT"); AddBRel("BF");
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/

static void MakeCode_78K0(void)
{
  CodeLen = 0;
  DontPrint = False;
  OpSize = 0;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
}

static Boolean IsDef_78K0(void)
{
  return False;
}

static void SwitchFrom_78K0(void)
{
  DeinitFields();
}

static void SwitchTo_78K0(void)
{
  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC";
  HeaderID = 0x7c;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xffff;

  MakeCode = MakeCode_78K0;
  IsDef = IsDef_78K0;
  SwitchFrom = SwitchFrom_78K0;
  InitFields();
}

void code78k0_init(void)
{
  CPU78070 = AddCPU("78070", SwitchTo_78K0);
}
