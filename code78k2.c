/* code78k2.c */
/*****************************************************************************/
/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only                     */
/*                                                                           */
/* AS-Portierung                                                             */
/*                                                                           */
/* Codegenerator 78K2-Familie                                                */
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
#include "intpseudo.h"
#include "codevars.h"
#include "headids.h"
#include "errmsg.h"

#include "code78k2.h"

/*-------------------------------------------------------------------------*/

enum
{
  ModNone = -1,
  ModImm = 0,
  ModReg8 = 1,
  ModReg16 = 2,
  ModMem = 3,
  ModAbs = 4,
  ModShort = 5,
  ModSFR = 6,
  ModPSW = 7,
  ModSP = 8,
  ModSTBC = 9
};

#define MModImm (1 << ModImm)
#define MModReg8 (1 << ModReg8)
#define MModReg16 (1 << ModReg16)
#define MModMem (1 << ModMem)
#define MModAbs (1 << ModAbs)
#define MModShort (1 << ModShort)
#define MModSFR (1 << ModSFR)
#define MModPSW (1 << ModPSW)
#define MModSP (1 << ModSP)
#define MModSTBC (1 << ModSTBC)

#define AccReg8 1
#define AccReg16 0

#define SFR_SP 0xfc
#define SFR_PSW 0xfe

/*-------------------------------------------------------------------------*/

static CPUVar CPU78214;

typedef struct
{
  ShortInt Mode, Val;
  Byte Vals[3];
  tSymbolFlags ValSymFlags;
  Boolean AltBank;
  int Cnt;
} tAdrResult;

static ShortInt OpSize;
static Byte *pCode;
static LongInt Reg_P6, Reg_PM6;

/*-------------------------------------------------------------------------*/
/* address decoders */

static Boolean SetOpSize(ShortInt NewSize)
{
  if (OpSize < 0)
  {
    OpSize = NewSize;
    return True;
  }
  else if (OpSize != NewSize)
  {
    WrError(ErrNum_ConfOpSizes);
    return False;
  }
  else
    return True;
}

static ShortInt DecodeReg8(char *pAsc)
{
  ShortInt Result = -1;
  int l = strlen(pAsc);

  if (l == 1)
  {
    static const char Reg8Names[9] = "XACBEDLH";
    const char *pPos = strchr(Reg8Names, as_toupper(*pAsc));

    if (pPos)
      Result = pPos - Reg8Names;
  }

  else if ((l == 2) && (toupper(pAsc[0]) == 'R'))
  {
    if ((pAsc[1] >= '0') && (pAsc[1] <= '7'))
      Result = pAsc[1] - '0';
  }

  return Result;
}

static ShortInt DecodeReg16(char *pAsc)
{
  ShortInt Result = -1;
  int l = strlen(pAsc);

  if (l == 2)
  {
    static const char Reg16Names[4][3] = {"AX", "BC", "DE", "HL"};
    int z;

    for (z = 0; z < 4; z++)
      if (!as_strcasecmp(Reg16Names[z], pAsc))
      {
        Result = z;
        break;
      }
  }

  else if ((l == 3) && (toupper(pAsc[0]) == 'R') && (toupper(pAsc[1]) == 'P'))
  {
    if ((pAsc[2] >= '0') && (pAsc[2] <= '3'))
      Result = pAsc[2] - '0';
  }

  return Result;
}

static ShortInt DecodeAdr(const tStrComp *pArg, Word Mask, tAdrResult *pResult)
{
  Word WordOp;
  LongWord LongOp;
  Boolean OK;
  tStrComp Arg;
  unsigned ForceLong;
  int ArgLen;

  pResult->Mode = ModNone;
  pResult->AltBank = False;
  pResult->Cnt = 0;

  /* immediate ? */

  if (*pArg->str.p_str == '#')
  {
    switch (OpSize)
    {
      case 0:
        pResult->Vals[0] = EvalStrIntExpressionOffs(pArg, 1, Int8, &OK);
        if (OK)
        {
          pResult->Cnt = 1;
          pResult->Mode = ModImm;
        }
        break;
      case 1:
        WordOp = EvalStrIntExpressionOffs(pArg, 1, Int16, &OK);
        if (OK)
        {
          pResult->Vals[0] = Lo(WordOp);
          pResult->Vals[1] = Hi(WordOp);
          pResult->Cnt = 2;
          pResult->Mode = ModImm;
        }
        break;
      default:
        WrError(ErrNum_UndefOpSizes);
    }
    goto AdrFound;
  }

  /* 8 bit registers? */

  if ((pResult->Val = DecodeReg8(pArg->str.p_str)) >= 0)
  {
    pResult->Mode = ModReg8;
    SetOpSize(0);
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "PSW"))
  {
    pResult->Mode = ModPSW;
    SetOpSize(0);
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "STBC"))
  {
    pResult->Mode = ModSTBC;
    SetOpSize(0);
    goto AdrFound;
  }

  /* 16 bit registers? */

  if ((pResult->Val = DecodeReg16(pArg->str.p_str)) >= 0)
  {
    pResult->Mode = ModReg16;
    SetOpSize(1);
    goto AdrFound;
  }

  if (!as_strcasecmp(pArg->str.p_str, "SP"))
  {
    pResult->Mode = ModSP;
    SetOpSize(1);
    goto AdrFound;
  }

  /* OK, everything that follows is memory: alternate bank ? */

  StrCompRefRight(&Arg, pArg, 0);
  if (*Arg.str.p_str == '&')
  {
    pResult->AltBank = True;
    StrCompIncRefLeft(&Arg, 1);
  }

  /* memory-indirect addressing? */

  ArgLen = strlen(Arg.str.p_str);
  if ((ArgLen >= 2) && (Arg.str.p_str[ArgLen - 1] == ']'))
  {
    tStrComp Base, Remainder;
    char *pStart;

    /* remove ']' */

    StrCompShorten(&Arg, 1);

    pStart = RQuotPos(Arg.str.p_str, '[');
    if (!pStart)
    {
      WrError(ErrNum_BrackErr);
      goto AdrFound;
    }

    /* purely indirect? */

    if (pStart == Arg.str.p_str)
    {
      static const char Modes[][5] = { "DE+",  "HL+",  "DE-",  "HL-",  "DE",  "HL",
                                       "RP2+", "RP3+", "RP2-", "RP3-", "RP2", "RP3" };
      unsigned z;
      char *pSep, Save;

      /* skip '[' */

      StrCompIncRefLeft(&Arg, 1);

      /* simple expression without displacement? */

      for (z = 0; z < sizeof(Modes) / sizeof(*Modes); z++)
        if (!as_strcasecmp(Arg.str.p_str, Modes[z]))
        {
          pResult->Mode = ModMem; pResult->Val = 0x16;
          pResult->Vals[0] = z % (sizeof(Modes) / sizeof(*Modes) / 2);
          pResult->Cnt = 1;
          goto AdrFound;
        }

      /* no -> extract base register. Its name ends with the first non-letter,
         which either means +/- or a blank */

      for (pSep = Arg.str.p_str; *pSep; pSep++)
        if (!as_isalpha(*pSep))
          break;

      /* decode base register.  SP is not otherwise handled. */

      Save = StrCompSplitRef(&Base, &Remainder, &Arg, pSep);
      if (!as_strcasecmp(Base.str.p_str, "SP"))
        pResult->Vals[0] = 1;
      else
      {
        int tmp;

        tmp = DecodeReg16(Base.str.p_str);
        if (tmp == 2) /* DE */
          pResult->Vals[0] = 0;
        else if (tmp == 3) /* HL */
          pResult->Vals[0] = 2;
        else
        {
          WrStrErrorPos(ErrNum_InvReg, &Base);
          goto AdrFound;
        }
      }

      /* now that we have the base, prepare displacement. */

      *pSep = Save;
      if (pSep > Arg.str.p_str)
        pSep--;
      *pSep = '0';
      pResult->Vals[1] = EvalStrIntExpressionOffs(&Arg, pSep - Arg.str.p_str, Int8, &OK);
      if (OK)
      {
        pResult->Mode = ModMem; pResult->Val = 0x06;
        pResult->Cnt = 2;
        goto AdrFound;
      }
    }

    /* no -> with outer displacement */

    else
    {
      tStrComp Disp, Reg;
      int tmp;

      /* split displacement + register */

      StrCompSplitRef(&Disp, &Reg, &Arg, pStart);

       /* handle base register */

      tmp = DecodeReg8(Reg.str.p_str);
      switch (tmp)
      {
        case 1: /* A/B */
        case 3:
          pResult->Vals[0] = tmp;
          break;
        case -1:
          tmp = DecodeReg16(Reg.str.p_str);
          if (tmp >= 2) /* DE/HL */
          {
            pResult->Vals[0] = (tmp - 2) << 1;
            break;
          }
          /* else fall-through */
        default:
          WrStrErrorPos(ErrNum_InvReg, &Reg);
          goto AdrFound;
      }

      /* compute displacement */

      WordOp = EvalStrIntExpression(&Disp, Int16, &OK);
      if (OK)
      {
        pResult->Mode = ModMem; pResult->Val = 0x0a;
        pResult->Vals[1] = Lo(WordOp); pResult->Vals[2] = Hi(WordOp);
        pResult->Cnt = 3;
        goto AdrFound;
      }
    }

  }

  /* OK, nothing but absolute left...exclamation mark enforces 16-bit addressing */

  ForceLong = !!(*Arg.str.p_str == '!');

  LongOp = EvalStrIntExpressionOffsWithFlags(&Arg, ForceLong, UInt20, &OK, &pResult->ValSymFlags);
  if (OK)
  {
    if (!mFirstPassUnknown(pResult->ValSymFlags))
    {
      LongWord CompBank = pResult->AltBank ? Reg_P6 : Reg_PM6;
      if (CompBank != (LongOp >> 16)) WrError(ErrNum_InAccPage);
    }

    WordOp = LongOp & 0xffff;

    if ((Mask & MModShort) && (!ForceLong) && ((WordOp >= 0xfe20) && (WordOp <= 0xff1f)))
    {
      pResult->Mode = ModShort; pResult->Cnt = 1;
      pResult->Vals[0] = Lo(WordOp);
    }
    else if ((Mask & MModSFR) && (!ForceLong) && (Hi(WordOp) == 0xff))
    {
      pResult->Mode = ModSFR; pResult->Cnt = 1;
      pResult->Vals[0] = Lo(WordOp);
    }
    else
    {
      pResult->Mode = ModAbs; pResult->Cnt = 2;
      pResult->Vals[0] = Lo(WordOp); pResult->Vals[1] = Hi(WordOp);
    }
  }

AdrFound:

  if ((pResult->Mode != ModNone) && (!(Mask & (1 << pResult->Mode))))
  {
    WrError(ErrNum_InvAddrMode);
    pResult->Mode = ModNone; pResult->Cnt = 0; pResult->AltBank = False;
  }
  return pResult->Mode;
}

static Boolean ChkAcc(const tStrComp *pArg)
{
  tAdrResult Result;

  if (DecodeAdr(pArg, OpSize ? MModReg16 : MModReg8, &Result) == ModNone)
    return False;

  if (((OpSize) && (Result.Val != AccReg16))
   || ((!OpSize) && (Result.Val != AccReg8)))
  {
    WrError(ErrNum_InvAddrMode);
    return False;
  }

  return True;
}

static Boolean ChkMem1(const tAdrResult *pResult)
{
  return (pResult->Val == 0x16) && (*pResult->Vals >= 4);
}

static Boolean DecodeBitAdr(const tStrComp *pArg, LongWord *pResult)
{
  char *pSplit;
  Boolean OK;

  pSplit = RQuotPos(pArg->str.p_str, '.');

  if (pSplit)
  {
    tStrComp Reg, Bit;

    StrCompSplitRef(&Reg, &Bit, pArg, pSplit);

    *pResult = EvalStrIntExpression(&Bit, UInt3, &OK) << 8;
    if (OK)
    {
      tAdrResult Result;

      switch (DecodeAdr(&Reg, MModReg8 | MModPSW | MModSFR | MModShort, &Result))
      {
        case ModReg8:
          if (Result.Val >= 2)
          {
            WrStrErrorPos(ErrNum_InvReg, &Reg);
            OK = FALSE;
          }
          else
            *pResult |= (((LongWord)Result.Val) << 11) | 0x00030000;
          break;
        case ModPSW:
          *pResult |= 0x00020000;
          break;
        case ModSFR:
          *pResult |= 0x01080800 | *Result.Vals;
          break;
        case ModShort:
          *pResult |= 0x01080000 | *Result.Vals;
          break;
        default:
          OK = FALSE;
      }
    }
  }
  else
    *pResult = EvalStrIntExpression(pArg, UInt32, &OK);

  return OK;
}

/*-------------------------------------------------------------------------*/
/* instruction decoders */

static void DecodeFixed(Word Index)
{
  if (ChkArgCnt(0, 0))
    *pCode++ = Index;
}

static void DecodeMOV(Word Index)
{
  UNUSED(Index);

  SetOpSize(0);
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg8 | MModShort | MModSFR | MModMem | MModAbs | MModPSW | MModSTBC, &DestResult))
    {
      case ModReg8:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm | MModReg8 | (DestResult.Val == AccReg8 ? (MModShort | MModSFR | MModAbs | MModMem | MModPSW) : 0), &SrcResult))
        {
          case ModImm:
            *pCode++ = 0xb8 | DestResult.Val;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModReg8:
            if (DestResult.Val == AccReg8)
              *pCode++ = 0xd0 | SrcResult.Val;
            else
            {
              *pCode++ = 0x24;
              *pCode++ = (DestResult.Val << 4) | SrcResult.Val;
            }
            break;
          case ModSFR:
            *pCode++ = 0x10;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModShort:
            *pCode++ = 0x20;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModAbs:
            if (SrcResult.AltBank)
              *pCode++ = 0x01;
            *pCode++ = 0x09;
            *pCode++ = 0xf0;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = 1[SrcResult.Vals];
            break;
          case ModMem:
            if (SrcResult.AltBank)
              *pCode++ = 0x01;
            if (SrcResult.Val == 0x16)
              *pCode++ = 0x58 | *SrcResult.Vals;
            else
            {
              *pCode++ = 0x00 | SrcResult.Val;
              *pCode++ = *SrcResult.Vals << 4;
              memcpy(pCode, SrcResult.Vals + 1, SrcResult.Cnt - 1);
              pCode += SrcResult.Cnt - 1;
            }
            break;
          case ModPSW:
            *pCode++ = 0x10;
            *pCode++ = SFR_PSW;
            break;
        }
        break;
      }

      case ModShort:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm | MModReg8 | MModShort, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x3a;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModReg8:
            if (SrcResult.Val != AccReg8) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x22;
              *pCode++ = *DestResult.Vals;
            }
            break;
          case ModShort:
            *pCode++ = 0x38;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
        }
        break;
      }

      case ModSFR:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm | MModReg8, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x2b;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModReg8:
            if (SrcResult.Val != AccReg8) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x12;
              *pCode++ = *DestResult.Vals;
            }
            break;
        }
        break;
      }

      case ModPSW:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm | MModReg8, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x2b;
            *pCode++ = SFR_PSW;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModReg8:
            if (SrcResult.Val != AccReg8) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x12;
              *pCode++ = SFR_PSW;
            }
            break;
        }
        break;
      }

      case ModSTBC:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x09;
            *pCode++ = 0xc0;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = ~(*SrcResult.Vals);
            break;
        }
        break;
      }

      /* only works against ACC - dump result first, since DecodeAdr
         destroys values */

      case ModMem:
        if (DestResult.AltBank)
          *pCode++ = 0x01;
        if (DestResult.Val == 0x16)
          *pCode++ = 0x50 | *DestResult.Vals;
        else
        {
          *pCode++ = 0x00 | DestResult.Val;
          *pCode++ = 0x80 | (*DestResult.Vals << 4);
          memcpy(pCode, DestResult.Vals + 1, DestResult.Cnt - 1);
          pCode += DestResult.Cnt - 1;
        }
        if (!ChkAcc(&ArgStr[2]))
          pCode = BAsmCode;
        break;

      case ModAbs:
        if (DestResult.AltBank)
          *pCode++ = 0x01;
        *pCode++ = 0x09;
        *pCode++ = 0xf1;
        *pCode++ = *DestResult.Vals;
        *pCode++ = 1[DestResult.Vals];
        if (!ChkAcc(&ArgStr[2]))
          pCode = BAsmCode;
        break;
    }
  }
}

static void DecodeXCH(Word Index)
{
  UNUSED(Index);

  SetOpSize(0);
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg8 | MModShort | MModSFR | MModMem, &DestResult))
    {
      case ModReg8:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg8 | ((DestResult.Val == AccReg8) ? (MModShort | MModSFR | MModMem) : 0), &SrcResult))
        {
          case ModReg8:
            if (DestResult.Val == AccReg8)
              *pCode++ = 0xd8 | SrcResult.Val;
            else if (SrcResult.Val == AccReg8)
              *pCode++ = 0xd8 | DestResult.Val;
            else
            {
              *pCode++ = 0x25;
              *pCode++ = (DestResult.Val << 4) | SrcResult.Val;
            }
            break;
          case ModShort:
            *pCode++ = 0x21;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModSFR:
            *pCode++ = 0x01;
            *pCode++ = 0x21;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModMem:
            if (SrcResult.AltBank)
              *pCode++ = 0x01;
            *pCode++ = SrcResult.Val;
            *pCode++ = (*SrcResult.Vals << 4) | 0x04;
            memcpy(pCode, SrcResult.Vals + 1, SrcResult.Cnt - 1);
            pCode += SrcResult.Cnt - 1;
            break;
        }
        break;
      }

      case ModShort:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg8 | MModShort, &SrcResult))
        {
          case ModReg8:
            if (SrcResult.Val != AccReg8) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x21;
              *pCode++ = *DestResult.Vals;
            }
            break;
          case ModShort:
            *pCode++ = 0x39;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
        }
        break;
      }

      case ModSFR:
        if (ChkAcc(&ArgStr[2]))
        {
          *pCode++ = 0x01;
          *pCode++ = 0x21;
          *pCode++ = *DestResult.Vals;
        }
        break;

      case ModMem:
        if (DestResult.AltBank)
          *pCode++ = 0x01;
        *pCode++ = DestResult.Val;
        *pCode++ = (*DestResult.Vals << 4) | 0x04;
        memcpy(pCode, DestResult.Vals + 1, DestResult.Cnt - 1);
        pCode += DestResult.Cnt - 1;
        if (!ChkAcc(&ArgStr[2]))
          pCode = BAsmCode;
        break;
    }
  }
}

static void DecodeMOVW(Word Index)
{
  UNUSED(Index);

  SetOpSize(1);
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg16 | MModSP | MModShort | MModSFR | MModMem, &DestResult))
    {
      case ModReg16:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg16 | MModImm | ((DestResult.Val == AccReg16) ? (MModSP | MModShort | MModSFR | MModMem) : 0), &SrcResult))
        {
          case ModReg16:
            *pCode++ = 0x24;
            *pCode++ = 0x08 | (DestResult.Val << 5) | (SrcResult.Val << 1);
            break;
          case ModImm:
            *pCode++ = 0x60 | (DestResult.Val << 1);
            *pCode++ = *SrcResult.Vals;
            *pCode++ = 1[SrcResult.Vals];
            break;
          case ModShort:
            *pCode++ = 0x1c;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModSFR:
            *pCode++ = 0x11;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModSP:
            *pCode++ = 0x11;
            *pCode++ = SFR_SP;
            break;
          case ModMem:
            if (ChkMem1(&SrcResult))
            {
              if (SrcResult.AltBank)
                *pCode++ = 0x01;
              *pCode++ = 0x05;
              *pCode++ = 0xe2 | (*SrcResult.Vals & 0x01);
            }
            break;
        }
        break;
      }

      case ModSP:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg16 | MModImm, &SrcResult))
        {
          case ModReg16:
            if (SrcResult.Val != AccReg16) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x13;
              *pCode++ = SFR_SP;
            }
            break;
          case ModImm:
            *pCode++ = 0x0b;
            *pCode++ = SFR_SP;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = 1[SrcResult.Vals];
            break;
        }
        break;
      }

      case ModShort:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg16 | MModImm, &SrcResult))
        {
          case ModReg16:
            if (SrcResult.Val != AccReg16) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x1a;
              *pCode++ = *DestResult.Vals;
            }
            break;
          case ModImm:
            *pCode++ = 0x0c;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = 1[SrcResult.Vals];
            break;
        }
        break;
      }

      case ModSFR:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg16 | MModImm, &SrcResult))
        {
          case ModReg16:
            if (SrcResult.Val != AccReg16) WrError(ErrNum_InvAddrMode);
            else
            {
              *pCode++ = 0x13;
              *pCode++ = *DestResult.Vals;
            }
            break;
          case ModImm:
            *pCode++ = 0x0b;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = 1[SrcResult.Vals];
            break;
        }
        break;
      }

      case ModMem:
        if (ChkMem1(&DestResult))
        {
          if (DestResult.AltBank)
            *pCode++ = 0x01;
          *pCode++ = 0x05;
          *pCode++ = 0xe6 | (*DestResult.Vals & 0x01);
          if (!ChkAcc(&ArgStr[2]))
            pCode = BAsmCode;
        }
        break;
    }
  }
}

static void DecodeALU(Word Index)
{
  SetOpSize(0);
  if (ChkArgCnt(2, 2))
  {
    tAdrResult DestResult;

    switch (DecodeAdr(&ArgStr[1], MModReg8 | MModShort | MModSFR, &DestResult))
    {
      case ModReg8:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModReg8 | ((DestResult.Val == AccReg8) ? (MModImm | MModShort | MModSFR | MModMem): 0), &SrcResult))
        {
          case ModReg8:
            *pCode++ = 0x88 | Index;
            *pCode++ = (DestResult.Val << 4) | SrcResult.Val;
            break;
          case ModImm:
            *pCode++ = 0xa8 | Index;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModShort:
            *pCode++ = 0x98 | Index;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModSFR:
            *pCode++ = 0x01;
            *pCode++ = 0x98 | Index;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModMem:
            if (SrcResult.AltBank)
              *pCode++ = 0x01;
            *pCode++ = 0x00 | SrcResult.Val;
            *pCode++ = 0x08 | (*SrcResult.Vals << 4) | Index;
            memcpy(pCode, SrcResult.Vals + 1, SrcResult.Cnt - 1);
            pCode += SrcResult.Cnt - 1;
            break;
        }
        break;
      }

      case ModShort:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm | MModShort, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x68 | Index;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
          case ModShort:
            *pCode++ = 0x78 | Index;
            *pCode++ = *SrcResult.Vals;
            *pCode++ = *DestResult.Vals;
            break;
        }
        break;
      }

      case ModSFR:
      {
        tAdrResult SrcResult;

        switch (DecodeAdr(&ArgStr[2], MModImm, &SrcResult))
        {
          case ModImm:
            *pCode++ = 0x01;
            *pCode++ = 0x68 | Index;
            *pCode++ = *DestResult.Vals;
            *pCode++ = *SrcResult.Vals;
            break;
        }
        break;
      }
    }
  }
}

static void DecodeALU16(Word Index)
{
  static Byte Vals[3] = { 0, 2, 7 };

  if (ChkArgCnt(2, 2))
  {
    SetOpSize(1);
    if (ChkAcc(&ArgStr[1]))
    {
      tAdrResult Result;

      switch (DecodeAdr(&ArgStr[2], MModImm | MModReg16 | MModShort | MModSFR, &Result))
      {
        case ModImm:
          *pCode++ = 0x2c | Index;
          *pCode++ = *Result.Vals;
          *pCode++ = 1[Result.Vals];
          break;
        case ModReg16:
          *pCode++ = 0x88 | Vals[Index - 1];
          *pCode++ = 0x08 | (Result.Val << 1);
          break;
        case ModShort:
          *pCode++ = 0x1c | Index;
          *pCode++ = *Result.Vals;
          break;
        case ModSFR:
          *pCode++ = 0x01;
          *pCode++ = 0x1c | Index;
          *pCode++ = *Result.Vals;
          break;
      }
    }
  }
}

static void DecodeMULDIV(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg8, &Result))
    {
      case ModReg8:
        *pCode++ = 0x05;
        *pCode++ = Index | Result.Val;
        break;
    }
  }
}

static void DecodeINCDEC(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg8 | MModShort, &Result))
    {
      case ModReg8:
        *pCode++ = 0xc0 | (Index << 3) | Result.Val;
        break;
      case ModShort:
        *pCode++ = 0x26 | Index;
        *pCode++ = *Result.Vals;
        break;
    }
  }
}

static void DecodeINCDECW(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg16 | MModSP, &Result))
    {
      case ModReg16:
        *pCode++ = 0x44 | (Index << 3) | Result.Val;
        break;
      case ModSP:
        *pCode++ = 0x05;
        *pCode++ = 0xc0 | Index;
        break;
    }
  }
}

static void DecodeShift8(Word Index)
{
  Boolean OK;
  Byte Shift;

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg8, &Result))
    {
      case ModReg8:
        Shift = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
        if (OK)
        {
          *pCode++ = 0x30 | Hi(Index);
          *pCode++ = Lo(Index) | (Shift << 3) | Result.Val;
        }
        break;
    }
  }
}

static void DecodeShift16(Word Index)
{
  Boolean OK;
  Byte Shift;

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg16, &Result))
    {
      case ModReg16:
        Shift = EvalStrIntExpression(&ArgStr[2], UInt3, &OK);
        if (OK)
        {
          *pCode++ = 0x30 | Hi(Index);
          *pCode++ = Lo(Index) | (Shift << 3) | (Result.Val << 1);
        }
        break;
    }
  }
}

static void DecodeShift4(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModMem, &Result))
    {
      case ModMem:
        if (ChkMem1(&Result))
        {
          if (Index)
            *pCode++ = 0x01;
          *pCode++ = 0x05;
          *pCode++ = 0x8c | ((*Result.Vals & 1) << 1);
        }
        break;
    }
  }
}

static void DecodePUSHPOP(Word Index)
{
  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModReg16 | MModPSW | MModSFR, &Result))
    {
      case ModReg16:
        *pCode++ = 0x34 | (Index << 3) | Result.Val;
        break;
      case ModPSW:
        *pCode++ = 0x48 | Index;
        break;
      case ModSFR:
        *pCode++ = Index ? 0x29 : 0x43;
        *pCode++ = *Result.Vals;
        break;
    }
  }
}

static void DecodeCALL(Word Index)
{
  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModAbs | MModReg16, &Result))
    {
      case ModAbs:
        *pCode++ = 0x28;
        *pCode++ = *Result.Vals;
        *pCode++ = 1[Result.Vals];
        break;
      case ModReg16:
        *pCode++ = 0x05;
        *pCode++ = 0x58 | (Result.Val << 1);
        break;
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
    tSymbolFlags Flags;

    AdrWord = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '!'), UInt12, &OK, &Flags);
    if (OK)
    {
      if (mFirstPassUnknown(Flags))
        AdrWord |= 0x800;
      if (AdrWord < 0x800) WrError(ErrNum_UnderRange);
      else
      {
        *pCode++ = 0x90 | (Hi(AdrWord) & 7);
        *pCode++ = Lo(AdrWord);
      }
    }
  }
}

static void DecodeCALLT(Word Index)
{
  Word AdrWord;
  Boolean OK;
  int ArgLen;
  tStrComp Arg;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    StrCompRefRight(&Arg, &ArgStr[1], 0); ArgLen = strlen(Arg.str.p_str);
    if ((*Arg.str.p_str != '[') || (Arg.str.p_str[ArgLen - 1] != ']')) WrError(ErrNum_InvAddrMode);
    else
    {
      tSymbolFlags Flags;

      StrCompIncRefLeft(&Arg, 1);
      StrCompShorten(&Arg, 1);
      AdrWord = EvalStrIntExpressionWithFlags(&Arg, UInt7, &OK, &Flags);
      if (OK)
      {
        if (mFirstPassUnknown(Flags))
        AdrWord = 0x40;
        if (ChkRange(AdrWord, 0x40, 0x7e))
        {
          if (AdrWord & 1) WrError(ErrNum_AddrMustBeEven);
          else
          {
            *pCode++ = 0xe0 | ((AdrWord - 0x40) >> 1);
          }
        }
      }
    }
  }
}

static void DecodeBR(Word Index)
{
  Boolean Rel;

  UNUSED(Index);

  if (ChkArgCnt(1, 1))
  {
    tAdrResult Result;
    tStrComp Arg;

    StrCompRefRight(&Arg, &ArgStr[1], 0);
    Rel = (*Arg.str.p_str == '$');
    if (Rel)
      StrCompIncRefLeft(&Arg, 1);
    switch (DecodeAdr(&Arg, MModAbs | MModReg16, &Result))
    {
      case ModAbs:
        if (Rel)
        {
          LongInt Addr = (((Word)1[Result.Vals]) << 8) | (*Result.Vals);

          Addr -= EProgCounter() + 2;
          if (!mSymbolQuestionable(Result.ValSymFlags) && ((Addr < -128) || (Addr > 127))) WrError(ErrNum_JmpDistTooBig);
          else
          {
            *pCode++ = 0x14;
            *pCode++ = Addr & 0xff;
          }
        }
        else
        {
          *pCode++ = 0x2c;
          *pCode++ = *Result.Vals;
          *pCode++ = 1[Result.Vals];
        }
        break;
      case ModReg16:
        *pCode++ = 0x05;
        *pCode++ = 0x48 | (Result.Val << 1);
        break;
    }
  }
}

static void DecodeBranch(Word Index)
{
  LongInt Addr;
  Boolean OK;

  if (ChkArgCnt(1, 1))
  {
    tSymbolFlags Flags;

    Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[1], !!(*ArgStr[1].str.p_str == '$'), UInt16, &OK, &Flags) - (EProgCounter() + 2);
    if (OK)
    {
      if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127))) WrError(ErrNum_JmpDistTooBig);
      else
      {
        *pCode++ = Index;
        *pCode++ = Addr & 0xff;
      }
    }
  }
}

static void DecodeDBNZ(Word Index)
{
  LongInt Addr;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    tAdrResult Result;

    switch (DecodeAdr(&ArgStr[1], MModShort | MModReg8, &Result))
    {
      case ModShort:
        *pCode++ = 0x3b;
        *pCode++ = *Result.Vals;
        break;
      case ModReg8:
        if ((Result.Val < 2) || (Result.Val > 3))
        {
          WrError(ErrNum_InvAddrMode);
          Result.Mode = ModNone;
        }
        else
          *pCode++ = 0x30 | Result.Val;
        break;
    }
    if (Result.Mode != ModNone)
    {
      tSymbolFlags Flags;

      Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], !!(*ArgStr[2].str.p_str == '$'), UInt16, &OK, &Flags) - (EProgCounter() + (pCode - BAsmCode) + 1);
      if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127)))
      {
        WrError(ErrNum_JmpDistTooBig);
        pCode = BAsmCode;
      }
      else
        *pCode++ = Addr & 0xff;
    }
  }
}

static void DecodeSEL(Word Index)
{
  Boolean OK;
  Byte Bank;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (as_strncasecmp(ArgStr[1].str.p_str, "RB", 2)) WrError(ErrNum_InvAddrMode);
  else
  {
    Bank = EvalStrIntExpressionOffs(&ArgStr[1], 2, UInt2, &OK);
    if (OK)
    {
      *pCode++ = 0x05;
      *pCode++ = 0xa8 | Bank;
    }
  }
}

static void DecodeBIT(Word Index)
{
  LongWord Result;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (DecodeBitAdr(&ArgStr[1], &Result))
    EnterIntSymbol(&LabPart, Result, SegNone, False);
}

static void DecodeMOV1(Word Index)
{
  LongWord Bit;
  int ArgPos;

  UNUSED(Index);

  if (ChkArgCnt(2, 2))
  {
    if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
      ArgPos = 2;
    else if (!as_strcasecmp(ArgStr[2].str.p_str, "CY"))
      ArgPos = 1;
    else
    {
      WrError(ErrNum_InvAddrMode);
      return;
    }
    if (DecodeBitAdr(&ArgStr[ArgPos], &Bit))
    {
      *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
      *pCode++ = ((2 - ArgPos) << 4) | ((Bit >> 8) & 0xff);
      if (Bit & 0x1000000)
        *pCode++ = Bit & 0xff;
    }
  }
}

static void DecodeANDOR1(Word Index)
{
  LongWord Bit;

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else
  {
    tStrComp *pArg = &ArgStr[2], BitArg;

    if (*pArg->str.p_str == '/')
    {
      StrCompRefRight(&BitArg, &ArgStr[2], 1);
      pArg = &BitArg;
      Index |= 0x10;
    }
    if (DecodeBitAdr(pArg, &Bit))
    {
      *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
      *pCode++ = Index  | ((Bit >> 8) & 0xff);
      if (Bit & 0x1000000)
        *pCode++ = Bit & 0xff;
    }
  }
}

static void DecodeXOR1(Word Index)
{
  LongWord Bit;

  UNUSED(Index);

  if (!ChkArgCnt(2, 2));
  else if (as_strcasecmp(ArgStr[1].str.p_str, "CY")) WrError(ErrNum_InvAddrMode);
  else
  {
    if (DecodeBitAdr(&ArgStr[2], &Bit))
    {
      *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
      *pCode++ = 0x60 | ((Bit >> 8) & 0xff);
      if (Bit & 0x1000000)
        *pCode++ = Bit & 0xff;
    }
  }
}

static void DecodeBit1(Word Index)
{
  LongWord Bit;

  UNUSED(Index);

  if (!ChkArgCnt(1, 1));
  else if (!as_strcasecmp(ArgStr[1].str.p_str, "CY"))
  {
    *pCode++ = 0x40 | (9 - (Index >> 4));
  }
  else if (DecodeBitAdr(&ArgStr[1], &Bit))
  {
    if ((Index >= 0x80) && ((Bit & 0xfffff800) == 0x01080000))
    {
      *pCode++ = (0x130 - Index) | ((Bit >> 8) & 7);
      *pCode++ = Bit & 0xff;
    }
    else
    {
      *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
      *pCode++ = Index | ((Bit >> 8) & 0xff);
      if (Bit & 0x1000000)
        *pCode++ = Bit & 0xff;
    }
  }
}

static void DecodeBrBit(Word Index)
{
  LongWord Bit;

  LongInt Addr;
  Boolean OK;

  UNUSED(Index);

  if (ChkArgCnt(2, 2)
   && DecodeBitAdr(&ArgStr[1], &Bit))
  {
    tSymbolFlags Flags;

    if ((Bit & 0xfffff800) == 0x01080000)
    {
      if (Index == 0x80)
      {
        *pCode++ = 0x70 | ((Bit >> 8) & 7);
	*pCode++ = Bit & 0xff;
      }
      else
      {
        *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
	*pCode++ = (0x130 - Index) | ((Bit >> 8) & 0xff);
	if (Bit & 0x1000000)
          *pCode++ = Bit & 0xff;
      }
    }
    else
    {
      *pCode++ = 0x00 | ((Bit >> 16) & 0xff);
      *pCode++ = (0x130 - Index) | ((Bit >> 8) & 0xff);
      if (Bit & 0x1000000)
        *pCode++ = Bit & 0xff;
    }

    Addr = EvalStrIntExpressionOffsWithFlags(&ArgStr[2], !!(*ArgStr[2].str.p_str == '$'), UInt16, &OK, &Flags) - (EProgCounter() + (pCode - BAsmCode) + 1);
    if (!mSymbolQuestionable(Flags) && ((Addr < -128) || (Addr > 127)))
    {
      WrError(ErrNum_JmpDistTooBig);
      pCode = BAsmCode;
    }
    else
      *pCode++ = Addr & 0xff;
  }
}

/*-------------------------------------------------------------------------*/
/* dynamic code table handling */

static void AddFixed(const char *NName, Word NCode)
{
  AddInstTable(InstTable, NName, NCode, DecodeFixed);
}

static void InitFields(void)
{
  InstTable = CreateInstTable(201);

  AddInstTable(InstTable, "MOV", 0, DecodeMOV);
  AddInstTable(InstTable, "XCH", 0, DecodeXCH);
  AddInstTable(InstTable, "MOVW", 0, DecodeMOVW);

  AddInstTable(InstTable, "ADD"  , 0, DecodeALU);
  AddInstTable(InstTable, "ADDC" , 1, DecodeALU);
  AddInstTable(InstTable, "SUB"  , 2, DecodeALU);
  AddInstTable(InstTable, "SUBC" , 3, DecodeALU);
  AddInstTable(InstTable, "AND"  , 4, DecodeALU);
  AddInstTable(InstTable, "OR"   , 6, DecodeALU);
  AddInstTable(InstTable, "XOR"  , 5, DecodeALU);
  AddInstTable(InstTable, "CMP"  , 7, DecodeALU);

  AddInstTable(InstTable, "ADDW", 1, DecodeALU16);
  AddInstTable(InstTable, "SUBW", 2, DecodeALU16);
  AddInstTable(InstTable, "CMPW", 3, DecodeALU16);

  AddInstTable(InstTable, "MULU" , 0x08, DecodeMULDIV);
  AddInstTable(InstTable, "DIVUW", 0x18, DecodeMULDIV);

  AddInstTable(InstTable, "INC", 0, DecodeINCDEC);
  AddInstTable(InstTable, "DEC", 1, DecodeINCDEC);

  AddInstTable(InstTable, "INCW", 0, DecodeINCDECW);
  AddInstTable(InstTable, "DECW", 1, DecodeINCDECW);

  AddInstTable(InstTable, "ROR"  , 0x040, DecodeShift8);
  AddInstTable(InstTable, "ROL"  , 0x140, DecodeShift8);
  AddInstTable(InstTable, "RORC" , 0x000, DecodeShift8);
  AddInstTable(InstTable, "ROLC" , 0x100, DecodeShift8);
  AddInstTable(InstTable, "SHR"  , 0x080, DecodeShift8);
  AddInstTable(InstTable, "SHL"  , 0x180, DecodeShift8);

  AddInstTable(InstTable, "SHRW" , 0x0c0, DecodeShift16);
  AddInstTable(InstTable, "SHLW" , 0x1c0, DecodeShift16);

  AddInstTable(InstTable, "ROR4" , 0, DecodeShift4);
  AddInstTable(InstTable, "ROL4" , 1, DecodeShift4);

  AddInstTable(InstTable, "POP"  , 0, DecodePUSHPOP);
  AddInstTable(InstTable, "PUSH" , 1, DecodePUSHPOP);

  AddInstTable(InstTable, "CALL" , 0, DecodeCALL);
  AddInstTable(InstTable, "CALLF", 0, DecodeCALLF);
  AddInstTable(InstTable, "CALLT", 0, DecodeCALLT);

  AddInstTable(InstTable, "BR"   , 0, DecodeBR);

  AddInstTable(InstTable, "BC"   , 0x83, DecodeBranch);
  AddInstTable(InstTable, "BL"   , 0x83, DecodeBranch);
  AddInstTable(InstTable, "BNC"  , 0x82, DecodeBranch);
  AddInstTable(InstTable, "BNL"  , 0x82, DecodeBranch);
  AddInstTable(InstTable, "BZ"   , 0x81, DecodeBranch);
  AddInstTable(InstTable, "BE"   , 0x81, DecodeBranch);
  AddInstTable(InstTable, "BNZ"  , 0x80, DecodeBranch);
  AddInstTable(InstTable, "BNE"  , 0x80, DecodeBranch);

  AddInstTable(InstTable, "DBNZ" , 0, DecodeDBNZ);

  AddInstTable(InstTable, "SEL", 0, DecodeSEL);

  AddInstTable(InstTable, "MOV1", 0, DecodeMOV1);
  AddInstTable(InstTable, "AND1", 0x20, DecodeANDOR1);
  AddInstTable(InstTable, "OR1" , 0x40, DecodeANDOR1);
  AddInstTable(InstTable, "XOR1" , 0, DecodeXOR1);

  AddInstTable(InstTable, "SET1", 0x80, DecodeBit1);
  AddInstTable(InstTable, "CLR1", 0x90, DecodeBit1);
  AddInstTable(InstTable, "NOT1", 0x70, DecodeBit1);

  AddInstTable(InstTable, "BT"    , 0x80, DecodeBrBit);
  AddInstTable(InstTable, "BF"    , 0x90, DecodeBrBit);
  AddInstTable(InstTable, "BTCLR" , 0x60, DecodeBrBit);

  AddFixed("NOP",   0x00);
  AddFixed("DI",    0x4a);
  AddFixed("EI",    0x4b);
  AddFixed("BRK",   0x5e);
  AddFixed("RET",   0x56);
  AddFixed("RETI",  0x57);
  AddFixed("RETB",  0x5f);
  AddFixed("ADJBA", 0x0e);
  AddFixed("ADJBS", 0x0f);

  AddInstTable(InstTable, "BIT", 0, DecodeBIT);
}

static void DeinitFields(void)
{
  DestroyInstTable(InstTable);
}

/*-------------------------------------------------------------------------*/
/* interface to common layer */

static void MakeCode_78K2(void)
{
  CodeLen = 0; DontPrint = False; OpSize = -1;

  /* zu ignorierendes */

  if (Memo("")) return;

  /* Pseudoanweisungen */

  if (DecodeIntelPseudo(False)) return;

  pCode = BAsmCode;
  if (!LookupInstTable(InstTable, OpPart.str.p_str))
    WrStrErrorPos(ErrNum_UnknownInstruction, &OpPart);
  else
    CodeLen = pCode - BAsmCode;
}

static void InitCode_78K2(void)
{
  Reg_PM6 = 0;
  Reg_P6  = 0;
}

static Boolean IsDef_78K2(void)
{
  return Memo("BIT");
}

static void SwitchFrom_78K2(void)
{
  DeinitFields();
}

static void SwitchTo_78K2(void)
{
  static const ASSUMERec ASSUME78K2s[] =
  {
    {"P6"  , &Reg_P6  , 0,  0xf,  0x10, NULL},
    {"PM6" , &Reg_PM6 , 0,  0xf,  0x10, NULL}
  };
  const TFamilyDescr *pDescr;

  pDescr = FindFamilyByName("78K2");

  TurnWords = False;
  SetIntConstMode(eIntConstModeIntel);

  PCSymbol = "PC";
  HeaderID = pDescr->Id;
  NOPCode = 0x00;
  DivideChars = ",";
  HasAttrs = False;

  ValidSegs = 1 << SegCode;
  Grans[SegCode] = 1; ListGrans[SegCode] = 1; SegInits[SegCode] = 0;
  SegLimits[SegCode] = 0xfffff;

  pASSUMERecs = ASSUME78K2s;
  ASSUMERecCnt = sizeof(ASSUME78K2s) / sizeof(ASSUME78K2s[0]);

  MakeCode = MakeCode_78K2; IsDef = IsDef_78K2;
  SwitchFrom = SwitchFrom_78K2; InitFields();
}

void code78k2_init(void)
{
  CPU78214 = AddCPU("78214", SwitchTo_78K2);

  AddInitPassProc(InitCode_78K2);
}
